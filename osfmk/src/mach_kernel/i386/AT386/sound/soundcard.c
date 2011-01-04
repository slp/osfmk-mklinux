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
 * This is the OS specific code for Mach.
 * MPP safe is not included in this porting.
 */
/*
 * sound/386bsd/soundcard.c
 * 
 * Soundcard driver for 386BSD.
 * 
 * Copyright by Hannu Savolainen 1993
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sb.h>
#include <himem.h>

#include <chips/busses.h>

#include <i386/mach_param.h>

#include <i386/AT386/sound/sound_config.h>

#ifdef CONFIGURE_SOUNDCARD

#include <i386/AT386/sound/dev_table.h>

#if 0
#ifdef DEB
#undef DEB
#endif
#define DEB(x) x
#endif /* 0 */

u_int	snd1mask;
u_int	snd2mask;
u_int	snd3mask;
u_int	snd4mask;
u_int	snd5mask;
u_int	snd6mask;
u_int	snd7mask;
u_int	snd8mask;
u_int	snd9mask;

static int      timer_running = 0;

static int      soundcards_installed = 0;	/* Number of installed
						 * soundcards */
static int      soundcard_configured = 0;

static struct fileinfo files[SND_NDEVS];

int             sndprobe (caddr_t port, struct bus_device *dev);
int             sndattach (struct bus_device *dev);

#ifndef MACH_KERNEL
int             sndopen (dev_t dev, int flags);
int             sndclose (dev_t dev, int flags);
int             sndioctl (dev_t dev, int cmd, caddr_t arg, int mode);
int             sndread (int dev, struct uio *uio);
int             sndwrite (int dev, struct uio *uio);
int             sndselect (int dev, int rw);
#endif /* MACH_KERNEL */

struct bus_device *sbinfo[NSB];

static caddr_t sb_std[NSB] = { 0 };
static struct bus_device *sb_info[NSB];
struct bus_driver sbdriver = {
    (probe_t) sndprobe, 0, (void (*)(struct bus_device *)) sndattach,
    0, sb_std, "sb", sb_info, 0, 0, 0 };

char *kalloc_interface(int nbytes)
{
    int *head;
    char *p;

    head = (int *) kalloc(nbytes + 4);
    if (head == NULL) {
	return (NULL);
    }
    *head = nbytes;
    p = (char *) (head + 1);
    return (p);
}

void kfree_interface(char *ptr)
{
    int *head;
    int nbytes;

    head = (int *) ptr;
    head--;
    nbytes = *head;
    kfree((vm_offset_t) head, (vm_size_t) nbytes + 4);
}

#if 0
unsigned long get_time(void)
{
    tvalspec_t t;
  
    rtc_gettime(&t);

    return (t.tv_nsec/(1000000000/HZ) + (unsigned long) t.tv_sec*HZ);
}
#endif

/*
 * Simply call ioctl for getstat and setstat operations in Mach.
 * This sould be rewritten for more strict error check.
 */
io_return_t sndgetstat(dev_t dev, dev_flavor_t flavor, int *data,
		       natural_t *count)
{
    return (sound_ioctl_sw(dev, &files[dev], flavor,
			(unsigned int) data)); 
}

io_return_t sndsetstat(dev_t dev, dev_flavor_t flavor, int *data,
		       natural_t count)
{
    return (sound_ioctl_sw(dev, &files[dev], flavor, 
			(unsigned int) data));
}

int sndread (dev_t dev, io_req_t ior)
{
    int             count = ior->io_count;

    dev = minor(dev);

    FIX_RETURN(sound_read_sw (dev, &files[dev], ior, count));
}

int sndwrite (dev_t dev, io_req_t ior)
{
    int             count = ior->io_count;

    dev = minor (dev);

    return(sound_write_sw (dev, &files[dev], ior, count));
}

io_return_t sndopen(dev_t dev, dev_mode_t flags, io_req_t ior)
{
    int             retval;

    dev = minor(dev);

    if (!soundcard_configured && dev) {
	printk("SoundCard Error: The soundcard system has not been configured\n");
	FIX_RETURN(-ENXIO);
    }

    files[dev].mode = 0;

    if (flags & D_READ && flags & D_WRITE) {
	files[dev].mode = OPEN_READWRITE;
    } else if (flags & D_READ) {
	files[dev].mode = OPEN_READ;
    } else if (flags & D_WRITE) {
	files[dev].mode = OPEN_WRITE;
    }

    FIX_RETURN(sound_open_sw(dev, &files[dev]));
}

