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
 * Revision 2.8  91/11/12  11:51:58  rvb
 * 	Added simple_lock_pause.
 * 	[91/11/12            rpd]
 * 
 * Revision 2.7  91/05/18  14:32:17  rpd
 * 	Added check_simple_locks.
 * 	[91/03/31            rpd]
 * 
 * Revision 2.6  91/05/14  16:43:51  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/05/08  12:47:17  dbg
 * 	When actually using the locks (on multiprocessors), import the
 * 	machine-dependent simple_lock routines from machine/lock.h.
 * 	[91/04/26  14:42:23  dbg]
 * 
 * Revision 2.4  91/02/05  17:27:37  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:14:39  mrt]
 * 
 * Revision 2.3  90/11/05  14:31:18  rpd
 * 	Added simple_lock_taken.
 * 	[90/11/04            rpd]
 * 
 * Revision 2.2  90/01/11  11:43:26  dbg
 * 	Upgraded to match mainline:
 * 	 	Added decl_simple_lock_data, simple_lock_addr macros.
 * 	 	Rearranged complex lock structure to use decl_simple_lock_data
 * 	 	for the interlock field and put it last (except on ns32000).
 * 	 	[89/01/15  15:16:47  rpd]
 * 
 * 	Made all machines use the compact field layout.
 * 
 * Revision 2.1  89/08/03  15:49:42  rwd
 * Created.
 * 
 * Revision 2.2  88/07/20  16:49:35  rpd
 * Allow for sanity-checking of simple locking on uniprocessors,
 * controlled by new option MACH_LDEBUG.  Define composite
 * MACH_SLOCKS, which is true iff simple locking calls expand
 * to code.  It can be used to #if-out declarations, etc, that
 * are only used when simple locking calls are real.
 * 
 *  3-Nov-87  David Black (dlb) at Carnegie-Mellon University
 *	Use optimized lock structure for multimax also.
 *
 * 27-Oct-87  Robert Baron (rvb) at Carnegie-Mellon University
 *	Use optimized lock "structure" for balance now that locks are
 *	done inline.
 *
 * 26-Jan-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Invert logic of no_sleep to can_sleep.
 *
 * 29-Dec-86  David Golub (dbg) at Carnegie-Mellon University
 *	Removed BUSYP, BUSYV, adawi, mpinc, mpdec.  Defined the
 *	interlock field of the lock structure to be a simple-lock.
 *
 *  9-Nov-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added "unsigned" to fields in vax case, for lint.
 *
 * 21-Oct-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added fields for sleep/recursive locks.
 *
 *  7-Oct-86  David L. Black (dlb) at Carnegie-Mellon University
 *	Merged Multimax changes.
 *
 * 26-Sep-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Removed reference to "caddr_t" from BUSYV/P.  I really
 *	wish we could get rid of these things entirely.
 *
 * 24-Sep-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Changed to directly import boolean declaration.
 *
 *  1-Aug-86  David Golub (dbg) at Carnegie-Mellon University
 *	Added simple_lock_try, sleep locks, recursive locking.
 *
 * 11-Jun-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Removed ';' from definitions of locking macros (defined only
 *	when NCPU < 2). so as to make things compile.
 *
 * 28-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Defined adawi to be add when not on a vax.
 *
 * 07-Nov-85  Michael Wayne Young (mwyoung) at Carnegie-Mellon University
 *	Overhauled from previous version.
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
 *	File:	kern/lock.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *	Date:	1985
 *
 *	Locking primitives definitions
 */

#ifndef	_KERN_LOCK_H_
#define	_KERN_LOCK_H_

/*
 * Configuration variables:
 *
 *
 *	MACH_LDEBUG:    record pc and thread of callers, turn on
 *			all lock debugging.
 *
 *
 *	ETAP:		The Event Trace Analysis Package (ETAP) monitors
 *			and records micro-kernel lock behavior and general
 *			kernel events.  ETAP supports two levels of
 *			tracing for locks:
 *				- cumulative (ETAP_LOCK_ACCUMULATE)
 *				- monitored  (ETAP_LOCK_MONITOR)
 *
 *			Note: If either level of tracing is configured then
 *			      ETAP_LOCK_TRACE is automatically defined to 
 *			      equal one.
 *
 * 		        Several macros are added throughout the lock code to
 *                      allow for convenient configuration.
 */


