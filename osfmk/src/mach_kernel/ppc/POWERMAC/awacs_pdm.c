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
#include <ppc/POWERMAC/awacs_pdm.h>
#include <ppc/POWERMAC/dbdma.h>
#include <ppc/io_map_entries.h>
#include <vm/vm_kern.h>


static void scale_volume(int vol, int *left, int *right, int fOutput);
static void set_volume(awacs_devc_t *devc, int vol);
static void unscale_volume(int left, int right, int *vol, int fOutput);
static void awacs_write_CodecControlReg(awacs_devc_t *devc, volatile int value);
static void awacs_write_SoundControlReg(awacs_devc_t *devc, volatile int value);
static int awacs_read_CodecStatusReg(awacs_devc_t *devc);
static int awacs_read_ClippingCountReg(awacs_devc_t *devc);
static void awacs_next_DMA_out(awacs_devc_t *devc);
static awacs_devc_t *awacs_dev_to_devc( int dev );
static void dma_output_intr(int device, struct ppc_saved_state *);
static int awacs_pdm_check(void *);

#define SPLAUDIO SPLTTY   /* Caution!  Needs to match registered I/O level */
#define splaudio spltty

/*
 *
 */
#define NAWACS 1

static int num_awacs = 0;
static awacs_dev_table_t awacs_dev_table[NAWACS];

static int awacs_rates[] = 
{
	22050, 29400, 44100
};

/*
 *
 */
int 
awacs_pdm_probe(caddr_t port, void *ui)
{
	awacs_devc_t         *devc;
	vm_offset_t          p;
	ulong_t              i;
	dbdma_command_t      *s;

#if 0
	printf("\n\rawacs_pdm_probe() - %s %d\n\r", __FILE__, __LINE__);
#endif
	if (powermac_info.class != POWERMAC_CLASS_PDM)
		return 0;
	if (powermac_info.dma_buffer_virt == 0) {
		printf("awacs_probe() - No DMA buffers?\n");
		return 0;
	}		
	kmem_alloc(kernel_map, (vm_offset_t *)&p, sizeof(awacs_devc_t));
	memset((void *)p, 0, sizeof(awacs_devc_t));
	devc = (awacs_devc_t *) p;
	simple_lock_init(&devc->lock, ETAP_IO_CHIP);
	devc->awacs_regs = (awacs_regmap_t *)POWERMAC_IO(PDM_AWACS_BASE_PHYS);

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

	/* Set up interrupt handlers */
	pmac_register_int(PMAC_DMA_AUDIO_OUT, SPLAUDIO, (void (*)(int, void *))dma_output_intr);
	/* Clear interrupts */
	devc->awacs_regs->DMAOut = DMA_IF0|DMA_IF1|DMA_ERR;
	devc->awacs_regs->DMAIn  = DMA_IF0|DMA_IF1|DMA_ERR|DMA_STAT;

	/* No buffer allocated */
	devc->DMA_out = (unsigned char *)NULL;

	/* Unknown speaker/headphone status */
	devc->status = 0xFFFFFFFF;

	return 1;
}

/*
 * Timeout routine which checks to see if the speakers/headphones
 * have changed state.
 */
static int
awacs_pdm_check(void *p)
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
	timeout((timeout_fcn_t)awacs_pdm_check, (void *)devc, hz/2);
	simple_unlock(&devc->lock);
	splx(x);
	return (0);
}

/*
 *
 */
void 
awacs_pdm_attach(struct bus_device *dev)
{
#if 0
	printf("\n\rawacs_attach() - %s %d\n\r", __FILE__, __LINE__);
#endif
}

/*
 *
 */
