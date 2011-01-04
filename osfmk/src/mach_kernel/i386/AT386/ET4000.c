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
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 *
 */

#include <vga.h>
#if NVGA > 0

#include <kern/misc_protos.h>

#include <ipc/ipc_port.h>
#include <vm/vm_kern.h>

#include <device/io_req.h>
#include <device/dev_hdr.h>

#include <chips/busses.h>
#include <i386/io_port.h>
#include <i386/pio.h>

#include <i386/AT386/vgareg.h>
#include <i386/AT386/vga_entries.h>

typedef struct {
  vgaHWRec std;               /* good old IBM VGA */
  unsigned char ExtStart;     /* Tseng ET4000 specials   CRTC 0x33/0x34/0x35 */
  unsigned char Compatibility;
  unsigned char OverflowHigh;
  unsigned char StateControl;    /* TS 6 & 7 */
  unsigned char AuxillaryMode;
  unsigned char Misc;           /* ATC 0x16 */
  unsigned char SegSel;
  } vgaET4000Rec, *vgaET4000Ptr;

static u_char	save_divide = 0;

static void	(*ClockSelect)(int);

static int	ET4000probe(caddr_t port, struct bus_device *dev);
static void	ET4000attach(struct bus_device *dev);

static void	ET4000ClockSelect(int);
static void	LegendClockSelect(int);
static void	ET4000Lock(void);
static void	ET4000Unlock(void);
static void     ET4000Save(vgaET4000Ptr);
static void     ET4000Restore(vgaET4000Ptr);

static caddr_t	ET4000_std[NVGA] = { 0 };
static struct	bus_device *ET4000_info[NVGA];	

struct bus_driver ET4000driver = {
	(probe_t)ET4000probe, 0, ET4000attach, 0, ET4000_std,
	"et4000", ET4000_info, 0, 0, 0 };

extern int	vgaPresent;
extern u_char	*vgaBase;
extern int	vgaIOBase;
extern int	vgaClock[VGA_MAXCLOCKS];
extern int	vgaClocks;

extern void	(*vgaSave)(void *);
extern void	(*vgaRestore)(void *);
extern void	vgaGetClocks(int, void (*)(int));
extern void	vgaRemap(void);
extern void	vga_save_text_state(void);

extern void	*vga_text_state;
extern void	*vga_curr_state;

extern void     vgaHWSave(vgaHWPtr);
extern void     vgaHWRestore(vgaHWPtr);

void
ET4000attach(struct bus_device *dev)
{
	int	r1, r2;

	r1 = kmem_alloc_wired(kernel_map,
			      (vm_offset_t *)&vga_text_state,
			      sizeof(vgaET4000Rec));
	r2 = kmem_alloc_wired(kernel_map,
			      (vm_offset_t *)&vga_curr_state,
			      sizeof(vgaET4000Rec));
	if (r1 || r2) {
		printf("ET4000attach: error allocating memory!\n");
		if (!r1)
			kmem_free(kernel_map,
				  (vm_offset_t)vga_text_state,
				  sizeof(vgaET4000Rec));
		if (!r2)
			kmem_free(kernel_map,
				  (vm_offset_t)vga_curr_state,
				  sizeof(vgaET4000Rec));
		vgaPresent = 0;
		return;
	}
	vgaSave = (void (*)(void *))ET4000Save;
	vgaRestore = (void (*)(void *))ET4000Restore;
	vgaBase = (u_char *)phystokv(dev->phys_address);

	if (dev->phys_address == (caddr_t)VGA_FB_ADDR) {
		vgaRemap();
	}

	/*
	 * Save the original text state
	 */
	vga_save_text_state();
}

/*
 * ET4000ClockSelect --
 *      select one of the possible clocks ...
 */

static void
ET4000ClockSelect(int no)
{
  unsigned char temp;

  temp = inb(0x3CC);
  outb(0x3C2, ( temp & 0xf3) | ((no << 2) & 0x0C));
  outb(vgaIOBase + 4, 0x34 | ((no & 0x04) << 7));
  outb(vgaIOBase + 5, ((no & 0x04) >> 1));

  outb(0x3C4, 7); temp = inb(0x3C5);
  outb(0x3C5, (save_divide ^ ((no & 0x08) << 3)) | (temp & 0xBF));
}

/*
 * LegendClockSelect --
 *      select one of the possible clocks ...
 */

static void
LegendClockSelect(int no)
{
  /*
   * Sigma Legend special handling
   *
   * The Legend uses an ICS 1394-046 clock generator.  This can generate 32
   * different frequencies.  The Legend can use all 32.  Here's how:
   *
   * There are two flip/flops used to latch two inputs into the ICS clock
   * generator.  The five inputs to the ICS are then
   *
   * ICS     ET-4000
   * ---     ---
   * FS0     CS0
   * FS1     CS1
   * FS2     ff0     flip/flop 0 output
   * FS3     CS2
   * FS4     ff1     flip/flop 1 output
   *
   * The flip/flops are loaded from CS0 and CS1.  The flip/flops are
   * latched by CS2, on the rising edge. After CS2 is set low, and then high,
   * it is then set to its final value.
   *
   */
  unsigned char temp = inb(0x3CC);

  outb(0x3C2, (temp & 0xF3) | ((no & 0x10) >> 1) | (no & 0x04));
  outb(vgaIOBase + 4, 0x34); outb(vgaIOBase + 5, 0x00);
  outb(vgaIOBase + 4, 0x34); outb(vgaIOBase + 5, 0x02);
  outb(vgaIOBase + 4, 0x34); outb(vgaIOBase + 5, (no & 0x08) >> 2);
  outb(0x3C2, (temp & 0xF3) | ((no << 2) & 0x0C));
}

