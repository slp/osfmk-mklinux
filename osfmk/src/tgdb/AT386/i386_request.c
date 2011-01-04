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
 * 
 */
/*
 * MkLinux
 */

#include <mach.h>
#include <mach/kern_return.h>
#include <mach/message.h>
#include <mach/notify.h>
#include <mach/machine/vm_param.h>
#include <mach/std_types.h>
#include <mach/vm_prot.h>
#include <mach/exc_server.h>
#include <mach/mach_host.h>
#include <device/device_types.h>
#include <tgdb.h>
#include <net_filter.h>
#include <gdb_packet.h>


unsigned int tgdb_debug_flags = 0;

#define TGDB_SESSIONS	10	/* can debug this many tasks simultaneously */

#define TASK_HAS_SESSION ((tgdb_session_t)-1) /* means task is already attached */

static mach_port_t
tgdb_task_port(unsigned int task_id);

static vm_prot_t	tgdb_make_writable(
					   mach_port_t task, 
					   vm_address_t addr, 
					   vm_size_t size);

static vm_prot_t	tgdb_get_prot(
				      mach_port_t task,
				      vm_address_t addr, 
				      vm_size_t size);

static void		tgdb_set_prot(mach_port_t task,
				      vm_address_t addr,
				      vm_size_t size,
				      vm_prot_t prot);

/*
** tgdb_do_timing is used for timing how long it takes to execute from the time
** the user hits continue to the time it takes for the thread to stop.
*/
static boolean_t    tgdb_do_timing = FALSE;
static unsigned int tgdb_start_time;
static unsigned int tgdb_end_time;
static unsigned int tgdb_kernel_time;


/*
** X-30129 - If kgdb is being used to debug a process that forks, and 
** assertion can happen if the child process hits a breakpoint because
** no session is attached to the child.  To fix this we will add a data
** structure that will keep track of breakpoints.  When a child hits a 
** breakpoint, the proper instruction will be replaced.  This is KLUDGE CITY,
** the proper fix would be complicated but the best solution by adding 
** another session for the child.
*/
static unsigned char break_insn[] = {0x00, 0x01, 0x00, 0x04};

#define BREAK_MAX 50
typedef struct {
    unsigned int       addr;                     /* Address of where BPT is */
    char        inst[sizeof(break_insn)]; /* Original instruction before BPT */
} breakpoint_type;
static breakpoint_type break_list[BREAK_MAX];
static int break_count = 0; 

/*
** The following is used for displaying information about break statements
** for debugging.
*/
#define BREAKPOINT_DEBUG 1      /* Used for debugging breakpoints. */
int debug_flag = 0;      /* Set to one of the defines. */

/*
 * host_priv        used to get processor set priv ports.
 * request_port	    requests from remote gdb come here.
 * wait_port_set    contains exception_ports (per session) and request_port.
 */
mach_port_t	tgdb_host_priv = MACH_PORT_NULL;
mach_port_t	tgdb_wait_port_set = MACH_PORT_NULL;

boolean_t tgdb_initialized = FALSE;

static struct tgdb_session tgdb_session_table[TGDB_SESSIONS];

void
simple_lock(struct slock *l) 
{
	while (!spin_try_lock(l));
}

void
simple_unlock(struct slock *l) 
{
	spin_unlock(l);
}

void
tgdb_debug(unsigned int flag, const char *format, int arg)
{
  if (tgdb_debug_flags & flag)
    printf(format, arg);
}

/*
** tgdb_read_timer: This routine returns the current interval timer.
**
*/
static unsigned int
tgdb_read_timer(void)

{
	unsigned int regval;
#if 0
	__asm__ volatile ("mfctl 16,%0" : "=r" (regval));
#else
	regval = 0;
#endif
	return regval;

}

void
tgdb_session_init(void)
{
	unsigned int i;

	for (i = 0; i < TGDB_SESSIONS; i++) {
		tgdb_session_table[i].valid = FALSE;
	}
}

/*
 * Look up the task port for task_id and allocate a new session to begin
 * debugging the task.
 */
static tgdb_session_t
tgdb_allocate_session(unsigned int task_id)
{
	unsigned int i;
	tgdb_session_t session = 0;
	mach_port_t task = MACH_PORT_NULL;

	for (i = 0; i < TGDB_SESSIONS; i++) {
		session = &tgdb_session_table[i];

		if (session->valid) 
		  {
		    if (session->task_id == task_id)
		      return TASK_HAS_SESSION;
		    else
		      continue;
		  }
		
		task = tgdb_task_port(task_id);
		if (task == MACH_PORT_NULL)
			continue;

		session->lock.lock_data[0] = -1;
		session->lock.lock_data[1] = -1;
		session->lock.lock_data[2] = -1;
		session->lock.lock_data[3] = -1;
		session->valid = TRUE;
		session->task_id = task_id;
		session->task = task;
		session->current_thread = MACH_PORT_NULL;
		session->task_suspended = FALSE;
		session->single_stepping = FALSE;
		session->one_thread_cont = FALSE;
		session->thread_count = 0;
		session->thread = 0;		
		session->exception = 0;		
		
		return session;
	}
	return 0;
}

/*
** tgdb_break_installed: Takes as input an address.  This routine checks
**                       to see if he has a breakpoint at that address and
**                       if so, returns the index into the array.
*/
static int
tgdb_break_installed(unsigned int addr)

