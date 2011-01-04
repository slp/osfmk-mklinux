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
 *  (c) Copyright 1991 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */

/*
 * @(#)asm_em.h: $Revision: 1.1.9.2 $ $Date: 1995/11/02 14:46:12 $
 * $Locker:  $
 * 
 */

/*
 * Byte Offsets into Emulation Floating Point Coprocessor Register Area
 */
#define	FPU_FR0	0
#define	FPU_FR1	8
#define	FPU_FR2	16
#define	FPU_FR3	24
#define	FPU_FR4	32
#define	FPU_FR5	40
#define	FPU_FR6	48
#define	FPU_FR7	56
#define	FPU_FR8	64
#define	FPU_FR9	72
#define	FPU_FR10	80
#define	FPU_FR11	88
#define	FPU_FR12	96
#define	FPU_FR13	104
#define	FPU_FR14	112
#define	FPU_FR15	120
#define	FPU_FR16	128
#define	FPU_FR17	136
#define	FPU_FR18	144
#define	FPU_FR19	152
#define	FPU_FR20	160
#define	FPU_FR21	168
#define	FPU_FR22	176
#define	FPU_FR23	184
#define	FPU_FR24	192
#define	FPU_FR25	200
#define	FPU_FR26	208
#define	FPU_FR27	216
#define	FPU_FR28	224
#define	FPU_FR29	232
#define	FPU_FR30	240
#define	FPU_FR31	248
#define	FPU_FRZ	256

#define	FPU_SF1	FPU_FR12
#define	FPU_SF2	FPU_FR13
#define	FPU_SF3	FPU_FR14
#define	FPU_SF4	FPU_FR15
#define	FPU_SF5	FPU_FR16
#define	FPU_SF6	FPU_FR17
#define	FPU_SF7	FPU_FR18
#define	FPU_SF8	FPU_FR19
#define	FPU_SF9	FPU_FR20
#define	FPU_SF10 FPU_FR21

/*
 * Byte Offsets into Emulation Multiply/Divide SFU Register Area
 */
#define	MDSFU_HI	0
#define	MDSFU_LO	4
#define	MDSFU_OV	8

/*
 * Word Offsets into Emulation Multiply/Divide SFU Register Area
 */
#define	HI	0
#define	LO	1
#define	OV	2
