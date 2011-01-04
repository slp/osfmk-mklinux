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
/*
 * sound/dmabuf.c
 *
 * The DMA buffer manager for digitized voice applications
 *
 * Copyright by Hannu Savolainen 1993, 1994
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <eisa.h>
#include <himem.h>

#include <i386/AT386/sound/sound_config.h>

#ifdef CONFIGURE_SOUNDCARD

#include <i386/AT386/sound/sound_calls.h>

#include <i386/AT386/sound/audio.h>

/*
#ifdef DEB
#undef DEB
#endif
#define DEB(x) x
*/

#if !defined(EXCLUDE_AUDIO) || !defined(EXCLUDE_GUS)

DEFINE_WAIT_QUEUES (dev_sleeper[MAX_AUDIO_DEV], dev_sleep_flag[MAX_AUDIO_DEV]);

static struct dma_buffparms dmaps[MAX_AUDIO_DEV] =
{0};				/*
		 * Primitive way to allocate
		 * such a large array.
		 * Needs dynamic run-time alloction.
		 */

static void
dma_init_buffers (int dev)
{
  struct dma_buffparms *dmap = audio_devs[dev]->dmap = &dmaps[dev];

  RESET_WAIT_QUEUE (dev_sleeper[dev], dev_sleep_flag[dev]);

  dmap->flags = DMA_BUSY;	/* Other flags off */
  dmap->dma_mode = DMODE_NONE;
}

int
DMAbuf_open (int dev, int mode)
{
  int             retval;
  struct dma_buffparms *dmap = NULL;

  if (dev >= num_audiodevs)
    {
      printk ("PCM device %d not installed.\n", dev);
      return RET_ERROR (ENXIO);
    }

  if (!audio_devs[dev])
    {
      printk ("PCM device %d not initialized\n", dev);
      return RET_ERROR (ENXIO);
    }

  dmap = audio_devs[dev]->dmap = &dmaps[dev];

  if (dmap->flags & DMA_BUSY)
    return RET_ERROR (EBUSY);

#ifdef USE_RUNTIME_DMAMEM
  dmap->raw_buf[0] = NULL;
  sound_dma_malloc (dev);
#endif

  if ((retval = audio_devs[dev]->open (dev, mode)) < 0)
    return retval;

  dmap->open_mode = mode;

  dma_init_buffers (dev);

  audio_devs[dev]->ioctl (dev, SOUND_PCM_WRITE_BITS, 8, 1);
  audio_devs[dev]->ioctl (dev, SOUND_PCM_WRITE_CHANNELS, 1, 1);
  audio_devs[dev]->ioctl (dev, SOUND_PCM_WRITE_RATE, DSP_DEFAULT_SPEED, 1);

  return 0;
}

static void
dma_reset (int dev)
{
  int             retval;
  unsigned long   flags;

  DISABLE_INTR (flags);

  audio_devs[dev]->reset (dev);
  audio_devs[dev]->close (dev);

  if ((retval = audio_devs[dev]->open (dev, audio_devs[dev]->dmap->open_mode)) < 0)
    printk ("Sound: Reset failed - Can't reopen device\n");
  RESTORE_INTR (flags);

  dma_init_buffers (dev);

}

int dma_sync( int );

