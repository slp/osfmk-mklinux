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
 */
/*
 * MkLinux
 */
/*
 *	File: scsi_53C825_hdw.c
 * 	Author: Randall W. Dean, OSF
 *	Date:	9/94
 *
 *	Bottom layer of the SCSI driver: chip-dependent functions
 *
 *	This file contains the code that is specific to the NCR 53C825
 *	SCSI chip (Host Bus Adapter in SCSI parlance): probing, start
 *	operation, and interrupt routine.
 */


#include <ncr825.h>
#if	NNCR825 > 0
#include <platforms.h>
#include <mp_v1_1.h>
#include <scsi.h>
#include <mach_kdb.h>

#include <string.h>

#include <mach/std_types.h>
#include <sys/types.h>
#include <chips/busses.h>
#include <scsi/compat_30.h>
#include <machine/machparam.h>

#include <busses/pci/pci.h>
#include <i386/pci/pci.h>
#include <i386/pci/pcibios.h>
#include <i386/pci/pci_device.h>
#include <i386/ipl.h>

#include <sys/syslog.h>

#include <scsi/scsi.h>
#include <scsi/scsi2.h>
#include <scsi/scsi_defs.h>

#include <scsi/adapters/scsi_53C825.h>
#include <kern/misc_protos.h>
#include <kern/spl.h>

#include <ddb/tr.h>

#ifdef	AT386
#include <i386/AT386/mp/mp.h>
#if	MP_V1_1
#include <i386/AT386/mp/mp_v1_1.h>
#endif	/* MP_V1_1 */
#endif	/* AT386 */

#ifdef	AT386
#define	MACHINE_PGBYTES		I386_PGBYTES
#endif

#define NCR825_STATS 0
#if	NCR825_STATS
#define nstat_decl(variable)	unsigned int nstat_##variable = 0;
#define nstat_inc(variable)	nstat_##variable++
#define nstat_add(variable, amount)	nstat_##variable += amount
#define nstat_print(variable)	iprintf("%s = %d\n",#variable,nstat_##variable)
#else
#define nstat_decl(x)
#define nstat_inc(x)
#define nstat_add(x,y)
#define nstat_print(x)
#endif

nstat_decl(tmode_cmds)
nstat_decl(imode_cmds)
nstat_decl(disconnects)
nstat_decl(resizes)
nstat_decl(mismatch);
nstat_decl(reset);
nstat_decl(tmode_intr);
nstat_decl(tmode_disc);
nstat_decl(reselected);

#if NCR825_STATS
void ncr825_stats(void);
void ncr825_stats(void)
{
	nstat_print(imode_cmds);
	nstat_print(tmode_cmds);
	nstat_print(disconnects);
	nstat_print(resizes);
	nstat_print(mismatch);
	nstat_print(reset);
	nstat_print(tmode_intr);
	nstat_print(tmode_disc);
	nstat_print(reselected);
}
#endif

#define NCOMMANDS 528

#ifdef	INB
#undef INB
#endif	/* INB */
#ifdef	INW
#undef INW
#endif	/* INW */
#ifdef	INL
#undef INL
#endif	/* INL */
#ifdef	OUTB
#undef OUTB
#endif	/* OUTB */
#ifdef	OUTW
#undef OUTW
#endif	/* OUTW */
#ifdef	OUTL
#undef OUTL
#endif	/* OUTL */

#define INB(r) (ncs->reg->r)
#define INW(r) (ncs->reg->r)
#define INL(r) (ncs->reg->r)

#define OUTB(r, val) ncs->reg->r = val
#define OUTW(r, val) ncs->reg->r = val
#define OUTL(r, val) ncs->reg->r = val

#define BOOLP(x) ((x)?"":"!")

#define MAXSG 17

typedef struct ncr825_command {
	ncrcmd move[(MAXSG*2)+2];
	struct ncr825_command *next;
	struct ncr825_softc_struct *ncs;
	target_info_t	*tgt;
	struct scr_tblsel select;
	struct scr_tblmove lun_tbl;
	struct scr_tblmove size_tbl;
	struct scr_tblmove data_tbl[MAXSG];
	unsigned char lun;
	unsigned char reselect;
	unsigned char passive;
	unsigned char
	  hth_count:4,
	  sto_count:4;
	volatile unsigned long size;
	scsi_command_group_5 cdb;
	void *space;
} *ncr825_command_t;

#define CMD_NULL ((ncr825_command_t)0)

typedef struct ncr825_softc_struct {
	struct watchdog_t	wd;
	pcici_t			tag;
	int			ncr825_unit;
	scsi_softc_t		*sc;
	vm_offset_t		vaddr;
	vm_offset_t		paddr;
	int			my_scsi_id;
	vm_offset_t		bios_paddr;
	volatile struct ncr_reg	*reg;
	vm_offset_t		vscript;
	vm_offset_t		pscript;
	decl_simple_lock_data(, lock);
	int			allocated;
	struct ncr825_command	commands[NCOMMANDS];
	ncr825_command_t	tmode_cmds[MAX_SCSI_TARGETS*MAX_LUNS];
	unsigned long		ptmode_cmds[MAX_SCSI_TARGETS*MAX_LUNS];
	ncr825_command_t	disc_cmds[MAX_SCSI_TARGETS*MAX_LUNS];
	unsigned int
	  script_running:1,
	  executing_command:1;
/* This must match the #defines for D_ done below */
	unsigned int		target;
	unsigned int		lun;
	vm_size_t		size;
	ncr825_command_t	new_cmd;
	vm_offset_t		*cmd_vector;
	vm_offset_t		pcmd_vector;
} *ncr825_softc_t;

static ncrcmd dummy[4];
#define D_TARGET 0
#define D_LUN 1
#define D_SIZE 2
#define D_NEW_COMMAND 3

ncr825_softc_t ncr825_softc[NNCR825];

int ncr825_units = 0;
static ncrcmd dev_null[4];

int ncr825_pci_probe(pcici_t config_id);
int ncr825_pci_attach(pcici_t config_id);
void ncr825_intr(int dev);
int ncr825_probe(caddr_t port, void *ui);
void ncr825_attach(struct bus_device *device);
void ncr825_go(target_info_t	*tgt,
	     unsigned int	cmd_count,
	     unsigned int	in_count,
	     boolean_t		cmd_only);
boolean_t ncr825_probe_target(target_info_t	*tgt,
			      io_req_t		ior);
void ncr825_tgt_setup(target_info_t *tgt);
int ncr825_remote_op(target_info_t	*tgt,
		     int		op,
		     unsigned int	remote_pa,
		     char		*buffer,
		     unsigned int	count);
target_info_t *ncr825_tgt_alloc(ncr825_softc_t	ncs,
				unsigned char	id,
				target_info_t	*tgt);
void ncr825_script_bind(ncr825_softc_t ncs, ncrcmd *p);
void ncr825_enqueue_command(ncr825_command_t cmd);
void ncr825_dequeue_command(ncr825_command_t cmd);
void ncr825_next_command(ncr825_softc_t ncs);
int ncr825_sg(ncr825_command_t	cmd,
	      int		ne,
	      vm_offset_t	virt,
	      vm_size_t		len);
void ncr825_resize(ncr825_command_t	cmd,
		   vm_size_t		size);
void ncr825_reset_chip(ncr825_softc_t ncs, char *why);
void ncr825_UDC(ncr825_softc_t ncs, unsigned char dstat);

#if	MACH_KDB

typedef void (*print_routine_t)(const char *format, ...);

void ncr825_debug(int unit);
void ncr825_abort(int unit);
void ncr825_bits(char *r,
		 char *s0,
		 char *s1,
		 char *s2,
		 char *s3,
		 char *s4,
		 char *s5,
		 char *s6,
		 char *s7,
		 unsigned char c,
		 print_routine_t printf);
void ncr825_dump(ncr825_softc_t ncs,
		 unsigned char dstat,
		 unsigned short sist,
		 print_routine_t printf);
#endif	/* MACH_KDB */

struct bus_driver ncr825_driver = {
	ncr825_probe,
	((int (*)(struct bus_device *device, caddr_t virt))scsi_slave),
	ncr825_attach,
	((int (*)(void))ncr825_go),
	0,
	"rz",
	0,
	"ncr53C825",
	0,
	0};

