/*
 * Copyright 1991-1998 by Open Software Foundation, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 * 
 */
/*
 * MkLinux
 */

#include <mesh.h>
#if	NMESH > 0
#include <ppc/proc_reg.h> /* For isync */
#include <mach_debug.h>
#include <kern/spl.h>
#include <mach/std_types.h>
#include <types.h>
#include <chips/busses.h>
#include <scsi/compat_30.h>
#include <scsi/scsi.h>
#include <scsi/scsi2.h>
#include <scsi/scsi_defs.h>
#include <ppc/POWERMAC/mesh.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/dbdma.h>
#include <ppc/POWERMAC/device_tree.h>

/* Used only for debugging asserts */
#include <ppc/mem.h>
#include <ppc/pmap.h>
#include <ppc/POWERMAC/interrupts.h>
#include <ppc/POWERMAC/powermac_gestalt.h>

#define	u_min(a, b)	(((a) < (b)) ? (a) : (b))

#define	MESH_SEL_TIMEOUT	25

/*
 * State kept for each active SCSI device.
 */

#define	MESH_MAX_SG_SEGS	128

struct sg_segment {
	unsigned long	sg_offset;
	unsigned long	sg_count;
	vm_offset_t	sg_physaddr;
	unsigned char	*sg_vmaddr;
};

struct mesh_state {
	char	s_cdb[16];	/* Command block */
	int	s_cdbsize;
	int	s_id;		/* The Target ID */
	int	s_error;	/* errno to pass back to device driver */
	unsigned char	*s_dma_area;	/* DMA Area */
	int	s_xfering;	/* amount to transfer in this chunk */
	int	s_total_count;	/* total bytes to transfer */
	int	s_residual;	/* total remaining amount of data to transfer */
	long	s_built_count;	/* Current byte count to transfer with DBDMAS */
	int	s_saved_residual;	/* Saved residual for Save Pointers Msg */
	int	s_saved_s_index;	/* Saved S/G Index for Save Pointers Msg */
	int	s_flags;		/* see below */
	int	s_segment_count;	/* Number of S/G segments */
	struct	sg_segment	s_list[MESH_MAX_SG_SEGS];
	int	s_index;	/* Index into s_list */
	int	s_dev_flags;	/* various device flags */
	int	s_msg_sent;	/* What message was sent to device */
	int	s_msglen;	/* number of message bytes to read */
	int	s_msgcnt;	/* number of message bytes received/sent */
	u_char	s_msg[16];	/* buffer for multibyte messages */
};

typedef struct mesh_state *mesh_state_t;

/* state flags */
#define STATE_DISCONNECTED	0x001	/* currently disconnected from bus */
#define STATE_DMA_RUNNING	0x002	/* data DMA started */
#define STATE_DMA_IN		0x004	/* reading from SCSI device */
#define STATE_DMA_OUT		0x010	/* writing to SCSI device */
#define	STATE_CHAINED_IO	0x020	/* request is a S/G one */
#define STATE_PARITY_ERR	0x080	/* parity error seen */
#define	STATE_FIFO_XFER		0x200	/* request has to use FIFO */
#define	STATE_DONE		0x400	/* Request is finished */

/* device flags */
#define	DEV_NO_DISCONNECT	0x001	/* Do not preform disconnect */
#define	DEV_RUN_SYNC		0x002	/* Run synchronous transfers */

#define	MESH_NCMD	8

#define MESH_DBDMA_BUFFERS	MESH_MAX_SG_SEGS

/*
 * State kept for each active SCSI host interface (mesh).
 */
struct mesh_softc {
	struct watchdog_t	m_wd;
	queue_head_t		m_target_queue;	/* Target Queue */
	scsi_softc_t		*m_sc;		/* Higher level SCSI driver */
	mesh_regmap_t		m_regs;	/* chip address */
	decl_simple_lock_data(,lock);
	decl_simple_lock_data(,queue_lock);
	int		m_unit;		/* Unit number */
	int		m_sc_id;	/* SCSI ID of this interface */
	int		m_state;	/* Current state */
	mesh_state_t	m_current;	/* Current target */
	int		m_max_transfer;
	int		m_xfering;	/* DMA transfer count */
	target_info_t	*m_cmd[MESH_NCMD];/* active command indexed by SCSI ID */
	dbdma_command_t	*m_dbdma_buffer;	/* DBDMA Buffers */

	struct mesh_state	m_st[MESH_NCMD];/* state info for each active command */
	int		m_interrupt_number;	/* our interrupt vector */
	dbdma_regmap_t	*m_dbdma_channel;	/* Our DBDMA channel */
	char		*m_cmd_areas;
};

typedef struct mesh_softc *mesh_softc_t;

struct mesh_softc	mesh_softc_data[NMESH];
mesh_softc_t		mesh_softc[NMESH];

boolean_t	mesh_probe_dynamically = FALSE;
boolean_t	mesh_polling = FALSE;

/*
 * Various prototypes..
 */


void mesh_start_new_request(mesh_softc_t);
void mesh_start_request(mesh_softc_t, mesh_regmap_t, mesh_state_t);
void mesh_start_selection(mesh_softc_t, mesh_regmap_t, mesh_state_t);
void mesh_send_identity(mesh_softc_t, mesh_regmap_t, mesh_state_t);
void mesh_send_command(mesh_softc_t, mesh_regmap_t, mesh_state_t);
void mesh_setup_for_msgin(mesh_softc_t, mesh_regmap_t, mesh_state_t);
void mesh_start_data_xfer(mesh_softc_t, mesh_regmap_t, mesh_state_t);
void mesh_end_transfer(mesh_softc_t, mesh_regmap_t, mesh_state_t);
void mesh_get_status(mesh_softc_t, mesh_regmap_t, mesh_state_t);
void mesh_intr(int , void *);
void mesh_end(mesh_softc_t, mesh_regmap_t, mesh_state_t);
void mesh_process_msgin(mesh_softc_t, mesh_regmap_t, mesh_state_t);
void mesh_disconnect(mesh_softc_t mesh, mesh_regmap_t, mesh_state_t);
void mesh_prep_for_status(mesh_softc_t mesh, mesh_regmap_t, mesh_state_t);
int mesh_reset_scsibus(mesh_softc_t	mesh);
void mesh_construct_entry(mesh_state_t, boolean_t, vm_offset_t, long, long);
void mesh_build_sglist(mesh_softc_t, mesh_state_t, target_info_t *);
void mesh_setup_chip(mesh_softc_t, mesh_regmap_t, boolean_t);
int  mesh_probe(caddr_t, void *);
void mesh_alloc_cmdptr(mesh_softc_t, target_info_t *);
void mesh_probe_for_targets(mesh_softc_t, mesh_regmap_t);
int mesh_go(target_info_t *, int, int, boolean_t);
boolean_t mesh_probe_target(target_info_t *, io_req_t);
void mesh_get_io_segment(mesh_softc_t, mesh_state_t,
		struct sg_segment **, unsigned long *,
		vm_offset_t *, unsigned long *);
