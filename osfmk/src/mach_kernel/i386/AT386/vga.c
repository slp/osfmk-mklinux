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

#include <vga.h>
#if NVGA > 0

#include <types.h>

#include <mach/vm_prot.h>
#include <mach/i386/vm_types.h>
#include <mach/i386/vm_param.h>

#include <kern/thread.h>
#include <kern/misc_protos.h>

#include <ipc/ipc_port.h>
#include <vm/vm_kern.h>

#include <device/io_req.h>
#include <device/dev_hdr.h>

#include <chips/busses.h>
#include <i386/io_port.h>
#include <i386/eflags.h>
#include <i386/pit.h>
#include <i386/pio.h>
#include <i386/iopb_entries.h>
#include <i386/AT386/vgareg.h>
#include <i386/AT386/vga_entries.h>

/*
 * vga device.
 */
ipc_port_t	vga_device_port = IP_NULL;
device_t	vga_device = 0;

static caddr_t	vga_std[NVGA] = { 0 };
static struct	bus_device *vga_info[NVGA];

static int	vgaprobe(caddr_t port, struct bus_device *dev);
static void	vgaattach(struct bus_device *dev);
void		vgaHWSave(vgaHWPtr save);
void		vgaHWRestore(vgaHWPtr restore);
void		vgaGetClocks(int, void (*)(int));

struct bus_driver vgadriver = {
	(probe_t)vgaprobe, 0, vgaattach, 0, vga_std, "vga", vga_info, 0, 0, 0};

int		vgaPresent = 0;
int		vga_open = 0;
int		vgaIOBase;
u_char		*vgaBase;
int		vgaClock[VGA_MAXCLOCKS];
int		vgaClocks;

extern u_char	*vid_start;
u_char		*screen_vid_start;

vgaHWPtr	vga_text_state = (vgaHWPtr) 0;
vgaHWPtr	vga_curr_state = (vgaHWPtr) 0;

void		(*vgaSave)(void *);
void		(*vgaRestore)(void *);

void		vgaRemap(void);
void		vga_kdb_enter(void);
void		vga_kdb_exit(void);
void		vga_save_text(void);
void		vga_save_text_state(void);
void		vga_restore_text_state(void);
void		vgaGetClocks(int, void (*)(int));

void
vgaRemap(void)
{

	/*
	 * map VGA memory to 0xa0000
	 */
	outb(0x3CE,0x06); outb(0x3CF, inb(0x3CF) & ~0x0C);
	screen_vid_start = vid_start = vgaBase;
}

static int
vgaprobe(caddr_t port, struct bus_device *dev)
{
	int	i, save, tmp;
	int	found = 0;

	if (vgaPresent)
		return 0;

	found = 1;
	save = inb(0x3CC);
	for (i=0; i<256; i++) {
		tmp = save & ~0x0C | i & 0x0C;
		outb(0x3CC, tmp);
		if (inb(0x3CC) & 0x0C != tmp & 0x0C) {
			found = 0;	
			break;
		}
	}
	outb(0x3CC, save);

	if (!found) 
		return 0;

	printf("vga: standard\n");
	vgaPresent = 1;
	return 1;
}

static void
vgaattach(struct bus_device *dev)
{
	int	r1, r2;

	if (!vga_text_state) {
		r1 = kmem_alloc_wired(kernel_map,
				      (vm_offset_t *)&vga_text_state,
				      sizeof(vgaHWRec));
		r2 = kmem_alloc_wired(kernel_map,
				      (vm_offset_t *)&vga_curr_state,
				      sizeof(vgaHWRec));
		if (r1 || r2) {
			printf("vgaattach: error allocating memory!\n");
			if (!r1)
				kmem_free(kernel_map,
					  (vm_offset_t)vga_text_state,
					  sizeof(vgaHWRec));
			if (!r2)
				kmem_free(kernel_map,
					  (vm_offset_t)vga_curr_state,
					  sizeof(vgaHWRec));
			vgaPresent = 0;
			return;
		}
		vgaSave = (void (*)(void *))vgaHWSave;
		vgaRestore = (void (*)(void *))vgaHWRestore;
		vgaBase = (u_char *)phystokv(dev->phys_address);
	}

	if (dev->phys_address == (caddr_t)VGA_FB_ADDR) {
		/*
		 * map VGA memory to 0xa0000
		 */
		vgaRemap();
	}

	/*
	 * save original text state
	 */
	vga_save_text_state();
}

