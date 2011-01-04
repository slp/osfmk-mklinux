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
 * Revision 2.12  92/07/20  13:32:18  cmaeda
 * 	Added private version of set_ras_address for fast_tas support.
 * 	[92/05/11  14:31:52  cmaeda]
 * 
 * Revision 2.11  92/05/05  10:03:46  danner
 * 	For merge purposes, backed-out the unstable stuff.
 * 	[92/05/04  11:12:01  af]
 * 
 * 	Now we can page an object across partitions.
 * 	Initial rough ideas about automatically extending
 * 	paging space.
 * 	[92/03/11  02:23:58  af]
 * 
 * Revision 2.10  92/03/06  13:58:48  rpd
 * 	Fixed pager_dealloc_page calls in pager_dealloc (from af).
 * 	Removed chatty printfs.
 * 	[92/03/06            rpd]
 * 
 * Revision 2.9  92/03/05  15:58:35  rpd
 * 	Changed PAGEMAP_ENTRIES from 128 to 64.  From af.
 * 	[92/03/05            rpd]
 * 
 * Revision 2.8  92/03/03  12:12:04  rpd
 * 	Changed to catch exception messages and handle bootstrap requests.
 * 	Added partition_init.
 * 	[92/03/03            rpd]
 * 
 * Revision 2.7  92/02/25  11:22:38  elf
 * 	Accept creation of objects bigger than any one partition, in
 * 	anticipation of the code that will page across partitions.
 * 	Since we are at it, also proceed with no paging partitions:
 * 	rely on killing unlucky objects on pageouts.
 * 	[92/02/25            af]
 * 
 * Revision 2.6  92/02/23  23:00:31  elf
 * 	Copyright updated, corrected history.
 * 	[92/02/23            elf]
 * 
 * Revision 2.5  92/02/23  22:25:35  elf
 * 	Improved handling of big objects, fixed a deadlock in
 * 	object relocation, improved printouts.
 * 	Now only crash if out of memory, otherwise use the old
 * 	code that just marked the object as in-error.
 * 	[92/02/23  13:25:49  af]
 * 
 * 	As per jsb instructions, removed all NORMA conditionals.
 * 	Rename port names to odd values, a trivial heuristic that
 * 	makes name conflicts even more unlikely.
 * 	[92/02/22            af]
 * 
 * 	Refined the port name conflict problem.  Instead of renaming
 * 	ports that we send to, just set aside the memory that we cannot
 * 	use.  When objects get deleted put back the memory in the system.
 * 	[92/02/21            af]
 * 
 * 	Added renaming of request and name ports (from af).
 * 	[92/02/21            danner]
 * 
 * 	Many changes. Now supports adding/removing paging files, it does
 * 	not immediately panic if a paging file fills up but relocates the
 * 	object elsewhere, it uses the precious attribute in data_supply
 * 	to reduce paging space usage (under USE_PRECIOUS conditional,
 * 	enabled).
 * 	[92/02/19  17:29:54  af]
 * 
 * 	Two mods: changed bitmap ops to work one int at a time rather
 * 	than one byte at a time.  This helps under load, e.g. when the
 * 	paging file is large and busy. Second mod to use port-to-pointer
 * 	casting in lookups, rather than hash+list searching.  This not
 * 	only helps under load (I see >600 objects on my pmax) but also
 * 	increases parallelism a little.
 * 	Shrunk the code size by one page in the process.
 * 	[92/02/14  01:44:23  af]
 * 
 * Revision 2.4  92/01/23  15:19:41  rpd
 * 	Changed to not include mig server interfaces.
 * 	[92/01/23            rpd]
 * 
 * Revision 2.3  92/01/14  16:43:14  rpd
 * 	Moved mach/default_pager_object.defs to mach/default_pager.defs.
 * 	Revised default_pager_info etc. for their new definitions.
 * 	Removed (now) unnecessary #define's to rename kernel functions.
 * 	[92/01/13            rpd]
 * 	Added page_size to default_pager_info.
 * 	Added default_pager_object_pages.
 * 	[92/01/03            rpd]
 * 
 * 	Updated to handle name ports from memory_object_create.
 * 	Changed to remember the name ports associated with objects.
 * 	Changed default_pager_objects to return the name ports.
 * 	[91/12/28            rpd]
 * 
 * 	Added default_pager_objects.
 * 	[91/12/15            rpd]
 * 
 * Revision 2.2  92/01/03  19:56:21  dbg
 * 	Simplify locking.
 * 	[91/10/02            dbg]
 * 
 * 	Convert to run outside of kernel.
 * 	[91/09/04            dbg]
 * 
 * Revision 2.17  91/08/29  13:44:27  jsb
 * 	A couple quick changes for NORMA_VM. Will be fixed later.
 * 
 * Revision 2.16  91/08/28  16:59:29  jsb
 * 	Fixed the default values of default_pager_internal_count and
 * 	default_pager_external_count.
 * 	[91/08/28            rpd]
 * 
 * Revision 2.15  91/08/28  11:09:32  jsb
 * 	Added seqnos_memory_object_change_completed.
 * 	From dlb: use memory_object_data_supply for pagein when buffer is
 * 	going to be deallocated.
 * 	From me: don't use data_supply under NORMA_VM (will be fixed).
 * 	[91/08/26  14:30:07  jsb]
 * 
 * 	Changed to process requests in parallel when possible.
 * 
 * 	Don't bother keeping track of mscount.
 * 	[91/08/16            rpd]
 * 	Added default_pager_info.
 * 	[91/08/15            rpd]
 * 
 * 	Added sequence numbers to the memory object interface.
 * 	Changed to use no-senders notifications.
 * 	Changed to keep track of port rights and not use mach_port_destroy.
 * 	Added dummy supply-completed and data-return stubs.
 * 	[91/08/13            rpd]
 * 
 * Revision 2.14  91/05/18  14:28:32  rpd
 * 	Don't give privileges to threads handling external objects.
 * 	[91/04/06            rpd]
 * 	Enhanced to use multiple threads, for performance and to avoid
 * 	a deadlock caused by default_pager_object_create.
 * 	Added locking to partitions.
 * 	Added locking to pager_port_hashtable.
 * 	Changed pager_port_hash to something reasonable.
 * 	[91/04/03            rpd]
 * 
 * Revision 2.13  91/05/14  15:21:41  mrt
 * 	Correcting copyright
 * 
 * Revision 2.12  91/03/16  14:41:26  rpd
 * 	Updated for new kmem_alloc interface.
 * 	Fixed memory_object_create to zero the new pager structure.
 * 	[91/03/03            rpd]
 * 	Removed thread_swappable.
 * 	[91/01/18            rpd]
 * 
 * Revision 2.11  91/02/05  17:00:49  mrt
 * 	Changed to new copyright
 * 	[91/01/28  14:54:31  mrt]
 * 
 * Revision 2.10  90/09/09  14:31:01  rpd
 * 	Use decl_simple_lock_data.
 * 	[90/08/30            rpd]
 * 
 * Revision 2.9  90/08/27  21:44:51  dbg
 * 	Add definitions of NBBY, howmany.
 * 	[90/07/16            dbg]
 * 
 * Revision 2.8  90/06/02  14:45:22  rpd
 * 	Changed default_pager_object_create so the out argument
 * 	is a poly send right.
 * 	[90/05/03            rpd]
 * 	Removed references to keep_wired_memory.
 * 	[90/04/29            rpd]
 * 	Converted to new IPC.
 * 	Removed data-request queue.
 * 	[90/03/26  21:30:57  rpd]
 * 
 * Revision 2.7  90/03/14  21:09:58  rwd
 * 	Call default_pager_object_server and add
 * 	default_pager_object_create
 * 	[90/01/22            rwd]
 * 
 * Revision 2.6  90/01/11  11:41:08  dbg
 * 	Use bootstrap-task print routines.
 * 	[89/12/20            dbg]
 * 
 * 	De-lint.
 * 	[89/12/06            dbg]
 * 
 * Revision 2.5  89/12/08  19:52:03  rwd
 * 	Turn off CHECKSUM
 * 	[89/12/06            rwd]
 * 
 * Revision 2.4  89/10/23  12:01:54  dbg
 * 	Change pager_read_offset and pager_write_offset to return block
 * 	number as function result.  default_read()'s caller must now
 * 	deallocate data if not the same as the data buffer passed in.
 * 	Add register declarations and clean up loops a bit.
 * 	[89/10/19            dbg]
 * 
 * 	Oops - nothing like having your debugging code introduce bugs...
 * 	[89/10/17            dbg]
 * 
 * Revision 2.3  89/10/16  15:21:59  rwd
 * 	debugging: checksum pages in each object.
 * 	[89/10/04            dbg]
 * 
 * Revision 2.2  89/09/08  11:22:06  dbg
 * 	Wait for default_partition to be set.
 * 	[89/09/01            dbg]
 * 
 * 	Modified to call outside routines for read and write.
 * 	Removed disk structure.  Added part_create.
 * 	Reorganized code.
 * 	[89/07/11            dbg]
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
 * 	Default pager.
 * 		Threads management.
 *		Requests handling.
 */

