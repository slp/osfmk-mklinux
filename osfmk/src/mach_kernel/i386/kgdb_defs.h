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

#ifndef	_I386_KGDB_DEFS_H_
#define	_I386_KGDB_DEFS_H_

#include <mach/boolean.h>
#include <mach/i386/vm_types.h>
#include <i386/thread.h>
#include <i386/eflags.h>
#include <i386/trap.h>

extern kgdb_regs_t	kgdb_regs;
extern int		kgdb_active;

typedef unsigned char kgdb_bkpt_inst_t;
#define	BKPT_INST	0xcc		/* breakpoint instruction */
#define	BKPT_SET(inst)	(BKPT_INST)

/*
 * Exported interfaces
 */
extern boolean_t kgdb_read_bytes(
	vm_offset_t	addr,
	int		size,
	char		*data);

extern boolean_t kgdb_write_bytes(
	vm_offset_t	addr,
	int		size,
	char		*data);

extern int kgdb_trap(
	int			type,
	int			code,
	struct i386_saved_state	* regs);

extern void kgdb_kintr(void);

extern int dr6(void);

extern void kgdb_on(
	int			cpu);
    
extern void kgdb_comintr(
	int			unit);

#endif	/* _I386_KGDB_DEFS_H_ */