void	mesh_panic(char *str);
void	mesh_send_msg(mesh_softc_t, mesh_regmap_t, mesh_state_t);
long	mesh_build_dbdmas(mesh_softc_t mesh, mesh_state_t st);

/* SCSI States - */

#define	MESH_STATE_SELECTION		0
#define	MESH_STATE_SEND_IDENTITY	1
#define	MESH_STATE_COMMAND		2
#define	MESH_STATE_DATA			3
#define	MESH_STATE_STATUS		4
#define	MESH_STATE_COMPLETE		5
#define	MESH_STATE_MSGIN		6
#define	MESH_STATE_MSGOUT		7
#define	MESH_STATE_BUS_FREE		8
#define	MESH_STATE_WAIT_RESEL		9
#define	MESH_STATE_FLOAT		10
#define	MESH_STATE_PREP_STATUS		11
#define	MESH_STATE_PREP_MSGIN		12

char *mesh_state_names[] = {
"selection", "send id", "command", "data", "status", "complete",
"msgin", "msgout", "bus free", "wait resel", "float", "prep status",
"prep msgin"
};

void (*mesh_states[])(mesh_softc_t, mesh_regmap_t, mesh_state_t) = {
	mesh_start_selection,	/* MESH_STATE_SELECTION */
	mesh_send_identity,	/* MESH_STATE_SEND_IDENTITY */
	mesh_send_command,	/* MESH_STATE_COMMAND */
	mesh_start_data_xfer,	/* MESH_STATE_DATA */
	mesh_get_status,	/* MESH_STATE_STATUS */
	mesh_end,		/* MESH_STATE_COMPLETE */
	mesh_process_msgin,	/* MESH_STATE_MSGIN */
	NULL,			/* MESH_STATE_MSGOUT */
	NULL,			/* MESH_STATE_BUS_FREE */
	NULL,			/* MESH_STATE_WAIT_RESEL */
	NULL,			/* MESH_STATE_FLOAT */
	mesh_prep_for_status,	/* MESH_STATE_PREP_STATUS */
	mesh_setup_for_msgin,	/* MESH_STATE_PREP_MSGIN */
};


#if DEBUG 
#define MESH_DEBUG 1
#endif

#define MESH_DEBUG 1

#if MESH_DEBUG
int	mesh_debug = 1;
int	mesh_trace_debug = 0;

static void	mesh_trace_log(mesh_softc_t mesh,
			char *msg, u_long a1, u_long a2, u_long a3,
			u_long a4, u_long a5);

#define	MESH_TRACE(msg)	mesh_trace_log(mesh, msg, 0, 0, 0, 0, 0)
#define	MESH_TRACE1(msg, a)	mesh_trace_log(mesh, msg, (u_long) (a), 0, 0, 0, 0)
#define	MESH_TRACE2(msg, a,b)	mesh_trace_log(mesh, msg, (u_long) (a), (u_long) (b), 0, 0, 0)
#define	MESH_TRACE3(msg, a,b,c)	mesh_trace_log(mesh, msg, (u_long) (a), (u_long) (b), (u_long) (c), 0, 0)
#define	MESH_TRACE4(msg, a,b,c,d) \
	mesh_trace_log(mesh, msg, (u_long) (a), (u_long) (b), (u_long) (c), (u_long)(d), 0)
#define	MESH_TRACE5(msg, a,b,c,d, e) \
	mesh_trace_log(mesh, msg, (u_long) (a), (u_long) (b), (u_long) (c), (u_long)(d), (u_long)(e))

#else

#define	MESH_TRACE(msg)	/* ; */
#define	MESH_TRACE1(msg, a)	/* ; */
#define	MESH_TRACE2(msg, a,b)	/* ; */
#define	MESH_TRACE3(msg, a,b,c)	/* ; */
#define	MESH_TRACE4(msg, a,b,c,d)	/* ; */
#define	MESH_TRACE5(msg, a,b,c,d,e)	/* ; */
#endif

/* Various MACH driver variables.. */
caddr_t	mesh_std[NMESH] = { 0 };
struct	bus_device *mesh_dinfo[NMESH*8];
struct	bus_ctlr *mesh_minfo[NMESH];
struct	bus_driver mesh_driver = 
        { mesh_probe, scsi_slave, scsi_attach, (int(*)(void))mesh_go,
	  mesh_std, "rz", mesh_dinfo, "mesh", mesh_minfo, BUS_INTR_B4_PROBE};



void
mesh_set_flags(unsigned char *env, int flags)
{
	register int unit, i;

	if (strcmp(env, "all") == 0) {
		for (unit = 0; unit < NMESH; unit++)
			for (i = 0; i < 7; i++)
				mesh_softc_data[unit].m_st[i].s_dev_flags |= flags;
	} else {
		while (*env) {
			if (*env >= '0' && *env < '7') {
				for (unit = 0; unit < NMESH; unit++)
					mesh_softc_data[unit].m_st[*env - '0'].s_dev_flags |= flags;
			}
			env++;
		}
	}
}