#include "default_pager_internal.h"

char	my_name[] = "(default pager): ";
struct mutex	dprintf_lock;
#if	DEFAULT_PAGER_DEBUG
int	debug_mask = 0;
#endif	/* DEFAULT_PAGER_DEBUG */

/*
 * Use 16 Kbyte stacks instead of the default 64K.
 * Use 4 Kbyte waiting stacks instead of the default 8K.
 */

vm_size_t	cthread_stack_size = 16 *1024;
extern vm_size_t cthread_wait_stack_size;

int		vm_page_mask;
int		vm_page_shift;

int 		norma_mk;

boolean_t	verbose;

task_port_t default_pager_self;		/* Our task port. */

mach_port_t default_pager_default_set;	/* Port set for "default" thread. */

mach_port_t default_pager_default_port;	/* Port for memory_object_create. */
thread_port_t default_pager_default_thread;	/* Thread for default_port. */

mach_port_t default_pager_internal_set;	/* Port set for internal objects. */
mach_port_t default_pager_external_set;	/* Port set for external objects. */

#if	PARALLEL
/* determine number of threads at run time */
#define DEFAULT_PAGER_INTERNAL_COUNT	(0)
#else	/* PARALLEL */
#define	DEFAULT_PAGER_INTERNAL_COUNT	(1)
#endif	/* PARALLEL */

