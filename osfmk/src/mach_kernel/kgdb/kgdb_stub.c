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
/*        $CHeader: kgdb_stub.c 1.30 93/12/26 22:09:15 $ */

/* 
 * Revised Copyright Notice:
 *
 * Copyright 1993, 1994 CONVEX Computer Corporation.  All Rights Reserved.
 *
 * This software is licensed on a royalty-free basis for use or modification,
 * so long as each copy of the software displays the foregoing copyright notice.
 *
 * This software is provided "AS IS" without warranty of any kind.  Convex 
 * specifically disclaims the implied warranties of merchantability and
 * fitness for a particular purpose.
 */

/*
 * Machine-independent stub for remote kernel gdb.
 *
 * Known problem areas are marked with "FIXME".
 */

#include <mach_rt.h>

#include <mach/mach_types.h>
#include <mach/boolean.h>
#include <kgdb/gdb_packet.h>
#include <kgdb/kgdb.h>
#include <kern/queue.h>
#include <kern/processor.h>
#include <kern/thread.h>
#include <kern/misc_protos.h>
#include <kern/sched_prim.h>
#include <kern/ipc_mig.h>
#include <machine/setjmp.h>
#include <hp_pa/misc_protos.h>
#include <hp_pa/HP700/kgdb_support.h>
#include <machine/trap.h>
#include <machine/endian.h>
#include <kgdb/kgdb_stub.h>
#include <vm/vm_kern.h>
#include <kern/mach_param.h>
#include <kern/cpu_data.h>

#if defined(KGDB_LEDS) /* XXX XYZZY */
#include <hp_pa/HP700/busconf.h>
#include <hp_pa/Jasp.h>
#define LEDCONTROL(on,off,toggle) ledcontrol(on,off,toggle)
#else
#define LEDCONTROL(on,off,toggle)
#endif

static boolean_t attached = FALSE;

static void		hp700ss_to_gdb(
				       const struct hp700_saved_state *hp,
				       struct gdb_registers *gdb);

static void		gdb_to_hp700ss(
				       const struct gdb_registers *gdb,
				       struct hp700_saved_state *hp);

static thread_act_t	kgdb_get_thread(
					int task_id,
					int thread_id);

static int		kgdb_get_thread_self_id(
						void);

static int		kgdb_get_task_self_id(
					      void);

static void		hp700ks_to_gdb(
				       const struct hp700_kernel_state *hp,
				       struct gdb_registers *gdb);

int			look_for_ret_pc(
					thread_act_t thread,
					struct hp700_saved_state *ssp);

void			kgdb_init_undesirable_pcs(void);

jmp_buf_t *kgdb_recover = 0;

int		task = 0;
boolean_t	kgdb_fastboot = TRUE;

/*
** kgdb_do_timing is used for timing how long it takes to execute from the time
** the user hits continue to the time it takes for the thread to stop.
*/
boolean_t kgdb_do_timing = FALSE;
unsigned int kgdb_start_time;
static unsigned int kgdb_end_time;
static unsigned int kgdb_kernel_time;

int kgdb_enabled = 0;		/* temp hack for hp_pa/HP700/dca.c */

unsigned int kgdb_debug_flags = 0;

static boolean_t
kgdb_validate_address(vm_map_t map, vm_offset_t address, int *size);

void
kgdb_debug(unsigned int flag, char *format, int arg)
{
	if (kgdb_debug_flags & flag)
		printf(format, arg);
}

/*
 * This routine is called by the mux initialization code as soon as the
 * mux is configured.
 */
void
kgdb_mux_init(void)
{
	extern int halt_in_debugger;

	kgdb_net_init();
	kgdb_init_undesirable_pcs();
	kgdb_initialized = TRUE;
	if (halt_in_debugger) {
		printf("kgdb waiting...");
		kgdb_break();
		printf("connected.\n");
	}
}

/*
 * Do a (slow) bcopy.  If we called the normal kernel bcopy, we would
 * get into trouble if we ever tried to step through it.
 */
void
kgdb_bcopy(const void *source, void *destination, unsigned int size)    
{
	char *from = (char *)source;
	char *to = (char *)destination;

	while (size-- > 0) {
		*to++ = *from++;
	}
}
	
/*
 * Do a (slow) bcopy, catching any VM faults and returning an error.
 */
