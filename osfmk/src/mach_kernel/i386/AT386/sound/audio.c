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
 * sound/audio.c
 *
 * Device file manager for /dev/audio
 *
 * Copyright by Hannu Savolainen 1993
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

#include <i386/AT386/sound/sound_config.h>

#ifdef CONFIGURE_SOUNDCARD
#ifndef EXCLUDE_AUDIO

#include <i386/AT386/sound/ulaw.h>

#include <i386/AT386/sound/audio.h>

#define ON		1
#define OFF		0

#define NO_INLINE_ASM

struct auqueue auq_out_queue;
struct auqueue *auq_out = &auq_out_queue;	/* Audio write requests queue */
int free_low_page;		/* number of free low memory pages */

static int      audio_mode[MAX_AUDIO_DEV];

#define		AM_NONE		0
#define		AM_WRITE	1
#define 	AM_READ		2

static int      audio_format[MAX_AUDIO_DEV];
static int      local_conversion[MAX_AUDIO_DEV];

static int
set_format (int dev, int fmt)
{
  if (fmt != AFMT_QUERY)
    {

      local_conversion[dev] = 0;

      if (!(audio_devs[dev]->format_mask & fmt))	/* Not supported */
	if (fmt == AFMT_MU_LAW)
	  {
	    fmt = AFMT_U8;
	    local_conversion[dev] = AFMT_MU_LAW;
	  }
	else
	  fmt = AFMT_U8;	/* This is always supported */

      audio_format[dev] = DMAbuf_ioctl (dev, SNDCTL_DSP_SETFMT, fmt, 1);
    }

  if (local_conversion[dev])	/* This shadows the HW format */
    return local_conversion[dev];

  return audio_format[dev];
}

int
audio_open (int dev, struct fileinfo *file)
{
  int             ret;
  int             bits;
  int             dev_type = dev & 0x0f;
  int             mode = file->mode & O_ACCMODE;

  dev = dev >> 4;

  if (dev_type == SND_DEV_DSP16)
    bits = 16;
  else
    bits = 8;

  if ((ret = DMAbuf_open (dev, mode)) < 0)
    return ret;

  local_conversion[dev] = 0;

  if (DMAbuf_ioctl (dev, SNDCTL_DSP_SETFMT, bits, 1) != bits)
    {
      audio_release (dev, file);
      return RET_ERROR (ENXIO);
    }

  if (dev_type == SND_DEV_AUDIO)
    {
      set_format (dev, AFMT_MU_LAW);
    }
  else
    set_format (dev, bits);

  audio_mode[dev] = AM_NONE;

  AUQ_INIT(auq_out);

#if HIMEM  
  /* Initilalize the number of free low memory pages reserved and its lock */
  simple_lock_init(&auq_free_low, ETAP_IO_REQ);
  free_low_page = AU_LOW_PAGES;
#endif /* HIMEM */

  return ret;
}

void
audio_release (int dev, struct fileinfo *file)
{
  int             mode;

  dev = dev >> 4;
  mode = file->mode & O_ACCMODE;

  DMAbuf_release (dev, mode);
}

#ifdef NO_INLINE_ASM
static void
translate_bytes (const unsigned char *table, unsigned char *buff, unsigned long n)
{
  unsigned long   i;

  for (i = 0; i < n; ++i)
    buff[i] = table[buff[i]];
}

#else
extern inline void
translate_bytes (const void *table, void *buff, unsigned long n)
{
  __asm__ ("cld\n"
	   "1:\tlodsb\n\t"
	   "xlatb\n\t"
	   "stosb\n\t"
	   "loop 1b\n\t":
	   :"b" ((long) table), "c" (n), "D" ((long) buff), "S" ((long) buff)
	   :"bx", "cx", "di", "si", "ax");
}

#endif

/*
 * machine dependent
 */
