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
 * Revision 2.7.2.2  92/05/26  17:38:51  jeffreyh
 *      Fix from dbg to not include AST_BLOCK in AST_PER_THREAD
 *      [92/05/04            jeffreyh]
 * 
 * Revision 2.7.2.1  92/04/08  15:44:44  jeffreyh
 *      Limit propagation of per thread AST's across context switches.
 *      (from David Golub).
 *      [92/04/05            dlb]
 * 
 * Revision 2.7  91/08/28  11:14:20  jsb
 *      Renamed AST_CLPORT to AST_NETIPC.
 *      [91/08/14  21:38:09  jsb]
 * 
 * Revision 2.6  91/06/06  17:06:48  jsb
 *      Added AST_CLPORT.
 *      [91/05/13  17:35:08  jsb]
 * 
 * Revision 2.5  91/05/14  16:39:58  mrt
 *      Correcting copyright
 * 
 * Revision 2.4  91/03/16  14:49:32  rpd
 *      Fixed dummy aston, astoff definitions.
 *      [91/02/12            rpd]
 *      Revised the AST interface, adding AST_NETWORK.
 *      Added volatile attribute to need_ast.
 *      [91/01/18            rpd]
 * 
 * Revision 2.3  91/02/05  17:25:38  mrt
 *      Changed to new Mach copyright
 *      [91/02/01  16:11:14  mrt]
 * 
 * Revision 2.2  90/06/02  14:53:34  rpd
 *      Merged with mainline.
 *      [90/03/26  22:02:55  rpd]
 * 
 * Revision 2.1  89/08/03  15:45:04  rwd
 * Created.
 * 
 *  6-Sep-88  David Golub (dbg) at Carnegie-Mellon University
 *      Adapted to MACH_KERNEL and VAX.
 *
 * 11-Aug-88  David Black (dlb) at Carnegie-Mellon University
 *      Created.  dbg gets equal credit for the design.
 *
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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

/*
 *      kern/ast.h: Definitions for Asynchronous System Traps.
 */

#ifndef _KERN_AST_H_
#define _KERN_AST_H_

/*
 *      A CPU takes an AST when it is about to return to user code.
 *      Instead of going back to user code, it calls ast_taken.
 *      Machine-dependent code is responsible for maintaining
 *      a set of reasons for an AST, and passing this set to ast_taken.
 */

#include <cpus.h>
#include <platforms.h>
#include <fast_idle.h>
#include <dipc.h>
#if	PARAGON860
#include <mcmsg.h>
#include <mcmsg_eng.h>
#endif	/* PARAGON860 */

#include <kern/assert.h>
#include <kern/cpu_number.h>
#include <kern/macro_help.h>
#include <kern/lock.h>
#include <kern/spl.h>
#include <machine/ast.h>

/*
 *      Bits for reasons
 */

#define AST_NONE	0x00
#define AST_HALT	0x01
#define AST_TERMINATE	0x02
#define AST_BLOCK       0x04
#define AST_NETWORK     0x08
#define AST_QUANTUM     0x10
#define AST_APC		0x20	/* migration APC hook */
#define	AST_URGENT	0x40
#if	PARAGON860
#if	MCMSG
#define AST_MCMSG       0x80
#endif	/* MCMSG */
#if	MCMSG_ENG
#define AST_RPCREQ	0x100
#define AST_RPCREPLY	0x200
#define AST_RPCDEPART	0x400
#define AST_RDMASEND	0x800
#define AST_RDMARECV	0x1000
#define	AST_RDMATXF	0x2000
#define	AST_RDMARXF	0x4000
#define	AST_SCAN_INPUT	0x8000
#endif	/* MCMSG_ENG */
#endif	/* PARAGON860 */

#if	DIPC
#define	AST_DIPC	0x10000
#endif	/* DIPC */

#define AST_SWAPOUT	0x20000

#define	AST_ALL		(~AST_NONE)

#define AST_SCHEDULING  (AST_HALT | AST_TERMINATE | AST_BLOCK | AST_SWAPOUT)
#define	AST_PREEMPT	(AST_BLOCK | AST_QUANTUM | AST_URGENT)

typedef unsigned int ast_t;

extern volatile ast_t need_ast[NCPUS];

#ifdef  MACHINE_AST
/*
 *      machine/ast.h is responsible for defining aston and astoff.
 */
#else   /* MACHINE_AST */

#define aston(mycpu)
#define astoff(mycpu)

#endif  /* MACHINE_AST */

/* Initialize module */
extern void	ast_init(void);

/* Handle ASTs */
#if	FAST_IDLE
/*
 *	Caller informs ast_taken of context in which
 *	it is invoked; under no circumstances may ast_taken
 *	block if the thread involved is idle.
 */
#define	NO_IDLE_THREAD		0
#define	FAKE_IDLE_THREAD	1
#define	REAL_IDLE_THREAD	2
#endif	/* FAST_IDLE */
extern void	ast_taken(
			boolean_t	preemption,
			ast_t		mask,
			spl_t		old_spl
#if	FAST_IDLE
			,int		thread_type
#endif	/* FAST_IDLE */
			);

/* Check for pending ASTs */
extern void    	ast_check(void);

/*
 * Per-thread ASTs are reset at context-switch time.
 */

#ifndef MACHINE_AST_PER_THREAD
#define MACHINE_AST_PER_THREAD  0
#endif

#define AST_PER_THREAD (MACHINE_AST_PER_THREAD|AST_HALT|AST_TERMINATE|AST_APC)

/*
 *      ast_needed, ast_on, ast_off, ast_context, and ast_propagate
 *      assume splsched.  mycpu is always cpu_number().  It is an
 *      argument in case cpu_number() is expensive.
 */

#define ast_needed(mycpu)               need_ast[mycpu]

#define ast_on(mycpu, reasons)					\
MACRO_BEGIN							\
	assert(mycpu == cpu_number());				\
	if ((need_ast[mycpu] |= (reasons)) != AST_NONE)		\
                { aston(mycpu);	}				\
MACRO_END

#define ast_off(mycpu, reasons)					\
MACRO_BEGIN							\
	assert(mycpu == cpu_number());				\
	if ((need_ast[mycpu] &= ~(reasons)) == AST_NONE)	\
		{ astoff(mycpu); }				\
MACRO_END

#define ast_propagate(act, mycpu)    ast_on((mycpu), (act)->ast)

#define ast_context(act, mycpu)						\
MACRO_BEGIN								\
	assert(mycpu == cpu_number());					\
	if ((need_ast[mycpu] =						\
	     (need_ast[mycpu] &~ AST_PER_THREAD) | (act)->ast)		\
					!= AST_NONE)			\
		{ aston(mycpu);	}					\
	else								\
		{ astoff(mycpu); }					\
MACRO_END

#define thread_ast_set(act, reason)          (act)->ast |= (reason)
#define thread_ast_clear(act, reason)        (act)->ast &= ~(reason)
#define thread_ast_clear_all(act)            (act)->ast = AST_NONE

/*
 *      NOTE: if thread is the current thread, thread_ast_set should
 *      be followed by ast_propagate().
 */

#endif  /* _KERN_AST_H_ */