#include <cpus.h>
#include <mach_rt.h>
#include <mach_ldebug.h>

#include <mach/boolean.h>
#include <kern/kern_types.h>
#include <kern/etap_options.h>

/*
 *	These configuration variables must be defined
 *	for the benefit of the machine-dependent lock
 *	package.
 */
#define	USLOCK_STATS	ETAP_LOCK_TRACE
#define	USLOCK_DEBUG	MACH_LDEBUG
#define	USLOCK_PROD	(!(USLOCK_STATS || USLOCK_DEBUG))


#include <machine/lock.h>
#include <kern/cpu_data.h>
#include <mach/etap_events.h>
#include <mach/etap.h>
#include <kern/etap_pool.h>

/*
 *	The Mach lock package exports the following lock abstractions:
 *
 *	Lock Type  Properties
 *	hw_lock	   lowest level hardware abstraction; atomic,
 *		   non-blocking, mutual exclusion; supports pre-emption
 *	usimple	   non-blocking spinning lock, available in all
 *		   kernel configurations; may be used from thread
 *		   and interrupt contexts; supports debugging,
 *		   statistics and pre-emption
 *	simple	   non-blocking spinning lock, intended for SMP
 *		   synchronization (vanishes on a uniprocessor);
 *		   supports debugging, statistics and pre-emption
 *	mutex	   blocking mutual exclusion lock, intended for
 *		   SMP synchronization (vanishes on a uniprocessor);
 *		   supports debugging, statistics, and pre-emption
 *	lock	   blocking synchronization permitting multiple
 *		   simultaneous readers or a single writer; supports
 *		   debugging and statistics but not pre-emption
 *
 *	In general, mutex locks are preferred over all others, as the
 *	mutex supports pre-emption and relinquishes the processor
 *	upon contention.
 *
 *	NOTES TO IMPLEMENTORS:  there are essentially two versions
 *	of the lock package.  One is portable, written in C, and
 *	supports all of the various flavors of debugging, statistics,
 *	uni- versus multi-processor, pre-emption, etc.  The "other"
 *	is whatever set of lock routines is provided by machine-dependent
 *	code.  Presumably, the machine-dependent package is heavily
 *	optimized and meant for production kernels.
 *
 *	We encourage implementors to focus on highly-efficient,
 *	production implementations of machine-dependent lock code,
 *	and use the portable lock package for everything else.
 */

/*
 *	The "hardware lock".  Low-level locking primitives that
 *	MUST be exported by machine-dependent code; this abstraction
 *	must provide atomic, non-blocking mutual exclusion that
 *	is invulnerable to uniprocessor or SMP races, interrupts,
 *	traps or any other events.
 *
 *		hw_lock_data_t		machine-specific lock data structure
 *		hw_lock_t		pointer to hw_lock_data_t
 *
 *	An implementation must export these data types and must
 *	also provide routines to manipulate them (see prototypes,
 *	below).  These routines may be external, inlined, optimized,
 *	or whatever, based on the kernel configuration.  In the event
 *	that the implementation wishes to define its own prototypes,
 *	macros, or inline functions, it may define LOCK_HW_PROTOS
 *	to disable the definitions below.
 *
 *	Mach does not expect these locks to support statistics,
 *	debugging, tracing or any other complexity.  In certain
 *	configurations, Mach will build other locking constructs
 *	on top of this one.  A correctly functioning Mach port need
 *	only implement these locks to be successful.  However,
 *	greater efficiency may be gained with additional machine-
 *	dependent optimizations for the locking constructs defined
 *	later in this file.
 */

#ifndef	HW_LOCK_PROTOS
/*
 *	Mach always initializes locks, even those statically
 *	allocated.
 *
 *	The conditional acquisition call, hw_lock_try,
 *	must return non-zero on success and zero on failure.
 *
 *	The hw_lock_held operation returns non-zero if the
 *	lock is set, zero if the lock is clear.  This operation
 *	should be implemented using an ordinary memory read,
 *	rather than a special atomic instruction, allowing
 *	a processor to spin in cache waiting for the lock to
 *	be released without chewing up bus cycles.
 */