/* Memory created by default_pager_object_create should mostly be resident. */
#define DEFAULT_PAGER_EXTERNAL_COUNT	(2)

unsigned int	default_pager_internal_count = DEFAULT_PAGER_INTERNAL_COUNT;
/* Number of "internal" threads. */
unsigned int	default_pager_external_count = DEFAULT_PAGER_EXTERNAL_COUNT;
/* Number of "external" threads. */

/*
 * Forward declarations.
 */
boolean_t default_pager_notify_server(mach_msg_header_t *,
				      mach_msg_header_t *);
boolean_t default_pager_demux_object(mach_msg_header_t *,
				     mach_msg_header_t *);
boolean_t default_pager_demux_default(mach_msg_header_t *,
				      mach_msg_header_t *);
void	default_pager_thread_privileges(void);
default_pager_thread_t *start_default_pager_thread(int, boolean_t);
void	default_pager(void);
void	default_pager_thread(void *);
void	default_pager_initialize(mach_port_t);
void	default_pager_set_policy(mach_port_t);
boolean_t	dp_parse_argument(char *);	/* forward; */
unsigned int	d_to_i(char *);			/* forward; */

int
dp_thread_id(void)
{
	default_pager_thread_t	*dpt;

	dpt = (default_pager_thread_t *) cthread_data(cthread_self());
	if (dpt != NULL)
		return dpt->dpt_id;
	else
		return -1;
}

