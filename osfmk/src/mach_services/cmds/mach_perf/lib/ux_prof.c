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
#include <sys/types.h>
#include <stdio.h>
#include <sys/signal.h>
#include "prof_ctrl.h"

/*
 * Mprof needs a time_value_t record first
 * struct time_value {
 *      long    seconds;
 *      long    microseconds;
 * };
 *
 * then just a list of PCs (unsigned ints).
 */

/*
 * Interfaces used to print profile output.
 *
 * When running under Ux, mach_perf uses the mprof command.
 * This cannot be done in a mach_forked() task (unknown to the Ux server).
 * Instead a thread in the master task is dedicated to it.
 * These interfaces are used to communicate with this thread and
 * start the mprof command.
 */

extern kern_return_t 	do_start_prof_cmd(
					  mach_port_t 	server,
					  char 		*command,
					  unsigned int 	len);

extern kern_return_t 	do_write_to_prof_cmd(
					     mach_port_t 	server,
					     char 		*buf,
					     unsigned int 	len);

extern kern_return_t 	do_stop_prof_cmd(
					 mach_port_t server);

struct prof_stubs {
	kern_return_t 	(*start_prof_cmd)();
	kern_return_t 	(*write_to_prof_cmd)(mach_port_t 	server,
					     char 		*buf,
					     unsigned int	len);
	kern_return_t	(*stop_prof_cmd)();
};

/*
 * call_write_to_prof_cmd uses a buf_t arg
 * with a fixed maximum size (PROF_CMD_BUF_LENGTH)
 * Need to split larger buffers.
 */

kern_return_t 
write_to_prof_cmd(
	mach_port_t	server,
	buf_t		buf,
	unsigned int	len)
{
	unsigned int 	n;

	while (len > 0) {
		n = (len < PROF_CMD_BUF_LENGTH) ? len : PROF_CMD_BUF_LENGTH;
		MACH_CALL(call_write_to_prof_cmd, (server,
						   buf,
						   n));
		len -= n;
		buf += n;
	}
	return(KERN_SUCCESS);
}

/* 
 * Prof command stubs used when running individual mach_perf packages
 * (i.e no mach_forked task, can use popen directly)
 */

struct prof_stubs ux_prof_stubs = {
       do_start_prof_cmd,
       do_write_to_prof_cmd,
       do_stop_prof_cmd
};

/* 
 * Prof command stubs used when running mach_perf monotor
 * (i.e mach_forked task, use RPCs with master task)
 */

struct prof_stubs mach_prof_stubs = {
       call_start_prof_cmd,
       write_to_prof_cmd,
       call_stop_prof_cmd
};


time_value_t            prof_time_start, prof_time_stop;
	
ux_prof_print()
{
	FILE *input;
	register i;
	struct sample_buf *buf;
	struct prof_stubs *prof_stubs;

	if (!prof_opt)
		return;

	if (prof_cmd_port) {
		if (debug)
			printf("mach_prof_stubs\n");
		prof_stubs = &mach_prof_stubs;
	} else {
		if (debug)
			printf("ux_prof_stubs\n");
		prof_stubs = &ux_prof_stubs;
	}

	prof_time_stop.seconds = client_stats.total.xsum / NSEC_PER_SEC;
	prof_time_stop.microseconds = client_stats.total.xsum -
	  				prof_time_stop.seconds * NSEC_PER_SEC;

	if (debug > 1)
		printf("prof_print(): %s\n", prof_command);

	MACH_CALL((*prof_stubs->start_prof_cmd), (prof_cmd_port,
						  prof_command,
						  strlen(prof_command)+1));

	if (debug > 1)
		printf("writing start time\n");


	MACH_CALL((*prof_stubs->write_to_prof_cmd), (prof_cmd_port,
						     (char *)&prof_time_start,
						     sizeof(prof_time_start)));

	for (i = sample_count, buf = sample_bufs; i; buf = buf->next) {
		int count;

		count = samples_per_buf - buf->free_count;
		if (debug > 1)
			printf("prof_print writes %d samples from %x\n", count, buf->samples);
		MACH_CALL((*prof_stubs->write_to_prof_cmd),
			  		(prof_cmd_port,
					 (char *)buf->samples,
					 count*sizeof(unsigned int)));
		i -= count;
	}
	if (debug > 1)
		printf("writing stop time\n");

	MACH_CALL((*prof_stubs->write_to_prof_cmd), (prof_cmd_port,
						     (char *)&prof_time_stop,
						     sizeof(prof_time_stop)));
	
	MACH_CALL((*prof_stubs->stop_prof_cmd), (prof_cmd_port));

	printf("\n");
}

FILE *prof_input;

kern_return_t
do_start_prof_cmd(
	mach_port_t	server,
	char 		*command,
	unsigned int	len)
{
	if (debug)
		printf("do_start_prof_cmd(%s)\n", command);

	if ((prof_input = popen(command, "w")) == (FILE *)NULL) {
		perror(command);
		exit(1);
	}
        if (setvbuf(prof_input, 0, _IONBF, 0) == -1)
               	perror("setvbuf");
	if (debug)
		printf("prof_input %d\n", prof_input);

	return(KERN_SUCCESS);
}

kern_return_t
do_write_to_prof_cmd(
	mach_port_t	server,
	char 		*buf,
	unsigned int	len)
{
	if (debug)
		printf("do_write_to_prof_cmd(%x, %d)\n", buf, len);

	if (fwrite(buf, len, 1, prof_input) != 1) {
		perror("fwrite");
		return(KERN_FAILURE);
	} 
	return(KERN_SUCCESS);
}

kern_return_t
do_stop_prof_cmd(
	mach_port_t	server)
{
	if (debug)
		printf("do_stop_prof_cmd()\n");

	pclose(prof_input);
	return(KERN_SUCCESS);
}

prof_cmd_thread_body(port)
mach_port_t port;
{
	extern boolean_t prof_ctrl_server();

	MACH_CALL(mach_msg_server, (prof_ctrl_server,
				    PROF_CMD_BUF_LENGTH*2,
				    port,
				    MACH_MSG_OPTION_NONE));
}

mach_thread_t	prof_cmd_thread;

void
ux_prof_init(void)
{
	if (standalone)
		return;
	
	MACH_CALL( mach_port_allocate, (mach_task_self(),
					MACH_PORT_RIGHT_RECEIVE,
					&prof_cmd_port));
	if (debug > 1)
		printf("prof_cmd_port %x\n", prof_cmd_port);

	new_thread(&prof_cmd_thread,
		   (vm_offset_t)prof_cmd_thread_body,
		   (vm_offset_t)prof_cmd_port);
	
	MACH_CALL( mach_port_insert_right, (mach_task_self(),
					    prof_cmd_port,
					    prof_cmd_port,
					    MACH_MSG_TYPE_MAKE_SEND));
}