void
mesh_getconfig(void)
{
	register int unit, i;
	unsigned char	*env;
	extern unsigned char *getenv(char *);

	if ((env = getenv("scsi_nodisconnect")) != NULL)
		mesh_set_flags(env, DEV_NO_DISCONNECT);

	if (powermac_info.class == POWERMAC_CLASS_PCI) {
		if ((env = getenv("scsi_sync")) != NULL)
			mesh_set_flags(env, DEV_RUN_SYNC);
	}
}

				
int
mesh_probe(
	caddr_t		reg,
	void		*ptr)
{
	struct bus_ctlr	*ui	 = (struct bus_ctlr *) ptr;
	int		unit	= ui->unit;
	/* TODO/HACK - better handle autoconf */
	register mesh_softc_t mesh = &mesh_softc_data[/*unit*/ 0];
	target_info_t	*tgt;
	scsi_softc_t	*sc;
	int id, s, i, a;
	device_node_t	*meshdev;

	meshdev = find_devices("mesh");

	if (meshdev == NULL) {
		return 0;
	}

	mesh->m_regs = (mesh_regmap_t) POWERMAC_IO(meshdev->addrs[0].address);
	mesh->m_interrupt_number = meshdev->intrs[0];
	mesh->m_dbdma_channel = (dbdma_regmap_t *)
				POWERMAC_IO(meshdev->addrs[1].address);

	simple_lock_init(&mesh->lock, ETAP_IO_CHIP);
	simple_lock_init(&mesh->queue_lock, ETAP_IO_CHIP);


#if 0
	if (mesh->m_regs->r_meshid != 0xE2) {
		printf("PCI: No MESH present (ID %x)\n", mesh->m_regs->r_meshid);
		return 0;
	}
#endif

	mesh->m_unit = unit;
	mesh_softc[0] = mesh;	/* TODO/HACK workout multiple controllers  */
				/* Requires better auto configuration */

	queue_init(&mesh->m_target_queue);

	sc = scsi_master_alloc(unit, (char *) mesh);
	mesh->m_sc = sc;

	sc->go = (void *) mesh_go;
	sc->watchdog = scsi_watchdog;
	sc->probe = mesh_probe_target;
	mesh->m_wd.reset = ((void (*)(struct watchdog_t *))mesh_reset_scsibus);


#if 0
	mesh->m_max_transfer = (63*1024);
	sc->max_dma_data = mesh->m_max_transfer;
#else
	mesh->m_max_transfer = -1;
	sc->max_dma_data = -1;
#endif
	sc->max_dma_segs = MESH_MAX_SG_SEGS / 2;

	mesh->m_current = NULL;
	mesh->m_dbdma_buffer = dbdma_alloc(MESH_DBDMA_BUFFERS+2);
	mesh->m_cmd_areas = (char *)kalloc(MESH_NCMD * 256);

	/* preserve our ID for now */
	mesh->m_sc_id = 7;/*regs->resh_cnfg1 & MESH_CNFG1_MY_BUS_ID; eieio();*/

	s = splbio();
	simple_lock(&mesh->lock);

	mesh_setup_chip(mesh, mesh->m_regs, TRUE);

	id = mesh->m_sc_id;
	sc->initiator_id = id;

	printf("%s%d: MESH SCSI ID %d", ui->name, unit, id); 

	tgt = scsi_slave_alloc(sc->masterno, sc->initiator_id, (char *) mesh);
	mesh_alloc_cmdptr(mesh, tgt);
	sccpu_new_initiator(tgt, tgt);

	simple_unlock(&mesh->lock);
	splx(s);
	mesh_probe_for_targets(mesh, mesh->m_regs);
	printf("\n");
	mesh_getconfig();
	pmac_register_ofint(mesh->m_interrupt_number, SPLBIO, mesh_intr);
	return 1;
}

boolean_t
mesh_probe_target(
	target_info_t 		*tgt,
	io_req_t		ior)
{
	/* TODO/HACK properly handle multiple SCSI controllers */
	mesh_softc_t     mesh;

	simple_lock(&mesh->lock);

	mesh = mesh_softc[ 0 /*tgt->masterno*/];

	if (scsi_inquiry(tgt, SCSI_INQ_STD_DATA) == SCSI_RET_DEVICE_DOWN)
		return FALSE;

	tgt->flags |= TGT_ALIVE|TGT_CHAINED_IO_SUPPORT;

	simple_unlock(&mesh->lock);
	return TRUE;
}

void
mesh_alloc_cmdptr(mesh_softc_t mesh, target_info_t *tgt)
{
	tgt->cmd_ptr = &mesh->m_cmd_areas[tgt->target_id*256];
	tgt->dma_ptr = 0;
}

void
mesh_probe_for_targets(mesh_softc_t mesh, mesh_regmap_t regs)
{
	int	target_id, did_banner = 0;
	scsi_softc_t		*sc = mesh->m_sc;
	mesh_state_t		state;
	spl_t			s;


    	if (mesh_probe_dynamically) {
		printf("mesh%d: will probe targets on demand\n",
			mesh->m_unit);
		return;
	}
	s = splbio();
	simple_lock(&mesh->lock);

	mesh_polling = TRUE;
	/*
	 * For all possible targets, see if there is one and allocate
	 * a descriptor for it if it is there.
	 */
	for (target_id = 0; target_id < 8; target_id++) {

		/* except of course ourselves */
		if (target_id == sc->initiator_id)
			continue;

		state = &mesh->m_st[target_id];

		state->s_id = target_id;

		bzero(state->s_cdb, 6);
		state->s_cdb[0] = SCSI_CMD_TEST_UNIT_READY;
		state->s_cdbsize = 6;
		state->s_residual = 0;
		state->s_total_count = 0;
		mesh->m_current = state;
		
		regs->r_interrupt = 0xff; eieio();

		mesh_start_request(mesh, regs, state);

		simple_unlock(&mesh->lock);
		while ((state->s_flags & STATE_DONE) == 0) {
			do {
				eieio();
			} while (regs->r_interrupt == 0);
			mesh_intr(mesh->m_interrupt_number, NULL);
		}

		simple_lock(&mesh->lock);
		if (state->s_error == SCSI_RET_DEVICE_DOWN)
			continue;

		printf(",%s%d", did_banner++ ? " " : " target(s) at ",
					target_id);

		{
			register target_info_t	*tgt;
			tgt = scsi_slave_alloc(sc->masterno, target_id, (char *)mesh);
			mesh_alloc_cmdptr(mesh, tgt);
			tgt->flags |= TGT_CHAINED_IO_SUPPORT;
		}
	}

	regs->r_interrupt = 0xff; eieio();	/* Clear out interrupts */

	mesh_polling = FALSE;
	simple_unlock(&mesh->lock);
	splx(s);

} 

/*
 * Start activity on a SCSI device.
 * We maintain information on each device separately since devices can
 * connect/disconnect during an operation.
 */