boolean_t
default_pager_notify_server(
	mach_msg_header_t *in,
	mach_msg_header_t *out)
{
	mach_no_senders_notification_t *n;

	n = (mach_no_senders_notification_t *) in;

	/* 
	 * The only send-once rights we create are for
	 * receiving no-more-senders notifications.
	 * Hence, if we receive a message directed to
	 * a send-once right, we can assume it is
	 * a genuine no-senders notification from the kernel.
	 */

	if ((n->not_header.msgh_bits != 
	     MACH_MSGH_BITS(0, MACH_MSG_TYPE_PORT_SEND_ONCE))
	    || (n->not_header.msgh_id != MACH_NOTIFY_NO_SENDERS))
		return FALSE;

	assert(n->not_header.msgh_size
		== sizeof * n - sizeof(mach_msg_format_0_trailer_t));
	assert(n->not_header.msgh_remote_port == MACH_PORT_NULL);

	default_pager_no_senders(n->not_header.msgh_local_port,
				 n->trailer.msgh_seqno, n->not_count);

	out->msgh_remote_port = MACH_PORT_NULL;
	return TRUE;
}

mach_msg_size_t default_pager_msg_size = 256;

boolean_t
default_pager_demux_object(
	mach_msg_header_t *in,
	mach_msg_header_t *out)
{
	/*
	 * We receive memory_object_data_initialize messages in
	 * the memory_object_default interface.
	 */

	return (seqnos_memory_object_server(in, out)
		|| seqnos_memory_object_default_server(in, out)
		|| default_pager_notify_server(in, out));
}

boolean_t
default_pager_demux_default(
	mach_msg_header_t *in,
	mach_msg_header_t *out)
{
	return (seqnos_memory_object_default_server(in, out)
		|| default_pager_object_server(in, out)
		|| device_reply_server(in, out));
}

/*
 * We use multiple threads, for two reasons.
 *
 * First, memory objects created by default_pager_object_create
 * are "external", instead of "internal".  This means the kernel
 * sends data (memory_object_data_return) to the object pageable.
 * To prevent deadlocks, the external and internal objects must
 * be managed by different threads.
 *
 * Second, the default pager uses synchronous IO operations.
 * Spreading requests across multiple threads should
 * recover some of the performance loss from synchronous IO.
 *
 * We have 3+ threads.
 * One receives memory_object_create and
 * default_pager_object_create requests.
 * One or more manage internal objects.
 * One or more manage external objects.
 */

extern void	mach_msg_destroy(mach_msg_header_t *);