static boolean_t
kgdb_safe_bcopy(const char *source, char *destination, unsigned int size)
{
	jmp_buf_t jmpbuf;
	jmp_buf_t *prev_jmpbuf;
	struct {
		const char *source;	/* By copying the arguments on the   */
		char *destination;	/* stack into a structure, we avoid  */
		unsigned int size;	/* a useless warning from gcc -Wall. */
	} arg;

	arg.source = source;
	arg.destination  = destination;
	arg.size = size;

	prev_jmpbuf = kgdb_recover;
	if (_setjmp((kgdb_recover = &jmpbuf))) {
		kgdb_debug(128, "kgdb_safe_bcopy: memory fault\n", 0);
		kgdb_recover = prev_jmpbuf;
		return FALSE;
	}

	/*
	 * Sizes of int, short, and char are done using their native
	 * accesses.  This is so that gdb can be used for
	 * examining/setting CSR's, which are particular about their
	 * access sizes.
	 */

#define ACCESSABLE(typ,siz,src,dst) ((siz) == sizeof(typ) \
	                             && !((unsigned)(src) % sizeof(typ)) \
                                     && !((unsigned)(dst) % sizeof(typ)) )

	if (arg.size > sizeof(int))
	  {
slow_bcopy:
	    while (arg.size-- > 0) {
	      *arg.destination++ = *arg.source++;
	    }
	  }
	else if (ACCESSABLE(int, arg.size, arg.source, arg.destination))
	  *(int*)arg.destination = *(const int*)arg.source;
	else if (ACCESSABLE(short, arg.size, arg.source, arg.destination))
	  *(short*)arg.destination = *(const short*)arg.source;
	else
	  goto slow_bcopy;

	kgdb_recover = prev_jmpbuf;
	return TRUE;
}

static struct gdb_response  kgdb_previous_response_buffer;
static unsigned int 	    kgdb_previous_xid = 0;
static unsigned int 	    kgdb_previous_response_size = 0;

/*
 * Wait (forever) until a request for debugging service is received.
 */
static gdb_request_t
kgdb_get_request(void)
{
	kgdb_debug(4, "kgdb_get_request:\n", 0);
	while (TRUE) {
		gdb_request_t request = &kgdb_request_buffer;
		
		while (!kgdb_request_ready) {
			kgdb_net_intr();
		}

		request->xid = ntohl(request->xid);
		request->type = ntohl(request->type);
		request->task_id = ntohl(request->task_id);
		request->thread_id = ntohl(request->thread_id);
		request->request = ntohl(request->request);
		request->address = ntohl(request->address);
		request->size = ntohl(request->size);

		if (request->type != 0) {
			kgdb_debug(4, "kgdb_get_request: non-request (%x) dropped\n",
			       (int)request->type);
			kgdb_request_ready = FALSE; LEDCONTROL(0,LED4,0);
			continue;
		}

		/*
		 * Check to see whether this is a duplicate of the
		 * last request received.  If so, retransmit the
		 * previous response.
		 */
		if (kgdb_previous_xid != 0 &&
		    kgdb_previous_xid == request->xid &&
		    kgdb_previous_response_size != 0) {
		    	kgdb_debug(4, "kgdb_get_request: xid %08x, resending last response\n",
				   (int)request->xid);
			kgdb_send_packet(&kgdb_previous_response_buffer,
					 kgdb_previous_response_size);
			delay(200000); /* don't flood the net! */
			kgdb_request_ready = FALSE; LEDCONTROL(0,LED4,0);
			continue;
		}
		
		kgdb_debug(4, "kgdb_get_request: received request %08x\n",
			   (int)request->xid);
		kgdb_previous_xid = request->xid;
		kgdb_previous_response_size = 0;
		return(request);
	}
}

/*
 * Copy any desired data into the response and send it.
 */
void
kgdb_response(gdb_response_t response, unsigned int data_size, void *data)
{
	unsigned int size;

	if (data_size > 0) {
		if (!kgdb_safe_bcopy((char *)data,
				     (char *)response->data,
				     data_size)) {
			response->return_code = 1;
		}
	}

	kgdb_debug(32, "code %x\n", (int)response->return_code);
	kgdb_debug(16, "kgdb_response: response for request %08x\n",
		   (int)response->xid);

	response->xid = htonl(response->xid);
	response->type = htonl(1); /* response */
	response->return_code = htonl(response->return_code);
	response->size = htonl(data_size);

	size = sizeof(*response) - sizeof(response->data) + data_size;
	kgdb_bcopy((char *) response, (char *) &kgdb_previous_response_buffer,
		   size);
	kgdb_previous_response_size = size;
	kgdb_send_packet(response, size);
	kgdb_request_ready = FALSE; /* now we can handle another request */ LEDCONTROL(0,LED4,0);
}

static char kgdb_thread_info_buffer[RESPONSE_DATA_SIZE];
static unsigned int kgdb_thread_info_size;

#define NUM_TD_ITEMS 5

