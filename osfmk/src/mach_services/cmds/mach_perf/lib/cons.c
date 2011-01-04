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

mach_port_t 		console_port = MACH_PORT_NULL;
boolean_t		console_active = FALSE;

volatile boolean_t      char_wait = TRUE;	/* Are we waiting chars ? */
volatile boolean_t	char_ready = FALSE;	/* Is a character ready ? */
char			cur_char;		/* last read character */

/*
 * Copied code from libsa_mach/printf.c
 * getchar() as defined there does not fit our needs.
 */

/*
 * Read character
 */
getchar() {
  	char c;

	if(!standalone) {
		return(unix_read_char(0, &c, 1)) ;
	}
	else {
		while (!char_ready)
			thread_switch(MACH_PORT_NULL,
				      SWITCH_OPTION_DEPRESS, 0);
		char_ready = FALSE;
		return(cur_char);
	}
}

read_char() {
	return(getchar());
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

	if (!standalone) {
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