io_return_t 
awacs_pdm_open(int dev, dev_mode_t mode, io_req_t ior)
{
	awacs_devc_t	*devc;
	vm_offset_t	buf;
	int		i;
	spl_t		x = splaudio();

#if 0
	printf("awacs_pdm_open(%x) - %s %d\n\r", (unsigned)dev, __FILE__, __LINE__);
#endif
	devc = awacs_dev_to_devc(dev);
	if (devc == NULL || devc->is_present == FALSE) {
		splx(x);
		return D_NO_SUCH_DEVICE;
	}

	if ((buf = (vm_offset_t)devc->DMA_out) == (vm_offset_t)NULL) {
		kmem_alloc(kernel_map, (vm_offset_t *)&buf, AWACS_DMA_BUFSIZE);
		if (!buf) {
			printf("awacs_open() - Can't allocate DMA buffers?\n");
			splx(x);
			return 1;
		}
		/* Start routine which checks for speaker/headphone state */
		timeout((timeout_fcn_t)awacs_pdm_check, (void *)devc, hz/2);
	}

	simple_lock(&devc->lock);
	/* Clear output DMA pointers */
	devc->DMA_out_buf0 = (unsigned long *)(powermac_info.dma_buffer_virt + PDM_DMA_BUFFER_AUDIO_OUT0);
	devc->DMA_out_buf1 = (unsigned long *)(powermac_info.dma_buffer_virt + PDM_DMA_BUFFER_AUDIO_OUT1);
	devc->DMA_out = (unsigned char *)buf;
	devc->DMA_out_len = 0;
	devc->DMA_out_put = 0;
	devc->DMA_out_get = 0;
	devc->DMA_out_busy = FALSE;
	devc->DMA_out_buf0_busy = TRUE;  /* Until the hardware tells us... */
	devc->DMA_out_buf1_busy = TRUE;
	devc->DMA_out_ior_put = 0;
	devc->DMA_out_ior_get = 0;
	devc->awacs_regs->DMAOut = DMA_IF0|DMA_IF1|DMA_ERR|DMA_IE0|DMA_IE1;
	devc->awacs_regs->BufferSize = (AWACS_DMA_HARDWARE_BUFSIZE>>2)-1;
	memset((void *)devc->DMA_out, 0, AWACS_DMA_BUFSIZE);
	if (!(devc->SoundControl & kDMAOutEnable)) {
		/* Prime output */
		devc->SoundControl |= kDMAOutEnable;
		awacs_write_SoundControlReg(devc, devc->SoundControl);
	}
	simple_unlock(&devc->lock);
	splx(x);
	return D_SUCCESS;
}

/*
 *
 */
void 
awacs_pdm_close(int dev)
{
	awacs_devc_t	*devc;
	spl_t		x = splaudio();

#if 0
	printf("awacs_pdm_close - %s %d\n\r", __FILE__, __LINE__);
#endif
	devc = awacs_dev_to_devc(dev);
	simple_lock(&devc->lock);
	/* Reset interrupt sources */
	devc->awacs_regs->DMAOut = DMA_IF0|DMA_IF1|DMA_ERR;
#if 0
	printf("Close - get: %d, put: %d, busy: %d.%d\n", 
	       devc->DMA_out_get, devc->DMA_out_put,
	       devc->DMA_out_buf0_busy, devc->DMA_out_buf1_busy);
#endif
	simple_unlock(&devc->lock);
}

/*
 *
 */
