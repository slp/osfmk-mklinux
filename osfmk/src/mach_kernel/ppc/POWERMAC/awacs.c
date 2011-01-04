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
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */

#include <mach_kgdb.h>

#include <platforms.h>

#include <mach_kdb.h>
#include <kgdb/gdb_defs.h>
#include <kgdb/kgdb_defs.h>     /* For kgdb_printf */

#include <kern/spl.h>
#include <mach/std_types.h>
#include <types.h>
#include <sys/syslog.h>
#include <device/io_req.h>
#include <device/tty.h>
#include <device/ds_routines.h>
#include <chips/busses.h>
#include <ppc/misc_protos.h>
#include <ppc/exception.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/interrupts.h>
#include <ppc/io_map_entries.h>
#include <vm/vm_kern.h>

#include <ppc/POWERMAC/device_tree.h>
#include <ppc/POWERMAC/dbdma.h>
#include <ppc/POWERMAC/awacs.h>
#define AWACS_PDM_EXTERNS
#include <ppc/POWERMAC/awacs_pdm.h>

static void scale_volume(int vol, int *left, int *right, int fOutput);
static void set_volume(awacs_devc_t *devc, int vol);
static void unscale_volume(int left, int right, int *vol, int fOutput);
static void awacs_write_CodecControlReg(awacs_devc_t *devc, volatile int value);
static void awacs_write_SoundControlReg(awacs_devc_t *devc, volatile int value);
static int awacs_read_CodecStatusReg(awacs_devc_t *devc);
static int awacs_read_ClippingCountReg(awacs_devc_t *devc);
static int awacs_check(void *);

#define SPLAUDIO SPLTTY   /* Caution!  Needs to match registered I/O level */
#define splaudio spltty


/*
 *
 */
#define NAWACS 1
struct bus_device *awacs_info[NAWACS];

caddr_t	awacs_std[NAWACS] = { (caddr_t) 0};

struct bus_driver awacs_driver =
{
	awacs_probe, 
	0, 
	awacs_attach, 
	(int(*)(void)) 0, 
	awacs_std, 
	"AWACS_PPC_MK", 
	awacs_info, 
	0, 
	0, 
	0
};

int num_awacs = 0;
awacs_dev_table_t awacs_dev_table[NAWACS];

int awacs_rates[] = 
{
	7350, 8820, 11025, 14700, 17640, 22050, 29400, 44100
};

ulong_t    dbdma_int_mask;

ulong_t buflog[8];
int     bufnum = 0;

/*
 *
 */
int 
awacs_probe(caddr_t port, void *ui)
{
	awacs_devc_t         *devc;
	vm_offset_t          p;
	ulong_t              i;
	dbdma_command_t      *s;
	device_node_t	     *awacs;

#if 0
	printf("\n\rawacs_probe() - %s %d\n\r", __FILE__, __LINE__);
#endif
	if (powermac_info.class == POWERMAC_CLASS_PDM)
		return awacs_pdm_probe(port, ui);
	kmem_alloc(kernel_map, (vm_offset_t *)&p, sizeof(awacs_devc_t));
	memset((void *)p, 0, sizeof(awacs_devc_t));
	devc = (awacs_devc_t *) p;
	simple_lock_init(&devc->lock, ETAP_IO_CHIP);

	switch (powermac_info.class) {
	case  POWERMAC_CLASS_PCI:
		awacs = find_devices("awacs");
		if (awacs == NULL) 
			return 0;

		devc->awacs_regs = (awacs_regmap_t *)POWERMAC_IO(awacs->addrs[0].address);
		devc->output_dbdma= (dbdma_regmap_t *) POWERMAC_IO(awacs->addrs[1].address);
		devc->input_dbdma = (dbdma_regmap_t *) POWERMAC_IO(awacs->addrs[2].address);

		pmac_register_ofint(awacs->intrs[1], SPLAUDIO, (void(*)(int,void *))awacs_output_int);
		pmac_register_ofint(awacs->intrs[2], SPLAUDIO, (void(*)(int,void *))awacs_input_int);

		break;

	default:
		return 0;
	}

	awacs_dev_table[num_awacs].dev  = 0;
	awacs_dev_table[num_awacs].devc = devc;
	num_awacs++;
      
	devc->is_present = TRUE;
	devc->SoundControl = (kInSubFrame0      |
				      kOutSubFrame0     |
				      kHWRate22050       );
	devc->rate = 22050;
	awacs_write_SoundControlReg(devc, devc->SoundControl);

	devc->CodecControl[0] = kCodecCtlAddress0 | kCodecCtlEMSelect0;
	devc->CodecControl[1] = kCodecCtlAddress1 | kCodecCtlEMSelect0;
	devc->CodecControl[2] = kCodecCtlAddress2 | kCodecCtlEMSelect0;
	devc->CodecControl[4] = kCodecCtlAddress4 | kCodecCtlEMSelect0;

	devc->CodecControl[0] |= (kCDInput | kDefaultCDGain /*| kMicInput | kDefaultMicGain*/);
	awacs_write_CodecControlReg( devc, devc->CodecControl[0]);

	/* Set initial volume[s] */
	set_volume(devc, (67)<<8|(67));
	devc->output_volume = (67)<<8|(67);
	devc->input_volume = (67)<<8|(67);

	/* Set loopback (for CD?) */
	devc->CodecControl[1] |= 0x440;
	awacs_write_CodecControlReg( devc, devc->CodecControl[1]);

	devc->status = (unsigned long) 0xFFFFFFFF;

	return 1;
}