int
mesh_go(
	target_info_t		*tgt,
	int			cmd_count,
	int			in_count,
	boolean_t		cmd_only)
{
	mesh_softc_t	mesh;
	mesh_state_t	state;
	int s;

	mesh = (mesh_softc_t) tgt->hw_state;

	if (mesh->m_cmd[tgt->target_id]) 
		panic("MESH SCSI Bus %d: Queueing Problem: Target %d busy at start\n", 
			mesh->m_unit, tgt->target_id);

	tgt->transient_state.cmd_count = cmd_count; /* keep it here */

	state = &mesh->m_st[tgt->target_id];

	MESH_TRACE2("go target %d, cmd %x",
			state->s_id, tgt->cmd_ptr[0]);

	/*
	 * Setup target state
	 */
	tgt->done = SCSI_RET_IN_PROGRESS;
	state->s_flags		= 0;
	state->s_total_count	= 0;
	state->s_saved_residual	= 0;
	state->s_index		= 0;
	state->s_segment_count	= 0;
	state->s_msgcnt		= 0;
	state->s_built_count	= 0;

	if ((state->s_dev_flags & DEV_RUN_SYNC) == 0)
		tgt->flags |= TGT_DID_SYNCH;	/* Prevent Sync Transfers*/

	/* 
 	 * WHY doesn't the MACH SCSI drivers tell
	 * the SCSI controllers what the I/O direction
	 * is supposed to be?!? ARGHH..
	 */

	switch (tgt->cur_cmd) {
	case SCSI_CMD_INQUIRY:
		if ((tgt->flags & TGT_DID_SYNCH) == 0) {
			tgt->flags |= TGT_TRY_SYNCH;
			state->s_flags |= STATE_DMA_IN;
			break;
		} 
		/* Fall thru.. */
	case SCSI_CMD_REQUEST_SENSE:
	case SCSI_CMD_MODE_SENSE:
	case SCSI_CMD_RECEIVE_DIAG_RESULTS:
	case SCSI_CMD_READ_CAPACITY:
	case SCSI_CMD_READ_BLOCK_LIMITS:
	case SCSI_CMD_READ_TOC:
	case SCSI_CMD_READ_SUBCH:
	case SCSI_CMD_READ_HEADER:
	case 0xc4:	/* despised: SCSI_CMD_DEC_PLAYBACK_STATUS */
	case 0xdd:	/* despised: SCSI_CMD_NEC_READ_SUBCH_Q */
	case 0xde:	/* despised: SCSI_CMD_NEC_READ_TOC */
	case SCSI_CMD_READ:
	case SCSI_CMD_LONG_READ:
		state->s_flags |= STATE_DMA_IN;
		break;

	case SCSI_CMD_WRITE:
	case SCSI_CMD_LONG_WRITE:
		state->s_flags |= STATE_DMA_OUT;
		break;

	case SCSI_CMD_MODE_SELECT:
	case SCSI_CMD_REASSIGN_BLOCKS:
	case SCSI_CMD_FORMAT_UNIT:
	case 0xc9: /* vendor-spec: SCSI_CMD_DEC_PLAYBACK_CONTROL */
		tgt->transient_state.cmd_count = sizeof_scsi_command(tgt->cur_cmd);
		tgt->transient_state.out_count =
			cmd_count - tgt->transient_state.cmd_count;

		state->s_flags |= STATE_DMA_OUT;
		break;

	case SCSI_CMD_TEST_UNIT_READY:
		/*
		 * Do the synch negotiation here, unless prohibited
		 * or done already
		 */

		if ((tgt->flags & TGT_DID_SYNCH) == 0) 
			tgt->flags |= TGT_TRY_SYNCH;
		break;

	 default:
		break;
	}

	tgt->transient_state.in_count = in_count;

	/* Figure out where to DMA to/from.. 
	  (Again, WHY?!? does MACH do this?!?)
	*/

	switch (tgt->cur_cmd) {
	case	SCSI_CMD_READ:
	case	SCSI_CMD_LONG_READ:
		state->s_total_count = in_count;
		goto check_chain;

	case	SCSI_CMD_WRITE:
	case	SCSI_CMD_LONG_WRITE:
		tgt->transient_state.out_count = tgt->ior->io_count;
		state->s_total_count = tgt->ior->io_count; 
		
check_chain:
		if (tgt->ior->io_op & IO_CHAINED) 
			state->s_flags |= STATE_CHAINED_IO;
		else
			state->s_dma_area = (u_char *) tgt->ior->io_data;

		break;

	case	SCSI_CMD_MODE_SELECT:
	case	SCSI_CMD_REASSIGN_BLOCKS:
	case	SCSI_CMD_FORMAT_UNIT:
	case	0xc9: /* vendor-spec: SCSI_CMD_DEC_PLAYBACK_CONTROL */
		state->s_total_count = tgt->transient_state.out_count;
		state->s_dma_area = (u_char *) tgt->cmd_ptr + sizeof(scsi_command_group_0);
		break;

	case SCSI_CMD_INQUIRY:
	case SCSI_CMD_REQUEST_SENSE:
	case SCSI_CMD_MODE_SENSE:
	case SCSI_CMD_RECEIVE_DIAG_RESULTS:
 	case SCSI_CMD_READ_CAPACITY:
	case SCSI_CMD_READ_BLOCK_LIMITS:
	case SCSI_CMD_READ_TOC:
	case SCSI_CMD_READ_SUBCH:
	case SCSI_CMD_READ_HEADER:
		state->s_total_count = tgt->transient_state.in_count;
		state->s_dma_area = (u_char *) tgt->cmd_ptr;
		break;

	default:
		state->s_total_count = 0;
		state->s_dma_area = 0;
		break;
	}

	state->s_residual = state->s_total_count;

	/*
	 * Save off the CDB just in case the CDB and the
	 * area to DMA from/to are in the same cache line.
	 */

	bcopy(tgt->cmd_ptr, state->s_cdb, tgt->transient_state.cmd_count);
	state->s_cdbsize = tgt->transient_state.cmd_count;


	s = splbio();
	simple_lock(&mesh->lock);
	/*
	 * Check if another command is already in progress.
	 * We may have to change this if we allow SCSI devices with
	 * separate LUNs.
	 */

	mesh->m_cmd[tgt->target_id] = tgt;

	if (mesh->m_current == NULL) 
		mesh_start_request(mesh, mesh->m_regs, state);
	else
		enqueue_tail(&mesh->m_target_queue, (queue_entry_t) tgt);

	simple_unlock(&mesh->lock);
	splx(s);
}

void
mesh_start_new_request(mesh_softc_t mesh)
{
	target_info_t	*tgt;


	tgt = (target_info_t *) dequeue_head(&mesh->m_target_queue);

	mesh_start_request(mesh, mesh->m_regs, &mesh->m_st[tgt->target_id]);

}

void
mesh_start_request(mesh_softc_t mesh, mesh_regmap_t regs,
	mesh_state_t state)
{

	MESH_SET_XFER(regs, 0);	
	mesh->m_current = state;
	MESH_TRACE("arb");
	/* From ModernIO HWMeshSupport.c - */
	/* MESH erroneously ORs in the destination ID and asserts select	*/
	/* before we actually do the selection. To be safe, ensure that		*/
	/* only the initiator ID is on the bus before the selection.		*/

	regs->r_destid = mesh->m_sc_id;		/* Load up the chip's SCSI id */
	regs->r_cmd = MESH_CMD_ARBITRATE; eieio();
	mesh->m_state = MESH_STATE_SELECTION;

}

void
mesh_start_selection(mesh_softc_t mesh, mesh_regmap_t regs,
	mesh_state_t state)
{

	MESH_TRACE("sel");

	regs->r_cmd = MESH_CMD_FLUSH_FIFO; eieio();
	regs->r_destid = state->s_id; eieio();
	regs->r_cmd = MESH_CMD_SELECT | MESH_SEQ_ATN; eieio();
	mesh->m_state = MESH_STATE_SEND_IDENTITY;

}

void
mesh_send_identity(mesh_softc_t mesh, mesh_regmap_t regs, mesh_state_t state)
{

	MESH_TRACE("send id");

	if (!mesh_polling) {
		if (mesh->m_wd.nactive++ == 0)
			mesh->m_wd.watchdog_state = SCSI_WD_ACTIVE;
	}

	MESH_SET_XFER(regs, 1);
	regs->r_cmd = MESH_CMD_MSGOUT;	eieio();
	/* First send the command, then the byte.. weird. */
	regs->r_fifo = SCSI_IDENTIFY; eieio();
	mesh->m_state = MESH_STATE_COMMAND;

}