extern void		hw_lock_init(hw_lock_t);
extern void		hw_lock_lock(hw_lock_t);
extern void		hw_lock_unlock(hw_lock_t);
extern unsigned int	hw_lock_try(hw_lock_t);
extern unsigned int	hw_lock_held(hw_lock_t);
#endif	/* HW_LOCK_PROTOS */


/*
 *	All of the remaining locking constructs may have two versions.
 *	One version is machine-independent, built in C on top of the
 *	hw_lock construct.  This version supports production, debugging
 *	and statistics configurations and is portable across architectures.
 *
 *	Any particular port may override some or all of the portable
 *	lock package for whatever reason -- usually efficiency.
 *
 *	The direct use of hw_locks by machine-independent Mach code
 *	should be rare; the preferred spinning lock is the simple_lock
 *	(see below).
 */

#if ETAP
/*
 *	Locks have a pointer into an event_table entry that names the
 *	corresponding lock event and controls whether it is being traced.
 *	Initially this pointer is into a read-only table event_table_init[].
 *	Once dynamic allocation becomes possible a modifiable copy of the table
 *	is allocated and pointers are set to within this copy.  The pointers
 *	that were already in place at that point need to be switched to point
 *	into the copy.  To do this we overlay the event_table_chain structure
 *	onto sufficiently-big elements of the various lock structures so we
 *	can sweep down this list switching the pointers.  The assumption is
 *	that we will not want to enable tracing before this is done (which is
 *	after all during kernel bootstrap, before any user tasks are launched).
 *
 *	This is admittedly rather ugly but so were the alternatives:
 *	- record the event_table pointers in a statically-allocated array
 *	  (dynamic allocation not yet being available) -- but there were
 *	  over 8000 of them;
 *	- add a new link field to each lock structure;
 *	- change pointers to array indices -- this adds quite a bit of
 *	  arithmetic to every lock operation that might be traced.
 */

struct event_table_chain {
	event_table_t event_tablep;
	struct event_table_chain *event_table_link;
};
#define lock_event_table(lockp)		((lockp)->u.s.event_tablep)
#define lock_start_hold_time(lockp)	((lockp)->u.s.start_hold_time)
#endif	/* ETAP_LOCK_TRACE */

/*
 *	A "simple" spin lock, providing non-blocking mutual
 *	exclusion and conditional acquisition.
 *
 *	The usimple_lock exists even in uniprocessor configurations.
 *	A data structure is always allocated for it and the following
 *	operations are always defined:
 *
 *		usimple_lock_data_t	machine-specific lock data structure
 *		usimple_lock_t		pointer to hw_lock_data_t
 *
 *		usimple_lock_init	lock initialization (mandatory!)
 *		usimple_lock		lock acquisition
 *		usimple_unlock		lock release
 *		usimple_lock_try	conditional lock acquisition;
 *					non-zero means success
#if	USLOCK_DEBUG
 *		usimple_lock_held	verify lock already held by me
 *		usimple_lock_none_held	verify no usimple locks are held
#endif
 *
 *	The usimple_lock may be used for synchronization between
 *	thread context and interrupt context, or between a uniprocessor
 *	and an intelligent device.  Obviously, it may also be used for
 *	multiprocessor synchronization.  Its use should be rare; the
 *	simple_lock is the preferred spinning lock (see below).
 *
 *	The usimple_lock supports optional lock debugging and statistics.
 *
 *	The usimple_lock may be inlined or optimized in ways that
 *	depend on the particular machine architecture and kernel
 *	build configuration; e.g., processor type, number of CPUs,
 *	production v. debugging.
 *
 *	Normally, we expect the usimple_lock data structure to be
 *	defined here, with its operations implemented in an efficient,
 *	machine-dependent way.  However, any implementation may choose
 *	to rely on a C-based, portable  version of the usimple_lock for
 *	debugging, statistics, and/or tracing.  Three hooks are used in
 *	the portable lock package to allow the machine-dependent package
 *	to override some or all of the portable package's features.
 *
 *	
The usimple_lock data structure
 *	can be overriden in a machine-dependent way by defining
 *	LOCK_USIMPLE_DATA, although we expect this to be unnecessary.
 *	The calls manipulating usimple_locks can be overriden by
 *	defining LOCK_USIMPLE_CALLS -- we expect this to be the typical
 *	case for "production" configurations.  (Note that if you choose
 *	to override LOCK_USIMPLE_DATA, you'd better also be prepared
 *	to override LOCK_USIMPLE_CALLS.)  Debugging, statistics,
 *	and the like typically will be handled in the C code.
 *
#if	MACH_RT
 *	The usimple_lock also handles pre-emption.  Lock acquisition
 *	implies disabling pre-emption, while lock release implies
 *	re-enabling pre-emption.  Conditional lock acquisition does
 *	not assume success:  on success, pre-emption is disabled
 *	but on failure the pre-emption state remains the same as
 *	the pre-emption state before the acquisition attempt.
#endif
 */

