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

/* INTEL_HIST */
/*
 * Revision 1.11  1994/07/18  21:12:29  regnier
 * Add VCF message passing protocol. No VCF code is compiled in unless
 * the '+vcf' option is used to build the kernel.
 *
 *  Reviewer: wms
 *  Risk: low
 *  Benefit or PTS #:
 *  Testing: EATS, developer
 *  Module(s): vm/vm_pageout.c  kern/syscall_sw.c   conf/MASTER.i860
 *             conf/files.i860  i860paragon/mcmsg/mcmsg_config.h
 *             i860paragon/mcmsg/mcmsg_config_vcf.h
 *             i860paragon/msgp/msgp_mp.c
 *             i860paragon/vcf/vcf*
 *
 * Revision 1.10  1994/07/12  19:23:40  andyp
 * Merge of the NORMA2 branch back to the mainline.
 *
 * Revision 1.9  1994/05/18  00:07:35  regnier
 *  Reviewer: None.
 *  Risk: Low
 *  Benefit: Hook for large packets.
 *  Testing: Message Passing Eats, developer tests.
 *  Module(s): syscall_subr.h, syscall_sw.c, i860_paragon/mcmsg/mcmsg_init.c
 *
 * Revision 1.8.4.3  1994/03/02  18:43:36  andyp
 * Added additional user-mode bindings for RDMA testing.
 *
 * Revision 1.8.4.2  1994/02/07  20:19:31  andyp
 * Rounded out RDMA interfaces and added user-mode bindings for testing.
 *
 * Revision 1.8.4.1  1994/02/04  07:53:29  andyp
 * Added machine-specific RPC and RDMA intialization calls to
 * startup.c (see i860paragon/model_dep.c).  Added temporary
 * user-mode test points for RPC.
 *
 * Revision 1.8  1993/09/29  23:08:08  prp
 * Merge in R1.2 branch message passing.
 *
 * Revision 1.2.2.5.2.1  1993/08/26  01:05:43  prp
 * Add VC message passing module
 */
/* INTEL_ENDHIST */

