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
 * Copyright (c) 1990, 1991 The University of Utah and
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
 * 	Utah $Hdr: machparam.h 1.7 92/05/22$
 */

#ifndef _PPC_MACHPARAM_H_
#define _PPC_MACHPARAM_H_

/*
 * Machine dependent constants for ppc. 
 * Added as needed (for device drivers).
 */
#define	NBPG	4096		/* bytes/page */
#define	PGSHIFT	12		/* LOG2(NBPG) */

#define DEV_BSHIFT      10               /* log2(DEV_BSIZE) */

/*
 * Disk devices do all IO in 1024-byte blocks.
 */
#define	DEV_BSIZE	1024

#define	btop(x)	((x)>>PGSHIFT)
#define	ptob(x)	((x)<<PGSHIFT)

/* Clicks to disk blocks */
#define ctod(x) ((x)<<(PGSHIFT-DEV_BSHIFT))

/* Disk blocks to clicks */
#define       dtoc(x) ((x)>>(PGSHIFT-DEV_BSHIFT))

/* clicks to bytes */
#define       ctob(x) ((x)<<PGSHIFT)

/* bytes to clicks */
#define       btoc(x) (((unsigned)(x)+(NBPG-1))>>PGSHIFT)

#endif /* _PPC_MACHPARAM_H_ */