mach_msg_return_t
mach_msg_server(
	boolean_t		(*demux)(mach_msg_header_t *,
					 mach_msg_header_t *),
	mach_msg_size_t		max_size,
	mach_port_t		rcv_name,
	mach_msg_options_t	server_options)
{
	mig_reply_error_t 	*bufRequest, *bufReply, *bufTemp;
	mach_msg_return_t 	mr;
	mach_msg_options_t	options;
	static char here[] =	"mach_msg_server";

	bufRequest = (mig_reply_error_t *)kalloc(max_size + MAX_TRAILER_SIZE);
	if (bufRequest == 0)
		return KERN_RESOURCE_SHORTAGE;
	bufReply = (mig_reply_error_t *)kalloc(max_size + MAX_TRAILER_SIZE);
	if (bufReply == 0)
		return KERN_RESOURCE_SHORTAGE;

	for (;;) {
	    get_request:
		mr = mach_msg(&bufRequest->Head, MACH_RCV_MSG | server_options,
			      0, max_size, rcv_name, MACH_MSG_TIMEOUT_NONE,
			      MACH_PORT_NULL);
		while (mr == MACH_MSG_SUCCESS) {
			/* we have a request message */

			(void) (*demux)(&bufRequest->Head, &bufReply->Head);

			if (!(bufReply->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX)
			    && bufReply->RetCode != KERN_SUCCESS) {
				if (bufReply->RetCode == MIG_NO_REPLY)
					goto get_request;

				/*
				 * Don't destroy the reply port right,
				 * so we can send an error message
				 */
				bufRequest->Head.msgh_remote_port =
					MACH_PORT_NULL;
				mach_msg_destroy(&bufRequest->Head);
			}

			if (bufReply->Head.msgh_remote_port == MACH_PORT_NULL) {
				/* no reply port, so destroy the reply */
				if (bufReply->Head.msgh_bits &
				    MACH_MSGH_BITS_COMPLEX)
					mach_msg_destroy(&bufReply->Head);

				goto get_request;
			}

			/* send reply and get next request */

			bufTemp = bufRequest;
			bufRequest = bufReply;
			bufReply = bufTemp;

			/*
			 * We don't want to block indefinitely because the
			 * client isn't receiving messages from the reply port.
			 * If we have a send-once right for the reply port,
			 * then this isn't a concern because the send won't
			 * block.
			 * If we have a send right, we need to use
			 * MACH_SEND_TIMEOUT.
			 * To avoid falling off the kernel's fast RPC path
			 * unnecessarily, we only supply MACH_SEND_TIMEOUT when
			 * absolutely necessary.
			 */

			options = MACH_SEND_MSG | MACH_RCV_MSG | server_options;
			if (MACH_MSGH_BITS_REMOTE(bufRequest->Head.msgh_bits)
			    != MACH_MSG_TYPE_MOVE_SEND_ONCE) {
				options |= MACH_SEND_TIMEOUT;
			}
			mr = mach_msg(&bufRequest->Head, options,
				      bufRequest->Head.msgh_size, max_size,
				      rcv_name, MACH_MSG_TIMEOUT_NONE,
				      MACH_PORT_NULL);
		}

		/* a message error occurred */

		switch (mr) {
		    case MACH_SEND_INVALID_DEST:
		    case MACH_SEND_TIMED_OUT:
			/* the reply can't be delivered, so destroy it */
			mach_msg_destroy(&bufRequest->Head);
			break;

		    case MACH_RCV_TOO_LARGE:
			/* the kernel destroyed the request */
			break;

		    default:
			dprintf(("mach_msg_overwrite_trap returned 0x%x %s\n",
				 mr, mach_error_string(mr)));
			Panic("mach_msg failed");
			/* should only happen if the server is buggy */
			kfree((char *) bufRequest, max_size + MAX_TRAILER_SIZE);
			kfree((char *) bufReply, max_size + MAX_TRAILER_SIZE);
			return mr;
		}
	}
}

void
default_pager_thread_privileges(void)
{
	/*
	 * Set thread privileges.
	 */

	cthread_wire();		/* attach kernel thread to cthread */
	wire_thread();		/* grab a kernel stack and memory allocation
				   privileges */

}

void
default_pager_thread(
	void *arg)
{
	default_pager_thread_t	*dpt;
	mach_port_t		pset;
	kern_return_t		kr;
	static char		here[] = "default_pager_thread";
	mach_msg_options_t	server_options;

	dpt = (default_pager_thread_t *)arg;
	cthread_set_data(cthread_self(), (char *) dpt);

	/*
	 * Threads handling external objects cannot have
	 * privileges.  Otherwise a burst of data-requests for an
	 * external object could empty the free-page queue,
	 * because the fault code only reserves real pages for
	 * requests sent to internal objects.
	 */

	if (dpt->dpt_internal) {
		default_pager_thread_privileges();
		pset = default_pager_internal_set;
	} else {
		pset = default_pager_external_set;
	}

	dpt->dpt_initialized_p = TRUE; /* Ready for requests.  */

	server_options = MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_SEQNO);
	for (;;) {
		kr = mach_msg_server(default_pager_demux_object,
				     default_pager_msg_size,
				     pset,
				     server_options);
		Panic("mach_msg_server failed");
	}
}


default_pager_thread_t *
start_default_pager_thread(
	int		id,
	boolean_t	internal)
{
	default_pager_thread_t	*dpt;
	kern_return_t		kr;
	static char		here[] = "start_default_pager_thread";

	dpt = (default_pager_thread_t *)kalloc(sizeof (default_pager_thread_t));
	if (dpt == 0)
		Panic("alloc pager thread");

	dpt->dpt_internal = internal;
	dpt->dpt_id = id;
	dpt->dpt_initialized_p = FALSE;

	kr = vm_allocate(default_pager_self, &dpt->dpt_buffer,
			 vm_page_size, TRUE);
	if (kr != KERN_SUCCESS)
		Panic("alloc thread buffer");
	wire_memory(dpt->dpt_buffer, vm_page_size,
		    VM_PROT_READ | VM_PROT_WRITE);

	dpt->dpt_thread = cthread_fork((cthread_fn_t) default_pager_thread,
				       (void *) dpt);
	return dpt;
}