io_return_t
vgaopen(
	dev_t		dev,
	dev_mode_t	flag,
	io_req_t	ior)
{
	if (!vgaPresent) return (D_NO_SUCH_DEVICE);
	if (vga_open) return (D_ALREADY_OPEN);
	vga_open = 1;
	vga_device_port = ior->io_device->port;
	vga_device = ior->io_device;

	vga_save_text();

	return (D_SUCCESS);
}

void
vgaclose(
	dev_t		dev)
{
	vga_restore_text_state();
	io_port_destroy(vga_device);
	vga_device_port = IP_NULL;
	vga_device = 0;
	vga_open = 0;
}

io_return_t
vgagetstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	dev_status_t	data,		/* pointer to OUT array */
	natural_t	*count)		/* OUT */
{
	return (D_SUCCESS);
}

io_return_t
vgasetstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	dev_status_t	data,
	natural_t	count)
{
	return (D_SUCCESS);
}

vm_offset_t
vgammap(
	dev_t		dev,
	vm_offset_t	off,
	vm_prot_t	prot)
{
    /* Get page frame number for the page to be mapped. */

    if (off > (0x100000 - VGA_FB_ADDR))
	return -1;

    return(i386_btop(VGA_FB_ADDR + off));
}

void
vga_kdb_enter(void)
{
	(*vgaSave)((void *)vga_curr_state);
	vid_start = screen_vid_start;
	(*vgaRestore)((void *)vga_text_state);
}

void
vga_kdb_exit(void)
{
	vga_save_text();
	vid_start = vga_text_state->TextInfo;
	(*vgaRestore)((void *)vga_curr_state);
}

void
vga_save_text_state(void)
{
	(*vgaSave)((void *)vga_text_state);
}

void
vga_save_text(void)
{
	bcopy((char *)vgaBase, (char *)vga_text_state->TextInfo, VGA_TEXT_SIZE);
	vid_start = vga_text_state->TextInfo;
}

void
vga_restore_text_state(void)
{
	(*vgaRestore)(vga_text_state);
	vid_start = screen_vid_start;
}