/*static*/ int
dma_sync (int dev)
{
  unsigned long   flags;
  boolean_t stop_waiting = FALSE;
  int qlen;

  if (audio_devs[dev]->dmap->dma_mode == DMODE_OUTPUT)
    {
      DISABLE_INTR (flags);
      simple_lock(&(auq_out)->auq_lock);
      qlen = auq_out->auq_len;
      simple_unlock(&(auq_out)->auq_lock);

      while (!PROCESS_ABORTING (dev_sleeper[dev], dev_sleep_flag[dev])
             && qlen)
        {
          DO_SLEEP (dev_sleeper[dev], dev_sleep_flag[dev], 5 * HZ);
          if (TIMED_OUT (dev_sleeper[dev], dev_sleep_flag[dev]))
            {
	      simple_lock(&(auq_out)->auq_lock);
	      qlen = auq_out->auq_len;
	      simple_unlock(&(auq_out)->auq_lock);
	      RESTORE_INTR (flags);
              return qlen;
            }
	      simple_lock(&(auq_out)->auq_lock);
	      qlen = auq_out->auq_len;
	      simple_unlock(&(auq_out)->auq_lock);
        }
      RESTORE_INTR (flags);
      /*
       * Some devices such as GUS have huge amount of on board RAM for the
       * audio data. We have to wait util the device has finished playing.
       */

      DISABLE_INTR (flags);
      if (audio_devs[dev]->local_qlen)  /* Device has hidden buffers */
        {
          while (!(PROCESS_ABORTING (dev_sleeper[dev], dev_sleep_flag[dev]))
                 && audio_devs[dev]->local_qlen (dev))
            {
              DO_SLEEP (dev_sleeper[dev], dev_sleep_flag[dev], HZ);
            }
        }
      RESTORE_INTR (flags);
    }

  DISABLE_INTR (flags);
  simple_lock(&(auq_out)->auq_lock);
  qlen = auq_out->auq_len;
  simple_unlock(&(auq_out)->auq_lock);
  RESTORE_INTR (flags);
  return qlen;
}

int
DMAbuf_release (int dev, int mode)
{
  unsigned long   flags;

  if (!(PROCESS_ABORTING (dev_sleeper[dev], dev_sleep_flag[dev]))
      && (audio_devs[dev]->dmap->dma_mode == DMODE_OUTPUT))
    {
      dma_sync (dev);
    }

#ifdef USE_RUNTIME_DMAMEM
  sound_dma_free (dev);
#endif

  DISABLE_INTR (flags);
  audio_devs[dev]->reset (dev);

  audio_devs[dev]->close (dev);

  audio_devs[dev]->dmap->dma_mode = DMODE_NONE;
  audio_devs[dev]->dmap->flags &= ~DMA_BUSY;
  RESTORE_INTR (flags);

  return 0;
}

int
DMAbuf_getrdbuffer (int dev, char **buf, int *len)
{
printf("DMAbuf_getrdbuffer NOT IMPLEMENTED\n");
  return 0;
}

int
DMAbuf_ioctl (int dev, unsigned int cmd, unsigned int arg, int local)
{
  struct dma_buffparms *dmap = audio_devs[dev]->dmap;

  switch (cmd)
    {
    case SNDCTL_DSP_RESET:
      dma_reset (dev);
      return 0;
      break;

    case SNDCTL_DSP_SYNC:
      dma_sync (dev);
      dma_reset (dev);
      return 0;
      break;

    default:
      return audio_devs[dev]->ioctl (dev, cmd, arg, local);
    }

  return RET_ERROR (EIO);
}

int
DMAbuf_prepare_for_output (int dev)
{
  int             err = EIO;
  struct dma_buffparms *dmap = audio_devs[dev]->dmap;

  if (dmap->dma_mode == DMODE_INPUT)	/* Direction change */
    {
      dma_reset (dev);
      dmap->dma_mode = DMODE_NONE;
    }
  else if (dmap->flags & DMA_RESTART)	/* Restart buffering */
    {
      dma_sync (dev);
      dma_reset (dev);
    }

  dmap->flags &= ~DMA_RESTART;

  if (!dmap->dma_mode)
    {
      int             err;

      dmap->dma_mode = DMODE_OUTPUT;
      if ((err = audio_devs[dev]->prepare_for_output (dev, 0, 0)) < 0)
	return err;
    }

  return 0;
}

int
DMAbuf_start_output (int dev, au_req_t aur)
{
  struct dma_buffparms *dmap = audio_devs[dev]->dmap;

    if (((audio_devs[dev]->flags & DMA_AUTOMODE) &&
	 audio_devs[dev]->flags & NEEDS_RESTART))
      dmap->flags |= DMA_RESTART;
    else
      dmap->flags &= ~DMA_RESTART;

  if (!(dmap->flags & DMA_ACTIVE)) {
    dmap->flags |= DMA_ACTIVE;

    audio_devs[dev]->output_block (dev, aur->phys_addr,
				   aur->length, 0,
				   1);
    dmap->flags |= DMA_STARTED;
  }
  
  return 0;
}