typedef enum {
	INT_Target_Mode = 1,
	INT_Command_Complete,
	INT_Reselected,
	INT_Confused,
	INT_Bad_DSA,
	INT_Remote_OP_Complete
    } INTs;

boolean_t ncr825_ssm = 0;
unsigned long ncr825_zeros = 0;

struct script {
	ncrcmd	start[4];
	ncrcmd	selected[23];
	ncrcmd	tgt_vector[1];
	ncrcmd	selected2[5];
	ncrcmd	lun_vector[1];
	ncrcmd	selected3[12];
	ncrcmd	test_reselect[2];
	ncrcmd  reselected[11];
	ncrcmd	pgi_to_confused[6];
	ncrcmd	pgi_to_reselect[6];
	ncrcmd  pgi[11];
	ncrcmd	drive_initiator[11];
	ncrcmd  target_done[6];
	ncrcmd  reselect[13];
	ncrcmd  initiator_done[10];
	ncrcmd	disconnect[4];
	ncrcmd  getbyte[2];
	ncrcmd	attention[8];
	ncrcmd	remote_op[5];
	ncrcmd	remote_size[1];
	ncrcmd  remote_addr[1];
	ncrcmd	remote_op2[4];
};

#define	offsetof(type, member)	((int)(&((type *)0)->member))
#define PADDR(label)        ((ncrcmd) &ncr825_script0.label)

struct script ncr825_script0 = {
/* start */
    {
	    SCR_SET(SCR_TRG), 0,
	    SCR_WAIT_SELECT, PADDR(pgi_to_reselect)
    },
/* selected */
    {
	    SCR_REG_SFBR(ssid, SCR_AND, 0x7), 0,
	    SCR_COPY(1), REG(sfbr), (ncrcmd)&dummy[D_TARGET],
	    SCR_SFBR_REG(sfbr, SCR_SHL, 0), 0,
	    SCR_SFBR_REG(scratcha, SCR_SHL, 0), 0,
	    SCR_TMOVE_ABS(4) ^ SCR_MSG_OUT, (ncrcmd)&dummy[D_SIZE],
	    SCR_TMOVE_ABS(1) ^ SCR_MSG_OUT, (ncrcmd)&dummy[D_LUN],
	    SCR_JUMP ^ IFTRUE(SCR_DATA(0xff)), PADDR(remote_op),
	    SCR_SFBR_REG(sfbr, SCR_SHL, 0), 0,
	    SCR_SFBR_REG(scratchb, SCR_SHL, 0), 0,
	    SCR_COPY(1), REG(scratcha), PADDR(tgt_vector),
	    SCR_COPY(4),
    },
/* tgt_vector */
    {
	    0 /* Patched by driver */
    },
/* selected2 */
    {
	    PADDR(lun_vector),
	    SCR_COPY(1), REG(scratchb), PADDR(lun_vector),
	    SCR_COPY(4),
    },
/* lun_vector */
    {
	    0 /* Patched by code above */
    },
/* selected3 */
    {
	    REG(dsa),
	    SCR_FROM_REG(dsa), 0,
	    SCR_INT ^ IFTRUE(SCR_DATA(0x00)), INT_Target_Mode,
#define STIME0_HTH 0x68				/* HTH=3.2ms, STO = 12.8ms */
#define STIME0 0x08				/* HTH=0, STO = 12.8ms */
	    SCR_LOAD_REG(stime0, STIME0_HTH), 0,
	    SCR_COPY(4), REG(dsa), REG(temp),
	    SCR_RETURN, 0
    },
/* test_reselect */
    {
	    SCR_WAIT_RESEL, PADDR(pgi_to_confused)
    },
/* reselected */
    {
	    SCR_REG_SFBR(ssid, SCR_AND, 0x7), 0,
	    SCR_COPY(1), REG(sfbr), (ncrcmd)&dummy[D_TARGET],
	    SCR_MOVE_ABS(1) ^ SCR_MSG_IN, (ncrcmd)&dummy[D_LUN],
	    SCR_CLR(SCR_ACK), 0,
	    SCR_INT, INT_Reselected
    },
/* pgi to confused*/	
    {
	    SCR_FROM_REG(ctest2), 0,
	    SCR_INT ^ IFFALSE(MASK(CSIGP,CSIGP)), INT_Confused,
	    SCR_JUMP, PADDR(pgi)
    },
/* pgi to reselect */	
    {
	    SCR_CLR(SCR_TRG), 0,
	    SCR_FROM_REG(ctest2), 0,
	    SCR_JUMP ^ IFFALSE(MASK(CSIGP,CSIGP)), PADDR(test_reselect)
    },
/* pgi */
    {
	    SCR_COPY(4), (ncrcmd)&dummy[D_NEW_COMMAND], REG(dsa),
	    SCR_FROM_REG(dsa), 0,
	    SCR_INT ^ IFTRUE(SCR_DATA(0x00)), INT_Bad_DSA,
	    SCR_FROM_REG(istat), 0,
	    SCR_JUMP ^ IFTRUE(MASK(SEM,SEM)), PADDR(reselect)
    },
/* drive initiator */
    {
	    SCR_SEL_TBL ^ offsetof(struct ncr825_command, select),
	    	PADDR(start),
	    SCR_MOVE_TBL ^ SCR_MSG_OUT,
	    	offsetof(struct ncr825_command, size_tbl),
	    SCR_MOVE_TBL ^ SCR_MSG_OUT,
	    	offsetof(struct ncr825_command, lun_tbl),
	    SCR_COPY(4), REG(dsa), REG(temp),
	    SCR_RETURN, 0
    },
/* target done */
    {
	    SCR_TMOVE_TBL ^ SCR_MSG_IN,
	        offsetof(struct ncr825_command, size_tbl),
	    SCR_DISCONNECT, 0,
	    SCR_INT, INT_Command_Complete
    },
/* reselect */
    {
	    SCR_SET(SCR_TRG), 0,
	    SCR_RESEL_TBL ^offsetof(struct ncr825_command, select),
	    	PADDR(start),
	    SCR_TMOVE_TBL ^ SCR_MSG_IN,
	    	offsetof(struct ncr825_command, lun_tbl),
	    SCR_LOAD_REG(stime0, STIME0_HTH), 0,
	    SCR_COPY(4), REG(dsa), REG(temp),
	    SCR_RETURN, 0
    },
/* initiator done */
    {
	    SCR_MOVE_ABS(4) ^ SCR_MSG_IN, (ncrcmd)&dummy[D_SIZE],
	    SCR_REG_REG(scntl2, SCR_AND, 0x7f), 0,
	    SCR_CLR(SCR_ACK), 0,
	    SCR_WAIT_DISC, 0,
	    SCR_INT, INT_Command_Complete
    },
/* disconnect */
    {
	    SCR_DISCONNECT, 0,
	    SCR_JUMP, PADDR(start)
    },
/* getbyte */
    {
	    SCR_MOVE_ABS(1) ^ SCR_DATA_OUT, (ncrcmd)&dev_null,
    },
/* attention */
    {
	    SCR_SET(SCR_ATN), 0,
	    SCR_JUMP ^ IFFALSE( WHEN(SCR_MSG_IN)), PADDR(getbyte),
	    SCR_CLR(SCR_ATN), 0,
	    SCR_JUMP, PADDR(initiator_done)
    },
/* remote op */
    {
	    SCR_TMOVE_ABS(4) ^ SCR_MSG_OUT, PADDR(remote_addr),
	    SCR_COPY(3), (ncrcmd)&dummy[D_SIZE], PADDR(remote_size),
    },
/* remote size */
    {
	    SCR_TMOVE_ABS(0) ^ SCR_DATA_IN
    },
/* remote addr */
    {
	    0 /* Patched by scripts code */
    },
/* remote op 2 */
    {
	    SCR_DISCONNECT, 0,
	    SCR_JUMP, PADDR(start)
    },
};

extern scsi_softc_t	scsi_softc_data[NSCSI];

int ncr825_probe(caddr_t port, void *ui)
{
	panic("ncr825_probe");
	return 0;
}