/*
 * $XConsortium: vgaHW.c,v 1.3 91/08/26 15:40:56 gildea Exp $
 *
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

void
vgaHWRestore(vgaHWPtr	restore)
{
  int i, tmp;

  outb(0x3C4, 0x00); outb(0x3C5, restore->Sequencer[0] & 0xFD);

  outb(0x3C2, restore->MiscOutReg);

  outb(vgaIOBase + 0x04, 0x11);
  outb(vgaIOBase + 0x05, 0x00);   /* unlock some important register */

  /*
   * We MUST explicitely restore the font we got, when we entered 
   * graphics mode.
   */

  if((restore->Attribute[0x10] & 0x01) == 0) {
    /*
     * here we switch temporary to 16 color-plane-mode, to simply
     * copy the font-info
     *
     * BUGALLERT: The vga's segment-select register MUST be set appropriate !
     */
    i = inb(vgaIOBase + 0x0A); /* reset flip-flop */
    outb(0x3C0,0x10); outb(0x3C0, 0x01); /* graphics mode */

    outb(0x3C4,0x02); outb(0x3C5, 0x04); /* write to plane 2 */
    outb(0x3C4,0x04); outb(0x3C5, 0x06); /* enable plane graphics */
    outb(0x3CE,0x04); outb(0x3CF, 0x02); /* read plane 2 */
    outb(0x3CE,0x05); outb(0x3CF, 0x00); /* write mode 0, read mode 0 */
    outb(0x3CE,0x06); outb(0x3CF, 0x05); /* set graphics */

    bcopy((char *)restore->FontInfo, (char *)vgaBase, VGA_FONT_SIZE);

    i = inb(vgaIOBase + 0x0A); /* reset flip-flop */
    outb(0x3C0,0x10); outb(0x3C0, 0x0C); /* graphics mode */

    outb(0x3C0,0x10); outb(0x3C0, 0x0C); /* alphanumeric mode */

    outb(0x3C4,0x02); outb(0x3C5, 0x03); /* write to plane 0 and 1 */
    outb(0x3C4,0x04); outb(0x3C5, 0x02); /* even/odd addressing mode */
    outb(0x3CE,0x04); outb(0x3CF, 0x00); /* read plane 0 */
    outb(0x3CE,0x05); outb(0x3CF, 0x10); /* write mode 0, read mode 0 */
    outb(0x3CE,0x06); outb(0x3CF, 0x02); /* set alphanumeric */

    bcopy((char *)restore->TextInfo, (char *)vgaBase, VGA_TEXT_SIZE);

  }

  for (i=0; i<5;  i++) { outb(0x3C4,i); outb(0x3C5, restore->Sequencer[i]); }
  for (i=0; i<21; i++) {
			 tmp = inb(vgaIOBase + 0x0A); /* reset flip-flop */
			 outb(0x3C0,i); outb(0x3C0, restore->Attribute[i]); }
  for (i=0; i<25; i++) { outb(vgaIOBase + 0x04,i);
			 outb(vgaIOBase + 0x05, restore->CRTC[i]); }
  for (i=0; i<9;  i++) { outb(0x3CE,i); outb(0x3CF, restore->Graphics[i]); }
  
  /*
   * Now restore graphics data
   */
  if(restore->Attribute[0x10] & 0x01) {
    bcopy((char *)restore->GraphicsInfo, (char *)vgaBase, VGA_GRAPHICS_SIZE);
  }

  outb(0x3C8,0x00);
  for (i=0; i<768; i++) outb(0x3C9, restore->DAC[i]);

  /* Turn on PAS bit */
  tmp = inb(vgaIOBase + 0x0A);
  outb(0x3C0, 0x20);
}

/*
 *-----------------------------------------------------------------------
 * vgaHWSave --
 *      save the current video mode
 *
 * Results:
 *      pointer to the current mode record.
 *
 * Side Effects: 
 *      None.
 *-----------------------------------------------------------------------
 */