/*
 *
 */
void 
awacs_attach(struct bus_device *dev)
{
#if 0
	printf("\n\rawacs_attach() - %s %d\n\r", __FILE__, __LINE__);
#endif
	if (powermac_info.class == POWERMAC_CLASS_PDM)
		awacs_pdm_attach(dev);
}

/*
 * Timeout routine which checks to see if the speakers/headphones
 * have changed state.
 */
static int
awacs_check(void *p)
{
	awacs_devc_t	*devc = (awacs_devc_t *)p;
	spl_t		x = splaudio();
	unsigned long status;
	simple_lock(&devc->lock);
	status = (unsigned long)awacs_read_CodecStatusReg(devc);
	if ((devc->status == (unsigned long)0xFFFFFFFF) ||
	    ((status & kHeadphoneSense) != (devc->status & kHeadphoneSense))) {
		devc->status = status;
		devc->CodecControl[1] &=  ~(kMuteInternalSpeaker | kMuteHeadphone);
		if (status & kHeadphoneSense) {
			/* Headphones/speakers are plugged in */
			devc->CodecControl[1] |=  kMuteInternalSpeaker;
		} else {
			/* Internal speaker */
			devc->CodecControl[1] |=  kMuteHeadphone;
		}
		awacs_write_CodecControlReg(devc, devc->CodecControl[1]); 
	}
	timeout((timeout_fcn_t)awacs_check, (void *)devc, hz/2);
	simple_unlock(&devc->lock);
	splx(x);
	return (0);
}


/*
 *
 */
void 
awacs_output_int(int device, struct ppc_saved_state *ssp)
{
	awacs_devc_t         *devc;
	int                  i;
	seq_num_t            *seq;
	io_req_t             ior;

	devc = awacs_dev_to_devc(0);
   
	simple_lock(&devc->lock);

	seq = &devc->output_seq;
 
	while(seq->num_active) {
		i = seq->seq_num_exp;
		ior = (io_req_t) devc->output_cmds[i].desc_stop.d_address;
#if 0
		printf("Output interrupt - buf: %d, ior: %x\n", i, (int)ior);
#endif
		if (ior) {
			devc->output_cmds[i].desc_stop.d_address = (int)NULL;
			iodone(ior);
		}

		seq->num_active--;

		if (++seq->seq_num_exp >= seq->max_buffers) {
			seq->seq_num_exp = 0;
		}
		if (*seq->seq_num_virt == i) {
			break;
		}
	}
	simple_unlock(&devc->lock);
}

/*
 *
 *
 */