int
audio_write (int dev, struct fileinfo *file, snd_rw_buf * buf, int size)
{
  int             c, p, l;
  int             err;
  boolean_t       ior_sync = ((buf->io_op & IO_SYNC) != 0);

  int off, count, length;
  vm_offset_t virt, phys;
  spl_t	ipl;
  au_req_t aur;
  
  dev = dev >> 4;

  count = size;
  off = 0;

#ifdef MACH_KERNEL
  WRITE_PREPARE(buf);
#endif /* MACH_KERNEL */

  virt = (vm_offset_t) ((io_req_t) buf)->io_data;  /* may be not page aligned */
  off = virt & (I386_PGBYTES-1);

  while (count) {			
    /*
     * Perform output blocking
     */
    /* allocate an audio dma request. freed in dma intr. handler */

    aur = (struct au_req *) kalloc (sizeof(struct au_req));

    if ((off + count) <= I386_PGBYTES)
      length = count;
    else
      length =  I386_PGBYTES - off;

    DMAbuf_prepare_for_output (dev);

    /*
     * Insert local processing here
     */
      
    if (local_conversion[dev] == AFMT_MU_LAW) {
      translate_bytes (ulaw_dsp, (unsigned char *) virt, length);
    }

    aur->ior = (io_req_t) buf;
    aur->dev = dev;
    aur->virt_addr = virt;
    aur->phys_addr = phys = (vm_offset_t) kvtophys(virt);
    aur->length = length;
    aur->flags = AUR_NONE;
    if ((count - length) == 0) 
      aur->flags |= AUR_IS_LAST;
      
#if HIMEM
    aur->hil = (hil_t) 0;

    aur->flags |= AUR_IS_LOW_MEM;
    if (high_mem_page(phys)) {
      boolean_t do_it = FALSE;

      ipl = splhi();
      simple_lock(&auq_free_low);
      if (free_low_page > 0) {
	do_it = TRUE;		/* do the himem_convert() after unlocking */
	free_low_page--;
      }
      else {
	aur->flags &= ~AUR_IS_LOW_MEM;
      }
      simple_unlock(&auq_free_low);
      splx(ipl);

      if (do_it)
	aur->phys_addr = himem_convert(phys, length, D_WRITE, &(aur->hil));
    }
#endif /* HIMEM */

    ipl = splhi();
    AU_ENQUEUE(auq_out, (au_req_t) aur);

    DMAbuf_start_output(dev, (au_req_t) aur);

    splx(ipl);

    count -= length;
    virt += length;
    off = virt & (I386_PGBYTES-1);	/* ALWAYS 0 ! */
  }

  if (ior_sync) {
      iowait((io_req_t) buf);
      return (D_SUCCESS);
  }
  
  return (D_IO_QUEUED);
}

/*
 * machine dependent
 */
int
audio_read (int dev, struct fileinfo *file, snd_rw_buf * buf, int count)
{
printf("audio_read NOT YET IMPLEMENTED\n");
  return 0;
}

int audio_ioctl(int dev, struct fileinfo *file,
	     unsigned int cmd, unsigned int arg)
{

  dev = dev >> 4;

  switch (cmd)
    {
    case SNDCTL_DSP_SYNC:
      return DMAbuf_ioctl (dev, cmd, arg, 0);
      break;

    case SNDCTL_DSP_RESET:
      return DMAbuf_ioctl (dev, cmd, arg, 0);
      break;

    case SNDCTL_DSP_GETFMTS:
      return IOCTL_OUT (arg, audio_devs[dev]->format_mask);
      break;

    case SNDCTL_DSP_SETFMT:
      return IOCTL_OUT (arg, set_format (dev, IOCTL_IN (arg)));

    default:
      return DMAbuf_ioctl (dev, cmd, arg, 0);
      break;
    }
}

long
audio_init (long mem_start)
{
  /*
 * NOTE! This routine could be called several times during boot.
 */
  return mem_start;
}

#else
/*
 * Stub versions
 */

int
audio_read (int dev, struct fileinfo *file, snd_rw_buf * buf, int count)
{
  return RET_ERROR (EIO);
}

int
audio_write (int dev, struct fileinfo *file, snd_rw_buf * buf, int count)
{
  return RET_ERROR (EIO);
}

int
audio_open (int dev, struct fileinfo *file)
{
  return RET_ERROR (ENXIO);
}

void
audio_release (int dev, struct fileinfo *file)
{
};
int
audio_ioctl (int dev, struct fileinfo *file,
	     unsigned int cmd, unsigned int arg)
{
  return RET_ERROR (EIO);
}

int
audio_lseek (int dev, struct fileinfo *file, off_t offset, int orig)
{
  return RET_ERROR (EIO);
}

long
audio_init (long mem_start)
{
  return mem_start;
}

#endif

#endif
