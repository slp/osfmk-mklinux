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
#include "printf_types.h"
#include <stdarg.h>

mach_port_t	printf_port = 0;

extern int gen_printf(const char *format, ...);

printf_thread_body(port)
mach_port_t port;
{
	extern boolean_t printf_server();

	if (debug)
		printf("printf_thread_body\n");
	MACH_CALL(mach_msg_server, (printf_server,
				    512,
				    port,
				    MACH_MSG_OPTION_NONE));
}

mach_thread_t	printf_thread;

_printf_init()
{
	if (debug)
		printf("_printf_init\n");

	if (is_master_task && !printf_port) {
		unbuffered_output(); 
		MACH_CALL( mach_port_allocate, (mach_task_self(),
				 MACH_PORT_RIGHT_RECEIVE,
				 &printf_port));
		if (debug > 1)
			printf("printf_port %x\n", printf_port);
		new_thread(&printf_thread,
			   (vm_offset_t)printf_thread_body,
			   (vm_offset_t)printf_port);
		printf_enable(mach_task_self());
	}
}

printf_enable(task)
mach_port_t task;
{
	if (is_master_task) {
		MACH_CALL( mach_port_insert_right, (task,
						    printf_port,
						    printf_port,
						    MACH_MSG_TYPE_MAKE_SEND));
	} else {
		/*
		 * printf possibly doesn't work at this stage (we wouldn't
		 * have called this routine otherwise :-).
		 * XXX Use exception_raise_state instead?
		 */
		printf("printf_enable called on non-master task.\n");
		exit(1);
	}
}

do_remote_printf(port, string, count) 
mach_port_t port;
char *string;
{
	printf("%s", string);
	return(KERN_SUCCESS);
}

char *blankline = "                                                                                ";

void
spaces(int n)
{
        if (n > 0)
		printf("%s", &blankline[80-n]);
}

#undef printf

/*
 * - We cant use printf from remote tasks.
 * - gen_printf() sends a message to the printf_thread in the case
 *   the current task is not the master one.
 * - The standalone library does not include float format, so we
 *   use our own version of doprnt via do_sprintf().
 * - The libmach_sa lib assumes printf strings are less than 128 bytes,
 *   so we must use a for() loop.
 */

int
gen_printf(const char *format, ...)
{
	va_list	args;

	char string[4096], *s, c;
	int length, l;
	boolean_t remote;

	va_start(args, format);
	do_sprintf(string, format, args);
	va_end(args);

	remote = ((!is_master_task) ||
			(printf_thread && thread_self() != printf_thread));

	for (length = strlen(string)+1, s = string;
	     length;
	     length -= l, s += l) {
		l = (length < PRINTF_LENGTH-1) ? length : PRINTF_LENGTH-1;
		c = *(s+l);
		*(s+l) = 0;
		if (!remote) {
                        if (!more(s))
                                return;
		} else { 
			remote_printf(printf_port, s, l+1);
		}
		*(s+l) = c;
	}
}

int max_lines = 22;
int lines = 1;

char read_char();
boolean_t	more_enabled = TRUE;
extern 		boolean_t console_active;
void		(*interrupt_func)(); 	       /* Used in standalone mode */

more(s)
char *s;
{
  	char c;
	char *l = s;

	while (*s) {
		while (*s && *s != '\n')
			s++;
		if (!*s) {
			printf("%s", l);
			return;
		}
		*s = 0;
		printf("%s\n", l);
		l = ++s;
		if (++lines >= max_lines && more_enabled && console_active) {
	  		lines = 0;
			printf("more ? ");
			c = read_char();
			printf("\r        \r");
			if ( c == 'q') {
				if (interrupt_func) {
					(*interrupt_func)();
					return(0);
				} else
					exit(0);
			}
		}
	}
	return(1);
}

#define MORE_ENABLE	0
#define MORE_DISABLE	1
#define MORE_RESET	2

void
disable_more()
{
	internal_more_ctrl(MORE_DISABLE);
}

void
enable_more()
{
	internal_more_ctrl(MORE_ENABLE);
}

void
reset_more()
{
	internal_more_ctrl(MORE_RESET);
}

void
do_more_ctrl(mach_port_t port, int op)
{
	switch(op) {
	case MORE_DISABLE:
		more_enabled = FALSE;
		break;
	case MORE_RESET:
		lines = 0;
        case MORE_ENABLE:
		more_enabled = TRUE;
		break;
	}
}

internal_more_ctrl(int op)
{
	if (is_master_task)
	  	do_more_ctrl(printf_port, op);
	else if (printf_port)
		more_ctrl(printf_port, op);
}