void 
awacs_input_int(int device, struct ppc_saved_state *ssp)
{
	awacs_devc_t         *devc;
	int                  i;
	seq_num_t            *seq;
	io_req_t             ior;

	devc = awacs_dev_to_devc(0);
   
	simple_lock(&devc->lock);

	seq = &devc->input_seq;
 
	while(seq->num_active) {
		i = seq->seq_num_exp;
		ior = (io_req_t) devc->input_cmds[i].desc_stop.d_address;
		if (ior) {
			iodone(ior);
		}

		seq->num_active--;

		if (++seq->seq_num_exp >= seq->max_buffers) {
			seq->seq_num_exp = 0;
		}
		if (*seq->seq_num_virt == i) {
			break;
		}
	}
	simple_unlock(&devc->lock);
}

/*
 *
 */
io_return_t 
awacs_open(int dev, dev_mode_t mode, io_req_t ior)
{
	awacs_devc_t	*devc;
	dbdma_regmap_t	*dmap;
	dbdma_command_t	*s;
	int		i;
	int		max_buffers;
	spl_t		x = splaudio();

#if 0
	printf("awacs_open() - %s %d\n\r", __FILE__, __LINE__);
#endif
	if (powermac_info.class == POWERMAC_CLASS_PDM)
		return awacs_pdm_open(dev, mode, ior);

	devc = awacs_dev_to_devc(dev);

	if (devc == NULL || devc->is_present == FALSE) {
		splx(x);
		return D_NO_SUCH_DEVICE;
	}

	simple_lock(&devc->lock);

	/*---------------------------------------*/
	/*    Init audio input DBDMA pointers    */
	/*---------------------------------------*/
	/*
	 * Allocate space for audio commands
	 *
	 * Each audio command consists of 3 DBDMA descriptors:
	 * 1. Data Xfer  - xfer data to/from device
	 * 2. Store Quad - record audio cmd #
	 * 3. Stop/Nop   - halt/continue to next buffer
	 */

	dmap = devc->input_dbdma;
	dbdma_reset(dmap);
  
	if (!devc->input_cmds) {
		simple_unlock(&devc->lock);
		devc->input_cmds  = (audio_dma_cmd_t *)io_map(0, PAGE_SIZE);
		if (!devc->input_cmds) {
			printf("awacs_open() - Cant allocate input DBDMA commands\n\r");
			splx(x);
			return 1;
		}
		simple_lock(&devc->lock);
	}

	max_buffers = PAGE_SIZE / sizeof(audio_dma_cmd_t) - 2;

	/* 
	 * Use last audio command (max_buffers) for branch back to begining
	 * of channel program and space for channel program to record the
	 * the last audio command number that ran. 
	 */
	s = (dbdma_command_t *)&devc->input_cmds[max_buffers];
  
	DBDMA_BUILD(s, 
		    DBDMA_CMD_NOP, 
		    DBDMA_KEY_STREAM0, 
		    0, 
		    0, 
		    DBDMA_INT_NEVER, 
		    DBDMA_WAIT_NEVER, 
		    DBDMA_BRANCH_ALWAYS);

	DBDMA_ST4_ENDIAN(&s->d_cmddep, (int)kvtophys((int)devc->input_cmds)); 
  
	/*
	 * set virt/phys pointers to where channel program will record
	 * last audio command number.
	 */
	s++;
	devc->input_seq.seq_num_virt = (ulong_t *)s;
	devc->input_seq.seq_num_phys = kvtophys((int)s);
  
	/*
	 * Set template descriptors for audio commands
	 */
	for (i=0; i < max_buffers; i++) {
		/*
		 * Create sequence number/host interrupt descriptor
		 */
		DBDMA_BUILD((&devc->input_cmds[i].desc_seq), 
			    DBDMA_CMD_STORE_QUAD, 
			    DBDMA_KEY_STREAM0, 
			    sizeof(ulong_t),
			    devc->input_seq.seq_num_phys, 
			    DBDMA_INT_ALWAYS, 
			    DBDMA_WAIT_NEVER, 
			    DBDMA_BRANCH_NEVER);
      
		devc->input_cmds[i].desc_seq.d_cmddep = i; 
      
		/*
		 * Create stop/nop descriptor
		 */
		DBDMA_BUILD((&devc->input_cmds[i].desc_stop), 
			    DBDMA_CMD_NOP, 
			    DBDMA_KEY_STREAM0, 
			    0, 
			    0, 
			    DBDMA_INT_NEVER, 
			    DBDMA_WAIT_NEVER, 
			    DBDMA_BRANCH_NEVER);
	}

	DBDMA_ST4_ENDIAN(&(dmap->d_cmdptrlo), 
			 (int)kvtophys((int)devc->input_cmds));
 
	devc->input_seq.seq_num_exp  = 0;
	devc->input_seq.seq_num_last = i-1; 
	devc->input_seq.num_active   = 0;
	devc->input_seq.max_buffers  = max_buffers;

	/*-------------------------------------------*/
	/*     Init audio output DBDMA pointers      */
	/*-------------------------------------------*/
	dbdma_reset(dmap);

	if (!devc->output_cmds) {
		simple_unlock(&devc->lock);
		devc->output_cmds  = (audio_dma_cmd_t *)io_map(0, 4096);
		if (!devc->output_cmds) {
			printf("awacs_open() - Cant allocate output DBDMA commands\n\r");
			splx(x);
			return D_NO_MEMORY;
		}
		simple_lock(&devc->lock);
	}

	s = (dbdma_command_t *)&devc->output_cmds[max_buffers];
  
	DBDMA_BUILD(s, 
		    DBDMA_CMD_NOP, 
		    DBDMA_KEY_STREAM0, 
		    0, 
		    0, 
		    DBDMA_INT_NEVER, 
		    DBDMA_WAIT_NEVER, 
		    DBDMA_BRANCH_ALWAYS);

	DBDMA_ST4_ENDIAN(&s->d_cmddep, (int)kvtophys((int)devc->output_cmds)); 

	s++;
	devc->output_seq.seq_num_virt = (ulong_t *)s;
	devc->output_seq.seq_num_phys = kvtophys((int)s);
  
	for (i=0; i < max_buffers; i++) {
		DBDMA_BUILD((&devc->output_cmds[i].desc_seq), 
			    DBDMA_CMD_STORE_QUAD, 
			    DBDMA_KEY_STREAM0, 
			    sizeof(ulong_t),
			    devc->output_seq.seq_num_phys, 
			    DBDMA_INT_ALWAYS, 
			    DBDMA_WAIT_NEVER, 
			    DBDMA_BRANCH_NEVER);
      
		devc->output_cmds[i].desc_seq.d_cmddep = i; 
      
		DBDMA_BUILD((&devc->output_cmds[i].desc_stop), 
			    DBDMA_CMD_NOP, 
			    DBDMA_KEY_STREAM0, 
			    0, 
			    0, 
			    DBDMA_INT_NEVER, 
			    DBDMA_WAIT_NEVER, 
			    DBDMA_BRANCH_NEVER);
	}

	dmap = DBDMA_REGMAP(DBDMA_AUDIO_OUT);
	DBDMA_ST4_ENDIAN(&(dmap->d_cmdptrlo),  
			 (int)kvtophys((int)devc->output_cmds));

	devc->output_seq.seq_num_exp  = 0;
	devc->output_seq.seq_num_last = i-1; 
	devc->output_seq.num_active   = 0;
	devc->output_seq.max_buffers  = max_buffers;

	DBDMA_ST4_ENDIAN(&dbdma_int_mask,  DBDMA_INT_ALWAYS << 20);

	timeout((timeout_fcn_t)awacs_check, (void *)devc, hz/2);

	simple_unlock(&devc->lock);
	splx(x);
	return D_SUCCESS;
}

