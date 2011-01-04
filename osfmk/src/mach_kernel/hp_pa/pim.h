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
 * Copyright (c) 1990 The University of Utah and
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
 * 	Utah $Hdr: pim.h 1.2 91/09/25$
 *	Author: Jeff Forys, University of Utah CSS
 */

/*
 *	Processor Internal Memory structure.
 */

#ifndef	_PIM_
#define	_PIM_

struct mchk_type {	/* reason for machine check */
	unsigned int cache:	1,	/* cache system error */
                       tlb:	1,	/* TLB system error */
                       bus:	1,	/* bus transation error */
                    assist:	1,	/* coprocessor or SFU error */
                      proc:	1,	/* processor internal error */
                      resv:	26,	/* (reserved) */
                    advise:	1;	/* PDC advisory shutdown */
};

struct mchk {		/* High/Low Priority Machine Check information */
	struct mchk_type type;	/* general location of error */
	unsigned int	cpustate;	/* validity of processor state (GRs,CRs,etc) */
	unsigned int	detector;	/* type of instruction that detected error */
	unsigned int	cache;		/* breakdown of `type.cache' */
	unsigned int	tlb;		/* breakdown of `type.tlb' */
	unsigned int	bus;		/* breakdown of `type.bus' */
	unsigned int	assist;		/* breakdown of `type.assist' */
	unsigned int	proc;		/* breakdown of `type.proc' */
	unsigned int	assist_id;	/* 3-bit unit id of failing coproc or SFU */
	unsigned int	slave;		/* slave bus addr (iff `type.bus') */
	unsigned int	master;		/* master bus addr (HPA) (iff `type.bus') */
};

struct pim {		/* Processor Internal Memory */
	unsigned int	gr[32];		/* 32 general registers (0 - 31) */
	unsigned int	cr[32];		/* 32 control registers */
	unsigned int	sr[8];		/* 8 space registers */
	unsigned int	pcsqt;		/* pc space queue (tail) */
	unsigned int	pcoqt;		/* pc offset queue (tail) */
	struct mchk mchk;	/* HPMC or LPMC information */

	/* H-VERSION Dependent information follows */
};

#endif	/* _PIM_ */

