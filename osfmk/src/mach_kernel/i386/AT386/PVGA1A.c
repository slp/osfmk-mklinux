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
  vgaHWRec std;          /* std IBM VGA register */
  unsigned char PR0A;           /* PVGA1A, WD90Cxx */
  unsigned char PR0B;
  unsigned char MemorySize;
  unsigned char VideoSelect;
  unsigned char CRTCCtrl;
  unsigned char VideoCtrl;
  unsigned char InterlaceStart; /* WD90Cxx */
  unsigned char InterlaceEnd;
  unsigned char MiscCtrl2;
  unsigned char InterfaceCtrl;  /* WD90C1x */
  } vgaPVGA1Rec, *vgaPVGA1Ptr;

static int	PVGA1probe(caddr_t port, struct bus_device *dev);
static void	PVGA1attach(struct bus_device *dev);
static void     PVGA1Save(vgaPVGA1Ptr);
static void     PVGA1Restore(vgaPVGA1Ptr);

static caddr_t	PVGA1_std[NVGA] = { 0 };
static struct	bus_device *PVGA1_info[NVGA];

struct bus_driver PVGA1driver = { 
	(probe_t)PVGA1probe, 0, PVGA1attach, 0, PVGA1_std,
	"pvga1", PVGA1_info, 0, 0, 0 };

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

extern void	vgaHWSave(vgaHWPtr);
extern void	vgaHWRestore(vgaHWPtr);

void
PVGA1attach(struct bus_device *dev)
{
	int	r1, r2;

	r1 = kmem_alloc_wired(kernel_map,
			      (vm_offset_t *)&vga_text_state,
			      sizeof(vgaPVGA1Rec));
	r2 = kmem_alloc_wired(kernel_map,
			      (vm_offset_t *)&vga_curr_state,
			      sizeof(vgaPVGA1Rec));
	if (r1 || r2) {
		printf("PVGA1attach: error allocating memory!\n");
		if (!r1)
			kmem_free(kernel_map,
				  (vm_offset_t)vga_text_state,
				  sizeof(vgaPVGA1Rec));
		if (!r2)
			kmem_free(kernel_map,
				  (vm_offset_t)vga_curr_state,
				  sizeof(vgaPVGA1Rec));
		vgaPresent = 0;
		return;
	}
	vgaSave = (void (*)(void *))PVGA1Save;
	vgaRestore = (void (*)(void *))PVGA1Restore;
	vgaBase = (u_char *)phystokv(dev->phys_address);

	if (dev->phys_address == (caddr_t)VGA_FB_ADDR) {
		vgaRemap();
	}

	/*
	 * Save the original text state
	 */
	vga_save_text_state();
}

static void
PVGA1ClockSelect(int no)
{
  unsigned char temp;

  temp = inb(0x3CC);
  outb(0x3C2, ( temp & 0xf3) | ((no << 2) & 0x0C));
  outw(0x3CE, 0x0C | ((no & 0x04) << 7));
}

static int
PVGA1probe(caddr_t port, struct bus_device *dev)
{
  char tmp[5];
  char *pvga;
  int base,i;
  int value0, value1, old_value;
  int type;
  unsigned char config;
  int videoRam;

  if (vgaPresent) return 0;

  /* PVGA1 ? */
  bcopy((char *)phystokv(0xC007D), (char *)tmp, 4);
  tmp[4] = '\0';
  for (pvga="VGA=",i=0; tmp[i] == *pvga && i < 4; pvga++, i++);
  if (i == 4) printf("vga: Paradise VGA ");
  else return 0;

  outb(0x3CE, 0x0B); config = inb(0x3CF);

  switch(config & 0xC0) {
  case 0x00:
  case 0x40:
    videoRam = 256;
    break;
  case 0x80:
    videoRam = 512;
    break;
  case 0xC0:
    videoRam = 1024;
    break;
  }

  if((inb(0x3cc) & 1) == 1) base = 0x3d0; 
  else base = 0x3b0;

  /* 
   * PVGA1 
   * no scratch register, then PVGA1 
   */
  outb(base+4, 0x2b); old_value = inb(base+5);
  outb(base+5, 0xaa); value0 = inb(base+5);
  outb(base+5, old_value);

  if(value0 != 0xaa) {
    printf("(PVGA1)"); 
    vgaPresent++;
    type = 1;
    goto found;
  }

  /*
   * wd90c00
   * 
   */
  outb(0x3c4,0x12); old_value = inb(0x3c5);
  outb(0x3c5,old_value&0xbf); value0 = inb(0x3c5)&0x40;
  outb(0x3c5,old_value|0x40); value1 = inb(0x3c5)&0x40;
  outb(0x3c5,old_value);
  if(value0 || !value1) {
    printf("(wd90c00)");
    vgaPresent++;
    type = 2;
    goto found;
  }

  /*
   * wd90c10
   *
   */
  outb(0x3c4,0x10); old_value = inb(0x3c5);
  outb(0x3c5,old_value&0xbf); value0 = inb(0x3c5)&0x40;
  outb(0x3c5,old_value|0x40); value1 = inb(0x3c5)&0x40;
  outb(0x3c5,old_value);
  if(value0 || !value1) {
    printf("(wd90c10)");
    vgaPresent++;
    type = 3;
    goto found;
  }

  /*
   * Has everything, must be a wd90c11
   */
  printf("(wd90c11)");
  vgaPresent++;
  type = 4;

  found:

#if 0
  vgaGetClocks(8, PVGA1ClockSelect);
#endif

  vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;

  printf(", memory = %dK bytes", videoRam);
  if (vgaClocks) {
    printf(", clocks = ");
    for (i=0; i<vgaClocks; i++)
      printf(" %d", vgaClock[i]);
  }
  printf("\n");

  return type;
}

