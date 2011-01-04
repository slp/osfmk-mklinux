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
/*
 * Video Relating to PDM class of Macintoshs
 * (6100, 7100, 8100)
 *
 * Michael Burg
 */

#include <ppc/POWERMAC/powermac.h>

/*
 * CLUT color controller
 */

#define	CLUT_ADDR	(PDM_IO_BASE_ADDR+0x24000)
#define	CLUT_DATA	(PDM_IO_BASE_ADDR+0x24001)
#define	CLUT_CONTROL	(PDM_IO_BASE_ADDR+0x24002)

/*
 * Video Controller.
 */

#define	VIDEO_MODE	 (PDM_IO_BASE_ADDR+0x28000)
#define	VIDEO_PIXELDEPTH (PDM_IO_BASE_ADDR+0x28001)
#define MONITOR_ID	 (PDM_IO_BASE_ADDR+0x28002)

/*
 * Sense lines
 */

#define	VPDM_DRIVE_A_LINE	0x03
#define	VPDM_DRIVE_B_LINE	0x05
#define	VPDM_DRIVE_C_LINE	0x06

/* Base address - 1-1 mapped */
#define	VPDM_PHYSADDR		0x100000
#define	VPDM_BASEADDR		0x100000

/* Needed to sort out memory allocation and caching attributes */
#define VPDM_FRAMEBUF_MAX_SIZE	0x096000
#define VPDM_FRAMEBUF_RESERVED  0x100000


/* Video Types - taken from Mac O/S Video.h */

#define	MAC_MONITOR_Zero21Inch			0x00	/* 21" RGB 								*/
#define	MAC_MONITOR_OnePortraitMono		0x14	/* Portrait Monochrome 					*/
#define	MAC_MONITOR_Two12Inch			0x21	/* 12" RGB								*/
#define	MAC_MONITOR_Three21InchRadius		0x31	/* 21" RGB (Radius)						*/
#define	MAC_MONITOR_Three21InchMonoRadius	0x34	/* 21" Monochrome (Radius) 				*/
#define	MAC_MONITOR_Three21InchMono		0x35	/* 21" Monochrome						*/
#define	MAC_MONITOR_FourNTSC			0x0A	/* NTSC 								*/
#define	MAC_MONITOR_FivePortrait		0x1E	/* Portrait RGB 						*/
#define	MAC_MONITOR_SixMSB1			0x03	/* MultiScan Band-1 (12" thru 1Six")	*/
#define	MAC_MONITOR_SixMSB2			0x0B	/* MultiScan Band-2 (13" thru 19")		*/
#define	MAC_MONITOR_SixMSB3			0x23	/* MultiScan Band-3 (13" thru 21")		*/
#define	MAC_MONITOR_SixStandard			0x2B	/* 13"/14" RGB or 12" Monochrome		*/
#define	MAC_MONITOR_SevenPAL			0x00	/* PAL									*/
#define	MAC_MONITOR_SevenNTSC			0x14	/* NTSC 								*/
#define	MAC_MONITOR_SevenVGA			0x17	/* VGA 									*/
#define	MAC_MONITOR_Seven16Inch			0x2D	/* 16" RGB (GoldFish) 				 	*/
#define	MAC_MONITOR_SevenPALAlternate		0x30	/* PAL (Alternate) 						*/
#define	MAC_MONITOR_Seven19Inch			0x3A	/* Third-Party 19Ó						*/
#define	MAC_MONITOR_SevenNoDisplay		0x3F	/* No display connected 	*/			

/* More generic types.. */

#define	DISPLAY_NoDisplay	 0	// No display attached
#define	DISPLAY_Unknown		 1	// A display is present, but it's type is unknown 
#define	DISPLAY_12Inch		 2	// 12" RGB (Rubik)
#define	DISPLAY_Standard	 3	// 13"/14" RGB or 12" Monochrome
#define	DISPLAY_Portrait	 4	// 15" Portait RGB (Manufactured by Radius)
#define	DISPLAY_PortraitMono	 5	// 15" Portrait Monochrome
#define	DISPLAY_16Inch		 6	// 16" RGB (GoldFish)
#define	DISPLAY_19Inch		 7	// 19" RGB (Third Party)
#define	DISPLAY_21Inch		 8	// 21" RGB (Vesuvio, Radius 21" RGB)
#define	DISPLAY_21InchMono	 9	// 21" Monochrome (Kong, Radius 21" Mono)
#define	DISPLAY_VGA		 10	// VGA
#define	DISPLAY_NTSC		 11	// NTSC
#define	DISPLAY_PAL		 12	// PAL
#define	DISPLAY_MultiScanBand1	 13	// MultiScan Band-1 (12" thru 16" resolutions)
#define	DISPLAY_MultiScanBand2	 14	// MultiScan Band-3 (13" thru 19" resolutions)
#define	DISPLAY_MultiScanBand3	 15	// MultiScan Band-3 (13" thru 21" resolutions)

/* Actual physical screen sizes.. (eventually things get mapped to this) */

#define	SCREEN_512x384At60HzNTSC	0x10	// NTSC vertical and horizontal timings
#define	SCREEN_512x384At60Hz		0x20	// Apple's 12" RGB
#define	SCREEN_640x480At50HzPAL		0x30	// PAL_ST
#define	SCREEN_640x480At60HzNTSC	0x40	// NTSC default resolution
#define	SCREEN_640x480At60HzVGA		0x50	// typical VGA monitor
#define	SCREEN_640x480At67Hz		0x60	// Apple's 13" & 14" RGB
#define	SCREEN_640x870At75Hz		0x70 	// Apple's Full Page Dispaly (Portrait)
#define	SCREEN_768x576At50HzPAL		0x80	// PAL_FF
#define	SCREEN_800x600At56HzVGA		0x90	// SVGA	(VESA Standard)
#define	SCREEN_800x600At60HzVGA		0xa0	// SVGA	(VESA Standard)
#define	SCREEN_800x600At72HzVGA		0xb0	// SVGA	(VESA Standard)
#define	SCREEN_800x600At75HzVGA		0xc0	// SVGA at a higher refresh rate
#define	SCREEN_832x624At75Hz		0xd0	// Apple's 16" RGB
#define	SCREEN_1024x768At60HzVGA	0xe0	// VESA 1K-60Hz
#define	SCREEN_1024x768At70HzVGA	0xf0	// VESA 1K-70Hz
#define	SCREEN_1024x768At75HzVGA	0xf1	// VESA 1K-75Hz....higher refresh rate
#define	SCREEN_1024x768At75Hz		0xf2	// Apple's 19" RGB
#define	SCREEN_1152x870At75Hz		0xf3	// Apple's 21" RGB
#define	SCREEN_1280x960At75Hz		0xf4	// Square Pixel version of 1280x1024
#define	SCREEN_1280x1024At75Hz		0xf5	// 