{

	int i;

	for (i=0; i < break_count; i++) {
		if (addr == break_list[i].addr) {
			return i;
		}
	}
	return -1;

}

/*
** tgdb_add_breakpoint: This routines takes an address and an instruction and
**                      adds that instruction to the breakpoint array.  We only
**                      keep BREAK_MAX breakpoints.
*/
static void
tgdb_add_breakpoint(unsigned int addr, char *inst)

{

   int i;
   int j;

   j = tgdb_break_installed(addr);

   if ((break_count < BREAK_MAX) && (j == -1)) {
      break_list[break_count].addr = addr;
      for (i=0; i < sizeof(break_insn); i++)
         break_list[break_count].inst[i] = inst[i];
      break_count++;
   }

}

/*
** tgdb_check_breakpoint: Takes as input the task, addr, and data.  This
**                        routine checks to see if we are going to add a
**                        breakpoint. If so, we need to update the
**                        breakpoint data structure.
** return:                1 is returned for an error and 0 for success.
*/
static int
tgdb_check_breakpoint(mach_port_t task, unsigned int addr, unsigned char *data)

{

   vm_address_t low_address = (vm_address_t) trunc_page(addr);
   vm_size_t    aligned_length =
                (vm_size_t) round_page(addr + sizeof(break_insn)) - low_address;
   pointer_t     copied_memory;
   unsigned      copy_count;
   kern_return_t kr = (kern_return_t)0;
   char          inst[sizeof(break_insn)];

   if ((data[0] == break_insn[0]) && (data[1] == break_insn[1]) && 
       (data[2] == break_insn[2]) && (data[3] == break_insn[3])) {

	if (debug_flag == BREAKPOINT_DEBUG) {
	   printf("tgdb: There is going to be a breakpoint at addr %x\n", addr);
	}
        kr = vm_read(task, low_address, aligned_length, &copied_memory,
                     &copy_count);

        if (kr != KERN_SUCCESS) {
                return 1;
        }
	bcopy((char *) addr - low_address + copied_memory,
	      inst, sizeof(break_insn));

        tgdb_add_breakpoint(addr, inst);

	if (debug_flag == BREAKPOINT_DEBUG) {
	   printf("tgdb: inst %x replaced with bpt at addr %x\n",
	       (unsigned int)inst, addr);
	}

        kr = vm_deallocate(mach_task_self(), copied_memory, copy_count);
        if (kr != KERN_SUCCESS) {
                printf("tgdb: vm_deallocate failed (%x)\n", kr);
        }

   }

   return 0;

}

/*
** tgdb_fix_breakpoint: takes as input the task and addr.  This is called
**                      if a break instruction has been executed.  If we
**                      have information about that address we will fix it
**                      otherwise, we will assert.
*/
static int
tgdb_fix_breakpoint(mach_port_t task, unsigned int addr)

{

   vm_address_t low_address = (vm_address_t) trunc_page(addr);
   vm_size_t aligned_length =
	    (vm_size_t) round_page(addr + sizeof(break_insn)) - low_address;
   pointer_t copied_memory;
   unsigned copy_count;
   kern_return_t kr;
   int     break_index;
   vm_prot_t saved_prot;

   if (debug_flag == BREAKPOINT_DEBUG) {
      printf("tgdb: in tgdb_fix_breakpoint going to look at %x\n", addr);
   }
   break_index = tgdb_break_installed(addr);
   if (break_index != -1) {
	if (debug_flag == BREAKPOINT_DEBUG) {
           printf("tgdb: We can fix this breakpoint\n");
	}
	kr = vm_read(task, low_address, aligned_length, &copied_memory,
		     &copy_count);
	if (kr != KERN_SUCCESS) {
		if (debug_flag == BREAKPOINT_DEBUG) {
        	   printf("tgdb: Error in reading breakpoint instruction\n");
		}
		return 0;
	}

	bcopy(break_list[break_index].inst,
	      (char *) addr - low_address + copied_memory,
	      sizeof(break_insn));

	saved_prot = tgdb_make_writable(task, low_address, aligned_length);

	kr = vm_write(task, low_address, copied_memory, aligned_length);
	if (kr != KERN_SUCCESS) {
		if (debug_flag == BREAKPOINT_DEBUG) {
        	   printf("tgdb: Error in writing previous instruction\n");
		}
		return 0;
	}

	tgdb_set_prot(task, low_address, aligned_length, saved_prot);

	kr = vm_deallocate(mach_task_self(), copied_memory, copy_count);
	if (kr != KERN_SUCCESS) {
		printf("tgdb: vm_deallocate failed (%x)\n", kr);
	}

#ifdef	FLUSH_CACHE
	{
   	vm_machine_attribute_val_t flush = MATTR_VAL_CACHE_FLUSH;
	kr = vm_machine_attribute(task, low_address, aligned_length,
				  MATTR_CACHE, &flush);
	if (kr != KERN_SUCCESS) {
		printf("tgdb: flush cache failed (%x)\n", kr);
	}
	}
#endif

	if (debug_flag == BREAKPOINT_DEBUG) {
	   printf("tgdb: returning after fixing instruction.\n");
	}
	return 2;

   }
   if (debug_flag == BREAKPOINT_DEBUG) {
      printf("tgdb: returning after not fixing instruction.\n");
   }
   return 0;

}

