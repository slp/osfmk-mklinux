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
 * OLD HISTORY
 * Revision 1.1.16.1  1996/03/27  13:23:31  yp
 * 	Fix CR-1358. First character typed in was eaten since count was
 * 	not initialized to the size of the buffer (device_read_inband())
 * 	and confused MIG marshalling. After a first pass, count was
 * 	correctly set but character "eaten".
 * 	[96/03/26            yp]
 * 
 * 	Moved generic getchar(), putchar, printf, vprintf to lib/cons.c.
 * 	[96/03/20            yp]
 * 
 * 	Moved standalone_token to lib/test.c.
 * 	[96/03/14            yp]
 * 
 * Revision 1.1.7.4  1996/01/25  11:33:11  bernadat
 * 	use char type for putchar 2nd arg.
 * 	[96/01/03            bernadat]
 * 
 * 	Accept ? to print console command usage
 * 	[95/12/21            bernadat]
 * 
 * Revision 1.1.7.3  1995/11/03  17:09:28  bruel
 * 	Improved console thread & getchar synchronization.
 * 	Cut & paste at prompt used to be verrrrry slow.
 * 	[95/10/26            bernadat]
 * 	[95/11/03            bruel]
 * 
 * Revision 1.1.7.2  1995/05/14  19:58:01  dwm
 * 	ri-osc CR1304 - merge (nmk19_latest - nmk19b1) diffs into mainline.
 * 	a couple of casts to quiet warnings.
 * 	[1995/05/14  19:47:58  dwm]
 * 
 * Revision 1.1.7.1  1995/04/12  12:27:45  madhu
 * 	copyright marker not _FREE
 * 	[1995/04/11  14:53:25  madhu]
 * 
 * Revision 1.1.2.3  1995/01/11  10:34:32  madhu
 * 	Fixed OSF/1 version of mach_perf to be in non-canonical mode.
 * 	[1995/01/11  10:05:12  madhu]
 * 
 * Revision 1.1.2.2  1994/12/09  14:03:21  madhu
 * 	changes by Nick for synthetic tests.
 * 	changes to enable mach_perf to run as second server.
 * 	[1994/12/09  13:48:27  madhu]
 * 
 * 	changes for running on top of Unix.
 * 
 * Revision 1.1.2.1  1994/09/21  08:23:05  bernadat
 * 	Fixed console_thread_body. Was failing when typing
 * 	ahead to fast.
 * 	[94/09/21            bernadat]
 * 
 * 	Initial revision.
 * 	Pulled out from sa.c
 * 	Added a special thread for asynchronous reads.
 * 	[1994/09/19  07:10:02  bernadat]
 * 
 */

#include <mach_perf.h>
#include <device/device.h>
#include <device/tty_status.h>
#include <stdarg.h>

#define CNTRL(x)	(x & 037)
#define INTR_CHAR 	CNTRL('c')

extern mach_port_t 	console_port;		/* Exported by libsa_mach */
extern boolean_t	console_active;
extern security_token_t standalone_token;
mach_port_t		console_reply_port = MACH_PORT_NULL;	/* Where to
						 * receive characters */
extern volatile boolean_t char_wait;		/* Are we waiting chars ? */
extern volatile boolean_t char_ready;		/* Is a character ready ? */
extern char		cur_char;		/* last read character */
volatile boolean_t	cons_sync_mode = TRUE;

/*
 * console command. Used to open alternate devices
 */

console(argc, argv)
char *argv[];
{
	mach_port_t port;

	if (argc != 2 || !strcmp(argv[1],"?")) {
		printf("syntax: console <device_name>\n");
		return;
	}

	if(!standalone)  {
		printf("console - This option is not available when you are running with the Unix server\n");
		return ;
	}
	get_security_token(&standalone_token);
	MACH_CALL(device_open,
		(master_device_port,
		MACH_PORT_NULL, 0, standalone_token, argv[1], &port));

	MACH_CALL(device_close, (console_port));
	if (console_port != port) {
	  	vm_opt = 1; /* get vm stats printed again */
		console_port = port;
		kernel_version();
		print_vm_stats();
	}
}

console_sync_mode(boolean_t on) {
	cons_sync_mode = on;
}

console_thread_body(port)
mach_port_t port;
{
	unsigned int count;
	unsigned char c;

	if (debug)
		printf("console_thread_body\n");

	for(;;) {
		kern_return_t kr;
		count = sizeof c;
		kr = device_read_inband(console_port, 0, 0,
					  sizeof c, (char *)&c, &count);
		if (kr != KERN_SUCCESS) {
			printf("console_thread_body: device_read error = %x\n",
			       kr);
			continue;	/* XXX? */
		}
		if (cons_sync_mode)
			while(char_ready)
				thread_switch(MACH_PORT_NULL,
					      SWITCH_OPTION_DEPRESS, 0);
		else if (c == INTR_CHAR) {
			printf("\nInterrupt\n");
			interrupt_cmd();
		}
		cur_char = c;
		char_ready = TRUE;
		thread_switch(MACH_PORT_NULL,
			      SWITCH_OPTION_DEPRESS, 0);
	}
}

mach_thread_t	console_thread;

console_init()
{
	if(standalone) {
		MACH_CALL(device_open,
			(master_device_port,
			MACH_PORT_NULL, 
			D_WRITE | D_READ, 
			standalone_token, 
			(char *)"console",
			&console_port));
		new_thread(&console_thread, console_thread_body, 0);
		if (debug)
			printf("console_init - mach version\n");
	}
	else 
		if (debug)
			printf("console_init - unix version\n");
	
	console_active = TRUE;
}