/*
 * This routine is machine depentent.
 */
int
DMAbuf_start_dma (int dev, unsigned long physaddr, int count, int dma_mode)
{
  int             chan = audio_devs[dev]->dmachan;
  struct dma_buffparms *dmap = audio_devs[dev]->dmap;
  unsigned long   flags;

  /*
   * This function is not as portable as it should be.
   */

  if (audio_devs[dev]->flags & DMA_AUTOMODE)
    {				/*
				 * Auto restart mode. Transfer the whole *
				 * buffer
				 */
      if (chan <= 3) {
	  dma_start((dma_mode == DMA_MODE_READ) ?
		    (DMA_WRITE | DMA_AUTOINITIALIZE) :
		    (DMA_READ | DMA_AUTOINITIALIZE),
		    physaddr, count, chan);
      } else {
	  dma_start16((dma_mode == DMA_MODE_READ) ?
		      (DMA_WRITE | DMA_AUTOINITIALIZE) :
		      (DMA_READ | DMA_AUTOINITIALIZE),
		      physaddr, count, chan);
      }
    }
  else
    {
      if (chan <= 3) {
	  dma_start((dma_mode == DMA_MODE_READ) ? DMA_WRITE : DMA_READ,
		    physaddr, count, chan);
      } else {
	  dma_start16((dma_mode == DMA_MODE_READ) ? DMA_WRITE : DMA_READ,
		    physaddr, count, chan);
      }
    }

  return count;
}

long
DMAbuf_init (long mem_start)
{
  int             dev;

  /*
 * NOTE! This routine could be called several times.
 */

  for (dev = 0; dev < num_audiodevs; dev++)
    audio_devs[dev]->dmap = &dmaps[dev];
  return mem_start;
}

void
DMAbuf_outputintr (int dev, int event_type)
{
  /*
 * Event types:
 *	0 = DMA transfer done. Device still has more data in the local
 *	    buffer.
 *	1 = DMA transfer done. Device doesn't have local buffer or it's
 *	    empty now.
 *	2 = No DMA transfer but the device has now more space in it's local
 *	    buffer.
 */

  unsigned long   flags;
  struct dma_buffparms *dmap = audio_devs[dev]->dmap;
  au_req_t aur;
  io_req_t ior;
  int audio_qlen = 0;
  hil_t hil;

  ior = (io_req_t) NULL;

  if (event_type != 2)
    {
      simple_lock(&(auq_out)->auq_lock);
      audio_qlen = auq_out->auq_len;
      simple_unlock(&(auq_out)->auq_lock);

      /* dequeue current dma request */
      assert(audio_qlen > 0);
      AU_DEQUEUE(auq_out, aur);

      if (aur->flags & AUR_IS_LAST) {	/* last dma request of the ior */

	ior = (io_req_t) aur->ior;	/* we call io_completed() later */
      }

#if HIMEM
      if (aur->hil) {
	hil = aur->hil;
	himem_revert(hil);
	simple_lock(&auq_free_low);
	free_low_page++;
	simple_unlock(&auq_free_low); /* Should not releae the lock till the end */
      }
#endif  /* HIMEM */
      
      kfree((vm_offset_t) aur, sizeof (struct au_req));

      simple_lock(&(auq_out)->auq_lock);
      audio_qlen = auq_out->auq_len;
      simple_unlock(&(auq_out)->auq_lock);

      if (audio_qlen == 0)
	dmap->flags &= ~DMA_ACTIVE;

      if (audio_qlen > 0)	/* Another dma request ready */
	{
vm_offset_t l_virt, l_phys;
int l_length;

	  AU_DEQUEUE(auq_out, aur);

	  assert(aur->dev == dev);

l_virt = aur->virt_addr;
l_length = aur->length;
l_phys = (vm_offset_t) aur->phys_addr;

#if HIMEM
	  if (!(aur->flags & AUR_IS_LOW_MEM)) {
	    /* we need to convert physical memory into low physical memory */
	    simple_lock(&auq_free_low);
	    free_low_page--;
	    simple_unlock(&auq_free_low);

	    aur->flags |= AUR_IS_LOW_MEM;
	    aur->hil = (hil_t) 0;
	    aur->phys_addr = 
	      himem_convert(l_phys, l_length, D_WRITE, (hil_t *) &(aur->hil));
	  }
#endif /* HIMEM */

	  AU_PREPEND(auq_out, aur);
	  dmap->flags |= DMA_ACTIVE;

	  audio_devs[dev]->output_block (dev, aur->phys_addr,
					 aur->length, 0,
					 1);
	}
      else if (event_type == 1)
	{
	  audio_devs[dev]->halt_xfer (dev);
	  if ((audio_devs[dev]->flags & DMA_AUTOMODE) &&
	      audio_devs[dev]->flags & NEEDS_RESTART)
	    dmap->flags |= DMA_RESTART;
	  else
	    dmap->flags &= ~DMA_RESTART;
	}

      if (ior) {
	io_completed((io_req_t) ior, FALSE);
      }
    }				/* event_type != 2 */

  DISABLE_INTR (flags);
  if (SOMEONE_WAITING (dev_sleeper[dev], dev_sleep_flag[dev]))
    {
      WAKE_UP (dev_sleeper[dev], dev_sleep_flag[dev]);
    }
  RESTORE_INTR (flags);
}