static void
kgdb_thread_info(gdb_request_t request, struct hp700_saved_state *ssp)
{
	processor_set_t pset;
	task_t task_p;
	struct thread_activation *thr_act;
	unsigned int *dp;
	int wanted_task;
	int thread_offset = 0;

	wanted_task = *((int *)request->data);
	if (request->size > sizeof (int))
		thread_offset = *(((int *)request->data)+1);

	if(wanted_task == -1)
		wanted_task = task;

	dp = (unsigned int *) kgdb_thread_info_buffer;
	kgdb_thread_info_size = 0;

	if (queue_first(&all_psets) == 0) {
		return;
	}
	queue_iterate(&all_psets, pset, processor_set_t, all_psets) {
	    if (queue_first(&pset->tasks) == 0) {
		continue;
	    }
	    queue_iterate(&pset->tasks, task_p, task_t, pset_tasks) {
		if (queue_first(&task_p->thr_acts) == 0) {
		    continue;
		}
		if (wanted_task-- > 0) {
		    /* printf("XXX: Skipped %08x\n", (int) &task); */
		    /* Skip tasks up until the one we want */
		    continue;
		}

		queue_iterate(&task_p->thr_acts, thr_act,
			      struct thread_activation*, thr_acts) {
		    if (thread_offset && thread_offset--)
		      	continue;
		    if (kgdb_thread_info_size >
				(RESPONSE_DATA_SIZE -
				 sizeof(unsigned int) * NUM_TD_ITEMS))
			return;
		    *dp++ = (unsigned int) thr_act;
		    if (thr_act->thread && thr_act->thread->top_act == thr_act) {
		        *dp = (unsigned int) thr_act->thread->kernel_stack;
			if (thr_act == current_act()) 
		        	*dp |= 1;
			dp++;
			*dp++ = (unsigned int) thr_act->thread->continuation;
			*dp++ = (unsigned int) thr_act->thread->wait_event;
			*dp++ = look_for_ret_pc(thr_act, ssp);
		    } else {
			/* No thread attach to this activation */
		        *dp++ = 0;	/* stack */
		        *dp++ = 0;	/* continuation */
		        *dp++ = 0;	/* wait event */
		        *dp++ = 0;	/* pc */
		    }

		    kgdb_thread_info_size += sizeof(unsigned int) *
		      				NUM_TD_ITEMS;
		}
		return;		
	    }
	}
}

static char kgdb_task_info_buffer[RESPONSE_DATA_SIZE];
static unsigned int kgdb_task_info_size;

static void
kgdb_task_info(gdb_request_t request)
{
	processor_set_t pset;
	task_t task;
	unsigned int *dp;
	int task_offset = 0;

	if (request->size)
		task_offset = *((int *)request->data);

	dp = (unsigned int *) kgdb_task_info_buffer;
	kgdb_task_info_size = 0;

	if (queue_first(&all_psets) == 0) {
		return;
	}
	queue_iterate(&all_psets, pset, processor_set_t, all_psets) {
		if (queue_first(&pset->tasks) == 0) {
			continue;
		}
		queue_iterate(&pset->tasks, task, task_t, pset_tasks) {
			if (task_offset && task_offset--)
				continue;
			*dp++ = (unsigned int) task;
			*dp++ = (unsigned int) task->map;
			*dp++ = (unsigned int) task->user_stop_count;
			*dp++ = task->thr_act_count;
			kgdb_task_info_size += sizeof(unsigned int) * 4;
			if (kgdb_task_info_size >= RESPONSE_DATA_SIZE) {
				return;
			}
		}
	}
}

static 
thread_act_t
kgdb_get_thread(int task_id, int thread_id)
{
	processor_set_t pset;
	task_t task;
	struct thread_activation *thr_act;
	int i = task_id;
	int j = thread_id;

	kgdb_thread_info_size = 0;

	if (queue_first(&all_psets) == 0) {
		printf("kgdb_get_thread(%d, %d) failed\n", task_id, thread_id);
		return(0);
	}
	queue_iterate(&all_psets, pset, processor_set_t, all_psets) {
		if (queue_first(&pset->tasks) == 0) {
			continue;
		}
		queue_iterate(&pset->tasks, task, task_t, pset_tasks) {
			if (queue_first(&task->thr_acts) == 0) {
				continue;
			}
			if (i-- > 0) {
				/* printf("XXX: Skipped %08x\n", (int) &task); */
				/* Skip tasks up until the one we want */
				continue;
			}

			queue_iterate(&task->thr_acts, thr_act, struct thread_activation*, thr_acts) {
				if (j-- > 0) 
					continue;
				return(thr_act);
			}
			printf("kgdb_get_thread(%d, %d): no such thread\n", task_id, thread_id);
			return(0);
		}
	}
	printf("kgdb_get_thread(%d, %d) no such task\n", task_id, thread_id);
	return(0);
}

static 
int
kgdb_get_thread_self_id()
{
	struct thread_activation *thr_act;
	int thread_id = 0;
	thread_act_t thread = current_act();
	task_t task = current_task();

	if (thread == 0)
		return(THREAD_MAX);

	kgdb_thread_info_size = 0;

	queue_iterate(&task->thr_acts, thr_act, struct thread_activation*, thr_acts) {
		if (thr_act == thread) 
			return(thread_id);
		thread_id++;
	}
/*	printf("kgdb_get_thread_self_id() failed\n"); */
	return(THREAD_MAX);
}

static 
int
kgdb_get_task_self_id()
{
	processor_set_t pset;
	task_t task;
	task_t wanted_task = current_task();
	int task_id = 0;

	if (queue_first(&all_psets) == 0) {
		return(0);
	}
	queue_iterate(&all_psets, pset, processor_set_t, all_psets) {
	    if (queue_first(&pset->tasks) == 0) {
	    	continue;
	    }
	    queue_iterate(&pset->tasks, task, task_t, pset_tasks) {
		if (queue_first(&task->thr_acts) == 0) {
		    continue;
		}
		if (task == wanted_task)
		    return(task_id);	
		else
		    task_id++;
	    }
	}
/* 	printf("kgdb_get_task_self_id() failed\n"); */
	return(0);
}

