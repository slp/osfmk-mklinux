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
 * Revision 2.5  91/05/14  16:18:26  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:15:33  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:38:51  mrt]
 * 
 * Revision 2.3  90/10/25  14:45:03  rwd
 * 	Added watchpoint support.
 * 	[90/10/18            rpd]
 * 
 * Revision 2.2  90/05/03  15:38:10  dbg
 * 	Created.
 * 	[90/02/08            dbg]
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

#ifndef	_PPC_TRAP_H_
#define	_PPC_TRAP_H_

/* maximum number of arguments to a syscall trap */
#define NARGS	12
/* Size to reserve in frame for arguments - first 8 are in registers */
#define ARG_SIZE ((NARGS-8)*4) /* TODO NMGS check space not needed for regs*/


/*
 * Hardware exception vectors for powerpc are in exception.h
 */

#ifndef	ASSEMBLER

#include <mach/thread_status.h>
#include <mach/boolean.h>
#include <ppc/thread.h>

extern void			doexception(int exc, int code, int sub);

extern void			thread_exception_return(void);

extern boolean_t 		alignment(unsigned long dsisr,
					  unsigned long dar,
					  struct ppc_saved_state *ssp);

extern struct ppc_saved_state*	trap(int trapno,
				     struct ppc_saved_state *ss,
				     unsigned int dsisr,
				     unsigned int dar);

extern struct ppc_saved_state* interrupt(int intno,
					 struct ppc_saved_state *ss,
					 unsigned int dsisr,
					 unsigned int dar);

extern void create_fake_interrupt(void);

extern int			syscall_error(int exception,
					      int code,
					      int subcode,
					      struct ppc_saved_state *ss);


extern int			procitab(unsigned, void (*)(int), int);

#endif	/* ASSEMBLER */

#endif	/* _PPC_TRAP_H_ */