/*
 *
 */
void 
awacs_close(int dev)
{
	awacs_devc_t	*devc;
	if (powermac_info.class == POWERMAC_CLASS_PDM) {
		awacs_pdm_close(dev);
		return;
	}
#if 0
	printf("awacs_close - %s %d\n\r", __FILE__, __LINE__);
#endif
	devc = awacs_dev_to_devc(dev);
	untimeout((timeout_fcn_t)awacs_check, (void *)devc);
}

/*
 *
 */
io_return_t 
awacs_write(int dev, io_req_t ior)
{
	awacs_devc_t	*devc;
	kern_return_t	rc;
	boolean_t	wait;
	spl_t		x = splaudio();

	if (powermac_info.class == POWERMAC_CLASS_PDM)
		return awacs_pdm_write(dev, ior);

	devc = awacs_dev_to_devc(dev);

	rc = device_write_get(ior, &wait);
	if (rc != KERN_SUCCESS) {
		printf("awacs_write() - device_write_get() - rc %d\n\r", rc);
		splx(x);
		return D_NO_MEMORY;
	}

	simple_lock(&devc->lock);

	rc = awacs_sg_space_avail(devc, ior, D_WRITE);
	if (rc != D_SUCCESS) {
		simple_unlock(&devc->lock);
		splx(x);
		return rc;
	}
	rc = awacs_sg_io(devc, ior, D_WRITE);

	simple_unlock(&devc->lock);
	splx(x);
	return (rc == D_SUCCESS) ? D_IO_QUEUED : rc;
}

