/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef	_OSFMACH3_SERVER_THREAD_H_
#define _OSFMACH3_SERVER_THREAD_H_

#include <linux/autoconf.h>

#include <mach/message.h>
#include <mach/kern_return.h>
#include <mach/host_info.h>

#include <osfmach3/assert.h>
#include <osfmach3/setjmp.h>

#include <linux/sched.h>

#define CTHREADS_MULTIPLEXING 0

extern cpu_type_t		 cpu_type;
extern cpu_subtype_t		 cpu_subtype;

extern host_priority_info_data_t host_pri_info;

#define BASEPRI_SERVER	host_pri_info.server_priority
#define BASEPRI_USER	host_pri_info.user_priority
#define BASEPRI_MINPRI	host_pri_info.minimum_priority

extern kern_return_t server_thread_start(void *(*func)(void *),
					 void *arg);
extern kern_return_t server_thread_priorities(int base_pri,
					      int max_pri);
extern void *server_thread_bootstrap(void *dummy);

extern void server_thread_yield(mach_msg_timeout_t time);

struct server_thread_priv_data {
	boolean_t		activation_ready; /* act. fully initialized */
	boolean_t		preemptive;	/* preemptive thread */
	struct task_struct	*current_task;	/* client user task */
	mach_port_t		reply_port;	/* for cleanup on exit() */
	osfmach3_jmp_buf	*jmp_buf;	/* for threads recycling */
};
static __inline__ struct server_thread_priv_data *
server_thread_get_priv_data(
	cthread_t	cthread)
{
	struct server_thread_priv_data *data;

	data = (struct server_thread_priv_data *) cthread_data(cthread);
	ASSERT(data != (struct server_thread_priv_data *) 0);
	return data;
}

static __inline__ void
server_thread_set_priv_data(
	cthread_t			cthread,
	struct server_thread_priv_data	*data)
{
	data->activation_ready = TRUE;
	data->preemptive = FALSE;
	data->current_task = FIRST_TASK;
	data->reply_port = MACH_PORT_NULL;
	data->jmp_buf = (osfmach3_jmp_buf *) 0;
	cthread_set_data(cthread, (void *) data);
}

extern void launch_new_ux_server_loop(struct task_struct *new_task);

#endif	/* _OSFMACH3_SERVER_THREAD_H_ */