/*
 * Trivial support for calling kernel functions from the debugger.  If
 * you set "kf" to the address of a function, kgdb will call
 * it, passing ka1..ka4 as parameters.  This is a quick hack to 
 * allow you to call things like the KDB output functions littered all
 * around.
 */

typedef int (*PFI)(int a1, int a2, int a3, int a4);

PFI kf = 0;
int ka1 = 0;
int ka2 = 0;
int ka3 = 0;
int ka4 = 0;

int nudge_thread = 0;

extern void nudge(thread_act_t thr_act);

/*
 * Perform requested action and send a response.
 */
static boolean_t
kgdb_request(gdb_request_t request, gdb_response_t response, int type,
	     struct hp700_saved_state *ssp)
{
	thread_act_t thread_act = (thread_act_t)0;
	int thread_id, task_id;
	int signal = 5;

	kgdb_debug(32, "kgdb_request: xid %08x ", (int)request->xid);
	kgdb_debug(32, "request (%c) ", (int)request->request);
	kgdb_debug(32, "size %d ", (int)request->size);
	kgdb_debug(32, "task %d ", (int)request->task_id);
	kgdb_debug(32, "thread %d ... ", (int)request->thread_id);
	if (request->size > 0) {
	    unsigned int i;
	    kgdb_debug(32, "\ndata = 0x%x", request->data[0]);
	    for (i = 1; i < request->size; i++)
		kgdb_debug(32, " 0x%x", request->data[i]);
	}
	kgdb_debug(32, "\ntask %d", task);
	task_id = kgdb_get_task_self_id();
	kgdb_debug(32, ", task_id %d", task_id);
	thread_id = kgdb_get_thread_self_id();
	kgdb_debug(32, ", thread_id %d ", thread_id);
	response->xid = request->xid;
	response->return_code = 0;

	if (nudge_thread) {
		thread_act_t th = (thread_act_t)nudge_thread;
		nudge_thread = 0;
		printf("nudge(th)...");
		nudge(th);
		printf("done\n");
	}
	response->task_id = task_id;
	response->thread_id = request->thread_id;
	switch (request->request) {
	case 'A':
	case 'a':
	  	if (request->request == 'a'  &&  attached)
		  response->return_code = 2;
		else {
		  attached = TRUE;
		}
		kgdb_response(response, 0, NULL);
		break;

	case 'w':
		response->thread_id = thread_id;
		signal = 5; /* SIGTRAP */
		kgdb_response(response, sizeof(signal), &signal);
		break;

	case '?':
		response->thread_id = thread_id;
		signal = 2; /* SIGINT */
		kgdb_response(response, sizeof(signal), &signal);
		break;
	    
	case 'g':		/* read registers */
		if (task >= 0 && request->thread_id < THREAD_MAX) {
		    thread_act = kgdb_get_thread(task, (int)request->thread_id);
			
		    if (thread_act == 0) {
			printf("kgdb: non-existent task or activation (%d %d)\n",
			       request->task_id, request->thread_id);
			response->return_code = 1;
			kgdb_response(response, 0, NULL);
			kgdb_debug(32, "code %x\n",
				   (int)response->return_code);
			return TRUE;
		    }
		} 
		  	
		if ((thread_act == (thread_act_t)0) ||
		    (thread_act == current_act()))
		       hp700ss_to_gdb(ssp, (struct gdb_registers *)response->data);
		else
		       hp700ks_to_gdb(
		          (struct hp700_kernel_state *) thread_act->thread->kernel_stack,
			  (struct gdb_registers *)response->data);

		kgdb_response(response,
			      (unsigned)REGISTER_BYTES,
			      response->data);
		break;
	    
	case 'G':		/* write registers */
		gdb_to_hp700ss((struct gdb_registers *)response->data, ssp);
		kgdb_response(response, 0, NULL);
		break;
	    
	case 'M':		/* write memory */
		if (request->size <= 512) {
			if (!kgdb_validate_address(kernel_map,
						   request->address,
						   (int *)&request->size)) {
				printf("Invalid address: 0x%08x\n",
				       request->address);
				response->return_code = 1;
			} else if (!kgdb_safe_bcopy((char *) request->data,
						    (char *) request->address,
						    request->size)) {
				response->return_code = 1;
			}
			kgdb_flush_cache(request->address, request->size);
		} else {
			printf("bad size %d\n", request->size);
			response->return_code = 3;
		}
		kgdb_response(response, 0, NULL);
		if (kf != 0) {
			int kr = (*kf)(ka1, ka2, ka3, ka4);
			printf("return from 0x%x is %d (0x%x)\n", 
			    (unsigned int)kf, kr, kr);
			kf = 0;
		}
		break;

	case 'm':		/* read memory */
		if (request->size <= 512)
		  {
		    if (!kgdb_validate_address(kernel_map,
					       request->address,
					       (int *)&request->size))
		      {
			printf("Invalid address: 0x%08x\n", request->address);
			response->return_code = 1;
			request->size = 0;
		      }
		    kgdb_response(response, request->size, 
				  (char *)request->address);
		  }
		else
		  {
		    printf("bad size %d\n", request->size);
		    response->return_code = 3;
		    kgdb_response(response, 0, NULL);
		  }
		break;
	    
	case 'c':		/* continue */
		kgdb_continue(type, ssp, request->address);
		kgdb_response(response, 0, NULL);
		return TRUE;

	case 'C':		/* detach and continue */
		kgdb_do_timing = FALSE;   /* Turn off timings */
		kgdb_continue(type, ssp, (vm_offset_t)0);
		kgdb_response(response, 0, NULL);
		attached = FALSE;
		return TRUE;

	case '#':		/* Start the timing function */
		if (kgdb_do_timing) {
			kgdb_do_timing = FALSE;   /* Turn off timings */
		} else {
			kgdb_do_timing = TRUE;    /* Turn on timings */
		}
		kgdb_response(response, 0, NULL);
		return TRUE;
		break;

	case '$':		/* read timing value */
		kgdb_debug(32, "kgdb_request: time 0x%x\n", 
			   (int)kgdb_kernel_time);
		kgdb_response(response, sizeof(kgdb_kernel_time), 
			      &kgdb_kernel_time);
 		return TRUE;
		break;

	case 's':		/* single step */
		kgdb_single_step(type, ssp, request->address);
		kgdb_response(response, 0, NULL);
		return TRUE;
	    
	case 'k':		/* kill kernel (reboot) */
		printf("kgdb: rebooting\n");
		kgdb_response(response, 0, NULL);
		fastboot(!kgdb_fastboot);/* reboot; should never return */
		return TRUE;
	case 'T':
		kgdb_task_info(request);
		kgdb_response(response, kgdb_task_info_size, 
			      kgdb_task_info_buffer);
		return FALSE;
	case 't':
		if (!request->size)
			*(int *)request->data = task;
		kgdb_thread_info(request, ssp);
		kgdb_response(response, kgdb_thread_info_size,
			      kgdb_thread_info_buffer);
		return FALSE;
	case 'v':
		response->return_code = CURRENT_KGDB_PROTOCOL_VERSSION;
		kgdb_response(response, 0, NULL);
		return FALSE;
	default:
		printf("kgdb_request: unknown request %c\n", request->request);
		response->return_code = 1;
		kgdb_response(response, 0, NULL);
		break;
	}
	