void
mesh_send_command(mesh_softc_t mesh, mesh_regmap_t regs, mesh_state_t state)
{
	int	i;


	MESH_TRACE1("cmd size %d", state->s_cdbsize);

	regs->r_cmd = MESH_CMD_FLUSH_FIFO; eieio();

	MESH_SET_XFER(regs, state->s_cdbsize);

	regs->r_cmd = MESH_CMD_COMMAND; eieio();

	for (i = 0; i < state->s_cdbsize; i++) {
		regs->r_fifo = state->s_cdb[i];
		eieio();
	}

	if (state->s_residual == 0)
		mesh->m_state = MESH_STATE_PREP_STATUS;	/* Not sure at this point */
	else
		mesh->m_state = MESH_STATE_DATA;

	if (state->s_total_count) 
		mesh_build_sglist(mesh, state, mesh->m_cmd[state->s_id]);

}

void
mesh_prep_for_status(mesh_softc_t mesh, mesh_regmap_t regs, mesh_state_t state)
{
	MESH_SET_XFER(regs, 1);
	MESH_TRACE("prep for status");
	mesh->m_state = MESH_STATE_STATUS;
	regs->r_cmd = MESH_CMD_STATUS; eieio();
}

void
mesh_setup_for_msgin(mesh_softc_t mesh, mesh_regmap_t regs, mesh_state_t state)
{
	MESH_TRACE("setup for msgin");
	MESH_SET_XFER(regs, 1);
	regs->r_cmd = MESH_CMD_MSGIN; eieio();
	mesh->m_state = MESH_STATE_MSGIN;
	state->s_msglen = 0;

}

void
mesh_start_data_xfer(mesh_softc_t mesh, mesh_regmap_t regs,
			mesh_state_t state)
{
	vm_offset_t	address;
	unsigned long	xfer, total_bytes, count, seg_count, i;
	unsigned char 	cmd;
	boolean_t	isread = (state->s_flags & STATE_DMA_IN) != 0;
	dbdma_command_t *dmap = mesh->m_dbdma_buffer;
	struct sg_segment	*sgp;



	if ((xfer = state->s_built_count) == 0) 
		xfer = mesh_build_dbdmas(mesh, state);

	state->s_built_count = 0;

	MESH_TRACE2("data xfer %s %d", isread ? "read" : "write",
			xfer);


	MESH_SET_XFER(regs, xfer);
	cmd = (isread ? MESH_CMD_DATAIN: MESH_CMD_DATAOUT) | MESH_SEQ_DMA;
	regs->r_cmd = cmd; eieio();

	dbdma_start(mesh->m_dbdma_channel, mesh->m_dbdma_buffer);

	state->s_flags |= STATE_DMA_RUNNING;
	mesh->m_xfering = xfer;

}


void
mesh_end_transfer(mesh_softc_t mesh, mesh_regmap_t regs, mesh_state_t state)
{
	unsigned short	xfer_count;
	unsigned long	count, seg_count;
	boolean_t	isread = (state->s_flags & STATE_DMA_IN) != 0;
	volatile unsigned char	*address;
	vm_offset_t	physaddr;
	struct sg_segment	*sgp;


	dbdma_stop(mesh->m_dbdma_channel);
	xfer_count = MESH_GET_XFER(regs);


	state->s_flags &= ~(STATE_DMA_RUNNING);

	mesh->m_xfering -= xfer_count;

	state->s_residual -= mesh->m_xfering;

	MESH_TRACE2("end xfer did %d left %d",
		xfer_count, state->s_residual);

	if (isread && (xfer_count = regs->r_fifo_cnt) != 0) { 
		/*
		 * Typically when an odd byte transfer occurs,
		 * the last byte will not make it from the fifo
		 * to the DMA engine.. handle the case..
		 * (ARGH!!!)
		 */

		count = 0;
		while (xfer_count && state->s_residual > 0) {
			if (count == 0) {
				mesh_get_io_segment(mesh, state,
					&sgp, &seg_count, &physaddr, &count);

				address = (volatile unsigned char *) phystokv(physaddr);
			}
			*address++ = regs->r_fifo;
			eieio();
			count--;
			xfer_count--;
			state->s_residual--;
		}

		if (xfer_count) {
			regs->r_cmd = MESH_CMD_FLUSH_FIFO; eieio();
		}
	}

	if (state->s_residual) 
		mesh->m_state = MESH_STATE_DATA;
	else 
		mesh->m_state = MESH_STATE_PREP_STATUS;

}

void
mesh_get_status(mesh_softc_t mesh, mesh_regmap_t regs, mesh_state_t state)
{
	scsi2_status_byte_t     statusByte;


	statusByte.bits = regs->r_fifo;

	if (statusByte.st.scsi_status_code != SCSI_ST_GOOD) {
		state->s_error =
			(statusByte.st.scsi_status_code == SCSI_ST_BUSY) ?
                        SCSI_RET_RETRY : SCSI_RET_NEED_SENSE;
        } else
                state->s_error = SCSI_RET_SUCCESS;

	MESH_TRACE1("status %x", statusByte.bits);

	regs->r_cmd = MESH_CMD_FLUSH_FIFO; eieio();
	mesh->m_state = MESH_STATE_MSGIN;

	MESH_SET_XFER(regs, 1);
	regs->r_cmd = MESH_CMD_MSGIN; eieio();

}

static __inline__ void 
mesh_determine_state(mesh_softc_t mesh, unsigned char bus0status)
{

	switch (MESH_PHASE_MASK(bus0status)) {
	case	MESH_PHASE_DATI:
	case	MESH_PHASE_DATO:
		mesh->m_state = MESH_STATE_DATA;
		break;

	case	MESH_PHASE_STATUS:
		mesh->m_state = MESH_STATE_PREP_STATUS;
		break;

	case	MESH_PHASE_CMD:
		mesh->m_state = MESH_STATE_COMMAND;
		break;

	case	MESH_PHASE_MSGOUT:
		mesh->m_state = MESH_STATE_MSGOUT;
		break;

	case	MESH_PHASE_MSGIN:
		mesh->m_state = MESH_STATE_PREP_MSGIN;
		break;

	default:
		mesh_panic("Unknown bus state");
		break;
	}

}