io_return_t 
awacs_pdm_write(int dev, io_req_t ior)
{
	awacs_devc_t	*devc;
	kern_return_t	rc;
	boolean_t	wait;
	int		space, len, scale;
	unsigned char 	*data;
	unsigned short	*bp, elem[2];
	int		i, j, next;
	spl_t		x = splaudio();

	devc = awacs_dev_to_devc(dev);

	rc = device_write_get(ior, &wait);
	if (rc != KERN_SUCCESS) {
		printf("awacs_write() - device_write_get() - rc %d\n\r", rc);
		splx(x);
		return D_NO_MEMORY;
	}

	simple_lock(&devc->lock);
	/* See if there's space to record the 'ior' */	
	next = devc->DMA_out_ior_put+1;
	if (next >= AWACS_NUM_ACTIVE)
		next = 0;
	if (next == devc->DMA_out_ior_get) {
		rc = D_NO_MEMORY;
		goto err;
	}
	/* See if there is buffer space */
	space = devc->DMA_out_get - devc->DMA_out_put;
	if (space <= 0)
		space += AWACS_DMA_BUFSIZE;
	scale = 1;
	if (devc->rate != devc->desired_rate) {
		/* See if we can scale the data */
		int r = devc->desired_rate;
		while (r < devc->rate) {
			scale++;
			r += devc->desired_rate;
		}
	}
	len = scale * ior->io_count;
#if 0
	printf("AWACS write %d[%d], space: %d\n", ior->io_count, len, space);
#endif
	if (len <= space) {
		data = (unsigned char *)ior->io_data;
		bp = (unsigned short *)((unsigned char *)&devc->DMA_out[devc->DMA_out_put]);
		for (i = 0;  i < ior->io_count;  i += 4) {
			elem[0] = (data[0] << 8) | data[1];
			data += 2;
			elem[1] = (data[0] << 8) | data[1];
			data += 2;
			for (j = 0;  j < scale;  j++) {
				*bp++ = elem[0];
				*bp++ = elem[1];
				devc->DMA_out_put += 4;
				devc->DMA_out_len += 4;
				if (devc->DMA_out_put >= AWACS_DMA_BUFSIZE) {
					bp = (unsigned short *)devc->DMA_out;
					devc->DMA_out_put = 0;
				}
			}
		}
		awacs_next_DMA_out(devc);
		/* Record 'ior' so we can send iodone when complete */
#if 0
printf("Set IOR[%d] = %x\n", devc->DMA_out_ior_put, (unsigned)ior);
#endif
		devc->DMA_out_ior_length[devc->DMA_out_ior_put] = len;
		devc->DMA_out_ior_list[devc->DMA_out_ior_put] = ior;
		devc->DMA_out_ior_put = next;
	} else {
		rc = D_NO_MEMORY;
	}
 err:
	simple_unlock(&devc->lock);
	splx(x);
	return (rc == D_SUCCESS) ? D_IO_QUEUED : rc;
}

/*
 * This routine is called to stage the next buffer into the DMA output buffer.
 * If there is no more data, then the DMA will be stopped.
 */

void
awacs_next_DMA_out(awacs_devc_t *devc)
{
	int count, total;
	int size, i;
	unsigned long *bp, *hbp;
#if 0
	printf("Next - get: %d, put: %d, busy: %d.%d\n", 
	       devc->DMA_out_get, devc->DMA_out_put,
	       devc->DMA_out_buf0_busy, devc->DMA_out_buf1_busy);
#endif
	while (devc->DMA_out_len != 0) {
		if (devc->DMA_out_buf0_busy == FALSE) {
			hbp = devc->DMA_out_buf0;
			devc->DMA_out_buf0_busy = TRUE;
		} else if (devc->DMA_out_buf1_busy == FALSE) {
			hbp = devc->DMA_out_buf1;
			devc->DMA_out_buf1_busy = TRUE;
		} else {
			/* No free hardware buffer available */
			break;
		}
		count = devc->DMA_out_len;
		if (count > AWACS_DMA_HARDWARE_BUFSIZE)
			count = AWACS_DMA_HARDWARE_BUFSIZE;
		bp = (unsigned long *)&devc->DMA_out[devc->DMA_out_get];
		devc->DMA_out_len -= count;
		for (i = 0;  i < count;  i += 4) {
			*hbp++ = *bp++;
			if ((devc->DMA_out_get += 4) >= AWACS_DMA_BUFSIZE) {
				devc->DMA_out_get = 0;
				bp = (unsigned long *)devc->DMA_out;
			}
		}
		if (count != AWACS_DMA_HARDWARE_BUFSIZE ) {
			/* Fill hardware buffers so no left over 'clicks' */
			while (hbp != (unsigned long *)((unsigned)devc->DMA_out_buf1+AWACS_DMA_HARDWARE_BUFSIZE)) {
				*hbp++ = 0;
			}
		}
	}
	if (devc->DMA_out_buf0_busy || devc->DMA_out_buf1_busy) {
		/* Start DMA */
		if (devc->DMA_out_len && !(devc->SoundControl & kDMAOutEnable)) {
			devc->SoundControl |= kDMAOutEnable;
			awacs_write_SoundControlReg(devc, devc->SoundControl);
		}
	} else {
		if (devc->SoundControl & kDMAOutEnable) {
			devc->SoundControl &= ~kDMAOutEnable;
			awacs_write_SoundControlReg(devc, devc->SoundControl);
		}
	}
}

