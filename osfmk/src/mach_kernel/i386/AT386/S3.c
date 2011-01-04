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
	vgaHWRec std;
	unsigned char s3reg[10];
	unsigned char s3sysreg[36];
} vgaS3Rec, *vgaS3Ptr;

static int	S3probe(caddr_t port, struct bus_device *dev);
static void	S3attach(struct bus_device *dev);
static void     S3Save(vgaS3Ptr);
static void     S3Restore(vgaS3Ptr);

static caddr_t	S3_std[NVGA] = { 0 };
static struct	bus_device *S3_info[NVGA];

struct bus_driver S3driver = {
	(probe_t)S3probe, 0, S3attach, 0, S3_std,
	"s3", S3_info, 0, 0, 0 };

extern int	vgaPresent;
extern u_char	*vgaBase;
extern int	vgaIOBase;

extern void	(*vgaSave)(void *);
extern void	(*vgaRestore)(void *);
extern void	vgaRemap(void);
extern void	vga_save_text_state(void);

extern void	*vga_text_state;
extern void	*vga_curr_state;

extern void	vgaHWSave(vgaHWPtr);
extern void	vgaHWRestore(vgaHWPtr);

static unsigned char	s3chipid, s3oldclock;
static unsigned short	s3reg50mask = 0x4003;

#define S3_911_ONLY(chip)     (chip==0x81)
#define S3_924_ONLY(chip)     (chip==0x82)
#define S3_801_ONLY(chip)       (chip==0xa0)
#define S3_928_ONLY(chip)       (chip==0x90)
#define S3_911_SERIES(chip)     ((chip&0xf0)==0x80)
#define S3_801_SERIES(chip)     ((chip&0xf0)==0xa0)
#define S3_928_SERIES(chip)     ((chip&0xf0)==0x90)
#define S3_964_SERIES(chip)     ((chip&0xf0)==0xd0)
#define S3_968_SERIES(chip)	((chip&0xf0)==0xe0)
#define S3_801_928_SERIES(chip) (S3_801_SERIES(chip) || \
				 S3_928_SERIES(chip) || \
				 S3_964_SERIES(chip) || \
				 S3_968_SERIES(chip))
#define S3_8XX_9XX_SERIES(chip) (S3_911_SERIES(chip) || \
				 S3_801_928_SERIES(chip))
#define S3_ANY_SERIES(chip)     (S3_8XX_9XX_SERIES(chip))

/*
 * This macro clears the two extended
 * bank switch bits.
 */

#define	cebank() do {							\
   	if (S3_801_928_SERIES(s3chipid)) {				\
		outb(vgaIOBase+4, 0x51);				\
		outb(vgaIOBase+5, (inb(vgaIOBase+5) & 0xf3));		\
	}								\
} while (1 == 0)

void
S3attach(struct bus_device *dev)
{
	int	i, size;
	int	r1, r2;

	r1 = kmem_alloc_wired(kernel_map,
			      (vm_offset_t *)&vga_text_state,
			      sizeof(vgaS3Rec));
	r2 = kmem_alloc_wired(kernel_map,
			      (vm_offset_t *)&vga_curr_state,
			      sizeof(vgaS3Rec));
	if (r1 || r2) {
		printf("S3attach: error allocating memory!\n");
		if (!r1)
			kmem_free(kernel_map,
				  (vm_offset_t)vga_text_state,
				  sizeof(vgaS3Rec));
		if (!r2)
			kmem_free(kernel_map,
				  (vm_offset_t)vga_curr_state,
				  sizeof(vgaS3Rec));
		vgaPresent = 0;
		return;
	}
	vgaSave = (void (*)(void *))S3Save;
	vgaRestore = (void (*)(void *))S3Restore;
	vgaBase = (u_char *)phystokv(dev->phys_address);

	outb(vgaIOBase+4, 0x36);
	i = (inb(vgaIOBase+5) & 0xe0) >> 5;
	switch (i) {
		case 0:
			size = 4 * 1024 * 1024;
			break;
		case 2:
			size = 3 * 1024 * 1024;
			break;
		case 4:
			size = 2 * 1024 * 1024;
			break;
		case 6:
			size = 1024 * 1024;
			break;
		case 7:
			size = 512 * 1024;
			break;
		default:
			size = 1024 * 1024;
	}

	if (dev->phys_address == (caddr_t)VGA_FB_ADDR) {
		vgaRemap();
	}

	/*
	 * Save the original text state
	 */
	vga_save_text_state();
}