void
vgaHWSave(vgaHWPtr	save)
{
  int           i, tmp;

  /*
   * Here we are, when we first save the videostate. This means we came here
   * to save the original Text (??) mode. Because some drivers may depend
   * on NoClock we set it here to a reasonable value.
   */
  save->NoClock = (inb(0x3CC) >> 2) & 3;

  /*
   * now get the register
   */
  save->MiscOutReg = inb(0x3CC);
  vgaIOBase = (save->MiscOutReg & 0x01) ? 0x3D0 : 0x3B0;
  save->General[1] = inb(0x3CA);
  save->General[2] = inb(0x3C2);
  save->General[3] = inb(vgaIOBase + 0xA);

  for (i=0; i<25; i++) { outb(vgaIOBase + 0x04,i);
			 save->CRTC[i] = inb(vgaIOBase + 0x05); }
  for (i=0; i<5;  i++) { outb(0x3C4,i); save->Sequencer[i]   = inb(0x3C5); }
  for (i=0; i<9;  i++) { outb(0x3CE,i); save->Graphics[i]  = inb(0x3CF); }
  for (i=0; i<21; i++) {
			 tmp = inb(vgaIOBase + 0x0A); /* reset flip-flop */
			 outb(0x3C0,i); save->Attribute[i] = inb(0x3C1);
			 outb(0x3C0, save->Attribute[i]); }
  save->CRTC[17] &= 0x7F;  /* clear protect bit !!! */
  
  /*
   * get the character set of the first non-graphics application
   */
  if (((save->Attribute[0x10] & 0x01) == 0)) {
    /*
     * Here we switch temporary to 16 color-plane-mode, to simply
     * copy the font-info
     *
     * BUGALLERT: The vga's segment-select register MUST be set appropriate !
     */
    i = inb(vgaIOBase + 0x0A); /* reset Attribute flip-flop */
    outb(0x3C0,0x10); outb(0x3C0, 0x01); /* graphics mode */

    outb(0x3C4,0x02); outb(0x3C5, 0x04); /* write to plane 2 */
    outb(0x3C4,0x04); outb(0x3C5, 0x06); /* enable plane graphics */
    outb(0x3CE,0x04); outb(0x3CF, 0x02); /* read plane 2 */
    outb(0x3CE,0x05); outb(0x3CF, 0x00); /* write mode 0, read mode 0 */
    outb(0x3CE,0x06); outb(0x3CF, 0x05); /* set graphics */

    bcopy((char *)vgaBase, (char *)save->FontInfo, VGA_FONT_SIZE);

    i = inb(vgaIOBase + 0x0A); /* reset Attribute flip-flop */
    outb(0x3C0,0x10); outb(0x3C0, 0x0C); /* alphanumeric mode */

    outb(0x3C4,0x02); outb(0x3C5, 0x03); /* write to plane 0 and 1 */
    outb(0x3C4,0x04); outb(0x3C5, 0x02); /* even/odd addressing mode */
    outb(0x3CE,0x04); outb(0x3CF, 0x00); /* read plane 0 */
    outb(0x3CE,0x05); outb(0x3CF, 0x10); /* write mode 0, read mode 0 */
    outb(0x3CE,0x06); outb(0x3CF, 0x02); /* set alphanumeric */

    bcopy((char *)vgaBase, (char *)save->TextInfo, VGA_TEXT_SIZE);

    /*
     * Now set things back, so that save doesn't destroy as well
     */
    i = inb(vgaIOBase + 0xA);
    outb(0x3C0,0x10); outb(0x3C0, save->Attribute[0x10]);

    outb(0x3C4,0x02); outb(0x3C5, save->Sequencer[0x2]);
    outb(0x3C4,0x04); outb(0x3C5, save->Sequencer[0x4]); 
    outb(0x3CE,0x04); outb(0x3CF, save->Graphics[0x4]);
    outb(0x3CE,0x05); outb(0x3CF, save->Graphics[0x5]); 
    outb(0x3CE,0x06); outb(0x3CF, save->Graphics[0x6]);

  } else {
    /*
     * save the graphics data
     */
    bcopy((char *)vgaBase, (char *)save->GraphicsInfo, VGA_GRAPHICS_SIZE);
  }

  /*			 
   * save the colorlookuptable 
   */
  outb(0x3C7,0x00);
  for (i=0; i<768; i++) save->DAC[i] = inb(0x3C9);

  /* Turn on PAS bit */
  tmp = inb(vgaIOBase + 0x0A);
  outb(0x3C0, 0x20);

  return;
}

void
vgaGetClocks(int num, void (*ClockFunc)(int))
{
  int          norm;
  register int status = vgaIOBase + 0x0A;
  unsigned long       i, j, cnt, rcnt, sync;
  int	   savedClock = (inb(0x3CC) & 0x0C) >> 2;

  vgaClocks = 0;

  for (i = 1; i < num; i++) {

    (*ClockFunc)(i);

    cnt  = 0;
    sync = 200000;

    __asm__("cli");
    while ((inb(status) & 0x08) == 0x00) if (sync-- == 0) goto finish;
    while ((inb(status) & 0x08) == 0x08) if (sync-- == 0) goto finish;
    while ((inb(status) & 0x08) == 0x00) if (sync-- == 0) goto finish;

    for (rcnt = 0; rcnt < 5; rcnt++) {
      while (!(inb(status) & 0x08)) cnt++;
      while ((inb(status) & 0x08)) cnt++;
    }

  finish:
    __asm__("sti");

    vgaClock[i] = cnt ? cnt : 1000000;
  }

  for (i = 2; i < num; i++)
    vgaClock[i] = (int)(0.5 +
      (28.322 * vgaClock[1]) / (vgaClock[i]));

  vgaClock[0] = 25;
  vgaClock[1] = 28;

  for (i=0; i < num; i++)
    for (j=i+1; j < num; j++)
      if (vgaClock[i] == vgaClock[j])
        vgaClock[j] = 0;

  vgaClocks = i;

  (ClockFunc)(savedClock);
}

#endif /* NVGA */
