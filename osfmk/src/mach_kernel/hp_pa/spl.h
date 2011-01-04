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
 * Copyright (c) 1990,1991,1992 The University of Utah and
 * the Center for Software Science at the University of Utah (CSS).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: spl.h 1.9 92/01/30$
 *	Author: Jeff Forys, Bob Wheeler, University of Utah CSS
 */

/*
 *	SPL definitions
 */

#ifndef	_HP700_SPL_H_
#define	_HP700_SPL_H_

/*
 * These are the bits in the eirr register that are assigned for various 
 * devices
 */
#define SPL_CLOCK_BIT		0
#define SPL_POWER_BIT		1
#define SPL_VM_BIT		4
#define SPL_CIO_BIT		8
#define SPL_HPIB_BIT		9
#define SPL_BIO_BIT		9
#define SPL_IMP_BIT		9
#define SPL_TTY_BIT		20
#define SPL_NET_BIT		28

#define SPL_NAMES \
	"clock", "power", NULL, NULL, "VM", NULL, NULL, NULL, \
	"CIO", "BIO", NULL, NULL, NULL, NULL, NULL, NULL, \
	NULL, NULL, NULL, NULL, "tty", NULL, NULL, NULL, \
	NULL, NULL, NULL, NULL, "network"

/*
 * While the original 8 SPL's were "plenty", the PA-RISC chip provides us
 * with 32 possible interrupt levels.  We take advantage of this by using
 * the standard SPL names (e.g. splbio, splimp) and mapping them into the
 * PA-RISC interrupt scheme.  Obviously, to understand how SPL's work on
 * the PA-RISC, one must first have an understanding as to how interrupts
 * are handled on these chips!
 *
 * Briefly, the CPU has a 32-bit control register for External Interrupt
 * Requests (EIR).  Each bit corresponds to a specific external interrupt.
 * Bits in the EIR can be masked by the External Interrupt Enable Mask
 * (EIEM) control register.  Zero bits in the EIEM mask pending external
 * interrupt requests for the corresponding bit positions.  Finally, the
 * PSW I-bit must be set to allow interrupts to occur.
 *
 * SPL values then, are possible values for the EIEM.  For example, SPL0
 * would set the EIEM to 0xffffffff (enable all external interrupts), and
 * SPLCLOCK would set the EIEM to 0x0 (disable all external interrupts).
 */

/*
 * Define all possible External Interrupt Enable Masks (EIEMs).
 */
#define	INTPRI_00	0x00000000
#define	INTPRI_01	0x80000000
#define	INTPRI_02	0xc0000000
#define	INTPRI_03	0xe0000000
#define	INTPRI_04	0xf0000000
#define	INTPRI_05	0xf8000000
#define	INTPRI_06	0xfc000000
#define	INTPRI_07	0xfe000000
#define	INTPRI_08	0xff000000
#define	INTPRI_09	0xff800000
#define	INTPRI_10	0xffc00000
#define	INTPRI_11	0xffe00000
#define	INTPRI_12	0xfff00000
#define	INTPRI_13	0xfff80000
#define	INTPRI_14	0xfffc0000
#define	INTPRI_15	0xfffe0000
#define	INTPRI_16	0xffff0000
#define	INTPRI_17	0xffff8000
#define	INTPRI_18	0xffffc000
#define	INTPRI_19	0xffffe000
#define	INTPRI_20	0xfffff000
#define	INTPRI_21	0xfffff800
#define	INTPRI_22	0xfffffc00
#define	INTPRI_23	0xfffffe00
#define	INTPRI_24	0xffffff00
#define	INTPRI_25	0xffffff80
#define	INTPRI_26	0xffffffc0
#define	INTPRI_27	0xffffffe0
#define	INTPRI_28	0xfffffff0
#define	INTPRI_29	0xfffffff8
#define	INTPRI_30	0xfffffffc
#define	INTPRI_31	0xfffffffe
#define	INTPRI_32	0xffffffff