	return FALSE;	/* not finished; wait for additional commands */
}

/*
 * This routine is called from the trap or interrupt handler when
 * a breakpoint instruction is encountered or a single step operation
 * completes. The argument is a pointer to a machine dependent 
 * saved_state structure that was built on the interrupt or kernel stack.
 */
void 
kgdb_trap(int type, struct hp700_saved_state *ssp)
{
	jmp_buf_t jmpbuf;
	jmp_buf_t *prev_jmpbuf = kgdb_recover;
	gdb_response_t response = &kgdb_response_buffer;

	if (!kgdb_initialized) {
		kgdb_recover = prev_jmpbuf;
		return;			/* not debugging */
	}

	/*
	** If timing is on, lets get the time right away.
	*/
	if (kgdb_do_timing && (type == 9)) {
		kgdb_end_time = kgdb_read_timer();
		kgdb_debug(64, "kgdb_end_time(0x%x)\n", (int)kgdb_end_time);
		kgdb_kernel_time = kgdb_end_time - kgdb_start_time;
		kgdb_debug(64, "kgdb_kernel_time(0x%x)\n", (int)kgdb_kernel_time);
	}

	kgdb_debug(64, "kgdb_trap(%d)\n", type);

	if (_setjmp((kgdb_recover = &jmpbuf))) {
		printf("kgdb_trap(%d): unexpected exception in debugger\n",
		       type);
#ifdef DEBUG
		dump_ssp(ssp);
#endif /* DEBUG */
		for(;;);
	}

	kgdb_stop = FALSE;
	LEDCONTROL(0,LED5,0);
#if MACH_RT
	disable_preemption();
#endif /* MACH_RT */
	kgdb_active = TRUE;
	LEDCONTROL(LED6,0,0);
	kgdb_fixup_pc(type, ssp);	/* machine-dependent fix PC */

	if (!kgdb_connected) {
		printf("kgdb: waiting for debugger connection\n");
	}

	task = kgdb_get_task_self_id();
	while (TRUE) {
		gdb_request_t request = kgdb_get_request();
		if (kgdb_request(request, response, type, ssp)) {
			break;
		}
	}

	kgdb_recover = prev_jmpbuf;
	kgdb_active = FALSE;
#if MACH_RT
	enable_preemption();
#endif /* MACH_RT */
	LEDCONTROL(0,LED6,0);
}

