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

#ifndef	_PPC_KGDB_DEFS_H_
#define	_PPC_KGDB_DEFS_H_

#include <mach_kgdb.h>
#include <mach_kdb.h>

#include <mach/boolean.h>
#include <mach/machine/vm_types.h>
#include <mach/machine/thread_status.h>
#include <ppc/thread.h>
#include <ppc/proc_reg.h>
#include <ppc/POWERMAC/serial_io.h> /* for kgdb_putc */

#if	MACH_KGDB

#include <ppc/gdb_defs.h>

extern kgdb_regs_t	kgdb_regs;
extern int		kgdb_active;

typedef unsigned char kgdb_signal;

typedef unsigned int kgdb_bkpt_inst_t;

/* WARNING - this must tie in with gdb/config/rs6000/tm-rs6000.h */

#define	BKPT_INST	0x7d821008	/* breakpoint instruction */
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
        struct ppc_saved_state *regs);

extern void kgdb_kintr(void);

extern int dr6(void);

extern void kgdb_on(
	int			cpu);
    
extern void kgdb_comintr(
	int			unit);

extern boolean_t kgdb_user_breakpoint(struct ppc_saved_state *ssp);

#define kgdb_break()	__asm__ volatile("twge  2,2")

#else	/* MACH_KGDB */
#if	MACH_KDB

#undef kgdb_printf
#define kgdb_printf printf
#endif	/* MACH_KDB */

#endif	/* MACH_KGDB */

#endif	/* _PPC_KGDB_DEFS_H_ */