extern int vstruct_def_clshift;

void
default_pager_initialize(
	mach_port_t host_port)
{
	kern_return_t		kr;
	static char		here[] = "default_pager_initialize";

	/* 
	 * Initial thread and task ports.
	 */
	default_pager_self = mach_task_self();
	default_pager_default_thread = mach_thread_self();

	PRINTF_LOCK_INIT();

	/*
	 * Make ourselves unswappable.
	 */
	kr = task_swappable(default_pager_host_port, default_pager_self, FALSE);
	if (kr != KERN_SUCCESS)
		dprintf(("task_swappable failed 0x%x %s\n",
			 kr, mach_error_string(kr)));

	/*
	 * Exported DMM port.
	 */
	kr = mach_port_allocate(default_pager_self, MACH_PORT_RIGHT_RECEIVE,
				&default_pager_default_port);
	if (kr != KERN_SUCCESS)
		Panic("default port");

	/*
	 * Port sets.
	 */
	kr = mach_port_allocate(default_pager_self, MACH_PORT_RIGHT_PORT_SET,
				&default_pager_internal_set);
	if (kr != KERN_SUCCESS)
		Panic("internal set");

	kr = mach_port_allocate(default_pager_self, MACH_PORT_RIGHT_PORT_SET,
				&default_pager_external_set);
	if (kr != KERN_SUCCESS)
		Panic("external set");

	/*
	 * Export pager interfaces.
	 */
#ifdef	USER_PAGER
	if ((kr = netname_check_in(name_server_port, "UserPager",
				   default_pager_self,
				   default_pager_default_port))
	    != KERN_SUCCESS) {
		dprintf(("netname_check_in returned 0x%x %s\n",
			 kr, mach_error_string(kr)));
		exit(1);
	}
#else	/* USER_PAGER */
	{
		int clsize;
		memory_object_t DMM;

		/* get a send right for vm_set_default_memory_manager */
		kr = mach_port_insert_right(default_pager_self,
					    default_pager_default_port,
					    default_pager_default_port,
					    MACH_MSG_TYPE_MAKE_SEND);
		DMM = default_pager_default_port;
		clsize = (vm_page_size << vstruct_def_clshift);

		kr = host_default_memory_manager(host_port, &DMM, clsize);
		if ((kr != KERN_SUCCESS) || (DMM != MACH_PORT_NULL))
			Panic("default memory manager");

		/* release the extra send right */
		(void) mach_port_mod_refs(default_pager_self,
					  default_pager_default_port,
					  MACH_PORT_RIGHT_SEND,
					  -1);
	}
#endif	/* USER_PAGER */

	kr = mach_port_allocate(default_pager_self, MACH_PORT_RIGHT_PORT_SET,
				&default_pager_default_set);
	if (kr != KERN_SUCCESS)
		Panic("default set");

	kr = mach_port_move_member(default_pager_self,
				   default_pager_default_port,
				   default_pager_default_set);
	if (kr != KERN_SUCCESS)
		Panic("set up default");

	/*
	 * Arrange for wiring privileges.
	 */
	wire_setup(host_port);

	/*
	 * Find out how many CPUs we have, to determine the number
	 * of threads to create.
	 */
	if (default_pager_internal_count == 0) {
		host_basic_info_data_t h_info;
		mach_msg_type_number_t h_info_count;

		h_info_count = HOST_BASIC_INFO_COUNT;
		(void) host_info(host_port, HOST_BASIC_INFO,
				(host_info_t) &h_info, &h_info_count);

		/*
		 * Random computation to get more parallelism on
		 * multiprocessors.
		 */
		default_pager_internal_count = ((h_info.avail_cpus > 32)
						? 32
						: h_info.avail_cpus) / 4 + 3;
	}

	/*
	 * Vm variables.
	 */
	vm_page_mask = vm_page_size - 1;
	vm_page_shift = log2(vm_page_size);

	/*
	 * List of all vstructs.
	 */
	VSL_LOCK_INIT();
	queue_init(&vstruct_list.vsl_queue);
	queue_init(&vstruct_list.vsl_leak_queue);
	vstruct_list.vsl_count = 0;

	VSTATS_LOCK_INIT(&global_stats.gs_lock);

	bs_initialize();
}


