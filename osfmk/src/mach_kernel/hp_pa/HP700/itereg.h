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
 * Copyright (c) 1988-1994, The University of Utah and
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
 *	Utah $Hdr: itereg.h 1.7 94/12/14$
 */

/*
 * Offsets into the display ROM that is part of the first 4K of each
 * display device.
 */
#define FONTROM		0x3B	/* Offset of font information structure. */
#define FONTADDR	0x4	/* Offset from FONTROM to font address. */
#define FONTHEIGHT	0x0	/* Offset from font address to font height. */
#define FONTWIDTH	0x2	/* Offset from font address to font width. */
#define FONTDATA	0xA	/* Offset from font address to font glyphs. */

#ifdef hp300
#define FBBASE		((volatile u_char *)ip->fbbase)
#else
#define FBBASE		((volatile u_long *)ip->fbbase)

#ifdef MACH_KERNEL
#define ITEDECL
#define ITEACCESS
#define ITEUNACCESS
#else
/*
 * XXX wretched hack to deal with the ioblk_* stuff.
 * When ioblk_* goes away, this should too.
 */
#include <machine/psl.h>
#define ITEDECL		int _om_
#define ITEACCESS	_om_ = rsm(PSW_P)
#define ITEUNACCESS	if (_om_ & PSW_P) ssmv(_om_)
#endif
#endif