void ncr825_attach(struct bus_device *device)
{
	panic("ncr825_attach");
}

int ncr825_pci_probe(pcici_t config_id)
{
	int unit;
	if (ncr825_units >= NNCR825) return (-1);
	for(unit=0;unit<NSCSI;unit++)
	    if (scsi_softc[unit] != &scsi_softc_data[unit]) {
		    scsi_softc[unit] = &scsi_softc_data[unit];
		    scsi_softc_data[unit].hw_state = (char *)config_id.cfg1;
		    return unit;
	}
	return (-1);
}

void ncr825_script_bind(ncr825_softc_t ncs, ncrcmd *p)
{
	ncrcmd  opcode, arg, a1;
	ncrcmd	*end, *start;
	
	start = p;
	for (end=p+(sizeof(struct script)/4);p<end;) {
		
		/*
		 *	If we forget to change the length
		 *	in struct script, a field will be
		 *	padded with 0. This is an illegal
		 *	command.
		 */
		
		if (!(opcode=*p++))
		    printf ("ncr%d: ERROR0 IN SCRIPT at %d.\n",
			    ncs->ncr825_unit, p-start-1);
		arg=*p;
		/*
		 *	We don't have to decode ALL commands
		 */
		switch (opcode >> 28) {
		      case 0xc:
			/*
			 *	COPY has TWO arguments.
			 */
			a1 = arg;
			if (a1 >= 256) {
				if (*p >= (ncrcmd)dummy &&
				    *p <= (ncrcmd)dummy + sizeof(dummy))
				    *p +=kvtophys(((vm_offset_t)&ncs->target))
						  - (vm_offset_t)dummy;
				else
				    *p = kvtophys (a1);
			} else
			    *p = ncs->paddr + a1;
			arg = *++p;
			if ((a1 ^ arg) & 3)
			    printf ("ncr%d: script: ERROR1 IN SCRIPT at %d.\n",
				    ncs->ncr825_unit, p-start-1);
			/* pass */
		      case 0x0:
			/*
			 *	MOVE (absolute address)
			 */
			if (arg >= 256) {
				if (*p >= (ncrcmd)dummy &&
				    *p <= (ncrcmd)dummy + sizeof(dummy))
				    *p +=kvtophys(((vm_offset_t)&ncs->target))
						  - (vm_offset_t)dummy;
				else
				    *p = kvtophys (arg);
			} else
			    *p = ncs->paddr + arg;
			break;
		      case 0x8:
			/*
			 *	JUMP / CALL
			 *	dont't relocate if relative :-)
			 */
			if (opcode & 0x00800000) break;
			*p = kvtophys(arg);
			break;
			/*
			 *	don't relocate 0 arguments.
			 */
		      case 0x4:
		      case 0x5:
		      case 0x6:
		      case 0x7:
			if (arg) {
				if (*p >= (ncrcmd)dummy &&
				    *p <= (ncrcmd)dummy + sizeof(dummy))
				    *p +=kvtophys(((vm_offset_t)&ncs->target))
						  - (vm_offset_t)dummy;
				else
				    *p = kvtophys (arg);
			}
			break;
		      default:
			break;
		};
		p++;
	};
}

void ncr825_reset_chip(ncr825_softc_t ncs, char *why)
{
	printf("Resetting ncr%d: %s\n",ncs->ncr825_unit,why);

	OUTB(nc_istat,  SRST);			/* Reset the Chip	*/
	OUTB(nc_istat,  0   );
	OUTB(nc_scntl1, 0xc0);
	OUTB(nc_scntl2, 0x00);
#define SCNTL3 0x1b				/* SCF=1, CCF=2, WIDE	*/
	OUTB(nc_scntl3, SCNTL3);
	OUTB(nc_scid,   RRE|SRE|ncs->my_scsi_id);/* Enable re/selection	*/
#define	SXFER 0x08				/* ofs = 8, XFERP = 4	*/
	OUTB(nc_sxfer, SXFER );
	OUTB(nc_istat,  0x00);
#if 0
	OUTB(nc_ctest4, MPEE);			/* Check for memory errors */
#else
	OUTB(nc_ctest4, 0x00);
#endif
	OUTB(nc_dcntl,  NOCOM|(ncr825_ssm?SSM:0));
	OUTB(nc_respid, 1<<ncs->my_scsi_id);	/* id to respond to */
	OUTB(nc_stest1, 0x00);
	OUTB(nc_stest2, 0x00);
	OUTB(nc_stest3, TE);
	OUTL(nc_dsa, 0);
	OUTB(nc_dmode, 0x82);			/* BL = 8, BOF */
	OUTB(nc_stime0, STIME0);
	OUTB(nc_stime1, 0x00);			/* GEN = 0 */
	
	OUTW(nc_sien , HTH|MA|SGE|UDC|RST|STO);
	OUTB(nc_dien , MDPE|BF|ABRT|SSI|SIR|IID);
}