/*
 * PVGA1Restore --
 *      restore a video mode
 */

static void
PVGA1Restore(vgaPVGA1Ptr restore)
{
  unsigned char temp;

  /*
   * First unlock all these special registers ...
   * NOTE: Locking will not be fully renabled !!!
   */
  outb(0x3CE, 0x0D); temp = inb(0x3CF); outb(0x3CF, temp & 0x1C);
  outb(0x3CE, 0x0E); temp = inb(0x3CF); outb(0x3CF, temp & 0xFB);

  outb(vgaIOBase + 4, 0x2A); temp = inb(vgaIOBase + 5);
  outb(vgaIOBase + 5, temp & 0xF8);

  outw(0x3CE, 0x0009);   /* segment select A */
  outw(0x3CE, 0x000A);   /* segment select B */

  vgaHWRestore((vgaHWPtr)restore);

  outw(0x3CE, (restore->PR0A << 8) | 0x09);
  outw(0x3CE, (restore->PR0B << 8) | 0x0A);

  outb(0x3CE, 0x0B); temp = inb(0x3CF);          /* banking mode ... */
  outb(0x3CF, (temp & 0xF7) | (restore->MemorySize & 0x08));
       
  outw(0x3CE, (restore->VideoSelect << 8) | 0x0C);
  outw(0x3CE, (restore->CRTCCtrl << 8)    | 0x0D);
  outw(0x3CE, (restore->VideoCtrl << 8)   | 0x0E);
  
  /*
   * Now the WD90Cxx specials (Register Bank 2)
   */
  outw(vgaIOBase + 4, (restore->InterlaceStart << 8) | 0x2C);
  outw(vgaIOBase + 4, (restore->InterlaceEnd << 8)   | 0x2D);
  outw(vgaIOBase + 4, (restore->MiscCtrl2 << 8)      | 0x2F);

  /*
   * For the WD90C10 & WD90C11 we have to select segment mapping.
   * NOTE: Only bit7 is save/restored !!!!
   */
  outb(0x3C4, 0x11); temp = inb(0x3C5);
  outb(0x3C5, (temp & 0x7F) | (restore->InterfaceCtrl & 0x80));

  outw(0x3C4, 0x0300); /* now reenable the timing sequencer */
}

/*
 * PVGA1Save --
 *      save the current video mode
 */

static void
PVGA1Save(vgaPVGA1Ptr save)
{
  unsigned char PR0A, PR0B, temp;

  vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
  outb(0x3CE, 0x0D); temp = inb(0x3CF); outb(0x3CF, temp & 0x1C);
  outb(0x3CE, 0x0E); temp = inb(0x3CF); outb(0x3CF, temp & 0xFD);

  outb(vgaIOBase + 4, 0x2A); temp = inb(vgaIOBase + 5);
  outb(vgaIOBase + 5, temp & 0xF8);

  outb(0X3CE, 0x09); PR0A = inb(0x3CF); outb(0x3CF, 0x00);
  outb(0X3CE, 0x0A); PR0B = inb(0x3CF); outb(0x3CF, 0x00);

  vgaHWSave((vgaHWPtr)save);

  save->PR0A = PR0A;
  save->PR0B = PR0B;
  outb(0x3CE, 0x0B); save->MemorySize  = inb(0x3CF);
  outb(0x3CE, 0x0C); save->VideoSelect = inb(0x3CF);
  outb(0x3CE, 0x0D); save->CRTCCtrl    = inb(0x3CF);
  outb(0x3CE, 0x0E); save->VideoCtrl   = inb(0x3CF);

  /* WD90Cxx */
  outb(vgaIOBase + 4, 0x2C); save->InterlaceStart = inb(vgaIOBase+5);
  outb(vgaIOBase + 4, 0x2D); save->InterlaceEnd   = inb(vgaIOBase+5);
  outb(vgaIOBase + 4, 0x2F); save->MiscCtrl2      = inb(vgaIOBase+5);

  /* WD90C1x */
  outb(0x3C4, 0x11); save->InterfaceCtrl = inb(0x3C5);
}
#endif
