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
#include <device/device.h>
#include <device/tty_status.h>
#include <stdarg.h>
#include <termio.h>

#define CNTRL(x)	(x & 037)
#define INTR_CHAR 	CNTRL('c')

extern mach_port_t 	console_port;		/* Exported by libsa_mach */
mach_port_t		console_reply_port = MACH_PORT_NULL;	/* Where to
						 * receive characters */
volatile boolean_t      char_wait = TRUE;	/* Are we waiting chars ? */
volatile boolean_t	char_ready = TRUE;	/* Is a character ready ? */
char			cur_char;		/* last read character */
boolean_t		console_active = FALSE;
volatile boolean_t	cons_sync_mode = TRUE;

security_token_t standalone_token;


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

/*
 * Used unbuffered I/Os. (default for libsa_mach)
 */

unbuffered_output() {
}


/*
 * We can not use libsa_mach. We want to do both
 * synchronous and asynchronous I/Os. 
 */

static char
unix_read_char() {
        struct termio s, saved;
        char c;

        (void)ioctl(0, TCGETA, &s);
        saved = s;
        s.c_lflag &= ~(ICANON);
        s.c_cc[VMIN] = 1;
        (void)ioctl(0, TCSETAW, &s);
        read(0, &c, 1);
        (void)ioctl(0, TCSETAW, &saved);
        return(c);
}


/*
 * Read character
 */

getchar() {
  	char c;

	if(!standalone) {
		return(unix_read_char(0, &c, 1)) ;
	}
	else {
		char_ready = FALSE;

		while (!char_ready)
			thread_switch(MACH_PORT_NULL,
				      SWITCH_OPTION_DEPRESS, 0);
		return(cur_char);
	}
}

read_char() {
	return(getchar());
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
		(void) device_read_inband(console_port, 0, 0,
					  1, (char *)&c, &count);
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

/*
 * Copied code from libsa_mach/printf.c
 * getchar() as defined there does not fit our needs.
 */

mach_thread_t	console_thread;
mach_port_t 	console_port;

console_init()
{
	if(standalone) {
/* MOD */
		MACH_CALL(device_open,
			(master_device_port,
			MACH_PORT_NULL, 
			D_WRITE, 
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
	

}

#define	PRINTF_BUFMAX	128

int printf_bufmax = PRINTF_BUFMAX;

struct printf_state {
	char buf[PRINTF_BUFMAX + 1]; /* extra for '\r\n' */
	unsigned int index;
};

static void
flush(struct printf_state *state)
{
	int amt;
	int done = 0;

	if(!standalone)
		return ;
	while (done != state->index) {
		if (device_write_inband(console_port,
					   0,
					   0,
					   state->buf+done,
					   state->index-done,
					   &amt) != KERN_SUCCESS)
			done += state->index-done;
		else
			done += amt;
	}
#ifdef	TTY_DRAIN
	{
		int word;
		/* flush console output */
		(void) device_set_status(console_port, TTY_DRAIN, &word, 0);
	}
#endif	/* TTY_DRAIN */
	state->index = 0;
}

static void
putchar(void *arg, char c)
{
	struct printf_state *state = (struct printf_state *) arg;

	if(!standalone) {
		write(1, &c, 1) ;
		return ;
	}
	if (c == '\n') {
	    state->buf[state->index] = '\r';
	    state->index++;
	}
	state->buf[state->index] = c;
	state->index++;

	if (state->index >= printf_bufmax)
	    flush(state);
}

/*
 * Printing (to console)
 */
void
vprintf(const char *fmt, va_list args)
{
	struct printf_state state;

	state.index = 0;
	doprnt(fmt, args, 0, putchar, (char *) &state);

	if (state.index != 0)
	    flush(&state);
}

#undef printf

/*VARARGS1*/
void
printf(const char *fmt, ...)
{
	va_list	args;

	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

