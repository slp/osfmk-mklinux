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
 * Copyright (c) 1990, 1994 University of Utah.
 * All rights reserved.  The Utah Software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	Utah $Hdr: grfreg.h 1.11 94/12/14$
 */

/* 300 bitmapped display hardware primary id */
#define GRFHWID		0x39

/* 300 internal bitmapped display address */
#define GRFIADDR	0x560000

/* 300 hardware secondary ids */
#define GID_GATORBOX	1
#define	GID_TOPCAT	2
#define GID_RENAISSANCE	4
#define GID_LRCATSEYE	5
#define GID_HRCCATSEYE	6
#define GID_HRMCATSEYE	7
#define GID_DAVINCI	8
#define GID_XXXCATSEYE	9
#define GID_XGENESIS   11
#define GID_TIGER      12
#define GID_YGENESIS   13
#define GID_HYPERION   14

/* 800 hardware secondary ids */
#define GID_FIREEYE	6

/* 700 does not need a secondary id, but we want it to fit the model */
#define GID_STI		69

typedef unsigned long	grftype;

struct	grfreg {
	grftype	gr_pad0,
		gr_id,		/* +0x01 */
		gr_pad1[0x3],
		gr_fbwidth_h,	/* +0x05 */
		gr_pad2,
		gr_fbwidth_l,	/* +0x07 */
		gr_pad3,
		gr_fbheight_h,	/* +0x09 */
		gr_pad4,
		gr_fbheight_l,	/* +0x0B */
		gr_pad5,
		gr_dwidth_h,	/* +0x0D */
		gr_pad6,
		gr_dwidth_l,	/* +0x0F */
		gr_pad7,
		gr_dheight_h,	/* +0x11 */
		gr_pad8,
		gr_dheight_l,	/* +0x13 */
		gr_pad9,
		gr_id2,		/* +0x15 */
		gr_pad10[0x47],
		gr_fbomsb,	/* +0x5d */
		gr_pad11,
		gr_fbolsb;	/* +0x5f */
};

/*
 * 700 specific stuff
 */
#include <hp_pa/HP700/grf_stireg.h>

/*
 * We need to keep track of the ROM address and the STI info.
 * Use this table, indexed by unit number.
 */
struct grfdev {
	caddr_t		   romaddr;		/* ROM address */
	caddr_t		   hpa;			/* device address */
	int		   type;		/* STI_TYPE_* */
	struct sti_entry   *ep;			/* STI routines */
	struct sti_region  regions[STI_REGIONS];/* Region list */
	struct sti_config  config;		/* "global" config info */
	char 		   devname[STI_NAMELEN];/* Name for GCDESCRIBE */
};