/*
 *
 */
io_return_t 
awacs_read(int dev, io_req_t ior)
{
	awacs_devc_t        *devc;
	kern_return_t        rc;
	spl_t		x = splaudio();

	if (powermac_info.class == POWERMAC_CLASS_PDM)
		return awacs_pdm_read(dev, ior);

	devc = awacs_dev_to_devc(dev);

	rc = device_read_alloc(ior, ior->io_count);
	if (rc != KERN_SUCCESS) {
		printf("awacs_read() - device_read_alloc() - rc %d\n\r", rc);
		simple_unlock(&devc->lock);
		splx(x);
		return D_NO_MEMORY;
	}

	simple_lock(&devc->lock);
	rc = awacs_sg_space_avail(devc, ior, D_READ);
	if (rc != D_SUCCESS) {
		simple_unlock(&devc->lock);
		splx(x);
		return rc;
	}
	rc = awacs_sg_io(devc, ior, D_READ);
	simple_unlock(&devc->lock);
	splx(x);
	return (rc == D_SUCCESS) ? D_IO_QUEUED : rc;
}
  
/*
 *
 *
 */
io_return_t 
awacs_sg_space_avail(awacs_devc_t *devc, io_req_t ior, int mode) 
{
	seq_num_t     *seq;
	int           resid;
	int           num_required;

	num_required = ior->io_count / PAGE_SIZE;

	resid = (-(int)ior->io_data) & (PAGE_SIZE-1); 

	if (resid) {
		num_required++;

		if (resid < ior->io_count) {
			num_required++;
		}
	}

	seq = (mode == D_WRITE) ? &devc->output_seq : &devc->input_seq;
    
	if (num_required > (seq->max_buffers - seq->num_active)) {
		return D_NO_MEMORY;
	}
	return D_SUCCESS;
}

/*
 *
 *
 */
io_return_t 
awacs_sg_io(awacs_devc_t *devc, io_req_t ior, int mode)
{
	io_req_t    ior_x;
	int         resid;
	char        *addr;
	int         count;
	int         n;
	int         rc = D_INVALID_OPERATION;

	addr  = ior->io_data;
	count = ior->io_count;

	if (powermac_info.io_coherent == FALSE)
		if (mode == D_WRITE)
			flush_dcache((vm_offset_t) addr, count, FALSE);
		else
			invalidate_cache_for_io((vm_offset_t) addr, count, FALSE);

	resid = (-(int)addr) & (PAGE_SIZE-1);
	if (resid > count) resid = count;

	if (resid) {
		ior_x = (resid >= count) ? ior : 0;

		rc = awacs_add_audio_buffer(devc,
					     addr,
					     resid,
					     (void *) ior_x,
					     mode    );
		count -= resid;
		addr  += resid;
	}
       
	while (count) {
		ior_x = (count <= PAGE_SIZE) ? ior : 0;

		n  = (count > PAGE_SIZE) ? PAGE_SIZE : count;

		rc = awacs_add_audio_buffer(devc,
					     addr,
					     n,
					     (void *) ior_x,
					     mode    );
		count -= n;
		addr  += n;
	}

	return rc;
}

/*
 *
 *
 */