void
DMAbuf_inputintr (int dev)
{
printf("DMAbuf_inputintr NOT YET SUPPORTED\n");
}

int
DMAbuf_open_dma (int dev)
{
  unsigned long   flags;
  int             chan = audio_devs[dev]->dmachan;

  if (ALLOC_DMA_CHN (chan))
    {
      printk ("Unable to grab DMA%d for the audio driver\n", chan);
      return RET_ERROR (EBUSY);
    }

  DISABLE_INTR (flags);

  RESTORE_INTR (flags);

  return 0;
}

void
DMAbuf_close_dma (int dev)
{
  int             chan = audio_devs[dev]->dmachan;

  DMAbuf_reset_dma (chan);
  RELEASE_DMA_CHN (chan);
}

void
DMAbuf_reset_dma (int chan)
{
}

/*
 * The sound_mem_init() is called by mem_init() immediately after mem_map is
 * initialized and before free_page_list is created.
 *
 * This routine allocates DMA buffers at the end of available physical memory (
 * <16M) and marks pages reserved at mem_map.
 */

#else
/*
 * Stub versions if audio services not included
 */

int
DMAbuf_open (int dev, int mode)
{
  return RET_ERROR (ENXIO);
}

int
DMAbuf_release (int dev, int mode)
{
  return 0;
}

int
DMAbuf_getwrbuffer (int dev, char **buf, int *size)
{
  return RET_ERROR (EIO);
}

int
DMAbuf_getrdbuffer (int dev, char **buf, int *len)
{
  return RET_ERROR (EIO);
}

int
DMAbuf_rmchars (int dev, int buff_no, int c)
{
  return RET_ERROR (EIO);
}

int
DMAbuf_start_output (int dev, int buff_no, int l)
{
  return RET_ERROR (EIO);
}

int
DMAbuf_ioctl (int dev, unsigned int cmd, unsigned int arg, int local)
{
  return RET_ERROR (EIO);
}

long
DMAbuf_init (long mem_start)
{
  return mem_start;
}

int
DMAbuf_start_dma (int dev, unsigned long physaddr, int count, int dma_mode)
{
  return RET_ERROR (EIO);
}

int
DMAbuf_open_dma (int chan)
{
  return RET_ERROR (ENXIO);
}

void
DMAbuf_close_dma (int chan)
{
  return;
}

void
DMAbuf_reset_dma (int chan)
{
  return;
}

void
DMAbuf_inputintr (int dev)
{
  return;
}

void
DMAbuf_outputintr (int dev, int underrun_flag)
{
  return;
}

#endif

#endif
