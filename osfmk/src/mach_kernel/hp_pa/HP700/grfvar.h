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
 *	Utah $Hdr: grfvar.h 1.16 94/12/14$
 */

/* internal structure of lock page */
#define GRFMAXLCK	2046
struct	grf_lockpage {
	short	gl_lockslot;
	u_char	gl_locks[GRFMAXLCK];
};

/* per display info */
struct	grf_softc {
	int	g_flags;		/* software flags */
	struct  grfsw *g_sw;		/* static configuration info */
	caddr_t	g_regkva;		/* KVA of registers */
	caddr_t	g_fbkva;		/* KVA of framebuffer */
	struct	grfinfo g_display;	/* hardware description (for ioctl) */
	struct	grf_lockpage *g_lock;	/* lock page associated with device */
	struct	proc *g_lockp;		/* process holding lock */
	short	*g_pid;			/* array of pids with device open */
	int	g_lockpslot;		/* g_pid entry of g_lockp */
	caddr_t	g_data;			/* device dependent data */
};

/*
 * Static configuration info for display types
 */
struct	grfsw {
	int	gd_hwid;	/* id returned by hardware */
	int	gd_swid;	/* id to be returned by software */
	char	*gd_desc;	/* description printed at config time */
	int	(*gd_init)(struct grf_softc*);	   /* boot time init routine */
	int	(*gd_mode)(struct grf_softc*, int, char*); /* misc function routine */
};

/* flags */
#define	GF_ALIVE	0x01
#define GF_OPEN		0x02
#define GF_EXCLUDE	0x04
#define GF_WANTED	0x08

/* requests to mode routine */
#define GM_GRFON	1
#define GM_GRFOFF	2
#define GM_GRFOVON	3
#define GM_GRFOVOFF	4
#define GM_DESCRIBE	5
#define GM_MAP		6
#define GM_UNMAP	7

/* minor device interpretation */
#define GRFOVDEV	0x10	/* overlay planes */
#define GRFIMDEV	0x20	/* images planes */
#define GRFUNIT(d)	((d) & 0x7)

#ifdef MACH_KERNEL
extern	struct grf_softc grf_softc[];
extern	struct grfsw grfsw[];
extern	int ngrfsw;
#endif

extern void grfconfig(void);
extern int sti_init(struct grf_softc *);
extern int sti_mode(struct grf_softc *, int, caddr_t);