int 
awacs_add_audio_buffer(awacs_devc_t *devc,  char *buf,  int count, 
			void * ior, int mode)
{
	audio_dma_cmd_t   *audio_cmd, *audio_cmd_last;
	dbdma_regmap_t    *dmap;
	seq_num_t         *seq;
	volatile uchar_t  *cmd;
	ulong_t           dma_cmd;
	int               i;
	int               j;

#if 0
	volatile dbdma_regmap_t *dmap;
	ulong_t           *l;
	int               j,k;
#endif
#if 0
	int                k;
	ulong_t           *l;
#endif

	/*
	 * Select output or input channel parms
	 */
	if (mode == D_WRITE) {
		seq         = &devc->output_seq;
		audio_cmd   =  devc->output_cmds;
		dma_cmd     =  DBDMA_CMD_OUT_MORE;
		dmap	    = devc->output_dbdma;
	} else {
		seq         = &devc->input_seq;
		audio_cmd   =  devc->input_cmds;
		dma_cmd     =  DBDMA_CMD_IN_MORE;
		dmap	    = devc->input_dbdma;
	}
  
	/*
	 * If we are out of buffers, return error 
	 */
	if (seq->num_active >= seq->max_buffers) {
		return D_WOULD_BLOCK;
	}

	/*
	 * Remeber the last audio command written. We will need to update it
	 * when we complete building the new audio command 
	 */
	audio_cmd_last = audio_cmd + seq->seq_num_last;
  
	/*
	 * Wrap around to the first audio command if we are at the
	 * last available command.
	 */
	i = seq->seq_num_last + 1;
	if (i >= seq->max_buffers) i = 0;
  
	/*
	 * Build the data transfer command for the new buffer
	 */
	audio_cmd += i;
   
	DBDMA_BUILD((&audio_cmd->desc_data), 
		     dma_cmd, 
		     DBDMA_KEY_STREAM0, 
		     count, 
		     (int)kvtophys((int)buf), 
		     DBDMA_INT_NEVER, 
		     DBDMA_WAIT_NEVER, 
		     DBDMA_BRANCH_NEVER);

#if 0  
	buflog[bufnum++] = (ulong_t)kvtophys((int)buf);
	if (bufnum >= 8) {
		for (j=0; j < 8; j++) {
			printf(" %08x", buflog[j]);
		}
		printf("\n\r");
		bufnum = 0;
	}
#endif

	/*
	 * Stop the channel program when the new (last) command is done.
	 */ 
	cmd     = (uchar_t *)&audio_cmd->desc_stop.d_cmd_count;
	cmd[3]  =  DBDMA_CMD_STOP << 4;
 
	/*
	 * Save the requesting IOR in an unused field of the stop/nop cmd
	 */
	audio_cmd->desc_stop.d_address = (int)ior;
#if 0
	printf("Start output - buf: %d(%x), addr: %x, len: %x, ior: %x\n", 
	       i, (int)audio_cmd, (int)buf, count, (int)ior);
#endif

	audio_cmd->desc_seq.d_cmd_count &= ~dbdma_int_mask;
	if (ior) {
		audio_cmd->desc_seq.d_cmd_count |= dbdma_int_mask;
	}

	seq->seq_num_last = i;
	seq->num_active++;

	/*
	 * Update the previous command to allow the channel program to 
	 * continue executing commands.
	 */
	eieio();
	cmd     = (uchar_t *)&audio_cmd_last->desc_stop.d_cmd_count;
	cmd[3]  = DBDMA_CMD_NOP << 4;
	eieio();

#if 0
	{
		int                k;
		ulong_t           *l;
		printf("dma ptrs = %08x %08x\n\r",  dmap->d_cmdptrlo,  
		       (int)kvtophys((int)devc->output_cmds));
		l = (ulong_t *) &devc->output_cmds[i];
		for (j=0; j <= seq->max_buffers; j++) {
			for (k=0; k < 3; k++, l+=4) {
				printf("%08x %08x %08x %08x\n\r", *l, *(l+1), *(l+2), *(l+3));
			}
		}
	}
#endif
#if 0
	{
		int                k;
		ulong_t           *l;
		l = (ulong_t *) &devc->output_cmds[i];
		for (k=0; k < 3; k++, l+=4) {
			printf("%08x %08x %08x %08x\n\r", *l, *(l+1), *(l+2), *(l+3));
		}
		printf("\n\r");
	}
#endif

	/*
	 * Unleash the "Power to be your best!" (or at least get-by).
	 */
	dbdma_continue(dmap);

	return D_SUCCESS;
}

/*
 *
 */