/* CMU_HIST */
/*
 * Revision 2.11.2.3  92/05/27  00:46:14  jeffreyh
 * 	Added system call decls within MCMSG ifdefs
 * 	[regnier@ssd.intel.com]
 * 
 * Revision 2.11.2.2  92/03/28  10:10:37  jeffreyh
 * 	Pick up changes from MK71
 * 	[92/03/20  13:17:52  jeffreyh]
 * 
 * Revision 2.13  92/02/19  16:06:58  elf
 * 	Added syscall_thread_depress_abort.
 * 	[92/01/20            rwd]
 * 
 * Revision 2.12  92/01/03  20:40:19  dbg
 * 	Use continuations always in evc_wait.
 * 	[91/12/27            af]
 * 
 * Revision 2.11  91/12/13  14:54:49  jsb
 * 	Added evc_wait().
 * 	[91/12/12  17:42:00  af]
 * 
 * Revision 2.10  91/05/14  16:47:45  mrt
 * 	Correcting copyright
 * 
 * Revision 2.9  91/03/16  14:52:12  rpd
 * 	Changed swtch, swtch_pri, and thread_switch to MACH_TRAP_STACK.
 * 	[91/01/17            rpd]
 * 
 * Revision 2.8  91/02/05  17:29:44  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:18:37  mrt]
 * 
 * Revision 2.7  91/01/08  15:17:21  rpd
 * 	Changed to use MACH_TRAP_STACK appropriately.
 * 	[90/12/27  21:20:57  rpd]
 * 
 * Revision 2.6  90/09/09  14:32:58  rpd
 * 	Added mach_port_allocate_name
 * 	[90/09/05            rwd]
 * 
 * Revision 2.5  90/06/19  22:59:30  rpd
 * 	Added mach_port_allocate, mach_port_deallocate, mach_port_insert_right.
 * 	[90/06/02            rpd]
 * 
 * Revision 2.4  90/06/02  14:56:26  rpd
 * 	Updated for new IPC and scheduling technology.
 * 	[90/03/26  22:20:34  rpd]
 * 
 * Revision 2.3  90/05/29  18:36:48  rwd
 * 	New traps from rfr (vm_map, task_create etc.)
 * 	[90/04/20            rwd]
 * 
 * Revision 2.2  89/10/16  15:22:13  rwd
 * 	Added debug option for kern_invalid.
 * 	[89/09/29            rwd]
 * 
 * 	Made swtch_pri trap call swtch, ignoring priority argument (it
 * 	scrambles scheduling).
 * 	[89/02/02            dbg]
 * 
 * 	Removed all non-MACH calls and options.
 * 	[88/10/28            dbg]
 * 
 * Revision 2.1  89/08/03  15:51:36  rwd
 * Created.
 * 
 * Revision 2.8  89/03/05  16:48:24  rpd
 * 	Renamed the obsolete msg_{send,receive,rpc} traps to
 * 	msg_{send,receive,rpc}_old.
 * 	[89/02/15  13:50:10  rpd]
 * 
 * Revision 2.7  89/02/25  18:09:05  gm0w
 * 	Changes for cleanup.
 * 
 * Revision 2.6  89/01/10  23:32:13  rpd
 * 	Added MACH_IPC_XXXHACK conditionals around the obsolete IPC traps.
 * 	[89/01/10  23:10:07  rpd]
 * 
 * Revision 2.5  88/12/19  02:47:31  mwyoung
 * 	Removed old MACH conditionals.
 * 	[88/12/13            mwyoung]
 * 
 * Revision 2.4  88/10/27  10:48:53  rpd
 * 	Changed msg_{send,receive,rpc}_trap to 20, 21, 22.
 * 	[88/10/26  14:45:13  rpd]
 * 
 * Revision 2.3  88/10/11  10:20:33  rpd
 * 	Changed includes to the new style.  Replaced msg_receive_,
 * 	msg_rpc_ with msg_send_trap, msg_receive_trap, msg_rpc_trap.
 * 	[88/10/06  12:22:30  rpd]
 * 
 *  7-Apr-88  David Black (dlb) at Carnegie-Mellon University
 *	removed thread_times().
 *
 * 18-Jan-88  David Golub (dbg) at Carnegie-Mellon University
 *	Replaced task_data with thread_reply; change is invisible to
 *	users.
 *
 * 03-Mar-88  Douglas Orr (dorr) at Carnegie-Mellon University
 *	Added htg_unix_syscall()
 *
 * 27-Apr-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added new forms of msg_receive, msg_rpc.
 *
 *  1-Apr-87  William Bolosky (bolosky) at Carnegie-Mellon University
 *	Added "WARNING:" comment.
 *
 * 30-Mar-87  David Black (dlb) at Carnegie-Mellon University
 *	Added kern_timestamp()
 *
 * 27-Mar-87  David Black (dlb) at Carnegie-Mellon University
 *	Added thread_times() for MACH_TIME_NEW.  Flushed MACH_TIME.
 *
 * 25-Mar-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added map_fd.
 *
 * 27-Feb-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	MACH_IPC: Turn off Accent compatibility traps.
 *
 *  4-Feb-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added ctimes() routine, to make a workable restore program.
 *
 * 10-Nov-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Eliminated KPortToPID.
 *
 *  7-Nov-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added "init_process".
 *
 * 29-Oct-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added inode_swap_preference.
 *
 * 12-Oct-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Major reorganization: added msg_send, msg_receive, msg_rpc;
 *	moved real Mach traps up front; renamed table.
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
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

#include <mach/boolean.h>
#include <mach/port.h>
#include <mach/kern_return.h>
#include <kern/misc_protos.h>
#include <kern/syscall_sw.h>
#include <mach/message.h>

/* Forwards */
extern kern_return_t	kern_invalid(void);
extern mach_port_t	null_port(void);
extern kern_return_t	not_implemented(void);