int ncr825_pci_attach(pcici_t config_id)
{
	int retval, unit, nunit, i;
	ncr825_softc_t ncs;
	scsi_softc_t *sc;
	target_info_t *tgt, *self;
	boolean_t memory_problems = FALSE;

	for(unit=0;unit<NSCSI;unit++)
	    if (scsi_softc[unit] == &scsi_softc_data[unit])
		if (scsi_softc_data[unit].hw_state == (char *)config_id.cfg1)
		    break;
	if (unit == NSCSI) {
		panic("ncr825_pci_attach");
		return (KERN_FAILURE);
	}
	nunit = ncr825_units++;

	printf("NCR 53C825");

	ncs = ncr825_softc[nunit] = (ncr825_softc_t) kalloc(
					  sizeof(struct ncr825_softc_struct));

	bzero((char *)ncs, sizeof(struct ncr825_softc_struct));

	simple_lock_init(&ncs->lock, ETAP_IO_CHIP);

	/*
	 * Enable Bus Mastering
	 */

	pci_conf_write(config_id, 0x04,
		       (pci_conf_read(config_id, 0x04) & 0xffff) | 6);

	/*
	 *	Try to map the controller chip to
	 *	virtual and physical memory.
	 */

	retval = pci_map_mem(config_id, 0x14, &ncs->vaddr, &ncs->paddr);

	if (retval) {
		printf("pci_map_mem failed 0x%x.\n", retval); 
		return (retval);
	};

	ncs->reg = (struct ncr_reg *)ncs->vaddr;
	ncs->tag = config_id;
	ncs->ncr825_unit = nunit;

#if 0
    {
	intpin = (pci_conf_read(config_id, 0x3c) >> 8) & 0xff;
	if (intpin) {
		printf(" irq %c", 0x60+intpin);
		/*XXX Hack to map PCI ints to ISA ints XXX*/
		take_irq(9+intpin, nunit, SPL5, (intr_t)ncr825_intr);
	} else {
		printf(" No irq");
	}
    }
#else
    {
	unsigned intinfo = pci_conf_read(config_id, 0x3c);
	unsigned int irq = intinfo & 0xf;
	unsigned int intpin = (intinfo >> 8) & 0x7;
	if (irq) {
		printf(" irq %d pin %c",irq, 0x60+intpin);
		take_irq(irq, nunit, SPL5, (intr_t)ncr825_intr);
	} else {
		printf(" No irq");
	}
    }
#endif

	/*
	 * Unfortunately, there is no way to get a non-default value here
	 * across reboots.  There is, however, a way to save the value
	 * we want for the scsid in the bios.  So, for now, we will depend
	 * on the ncr8251s all having the bios appear at a limited set
	 * of possible phsyical addresses.  We will check for the copyright
	 * string to find the board.
	 * 
	 */

	ncs->my_scsi_id = INB(nc_scid) & 0x07;

	for(ncs->bios_paddr =  0xc8000;
	    ncs->bios_paddr <= 0xd8000;
	    ncs->bios_paddr += 0x08000)
	    if (!strcmp("Copyright 1993 NCR Corporation.                  ",
			(char *)phystokv(ncs->bios_paddr + 0x1c))) break;

	if (strcmp("Copyright 1993 NCR Corporation.                  ",
		   (char *)phystokv(ncs->bios_paddr + 0x1c))) {
		printf("\nNo BIOS at expected location\n");
	} else {
		ncs->my_scsi_id = *((char *) phystokv(ncs->bios_paddr + 0x12));
	}

	printf(" BIOS 0x%x Scsi Id %d Version %d Type %d\n",
	       ncs->bios_paddr,
	       ncs->my_scsi_id,
	       INB(nc_ctest3)>>4,
	       INB(nc_macntl)>>4);

	if (INL(nc_ctest0) != pci_conf_read(config_id, 98)) {
		printf("***Bad Memory***\n");
		memory_problems = TRUE;
	}

	if (!memory_problems)
	    ncr825_reset_chip(ncs, "Attaching");

	ncs->vscript = (vm_offset_t)&ncr825_script0;
	ncs->pscript = kvtophys((vm_offset_t)&ncr825_script0);

	ncr825_script_bind(ncs, (ncrcmd*) ncs->vscript);

#define ASIZE ((MAX_SCSI_TARGETS + 1)*256)
	ncs->cmd_vector = (vm_offset_t *)kalloc(ASIZE+256);
	ncs->cmd_vector = (vm_offset_t *)
	    (((int)ncs->cmd_vector + 255) & (~255)); 
	ncs->pcmd_vector = kvtophys((vm_offset_t)ncs->cmd_vector);
	bzero((char *)ncs->cmd_vector, ASIZE);
	for(i=0;i<MAX_SCSI_TARGETS;i++)
	    ncs->cmd_vector[i]=kvtophys((vm_offset_t)ncs->cmd_vector+(i+1)*256);

	/*
	 * We now must make the memory in the softc structure, cmd_vector and
	 * the scripts code uncacheable so we can share it with
	 * the processor on the controller
	 */

        {
		vm_offset_t virt;
		vm_offset_t phys;

		for(virt = trunc_page(ncs), phys = kvtophys(virt);
		    virt < (vm_offset_t)ncs+sizeof(struct ncr825_softc_struct);
		    virt+=PAGE_SIZE,phys = kvtophys(virt)) 
		    pmap_map_bd(virt,
				phys,
				phys+PAGE_SIZE,
				VM_PROT_READ|VM_PROT_WRITE);
	}

	/*
	 * This is physically contiquous since we allocate at link time
	 */
	pmap_map_bd(trunc_page(ncs->vscript),
		    trunc_page(ncs->pscript),
		    round_page(ncs->pscript + sizeof(struct script)),
		    VM_PROT_READ|VM_PROT_WRITE);

	/*
	 * This is less then one page
	 */
	pmap_map_bd(trunc_page(ncs->cmd_vector),
		    trunc_page(ncs->pcmd_vector),
		    round_page(ncs->pcmd_vector + ASIZE),
		    VM_PROT_READ|VM_PROT_WRITE);


	/*
	 * The interrupt routine will enable the scripts processor
	 */

	if (!memory_problems) {
		ncs->script_running = TRUE;
		OUTB(nc_scntl1, CRST);
		ncr825_intr(nunit);
	}

	bzero((char *)ncs->commands,NCOMMANDS * sizeof(struct ncr825_command));

	ncr825_script0.tgt_vector[0] = (ncrcmd) ncs->pcmd_vector;

	ncs->allocated = 0;

	sc = scsi_master_alloc(unit, (char *)ncs);
	ncs->sc = sc;
	sc->go = ncr825_go;
	sc->watchdog = 0;
	sc->probe = ncr825_probe_target;
	sc->tgt_setup = ncr825_tgt_setup;
	sc->remote_op = ncr825_remote_op;
	sc->max_dma_data = -1;
	sc->initiator_id = ncs->my_scsi_id;

	self = ncr825_tgt_alloc(ncs, ncs->my_scsi_id, 0);
	sccpu_new_initiator(self, self);

	for (i=0;i<8;i++)
	    if (ncs->my_scsi_id != i) {
		    tgt = ncr825_tgt_alloc(ncs, i, 0);
		    sccpu_new_initiator(self, tgt);
		    tgt->dev_info.cpu.req_pending = FALSE;
		    tgt->flags |= TGT_PROBED_LUNS;
	    }

	printf("\n");
	return(KERN_SUCCESS);
}

target_info_t *ncr825_tgt_alloc(ncr825_softc_t	ncs,
				unsigned char	id,
				target_info_t	*tgt)
{
	if (tgt == 0)
	    tgt = scsi_slave_alloc(ncs->sc->masterno, id, (char *)ncs);

	ncr825_tgt_setup(tgt);
	return tgt;
}

void ncr825_tgt_setup(target_info_t *tgt)
{
	ncr825_command_t cmd;
	ncr825_softc_t ncs = (ncr825_softc_t)tgt->hw_state;

	simple_lock(&ncs->lock);
	cmd = &ncs->commands[ncs->allocated++];
	if (((int)cmd & ~0xff) == (int)cmd)
	    (int)cmd += sizeof(void *);
	while(kvtophys((vm_offset_t)cmd) + sizeof(struct ncr825_command) !=
	      kvtophys((vm_offset_t)cmd + sizeof(struct ncr825_command)))
	    cmd = &ncs->commands[ncs->allocated++];
	assert(ncs->allocated <= NCOMMANDS);
	simple_unlock(&ncs->lock);
	tgt->cmd_ptr = (char *)&cmd->cdb;
	tgt->dma_ptr = 0; /* and possibly here */
	cmd->lun_tbl.size = sizeof(cmd->lun);
	cmd->lun_tbl.addr = kvtophys((vm_offset_t)&cmd->lun);
	cmd->size_tbl.size = sizeof(cmd->size);
	cmd->size_tbl.addr = kvtophys((vm_offset_t)&cmd->size);
	cmd->select.sel_0 = 0;
	cmd->select.sel_sxfer = SXFER;
	cmd->select.sel_scntl3 = SCNTL3;
	cmd->ncs = ncs;
	cmd->tgt = tgt;
}

boolean_t ncr825_probe_target(target_info_t	*tgt,
			      io_req_t		ior)
{
	panic("ncr825_probe_target");
	return FALSE;
}

#define ncr825_cdb_to_cmd(cmd_ptr) \
    (ncr825_command_t)((int)(cmd_ptr) - \
		       offsetof(struct ncr825_command, cdb))

int ncr825_sg(ncr825_command_t	cmd,
	      int		ne,
	      vm_offset_t	virt,
	      vm_size_t		len)
{
	vm_offset_t l1,l2;
	int off;

	l1 = MACHINE_PGBYTES - (virt & (MACHINE_PGBYTES - 1));
	if (l1 > len)
	    l1 = len ;

	len -= l1;
	off = ne;
	while (1) {
		assert(off < MAXSG);
		while(len>0 && (kvtophys(virt) + l1 == kvtophys(virt + l1))) {
			l2 = (len > MACHINE_PGBYTES) ? MACHINE_PGBYTES : len;
			l1 += l2;
			len -= l2;
		}
		cmd->data_tbl[off].addr = kvtophys(virt);
		cmd->data_tbl[off].size = l1;
		cmd->move[2*off] = (cmd->passive?
				  SCR_TCHMOVE_TBL:SCR_CHMOVE_TBL) ^
				      SCR_DATA_OUT;
		cmd->move[2*off+1] = offsetof(struct ncr825_command,
					      data_tbl[off]);
		if (len <= 0)
		    break;
		virt += l1;
		off++;

		l1 = (len > MACHINE_PGBYTES) ? MACHINE_PGBYTES : len;
		len-= l1 ;
	}
	return (off-ne+1);
}