awacs_devc_t *
awacs_dev_to_devc(int dev)
{
	int          i;

	for (i=0; i < num_awacs; i++) {
		if (awacs_dev_table[i].dev == dev) {
			return awacs_dev_table[i].devc;
		}
	}
	return 0;
}

/*
 *
 *
 */
io_return_t 
awacs_setstat(dev_t dev, dev_flavor_t flavor, dev_status_t data,
	       mach_msg_type_number_t count)
{
	awacs_devc_t                  *devc;
	dbdma_regmap_t		*dmap;
	seq_num_t                     *seq;
	io_req_t                      ior;
	audio_dma_cmd_t               *cmds;

	int                           channel;
	int                           left;
	int                           right;
	int                           mask;
	int                           rec;
	int                           i;
	int                           rate;
	int                           fOutput;
	spl_t		x = splaudio();

	if (powermac_info.class == POWERMAC_CLASS_PDM)
		return awacs_pdm_setstat(dev, flavor, data, count);

	devc = awacs_dev_to_devc(dev);
	simple_lock(&devc->lock);

#if 0
	printf(" awacs_setstat(%d,%d) - %s %d\n\r", flavor, *(int *)data, __FILE__,__LINE__);
#endif
	switch (flavor) {
	case AWACS_DEVCMD_INPUTVOL:
		scale_volume(*(int *)data, &left, &right, 0);
		devc->input_volume = *(int *)data;
		devc->CodecControl[0] &= ~(kLeftInputGainMask | kRightInputGainMask);
		devc->CodecControl[0] |= (right | (left << 4));
		awacs_write_CodecControlReg(devc, devc->CodecControl[0]);
		break;
      
	case AWACS_DEVCMD_OUTPUTVOL:
		set_volume(devc, *(int *)data);
		devc->output_volume = *(int *)data;
		break;

	case AWACS_DEVCMD_RATE:
		rate = *(int *) data;
		for (i=0; i < sizeof(awacs_rates)/sizeof(int) - 1; i++) {
			if (awacs_rates[i] >= rate) {
				break;
			}
		}
		devc->rate = awacs_rates[i];

		devc->SoundControl &= ~kHWRateMask;
		devc->SoundControl |= kHWRateMask - (i << kHWRateShift);
		awacs_write_SoundControlReg(devc, devc->SoundControl);
		break;

	case AWACS_DEVCMD_RECSRC:
		mask = *(int *)data;
		if (mask & SOUND_MASK_MIC) {
			rec |= kMicInput;
		}
		if (mask & SOUND_MASK_CD) {
			rec |= kCDInput;
		}

		devc->recsrc = mask & (SOUND_MASK_MIC | SOUND_MASK_CD);

		devc->CodecControl[0] &= ~(kInputMuxMask);
		devc->CodecControl[0] |= rec;
		awacs_write_CodecControlReg(devc, devc->CodecControl[0]);
		break;
    

	case AWACS_DEVCMD_HALT:
		fOutput = *(int *)data;
		if (fOutput) {
			channel =  DBDMA_AUDIO_OUT;
			seq     =  &devc->output_seq;
			cmds    =  devc->output_cmds;
		} else {
			channel = DBDMA_AUDIO_IN;
			seq   = &devc->input_seq;
			cmds  = devc->input_cmds;
		}

		dbdma_reset(DBDMA_REGMAP(channel));

		i = -1; 
		while(seq->num_active) {
			i = seq->seq_num_exp;
			ior = (io_req_t) cmds[i].desc_stop.d_address;
			if (ior) {
				ior->io_error = D_DEVICE_DOWN;
				iodone(ior);
			}

			seq->num_active--;

			if (++seq->seq_num_exp >= seq->max_buffers) {
				seq->seq_num_exp = 0;
			}
			if (*seq->seq_num_virt == i) {
				break;
			}
		}

		dmap = DBDMA_REGMAP(channel);
		DBDMA_ST4_ENDIAN(&(dmap->d_cmdptrlo),  
				 (int)kvtophys((int)cmds));

		seq->seq_num_exp = 0;
		seq->seq_num_last = seq->max_buffers-1;
		break;
      
	default:
		simple_unlock(&devc->lock);
		splx(x);
		return D_INVALID_OPERATION;
	}
  
	simple_unlock(&devc->lock);
	splx(x);
	return D_SUCCESS;
}

