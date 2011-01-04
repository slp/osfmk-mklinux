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
 * Copyright (c) 1987-1994, The University of Utah and
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
 *	Utah $Hdr: grfioctl.h 1.22 94/12/14$
 */

struct	grfinfo {
	int	gd_id;			/* HPUX identifier */
	caddr_t	gd_regaddr;		/* control registers physaddr */
	int	gd_regsize;		/* control registers size */
	caddr_t	gd_fbaddr;		/* frame buffer physaddr */
	int	gd_fbsize;		/* frame buffer size */
	short	gd_colors;		/* number of colors */
	short	gd_planes;		/* number of planes */
/* new stuff */
	int	gd_fbwidth;		/* frame buffer width */
	int	gd_fbheight;		/* frame buffer height */
	int	gd_dwidth;		/* displayed part width */
	int	gd_dheight;		/* displayed part height */
	int	gd_pad[6];		/* for future expansion */
};

/* types */
#define GRFGATOR	8
#define GRFBOBCAT	9
#define	GRFCATSEYE	9
#define GRFRBOX		10
#define GRFFIREEYE	11
#define GRFHYPERION	12
#define GRFDAVINCI	14

struct	grf_fbinfo {
	int	id;
	int	mapsize;
	int	dwidth, dlength;
	int	width, length;
	int	xlen;
	int	bpp, bppu;
	int	npl, nplbytes;
	char	name[32];
	int	attr;
	caddr_t	fbbase, regbase;
	caddr_t	regions[6];
};

#ifndef _IOH
#define _IOH(x,y)	_IO(x,y)

#define	GCID		_IOR('G', 0, int)
#define	GCON		_IOH('G', 1)
#define	GCOFF		_IOH('G', 2)
#define	GCAON		_IOH('G', 3)
#define	GCAOFF		_IOH('G', 4)
#define	GCMAP		_IOWR('G', 5, int)
#define	GCUNMAP		_IOWR('G', 6, int)
#define	GCMAP_HPUX	_IO('G', 5)
#define	GCUNMAP_HPUX	_IO('G', 6)
#define	GCLOCK		_IOH('G', 7)
#define	GCUNLOCK	_IOH('G', 8)
#define	GCLOCK_MINIMUM	_IOH('G', 9)
#define	GCUNLOCK_MINIMUM _IOH('G', 10)
#define	GCSTATIC_CMAP	_IOH('G', 11)
#define	GCVARIABLE_CMAP _IOH('G', 12)
#define GCDESCRIBE	_IOR('G', 21, struct grf_fbinfo)
#define GCFASTLOCK	_IOH('G', 26)

#endif