#ifndef	USIMPLE_LOCK_DATA

#if	USLOCK_DEBUG
/*
 *	This structure records additional information about lock state
 *	and recent operations.  The data are carefully organized so that
 *	some portions of it can be examined BEFORE actually acquiring
 *	the lock -- for instance, the lock_thread field, to detect an
 *	attempt to acquire a lock already owned by the calling thread.
 *	All *updates* to this structure are governed by the lock to which
 *	this structure belongs.
 *
 *	Note cache consistency dependency:  being able to examine some
 *	of the fields in this structure without first acquiring a lock
 *	implies strongly-ordered cache coherency OR release consistency.
 *	Perhaps needless to say, acquisition consistency may not suffice.
 *	However, it's hard to imagine a scenario using acquisition
 *	consistency that results in using stale data from this structure.
 *	It would be necessary for the thread manipulating the lock to
 *	switch to another processor without first executing any instructions
 *	that would cause the needed consistency updates; basically, without
 *	taking a lock.  Not possible in this kernel!
 */
typedef struct uslock_debug {
        void		*lock_pc;	/* pc where lock operation began    */
	void		*lock_thread;	/* thread that acquired lock */
	unsigned long	duration[2];
	unsigned short	state;
	etap_event_t	etap_type;	/* ETAP event class */
	unsigned char	lock_cpu;
	void		*unlock_thread;	/* last thread to release lock */
	unsigned char	unlock_cpu;
        void		*unlock_pc;	/* pc where lock operation ended    */
} uslock_debug;
#endif	/* USLOCK_DEBUG */

typedef struct uslock {
	hw_lock_data_t	interlock;	/* must be first... see lock.c */
#if	USLOCK_DEBUG
	unsigned short	lock_type;	/* must be second... see lock.c */
#define	USLOCK_TAG	0x5353
	uslock_debug	debug;
#endif	/* USLOCK_DEBUG */
#if	ETAP_LOCK_TRACE
	union {		/* Must be overlaid on the event_tablep */
	    struct event_table_chain event_table_chain;
	    struct {
		event_table_t   event_tablep;     /* ptr to event table entry */
		etap_time_t	start_hold_time;  /* Time of last acquistion */
	    } s;
	} u;
#endif	/* ETAP_LOCK_TRACE */
#if     ETAP_LOCK_ACCUMULATE
        cbuff_entry_t  	cbuff_entry;	  /* cumulative buffer entry          */
#endif 	/* ETAP_LOCK_ACCUMULATE */
#if	ETAP_LOCK_MONITOR
        vm_offset_t	start_pc;	  /* pc where lock operation began    */
        vm_offset_t	end_pc;		  /* pc where lock operation ended    */
#endif 	/* ETAP_LOCK_MONITOR */
} usimple_lock_data_t, *usimple_lock_t;

#define	USIMPLE_LOCK_NULL	((usimple_lock_t) 0)

#endif	/* USIMPLE_LOCK_DATA */


#ifndef	USIMPLE_LOCK_PROTOS
/*
 *	Each usimple_lock has a type, used for debugging and
 *	statistics.  This type may safely be ignored in a
 *	production configuration.
 *
 *	The conditional acquisition call, usimple_lock_try,
 *	must return non-zero on success and zero on failure.
 */