static void
tgdb_deallocate_session(tgdb_session_t session)
{
	assert(session->valid);
	session->valid = FALSE;
}

/*
 * Look up the session for the specified task port.  This is used when
 * we intercept an exception for a thread in a task being debugged.  The
 * exception message contains a task port.
 */
tgdb_session_t
tgdb_lookup_task(mach_port_t task)
{
	unsigned int i;

	for (i = 0; i < TGDB_SESSIONS; i++) {
		if (tgdb_session_table[i].valid &&
		    tgdb_session_table[i].task == task) {
			return &tgdb_session_table[i];
		}
	}
	return 0;
}

/*
 * Look up the session for the specified task_id and thread_id.  This is
 * used when the user changes his notion of the "current thread."
 */
tgdb_session_t
tgdb_lookup_task_id(unsigned int task_id, unsigned int thread_id)
{
	unsigned int i;
	
	for (i = 0; i < TGDB_SESSIONS; i++) {
		if (tgdb_session_table[i].valid &&
		    tgdb_session_table[i].task_id == task_id &&
		    tgdb_session_table[i].thread_count > thread_id) {
			return &tgdb_session_table[i];
		}
	}
	return 0;
}

static struct gdb_response tgdb_previous_response_buffer;
static unsigned int 	    tgdb_previous_response_size = 0;

/*
 * Convert fields in the response to network byte order, save it for
 * possible retransmission (we should save it in a per-session buffer),
 * and send the packet.
 */
static void
tgdb_response(gdb_response_t response)
{
	unsigned int size;

	size = sizeof(*response) - sizeof(response->data) + response->size;

	tgdb_debug(128, "tgdb_response: response for request %08x\n",
		   (int)response->xid);
	tgdb_debug(128, "tgdb_response: thread %d\n", (int)response->thread_id);

	/*
	 * It makes little sense to me to pass the header around in
	 * network byte order when the data section is in host byte
	 * order.  Mucho better to do it all in host order, and let
	 * the gdb target deal with it (since it's used to having to
	 * do that, anyhow). jcf XXX
	 */
	response->xid 		= htonl(response->xid);
	response->type 		= htonl(1); 			/* response */
	response->task_id	= htonl(response->task_id);
	response->thread_id	= htonl(response->thread_id);
	response->return_code 	= htonl(response->return_code);
	response->size 		= htonl(response->size);

	bcopy((char *) response, (char *) &tgdb_previous_response_buffer,
	      size);

	tgdb_previous_response_size = size;
	tgdb_send_packet((char *) response, size);
}

/*
 * Look up the task port for the task_id'th task.  The order of processor
 * sets and tasks returned is random, so this may not be the best solution.
 *
 * This is used only when first attaching to a task.  Other times, we look
 * up a session based on a task_id or a task port.
 */
static mach_port_t
tgdb_task_port(unsigned int task_id)
{
	mach_port_t *processor_set_names = 0; 
	mach_port_t processor_set_control = MACH_PORT_NULL;
	mach_port_t *task_ports = 0;
	mach_port_t task = MACH_PORT_NULL;
	unsigned int count, i;
	kern_return_t kr;

	count = 0;
	kr = host_processor_sets(tgdb_host_priv, &processor_set_names,
				 &count);
	if (kr != KERN_SUCCESS || processor_set_names == 0 || count == 0) {
		return MACH_PORT_NULL;
	}

	kr = host_processor_set_priv(tgdb_host_priv, processor_set_names[0],
				     &processor_set_control);
	
	for (i = 0; i < count; i++) {
		mach_port_deallocate(mach_task_self(), processor_set_names[i]);
	}
	vm_deallocate(mach_task_self(), (vm_address_t) processor_set_names,
		      count * sizeof(mach_port_t));
	
 	if (kr != KERN_SUCCESS || processor_set_control == MACH_PORT_NULL) {
		return MACH_PORT_NULL;
	}

	count = 0;
	kr = processor_set_tasks(processor_set_control, &task_ports, &count);
	if (kr != KERN_SUCCESS || task_ports == 0 || count == 0) {
		return MACH_PORT_NULL;
	}

	task = (task_id < count) ? task_ports[task_id] : MACH_PORT_NULL;
	
	for (i = 0; i < count; i++) {
		if (i != task_id) {
			mach_port_deallocate(mach_task_self(), task_ports[i]);
		}
	}

	vm_deallocate(mach_task_self(), (vm_address_t) task_ports,
		      count * sizeof(mach_port_t));
	
	return task;
}

/*
 * Get the list of threads in the task.  This may have changed if 
 * thread_create or thread_terminate has been called since the last
 * time we stopped.
 *
 * This should be called whenever a thread in the task stops (i.e., after
 * single stepping or at a breakpoint) because new threads may have been
 * created.
 *
 * We need to deallocate the old thread list's memory, but I don't think
 * the ports need to be deallocated; for threads that went away, the port
 * was destroyed then, and for threads that still exist, we'll get the same
 * port names we had before.
 */
static void
tgdb_update_thread_list(tgdb_session_t session)
{
	mach_port_t *threads = 0;
	unsigned int count = 0;

	if (session->thread != 0) {
		vm_deallocate(mach_task_self(), (vm_offset_t) session->thread,
			      session->thread_count * sizeof(mach_port_t));
	}

	if (task_threads(session->task, &threads, &count) == KERN_SUCCESS) {
		session->thread_count = count;
		session->thread = threads;
	} else {
		session->thread_count = 0;
		session->thread = 0;
	}
}