void sndclose (dev_t dev)
{
    dev = minor (dev);
    sound_release_sw(dev, &files[dev]);
}

#if 0
int
sndioctl (dev_t dev, int cmd, caddr_t arg, int mode)
{
  dev = minor (dev);

  FIX_RETURN (sound_ioctl_sw (dev, &files[dev], cmd, (unsigned int) arg));
}

int
sndselect (int dev, int rw)
{
  dev = minor (dev);

  DEB (printk ("sound_ioctl(dev=%d, cmd=0x%x, arg=0x%x)\n", dev, cmd, arg));

  FIX_RETURN (0);
}
#endif

static short
ipri_to_irq (unsigned short ipri)
{
  /*
   * Converts the ipri (bitmask) to the corresponding irq number
   */
  int             irq;

  for (irq = 0; irq < 16; irq++)
    if (ipri == (1 << irq))
      return irq;

  return -1;			/* Invalid argument */
}

/*
 * This routine should be called for each part of the sound driver.
 * That is, it is called for audio driver, midi driver ....
 */
int sndprobe (caddr_t port, struct bus_device *dev)
{
    struct address_info hw_config;
    int ret;

    hw_config.io_base = (int) dev->address;
    hw_config.irq = dev->sysdep1;
    /*
     * For sound blaster 16, set DMA address for 16bit DMA.
     */
    hw_config.dma = dev->am;	/* DMA channel is stored in address modifier */

    ret = sndtable_probe(dev->unit, &hw_config);

    return(ret);
}

extern int sb_irq_usecount;

int sndattach (struct bus_device *dev)
{
  int             i;
  static int      midi_initialized = 0;
  static int      seq_initialized = 0;
  static int 	  generic_midi_initialized = 0; 
  unsigned long	  mem_start = 0xefffffff;
  struct address_info hw_config;

  hw_config.io_base = (int) dev->address;
  hw_config.irq = dev->sysdep1;
  hw_config.dma = dev->am;

  if (dev->unit)		/* Card init */
    if (!sndtable_init_card (dev->unit, &hw_config))
      {
	panic(" <Driver not configured>");
	return FALSE;
      }

  /*
   * Init the high level sound driver
   */

  if (!(soundcards_installed = sndtable_get_cardcount ()))
    {
      panic(" <No such hardware>");
      return FALSE;		/* No cards detected */
    }

  if (sb_irq_usecount == 0) {
      take_dev_irq(dev);
      (void) sb_get_irq();
  }


#ifndef EXCLUDE_AUDIO
  DEB(printf("num_audiodevs = %d\n", num_audiodevs));
  if (num_audiodevs)	/* Audio devices present */
    {
      mem_start = DMAbuf_init (mem_start);
    }

  soundcard_configured = 1;
#endif

  if (num_midis && !midi_initialized)
    {
      midi_initialized = 1;
      mem_start = MIDIbuf_init (mem_start);
    }

  if ((num_midis + num_synths) && !seq_initialized)
    {
      seq_initialized = 1;
      mem_start = sequencer_init (mem_start);
    }

  return TRUE;
}

void
tenmicrosec (void)
{
    delay(10);
}

/*
 * count must be evaluated by 10ms (100Hz).
 */
void
request_sound_timer (int count)
{
  static int      current = 0;
  int             tmp;

  /* convert to the unit of 10ms */
  tmp = count = count * HZ / 100;
  if (count < 0)
    timeout ((timeout_fcn_t) sequencer_timer, 0, -count);
  else
    {

      if (count < current)
	current = 0;		/* Timer restarted */

      count = count - current;

      current = tmp;

      if (!count)
	count = 1;

      timeout ((timeout_fcn_t) sequencer_timer, 0, count);
    }
  timer_running = 1;
}

void
sound_stop_timer (void)
{
  if (timer_running)
    untimeout ((timeout_fcn_t) sequencer_timer, 0);
  timer_running = 0;
}

#if 0
struct isa_driver snddriver =
{sndprobe, sndattach, "snd"};
#endif

int snd_ioctl_return (int *addr, int value)
{
    if (value < 0) {
	return (value);		/* Error */
    }

    PUT_WORD_TO_USER(addr, 0, value);
    return 0;
}

int
snd_set_irq_handler (int interrupt_level, void(*hndlr)(int))
{
  return 1;
}

void
snd_release_irq(int vect)
{
}

#endif /* CONFIGURE_SOUNDCARD */
