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
 * Thread scheduling policy 
 */

struct policy_state {
	policy_t policy;
	union {
		struct policy_timeshare_base	tr_base;
		struct policy_fifo_base 	ff_base;
		struct policy_rr_base		rr_base;
	} base;
	union {
		struct policy_timeshare_limit 	tr_limit;
		struct policy_fifo_limit 	ff_limit;
		struct policy_rr_limit 		rr_limit;
	} limit;
}; 

typedef struct policy_state *policy_state_t;

extern kern_return_t 	get_sched_attr(mach_port_t	task,
				       mach_port_t	thread,
				       policy_state_t 	ps,
				       boolean_t	print);

extern kern_return_t	set_sched_attr(mach_port_t	task,
				       mach_port_t	thread,
				       policy_state_t 	ps,
				       boolean_t	print);


#define get_thread_sched_attr(thread, ps, print) \
	get_sched_attr(0, thread, ps, print);

#define set_thread_sched_attr(thread, ps, print) \
	set_sched_attr(0, thread, ps, print);

#define get_task_sched_attr(task, ps, print) \
	get_sched_attr(task, 0, ps, print);

#define set_task_sched_attr(task, ps, print) \
	set_sched_attr(task, 0, ps, print);

extern int	get_base_pri(struct policy_state *ps);
	
extern void	set_base_pri(struct policy_state *ps, int base_pri);

extern int	get_max_pri(struct policy_state *ps);

extern void	set_max_pri(struct policy_state *ps, int max_pri);

extern int	get_quantum(struct policy_state *ps);

extern void	set_quantum(struct policy_state *ps, int quantum);