void ncr825_resize(ncr825_command_t	cmd,
		   vm_size_t		size)
{
	target_info_t *tgt = cmd->tgt;
	io_req_t ior = tgt->ior;
	int ne, len;
	TR_DECL("ncr825_resize");

	nstat_inc(resizes);
	tr4("enter: cmd = 0x%x old_size = 0x%x new_size = 0x%x",
	    kvtophys((vm_offset_t)cmd), cmd->size, size);

	assert(cmd->size > size);
	assert(cmd->passive);

	ior->io_residual = cmd->size - size;
	len = cmd->size = size;
	if (ior->io_op & IO_SCATTER) {
		io_scatter_t ios = (io_scatter_t)ior->io_data;
		ne = 0;
		while(len) {
			ne += ncr825_sg(cmd, ne, ios->ios_address,
					ios->ios_length);
			len -= ios->ios_length;
			ios++;
			assert (len >= 0);
		}
	} else {
		ne = ncr825_sg(cmd, 0, (vm_offset_t)ior->io_data, len);
	}
	cmd->move[(ne-1)*2] = SCR_TMOVE_TBL ^ SCR_DATA_OUT;
	cmd->move[ne*2] = SCR_JUMP;
	cmd->move[ne*2+1] = kvtophys(PADDR(target_done));
}

int ncr825_remote_op(target_info_t	*tgt,
		     int		op,
		     unsigned int	remote_pa,
		     char		*buffer,
		     unsigned int	count)
{
	io_req_t ior = tgt->ior;
	ncr825_command_t cmd = ncr825_cdb_to_cmd(tgt->cmd_ptr);
	ncr825_softc_t ncs = (ncr825_softc_t)tgt->hw_state;
	spl_t s;
	TR_DECL("ncr825_remote_op");
	at386_io_lock_state();

	tr5("op = 0x%x remote_pa = 0x%x buffer = 0x%x count = 0x%x",
	    op, remote_pa, buffer, count);

	at386_io_lock(MP_DEV_WAIT);

	assert(ior);

	cmd->reselect = FALSE;
	cmd->select.sel_id = tgt->target_id;
	cmd->lun = 0xff;
	cmd->size = count;
	cmd->hth_count = 0;
	cmd->sto_count = 0;

	cmd->data_tbl[0].addr = remote_pa;
	cmd->move[0] = SCR_MOVE_ABS(4) ^ SCR_MSG_OUT;
	cmd->move[1] = (ncrcmd)kvtophys((vm_offset_t)&cmd->data_tbl[0].addr);

	cmd->move[2] = SCR_REG_REG(scntl2, SCR_AND, 0x7f);
	cmd->move[3] = 0;

	cmd->data_tbl[1].addr = kvtophys((vm_offset_t)buffer);
	cmd->data_tbl[1].size = count;
	cmd->move[4] = SCR_MOVE_TBL ^ SCR_DATA_IN;
	cmd->move[5] = offsetof(struct ncr825_command, data_tbl[1]);

	cmd->move[6] = SCR_WAIT_DISC;
	cmd->move[7] = 0;

	if (op && 8) {
		cmd->move[8] = SCR_COPY(4);
		cmd->move[9] = kvtophys((vm_offset_t)&ncr825_zeros);
		cmd->move[10] = kvtophys((vm_offset_t)&cmd->size);
		cmd->move[11] = SCR_COPY(4);
		cmd->move[12] = kvtophys((vm_offset_t)&ncr825_zeros);
		cmd->move[13] = REG(dsa);
		cmd->move[14] = SCR_JUMP;
		cmd->move[15] = ncs->pscript;
		ncr825_enqueue_command(cmd);
		at386_io_unlock();
		while(cmd->size);
		tr2("Remote OP Complete: cmd = 0x%x",
		    kvtophys((vm_offset_t)cmd));
		at386_io_lock(MP_DEV_WAIT);
		ncr825_dequeue_command(cmd);
		s = splbio();
		ncs->executing_command = FALSE;
		ncr825_next_command(ncs);
		splx(s);
	} else {
		cmd->move[8] = SCR_INT;
		cmd->move[9] = INT_Remote_OP_Complete;
		ncr825_enqueue_command(cmd);
	}
	at386_io_unlock();
	return 0;
}

void ncr825_go(target_info_t	*tgt,
	       unsigned int	cmd_count,
	       unsigned int	in_count,
	       boolean_t	cmd_only)
{
	vm_offset_t		virt;
	int			len;
	ncr825_softc_t ncs = (ncr825_softc_t)tgt->hw_state;
	int tid = tgt->target_id ;
	io_req_t ior = tgt->ior;
	boolean_t sg;
	ncr825_command_t cmd = ncr825_cdb_to_cmd(tgt->cmd_ptr);
	int ne, index;

	TR_DECL("ncr825_go");
	at386_io_lock_state();

	tr2("cmd = 0x%x",kvtophys((vm_offset_t)cmd));

	at386_io_lock(MP_DEV_WAIT);
	assert(ior);
	assert(tgt->cur_cmd == SCSI_CMD_SEND);

	sg = (ior->io_op & IO_SCATTER)?TRUE:FALSE;
	cmd->passive = (ior->io_op & IO_PASSIVE)?TRUE:FALSE;
	len = ior->io_count;
#ifdef not_needed
	tgt->transient_state.cmd_count = cmd_count;
	tgt->transient_state.out_count = (cmd->passive?0:ior->io_count);
	tgt->transient_state.in_count = in_count;
#endif
	assert(!cmd->passive || in_count == ior->io_count);
	tgt->done = SCSI_RET_IN_PROGRESS;
	virt = (vm_offset_t)ior->io_data;
	cmd->reselect = FALSE;
	cmd->select.sel_id = tid;
	cmd->lun = tgt->lun;
	cmd->size = len;
	cmd->hth_count = 0;
	cmd->sto_count = 0;

	if (sg) {
		io_scatter_t ios = (io_scatter_t)virt;
		ne = 0;
		while(len) {
			ne += ncr825_sg(cmd, ne, ios->ios_address,
					ios->ios_length);
			len -= ios->ios_length;
			ios++;
			assert (len >= 0);
		}
	} else {
		ne = ncr825_sg(cmd, 0, virt, len);
	}
	cmd->move[(ne-1)*2] = (cmd->passive?SCR_TMOVE_TBL:SCR_MOVE_TBL) ^
	    SCR_DATA_OUT;

	cmd->move[ne*2] = SCR_JUMP;
	if (!cmd->passive) {
		nstat_inc(imode_cmds);
		cmd->move[ne*2+1] = kvtophys(PADDR(initiator_done));
		assert(ncs->disc_cmds[cmd->lun+(MAX_LUNS*tid)] == CMD_NULL);
		ncr825_enqueue_command(cmd);
	} else {
		spl_t s;
		nstat_inc(tmode_cmds);
		cmd->move[ne*2+1] = kvtophys(PADDR(target_done));
		s  = splbio();
		index = cmd->lun + (MAX_LUNS*tid);
		if (ncs->ptmode_cmds[index] != 0) {
			splx(s);
			tr4("pending operation: tgt 0x%x lun 0x%x size=0x%x",
			    cmd->select.sel_id, cmd->lun, cmd->size);
			if ((int)ncs->ptmode_cmds[index] < cmd->size)
			    ncr825_resize(cmd, 
					  (vm_size_t)ncs->ptmode_cmds[index]);
			ncs->ptmode_cmds[index] = 0;
			cmd->reselect = TRUE;
			ncr825_enqueue_command(cmd);
		} else {
			vm_offset_t *cmd_array;
			tr4("passive operation: tgt 0x%x lun 0x%x size=0x%x",
			    cmd->select.sel_id, cmd->lun, cmd->size);
			ncs->tmode_cmds[index] = cmd;
			cmd_array=(vm_offset_t *)phystokv(ncs->cmd_vector[tid]);
			cmd_array[cmd->lun]=kvtophys((vm_offset_t)cmd);
			splx(s);
		}
	}
	at386_io_unlock();
}

void ncr825_next_command(ncr825_softc_t ncs)
{
	ncr825_command_t cmd;
	TR_DECL("ncr825_next_command");

	/* Called at splbio */
	if (!ncs->executing_command)
	    if ((cmd = ncs->new_cmd)) {
		    ncs->executing_command = TRUE;
		    cmd = (ncr825_command_t)phystokv(cmd);
		    tr2("cmd = 0x%x",kvtophys((vm_offset_t)cmd));
		    tr5("executing target 0x%x lun 0x%x size 0x%x %sReselect",
			cmd->select.sel_id, cmd->lun, cmd->size,
			BOOLP(cmd->reselect));
		    assert(cmd->size != 0);
		    if (cmd->reselect)
			OUTB(nc_istat, SIGP|SEM);
		    else
			OUTB(nc_istat, SIGP);
	    }
	if (!ncs->script_running) {
	    ncs->script_running = TRUE;
	    OUTL(nc_dsp, ncs->pscript);
	}
}

