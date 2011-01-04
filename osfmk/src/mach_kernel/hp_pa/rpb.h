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
 * Copyright (c) 1990-1992, The University of Utah and
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
 * 	Utah $Hdr: rpb.h 1.4 92/06/27$
 */

/*
 * On the hppa, the `restart parameter block' looks like
 * Processor Internal Memory.  It runs after a crash and takes
 * a dump.  `rp_sp' and `rp_flag' are here for HP-UX compatibility.
 */

#ifndef	ASSEMBLER
#ifdef MACH_KERNEL
#include "pim.h"
#else
#include <machine/pim.h>
#endif

struct rpb {
	unsigned int	rp_sp;		/* stack pointer */
	unsigned int	rp_flag;	/* set when doadump runs (see below) */
	struct pim rp_pim;	/* Processor Internal Memory */
};
#endif	/* !LOCORE */

/* rp_flags */
#define	RPB_DUMP	1	/* call to doadump() */
#define	RPB_B_DUMP	31		/* bit position for RPB_DUMP */
#define	RPB_CRASH	2	/* call to crashdump/doadump() */
#define	RPB_B_CRASH	30		/* bit position for RPB_CRASH */

#define	rp_gr0	rp_pim.gr[0]	/* start of general registers */
#define	rp_gr1	rp_pim.gr[1]	/* gr0 is useless, start at gr1 */
#define	rp_sr0	rp_pim.sr[0]	/* start of space registers */
#define	rp_cr0	rp_pim.cr[0]	/* start of control registers */
#define	rp_cr8	rp_pim.cr[8]	/* cr0-7 is useless, start at cr8 */

#if defined(MACH_KERNEL) && !defined(ASSEMBLER)
extern	struct rpb rpb;
#endif