extern void		usimple_lock_init(usimple_lock_t,etap_event_t);
extern void		usimple_lock(usimple_lock_t);
extern void		usimple_unlock(usimple_lock_t);
extern unsigned int	usimple_lock_try(usimple_lock_t);
#if	USLOCK_DEBUG
extern void		usimple_lock_held(usimple_lock_t);
extern void		usimple_lock_none_held(void);
#else	/* USLOCK_DEBUG */
#define	usimple_lock_held(l)
#define	usimple_lock_none_held()
#endif	/* USLOCK_DEBUG */
#endif	/* USIMPLE_LOCK_PROTOS */


/*
 *	Upon the usimple_lock we define the simple_lock, which
 *	exists for SMP configurations.  These locks aren't needed
 *	in a uniprocessor configuration, so compile-time tricks
 *	make them disappear when NCPUS==1.  (For debugging purposes,
 *	however, they can be enabled even on a uniprocessor.)  This
 *	should be the "most popular" spinning lock; the usimple_lock
 *	and hw_lock should only be used in rare cases.
 *
 *	IMPORTANT:  simple_locks that may be shared between interrupt
 *	and thread context must have their use coordinated with spl.
 *	The spl level must alway be the same when acquiring the lock.
 *	Otherwise, deadlock may result.
 *
 *	Given that, in some configurations, Mach does not need to
 *	allocate simple_lock data structures, users of simple_locks
 *	should employ the "decl_simple_lock_data" macro when allocating
 *	simple_locks.  Note that it use should be something like
 *		decl_simple_lock_data(static,foo_lock)
 *	WITHOUT any terminating semi-colon.  Because the macro expands
 *	to include its own semi-colon, if one is needed, it may safely
 *	be used multiple times at arbitrary positions within a structure.
 *	Adding a semi-colon will cause structure definitions to fail
 *	when locks are turned off and a naked semi-colon is left behind. 
 */

/*
 *	Decide whether to allocate simple_lock data structures.
 *	If the machine-dependent code has turned on LOCK_SIMPLE_DATA,
 *	then it assumes all responsibility.  Otherwise, we need
 *	these data structures if the configuration includes SMP or
 *	lock debugging or statistics.
 *
 *	N.B.  Simple locks should be declared using
 *		decl_simple_lock_data(class,name)
 *	with no trailing semi-colon.  This syntax works best because
 *		- it correctly disappears in production uniprocessor
 *		  configurations, leaving behind no allocated data
 *		  structure
 *		- it can handle static and extern declarations:
 *			decl_simple_lock_data(extern,foo)	extern
 *			decl_simple_lock_data(static,foo)	static
 *			decl_simple_lock_data(,foo)		ordinary
 */
typedef	usimple_lock_data_t	*simple_lock_t;

#if     (!defined(LOCK_SIMPLE_DATA) && \
	 ((NCPUS > 1) || USLOCK_DEBUG || ETAP_LOCK_TRACE))
typedef usimple_lock_data_t	simple_lock_data_t;
#define	decl_simple_lock_data(class,name) \
class	simple_lock_data_t	name;
#else	
#define	decl_simple_lock_data(class,name)
#endif	

/*
 *	Decide whether the portable lock package is responsible
 *	for simple_lock calls and, if so, whether they should
 *	be real or just all go away (like a bad dream).
 */
#if 0
#define	SL_CALLS	(!defined(LOCK_SIMPLE_CALLS) &&		\
			 ((NCPUS > 1) || USLOCK_DEBUG ||	\
			  ETAP_LOCK_TRACE || MACH_RT))
#endif

#if		(!defined(LOCK_SIMPLE_CALLS) &&	\
		 ((NCPUS > 1) || USLOCK_DEBUG ||	\
		  ETAP_LOCK_TRACE || MACH_RT))