static boolean_t	kgdb_validate = TRUE;	/* validate addresses? */
static boolean_t
kgdb_validate_address(vm_map_t map, vm_offset_t address, int *size)
{
	vm_map_entry_t	entry;
	int		amt_found = 0;
	int		sub_size;

/*	printf("Checking map 0x%08x\n", (unsigned int)map); */
	if (map == VM_MAP_NULL)		/* not initialized */
		return 1;

	/*
	 *	If we are attempting to access the kgdb_validate variable
	 *	then allow it.  Else if it is set to false, allow all access
	 */
	if ((address == (vm_offset_t)&kgdb_validate) || !kgdb_validate)
		return 1;

	for (entry = vm_map_first_entry(map);
	     entry != vm_map_to_entry(map);
	     entry = entry->vme_next) {
	     	if (entry->is_sub_map) {
	     		sub_size = *size;
	     		if (!kgdb_validate_address(entry->object.sub_map,
	     				address, &sub_size)) {
				continue;
			} else {
				if (sub_size != *size) {
					amt_found += sub_size;
					*size -= sub_size;
					address += sub_size;
				} else {
					amt_found += sub_size;
					break;
				}
			}
		}
		if (address < entry->vme_start || address > entry->vme_end)
			continue;
		if (address + *size > entry->vme_end) {
			amt_found = (entry->vme_end - address);
			*size -= amt_found;
			address = entry->vme_end;
		} else {
			amt_found += *size;
			break;
		}
	}
	if (entry == vm_map_to_entry(map) && (amt_found == 0))
		return 0;
	*size = amt_found;
	return 1;
}

/*
 * Macro for copying doubles without using the float registers, and
 * allowing for doubles that are on 4 byte boundaries instead of 8
 * byte boundaries.
 */

#define dblcpy(dst,src)						\
{								\
  ((unsigned*)&(dst))[0] = ((const unsigned*)&(src))[1];	\
  ((unsigned*)&(dst))[1] = ((const unsigned*)&(src))[1];	\
}

static void
hp700ss_to_gdb(const struct hp700_saved_state *hp,
	       struct gdb_registers *gdb)
{
  /* 
   * Copy a saved_state structure into a gdb_registers.  The
   * structures are very close, but they sometimes change, so this
   * function keeps the kernel people from having to worry about gdb,
   * and vice versa.  
   *
   * NOTE: Several of the control registers are not saved in the
   * thread state.  This function reads the "live" values from the
   * kernel and puts them into *gdb.
   */

  gdb->flags		= hp->flags;
  gdb->r1		= hp->r1;
  gdb->rp		= hp->rp;
  gdb->r3		= hp->r3;
  gdb->r4		= hp->r4;
  gdb->r5		= hp->r5;
  gdb->r6		= hp->r6;
  gdb->r7		= hp->r7;
  gdb->r8		= hp->r8;
  gdb->r9		= hp->r9;
  gdb->r10		= hp->r10;
  gdb->r11		= hp->r11;
  gdb->r12		= hp->r12;
  gdb->r13		= hp->r13;
  gdb->r14		= hp->r14;
  gdb->r15		= hp->r15;
  gdb->r16		= hp->r16;
  gdb->r17		= hp->r17;
  gdb->r18		= hp->r18;
  gdb->t4		= hp->t4;
  gdb->t3		= hp->t3;
  gdb->t2		= hp->t2;
  gdb->t1		= hp->t1;
  gdb->arg3		= hp->arg3;
  gdb->arg2		= hp->arg2;
  gdb->arg1		= hp->arg1;
  gdb->arg0		= hp->arg0;
  gdb->dp		= hp->dp;
  gdb->ret0		= hp->ret0;
  gdb->ret1		= hp->ret1;
  gdb->sp		= hp->sp;
  gdb->r31		= hp->r31;
  gdb->sar		= hp->sar;
  gdb->iioq_head	= hp->iioq_head;
  gdb->iisq_head	= hp->iisq_head;
  gdb->iioq_tail	= hp->iioq_tail;
  gdb->iisq_tail	= hp->iisq_tail;
  gdb->eiem		= hp->eiem;
  gdb->iir		= hp->iir;
  gdb->isr		= hp->isr;
  gdb->ior		= hp->ior;
  gdb->ipsw		= hp->ipsw;
  gdb->sr4		= hp->sr4;
  gdb->sr0		= hp->sr0;
  gdb->sr1		= hp->sr1;
  gdb->sr2		= hp->sr2;
  gdb->sr3		= hp->sr3;
  gdb->sr5		= hp->sr5;
  gdb->sr6		= hp->sr6;
  gdb->sr7		= hp->sr7;
  gdb->rctr		= hp->rctr;
  gdb->pidr1		= hp->pidr1;
  gdb->pidr2		= hp->pidr2;
  gdb->ccr		= hp->ccr;
  gdb->pidr3		= hp->pidr3;
  gdb->pidr4		= hp->pidr4;
  gdb->tr2		= hp->tr2;     /* usr0, cr26 */
  gdb->fpu              = hp->fpu;
}

static void
gdb_to_hp700ss(const struct gdb_registers *gdb,
	       struct hp700_saved_state *hp)
{
  /* 
   * Copy a saved_state structure into a gdb_registers.  The
   * structures are very close, but they sometimes change, so this
   * function keeps the kernel people from having to worry about gdb,
   * and vice versa.  
   *
   * NOTE: Several of the control registers are not saved in the
   * thread state.  In most cases this function will copy the
   * registers striaght out of *gdb into the "live" registers.
   * Notable excepts are cr1-cr7 and cr
   */