static void 
dma_output_intr(int device, struct ppc_saved_state *state)
{
	int len, stat;
	awacs_devc_t	*devc;
	devc = awacs_dev_table[0].devc;
	stat = devc->awacs_regs->DMAOut;
#if 0
	printf("DMA intr - dev: %x, stat: %x\n", device, stat);
#endif
	if (stat & DMA_IF0) {
		/* Buffer 0 has been used up */
		devc->DMA_out_buf0_busy = FALSE;
		devc->awacs_regs->DMAOut |= DMA_IF0;  /* Clear interrupt */
		len = AWACS_DMA_HARDWARE_BUFSIZE;
	} else if (stat & DMA_IF1) {
		/* Buffer 1 has been used up */
		devc->DMA_out_buf1_busy = FALSE;
		devc->awacs_regs->DMAOut |= DMA_IF1;  /* Clear interrupt */
		len = AWACS_DMA_HARDWARE_BUFSIZE;
	} else {
		/* Buffer overrun! Try resetting both buffers to continue */
		devc->DMA_out_buf0_busy = FALSE;
		devc->DMA_out_buf1_busy = FALSE;
		devc->awacs_regs->DMAOut |= DMA_IF0|DMA_IF1|DMA_ERR;  /* Clear interrupt */
		len = 2*AWACS_DMA_HARDWARE_BUFSIZE;
	}
#if 0
printf("Len[%d] = %d, get: %d, put: %d, busy: %d.%d, stat: %x\n", 
       devc->DMA_out_ior_get, devc->DMA_out_ior_length[devc->DMA_out_ior_get],
       devc->DMA_out_get, devc->DMA_out_put,
       devc->DMA_out_buf0_busy, devc->DMA_out_buf1_busy, stat);
#endif
	awacs_next_DMA_out(devc);
	/* See if we've completed a pending I/O */
	if (devc->DMA_out_ior_length[devc->DMA_out_ior_get] > 0) {
		devc->DMA_out_ior_length[devc->DMA_out_ior_get] -= len;
		if (devc->DMA_out_ior_length[devc->DMA_out_ior_get] <= 0) {
#if 0
printf("Signal iodone(%d.%x)\n", devc->DMA_out_ior_get, (unsigned)devc->DMA_out_ior_list[devc->DMA_out_ior_get]);
#endif
			iodone(devc->DMA_out_ior_list[devc->DMA_out_ior_get]);
			devc->DMA_out_ior_list[devc->DMA_out_ior_get] = (io_req_t)NULL;
			devc->DMA_out_ior_length[devc->DMA_out_ior_get] = 0;
			if (++devc->DMA_out_ior_get >= AWACS_NUM_ACTIVE) {
				devc->DMA_out_ior_get = 0;
			}
			if ((devc->DMA_out_len == 0) && (devc->SoundControl & kDMAOutEnable)) {
				/* The hardware may miss a final interrupt? */
				devc->SoundControl &= ~kDMAOutEnable;
				awacs_write_SoundControlReg(devc, devc->SoundControl);
			}
		}
	}
}

/*
 *
 */