/*
 * Look up the id of the specified thread.  This is used when a thread
 * takes a breakpoint so we can tell gdb which thread stopped.  The thread
 * list should be up-to-date before this is called, so if the thread is
 * not in the list, something's wrong.
 */
static int
tgdb_thread_id(tgdb_session_t session, mach_port_t thread)
{
	unsigned int i;
	
	for (i = 0; i < session->thread_count; i++) {
		if (session->thread[i] == thread) {
			return i;
		}
	}
	printf("tgdb: unknown thread took a breakpoint\n");
	return 0;
}

/*
 * Suspend the task and get an up-to-date list of its threads.
 */
static void
tgdb_stop(tgdb_session_t session)
{
       	if (session->task_suspended == FALSE)  {
	  session->task_suspended = TRUE;
	  if (task_suspend(session->task) != KERN_SUCCESS) {
		printf("tgdb: task suspend failed\n");
	      }
	}
	tgdb_update_thread_list(session);
}

/*
 * Suspend task and arrange to intercept its exceptions.
 * This should be called only once per task, when attaching to it.
 */
static tgdb_session_t
tgdb_attach(unsigned int task_id)
{
	kern_return_t		kr;
	thread_port_array_t 	threads;	
	unsigned int 		count, i;


	mach_msg_type_number_t old_exception_count;
	exception_behavior_t old_behaviours[EXC_TYPES_COUNT];
	thread_state_flavor_t old_flavors[EXC_TYPES_COUNT];

	tgdb_session_t session = 0;

        printf("Attach task %d\n", task_id);

	session = tgdb_allocate_session(task_id);

	if (session == 0  ||  session == TASK_HAS_SESSION) {
		return session;
	}

	simple_lock(&session->lock);

	tgdb_stop(session);

	/*
	 * Save the task's original exception port and insert ours in
	 * its place.  (Do we need to generate another send right for it?)
	 */
        old_exception_count = EXC_TYPES_COUNT;
	if (task_get_exception_ports(
		session->task, 
		(EXC_MASK_ALL & ~(EXC_MASK_SYSCALL|EXC_MASK_MACH_SYSCALL)),
		session->old_exception_masks,
		&old_exception_count,
		(exception_port_array_t)session->original_exception_ports, 
		old_behaviours,
		old_flavors) != KERN_SUCCESS) {
			printf("tgdb: can't get exception port\n");
			tgdb_deallocate_session(session);
			simple_unlock(&session->lock);
			return 0;
	}

	/*
	 * Allocate a new port to be used as the exception interception
	 * port for the new target task and add it to the port set that
	 * tgdb receives on.  Each task needs its own interception port
	 * because it is the only way to find the proper session when an
	 * exception is received.
	 */
	if (mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
			       &session->exception_port) != KERN_SUCCESS) {
		printf("tgdb: can't allocate new exception port\n");
		tgdb_deallocate_session(session);
		simple_unlock(&session->lock);
		return 0;
	}
	if (mach_port_insert_right(mach_task_self(), session->exception_port,
				   session->exception_port,
				   MACH_MSG_TYPE_MAKE_SEND) != KERN_SUCCESS) {
		printf("tgdb: can't make send right for exception port\n");
		tgdb_deallocate_session(session);
		simple_unlock(&session->lock);
		return 0;
	}
	if (mach_port_request_notification(mach_task_self(),
					   session->task,
					   MACH_NOTIFY_DEAD_NAME,
					   0,	/*sync */
					   session->exception_port,
					   MACH_MSG_TYPE_MAKE_SEND_ONCE,
					   &session->original_notify_port)
	  != KERN_SUCCESS) {
		printf("tgdb: can't request death notification\n");
	}
	if (mach_port_move_member(mach_task_self(), session->exception_port,
				  tgdb_wait_port_set) != KERN_SUCCESS) {
		printf("tgdb: can't add exception port to port set\n");
		tgdb_deallocate_session(session);
		simple_unlock(&session->lock);
		return 0;
	}

	if (task_set_exception_ports(
		  session->task,
		  (EXC_MASK_ALL & ~
		  (EXC_MASK_SYSCALL|EXC_MASK_MACH_SYSCALL|EXC_MASK_RPC_ALERT)),
	          session->exception_port,
		  EXCEPTION_DEFAULT, 0) != KERN_SUCCESS) {
		printf("tgdb: can't set task exception ports\n");
		tgdb_deallocate_session(session);
		simple_unlock(&session->lock);
		return 0;
	}


	/*
	 * Set up the exception ports for each thread in the task also,
	 * since they may override the task exception ports. (The Unix
	 * server has a habit of setting thread exception ports in tasks
	 * under its control.)
	 */
	
    	kr = task_threads(session->task, &threads, &count);
    	if (kr != KERN_SUCCESS) {
                printf("tgdb: can't get tasks's thread list\n");
		tgdb_deallocate_session(session);
		simple_unlock(&session->lock);
        	return 0;
	}

	for (i = 0 ; i < count ; i++) {
		kr = thread_set_exception_ports(
		  threads[i],
		  (EXC_MASK_ALL & ~
		  (EXC_MASK_SYSCALL|EXC_MASK_MACH_SYSCALL|EXC_MASK_RPC_ALERT)),
	          MACH_PORT_NULL,
		  EXCEPTION_DEFAULT, 0);

		if (kr != KERN_SUCCESS) {
			printf("tgdb: can't set thread exception port\n");
			tgdb_deallocate_session(session);
			simple_unlock(&session->lock);
			return 0;
		}
	}

	simple_unlock(&session->lock);
	return session;
}