  hp->flags		= gdb->flags;
  hp->r1		= gdb->r1;
  hp->rp		= gdb->rp;
  hp->r3		= gdb->r3;
  hp->r4		= gdb->r4;
  hp->r5		= gdb->r5;
  hp->r6		= gdb->r6;
  hp->r7		= gdb->r7;
  hp->r8		= gdb->r8;
  hp->r9		= gdb->r9;
  hp->r10		= gdb->r10;
  hp->r11		= gdb->r11;
  hp->r12		= gdb->r12;
  hp->r13		= gdb->r13;
  hp->r14		= gdb->r14;
  hp->r15		= gdb->r15;
  hp->r16		= gdb->r16;
  hp->r17		= gdb->r17;
  hp->r18		= gdb->r18;
  hp->t4		= gdb->t4;
  hp->t3		= gdb->t3;
  hp->t2		= gdb->t2;
  hp->t1		= gdb->t1;
  hp->arg3		= gdb->arg3;
  hp->arg2		= gdb->arg2;
  hp->arg1		= gdb->arg1;
  hp->arg0		= gdb->arg0;
  hp->dp		= gdb->dp;
  hp->ret0		= gdb->ret0;
  hp->ret1		= gdb->ret1;
  hp->sp		= gdb->sp;
  hp->r31		= gdb->r31;
  hp->sar		= gdb->sar;
  hp->iioq_head		= gdb->iioq_head;
  hp->iisq_head		= gdb->iisq_head;
  hp->iioq_tail		= gdb->iioq_tail;
  hp->iisq_tail		= gdb->iisq_tail;
  hp->eiem		= gdb->eiem;
  hp->iir		= gdb->iir;
  hp->isr		= gdb->isr;
  hp->ior		= gdb->ior;
  hp->ipsw		= gdb->ipsw;
  hp->sr4		= gdb->sr4;
  hp->sr0		= gdb->sr0;
  hp->sr1		= gdb->sr1;
  hp->sr2		= gdb->sr2;
  hp->sr3		= gdb->sr3;
  hp->sr5		= gdb->sr5;
  hp->sr6		= gdb->sr6;
  hp->sr7		= gdb->sr7;
  hp->rctr		= gdb->rctr;
  hp->pidr1		= gdb->pidr1;
  hp->pidr2		= gdb->pidr2;
  hp->ccr		= gdb->ccr;
  hp->pidr3		= gdb->pidr3;
  hp->pidr4		= gdb->pidr4;
  hp->tr2		= gdb->tr2;   /* usr0, cr26 */
  hp->fpu               = gdb->fpu;
}


static void
hp700ks_to_gdb(const struct hp700_kernel_state *hp,
	       struct gdb_registers *gdb)
{
  /* 
   * Copy a saved_state structure into a gdb_registers.  The
   * structures are very close, but they sometimes change, so this
   * function keeps the kernel people from having to worry about gdb,
   * and vice versa.  
   *
   * NOTE: Several of the control registers are not saved in the
   * thread state.  This function reads the "live" values from the
   * kernel and puts them into *gdb.
   */

  gdb->flags		= 0xdead0100;
  gdb->r1		= 0xdead0200;
  gdb->rp		= hp->rp;	/* r2		*/
  gdb->r3		= hp->r3;
  gdb->r4		= hp->r4;
  gdb->r5		= hp->r5;
  gdb->r6		= hp->r6;
  gdb->r7		= hp->r7;
  gdb->r8		= hp->r8;
  gdb->r9		= hp->r9;
  gdb->r10		= hp->r10;
  gdb->r11		= hp->r11;
  gdb->r12		= hp->r12;
  gdb->r13		= hp->r13;
  gdb->r14		= hp->r14;
  gdb->r15		= hp->r15;
  gdb->r16		= hp->r16;
  gdb->r17		= hp->r17;
  gdb->r18		= hp->r18;
  gdb->t4		= 0xdead0300;	/* r19		*/
  gdb->t3		= 0xdead0400;	/* r20		*/
  gdb->t2		= 0xdead0500;	/* r21		*/
  gdb->t1		= 0xdead0600;	/* r22		*/
  gdb->arg3		= 0xdead0700;	/* r23		*/
  gdb->arg2		= 0xdead0800;	/* r24		*/
  gdb->arg1		= 0xdead0900;	/* r25		*/
  gdb->arg0		= 0xdead1000;	/* r26		*/
  gdb->dp		= 0xdead1100;	/* r27	 	*/
  gdb->ret0		= 0xdead1200;	/* r28	 	*/
  gdb->ret1		= 0xdead1300;	/* r29	 	*/
  gdb->sp		= hp->sp;	/* r30	 	*/
  gdb->r31		= 0xdead1400;
  gdb->sar		= 0xdead1500;	/* cr11		*/
  gdb->iioq_head	= hp->rp;
  gdb->iisq_head	= 0xdead1700;
  gdb->iioq_tail	= hp->rp+4;
  gdb->iisq_tail	= 0xdead1900;
  gdb->eiem		= 0xdead2000;	/* cr15		*/
  gdb->iir		= 0xdead2100;	/* cr19		*/
  gdb->isr		= 0xdead2200;	/* cr20		*/
  gdb->ior		= 0xdead2300;	/* cr21		*/
  gdb->ipsw		= 0xdead2400;	/* cr22		*/
  gdb->sr4		= 0xdead2600;
  gdb->sr0		= 0xdead2700;
  gdb->sr1		= 0xdead2800;
  gdb->sr2		= 0xdead2900;
  gdb->sr3		= 0xdead3000;
  gdb->sr5		= 0xdead3100;
  gdb->sr6		= 0xdead3200;
  gdb->sr7		= 0xdead3300;
  gdb->rctr		= 0xdead3400;	/* cr0 		*/
  gdb->pidr1		= 0xdead3500;	/* cr8		*/
  gdb->pidr2		= 0xdead3600;	/* cr9		*/
  gdb->ccr		= 0xdead3700;	/* cr10		*/
  gdb->pidr3		= 0xdead3800;	/* cr12		*/
  gdb->pidr4		= 0xdead3900;	/* cr13		*/
  gdb->tr2		= 0xdead4000;	/* cr26,usr0	*/
  gdb->fpu              = 0;
}