void ncr825_dequeue_command(ncr825_command_t cmd)
{
	ncr825_softc_t ncs = cmd->ncs;
	TR_DECL("ncr825_dequeue_command");

	/* Called at splbio */
	tr2("cmd = 0x%x",kvtophys((vm_offset_t)cmd));
	assert(ncs->new_cmd == (ncr825_command_t)kvtophys((vm_offset_t)cmd));
	ncs->new_cmd = cmd->next;
}

void ncr825_enqueue_command(ncr825_command_t cmd)
{
	ncr825_softc_t ncs = cmd->ncs;
	spl_t s;
	ncr825_command_t *ncmd;
	TR_DECL("ncr825_enqueue_command");

	tr3("queuing: cmd = 0x%x size = 0x%x",kvtophys((vm_offset_t)cmd),
	    cmd->size);
	cmd->next = CMD_NULL;
	s = splbio();
	ncmd = &ncs->new_cmd;
	while(*ncmd)
	    ncmd = &(((ncr825_command_t)phystokv(*ncmd))->next);
	*ncmd = (ncr825_command_t) kvtophys((vm_offset_t)cmd);
	ncr825_next_command(ncs);
	splx(s);
}

void ncr825_UDC(ncr825_softc_t ncs, unsigned char dstat)
{
	vm_offset_t pcmd;
	ncr825_command_t cmd;
	int index;
	TR_DECL("ncr825_UDC");

	nstat_inc(disconnects);
	pcmd = INL(nc_dsa);
	cmd = (ncr825_command_t)phystokv(pcmd);
	assert(pcmd != (vm_offset_t)CMD_NULL);
	tr5("Disconnect cmd 0x%x target 0x%x lun 0x%x size 0x%x",
	    pcmd, cmd->select.sel_id, cmd->lun, cmd->size);
	OUTL(nc_dsa, 0);
	index = cmd->lun + (MAX_LUNS*cmd->select.sel_id);
	assert(ncs->disc_cmds[index] == CMD_NULL);
	ncs->disc_cmds[index] = cmd;
	/*
	 * We may be disconnecting in the middle of reselected command
	 * in which case the cmd will not be on the queue.  So check first.
	 */
	if (phystokv(ncs->new_cmd) == (vm_offset_t)cmd)
	    ncr825_dequeue_command(cmd);
}

unsigned int ncr825_spurious_count = 0;
#if	MACH_ASSERT
unsigned int ncr825_spurious_debug = 5;
#endif	/* MACH_ASSERT */