/*
 * We are finished debugging a task; restore its original exception port,
 * resume it, and deallocate the session.
 */
void
tgdb_detach(tgdb_session_t session)
{
	assert((!session->task) || session->task_suspended);

	break_count = 0;

	/*
	 * Remove the task's exception interception port from the tgdb
	 * port set and deallocate it.  Do this before resuming the task
	 * so there's no chance that the task will generate more exceptions
	 * on the interception port.
	 */
	if (mach_port_move_member(mach_task_self(), session->exception_port,
				  tgdb_wait_port_set) != KERN_SUCCESS) {
		printf("tgdb: can't remove exception port from port set\n");
	}
	if (mach_port_deallocate(mach_task_self(), session->exception_port) !=
	    KERN_SUCCESS) {
		printf("tgdb: can't deallocate exception port\n");
	}
				
	if (session->task) {  
		printf("Resume task %d\n", session->task_id);

		/*
		 * Restore the task's original exception port and resume it.
		 * We may get an error for either of these operations if the
		 * task has already died.  We can't do much about that.
		 */
		task_set_exception_ports(
		    session->task,
		    (EXC_MASK_ALL & ~(EXC_MASK_SYSCALL|EXC_MASK_MACH_SYSCALL|EXC_MASK_RPC_ALERT)),
		    session->original_exception_ports[0],
		    EXCEPTION_DEFAULT, 0);

		mach_port_request_notification(mach_task_self(),
				       session->task,
				       MACH_NOTIFY_DEAD_NAME,
				       0,	/*sync */
				       session->original_notify_port,
				       MACH_MSG_TYPE_MOVE_SEND_ONCE,
				       &session->original_notify_port); 
		task_resume(session->task);
	}
	tgdb_deallocate_session(session);
}

/*
 * This is used when the trace trap is caught after single-stepping a
 * thread.  Prior to stepping the thread, all the other threads in the
 * task were suspended, so resume them now.  (The task remains suspended,
 * so the threads won't resume until a continue request is received.)
 */
static void
tgdb_resume_all_threads(tgdb_session_t session)
{
	unsigned int i;

	assert(session->task_suspended);
	for (i = 0; i < session->thread_count; i++)
		thread_resume(session->thread[i]);
}

/*
 * This is used when single-stepping.  We want only the thread being
 * single-stepped to do anything, so we suspend all the threads, resume
 * the thread of interest, and resume the task.  When we catch the
 * trace exception, we'll resume all the threads again.
 */
static void
tgdb_suspend_all_threads(tgdb_session_t session)
{
	unsigned int i;

	assert(session->task_suspended);
	for (i = 0; i < session->thread_count; i++) {
		if (thread_suspend(session->thread[i]) != KERN_SUCCESS) {
			printf("tgdb: suspend thread %d failed\n", i);
		}
	}
}

/*
 * Read memory from the specified task and address, package it up in
 * the response, and ship it off.  Eventually, this should use the
 * new interface to read thread memory.
 */
static void
tgdb_read(mach_port_t task, gdb_request_t request, gdb_response_t response)
{
	kern_return_t kr;

	kr = tgdb_vm_read(task,
			  (vm_offset_t) request->address,
			  (vm_size_t) request->size,
			  (vm_offset_t)response->data);

	if (kr != KERN_SUCCESS)
		response->return_code = kr;
	else
		response->size = request->size;
#if 0
	tgdb_response(response);
#endif
}

/*
 * Attempt to make the specified memory writable.  This is important
 * for setting breakpoints!
 *
 * FIXME  There are some boundary
 * conditions that aren't handled correctly, such as memory ranges that
 * span vm_regions.
 */

static vm_prot_t
tgdb_make_writable(mach_port_t task, vm_address_t addr, vm_size_t size)
{
	vm_prot_t prot;

	prot = tgdb_get_prot(task, addr, size);
	if (prot != VM_PROT_NO_CHANGE && !(prot & VM_PROT_WRITE)) {
		tgdb_set_prot(task, addr, size, prot | VM_PROT_WRITE);
		return(prot);
	} else
		return(VM_PROT_NO_CHANGE);
}

/*
 * Get protection for specified memory. 
 */

static vm_prot_t
tgdb_get_prot(mach_port_t task, vm_address_t addr, vm_size_t size)
{
	vm_region_basic_info_data_t vm_info;
	mach_port_t object_name;
	unsigned int	count = sizeof(vm_info);
	kern_return_t kr;

	kr = vm_region(task, &addr, &size, VM_REGION_BASIC_INFO,
		       (vm_region_info_t) &vm_info, &count, &object_name);
	mach_port_deallocate (mach_task_self(), object_name);
	if (kr != KERN_SUCCESS) {
                printf("tgdb: vm_region failed (%x)\n", kr);
		return(VM_PROT_NO_CHANGE);
	}
	return(vm_info.protection);
}

/*
 * Set protection for specified memory. 
 */