io_return_t 
awacs_pdm_read(int dev, io_req_t ior)
{
	awacs_devc_t        *devc;
	kern_return_t        rc;
	spl_t		x = splaudio();

	devc = awacs_dev_to_devc(dev);

	rc = device_read_alloc(ior, ior->io_count);
	if (rc != KERN_SUCCESS) {
		printf("awacs_read() - device_read_alloc() - rc %d\n\r", rc);
		simple_unlock(&devc->lock);
		splx(x);
		return D_NO_MEMORY;
	}

	simple_lock(&devc->lock);
		rc = D_INVALID_OPERATION;
	simple_unlock(&devc->lock);
	splx(x);
	return (rc == D_SUCCESS) ? D_IO_QUEUED : rc;
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
awacs_pdm_setstat(dev_t dev, dev_flavor_t flavor, dev_status_t data,
	       mach_msg_type_number_t count)
{
	awacs_devc_t                  *devc;
	io_req_t                      ior;

	int                           channel;
	int                           left;
	int                           right;
	int                           mask;
	int                           rec;
	int                           i;
	int                           rate;
	int                           fOutput;
	spl_t		x = splaudio();

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
		devc->desired_rate = rate;
		for (i=0; i < sizeof(awacs_rates)/sizeof(int) - 1; i++) {
			if (awacs_rates[i] >= rate) {
				break;
			}
		}

		devc->rate = awacs_rates[i];

		devc->SoundControl &= ~kHWRateMask;
		devc->SoundControl |= (i << kHWRateShift);
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
		while (devc->DMA_out_ior_get != devc->DMA_out_ior_put) {
			ior = devc->DMA_out_ior_list[devc->DMA_out_ior_get];			
			if (ior) {
				ior->io_error = D_DEVICE_DOWN;
				iodone(ior);
				devc->DMA_out_ior_list[devc->DMA_out_ior_get] = (io_req_t)NULL;
				devc->DMA_out_ior_length[devc->DMA_out_ior_get] = 0;
			}
			if (++devc->DMA_out_ior_get >= AWACS_NUM_ACTIVE) {
				devc->DMA_out_ior_get = 0;
			}
		}
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
	/* External/headphone output */
	devc->CodecControl[2] &=  ~(kLeftSpeakerAttenMask | kRightSpeakerAttenMask);
	devc->CodecControl[2] |=  right | (left << kLeftSpeakerAttenShift);
	awacs_write_CodecControlReg(devc, devc->CodecControl[2]); 
	/* Internal speaker */
	devc->CodecControl[4] &=  ~(kLeftSpeakerAttenMask | kRightSpeakerAttenMask);
	devc->CodecControl[4] |=  right | (left << kLeftSpeakerAttenShift);
	awacs_write_CodecControlReg(devc, devc->CodecControl[4]); 
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
awacs_pdm_getstat(dev_t dev, dev_flavor_t flavor, dev_status_t data,
	       mach_msg_type_number_t *count)
{
	awacs_devc_t       *devc;
	int                left;
	int                right;
	int                bits;

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
	printf("CodecControlReg = %08x\n", (kExpand|kCommand) | value);
#endif
	devc->awacs_regs->CodecControl[0] = ((kExpand|kCommand) | value) >> 16;
	devc->awacs_regs->CodecControl[1] = (value & 0xFF00) >> 8;
	devc->awacs_regs->CodecControl[2] = (value & 0x00FF);
	eieio();
}

static void 
awacs_write_SoundControlReg(awacs_devc_t *devc, volatile int value)
{

#if 0
	printf(" SoundControlReg[%x] = %04x\n", &devc->awacs_regs->SoundControl0, value);
#endif
	devc->awacs_regs->SoundControl[0] = value>>8;
	devc->awacs_regs->SoundControl[1] = value;
	eieio();
}  

static int 
awacs_read_CodecStatusReg(awacs_devc_t *devc)
{       
	return ((devc->awacs_regs->CodecStatus[0] << 16) | 
		(devc->awacs_regs->CodecStatus[1] << 8) | 
		(devc->awacs_regs->CodecStatus[2]));
}