void ncr825_intr(int unit)
{
	ncr825_softc_t ncs = ncr825_softc[unit];
	unsigned char dstat;
	unsigned char num;
	unsigned char istat = INB(nc_istat);
	unsigned short sist;
	ncr825_command_t cmd;
	vm_offset_t pcmd;
	int index;
	boolean_t do_next_command;

	TR_DECL("ncr825_intr");
	at386_io_lock_state();

	at386_io_lock(MP_DEV_WAIT);
	OUTB(nc_stime0, STIME0);
	if (!(istat & (INTF|SIP|DIP))){
		tr1("Spurious");
		ncr825_spurious_count++;
#if	MACH_ASSERT && MACH_KDB
		if ((ncr825_spurious_count % ncr825_spurious_debug) == 0)
		    Debugger("Spurious Interrupts");
#endif	/* MACH_ASSERT && MACH_KDB */
		goto done;
	}
	assert(ncs->script_running);
	while (istat & (INTF|SIP|DIP)) {
		do_next_command = FALSE;
		ncs->script_running = FALSE;
		ncs->executing_command = FALSE;
#if MACH_ASSERT
		if (istat & INTF) {
			tr1("INTF");
#if	MACH_KDB
			ncr825_dump(ncs, INB(nc_dstat), INW(nc_sist),
				    (print_routine_t)printf);
			Debugger("INTF");
#endif	/* MACH_KDB */
			ncs->script_running = TRUE;
			OUTL(nc_dsp, ncs->pscript);
		}
		
		if (istat & CABRT) {
			tr1("CABRT");
			OUTB(nc_istat, INB(nc_istat)&~CABRT);
		}
#endif	MACH_ASSERT
		
		sist = INW(nc_sist);
		dstat = INB(nc_dstat);
		
		tr4("istat 0x%x sist 0x%x dstat 0x%x",istat,sist,dstat);

		if (!(sist&(MA|UDC|STO|SGE|GEN|PAR|RST|HTH)) &&
		    !(dstat&(IID|ABRT|SSI)) &&
		    (dstat & (DFE|SIR)) == (DFE|SIR))
		    goto sir;

		if (!(dstat & DFE)) {
			OUTB(nc_ctest3,INB(nc_ctest3)|CLF);
			OUTB(nc_stest3,INB(nc_stest3)|CSF);
		}

		if (sist & MA) {
			/*
			 * We expect to get here when we have a size
			 * mismatch.  If we are sending too little,
			 * then we assert SATN/ to wakeup
			 * the target; otherwise, the target will just receive
			 * the bytes it can and then switch to MSG_IN
			 * which will trigger the MA.  We now just want
			 * to continue with initiator_done and get the
			 * size actually transfered from the target
			 */
			nstat_inc(mismatch);
			pcmd = INL(nc_dsa);
			cmd = (ncr825_command_t)phystokv(pcmd);
			assert(cmd != CMD_NULL);
			tr5("MA cmd 0x%x target 0x%x lun 0x%x size 0x%x",
			    pcmd, cmd->select.sel_id, cmd->lun, cmd->size);
			ncs->script_running = TRUE;
			ncs->executing_command = TRUE;
			if (cmd->passive) {
				tr1("passive");
				OUTL(nc_dsp, ncs->pscript +
				     offsetof(struct script,
					      target_done));
			} else {
				if(INL(nc_dsp) < ncs->pscript || INL(nc_dsp) >
				   ncs->pscript + sizeof(struct script)) {
					/* Sending too much */
					tr1("too much");
					OUTL(nc_dsp, ncs->pscript +
					     offsetof(struct script,
						      initiator_done));
				} else {
					/* Sending too little */
					tr1("too little");
					OUTL(nc_dsp, ncs->pscript +
					     offsetof(struct script,
						      attention));
				}
			}
		}

		if (sist & UDC) {
			/* 
			 * We expect to get here when we are the initiator
			 * and the target disconnects because it cannot handle
			 * the command yet
			 */
			ncr825_UDC(ncs, dstat);
			do_next_command = TRUE;
		}

		if (sist & STO) {
			tr1("STO");
			if ((pcmd = INL(nc_dsa))) {
				cmd = (ncr825_command_t)phystokv(pcmd);
				tr2("active cmd 0x%x",cmd);
				if(cmd->sto_count++ > 4) {
#if	MACH_KDB
					Debugger("STO");
					if (cmd->sto_count > 6) {
						cmd->tgt->ior->io_error =
						    KERN_FAILURE;
						goto Command_Done;
					}
#else	/* MACH_KDB */
					cmd->tgt->ior->io_error = KERN_FAILURE;
					goto Command_Done;
#endif	/* MACH_KDB */
				}
			}
			do_next_command = TRUE;
		}

		if (sist & SGE) {
			tr1("SGE");
			OUTB(nc_stest2, ROF);
			if(!(INB(nc_scntl0) & TRG) &&
			   (pcmd = INL(nc_dsa)) &&
			   pcmd != (vm_offset_t)ncs->new_cmd)
			    ncr825_UDC(ncs, dstat);
			ncr825_reset_chip(ncs, "SGE");
			do_next_command = TRUE;
		}

		if (dstat & IID) {
			tr1("IID");
			assert(FALSE);
			if(!(INB(nc_scntl0) & TRG) &&
			   (pcmd = INL(nc_dsa)) &&
			   pcmd != (vm_offset_t)ncs->new_cmd)
			    ncr825_UDC(ncs, dstat);
			ncr825_reset_chip(ncs, "IID");
			do_next_command = TRUE;
		}

		if (dstat & ABRT) {
			tr1("ABRT");
			if(!(INB(nc_scntl0) & TRG) &&
			   (pcmd = INL(nc_dsa)) &&
			   pcmd != (vm_offset_t)ncs->new_cmd)
			    ncr825_UDC(ncs, dstat);
			ncr825_reset_chip(ncs, "IID");
			do_next_command = TRUE;
		}

#if	MACH_KDB
		if ((sist&(GEN|PAR)) ||
		    (dstat&(MDPE|BF))) {
			tr1("Bad Interrupt");
			ncr825_dump(ncs, dstat, sist,
				    (print_routine_t)printf);
			Debugger("Bad Interrupt");
			do_next_command = TRUE;
		}
#endif	/* MACH_KDB */
		
		if (sist & RST) {
			nstat_inc(reset);
			tr1("RST");
			OUTB(nc_scntl1, INB(nc_scntl1)&~CRST);
			pcmd = INL(nc_dsa);
			do_next_command = TRUE;
			if (pcmd) {
				cmd = (ncr825_command_t)phystokv(pcmd);
				if (pcmd == (vm_offset_t)ncs->new_cmd)
				    ncr825_dequeue_command(cmd);
				if (cmd->passive) {
					cmd->reselect = TRUE;
					ncr825_enqueue_command(cmd);
					do_next_command = FALSE;
				} else {
					ncr825_UDC(ncs, dstat);
				}
			}
		}
		
		if (sist & HTH) {
			tr1("HTH");
			assert(INB(nc_scntl0) & TRG);
			if ((pcmd = INL(nc_dsa))) {
				cmd = (ncr825_command_t)phystokv(pcmd);
				tr2("active cmd 0x%x",cmd);
				if(cmd->hth_count++ > 4) {
#if	MACH_KDB
					Debugger("HTH");
					if (cmd->hth_count > 6) {
						cmd->tgt->ior->io_error =
						    KERN_FAILURE;
						goto Command_Done;
					}
#else	MACH_KDB
					cmd->tgt->ior->io_error = KERN_FAILURE;
					goto Command_Done;
#endif	/* MACH_KDB */
				}
			}
			/*
			 * What we want to do here is force a disconnect
			 * and restart the command.
			 */
			OUTB(nc_scntl1,INB(nc_scntl1)&~ISCON);
			/*
			 * It appears that the board will keep driving
			 * the BSY/ signal even after forcing a disconnect.
			 * We need to stop this board from driving any signals
			 * at this time.
			 */
			OUTB(nc_socl, 0);
			do_next_command = TRUE;
		}

		if (dstat & SSI) {
#if	MACH_KDB
			if (ncr825_ssm&8)
			    printf("SSI: 0x%x\n", INL(nc_dsp));
			if (ncr825_ssm&4)
			    ncr825_dump(ncs, dstat, sist,
					(print_routine_t)printf);
			if (ncr825_ssm&2)
			    Debugger("SSI");
#endif	/* MACH_KDB */
			if (!(dstat & SIR))
			    if (!ncs->script_running) {
				    ncs->script_running = TRUE;
				    OUTB(nc_dcntl,INB(nc_dcntl)|STD);
			    }
		}
		

		if (!(dstat & SIR))
		    goto loop;

	      sir:
		num = INB(nc_dsps);
		
		switch (num) {
		      case INT_Target_Mode:
			nstat_inc(tmode_intr);
			tr4("Target_Mode: tgt 0x%x lun 0x%x size 0x%x",
			    ncs->target, ncs->lun, ncs->size);
			index = ncs->lun + (MAX_LUNS*ncs->target);
			cmd = ncs->tmode_cmds[index];
			if (cmd != CMD_NULL) {
				tr2("cmd = 0x%x",kvtophys((vm_offset_t)cmd));
				if (cmd->size > ncs->size)
				    ncr825_resize(cmd, ncs->size);
				OUTL(nc_dsa, kvtophys((vm_offset_t)cmd));
				OUTB(nc_stime0, STIME0_HTH);
				assert(INB(nc_scntl0) & TRG);
				ncs->script_running = TRUE;
				ncs->executing_command = TRUE;
				OUTL(nc_dsp, kvtophys((vm_offset_t)cmd));
			} else {
				nstat_inc(tmode_disc);
				ncs->ptmode_cmds[index] = ncs->size;
				tr1("Disconnecting");
				ncs->script_running = TRUE;
				OUTL(nc_dsa, 0);
				ncr825_next_command(ncs);
				OUTL(nc_dsp, ncs->pscript +
				     offsetof(struct script, disconnect));
			}
			break;
		      case INT_Command_Complete:
			pcmd = INL(nc_dsa);
			tr2("Command Complete: cmd = 0x%x",pcmd);
			cmd = (ncr825_command_t)phystokv(pcmd);
		      Command_Done:
			assert(cmd->ncs == ncs);
			OUTL(nc_dsa, 0);
			if (pcmd == (vm_offset_t)ncs->new_cmd)
			    ncr825_dequeue_command(cmd);
			if (cmd->passive) {
				vm_offset_t *cmd_array;
				index =cmd->lun+(MAX_LUNS*cmd->tgt->target_id);
				ncs->tmode_cmds[index] = CMD_NULL;
				((struct script *)ncs->vscript)
				    ->tgt_vector[0] = (ncrcmd)ncs->pcmd_vector;
				cmd_array = (vm_offset_t *)
				    phystokv(ncs->cmd_vector
					     [cmd->tgt->target_id]);
				cmd_array[cmd->lun]=0;
				if (cmd->reselect)
				    ncs->size = cmd->size;
				tr4("io_residual(0x%x) += 0x%x - 0x%x",
				    cmd->tgt->ior->io_residual,
				    cmd->size,
				    ncs->size);
				cmd->tgt->ior->io_residual +=
				    cmd->size - ncs->size;
			} else {
				tr3("io_residual = 0x%x - 0x%x", cmd->size,
				    ncs->size);
				cmd->tgt->ior->io_residual =
				    cmd->size - ncs->size;
			}
			ncr825_next_command(ncs);
			assert(cmd->tgt->ior == (io_req_t)(cmd->tgt + 1));
			(*cmd->tgt->dev_ops->restart)(cmd->tgt, TRUE);
			break;
		      case INT_Reselected:
			nstat_inc(reselected);
			tr3("Reselected: tgt 0x%x lun 0x%x", ncs->target,
			    ncs->lun);
			index = ncs->lun + (MAX_LUNS*ncs->target);
			cmd = ncs->disc_cmds[index];
			tr2("cmd = 0x%x",kvtophys((vm_offset_t)cmd));
			assert(cmd != CMD_NULL);
			ncs->disc_cmds[index] = CMD_NULL;
			assert(!(INB(nc_scntl0)&TRG));
			OUTL(nc_dsa, kvtophys((vm_offset_t)cmd));
			ncs->script_running = TRUE;
			ncs->executing_command = TRUE;
			OUTL(nc_dsp, kvtophys((vm_offset_t)cmd));
			break;
		      case INT_Remote_OP_Complete:
			pcmd = INL(nc_dsa);
			tr2("Remote OP Complete: cmd = 0x%x",pcmd);
			cmd = (ncr825_command_t)phystokv(pcmd);
			assert(cmd->ncs == ncs);
			OUTL(nc_dsa, 0);
			assert(pcmd == (vm_offset_t)ncs->new_cmd);
			ncr825_dequeue_command(cmd);
			ncr825_next_command(ncs);
			assert(cmd->tgt->ior == (io_req_t)(cmd->tgt + 1));
			(*cmd->tgt->dev_ops->restart)(cmd->tgt, TRUE);
			break;
		      case INT_Bad_DSA:
			tr1("Bad DSA");
			Debugger("Bad DSA");
			do_next_command = TRUE;
			break;
		      case INT_Confused:
			tr1("Confused");
			assert(FALSE);
		      default:
			tr1("Very Confused");
			assert(FALSE);
		}
	      loop:
		istat = INB(nc_istat);
	}
	if (!ncs->script_running && do_next_command)
	    ncr825_next_command(ncs);
      done:
	at386_io_unlock();
}