/*
 * Convert PA-RISC EIEMs into machine-independent SPLs as follows:
 *
 *                        1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
 *  0 1 2 3 4 5 6 7  8  9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+---+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |c|p| | |v| | | |b i| | | | | | | | | | | |t| | | | | | | |n| |s| |
 * |l|w| | |m| | | |i m| | | | | | | | | | | |t| | | | | | | |e| |c| |
 * |k|r| | | | | | |o p| | | | | | | | | | | |y| | | | | | | |t| |l| |
 * | | | | | | | | |   | | | | | | | | | | | | | | | | | | | | | |k| |
 * +-+-+-+-+-+-+-+-+---+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * The machine-dependent SPL's are also included below (indented).
 * To change the interrupt priority of a particular device, you need
 * only change it's corresponding #define below.
 *
 * Notes:
 *	- software prohibits more than one machine-dependent SPL per bit on
 *	  a given architecture (e.g. hp700 or hp800).  In cases where there
 *	  are multiple equivalent devices which interrupt at the same level
 *	  (e.g. ASP RS232 #1 and #2), the interrupt table insertion routine
 *	  will always add in the unit number (arg0) to offset the entry.
 *	- hard clock must be the first bit (i.e. 0x80000000).
 *	- SPL7 is any non-zero value (since the PSW I-bit is off).
 *	- SPLIMP serves two purposes: blocks network interfaces and blocks
 *	  memory allocation via malloc.  In theory, SPLLAN would be high
 *	  enough.  However, on the 700, the SCSI driver uses malloc at
 *	  interrupt time requiring SPLIMP >= SPLBIO.  On the 800, we are
 *	  still using HP-UX drivers which make the assumption that
 *	  SPLIMP >= SPLCIO.  New drivers would address both problems.
 */
#define	SPLHIGH		0x00000007	/* any non-zero, non-INTPRI value */
#define	SPLCLOCK	INTPRI_00	/* hard clock */
#define	SPLPOWER	INTPRI_01	/* power failure (unused) */
#define	 SPLVIPER	 INTPRI_03		/* (hp700) Viper */
#define	SPLVM		INTPRI_04	/* TLB shootdown (unused) */
#define	SPLBIO		INTPRI_08	/* block I/O */
#define	 SPLWAX		 INTPRI_07		/* (hp700) WAX */
#define	 SPLASP		 INTPRI_08		/* (hp700) ASP */
#define	 SPLCIO		 INTPRI_08		/* (hp800) CIO */
#define	SPLIMP		INTPRI_08	/* network & malloc */
#define	 SPLBDMA	 INTPRI_09		/* GSC type B DMA board */
#define	 SPLADMA	 INTPRI_10		/* GSC type A DMA board */
#define	 SPLADIRECT	 INTPRI_11		/* GSC type A Direct Board */
#define	 SPLEISANMI	 INTPRI_13		/* (hp700 EISA NMI) */
#define	 SPLEISA	 INTPRI_14		/* (hp700 EISA) */
#define  SPLCIOHPIB	 INTPRI_14		/* (hp800) CIO HP-IB */
#define	 SPLLAN		 INTPRI_15		/* (hp700 LAN) */
#define	 SPLCIOLAN	 INTPRI_15		/* (hp800 CIO LAN) */
#define	 SPLSCSI	 INTPRI_16		/* (hp700 internal SCSI) */
#define  SPLFWSCSI       INTPRI_17              /* (hp700 internal FW SCSI) */
#define	SPLTTY		INTPRI_20	/* TTY */
#define	 SPLCIOMUX	 INTPRI_20		/* (hp800) CIO MUX */
#define	 SPLDCA		 INTPRI_20		/* (hp700) RS232 #1 */
#define	 SPLGRF		 INTPRI_23		/* (hp700/hp800) graphics */
#define	 SPLHIL		 INTPRI_24		/* (hp700/hp800) HIL */
#define	 SPLGKD		 INTPRI_25		/* (hp700/hp800) GKD */
#define	SPLNET		INTPRI_28	/* soft net */
#define	SPLSCLK		INTPRI_30	/* soft clock */
#define	SPL0		INTPRI_32	/* no interrupts masked */


#ifndef ASSEMBLER
/*
 * This is a generic interrupt switch table.  It may be used by various
 * interrupt systems.  For each interrupt, it holds a handler and an
 * EIEM mask (selected from SPL* or, more generally, INTPRI*).
 *
 * So that these tables can be easily found, please prefix them with
 * the label "itab_" (e.g. "itab_proc").
 */
#include <mach/hp_pa/thread_status.h>

typedef unsigned spl_t;

struct intrtab {
	void (*handler)(int); /* ptr to routine to call */
	unsigned int intpri;	/* INTPRI (SPL) with which to call it */
	int arg0, arg1;		/* 2 arguments to handler: arg0 is unit */
};

extern spl_t	(splhil)(void);
extern spl_t	(splasp)(void);
extern spl_t	(splwax)(void);
extern spl_t	(splgkd)(void);
extern spl_t	(spllan)(void);

#endif /* ASSEMBLER */

#endif	/* _HP700_SPL_H_ */