static void
gdb_to_hp700ks(const struct gdb_registers *gdb,
	       struct hp700_kernel_state *hp)
{
  /* 
   * Copy a saved_state structure into a gdb_registers.  The
   * structures are very close, but they sometimes change, so this
   * function keeps the kernel people from having to worry about gdb,
   * and vice versa.  
   *
   * NOTE: Several of the control registers are not saved in the
   * thread state.  In most cases this function will copy the
   * registers striaght out of *gdb into the "live" registers.
   * Notable excepts are cr1-cr7 and cr
   */

  hp->rp		= gdb->rp;
  hp->r3		= gdb->r3;
  hp->r4		= gdb->r4;
  hp->r5		= gdb->r5;
  hp->r6		= gdb->r6;
  hp->r7		= gdb->r7;
  hp->r8		= gdb->r8;
  hp->r9		= gdb->r9;
  hp->r10		= gdb->r10;
  hp->r11		= gdb->r11;
  hp->r12		= gdb->r12;
  hp->r13		= gdb->r13;
  hp->r14		= gdb->r14;
  hp->r15		= gdb->r15;
  hp->r16		= gdb->r16;
  hp->r17		= gdb->r17;
  hp->r18		= gdb->r18;
  hp->sp		= gdb->sp;
}

#define KGDB_DFLT_FUNCSZ 	0x40	/* 64 words, 256 bytes */
#define KGDB_MAX_FUNCSZ 	0x400	/* 1024 words, 4 k */
#define IND_PC(name) (int *)name, (int *)name+KGDB_DFLT_FUNCSZ

struct {
	int 	*start,
      		*end;
} undesirable_pcs[] = {
	IND_PC(thread_invoke),
	IND_PC(thread_block_reason),
	IND_PC(thread_block),
	IND_PC(ipc_mqueue_receive),
	IND_PC(mach_msg_rpc_from_kernel),
	IND_PC(0)
};

boolean_t	kgdb_is_undesirable_pc(int *pc);

void
kgdb_init_undesirable_pcs(void)
{
	int i = 0;
	int size = 0;
	int *pc;

	for (i = 0; pc = undesirable_pcs[i].start; i++)
		for (size = 0; size < KGDB_MAX_FUNCSZ; size++, pc++)
			if (*pc == 0xe840c002) { /* bv,n r0(rp) */
				undesirable_pcs[i].end = pc;
				break;
			}
}

boolean_t
kgdb_is_undesirable_pc(int *pc)
{
	int i = 0;
	for (i = 0; undesirable_pcs[i].start; i++)
		if (pc >= undesirable_pcs[i].start &&
		    pc < undesirable_pcs[i].end)
			return TRUE;
	return FALSE;
}

#define KGDB_FRAME_SIZE 5

int
look_for_ret_pc(thread_act_t thread_act, struct hp700_saved_state *ssp)
{
	jmp_buf_t jmpbuf;
	jmp_buf_t *prev_jmpbuf;
	int *frame;
	int *pc;

	if (!thread_act)
		return 0;

	prev_jmpbuf = kgdb_recover;
	if (_setjmp((kgdb_recover = &jmpbuf))) {
		kgdb_debug(128, "look_for_ret_pc: memory fault\n", 0);
		kgdb_recover = prev_jmpbuf;
		return 0;
	}

	if (thread_act == current_act())
		frame = (int *) ssp->r3;
	else
		frame = (int *) ((struct hp700_kernel_state *)
				 thread_act->thread->kernel_stack)->r3;
	if (frame < (int *)VM_MIN_KERNEL_ADDRESS ||
	    frame >= (int *)VM_MAX_KERNEL_ADDRESS)
		return 0;

	/* skip first frame, meaningless if stop at top of a procedure */
	do {
		frame = (int *)*frame;
		if (frame < (int *)VM_MIN_KERNEL_ADDRESS ||
		    frame >= (int *)VM_MAX_KERNEL_ADDRESS)
			return 0;
	} while (kgdb_is_undesirable_pc((int *)*(frame-KGDB_FRAME_SIZE)));

	kgdb_recover = prev_jmpbuf;

	return(*(frame-KGDB_FRAME_SIZE));
}