static int
S3probe(caddr_t port, struct bus_device *dev)
{
	register int i;

	if (vgaPresent)
		return 0;

       /*
	* For S3 we must unlock S3R8 lock register
	*/

	vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
	outb(vgaIOBase+4,0x38);
	outb(vgaIOBase+5, 0x48);

	/*
	 * Unlock CR0 through CR7.
	 */

	outb(vgaIOBase+0x4,0x11); i = inb(vgaIOBase+5);
	outb(vgaIOBase+5, i & 0x7f);

       /* 
	* Fetch board ID register. Note 0x82 value not 0x81 value as
	* stated in 86c911 chip spec. We currently know of:
	*
	*	ID	S3 Chip		Vendor
 	*      ----    ---------	------
	*      0x82       911		  
 	*      0x90	  928	  	Number 9	
	*      0xa0	  801	 	Actix GraphicsEngine 32 (beta?)
	*      0xd0	  964	 	Diamond Stealth 
	*      0xe0	  968	 	Diamond Stealth 
	*/

	outb(vgaIOBase+4, 0x30); s3chipid = i = (inb(vgaIOBase+5) & 0xF0);
	printf("S3 chip ID probe returned 0x%x\n", s3chipid);
	if (i == 0x82 || i == 0x90 || i == 0xa0 || i == 0xd0 || i == 0xe0) {
		char tmp[70];

		/*
		 * Unlock second set of VGA registers.
		 */

		outb(vgaIOBase+4,0x39);
		outb(vgaIOBase+5, 0xa5);

		/*
		 * Get video BIOS vendor string
		 */

		if (i == 0x82) {
			bcopy((char *)phystokv(0xC0032), tmp, 69);
			tmp[69] = 0;
		}
		if (i == 0x90) {
			bcopy((char *)phystokv(0xC008e), tmp, 69);
			tmp[69] = 0;
		}
		if (i == 0xa0) {
			bcopy((char *)phystokv(0xC00E4), tmp, 69);
			tmp[69] = 0;
		}

		if (i == 0xd0) {
			bcopy((char *)phystokv(0xC0043), tmp, 59);
			tmp[59] = 0;
		}
		if (i == 0xe0) {
			bcopy((char *)phystokv(0xC0043), tmp, 59);
			tmp[59] = 0;
		}
		printf("%s\n", tmp);
		vgaPresent++;
		return 1;
	}
	else return 0;
}

/*
 *-----------------------------------------------------------------------
 * S3Restore --
 *      restore a video mode
 *
 * Results:
 *      nope.
 *
 * Side Effects: 
 *      the display enters a new graphics mode. 
 *-----------------------------------------------------------------------
 */
void 
S3Restore(vgaS3Ptr restore)
{
	register int i, tmp;

	for (i = 1000; (inw(0x9ae8) & 0x0100) && i; i--);
	if (inw(0x9ae8) & 0x100) {
		outw(0x42e8, 0x8000|0x1000|0x8|0x4);
	}

	/*
	 * Clear the advanced function control register
	 */

	outw(0x4ae8, 0);

	/*
	 * Reset the timing
	 */

	if (!S3_801_928_SERIES(s3chipid)) {
		/* set MODE-CTL */
		outb(vgaIOBase+4, 0x42); outb(vgaIOBase+5, 0x08);   
		tmp = inb(0x3C6); tmp &= 0xf3; outb(0x3C6, tmp);
		/* reset clock */
		outb(vgaIOBase+4, 0x42); outb(vgaIOBase+5, 0x01);   
		outb(0x3C6, 0xff);
	}

	outb(vgaIOBase+4, 0x31);
	outb(vgaIOBase+5, 0x8d);

	/*
	 * Select segment zero
	 */

	outb(vgaIOBase+4, 0x35);
	i = inb(vgaIOBase+5);
	outb(vgaIOBase+5, i & 0xf0);
	cebank();

	vgaHWRestore((void *)restore);

	/*
	 * Restore the S3 registers
	 */

	for (i = 0; i < 5; i++) {
		outb(vgaIOBase+4, 0x30 + i);
		outb(vgaIOBase+5, restore->s3reg[i]);
		outb(vgaIOBase+4, 0x38 + i);
		outb(vgaIOBase+5, restore->s3reg[5 + i]);
	}

	for (i = 0; i < 16; i++) {
		outb(vgaIOBase+4, 0x40 + i);
		outb(vgaIOBase+5, restore->s3sysreg[i]);
	}

	/*
	 * Enable PAS
	 */

	i = inb(vgaIOBase + 0x0A);
	outb(0x3c0, 0x20);

	if (S3_801_928_SERIES(s3chipid)) {
		for (i = 32; i < 35; i++) {
			outb(vgaIOBase+4, 0x40 + i);
			outb(vgaIOBase+5, restore->s3sysreg[i]);
		}
		for (i = 0; i < 16; i++) {
			if (!((1 << i) & s3reg50mask))
				continue;
			outb(vgaIOBase+4, 0x50 + i);
			outb(vgaIOBase+5, restore->s3sysreg[i + 16]);
		}
		outb(0x3c2, s3oldclock);
	}

	/*
	 * Start timing sequencer.
	 */

	outw(0x3C4,0x0300); 
}

/*
 *-----------------------------------------------------------------------
 * S3Save --
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
S3Save(vgaS3Ptr save)
{
	register unsigned i, tmp;
	static first_time = 1;

	if (first_time) {
		first_time = 0;
	
		for (i = 0; i < 5; i++) {
			outb(vgaIOBase+4, 0x30 + i);
			save->s3reg[i] = inb(vgaIOBase+5);
			outb(vgaIOBase+4, 0x38 + i);
			save->s3reg[5 + i] = inb(vgaIOBase+5);
		}


		for (i = 0; i < 16 ; i++) {
			outb(vgaIOBase+4, 0x40 + i);
			save->s3sysreg[i] = inb(vgaIOBase+5);
		}


		if (S3_801_928_SERIES(s3chipid)) {
			s3oldclock = inb(0x3cc);

			for (i = 0; i < 16; i++) {
				if (!((1 << i) & s3reg50mask))
					continue;
				outb(vgaIOBase+4, 0x50 + i);
				save->s3sysreg[i + 16] = inb(vgaIOBase+5);
			}

			for (i = 32; i < 35; i++) {
				outb(vgaIOBase+4, 0x40 + i);
				save->s3sysreg[i] = inb(vgaIOBase+5);
			}
		}
	}

	/*
	 * Select segment zero
	 */

	outb(vgaIOBase+4, 0x35);
	i = inb(vgaIOBase+5);
	outb(vgaIOBase+5, i & 0xf0);
	cebank();

	vgaHWSave((void *)save);


	i = inb(vgaIOBase + 0x0A);
	outb(0x3c0, 0x20);

	return;
}

#endif
