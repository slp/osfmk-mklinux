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
 * This file is not ready for general use.  It is here
 * to see how far a compile will go and to help figure
 * out just what needs to be changed
*/

/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * Copyright (c) 1991,1992 The University of Utah and
 * the Center for Software Science (CSS).
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON, THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION, AND DISCLAIM ANY LIABILITY
 * OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF
 * THIS SOFTWARE.
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
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: db_machdep.h 1.3 92/06/06$
 */
/*
 *	File: db_machdep.h
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	8/90
 *
 *	Machine-dependent defines for new kernel debugger.
 */

#ifndef	_HP700_DB_MACHDEP_H_
#define	_HP700_DB_MACHDEP_H_

#include <mach/machine/vm_types.h>
#include <mach/machine/vm_param.h>
#include <machine/thread.h>		/* for thread_status */
#include <machine/break.h>                /* for break point information */
#include <machine/cpu.h>
#include <mach/boolean.h>
#include <machine/spl.h>
#include <machine/opcode_is_something.h>

typedef	vm_offset_t	db_addr_t;	/* address - unsigned */
typedef	int		db_expr_t;	/* expression - signed */

typedef struct hp700_saved_state db_regs_t;
db_regs_t	*db_cur_exc_frame;	/* register state */
#define	DDB_REGS	db_cur_exc_frame

/* 
 * Does pc mean program counter here?  If so what is the program
 * counter for the hppa?  Is it the rp? 
 *
 * Setting to be IIOQ.(Shouldn't this be IAOQ?)  The instruction address 
 * offset queue.  Not sure if this is the way to do it because of the 2
 * instruction queues.
 *
 */

#define	PC_REGS(regs)	((db_addr_t)(regs)->iioq_head)

/*------------------------------------------------------------------------*/
/*
 * BKPT_INST has zeros in bits 0-5 and 19-26.  The other bits can be used
 * as parameters to the break point code.
 */

/* breakpoint instruction */
#define	BKPT_INST	((BREAK_MACH_DEBUGGER) << 13)
#define	BKPT_SIZE	(4)		/* size of breakpoint inst */
#define	BKPT_SET(inst)	((inst) & 0x03ffe01f) /* zero out bits 0-5 and 19-26 */

/* This is wrong. */
#define BREAK_MASK 0xffffffff

#define	IS_BREAKPOINT_TRAP(type, code)	(((type) & BREAK_MASK) == 0)
#define	IS_WATCHPOINT_TRAP(type, code)	((code) == 2)

#define	SOFTWARE_SSTEP			1	/* no hardware support */

#define	inst_trap_return(ins)		OPCODE_IS_INTERRUPT_RETURN(ins)
#define	inst_return(ins)		OPCODE_IS_RETURN(ins)
#define	inst_call(ins)			OPCODE_IS_CALL(ins)
#define inst_branch(ins)		OPCODE_IS_BRANCH(ins)
#define inst_load(ins)			OPCODE_IS_LOAD(ins)
#define	inst_store(ins)			OPCODE_IS_STORE(ins)
#define inst_unconditional_flow_transfer(ins) \
		                        OPCODE_IS_UNCONDITIONAL_BRANCH(ins)
/* This is probably wrong */
#define next_instr_address(v,b,task)    ((unsigned)(v)+4)

#define	isa_kbreak(ins)			((ins) == BKPT_INST)


/* access capability and access macros */

#define	DB_ACCESS_LEVEL		2	/* access any space */

#define DB_CHECK_ACCESS(addr,size,task)				\
	db_check_access(addr,size,task)
#define DB_PHYS_EQ(task1,addr1,task2,addr2)			\
	db_phys_eq(task1,addr1,task2,addr2)

#define DB_VALID_KERN_ADDR(addr)                                \
        ((addr) >= VM_MIN_KERNEL_ADDRESS && (addr) < VM_MAX_KERNEL_ADDRESS)
#define DB_VALID_ADDRESS(addr,user)	                        \
        ((user != 0) ^ DB_VALID_KERN_ADDR(addr))                        

boolean_t	db_check_access(vm_offset_t, int, task_t);
boolean_t	db_phys_eq(task_t, vm_offset_t, task_t, vm_offset_t);

/* macros for printing OS server dependent task name */

#define DB_TASK_NAME(task)		db_task_name(task)
#define DB_TASK_NAME_TITLE		"COMMAND                "
#define DB_TASK_NAME_LEN		23
#define DB_NULL_TASK_NAME		"?                      "

/* debugger screen size definitions */

#define	DB_MAX_LINE	50	/* max output lines */
#define DB_MAX_WIDTH	132	/* max output colunms */

void		db_task_name(task_t);

/* definitions of object format */

#define	DDB_COFF_SYMTAB			0

/* Define DB_NO_AOUT if you want to use your own symbol table routines. */
/*#define	DB_NO_AOUT			1*/

#define kdbsplhigh  splhigh
#define kdbsplx     splx

#endif	/* _HP700_DB_MACHDEP_H_ */