void
default_pager_set_policy(
	mach_port_t master_host_port)
{
	host_priority_info_data_t	host_pri_info;
	mach_port_t		default_processor_set_name;
	mach_port_t		default_processor_set;
	policy_rr_base_data_t	rr_base;
	policy_rr_limit_data_t	rr_limit;
	kern_return_t		r;
	mach_msg_type_number_t	count;
	static char here[] = "default_pager_set_policy";

	count = HOST_PRIORITY_INFO_COUNT;
	r = host_info(mach_host_self(),
		      HOST_PRIORITY_INFO,
		      (host_info_t) &host_pri_info,
		      &count);
	if (r != KERN_SUCCESS)
		dprintf(("Could not get host priority info.  Error = 0x%x\n",
			 r));

	rr_base.quantum = 0;
	rr_base.base_priority = host_pri_info.system_priority;
	rr_limit.max_priority = host_pri_info.system_priority;

	(void)processor_set_default(mach_host_self(),
				    &default_processor_set_name);
	(void)host_processor_set_priv(master_host_port,
				      default_processor_set_name,
				      &default_processor_set);

	r = task_set_policy(default_pager_self, default_processor_set,
			    POLICY_RR,
			    (policy_base_t) & rr_base, POLICY_RR_BASE_COUNT,
			    (policy_limit_t) & rr_limit, POLICY_RR_LIMIT_COUNT,
			    TRUE);
	if (r != KERN_SUCCESS)
		dprintf(("task_set_policy returned 0x%x %s\n",
			 r, mach_error_string(r)));
}


/*
 * Initialize and Run the default pager
 */
void
default_pager(void)
{
	int			i, id;
	static char		here[] = "default_pager";
	mach_msg_options_t 	server_options;
	default_pager_thread_t	dpt;
	default_pager_thread_t	**dpt_array;

	default_pager_thread_privileges();

	/*
	 * Wire down code, data, stack
	 */
	wire_all_memory();

	/*
	 * Give me space for the thread array and zero it.
	 */
	i = default_pager_internal_count + default_pager_external_count + 1;
	dpt_array = (default_pager_thread_t **)
	    kalloc(i * sizeof(default_pager_thread_t *));
	memset(dpt_array, 0, i * sizeof(default_pager_thread_t *));

	/* Setup my thread structure.  */
	id = 0;
	dpt.dpt_thread = cthread_self();
	dpt.dpt_buffer = 0;
	dpt.dpt_internal = FALSE;
	dpt.dpt_id = id++;
	dpt.dpt_initialized_p = TRUE;
	cthread_set_data(cthread_self(), (char *) &dpt);
	dpt_array[0] = &dpt;

	/*
	 * Now we create the threads that will actually
	 * manage objects.
	 */

	for (i = 0; i < default_pager_internal_count; i++) {
		dpt_array[id] = start_default_pager_thread(id, TRUE);
		id++;
	 }

	for (i = 0; i < default_pager_external_count; i++) {
		dpt_array[id] = start_default_pager_thread(id, FALSE);
		id++;
	}

	/* Is everybody ready?  */
	for (i = 0; i < id; i++)
	    while (!dpt_array[i])
		cthread_yield();

	/* Tell the bootstrap process to go ahead.  */
	bootstrap_completed(bootstrap_port, mach_task_self());

	/* Start servicing requests.  */
	server_options = MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_SEQNO);
	for (;;) {
		mach_msg_server(default_pager_demux_default,
				default_pager_msg_size,
				default_pager_default_set,
				server_options);
		Panic("default server");
	}
}

