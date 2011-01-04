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
/* This file was formerly i386/act.h; history below:
 *
 * Revision 1.1.2.3  1994/02/09  21:48:30  condict
 * 	Merge changes.
 * 	[1994/02/09  17:31:03  condict]
 *
 * 	Move struct i386_saved_state to mach/i386/thread_status.h, to export
 * 	it to servers.  Also, add act_machine_state_ptr, and state_count
 * 	array that maps state flavor to size.
 * 	[1994/02/09  02:59:43  condict]
 *
 * Revision 1.1.2.2  1994/02/09  00:42:53  dwm
 * 	Remove various unused future hooks to conserve wired memory.
 * 	[1994/02/09  00:35:20  dwm]
 * 
 * Revision 1.1.2.1  1994/01/12  17:51:34  dwm
 * 	Coloc: initial restructuring to follow Utah model.
 * 	machine-dependent act; moved stuff from machine/thread.h
 * 	[1994/01/12  17:14:05  dwm]
 * 
 */
#ifndef	_I386_THREAD_ACT_H_
#define	_I386_THREAD_ACT_H_

#include <mach/boolean.h>
#include <mach/i386/vm_types.h>
#include <mach/i386/fp_reg.h>
#include <mach/thread_status.h>

#include <kern/lock.h>

#include <i386/iopb.h>
#include <i386/tss.h>
#include <i386/eflags.h>

/*
 *	i386_saved_state:
 *
 *	Has been exported to servers.  See: mach/i386/thread_status.h
 *
 *	This structure corresponds to the state of user registers
 *	as saved upon kernel entry.  It lives in the pcb.
 *	It is also pushed onto the stack for exceptions in the kernel.
 *	For performance, it is also used directly in syscall exceptions
 *	if the server has requested i386_THREAD_STATE flavor for the exception
 *	port.
 *
 *	We define the following as an alias for the "esp" field of the
 *	structure, because we actually save cr2 here, not the kernel esp.
 */
#define cr2	esp

/*
 *	Save area for user floating-point state.
 *	Allocated only when necessary.
 */

struct i386_fpsave_state {
	boolean_t		fp_valid;
	struct i386_fp_save	fp_save_state;
	struct i386_fp_regs	fp_regs;
};

/*
 *	v86_assist_state:
 *
 *	This structure provides data to simulate 8086 mode
 *	interrupts.  It lives in the pcb.
 */

struct v86_assist_state {
	vm_offset_t		int_table;
	unsigned short		int_count;
	unsigned short		flags;	/* 8086 flag bits */
};
#define	V86_IF_PENDING		0x8000	/* unused bit */

/*
 *	i386_interrupt_state:
 *
 *	This structure describes the set of registers that must
 *	be pushed on the current ring-0 stack by an interrupt before
 *	we can switch to the interrupt stack.
 */

struct i386_interrupt_state {
	int	es;
	int	ds;
	int	edx;
	int	ecx;
	int	eax;
	int	eip;
	int	cs;
	int	efl;
};

/*
 *	i386_kernel_state:
 *
 *	This structure corresponds to the state of kernel registers
 *	as saved in a context-switch.  It lives at the base of the stack.
 */

struct i386_kernel_state {
	int			k_ebx;	/* kernel context */
	int			k_esp;
	int			k_ebp;
	int			k_edi;
	int			k_esi;
	int			k_eip;
};

/*
 *	i386_machine_state:
 *
 *	This structure corresponds to special machine state.
 *	It lives in the pcb.  It is not saved by default.
 */

struct i386_machine_state {
	iopb_tss_t		io_tss;
	struct user_ldt	*	ldt;
	struct i386_fpsave_state *ifps;
	struct v86_assist_state	v86s;
};

typedef struct pcb {
	struct i386_interrupt_state iis[2];	/* interrupt and NMI */
	struct i386_saved_state iss;
	struct i386_machine_state ims;
	decl_simple_lock_data(,lock)
} *pcb_t;

/*
 * Maps state flavor to number of words in the state:
 */
extern unsigned int state_count[];


#define USER_REGS(ThrAct)	(&(ThrAct)->mact.pcb->iss)

#define act_machine_state_ptr(ThrAct)	(thread_state_t)USER_REGS(ThrAct)


#define	is_user_thread(ThrAct)	\
  	((USER_REGS(ThrAct)->efl & EFL_VM) \
	 || ((USER_REGS(ThrAct)->cs & 0x03) != 0))

#define	user_pc(ThrAct)		(USER_REGS(ThrAct)->eip)
#define	user_sp(ThrAct)		(USER_REGS(ThrAct)->uesp)

#define syscall_emulation_sync(task)	/* do nothing */

typedef struct MachineThrAct {
	/*
	 * pointer to process control block
	 *	(actual storage may as well be here, too)
	 */
	struct pcb xxx_pcb;
	pcb_t pcb;

} MachineThrAct, *MachineThrAct_t;

/*
 * On i386, user stacks of collocated servers are wired.
 * Unwire these user stacks when their threads are swapped out.
 */
#define THREAD_SWAP_UNWIRE_USER_STACK	TRUE

#endif	/* _I386_THREAD_ACT_H_ */