static void
ET4000Unlock(void)
{
  u_char	temp;

  outb(0x3BF, 0x03);                           /* unlock ET4000 special */
  outb(vgaIOBase + 8, 0xA0);
  outb(vgaIOBase + 4, 0x11); temp = inb(vgaIOBase + 5);
  outb(vgaIOBase + 5, temp & 0x7F);
}

static void
ET4000Lock(void)
{
  outb(0x3BF, 0x01);                           /* relock ET4000 special */
  outb(vgaIOBase + 8, 0xA0);
}

static int
ET4000probe(caddr_t port, struct bus_device *dev)
{
  unsigned char temp, origVal, newVal, config;
  int videoRam;
  int i, numClocks;


  if (vgaPresent) return 0;

  vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;

  ET4000Unlock();

  /*
   * Check first that there is a ATC[16] register and then look at
   * CRTC[33]. If both are R/W correctly it's a ET4000 !
   */
  temp = inb(vgaIOBase+0x0A);

  outb(0x3C0, 0x16); origVal = inb(0x3C1);
  outb(0x3C0, origVal ^ 0x10);
  outb(0x3C0, 0x16); newVal = inb(0x3C1);
  outb(0x3C0, origVal);
  if (newVal != (origVal ^ 0x10)) {
    ET4000Lock();
    return 0;
  }

  outb(vgaIOBase+0x04, 0x33);          origVal = inb(vgaIOBase+0x05);
  outb(vgaIOBase+0x05, origVal ^ 0x0F); newVal = inb(vgaIOBase+0x05);
  outb(vgaIOBase+0x05, origVal);
  if (newVal != (origVal ^ 0x0F)) {
    ET4000Lock();
    return 0;
  }

  outb(vgaIOBase+0x04, 0x36); config = inb(vgaIOBase+0x05);
  outb(vgaIOBase+0x05, config & ~(1<<7));

  outb(vgaIOBase+0x04, 0x37); config = inb(vgaIOBase+0x05);

  switch(config & 0x03) {
  case 1: videoRam = 256; break;
  case 2: videoRam = 512; break;
  case 3: videoRam = 1024; break;
  }

  if (config & 0x80) videoRam <<= 1;

  /* Check for divide flag */
  outb(0x3C4, 7);
  save_divide = inb(0x3C5) & 0x40;

  temp = inb(vgaIOBase + 0x0A);
  outb(0x3C0, 0x20);

  /*
   * Find all the clock frequencies. See if LegendClockSelect() generates
   * more than 16.
   */
  ClockSelect = LegendClockSelect;

#if 0
  vgaGetClocks(VGA_MAXCLOCKS, ClockSelect);
  for (i=0, numClocks=0; i<VGA_MAXCLOCKS; i++)
    if (vgaClock[i])
      numClocks++;

  if (numClocks <= 16) {
    ClockSelect = LegendClockSelect;
    vgaGetClocks(16, ClockSelect);
  }
#endif

  printf("vga: ET4000%s, memory=%dK bytes",
	 ClockSelect == LegendClockSelect ?
	 "(Legend)" : "", videoRam);
  if (vgaClocks) {
    printf(", clocks=");
    for (i=0; i<vgaClocks; i++)
      printf(" %d", vgaClock[i]);
  }
  printf("\n");

  ET4000Lock();

  vgaPresent++;
  return 1;
}

/*
 * ET4000Restore --
 *      restore a video mode
 */

static void 
ET4000Restore(vgaET4000Ptr restore)
{
  unsigned char i;

  ET4000Unlock();

  outb(0x3CD, 0x00); /* segment select */

  vgaHWRestore((void *)restore);

  (ClockSelect)(restore->std.NoClock);

  outb(0x3C4, 0x06); outb(0x3C5, restore->StateControl);
  outb(0x3C4, 0x07); outb(0x3C5, restore->AuxillaryMode);
  i = inb(vgaIOBase + 0x0A); /* reset flip-flop */
  outb(0x3C0, 0x36); outb(0x3C0, restore->Misc);
  outb(vgaIOBase + 0x04, 0x33); outb(vgaIOBase + 0x05, restore->ExtStart);
  outb(vgaIOBase + 0x04, 0x34); outb(vgaIOBase + 0x05, restore->Compatibility);
  outb(vgaIOBase + 0x04, 0x35); outb(vgaIOBase + 0x05, restore->OverflowHigh);
  outb(0x3CD, restore->SegSel);

  ET4000Lock();

  outb(0x3C4, 0x00); outb(0x3C5,0x03); /* now reenable the timing sequencer */
}

/*
 * ET4000Save --
 *      save the current video mode
 */

static void
ET4000Save(vgaET4000Ptr save)
{
  unsigned char             i;
  unsigned char             temp1, temp2;

  /*
   * we need this here , cause we MUST disable the ROM SYNC feature
   */
  ET4000Lock();

  outb(vgaIOBase + 4, 0x34); temp1 = inb(vgaIOBase + 5);
  outb(vgaIOBase + 5, temp1 & 0x0F);
  temp2 = inb(0x3CD); outb(0x3CD, 0x00); /* segment select */

  vgaHWSave((void *)save);
  save->Compatibility = temp1;
  save->SegSel = temp2;

  outb(vgaIOBase + 4, 0x33); save->ExtStart     = inb(vgaIOBase + 5);
  outb(vgaIOBase + 4, 0x35); save->OverflowHigh = inb(vgaIOBase + 5);
  outb(0x3C4, 6); save->StateControl  = inb(0x3C5);
  outb(0x3C4, 7); save->AuxillaryMode = inb(0x3C5);
  i = inb(vgaIOBase + 0x0A); /* reset flip-flop */
  outb(0x3C0,0x36); save->Misc = inb(0x3C1); outb(0x3C0, save->Misc);

}
#endif
