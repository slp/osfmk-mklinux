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
 * 
 */
/*
 * MkLinux
 */
/* 
 * Copyright (c) 1990 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: conf.c 1.2 94/12/16$
 */

#include "stand.h"

extern int	nullsys(), nodev(), noioctl();

#define	RD		"rd"
int	rdstrategy(), rdopen();
int	rddevix = 2;

#define	FL		"fl"
int	flstrategy(), flopen();
int	fldevix = 4;

#define	SD		"sd"
int	sdstrategy(), sdopen();
int	sddevix = 5;

#define	CT		"ct"
int	ctstrategy(), ctopen(), ctclose();
int	ctdevix = 0;

struct devsw devsw[] = {
	{ CT,	ctstrategy,	ctopen,	ctclose, noioctl },  /*  0 = ct */
	{ 0,	nodev,		nodev,	nodev,   noioctl },  /*  1 = nodev */
	{ RD,	rdstrategy,	rdopen,	nullsys, noioctl },  /*  2 = rd */
	{ 0,	nodev,		nodev,	nodev,   noioctl },  /*  3 = sw */
	{ FL,	flstrategy,	flopen,	nullsys, noioctl },  /*  4 = fl */
	{ SD,	sdstrategy,	sdopen,	nullsys, noioctl },  /*  5 = sd */
};

int	ndevs = (sizeof(devsw)/sizeof(devsw[0]));

