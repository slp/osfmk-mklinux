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
/* CMU_HIST */
/*
 * Revision 2.1.2.1  92/04/30  11:54:43  bernadat
 * 	Copied from main line
 * 	[92/04/21            bernadat]
 * 
 * Revision 2.2  92/04/04  11:36:43  rpd
 * 	Fix Intel Copyright as per B. Davies authorization.
 * 	[92/04/03            rvb]
 * 	Taken from 2.5 bootstrap.
 * 	[92/03/30            rvb]
 * 
 * Revision 2.2  91/04/02  14:42:22  mbj
 * 	Add Intel copyright
 * 	[90/02/09            rvb]
 * 
 */
/* CMU_ENDHIST */
/*
 * Mach Operating System
 * Copyright (c) 1992, 1991 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 */

/*
  Copyright 1988, 1989, 1990, 1991, 1992 
   by Intel Corporation, Santa Clara, California.

                All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*  Segment Descriptor
 *
 * 31          24         19   16                 7           0
 * ------------------------------------------------------------
 * |             | |B| |A|       | |   |1|0|E|W|A|            |
 * | BASE 31..24 |G|/|0|V| LIMIT |P|DPL|  TYPE   | BASE 23:16 |
 * |             | |D| |L| 19..16| |   |1|1|C|R|A|            |
 * ------------------------------------------------------------
 * |                             |                            |
 * |        BASE 15..0           |       LIMIT 15..0          |
 * |                             |                            |
 * ------------------------------------------------------------
 */

struct seg_desc {
	int	limit_15_0:16;
#define LIM16	0xffff
#define LIM32	0xfffff
	int	base_15_0:16;
#define BOOTBASE 0x1000
#define INITBASE 0x0
	int	base_23_16:8;
	int	type:4;
#define CODE	0xe		/* 0x8 + C + R */
#define DATA	0x2		/* W */
	int	mbo:1;
	int	dpl:2;
#define DPL0	0
	int	p:1;
	int	limit_19_16:4;
	int	avl:1;
	int	mbz:1;
	int	d:1;
#define SEG16	0
#define SEG32	1
	int	g:1;
#define	BYTES	0
#define PAGES	1
	int	base_31_24:8;
};
#define SEG_DESC_SIZE	8

#define DESC(base, limit, type, seg, gran)  \
	{ limit & 0xffff, base & 0xffff, (base >> 16) & 0xff, \
	  type, 1, DPL0, 1, (limit >> 16) & 0x0f, 0, 0, seg, gran, \
	  (base >> 24) & 0xff}

#define NGDTENT		6

const struct seg_desc Gdt[NGDTENT] = {
	DESC(0, 0, 0, 0, 0),			/* 0x0 : null */
	DESC(BOOTBASE, LIM32, CODE, SEG32, PAGES), /* 0x8 : boot code */
	DESC(BOOTBASE, LIM32, DATA, SEG32, PAGES), /* 0x10 : boot data */
	DESC(BOOTBASE, LIM16, CODE, SEG16, BYTES), /* 0x18 : boot code 16 */
	DESC(INITBASE, LIM32, DATA, SEG32, PAGES), /* 0x20 : init data */
	DESC(INITBASE, LIM32, CODE, SEG32, PAGES)  /* 0x28 : init code */
};

struct pseudo_desc {
	int	limit:16;
	int	base_low:16;
	int	base_high:16;
};

/*
 * boot is loaded at 4k, Gdt is located at 4k+512
 */
#define GDTLIMIT	(NGDTENT* SEG_DESC_SIZE)
#define SEC_SIZE	512
#define GDTBASE		(BOOTBASE+SEC_SIZE)

const struct pseudo_desc Gdtr = {
	GDTLIMIT, GDTBASE & 0xffff, (GDTBASE >> 16) & 0xffff
};