static void
tgdb_set_prot(mach_port_t task,
  	      vm_address_t addr,
              vm_size_t size,
              vm_prot_t prot)
{
	kern_return_t kr;
	if (prot != VM_PROT_NO_CHANGE) {
		kr = vm_protect(task, addr, size, FALSE, prot);
		if (kr != KERN_SUCCESS)
                	printf("tgdb: vm_protect failed (%x)\n", kr);
	}
}
        
/*
 * Write the specified data from the request into the task's address space.
 * Eventually this, too, should use the thread memory interface.  Since
 * vm_write works on a page-size granularity, we have to read the appropriate
 * page(s) first, make sure the region is writable, merge in the new data, and
 * write the page(s) back.
 *
 * FIXME  The original protection of the region should be restored after
 * the write.
 */
static void
tgdb_write(mach_port_t task, gdb_request_t request, gdb_response_t response)
{
	vm_address_t low_address = (vm_address_t) trunc_page(request->address);
	vm_size_t aligned_length =
	    (vm_size_t) round_page(request->address + request->size) -
		low_address;
	pointer_t copied_memory;
	unsigned copy_count;
	kern_return_t kr;
	vm_prot_t saved_prot;

	kr = vm_read(task, low_address, aligned_length, &copied_memory,
		     &copy_count);
	if (kr != KERN_SUCCESS) {
		response->return_code = kr;
		return;
	}

	bcopy((char *) request->data,
	      (char *) request->address - low_address + copied_memory,
	      (int)request->size);

	saved_prot = tgdb_make_writable(task, low_address, aligned_length);

	kr = vm_write(task, low_address, copied_memory, aligned_length);
	if (kr != KERN_SUCCESS) {
		response->return_code = kr;
		return;
	}

	tgdb_set_prot(task, low_address, aligned_length, saved_prot);

	kr = vm_deallocate(mach_task_self(), copied_memory, copy_count);
	if (kr != KERN_SUCCESS) {
		printf("tgdb: vm_deallocate failed (%x)\n", kr);
	}

#ifdef	FLUSH_CACHE
	{
	vm_machine_attribute_val_t flush = MATTR_VAL_CACHE_FLUSH;
	kr = vm_machine_attribute(task, low_address, aligned_length,
				  MATTR_CACHE, &flush);
	if (kr != KERN_SUCCESS) {
		printf("tgdb: flush cache failed (%x)\n", kr);
	}
	}
#endif
}

/*
 * Handle an exception raised by a thread in a task being debugged.
 * We are only interested in breakpoint exceptions; others should be
 * passed on to the task's original exception port.
 *
 * FIXME  Exceptions other than breakpoints should be passed to the
 * original exception port.
 */
kern_return_t
catch_exception_raise(mach_port_t exception_port, mach_port_t thread,
		      mach_port_t task, exception_type_t exception,
		      exception_data_t codes, mach_msg_type_number_t codeCnt)
{
	tgdb_session_t session = 0;
	vm_address_t   addr;
	int            i, retval;

	if (exception != EXC_BREAKPOINT) {
		printf("tgdb: caught non-breakpoint exception (%d %x %x %x)\n",
		       exception, exception_port, (int)codes, codeCnt);

		for (i = 0; i < codeCnt; i++)
               		printf("code[i]=0x%x\n", codes[i]);
	}

        /*
	** If timing is on, lets get timing right away.
	*/
        if (tgdb_do_timing) {
                tgdb_end_time = tgdb_read_timer();
                tgdb_kernel_time = tgdb_end_time - tgdb_start_time;
        }

	session = tgdb_lookup_task(task);

	tgdb_debug(512, "exception (%d,", exception);
	tgdb_debug(512, "%d): ", codes[0]);
	tgdb_debug(512, "session %x\n", (int)session);
	if (session) {
		tgdb_debug(512, "\tss %d ", session->single_stepping);
		tgdb_debug(512, "otc %d\n", session->one_thread_cont);
	}

	if (session == 0) {
		addr = (vm_address_t)tgdb_get_break_addr(thread);
		if (debug_flag == BREAKPOINT_DEBUG) {
			printf("tgdb: break hit at addr %x\n", addr);
		}
		retval = tgdb_fix_breakpoint(task, addr);
		if (retval) {
			if (debug_flag == BREAKPOINT_DEBUG) {
				printf("Was able to fix breakpoint\n");
			}
			tgdb_clear_trace(thread);
			thread_resume(thread);
			return KERN_SUCCESS;
		}
	}

	assert(session != 0);

	simple_lock(&session->lock);

	tgdb_stop(session);

	if (session->single_stepping) {
		/*
		 * Make all threads runnable (leave task suspended).
		 * Only the stepped thread was resumed, so no other
		 * thread should have gotten an exception.
		 */
		assert(thread == session->current_thread);
		tgdb_clear_trace(thread);
		tgdb_resume_all_threads(session);
		session->single_stepping = FALSE;
	} else  if (session->one_thread_cont) {
	        /* 
		 * Only one thread is running from a 1cont command.
		 * We need to get things back to normal. Make sure
		 * they are all suspended, then resume them.
		 */
		assert(thread == session->current_thread);
	        thread_suspend(session->current_thread);
		tgdb_resume_all_threads(session);
		session->one_thread_cont = FALSE;
	} else {
		/*
		 * Any thread could have taken the breakpoint; remember
		 * which one did so we can tell gdb who stopped.
		 */
		session->current_thread = thread;
	}
	session->exception = exception;
	simple_unlock(&session->lock);
	return KERN_SUCCESS;
}

