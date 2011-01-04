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
/* CMU_HIST */
/*
 * Revision 2.3  91/05/14  16:10:04  mrt
 * 	Correcting copyright
 * 
 * Revision 2.2  91/05/08  12:38:46  dbg
 * 	Created.
 * 	[91/03/21            dbg]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 */

#ifndef	_I386_IOPB_H_
#define	_I386_IOPB_H_

#include <i386/tss.h>
#include <kern/queue.h>

/*
 * IO permission bitmap.
 *
 * Allows only IO ports 0 .. 0xffff: for ISA machines.
 */

#define	iopb_howmany(a,b)	(((a)+(b)-1)/(b))

#define	IOPB_MAX	0xffff		/* x86 allows ports 0..ffff */
#define	IOPB_BYTES	(iopb_howmany(IOPB_MAX+1,8))

typedef	unsigned char	isa_iopb[IOPB_BYTES];

/*
 * An IO permission map is a task segment with an IO permission bitmap.
 */

struct iopb_tss {
	struct i386_tss	tss;		/* task state segment */
	isa_iopb	bitmap;		/* bitmap of mapped IO ports */
	unsigned int	barrier;	/* bitmap barrier for CPU slop */
	queue_head_t	io_port_list;	/* list of mapped IO ports */
	int		iopb_desc[2];	/* descriptor for this TSS */
};

typedef	struct iopb_tss	*iopb_tss_t;

#endif	/* _I386_IOPB_H_ */