#if	MACH_RT && NCPUS == 1 && !ETAP_LOCK_TRACE && !USLOCK_DEBUG
/*
 *	MACH_RT is a very special case:  in the case that the
 *	machine-dependent lock package hasn't taken responsibility
 *	but there is no other reason to turn on locks, if MACH_RT
 *	is turned on locks denote critical, non-preemptable points
 *	in the code.
 *
 *	Otherwise, simple_locks may be layered directly on top of
 *	usimple_locks.
 *
 *	N.B.  The reason that simple_lock_try may be assumed to
 *	succeed under MACH_RT is that the definition only is used
 *	when NCPUS==1 AND because simple_locks shared between thread
 *	and interrupt context are always acquired with elevated spl.
 *	Thus, it is never possible to be interrupted in a dangerous
 *	way while holding a simple_lock.
 */
#define simple_lock_init(l,t)
#define	simple_lock(l)		disable_preemption()
#define	simple_unlock(l)	enable_preemption()
#define simple_lock_try(l)	(disable_preemption(), 1)
#define simple_lock_addr(lock)	((simple_lock_t)0)
#define	__slock_held_func__(l)	preemption_is_disabled()
#else	/* MACH_RT && NCPUS == 1 && !ETAP_LOCK_TRACE && !USLOCK_DEBUG */
#define simple_lock_init(l,t)	usimple_lock_init(l,t)
#define	simple_lock(l)		usimple_lock(l)
#define	simple_unlock(l)	usimple_unlock(l)
#define simple_lock_try(l)	usimple_lock_try(l)
#define simple_lock_addr(l)	(&(l))
#define	__slock_held_func__(l)	usimple_lock_held(l)

#if	ETAP_LOCK_TRACE
extern	void	simple_lock_no_trace(simple_lock_t l);
extern	int	simple_lock_try_no_trace(simple_lock_t l);
extern	void	simple_unlock_no_trace(simple_lock_t l);
#endif	/* ETAP_LOCK_TRACE */

#endif	/* MACH_RT && NCPUS == 1 && !ETAP_LOCK_TRACE && !USLOCK_DEBUG */

#else	/* SL_CALLS */

/*
 *	The machine-dependent lock package isn't taking responsibility
 *	for locks and there is no other apparent reason to turn them on.
 *	So make them disappear.
 */
#define simple_lock_init(l,t)
#define	simple_lock(l)
#define	simple_unlock(l)
#define simple_lock_try(l)	(1)
#define simple_lock_addr(l)	((simple_lock_t)0)
#define	__slock_held_func__(l)	FALSE

#endif	/* SL_CALLS */

#if	USLOCK_DEBUG
/*
 *	Debug-time only:
 *		+ verify that usimple_lock is already held by caller
 *		+ verify that usimple_lock is NOT held by caller
 *		+ verify that current processor owns no usimple_locks
 *
 *	We do not provide a simple_lock_NOT_held function because
 *	it's impossible to verify when only MACH_RT is turned on.
 *	In that situation, only preemption is enabled/disabled
 *	around lock use, and it's impossible to tell which lock
 *	acquisition caused preemption to be disabled.  However,
 *	note that it's still valid to use check_simple_locks
 *	when only MACH_RT is turned on -- no locks should be
 *	held, hence preemption should be enabled.
 *	Actually, the above isn't strictly true, as explicit calls
 *	to disable_preemption() need to be accounted for.
 */
#define	simple_lock_held(l)	__slock_held_func__(l)
#define	check_simple_locks()	usimple_lock_none_held()
#else	/* USLOCK_DEBUG */
#define	simple_lock_held(l)
#define	check_simple_locks()
#endif	/* USLOCK_DEBUG */


/*
 *	A simple mutex lock.
 *	Do not change the order of the fields in this structure without
 *	changing the machine-dependent assembler routines which depend
 *	on them.
 */

