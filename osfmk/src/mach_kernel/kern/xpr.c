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
 * Revision 2.9.5.2  92/03/03  16:20:57  jeffreyh
 * 	Fix Log.
 * 	[92/02/24  13:24:44  jeffreyh]
 * 
 * Revision 2.9.5.1  92/02/18  19:13:03  jeffreyh
 * 	Added an xpr_search function to which you can give
 * 	a selection function.
 * 	[92/02/11  08:13:23  bernadat]
 * 
 * Revision 2.9.4.1  92/02/13  18:53:47  jeffreyh
 * 	Added an xpr_search function to which you can give
 * 	a selection function.
 * 	[92/02/11  08:13:23  bernadat]
 * 
 * Revision 2.9.3.1  92/02/11  17:19:59  jeffreyh
 * 	Added an xpr_search function to which you can give
 * 	a selection function.
 * 	[92/02/11  08:13:23  bernadat]
 * 
 * Revision 2.9.2.1  92/02/11  08:13:23  bernadat
 * 	Added an xpr_search function to which you can give
 * 	a selection function.
 * 
 * 
 * Revision 2.9  91/10/09  16:11:50  af
 * 	Removed xpr_save.  Modified xpr_dump to make it useful
 * 	for dumping xpr buffers in user space tasks.
 * 	[91/09/20            rpd]
 * 
 * 	Turned on xprenable by default.  xprbootstrap now preserves
 * 	the original contents of the buffer if xprenable is off.
 * 	[91/09/18            rpd]
 * 
 * Revision 2.8  91/08/28  11:14:56  jsb
 * 	Fixed xprbootstrap to zero the allocate memory.
 * 	[91/08/18            rpd]
 * 
 * Revision 2.7  91/05/18  14:34:37  rpd
 * 	Added xprenable and other minor changes so that the xpr buffer
 * 	may be examined after a spontaneous reboot.
 * 	[91/05/03            rpd]
 * 	Fixed the initialization check in xpr.
 * 	Fixed xpr_dump.
 * 	[91/04/02            rpd]
 * 
 * Revision 2.6  91/05/14  16:50:09  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/03/16  14:53:24  rpd
 * 	Updated for new kmem_alloc interface.
 * 	[91/03/03            rpd]
 * 
 * Revision 2.4  91/02/05  17:31:13  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:21:17  mrt]
 * 
 * Revision 2.3  90/09/09  14:33:04  rpd
 * 	Use decl_simple_lock_data.
 * 	[90/08/30            rpd]
 * 
 * Revision 2.2  89/11/29  14:09:21  af
 * 	Added xpr_dump() to print on console the content of the buffer,
 * 	only valid for KDB usage.
 * 	[89/11/12            af]
 * 
 * 	MACH_KERNEL: include sys/cpu_number.h instead of machine/cpu.h.
 * 	Clean up comments.
 * 	[88/12/19            dbg]
 * 
 * Revision 2.1  89/08/03  15:49:11  rwd
 * Created.
 * 
 * Revision 2.2  88/12/19  02:48:30  mwyoung
 * 	Fix include file references.
 * 	[88/11/22  02:17:01  mwyoung]
 * 	
 * 	Separate initialization into two phases.
 * 	[88/11/22  01:13:11  mwyoung]
 * 
 *  6-Jan-88  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Eliminate use of arg6 in order to allow a more shapely event structure.
 *
 * 30-Dec-87  David Golub (dbg) at Carnegie-Mellon University
 *	Delinted.
 *
 *  7-Dec-87  Richard Sanzi (sanzi) at Carnegie-Mellon University
 *	Added xpr_save() routine.
 *
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
#include <mach_kdb.h>
/*
 * xpr silent tracing circular buffer.
 */

#include <cpus.h>

#include <mach/machine/vm_types.h>
#include <kern/xpr.h>
#include <kern/lock.h>
#include <kern/spl.h>
#include <kern/cpu_number.h>
#include <kern/misc_protos.h>
#include <vm/vm_kern.h>
#include <string.h>

/*
 *	After a spontaneous reboot, it is desirable to look
 *	at the old xpr buffer.  Assuming xprbootstrap allocates
 *	the buffer in the same place in physical memory and
 *	the reboot doesn't clear memory, this should work.
 *	xprptr will be reset, but the saved value should be OK.
 *	Just set xprenable false so the buffer isn't overwritten.
 */

decl_simple_lock_data(,xprlock)
boolean_t xprenable = TRUE;	/* Enable xpr tracing */
int nxprbufs = 0;	/* Number of contiguous xprbufs allocated */
int xprflags = 0;	/* Bit mask of xpr flags enabled */
struct xprbuf *xprbase;	/* Pointer to circular buffer nxprbufs*sizeof(xprbuf)*/
struct xprbuf *xprptr;	/* Currently allocated xprbuf */
struct xprbuf *xprlast;	/* Pointer to end of circular buffer */