/*
 *	To add a new entry:
 *		Add an "MACH_TRAP(routine, arg count)" to the table below.
 *
 *		Add trap definition to mach/syscall_sw.h and
 *		recompile user library.
 *
 * WARNING:	If you add a trap which requires more than 7
 *		parameters, mach/{machine}/syscall_sw.h and {machine}/trap.c
 *		and/or {machine}/locore.s may need to be modified for it
 *		to work successfully.
 *
 * WARNING:	Don't use numbers 0 through -9.  They (along with
 *		the positive numbers) are reserved for Unix.
 */

int kern_invalid_debug = 0;

/* Include declarations of the trap functions. */

#include <mach/mach_traps.h>
#include <mach/message.h>
#include <mach/rpc.h>
#include <mach/mach_syscalls.h>
#include <kern/syscall_subr.h>
#include <platforms.h>
#if	(iPSC860 || PARAGON860)
#include <mcmsg.h>
#endif	/* (iPSC860 || PARAGON860) */

mach_trap_t	mach_trap_table[] = {
	MACH_TRAP(kern_invalid, 0),		/* 0 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 1 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 2 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 3 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 4 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 5 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 6 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 7 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 8 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 9 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 10 obs. task_self */
	MACH_TRAP(kern_invalid, 0),		/* 11 obs. thread_reply */
	MACH_TRAP(kern_invalid, 0),		/* 12 obs. task_notify */
	MACH_TRAP(kern_invalid, 0),		/* 13 obs. thread_self */
	MACH_TRAP(kern_invalid, 0),		/* 14 */
	MACH_TRAP(kern_invalid, 0),		/* 15 */
	MACH_TRAP(kern_invalid, 0),		/* 16 */
	MACH_TRAP_STACK(evc_wait, 1),		/* 17 */
	MACH_TRAP(kern_invalid, 0),		/* 18 */
	MACH_TRAP(kern_invalid, 0),		/* 19 */
	MACH_TRAP(kern_invalid, 0),		/* 20 obs. msg_send_trap */
	MACH_TRAP(kern_invalid, 0),		/* 21 obs. msg_receive_trap */
	MACH_TRAP(kern_invalid, 0),		/* 22 obs. msg_rpc_trap */
	MACH_TRAP(kern_invalid, 0),		/* 23 */
	MACH_TRAP(kern_invalid, 0),		/* 24 */
	MACH_TRAP(kern_invalid, 0),		/* 25 */	/* obsolete */
	MACH_TRAP(mach_reply_port, 0),		/* 26 */
	MACH_TRAP(mach_thread_self, 0),		/* 27 */
	MACH_TRAP(mach_task_self, 0),		/* 28 */
	MACH_TRAP(mach_host_self, 0),		/* 29 */
	MACH_TRAP(syscall_vm_read_overwrite, 5),	/* 30 */
	MACH_TRAP(syscall_vm_write, 4),			/* 31 */
	MACH_TRAP_STACK(mach_msg_overwrite_trap, 9),	/* 32 */
	MACH_TRAP(kern_invalid, 0),		/* 33 rsvd for task_by_pid */
	MACH_TRAP(kern_invalid, 0),		/* 34 rsvd for pid_by_task */
#ifdef	i386
	MACH_TRAP(mach_rpc_trap, 4),		/* 35 */
	MACH_TRAP(mach_rpc_return_trap, 0),	/* 36 */
#else
	MACH_TRAP(not_implemented, 0),		/* 35 */
	MACH_TRAP(not_implemented, 0),		/* 36 */
#endif	/* i386 */
	MACH_TRAP(not_implemented, 0),		/* 37 */
	MACH_TRAP(not_implemented, 0),		/* 38 */
	MACH_TRAP(not_implemented, 0),		/* 39 */

	MACH_TRAP(not_implemented, 0),		/* 40 */
	MACH_TRAP(kern_invalid, 0),		/* 41 rsvd for init_process */
	MACH_TRAP(not_implemented, 0),		/* 42 */
	MACH_TRAP(not_implemented, 0),		/* 43 */
	MACH_TRAP(not_implemented, 0),		/* 44 */
	MACH_TRAP(not_implemented, 0),		/* 45 */
	MACH_TRAP(not_implemented, 0),		/* 46 */
	MACH_TRAP(not_implemented, 0),		/* 47 */
	MACH_TRAP(not_implemented, 0),		/* 48 */
	MACH_TRAP(not_implemented, 0),		/* 49 */

#if    VCF
	MACH_TRAP(syscall_vcf_init, 2),		/* 50 */
#else  /* VCF */
	MACH_TRAP(kern_invalid, 0),		/* 50 */
#endif /* VCF */
	MACH_TRAP(not_implemented, 0),		/* 51 */
	MACH_TRAP(not_implemented, 0),		/* 52 */
	MACH_TRAP(not_implemented, 0),		/* 53 */
	MACH_TRAP(not_implemented, 0),		/* 54 */
	MACH_TRAP(null_port, 0),		/* 55 obs host_self */
	MACH_TRAP(null_port, 0),		/* 56 */
	MACH_TRAP(not_implemented, 0),		/* 57 */
	MACH_TRAP(not_implemented, 0),		/* 58 */
 	MACH_TRAP_STACK(swtch_pri, 1),		/* 59 */

	MACH_TRAP_STACK(swtch, 0),		/* 60 */
	MACH_TRAP_STACK(syscall_thread_switch, 3),	/* 61 */
	MACH_TRAP_STACK(clock_sleep_trap, 5),	/* 62 */
	MACH_TRAP(syscall_task_create, 5),		/* 63 */
	MACH_TRAP(syscall_vm_map, 11),			/* 64 */
	MACH_TRAP(syscall_vm_allocate, 4),		/* 65 */
	MACH_TRAP(syscall_vm_deallocate, 3),		/* 66 */
	MACH_TRAP(syscall_vm_behavior_set, 4),		/* 67 */
	MACH_TRAP(not_implemented, 0),			/* 68 */
	MACH_TRAP(syscall_task_terminate, 1),		/* 69 */
	MACH_TRAP(syscall_task_suspend, 1),		/* 70 */
	MACH_TRAP(syscall_task_set_special_port, 3),	/* 71 */
	MACH_TRAP(syscall_mach_port_allocate, 3),	/* 72 */
	MACH_TRAP(syscall_mach_port_deallocate, 2),	/* 73 */
	MACH_TRAP(syscall_mach_port_insert_right, 4),	/* 74 */
	MACH_TRAP(syscall_mach_port_allocate_name, 3),	/* 75 */
	MACH_TRAP(syscall_thread_depress_abort, 1),	/* 76 */
        MACH_TRAP(syscall_device_read, 6),              /* 77 */
        MACH_TRAP(syscall_device_read_request, 5),      /* 78 */
        MACH_TRAP(syscall_device_write, 6),             /* 79 */
        MACH_TRAP(syscall_device_write_request, 6),     /* 80 */
	MACH_TRAP(syscall_device_read_overwrite, 6),	/* 81 */
	MACH_TRAP(syscall_device_read_overwrite_request, 6),	/* 82 */
	MACH_TRAP(syscall_clock_get_time, 2),		/* 83 */
	MACH_TRAP(syscall_thread_create_running, 5),	/* 84 */
	MACH_TRAP(syscall_mach_port_destroy, 2),	/* 85 */
	MACH_TRAP(syscall_vm_remap, 11),		/* 86 */
	MACH_TRAP(syscall_vm_region, 7),		/* 87 */
	MACH_TRAP(syscall_mach_port_move_member, 3),	/* 88 */
	MACH_TRAP(syscall_task_set_exception_ports, 5),	/* 89 */

	MACH_TRAP(syscall_mach_port_rename, 3),		/* 90 */
	MACH_TRAP(syscall_vm_protect, 5),		/* 91 */
	MACH_TRAP(syscall_vm_msync, 4),			/* 92 */
	MACH_TRAP(syscall_task_info, 4),		/* 93 */
	MACH_TRAP(device_read_async, 6),		/* 94 */
	MACH_TRAP(device_read_overwrite_async, 7),	/* 95 */
	MACH_TRAP(device_write_async, 7),		/* 96 */
	MACH_TRAP(io_done_queue_wait, 2),		/* 97 */
	MACH_TRAP(device_write_async_inband, 7),	/* 98 */
	MACH_TRAP(device_read_async_inband, 6),		/* 99 */
	MACH_TRAP(syscall_mach_port_allocate_subsystem, 3), /* 100 */
	MACH_TRAP(syscall_mach_port_allocate_qos, 4),	/* 101 */
	MACH_TRAP(syscall_task_set_port_space, 2),	/* 102 */
	MACH_TRAP(syscall_mach_port_allocate_full, 5),	/* 103 */
	MACH_TRAP(syscall_kernel_task_create, 4),	/* 104 */
	MACH_TRAP(syscall_thread_set_exception_ports, 5), /* 105 */
	MACH_TRAP(syscall_thread_info, 4),		/* 106 */
	MACH_TRAP(syscall_thread_create, 2),		/* 107 */
	MACH_TRAP(etap_probe, 3),			/* 108 */
	MACH_TRAP(etap_trace_event, 5),			/* 109 */

/* XXX temporary for limited backward compatibility */
#if	MCMSG && PARAGON860
	MACH_TRAP(syscall_thread_terminate, 1),	/* 110 */
	MACH_TRAP(syscall_mcmsg_flick, 0),	/* 111 */
	MACH_TRAP(syscall_vm_wire, 5),		/* 112 */
	MACH_TRAP(syscall_vm_machine_attribute, 5),	/* 113 */
	MACH_TRAP(syscall_thread_abort_safely, 1),	/* 114 */
	MACH_TRAP(syscall_thread_set_state, 4),	/* 115 */
	MACH_TRAP(syscall_thread_suspend, 1),	/* 116 */
	MACH_TRAP(syscall_host_statistics, 4),	/* 117 */
	MACH_TRAP(syscall_mach_port_mod_refs, 4),	/* 118 */
	MACH_TRAP(syscall_mach_port_request_notification, 7),	/* 119 */
	MACH_TRAP(not_implemented, 0),		/* 120 */
	MACH_TRAP(syscall_mcmsg_boot_send, 7),	/* 121 */
	MACH_TRAP(syscall_mcmsg_console_open, 2),	/* 122 */
	MACH_TRAP(syscall_mcmsg_console_close, 1),	/* 123 */
	MACH_TRAP(syscall_mcmsg_console_read, 3),	/* 124 */
	MACH_TRAP(syscall_mcmsg_console_write, 3),	/* 125 */
	MACH_TRAP(syscall_mcmsg_myphysnode, 0),		/* 126 */
	MACH_TRAP(kern_invalid, 0),		/* 127 */
#else   /* MCMSG && PARAGON860 */
	MACH_TRAP(syscall_thread_terminate, 1),	/* 110 */
	MACH_TRAP(not_implemented, 0),		/* 111 */
	MACH_TRAP(syscall_vm_wire, 5),		/* 112 */
	MACH_TRAP(syscall_vm_machine_attribute, 5),	/* 113 */
	MACH_TRAP(syscall_thread_abort_safely, 1),	/* 114 */
	MACH_TRAP(syscall_thread_set_state, 4),	/* 115 */
	MACH_TRAP(syscall_thread_suspend, 1),	/* 116 */
	MACH_TRAP(syscall_host_statistics, 4),	/* 117 */
	MACH_TRAP(syscall_mach_port_mod_refs, 4),	/* 118 */
	MACH_TRAP(syscall_mach_port_request_notification, 7),	/* 119 */
	MACH_TRAP(not_implemented, 0),		/* 120 */
	MACH_TRAP(not_implemented, 0),		/* 121 */
	MACH_TRAP(not_implemented, 0),		/* 122 */
	MACH_TRAP(not_implemented, 0),		/* 123 */
	MACH_TRAP(not_implemented, 0),		/* 124 */
	MACH_TRAP(not_implemented, 0),		/* 125 */
	MACH_TRAP(not_implemented, 0),		/* 126 */
	MACH_TRAP(not_implemented, 0),		/* 127 */
#endif	/* MCMSG && PARAGON860 */
	MACH_TRAP(not_implemented, 0),		/* 128 */
	MACH_TRAP(not_implemented, 0),		/* 129 */

#if	MCMSG && PARAGON860
	MACH_TRAP(syscall_mcmsg_interface, 12),	/* 130 */
	MACH_TRAP(syscall_mcmsg_myphysnode, 0),	/* 131 */
	MACH_TRAP(syscall_mcmsg_mp_access, 1),	/* 132 */
	MACH_TRAP(syscall_mcmsg_flick, 0),	/* 133 */
	MACH_TRAP(syscall_mcmsg_boot_send, 7),	/* 134 */
	MACH_TRAP(syscall_mcmsg_console_open, 2),	/* 135 */
	MACH_TRAP(syscall_mcmsg_console_close, 1),	/* 136 */
	MACH_TRAP(syscall_mcmsg_console_read, 3),	/* 137 */
	MACH_TRAP(syscall_mcmsg_console_write, 3),	/* 138 */
	MACH_TRAP(not_implemented, 0),		/* 139 */

#if	NX
	MACH_TRAP(syscall_mcmsg_wire, 2),	/* 140 */
	MACH_TRAP(syscall_mcmsg_nodeinfo, 1),	/* 141 */
	MACH_TRAP(syscall_mcmsg_nxport_setup, 1),	/* 142 */
	MACH_TRAP(syscall_mcmsg_nx_local_ast, 1),	/* 143 */
	MACH_TRAP(syscall_mcmsg_nx_op, 9),		/* 144 */
	MACH_TRAP(not_implemented, 0),		/* 145 */
	MACH_TRAP(not_implemented, 0),		/* 146 */
	MACH_TRAP(not_implemented, 0),		/* 147 */
	MACH_TRAP(not_implemented, 0),		/* 148 */
	MACH_TRAP(not_implemented, 0),		/* 149 */
#else	/* NX */
	MACH_TRAP(not_implemented, 0),		/* 140 */
	MACH_TRAP(not_implemented, 0),		/* 141 */
	MACH_TRAP(not_implemented, 0),		/* 142 */
	MACH_TRAP(not_implemented, 0),		/* 143 */
	MACH_TRAP(not_implemented, 0),		/* 144 */
	MACH_TRAP(not_implemented, 0),		/* 145 */
	MACH_TRAP(not_implemented, 0),		/* 146 */
	MACH_TRAP(not_implemented, 0),		/* 147 */
	MACH_TRAP(not_implemented, 0),		/* 148 */
#if	VC
	MACH_TRAP(syscall_mcmsg_vc, 9),		/* 149 */
#else	/* VC */
	MACH_TRAP(not_implemented, 0),		/* 149 */
#endif	/* VC */
#endif	/* NX */
	MACH_TRAP(not_implemented, 0),		/* 150 */
	MACH_TRAP(not_implemented, 0),		/* 151 */
	MACH_TRAP(not_implemented, 0),		/* 152 */
	MACH_TRAP(not_implemented, 0),		/* 153 */
	MACH_TRAP(not_implemented, 0),		/* 154 */
	MACH_TRAP(not_implemented, 0),		/* 155 */
	MACH_TRAP(not_implemented, 0),		/* 156 */
	MACH_TRAP(not_implemented, 0),		/* 157 */
	MACH_TRAP(not_implemented, 0),		/* 158 */
	MACH_TRAP(not_implemented, 0),		/* 159 */
	MACH_TRAP(not_implemented, 0),		/* 160 */
	MACH_TRAP(not_implemented, 0),		/* 161 */
	MACH_TRAP(not_implemented, 0),		/* 162 */
	MACH_TRAP(not_implemented, 0),		/* 163 */
	MACH_TRAP(not_implemented, 0),		/* 164 */
	MACH_TRAP(not_implemented, 0),		/* 165 */
	MACH_TRAP(not_implemented, 0),		/* 166 */
	MACH_TRAP(not_implemented, 0),		/* 167 */
	MACH_TRAP(not_implemented, 0),		/* 168 */
	MACH_TRAP(not_implemented, 0),		/* 169 */
	MACH_TRAP(not_implemented, 0),		/* 170 */
#else	/* MCMSG && PARAGON860 */
	MACH_TRAP(not_implemented, 0),		/* 130 */
	MACH_TRAP(not_implemented, 0),		/* 131 */
	MACH_TRAP(not_implemented, 0),		/* 132 */
	MACH_TRAP(not_implemented, 0),		/* 133 */
	MACH_TRAP(not_implemented, 0),		/* 134 */
	MACH_TRAP(not_implemented, 0),		/* 135 */
	MACH_TRAP(not_implemented, 0),		/* 136 */
	MACH_TRAP(not_implemented, 0),		/* 137 */
	MACH_TRAP(not_implemented, 0),		/* 138 */
	MACH_TRAP(not_implemented, 0),		/* 139 */
	MACH_TRAP(not_implemented, 0),		/* 140 */
	MACH_TRAP(not_implemented, 0),		/* 141 */
	MACH_TRAP(not_implemented, 0),		/* 142 */
	MACH_TRAP(not_implemented, 0),		/* 143 */
	MACH_TRAP(not_implemented, 0),		/* 144 */
	MACH_TRAP(not_implemented, 0),		/* 145 */
	MACH_TRAP(not_implemented, 0),		/* 146 */
	MACH_TRAP(not_implemented, 0),		/* 147 */
	MACH_TRAP(not_implemented, 0),		/* 148 */
	MACH_TRAP(not_implemented, 0),		/* 149 */
	MACH_TRAP(not_implemented, 0),		/* 150 */
	MACH_TRAP(not_implemented, 0),		/* 151 */
	MACH_TRAP(not_implemented, 0),		/* 152 */
	MACH_TRAP(not_implemented, 0),		/* 153 */
	MACH_TRAP(not_implemented, 0),		/* 154 */
	MACH_TRAP(not_implemented, 0),		/* 155 */
	MACH_TRAP(not_implemented, 0),		/* 156 */
	MACH_TRAP(not_implemented, 0),		/* 157 */
	MACH_TRAP(not_implemented, 0),		/* 158 */
	MACH_TRAP(not_implemented, 0),		/* 159 */
	MACH_TRAP(not_implemented, 0),		/* 160 */
	MACH_TRAP(not_implemented, 0),		/* 161 */
	MACH_TRAP(not_implemented, 0),		/* 162 */
	MACH_TRAP(not_implemented, 0),		/* 163 */
	MACH_TRAP(not_implemented, 0),		/* 164 */
	MACH_TRAP(not_implemented, 0),		/* 165 */
	MACH_TRAP(not_implemented, 0),		/* 166 */
	MACH_TRAP(not_implemented, 0),		/* 167 */
	MACH_TRAP(not_implemented, 0),		/* 168 */
	MACH_TRAP(not_implemented, 0),		/* 169 */
	MACH_TRAP(not_implemented, 0),		/* 170 */
#endif	/* MCMSG && PARAGON860 */
	MACH_TRAP(syscall_memory_object_change_attributes, 5),	/* 171 */
	MACH_TRAP(syscall_memory_object_data_error, 4),		/* 171 */
	MACH_TRAP(syscall_memory_object_data_unavailable, 3),	/* 173 */
	MACH_TRAP(syscall_memory_object_synchronize_completed, 3),/* 174 */
	MACH_TRAP(syscall_memory_object_lock_request, 7),	/* 175 */
	MACH_TRAP(syscall_memory_object_discard_reply, 7),	/* 176 */
	MACH_TRAP(syscall_memory_object_data_supply, 8),	/* 177 */

};

int	mach_trap_count = (sizeof(mach_trap_table) / sizeof(mach_trap_table[0]));

mach_port_t
null_port(void)
{
	if (kern_invalid_debug) Debugger("null_port mach trap");
	return(MACH_PORT_NULL);
}

kern_return_t
kern_invalid(void)
{
	if (kern_invalid_debug) Debugger("kern_invalid mach trap");
	return(KERN_INVALID_ARGUMENT);
}

kern_return_t
not_implemented(void)
{
	return(MACH_SEND_INTERRUPTED);
}