struct	pci_driver ncr825device = {
	ncr825_pci_probe,
	ncr825_pci_attach,
	OLD_NCR_ID,
	NCR_825,
	"ncr",
	"ncr 53C825 scsi",
	ncr825_intr
};

#if	MACH_KDB

#define BOOLP(x) ((x)?"":"!")

void ncr825_bits(char *r,
		 char *s0,
		 char *s1,
		 char *s2,
		 char *s3,
		 char *s4,
		 char *s5,
		 char *s6,
		 char *s7,
		 unsigned char c,
		 print_routine_t printf)
{
	(*printf)("  %6s: %s%s %s%s %s%s %s%s %s%s %s%s %s%s %s%s\n", 
	       r,
	       BOOLP(c&0x80),s0,
	       BOOLP(c&0x40),s1,
	       BOOLP(c&0x20),s2,
	       BOOLP(c&0x10),s3,
	       BOOLP(c&0x08),s4,
	       BOOLP(c&0x04),s5,
	       BOOLP(c&0x02),s6,
	       BOOLP(c&0x01),s7);
}

void ncr825_dump(ncr825_softc_t ncs,
		 unsigned char dstat,
		 unsigned short sist,
		 print_routine_t printf)
{
	int i,j,k;

	(*printf)("NCR unit %d SCSI id %d softc 0x%x regs 0x%x script 0x%x\n",
	       ncs->ncr825_unit, ncs->my_scsi_id, ncs, ncs->reg, ncs->vscript);
	(*printf)("Target 0x%x Lun 0x%x Size 0x%x Cmd 0x%x cmd_vector 0x%x\n",
		  ncs->target, ncs->lun, ncs->size, ncs->new_cmd,
		  ncs->cmd_vector);
	(*printf)("NCOMMANDS 0x%x allocated 0x%x\n", NCOMMANDS,
		  ncs->allocated);
	(*printf)("Scripts Processor is %sRunning and Command %sExecuting\n",
		  BOOLP(ncs->script_running), BOOLP(ncs->executing_command));
	(*printf)("Tmode Commands:");
	k=0;
	for(i=0;i<MAX_SCSI_TARGETS;i++)
	    for(j=0;j<MAX_LUNS;j++) {
		    if (ncs->tmode_cmds[k])
			(*printf)(" [%d/%d]0x%x",i,j,ncs->tmode_cmds[k]);
		    k++;
	    }
	(*printf)("\nPTmode Commands:");
	k=0;
	for(i=0;i<MAX_SCSI_TARGETS;i++)
	    for(j=0;j<MAX_LUNS;j++) {
		    if (ncs->ptmode_cmds[k])
			(*printf)(" [%d/%d]0x%x",i,j,ncs->ptmode_cmds[k]);
		    k++;
	    }
	(*printf)("\nDisconnected Commands:");
	k=0;
	for(i=0;i<MAX_SCSI_TARGETS;i++)
	    for(j=0;j<MAX_LUNS;j++) {
		    if (ncs->disc_cmds[k])
			(*printf)(" [%d/%d]0x%x",i,j,ncs->disc_cmds[k]);
		    k++;
	    }
	(*printf)("\n");
	ncr825_bits("istat","CABRT","SRST","SIGP","SEM","CON","INTF",
		    "SIP","DIP", INB(nc_istat), printf);
	ncr825_bits("scntl0","ARB1","ARB0","START","WATN","EPC","R",
		    "AAP","TRG",INB(nc_scntl0), printf);
	ncr825_bits("scntl1","EXC","ADB","DHP","CON","RST","AESP","IARB",
		    "SST",INB(nc_scntl1), printf);
	ncr825_bits("scntl2","SDU","CHM","R","R","WSS","R","R","WSR",
		    INB(nc_scntl2), printf);
	(*printf)("  scntl3: SCF=%d, %sEWS",(INB(nc_scntl3)>>4)&0x7,
	       BOOLP(INB(nc_scntl3)&0x8));
	(*printf)("    scid: %sRRE %sSRE id 0x%x\n", BOOLP(INB(nc_scid)&0x40),
	       BOOLP(INB(nc_scid)&0x20), INB(nc_scid)&0xf);
	(*printf)("   sxfer: TP=%d MO=%d",INB(nc_sxfer)>>5,
		  INB(nc_sxfer)&0xf);
	(*printf)("    sdid: ENCD=0x%x",INB(nc_sdid)&0xf);
	(*printf)("    sfbr: 0x%x\n", INB(nc_sfbr));
	ncr825_bits("sbcl","SREQ/","SACK/","SBSY/","SSEL/","SATN/",
		    "SMSG/","SC_D/","SI_O/",INB(nc_sbcl), printf);
	ncr825_bits("dstat","DFE","MDPE","BF","ABRT","SSI","SIR","R","IID",
		    dstat, printf);
	ncr825_bits("sstat0","ILF","ORF","OLF","AIP","LOA","WOA","RST/",
		    "SDP0/",INB(nc_sstat0), printf);
	(*printf)("sstat1 SCSI FIFO %d %sSDP0L %sMSG/ %sC/D/ %sI/O/\n",
		  INB(nc_sstat1)>>4, BOOLP(INB(nc_sstat1)&SDP0L),
		  BOOLP(INB(nc_sstat1)&CMSG), BOOLP(INB(nc_sstat1)&CC_D),
		  BOOLP(INB(nc_sstat1)&CI_O));
	ncr825_bits("sstat2","ILF1","ORF1","OLF1","R","SPL1","R","LDSC","SDP1",
		    INB(nc_sstat2), printf);
	ncr825_bits("ctest2","DDIR","SIGP","CIO","CM","R","TEOP","DREQ",
		    "DACK", INB(nc_ctest2), printf);
	(*printf)("     dsa: 0x%x",INL(nc_dsa));
	(*printf)("     dbc: 0x%x",INL(nc_dbc));
	(*printf)("    dnad: 0x%x\n",INL(nc_dnad));
	(*printf)("     dsp: 0x%x",INL(nc_dsp));
	(*printf)("    dsps: 0x%x",INL(nc_dsps));
	(*printf)("   dfifo: 0x%x\n",INL(nc_dfifo));
	ncr825_bits("dien","R","MDPE","BF","ABRT","SSI","SIR","R","IID",
		    INB(nc_dien), printf);
	(*printf)("   dcntl: %sSSM %sCOM\n",BOOLP(INB(nc_dcntl)&SSM),
		  BOOLP(!(INB(nc_dcntl)&NOCOM)));
	ncr825_bits("sien0","M/A","CMP","SEL","RSL","SGE","UDC","RST","PAR",
		    INB(nc_sien)&0xff, printf);
	(*printf)("   sien1: %sSTO %sGEN %sHTH\n", BOOLP(INB(nc_sien)&STO),
	       BOOLP(INB(nc_sien)&GEN), BOOLP(INB(nc_sien)&HTH));
	ncr825_bits("sist0","M/A","CMP","SEL","RSL","SGE","UDC","RST","PAR",
		    sist&0xff, printf);
	(*printf)("   sist1: %sSTO %sGEN %sHTH\n", BOOLP(sist&STO),
	       BOOLP(sist&GEN), BOOLP(sist&HTH));
	(*printf)("  respid: 0x%x\n",INW(nc_respid));
	(*printf)("  stest0: %sSLT %sART %sSOZ %sSOM\n", BOOLP(INB(nc_stest0)&SLT),
		  BOOLP(INB(nc_stest0)&ART),BOOLP(INB(nc_stest0)&SOZ),
		  BOOLP(INB(nc_stest0)&SOM));
}

void ncr825_debug(int unit)
{
	ncr825_softc_t ncs = ncr825_softc[unit];
	ncr825_dump(ncs, INB(nc_dstat), INW(nc_sist),
		    (print_routine_t)db_printf);
}

void ncr825_abort(int unit)
{
	ncr825_softc_t ncs = ncr825_softc[unit];
	OUTB(nc_istat, INB(nc_istat)|CABRT);
}
#endif	/* MACH_KDB */
#endif	/* NNCR825 > 0 */