void
xpr(
	char	*msg,
	long	arg1,
	long	arg2,
	long	arg3,
	long	arg4,
	long	arg5)
{
	spl_t s;
	register struct xprbuf *x;

	/* If we aren't initialized, ignore trace request */
	if (!xprenable || (xprptr == 0))
		return;
	/* Guard against all interrupts and allocate next buffer. */

	s = splhigh();
	simple_lock(&xprlock);
	x = xprptr++;
	if (xprptr >= xprlast) {
		/* wrap around */
		xprptr = xprbase;
	}
	/* Save xprptr in allocated memory. */
	*(struct xprbuf **)xprlast = xprptr;
	simple_unlock(&xprlock);
	x->timestamp = XPR_TIMESTAMP;
	splx(s);
	x->msg = msg;
	x->arg1 = arg1;
	x->arg2 = arg2;
	x->arg3 = arg3;
	x->arg4 = arg4;
	x->arg5 = arg5;
	mp_disable_preemption();
	x->cpuinfo = cpu_number();
	mp_enable_preemption();
}

void 
xprbootstrap(void)
{
	vm_offset_t	addr;
	vm_size_t	size;
	kern_return_t	kr;

	simple_lock_init(&xprlock, ETAP_MISC_XPR);
	if (nxprbufs == 0)
		return;	/* assume XPR support not desired */

	/* leave room at the end for a saved copy of xprptr */
	size = nxprbufs * sizeof(struct xprbuf) + sizeof xprptr;

	kr = kmem_alloc_wired(kernel_map, &addr, size);
	if (kr != KERN_SUCCESS)
		panic("xprbootstrap");

	if (xprenable) {
		/*
		 *	If xprenable is set (the default) then we zero
		 *	the buffer so xpr_dump doesn't encounter bad pointers.
		 *	If xprenable isn't set, then we preserve
		 *	the original contents of the buffer.  This is useful
		 *	if memory survives reboots, so xpr_dump can show
		 *	the previous buffer contents.
		 */

		(void) memset((void *) addr, 0, size);
	}

	xprbase = (struct xprbuf *) addr;
	xprlast = &xprbase[nxprbufs];
	xprptr = xprbase;	/* setting xprptr enables tracing */
}

int		xprinitial = 0;

void
xprinit(void)
{
	xprflags |= xprinitial;
}

#if	MACH_KDB
#include <ddb/db_output.h>

/*
 * Prototypes for functions called from the debugger
 */
void
xpr_dump(
	struct xprbuf	*base,
	int		nbufs);

void
xpr_search(
	int	arg_index,
	int	value);

extern jmp_buf_t *db_recover;

/*
 *	Print current content of xpr buffers (KDB's sake)
 *	Use stack order to make it understandable.
 *
 *	Called as "!xpr_dump" this dumps the kernel's xpr buffer.
 *	Called with arguments, it can dump xpr buffers in user tasks,
 *	assuming they use the same format as the kernel.
 */
void
xpr_dump(
	struct xprbuf	*base,
	int		nbufs)
{
	jmp_buf_t db_jmpbuf;
	jmp_buf_t *prev;
	struct xprbuf *last, *ptr;
	register struct xprbuf *x;
	int i;
	spl_t s;

	if (base == 0) {
		base = xprbase;
		nbufs = nxprbufs;
	}

	if (nbufs == 0)
		return;

	if (base == xprbase) {
		s = splhigh();
		simple_lock(&xprlock);
	}

	last = base + nbufs;
	ptr = * (struct xprbuf **) last;

	prev = db_recover;
	if (_setjmp(db_recover = &db_jmpbuf) == 0)
	    for (x = ptr, i = 0; i < nbufs; i++) {
		if (--x < base)
			x = last - 1;

		if (x->msg == 0)
			break;

		db_printf("<%d:%x:%x> ", x - base, x->cpuinfo, x->timestamp);
		db_printf(x->msg, x->arg1,x->arg2,x->arg3,x->arg4,x->arg5);
	    }
	db_recover = prev;

	if (base == xprbase) {
		simple_unlock(&xprlock);
		splx(s);
	}
}

/*
 * dump xpr table with a selection criteria.
 * argument number "arg_index" must equal "value"
 */

void
xpr_search(
	int	arg_index,
	int	value)
{
	jmp_buf_t db_jmpbuf;
	jmp_buf_t *prev;
	register struct xprbuf *x;
	spl_t s;
	int n;

	if (!nxprbufs)
		return;

	n = nxprbufs;

	s = splhigh();
	simple_lock(&xprlock);

	prev = db_recover;
	if (_setjmp(db_recover = &db_jmpbuf) == 0)
  	    for (x = *(struct xprbuf **)xprlast ; n--; ) {
		if (--x < xprbase)
			x = xprlast - 1;

		if (x->msg == 0) {
			break;
		}

		if (*((&x->arg1)+arg_index) != value)
			continue;

		db_printf("<%d:%d:%x> ", x - xprbase,
			  x->cpuinfo, x->timestamp);
		db_printf(x->msg, x->arg1,x->arg2,x->arg3,x->arg4,x->arg5);
	    }
	db_recover = prev;

	simple_unlock(&xprlock);
	splx(s);
}
#endif	/* MACH_KDB */