void
mesh_intr(int device, void *ssp)
{
	mesh_softc_t	mesh;
	mesh_regmap_t	regs;
	mesh_state_t	state;
	unsigned char	int_status, exception, error, bus0status, bus1status;
	int	i;


	mesh = mesh_softc[0];
	regs = mesh->m_regs;
	state = mesh->m_current;

	simple_lock(&mesh->lock);
again:
	int_status = regs->r_interrupt; eieio();
	error = regs->r_error; eieio();
	exception = regs->r_excpt; eieio();
	bus0status = regs->r_bus0status; eieio();
	bus1status = regs->r_bus1status; eieio();

	MESH_TRACE5("intr intstatus %x, excpt %x, err %x, bus0 %x, bus1 %x",
		int_status, exception, error, bus0status, bus1status);

	if (int_status == 0) {
		simple_unlock(&mesh->lock);
		return;
	}


	/* Clear out interrupts */
	regs->r_interrupt = int_status; eieio();

	state = mesh->m_current;

	if (state && state->s_flags & STATE_DMA_RUNNING)
		mesh_end_transfer(mesh, regs, state);


	if (int_status & MESH_INTR_ERROR) {
		/* Argh! An error.. figure out what it is.. */

		if (error & MESH_ERR_SCSI_RESET) {
			mesh->m_current = 0;
			mesh->m_wd.nactive = 0;
			for (i = 0; i < MESH_NCMD; i++) {
				mesh->m_st[i].s_flags = 0;
				mesh->m_cmd[i] = NULL;
			}
			/* Wait until bus reset is deasserted.. */
			do {
				eieio();
			} while (regs->r_bus1status & MESH_BUS1_STATUS_RESET);

			mesh_setup_chip(mesh, regs, FALSE);
			scsi_bus_was_reset(mesh->m_sc);
			simple_unlock(&mesh->lock);
			return;
		}

		if (error & MESH_ERR_DISCONNECT) {
			if (mesh->m_state == MESH_STATE_BUS_FREE) {
				simple_unlock(&mesh->lock);
				return;	/* Don't need to do anything */
			} else if (mesh->m_state == MESH_STATE_COMPLETE) {
				mesh_end(mesh, regs, state);
				simple_unlock(&mesh->lock);
				return;
			} else {
				mesh->m_state = MESH_STATE_BUS_FREE;
				state->s_error = SCSI_RET_DEVICE_DOWN;
				mesh_end(mesh, regs, state);
				simple_unlock(&mesh->lock);
				return;
			} 
		}

		if (error & MESH_ERR_PARITY0) 
			mesh_panic("Parity Error");

	} else if (int_status & MESH_INTR_EXCPT) {
		if (exception & MESH_EXCPT_SELTO) {
			state->s_error = SCSI_RET_DEVICE_DOWN;
			mesh_end(mesh, regs, state);
			simple_unlock(&mesh->lock);
			return;
		}

		if (exception & (MESH_EXCPT_ARBLOST | MESH_EXCPT_RESEL)
		&& mesh->m_state < MESH_STATE_COMMAND) {
			panic("TODO - MESH HANDLE LOST ARB (#2)!");
			mesh->m_state = MESH_STATE_WAIT_RESEL;
		} else if (exception & MESH_EXCPT_PHASE) {
			MESH_TRACE("phase change");
			/* Different bus phase.. figure things out */
			mesh_determine_state(mesh, bus0status);
		}
	}


run_sequence:
	if (mesh->m_state == MESH_STATE_FLOAT) {
		MESH_TRACE("floating phase state");
		mesh_determine_state(mesh, bus0status);
	}

	if (mesh_states[mesh->m_state] == NULL) 
		mesh_panic("Unknown state!");
	else
		mesh_states[mesh->m_state](mesh, regs, state);

	simple_unlock(&mesh->lock);
}

void
mesh_end(mesh_softc_t mesh, mesh_regmap_t regs, mesh_state_t state)
{
	register target_info_t *tgt;
	register int i;

	MESH_TRACE1("end request. error = %d", state->s_error);

	mesh->m_state = MESH_STATE_BUS_FREE;
	mesh->m_current = NULL;
	tgt = mesh->m_cmd[state->s_id];
	mesh->m_cmd[state->s_id] = (target_info_t *)0;
	state->s_flags |= STATE_DONE;

#if 0
	/* look for disconnected devices */
	for (i = 0; i < MESH_NCMD; i++) {
		if (!mesh->m_cmd[i] || !(mesh->m_st[i].s_flags & STATE_DISCONNECTED))
			continue;
		MESH_TRACE1("end {reconn tgt %d}", i);
		regs->r_cmd = MESH_CMD_ENABLE_RESELECT; eieio();
		mesh->m_sate = MESH_STATE_WAIT_RESEL;
		break;
	}
#endif


	/*
	 * Look for another device that is ready.
	 * May want to keep last one started and increment for fairness
	 * rather than always starting at zero.
	 */

	if (mesh_polling == FALSE) {
		/* TODO - upper SCSI layers need to be fixed
		 * to accept residual counts.
		 */

		tgt->done = state->s_error;

		if (!queue_empty(&mesh->m_target_queue))
			mesh_start_new_request(mesh);

		if (tgt->done == SCSI_RET_IN_PROGRESS)
			tgt->done = SCSI_RET_SUCCESS;

		if (mesh->m_wd.nactive-- == 1)
			mesh->m_wd.watchdog_state = SCSI_WD_INACTIVE;

		if (tgt->ior) {
			simple_unlock(&mesh->lock);
			tgt->dev_ops->restart(tgt, TRUE);
			simple_lock(&mesh->lock);
		}
	}

	return;
}

