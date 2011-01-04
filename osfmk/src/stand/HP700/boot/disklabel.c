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
 * Copyright (c) 1990 mt Xinu, Inc.  All rights reserved.
 * Copyright (c) 1990 University of Utah.  All rights reserved.
 *
 * This file may be freely distributed in any form as long as
 * this copyright notice is included.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	Utah $Hdr: disklabel.c 1.2 94/12/16$
 */

#include "stand.h"
#include "saio.h"
#include <sys/param.h>
#include <ufs/fs.h>
#include <sys/disklabel.h>

readdisklabel(io, strat, lp)
	struct iob *io;
	int (*strat)();
	register struct disklabel *lp;
{
	static char labelbuf[DEV_BSIZE];
	register struct iob *iop;
	register struct disklabel *llp;
	struct iob loc_io;

	/*
	 * Set up i/o buffer and read the label
	 */
	iop = &loc_io;
	*iop = *io;
	iop->i_part = 2;	/* `C' partition */
	iop->i_boff = 0;
	iop->i_cyloff = 0;
	iop->i_offset = 0;
	iop->i_bn = LABELSECTOR;
	iop->i_ma = labelbuf;
	iop->i_cc = DEV_BSIZE;
	if ((*strat)(iop, F_READ) != DEV_BSIZE) 
		return (ERDLAB);
	llp = (struct disklabel *)(iop->i_ma + LABELOFFSET);
	if (llp->d_magic == DISKMAGIC && llp->d_magic2 == DISKMAGIC &&
	    llp->d_npartitions != 0)
		*lp = *llp;

	return (0);
}