/*
 * Retrieve basic information about each thread in the task.  This will
 * be simply formatted and printed for the remote gdb user. The 't'
 * request contains two pieces of data: the task id and the starting thread
 * index. Here, we ignore the task id, since we can only collect info for
 * our current task. 
 */
static void
tgdb_thread_info(
	tgdb_session_t session,
	gdb_request_t request,
	gdb_response_t response)
{
	unsigned int *dp;
	unsigned int i, threads;
	int thread_offset = 0;

	/* Two casts needed to shut gcc up.  But it has a point.
	   ("cast increases required alignment") */
	if (request->size == 8)
		thread_offset = *((int *)(void *)&request->data[4]);
	if (thread_offset < 0 || thread_offset > 1000)
		return;

	threads = sizeof(response->data) / (sizeof(int) * 4);

	if (session->thread_count-thread_offset < threads)
	  threads = ((int)session->thread_count) - thread_offset;
	response->size = threads * sizeof(int) * 4;

	dp = (unsigned int *) (void *) response->data;
	for (i = thread_offset; i < thread_offset+threads; i++) {
		tgdb_basic_thread_info(session->thread[i],
				       session->task,
				       dp);
		dp += 4;
	}
}

void
tgdb_request(gdb_request_t request, gdb_response_t response)
{
	int signal = 5;
	tgdb_session_t session;
	int retval;
	struct i386_thread_state ts;
	struct i386_float_state fs;


	if (request->type != 0) {
		printf("TGDB: request->type != 0 !!!\n");
		return;
	}

	/*
	 * Check to see whether this is a duplicate of the
	 * last request received.  If so, retransmit the
	 * previous response.
	 */

	if ((tgdb_previous_response_size != 0) &&
	    (request->xid == tgdb_previous_response_buffer.xid))
	{
		tgdb_send_packet((char *)
				 &tgdb_previous_response_buffer,
				 tgdb_previous_response_size);
		return;
	}
		
	tgdb_previous_response_size = 0;

	request->xid 		= ntohl(request->xid);
	request->type		= ntohl(request->type);
	request->request	= ntohl(request->request);
	request->size		= ntohl(request->size);
	request->address	= ntohl(request->address);
	request->task_id	= ntohl(request->task_id);
	request->thread_id	= ntohl(request->thread_id);

        response->xid = request->xid;
        response->task_id = request->task_id;
        response->thread_id = request->thread_id;
        response->return_code = 0;
        response->size = 0;

	tgdb_debug(32, "tgdb_request: xid %08x ", (int)request->xid);
	tgdb_debug(32, "request (%c) ", (int)request->request);
	tgdb_debug(32, "size %d ", (int)request->size);
	tgdb_debug(32, "task %d ", (int)request->task_id);
	tgdb_debug(32, "thread %d ... \n", (int)request->thread_id);

	if (request->request == 'a'  ||  request->request == 'A') 
	  {
	    /*
	     * It's an attach request.  If normal ('a') then attach
	     * if nobody else is, or send an error if someone already
	     * attached.  If forced attach ('A') then Just Do It.
	     */
	      session = tgdb_attach(request->task_id);
	      if (session == 0) 
		  response->return_code = 1;
	      else if (session == TASK_HAS_SESSION  &&  request->request != 'A')
		  response->return_code = 2;
	      tgdb_debug(32, "code %x\n", (int)response->return_code);
	      tgdb_response(response);
	      return;
	  }

	session = tgdb_lookup_task_id(request->task_id, request->thread_id);

	if (session == 0) {
		printf("tgdb_request: non-existent task or thread (%d %d)\n",
		       request->task_id, request->thread_id);
		response->return_code = -1;
	        tgdb_debug(32, "code %x\n", (int)response->return_code);
		tgdb_response(response);
		return;
	}

	simple_lock(&session->lock);

	if (!session->task_suspended) {
		tgdb_stop(session);
	}

	if (session->single_stepping) {
		/*
		 * We interrupted a single step before receiving the trace
		 * exception.  All threads except the traced thread are
		 * suspended.
		 *
		 * Turn off the trace bit and resume all the threads so
		 * they're ready to go again.
		 */
		tgdb_clear_trace(session->current_thread);
		tgdb_resume_all_threads(session);
		session->single_stepping = FALSE;
	}

        if (session->one_thread_cont) {
	        /* 
		 * Only one thread is running from a 1cont command.
		 * We need to get things back to normal. Make sure
		 * they are all suspended, then resume them.
		 */
	        thread_suspend(session->current_thread);
		tgdb_resume_all_threads(session);
		session->one_thread_cont = FALSE;
	}


	if (session->current_thread == 0) {
		session->current_thread = session->thread[request->thread_id];
	}

	switch (request->request) {
	case '?':
		response->thread_id = tgdb_thread_id(session,
						     session->current_thread);
		response->size = sizeof(signal);
		signal = 2; /* SIGINT */
		bcopy((char *) &signal,
		      (char *) response->data, 
		      sizeof(signal));
		break;

	case 'w':
		response->thread_id = tgdb_thread_id(session,
						     session->current_thread);
		response->size = sizeof(signal);
		switch (session->exception) {
		case EXC_BAD_ACCESS:
			signal = 11; /* SIGSEGV */
			break;
		case EXC_BAD_INSTRUCTION:
			signal = 4; /* SIGILL */
			break;
		case EXC_ARITHMETIC:
			signal = 8; /* SIGFPE */
			break;
		case EXC_EMULATION:
		case EXC_SOFTWARE:
			signal = 7; /* SIGEMT */
			break;
		case EXC_BREAKPOINT:
			signal = 5; /* SIGTRAP */
			break;
		case EXC_SYSCALL:
		case EXC_MACH_SYSCALL:
			printf("Got undue syscall exception %d\n",
			       session->exception);
		default:
			signal = 5; /* SIGTRAP */
		}
		bcopy((char *) &signal, (char *)
		      response->data,
		      sizeof(signal));
		break;

	case 'c':		/* continue task */
        	/*
        	** If timing is turned on, lets start the timing now.
        	*/
        	if (tgdb_do_timing) {
                	tgdb_start_time = tgdb_read_timer();
        	}
		response->return_code = task_resume(session->task);
		session->task_suspended = FALSE;
		break;

	case 'C':		/* detach and continue */
		tgdb_do_timing = FALSE;   /* Turn off timings */
		tgdb_detach(session);
		break;

        case '#':               /* Start the timing function */
                if (tgdb_do_timing) {
                        tgdb_do_timing = FALSE;   /* Turn off timings */
                } else {
                        tgdb_do_timing = TRUE;    /* Turn on timings */
                }
                break;

        case '$':               /* read timing value */
		bcopy((char *)&tgdb_kernel_time,
		      (char *) response->data, 
                      sizeof(tgdb_kernel_time)); 
		response->size = sizeof(tgdb_kernel_time);
                break;

	case '1':               /* single thread continue */
		tgdb_suspend_all_threads(session);
        	/*
        	** If timing is turned on, lets start the timing now.
        	*/
        	if (tgdb_do_timing) {
                	tgdb_start_time = tgdb_read_timer();
        	}
		thread_resume(session->current_thread);
		session->one_thread_cont = TRUE;
		task_resume(session->task);
		session->task_suspended = FALSE;
		break;

	case 's':		/* single step a thread */
		assert(request->thread_id < session->thread_count);
		session->current_thread = session->thread[request->thread_id];
		tgdb_set_trace(session->current_thread);

		/*
		 * Suspend other threads in task so only the current thread
		 * single-steps.
		 */
		session->single_stepping = TRUE;
		tgdb_suspend_all_threads(session);

        	/*
        	** If timing is turned on, lets start the timing now.
        	*/
        	if (tgdb_do_timing) {
                	tgdb_start_time = tgdb_read_timer();
        	}
		thread_resume(session->current_thread);
		if (task_resume(session->task) != KERN_SUCCESS)
		  printf("tgdb: task resume failure\n");
		session->task_suspended = FALSE;
		break;

	case 'g':		/* read registers */
		assert(request->thread_id < session->thread_count);
		session->current_thread = session->thread[request->thread_id];
		tgdb_get_registers(session->current_thread, &ts, &fs);
		response->size = REGISTER_BYTES;
		i386ts_to_gdb(&ts, &fs, (void *) response->data);
		break;

	case 'G':		/* write registers */
		assert(request->thread_id < session->thread_count);
		session->current_thread = session->thread[request->thread_id];
		gdb_to_i386ts((void *) request->data, &ts, &fs);
		tgdb_set_registers(session->current_thread, &ts, &fs);
		break;

	case 'm':		/* read memory */
		tgdb_read(session->task, request, response);
		break;

	case 'M':		/* write memory */
                retval = tgdb_check_breakpoint(session->task, request->address, 
				      request->data);
		tgdb_write(session->task, request, response);
		break;

	case 'k':		/* kill task */
		printf("tgdb: terminating task %d\n", request->task_id);
		task_terminate(session->task);
		break;

	case 't':		/* thread list */
		tgdb_thread_info(session, request, response);
		break;

	case 'v':		/* request protocol version */
		response->return_code = CURRENT_KGDB_PROTOCOL_VERSSION;
		break;
	default:
		printf("tgdb_request: %c not implemented\n",
		       request->request);
		response->return_code = 1;
		break;
	}

	tgdb_debug(32, "code %x\n", (int)response->return_code);
	tgdb_response(response);
	simple_unlock(&session->lock);

	return;
}


/*
 * Read memory from the specified task and address
 * Eventually, this should use the
 * new interface to read thread memory.
 */

kern_return_t
tgdb_vm_read(
	     mach_port_t task,
	     vm_offset_t from,
	     vm_size_t size,
	     vm_offset_t to)
{
	vm_offset_t low_address = (vm_offset_t) trunc_page(from);
	vm_size_t aligned_length =
	    (vm_size_t) round_page(from + size) - low_address;
	pointer_t copied_memory;
	unsigned copy_count;
	kern_return_t kr;

	kr = vm_read(task, low_address, aligned_length, &copied_memory,
		     &copy_count);
	if (kr != KERN_SUCCESS) {
		return(kr);
	}

	bcopy((char *) from - low_address + copied_memory,
	      (char *) to, size);

	kr = vm_deallocate(mach_task_self(), copied_memory, copy_count);
	if (kr != KERN_SUCCESS) {
		printf("tgdb: vm_deallocate failed (%x)\n", kr);
	}
	return(KERN_SUCCESS);
}


	