/* ARGSUSED */
void
mesh_process_msgin(mesh_softc_t mesh, mesh_regmap_t regs, mesh_state_t state)
{
	target_info_t	*tgt = mesh->m_cmd[state->s_id];
	unsigned char msg, *msgp;
	int i;


	/* read one message byte */
	msg = regs->r_fifo; eieio();

	MESH_TRACE2("msg_in {msg %x, len %d}",
		msg, state->s_msglen);

	/* check for multi-byte message */
	if (state->s_msglen != 0) {
		/* first byte is the message length */
		if (state->s_msglen < 0) {
			state->s_msglen = msg;
			goto xfer_msgin;
		}

		if (state->s_msgcnt >= state->s_msglen)
			goto reject;

		state->s_msg[state->s_msgcnt++] = msg;

		/* did we just read the last byte of the message? */
		if (state->s_msgcnt != state->s_msglen) 
			goto xfer_msgin;

		/* process an extended message */
		switch (state->s_msg[0]) {
		case	SCSI_SYNC_XFER_REQUEST:
			/*
			 * Setup to send back a message.. 
			 */

			state->s_msgcnt = 5;
			state->s_msg_sent = SCSI_EXTENDED_MESSAGE;
			
			state->s_msg[4] = 0; /* Sync Offset - 0 means async */

			mesh_send_msg(mesh, regs, state);
			return;

		default:
			printf("SCSI Bus %d target %d: rejecting extended message 0x%x\n",
				mesh->m_unit, mesh->m_current->s_id,
				state->s_msg[0]);
			goto reject;
		}
	}

	/* process first byte of a message */

	switch (msg) {
	case SCSI_MESSAGE_REJECT:
		MESH_TRACE("msg reject");
		switch (state->s_msg_sent) {
		case	SCSI_SYNC_XFER_REQUEST:
			tgt->flags |= TGT_DID_SYNCH;	/* No sync transfers */
			tgt->flags &= ~TGT_TRY_SYNCH;
			state->s_dev_flags &= ~DEV_RUN_SYNC;
			tgt->sync_offset = 0;
			break;

		case	SCSI_IFY_ENABLE_DISCONNECT:
			state->s_dev_flags &= ~DEV_NO_DISCONNECT;
			break;

		}
		break;

	case SCSI_EXTENDED_MESSAGE: /* read an extended message */
		/* setup to read message length next */
		state->s_msglen = -1;
		state->s_msgcnt = 0;
		goto xfer_msgin;

	case SCSI_NOP:
		MESH_TRACE("msg nop");
		break;

	case SCSI_SAVE_DATA_POINTER:
		MESH_TRACE("msg save dataptr");
		state->s_saved_residual = state->s_residual;
		state->s_saved_s_index  = state->s_index;
		/* expect another message (Disconnect) */
		regs->r_cmd = MESH_CMD_BUSFREE; eieio();
		return;

	case SCSI_RESTORE_POINTERS:
		MESH_TRACE("msg restore dataptr");
		state->s_residual = state->s_saved_residual;
		state->s_index = state->s_saved_s_index;
		state->s_built_count = 0;
		break;

	case SCSI_DISCONNECT:
		MESH_TRACE("msg disconnect");
		if (state->s_flags & STATE_DISCONNECTED)
			goto reject;
		state->s_flags |= STATE_DISCONNECTED;
		regs->r_cmd = MESH_CMD_BUSFREE; eieio();
		mesh->m_state = MESH_STATE_BUS_FREE;
		return;

	case SCSI_COMMAND_COMPLETE:
		MESH_TRACE("msg complete");
		state->s_flags |= STATE_DISCONNECTED;
		regs->r_cmd = MESH_CMD_BUSFREE; eieio();
		mesh->m_state = MESH_STATE_COMPLETE;
		return;

	default:
		printf(" SCSI Bus %d target %d: rejecting message 0x%x\n",
			mesh->m_unit, mesh->m_current->s_id, msg);
	reject:
		/* request a message out before acknowledging this message */
		state->s_msg[0] = SCSI_MESSAGE_REJECT;
		state->s_msgcnt = 1;
		mesh_send_msg(mesh, regs, state);
		return;
	}

done:
	/* return to original script */
	regs->r_cmd = MESH_CMD_BUSFREE; eieio();
	mesh->m_state = MESH_STATE_FLOAT;
	return;

xfer_msgin:

	mesh->m_state = MESH_STATE_MSGIN;
	MESH_SET_XFER(regs, 1);
	regs->r_cmd = MESH_CMD_MSGIN; eieio();
	return;
}

void
mesh_send_msg(mesh_softc_t mesh, mesh_regmap_t regs, mesh_state_t state)
{
	unsigned char *msgp = state->s_msg;
	int	i;


	/*
	 * Assumption - No message out packet will be bigger than 16 bytes
	 */

	MESH_SET_XFER(regs, state->s_msgcnt);
	regs->r_cmd = MESH_SEQ_ATN | MESH_CMD_MSGOUT; eieio();

	for (i = 0; i < state->s_msgcnt; i++) {
		regs->r_fifo = *msgp++; eieio();
	}

	state->s_msgcnt = 0;

	mesh->m_state = MESH_STATE_FLOAT;

}

void
mesh_disconnect(mesh_softc_t mesh, mesh_regmap_t regs, mesh_state_t state)
{

	MESH_TRACE("disconnect");

	mesh->m_current = NULL;

	mesh->m_state = MESH_STATE_WAIT_RESEL;

#if 1
	regs->r_cmd = MESH_CMD_ENABLE_RESELECT; eieio();
#else
	if (!queue_empty(&mesh->m_target_queue)) {
		regs->r_cmd = MESH_CMD_ENABLE_RESELECT; eieio();
		mesh_start_new_request(mesh);
	}
#endif

}


/*
 * Watchdog
 *
 * So far I have only seen the chip get stuck in a
 * select/reselect conflict: the reselection did
 * win and the interrupt register showed it but..
 * no interrupt was generated.
 * But we know that some (name withdrawn) disks get
 * stuck in the middle of dma phases...
 */

int
mesh_reset_scsibus(mesh_softc_t	mesh)
{
	mesh_regmap_t	regs;

	simple_lock(&mesh->lock);

	regs = mesh->m_regs;

#if DEBUG
	mesh_DumpLog("Request Timeout");
	printf("MESH SCSI Timeout\n");
#endif
	mesh->m_wd.nactive = 0;

	panic("Need to handle RESET\n");
#if 0
	regs->r_cmd = MESH_CMD_RESET_BUS; eieio();
	delay(35);
#endif

	simple_unlock(&mesh->lock);
}

void __inline__ 
mesh_construct_entry(mesh_state_t st, boolean_t isread, 
			vm_offset_t address, long count, long offset)
{
	struct sg_segment *sgp = &st->s_list[st->s_segment_count],
			  *prev_sgp;
	vm_offset_t	physaddr;
	long	real_count;
	int	i;


	if ((i = st->s_segment_count) != 0)
		prev_sgp = &st->s_list[i-1];
	else
		prev_sgp = NULL;

	if (powermac_info.io_coherent == FALSE) {
		if (isread)
			invalidate_cache_for_io(address, count, FALSE);
		else
			flush_dcache(address, count, FALSE);
	}

	for (i = st->s_segment_count; i < MESH_MAX_SG_SEGS && count > 0;) {
		physaddr = kvtophys(address);

		if (physaddr == 0)
			panic("SCSI DMA - Zero physical address");

		real_count = 0x1000 - (physaddr & 0xfff);

		real_count = u_min(real_count, count);

		/*
		 * Check to see if this address and count
		 * can be combined with the previous 
		 * S/G element. This helps to reduce the
		 * number of interrupts.
		 */

		if (prev_sgp && (prev_sgp->sg_physaddr+prev_sgp->sg_count) == physaddr) {
			prev_sgp->sg_count += real_count;

		} else {
			sgp->sg_offset = offset;
			sgp->sg_count = real_count;
			sgp->sg_physaddr = physaddr;
			sgp->sg_vmaddr = (unsigned char *) address;
			prev_sgp =  sgp++;
			i++;
		}

		count -= real_count;
		offset += real_count;
		address += real_count;
	}

	if (count) 
		panic("Mesh SCSI - S/G overflow.");

	st->s_segment_count = i;

}

void
mesh_build_sglist(mesh_softc_t mesh, mesh_state_t st, target_info_t *tgt)
{
	io_req_t	ior;
	int		i;
	struct sg_segment	*sgp = st->s_list;
	unsigned long offset = 0, count, total_bytes;
	boolean_t	isread = (st->s_flags & STATE_DMA_IN) != 0;


	st->s_segment_count = 0;
	st->s_index = 0;

	if ((st->s_flags & STATE_CHAINED_IO) == 0)  {
		mesh_construct_entry(st, isread,
				(vm_offset_t) st->s_dma_area,
				st->s_total_count, 0);
	} else {

		for (ior = tgt->ior; ior ; ior = ior->io_link) {
			count = ior->io_count;

			if (ior->io_link)
				count -= ior->io_link->io_count;
	
			mesh_construct_entry(st, isread,
				(vm_offset_t) ior->io_data, count, offset);
			offset += count;
		}
	}

	st->s_built_count  = mesh_build_dbdmas(mesh, st);
}

