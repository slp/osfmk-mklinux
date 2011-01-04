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
 * Copyright (c) 1988-1992, The University of Utah and
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
 *	Utah $Hdr: cons_conf.c 1.5 92/05/27$
 */

/*
 * This entire table could be autoconfig()ed but that would mean that
 * the kernel's idea of the console would be out of sync with that of
 * the standalone boot.  I think it best that they both use the same
 * known algorithm unless we see a pressing need otherwise.
 */
#include <kgdb.h>

#include <types.h>
#include <mach/boolean.h>
#include <device/device_types.h>
#include <hp_pa/HP700/cons.h>

#include "ite.h"
#include "dca.h"

#if NITE > 0
extern void itecnprobe(struct consdev *cp);
extern void itecninit(struct consdev *cp);
extern int itecngetc(dev_t dev, boolean_t);
extern void itecnputc(dev_t dev, char c);
#endif
#if NDCA > 0
extern void dcacnprobe(struct consdev *cp);
extern void dcacninit(struct consdev *cp);
extern int dcacngetc(dev_t dev, boolean_t);
extern void dcacnputc(dev_t dev, char c);
#endif

struct	consdev constab[] = {
#if NITE > 0
	{
		"ite",
		itecnprobe,	itecninit,	itecngetc,	itecnputc
	},
#else
#ifdef KGDB
	{
		"com",
		dcacnprobe,	dcacninit,	dcacngetc,	dcacnputc
	},
#endif
#endif
#if NDCA > 0
	{
		"com",
		dcacnprobe,	dcacninit,	dcacngetc,	dcacnputc
	},
#endif
	{ 0 },
};
