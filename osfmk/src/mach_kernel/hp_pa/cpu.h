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
 * Copyright (c) 1988, 1992, 1994, The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: cpu.h 1.14 94/12/15$
 */

#ifndef _HP700_CPU_H_
#define _HP700_CPU_H_

/*
 * CPU types
 */


#define CPU_M808	0x103   /* Restricted 815. Use SW_CAP */
#define CPU_M810	0x100
#define CPU_M815	0x103
#define CPU_M820	0x101
#define CPU_M825	0x008
#define CPU_M827	0x102
#define CPU_M832	0x102
#define CPU_M834	0x00A	/* 835 2 user WS */
#define CPU_M835	0x00A
#define CPU_M836	0x00A	/* 835 2 user Server */
#define CPU_M840	0x004
#define CPU_M844	0x00B	/* 845 2 user WS */
#define CPU_M845	0x00B
#define CPU_M846	0x00B	/* 845 2 user Server */
#define CPU_M850	0x080
#define CPU_M850_OPDC	0x00C	/* Old number for 850 */
#define CPU_M855	0x081

#define	CPU_M705	0x302
#define	CPU_M710	0x300

#define	CPU_M712_60	0x600	
#define	CPU_M712_80	0x601	
#define	CPU_M712_3	0x602	
#define	CPU_M712_4	0x605	

#define	CPU_M715_33	0x311	
#define	CPU_M715_50	0x310	
#define	CPU_M715T_50	0x312	
#define	CPU_M715S_50	0x314	
#define	CPU_M715_75	0x316	

#define	CPU_M715_64	0x60A
#define	CPU_M715_100	0x60B	

#define	CPU_M720	0x200

#define	CPU_M725_100	0x60D

#define	CPU_M725_50	0x318
#define	CPU_M725_75	0x319

#define	CPU_M730	0x202
#define	CPU_M750	0x201

#define	CPU_M770_100	0x585	/* J-class J200 */
#define	CPU_M770_120	0x586	/* J-class J210 */
#define	CPU_M777_100	0x592	/* C-class C200 */
#define	CPU_M777_120	0x58E	/* C-class C210 */

#define CPU_UNRESTRICTED 0      /* unrestricted */
#define CPU_SERVER       1      /* 2 user server */     
#define CPU_WS           2      /* 2 user workstation */
#define CPU_RESTRICTED   1      /* general limited I/O slots */

#define ASP_PROM       0xF0810000

/*
 * Expected (and optimized for) cache line size (in bytes).
 */
#define CACHE_LINE_SIZE	32

#define CPU_PCXS 1
#define CPU_PCXT 2
#define CPU_PCXL 3

#endif /* _HP700_CPU_H_ */