typedef struct {
	hw_lock_data_t	interlock;
	hw_lock_data_t	locked;
	short		waiters;
#if	MACH_LDEBUG
	int		type;
#define	MUTEX_TAG	0x4d4d
	vm_offset_t	pc;
	vm_offset_t	thread;
#endif	/* MACH_LDEBUG */
#if     ETAP_LOCK_TRACE
	union {		/* Must be overlaid on the event_tablep */
	    struct event_table_chain event_table_chain;
	    struct {
		event_table_t   event_tablep;     /* ptr to event table entry */
		etap_time_t	start_hold_time;  /* Time of last acquistion */
	    } s;
	} u;
#endif 	/* ETAP_LOCK_TRACE */
#if     ETAP_LOCK_ACCUMULATE
        cbuff_entry_t  	cbuff_entry;	  /* cumulative buffer entry          */
#endif 	/* ETAP_LOCK_ACCUMULATE */
#if	ETAP_LOCK_MONITOR
        vm_offset_t	start_pc;	  /* pc where lock operation began    */
        vm_offset_t	end_pc;		  /* pc where lock operation ended    */
#endif 	/* ETAP_LOCK_MONITOR */
} mutex_t;

#define	decl_mutex_data(class,name)	class mutex_t name;

extern void	mutex_init		(mutex_t*, etap_event_t);

#if	MACH_RT || (NCPUS > 1) || MACH_LDEBUG || ETAP_LOCK_TRACE

extern void	_mutex_lock		(mutex_t*);
extern void	mutex_unlock		(mutex_t*);
extern boolean_t  _mutex_try		(mutex_t*);

#else	/* MACH_RT || (NCPUS > 1) || MACH_LDEBUG || ETAP_LOCK_TRACE */

#define	_mutex_lock(l)
#define	mutex_unlock(l)
#define	_mutex_try(l)			(1)

#endif	/* MACH_RT || (NCPUS > 1) || MACH_LDEBUG || ETAP_LOCK_TRACE */

#if	MACH_LDEBUG
#define mutex_held(m) (hw_lock_held(&((m)->locked)) && \
		       ((m)->thread == (int)current_thread()))
#else	/* MACH_LDEBUG */
#define mutex_held(m) hw_lock_held(&((m)->locked))
#endif	/* MACH_LDEBUG */

extern void	mutex_lock_wait		(mutex_t*);
extern void	mutex_unlock_wakeup	(mutex_t*);
extern void	mutex_pause		(void);
extern void	interlock_unlock	(hw_lock_t);

/*
 *	The general lock structure.  Provides for multiple readers,
 *	upgrading from read to write, and sleeping until the lock
 *	can be gained.
 *
 *	On some architectures, assembly language code in the 'inline'
 *	program fiddles the lock structures.  It must be changed in
 *	concert with the structure layout.
 *
 *	Only the "interlock" field is used for hardware exclusion;
 *	other fields are modified with normal instructions after
 *	acquiring the interlock bit.
 */
typedef struct {
	decl_simple_lock_data(,interlock) /* "hardware" interlock field */
	volatile unsigned int
		read_count:16,	/* No. of accepted readers */
		want_upgrade:1,	/* Read-to-write upgrade waiting */
		want_write:1,	/* Writer is waiting, or
				   locked for write */
		waiting:1,	/* Someone is sleeping on lock */
		can_sleep:1;	/* Can attempts to lock go to sleep? */
#if     ETAP_LOCK_TRACE
	union {		/* Must be overlaid on the event_tablep */
	    struct event_table_chain event_table_chain;
	    struct {
		event_table_t event_tablep;	/* ptr to event table entry */
		start_data_node_t start_list;	/* linked list of start times
						   and pcs */
	    } s;
	} u;
#endif 	/* ETAP_LOCK_TRACE */
#if     ETAP_LOCK_ACCUMULATE
       	cbuff_entry_t	cbuff_write;	/* write cumulative buffer entry      */
	cbuff_entry_t	cbuff_read;	/* read  cumulative buffer entry      */
#endif 	/* ETAP_LOCK_ACCUMULATE */
} lock_t;

/* Sleep locks must work even if no multiprocessing */

/*
 * Complex lock operations
 */

extern void	lock_init		(lock_t*,
					 boolean_t,
					 etap_event_t,
					 etap_event_t);

extern void	lock_write		(lock_t*);
extern void	lock_read		(lock_t*);
extern void	lock_done		(lock_t*);
extern void	lock_write_to_read	(lock_t*);

#define	lock_read_done(l)		lock_done(l)
#define	lock_write_done(l)		lock_done(l)

extern boolean_t lock_read_to_write	(lock_t*);  /* vm_map is only user */

#endif	/* _KERN_LOCK_H_ */
