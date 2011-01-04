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


#include <mach_perf.h>

char prof_command[1024] = "";
int 			prof_opt = 0;		
char 			mprof_args[256];
int 			prof_alloc_unit = 0;
float			prof_trunc = 0;

mach_port_t		kernel_task;
mach_port_t    		prof_port = MACH_PORT_NULL;
mach_port_t		prof_cmd_port = MACH_PORT_NULL;

int			samples_per_buf;

sample_buf_t 		sample_bufs;
sample_buf_t 		cur_samples;

int			sample_count = 0;

struct {
	int		sample_count;
	sample_buf_t	cur_samples;
	unsigned 	free_count;
	unsigned 	*next_sample;
} prof_saved_state;

mach_thread_t 		prof_thread;
extern boolean_t 	prof_server();
extern boolean_t 	prof_ctrl_server();
boolean_t 		prof_demux();
extern sample_buf_t	next_samples_buf();
void (*prof_print_routine)();

prof_thread_body()
{

	if (debug)
		printf("prof thread\n");
				
	MACH_CALL(mach_msg_server, (prof_demux,
				    2048,
				    prof_port,
				    MACH_MSG_OPTION_NONE));
}

boolean_t prof_demux(in, out)
{
	if (prof_server(in, out))
		return(TRUE);
	else
	    	return(prof_ctrl_server(in, out));
}

prof_reset()
{
	if (prof_opt && prof_port != MACH_PORT_NULL) {
		MACH_CALL(call_prof_reset, (prof_port));
	}
}

do_prof_reset(prof_port)
{
	register i;

	if (debug > 1)
		printf("do_prof_reset\n");
	sample_count = 0;
	cur_samples = sample_bufs;
	cur_samples->free_count = samples_per_buf;
	cur_samples->next_sample = cur_samples->samples;
	do_prof_save();
	return(KERN_SUCCESS);
}

do_prof_start(port)
mach_port_t port;
{
	if (task_sample(kernel_task, prof_port) != KERN_SUCCESS) {
		/* In case profiler has been interrupted */
		(void) task_sample(kernel_task, MACH_PORT_NULL);
		MACH_CALL(task_sample, (kernel_task, prof_port));
	}
	return(KERN_SUCCESS);
}

do_prof_stop(port)
mach_port_t port;
{
	return(task_sample(kernel_task, MACH_PORT_NULL));
}

prof_save()
{
	if (prof_opt && prof_port != MACH_PORT_NULL) {
		MACH_CALL(call_prof_save, (prof_port));
	}
}

do_prof_save(port)
mach_port_t port;
{
	if (debug)
		printf("saved %d samples, count = %d\n", 
		       sample_count - prof_saved_state.sample_count,
		       sample_count);
	prof_saved_state.sample_count = sample_count;
	prof_saved_state.cur_samples = cur_samples;
	prof_saved_state.free_count = cur_samples->free_count;
	prof_saved_state.next_sample = cur_samples->next_sample;
	return(KERN_SUCCESS);
}

prof_drop()
{
	if (prof_opt && prof_port != MACH_PORT_NULL) {
		MACH_CALL(call_prof_drop, (prof_port));
	}
}

do_prof_drop(port)
mach_port_t port;
{
	if (debug)
		printf("dropped %d samples, count = %d\n", 
		       sample_count - prof_saved_state.sample_count,
		       prof_saved_state.sample_count);
	sample_count = prof_saved_state.sample_count;
	cur_samples = prof_saved_state.cur_samples;
	cur_samples->free_count = prof_saved_state.free_count;
	cur_samples->next_sample = prof_saved_state.next_sample;
	return(KERN_SUCCESS);
}

struct sample_buf *
alloc_sample_buf()
{
	struct sample_buf *buf;

	if (!prof_alloc_unit)
		prof_alloc_unit = PROF_ALLOC_UNIT;
	MACH_CALL(vm_allocate, (mach_task_self(),
			       (vm_offset_t *)&buf,
			       prof_alloc_unit,
			       TRUE));
	buf->free_count = samples_per_buf;
	buf->next_sample = buf->samples;
	buf->next = 0;
	if (debug > 1)
		printf("alloc_sample_buf returns %x\n", buf);
	return(buf);
}

receive_samples(reply_port, samples, count)
mach_port_t reply_port;
unsigned *samples;
unsigned count;
{
	register n;

	if (debug)
		printf("receiving %d samples\n", count);
	if (!count)
		return(KERN_SUCCESS);
	sample_count += count;
	while (count) {
		if (!cur_samples->free_count)
			cur_samples = next_samples_buf(cur_samples);
			
		if (cur_samples->free_count > count)
			n = count;
		else 
			n = cur_samples->free_count;

		if (debug > 1)
			printf("receive samples copies %d samples from %x to %x\n", n, samples, cur_samples->next_sample);
		bcopy((char *)samples,
		      (char *)cur_samples->next_sample,
		      n*sizeof(unsigned));
		cur_samples->free_count -= n;
		cur_samples->next_sample += n;
		count -= n;
		samples += n;
	}
	return(KERN_SUCCESS);
}

prof_init()
{
	struct sample_buf *sample_buf = 0;

	if (!prof_opt)
		return;
	if (!prof_port) {
		if (!prof_alloc_unit)
			prof_alloc_unit = PROF_ALLOC_UNIT;
		samples_per_buf = prof_alloc_unit -
		                  (int)&(sample_buf->samples);
		samples_per_buf /= sizeof (unsigned int);
				 
		if (debug)
			printf("prof_init, %d samples ber buf\n",
			       samples_per_buf);

		sample_bufs = alloc_sample_buf();
		kernel_task = get_kernel_task();
		MACH_CALL(mach_port_allocate, (mach_task_self(),
					       MACH_PORT_RIGHT_RECEIVE,
					       &prof_port));
		new_thread(&prof_thread, (vm_offset_t)prof_thread_body, 
					 (vm_offset_t)0);
		MACH_CALL( mach_port_insert_right, (mach_task_self(),
						    prof_port,
						    prof_port,
						    MACH_MSG_TYPE_MAKE_SEND));
	}
	prof_reset();
}

sample_buf_t
next_samples_buf(sbp)
sample_buf_t sbp;
{
	if (!sbp->next)
		sbp->next = alloc_sample_buf();
	sbp = sbp->next;
	sbp->free_count = samples_per_buf;
	sbp->next_sample = sbp->samples;
	return(sbp);
}

prof_terminate()
{
}

receive_notices(reply_port, samples, count, option)
mach_port_t reply_port;
unsigned *samples;
unsigned count;
unsigned option;
{
	return (receive_samples(reply_port, samples, count));
}