kern_return_t
default_pager_info(
	mach_port_t		pager,
	default_pager_info_t	*infop)
{
	vm_size_t	pages_total, pages_free;

	if (pager != default_pager_default_port)
		return KERN_INVALID_ARGUMENT;

	bs_global_info(&pages_total, &pages_free);

	infop->dpi_total_space = ptoa(pages_total);
	infop->dpi_free_space = ptoa(pages_free);
	infop->dpi_page_size = vm_page_size;

	return KERN_SUCCESS;
}

/* simple utility: only works for 2^n */
int
log2(
	unsigned int n)
{
	register int	i = 0;

	while ((n & 1) == 0) {
		i++;
		n >>= 1;
	}
	return i;
}

/* another simple utility, d_to_i(char*) supporting only decimal
 * and devoid of range checking; obscure name chosen deliberately
 * to avoid confusion with semantic-rich POSIX routines */
unsigned int
d_to_i(char * arg)
{
    unsigned int rval = 0;
    char ch;

    while ((ch = *arg++) && ch >= '0' && ch <= '9') {
	rval *= 10;
	rval += ch - '0';
    }
    return(rval);
}

/*
 * Check for non-disk-partition arguments of the form
 *	attribute=argument
 * returning TRUE if one if found
 */
boolean_t dp_parse_argument(char *av)
{
	char *rhs = av;
	static char	here[] = "dp_parse_argument";

	/* Check for '-v' flag */

	if (av[0] == '-' && av[1] == 'v' && av[2] == 0) {
		verbose = TRUE ;
		return TRUE;
	}

	/*
	 * If we find a '=' followed by an argument in the string,
	 * check for known arguments
	 */
	while (*rhs && *rhs != '=')
		rhs++;
	if (*rhs && *++rhs) {
		/* clsize=N pages */
		if (strprefix(av,"cl")) {
			if (!bs_set_default_clsize(d_to_i(rhs)))
				dprintf(("Bad argument (%s) - ignored\n", av));
			return(TRUE);
		}
		/* else if strprefix(av,"another_argument")) {
			handle_another_argument(av);
			return(TRUE);
		} */
	}
	return(FALSE);
}

mach_port_t	default_pager_host_port = MACH_PORT_NULL;

int
main(
	int argc,
	char **argv)
{
	int			my_node;
	mach_port_t		master_device_port;
	mach_port_t		security_port;
	mach_port_t		root_ledger_wired;
	mach_port_t		root_ledger_paged;
	static char		here[] = "main";
	int			need_dp_init = 1;

	/*
	 * Use 4Kbyte cthread wait stacks.
	 */
	cthread_wait_stack_size = 4 * 1024;

	if (bootstrap_ports(bootstrap_port, &default_pager_host_port,
			    &master_device_port, &root_ledger_wired,
			    &root_ledger_paged, &security_port))
		Panic("privileged ports");

	printf_init(master_device_port);
	panic_init(default_pager_host_port);


	norma_mk = (norma_node_self(default_pager_host_port,
				    &my_node) == KERN_SUCCESS);
	printf("(default_pager): started%s\n",
	       norma_mk ? " on a NORMA kernel" : "");

	/*
	 * Process arguments:
	 *	list of attributes, list of device names.
	 * Must do device names after ports are initialised.
	 */
	while (--argc > 0) {
	    argv++;
	    if (!dp_parse_argument(*argv)) {
		if (need_dp_init) {
			/*
			 * Set up ports, port sets and thread counts.
			 */
			default_pager_initialize(default_pager_host_port);
			need_dp_init = 0;
		}
		if (!bs_add_device(*argv, master_device_port))
			printf("(default_pager): couldn't add device %s\n",
			       *argv);
		else
			printf("(default_pager): added device %s\n",
			       *argv);
	    }
	}
	if (need_dp_init) {
		default_pager_initialize(default_pager_host_port);
		need_dp_init = 0;
	}

	/*
	 * Change our scheduling policy to round-robin, and boost our priority.
	 */
	default_pager_set_policy(default_pager_host_port);

	/* Start threads and message processing.
	 */
	for (;;) {
		default_pager();
		/*NOTREACHED*/
	}
}