static void
set_volume(awacs_devc_t *devc, int level)
{
	int left, right;
	scale_volume(level /* % */, &left, &right, 1);
	/* Internal speaker */
	devc->CodecControl[4] &=  ~(kLeftSpeakerAttenMask | kRightSpeakerAttenMask);
	devc->CodecControl[4] |=  right | (left << kLeftSpeakerAttenShift);
	awacs_write_CodecControlReg(devc, devc->CodecControl[4]); 
	/* External/headphone output */
	devc->CodecControl[2] &=  ~(kLeftSpeakerAttenMask | kRightSpeakerAttenMask);
	devc->CodecControl[2] |=  right | (left << kLeftSpeakerAttenShift);
	awacs_write_CodecControlReg(devc, devc->CodecControl[2]); 
}

static void 
scale_volume(int vol, int *left, int *right, int fOutput)
{
	*left  = ((vol & 0xff) * 16) / 100;          
	*right = (((vol >> 8) & 0xff) * 16) / 100;
	if (fOutput) {
		*left  =  16 - *left;
		*right =  16 - *right;
	}

	if (*left  > 15) *left  = 15;
	if (*right > 15) *right = 15;
	if (*left < 0) *left = 0;
	if (*right < 0) *right = 0;
}  
 
static void 
unscale_volume(int left, int right, int *vol, int fOutput)
{
	if (fOutput) {
		left  = 15 - left;
		right = 15 - right;
	}

	left  = (left * 100)  / 16;
	right = (right * 100) / 16;

	if (left  > 100) left = 100;
	if (right > 100) right = 100;

	*vol = left | (right << 8); 
} 

/*
 *
 *
 */
io_return_t 
awacs_getstat(dev_t dev, dev_flavor_t flavor, dev_status_t data,
	       mach_msg_type_number_t *count)
{
	awacs_devc_t       *devc;
	int                left;
	int                right;
	int                bits;

	if (powermac_info.class == POWERMAC_CLASS_PDM)
		return awacs_pdm_getstat(dev, flavor, data, count);

	devc = awacs_dev_to_devc(0);
#if 0
	printf(" awacs_getstat(%d,%d) - %s %d\n\r", flavor, *(int *)data, __FILE__,__LINE__);
#endif
	switch (flavor) {
	case AWACS_DEVCMD_RATE:
		*(int *)data = devc->rate;
		break;

	case AWACS_DEVCMD_RECSRC:
		*(int *)data = devc->recsrc;
		break;

	case AWACS_DEVCMD_OUTPUTVOL:
		*(int *)data = devc->output_volume;
		break;

	case AWACS_DEVCMD_INPUTVOL:
		*(int *)data = devc->input_volume;
		break;

	default:
		return D_INVALID_OPERATION;
	}

	return D_SUCCESS;
}

static void 
awacs_write_CodecControlReg(awacs_devc_t *devc, volatile int value)
{
	volatile int          CodecControlReg;

#if 0
	printf("CodecControlReg = %08x - %s %d\n\r", value, __FILE__, __LINE__);
#endif

	DBDMA_ST4_ENDIAN(&devc->awacs_regs->CodecControl, value);
	eieio();

	do {
		CodecControlReg =  DBDMA_LD4_ENDIAN(&devc->awacs_regs->CodecControl);
		eieio();
	}
	while (CodecControlReg & kCodecCtlBusy);
}

static void 
awacs_write_SoundControlReg(awacs_devc_t *devc, volatile int value)
{

#if 0
	printf(" SoundControlReg = %08x - %s %d\n\r", value, __FILE__, __LINE__);
#endif

	DBDMA_ST4_ENDIAN(&devc->awacs_regs->SoundControl, value);
	eieio();
}  

static int 
awacs_read_CodecStatusReg(awacs_devc_t *devc)
{
	return DBDMA_LD4_ENDIAN(&devc->awacs_regs->CodecStatus);
}

static int 
awacs_read_ClippingCountReg(awacs_devc_t *devc)
{ 
	return DBDMA_LD4_ENDIAN(&devc->awacs_regs->ClippingCount);
}