long
mesh_build_dbdmas(mesh_softc_t mesh, mesh_state_t st)
{
	int	i;
	long	retbytes, total_bytes, count,seg_count;
	vm_offset_t	address;
	long	cmd;
	dbdma_command_t	*dmap = mesh->m_dbdma_buffer;
	boolean_t	isread = (st->s_flags & STATE_DMA_IN) != 0;
	struct sg_segment	*sgp;

	mesh_get_io_segment(mesh, st, &sgp, &seg_count, &address, &count);


	if ((total_bytes = st->s_residual) > MESH_MAX_TRANSFER)
		total_bytes = MESH_MAX_TRANSFER;

	retbytes = total_bytes;

	cmd = isread ? DBDMA_CMD_IN_MORE : DBDMA_CMD_OUT_MORE;

	for (i = 0; total_bytes > 0 && i < MESH_DBDMA_BUFFERS; i++, dmap++, sgp++) {
		if (i) {
			count = sgp->sg_count;
			address = sgp->sg_physaddr;
		}

		if (count > total_bytes)
			count = total_bytes;

		total_bytes -= count;

		if (total_bytes == 0)
			cmd = isread ? DBDMA_CMD_IN_LAST : DBDMA_CMD_OUT_LAST;

		DBDMA_BUILD(dmap, cmd, 0, count, address,
			DBDMA_INT_NEVER, DBDMA_BRANCH_NEVER, DBDMA_WAIT_NEVER);

	}


        DBDMA_BUILD(dmap, DBDMA_CMD_STOP, 0, 0, 0, DBDMA_INT_NEVER,
			DBDMA_BRANCH_NEVER, DBDMA_WAIT_NEVER);
	
	return (retbytes - total_bytes);
}

/*
 * mesh_get_io_segment
 *
 * Get the next physical address, count to transfer. 
 * The next S/G segment and segments left count is also returned.
 *
 * This returned is should only be called when the DBDMA buffer
 * needs to be rebuild either to due a disconnect or restore ptrs
 * message.
 */

void
mesh_get_io_segment(mesh_softc_t mesh, mesh_state_t st,
		struct sg_segment **seg,
		unsigned long *seg_count,
		vm_offset_t   *address,
		unsigned long *count)
{
	long offset, xfer;
	struct sg_segment	*sgp;


	*count = st->s_residual;
	offset = st->s_total_count - *count;

	sgp = &st->s_list[st->s_index];

	while (st->s_index < st->s_segment_count) {
		if (offset >= sgp->sg_offset
		&& offset < (sgp->sg_offset + sgp->sg_count)) 
			goto done;
		st->s_index++;
		sgp++;
	}

#if DEBUG
	mesh_DumpLog("SCSI DMA Error");
#endif
	printf("Current offset %d, s/g item offset %d\n",
			offset, sgp->sg_offset);
	panic("SCSI Error : DMA Offset Error");

done:
	offset -= sgp->sg_offset;
	xfer = sgp->sg_count - offset;

	if (*count > xfer)
		*count = xfer;

	if (*count > MESH_MAX_TRANSFER)
		*count = MESH_MAX_TRANSFER;

	*address = sgp->sg_physaddr + offset;

	MESH_TRACE4("Data {Phys %x Addr %x, S/G Count %d Xfer Count %d}",
		sgp->sg_physaddr, *address, sgp->sg_count, *count);

	if (seg) {
		*seg = sgp;
		*seg_count = st->s_segment_count - st->s_index;
	}

	return;
}

void
mesh_setup_chip(mesh_softc_t mesh, mesh_regmap_t regs, boolean_t reset_bus)
{
	volatile unsigned char	r;


	MESH_TRACE("chip reset");

	regs->r_intmask = 0; eieio();	/* No interrupts */

	if (reset_bus) {
		regs->r_bus1status = MESH_BUS1_STATUS_RESET; eieio();
		delay(300);	/* Give the bus some time to settle.. */
		regs->r_bus1status = 0; eieio();
	}

	regs->r_cmd = MESH_CMD_RESET_MESH; eieio();

	delay(1);

	do {
		eieio();
	} while (regs->r_interrupt == 0);

	regs->r_interrupt = 0xff; eieio(); /* Clear interrupts */

	regs->r_sync =  2; eieio();
	regs->r_sourceid = 7; eieio();		/* SCSI ID */
	regs->r_sel_timeout = 25; eieio();	/* Selection timeout */
	regs->r_cmd = MESH_CMD_ENABLE_PARITY; eieio();

	regs->r_intmask = 0x7;	/* Enable all interrupts */
	eieio();

}


#if MESH_DEBUG
#define NLOG 32

struct mesh_log {
	char	*msg;
	int	sequence;
	int	state;
	int	unit;
	int	target;
	u_long	args[5];
} mesh_log[NLOG], *mesh_logp = mesh_log;

static int mesh_trace_sequence = 0;

void
mesh_print_log(struct mesh_log *lp)
{
	if (lp->msg) {
		printf("%d %d:%d %s ", lp->sequence, lp->unit, lp->target,
			mesh_state_names[lp->state]);
		printf(lp->msg, lp->args[0], lp->args[1], lp->args[2],
				lp->args[3], lp->args[4]);
		printf("\n");
	}
}

mesh_DumpLog(str)
	char *str;
{
	register struct mesh_log *lp;


	printf("*** Dumping SCSI Trace Log. Reason: %s\n", str);

	lp = mesh_logp;
	do {
		mesh_print_log(lp);
		if (++lp >= &mesh_log[NLOG])
			lp = mesh_log;
	} while (lp != mesh_logp);
	printf("**** Please try to submit **ALL** of  the above trace log\n");
	printf("**** to bugs@mklinux.apple.com. Thank you.\n\n");

}

void
mesh_trace_log(mesh_softc_t mesh, char *msg, u_long a1, u_long a2,
		u_long a3, u_long a4, u_long a5)
{
	struct mesh_log	*lp = mesh_logp;


	lp->unit = mesh->m_unit;

	if (mesh->m_current)
		lp->target = mesh->m_current->s_id;
	else
		lp->target = -1;

	lp->state = mesh->m_state;

	lp->sequence = mesh_trace_sequence++;
	lp->msg = msg;
	lp->args[0] = a1;
	lp->args[1] = a2;
	lp->args[2] = a3;
	lp->args[3] = a4;
	lp->args[4] = a5;

	if (mesh_trace_debug) 
		mesh_print_log(lp);

	lp++;
	if (lp >= &mesh_log[NLOG])
		lp = mesh_log;

	mesh_logp = lp;

}

#endif /*DEBUG*/

void
mesh_panic(char *str)
{
#if MESH_DEBUG
	mesh_DumpLog(str);
#endif
	panic(str);
}
#endif	/* NMESH > 0 */
