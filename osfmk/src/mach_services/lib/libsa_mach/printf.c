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
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * MkLinux
 */

#define EXPORT_BOOLEAN
#include <mach/boolean.h>
#include <mach.h>
#include <sa_mach.h>
#include <device/device.h>
#include <device/tty_status.h>
#include <mach/bootstrap.h>
#include <stdarg.h>
#include <stdio.h>

/*
 *  Common code for printf et al.
 *
 *  The calling routine typically takes a variable number of arguments,
 *  and passes the address of the first one.  This implementation
 *  assumes a straightforward, stack implementation, aligned to the
 *  machine's wordsize.  Increasing addresses are assumed to point to
 *  successive arguments (left-to-right), as is the case for a machine
 *  with a downward-growing stack with arguments pushed right-to-left.
 *
 *  This version implements the following printf features:
 *
 *	%d	decimal conversion
 *	%u	unsigned conversion
 *	%x	hexadecimal conversion
 *	%X	hexadecimal conversion with capital letters
 *	%o	octal conversion
 *	%c	character
 *	%s	string
 *	%m.n	field width, precision
 *	%-m.n	left adjustment
 *	%0m.n	zero-padding
 *	%*.*	width and precision taken from arguments
 *
 *  This version does not implement %f, %e, or %g.  It accepts, but
 *  ignores, an `l' as in %ld, %lo, %lx, and %lu, and therefore will not
 *  work correctly on machines for which sizeof(long) != sizeof(int).
 *  It does not even parse %D, %O, or %U; you should be using %ld, %o and
 *  %lu if you mean long conversion.
 *
 *  As mentioned, this version does not return any reasonable value.
 *
 *  Permission is granted to use, modify, or propagate this code as
 *  long as this notice is incorporated.
 *
 *  Steve Summit 3/25/87
 */

/*
 * Added formats for decoding device registers:
 *
 * printf("reg = %b", regval, "<base><arg>*")
 *
 * where <base> is the output base expressed as a control character:
 * i.e. '\10' gives octal, '\20' gives hex.  Each <arg> is a sequence of
 * characters, the first of which gives the bit number to be inspected
 * (origin 1), and the rest (up to a control character (<= 32)) give the
 * name of the register.  Thus
 *	printf("reg = %b\n", 3, "\10\2BITTWO\1BITONE")
 * would produce
 *	reg = 3<BITTWO,BITONE>
 *
 * If the second character in <arg> is also a control character, it
 * indicates the last bit of a bit field.  In this case, printf will extract
 * bits <1> to <2> and print it.  Characters following the second control
 * character are printed before the bit field.
 *	printf("reg = %b\n", 0xb, "\10\4\3FIELD1=\2BITTWO\1BITONE")
 * would produce
 *	reg = b<FIELD1=2,BITONE>
 */
/*
 * Added for general use:
 *	#	prefix for alternate format:
 *		0x (0X) for hex
 *		leading 0 for octal
 *	+	print '+' if positive
 *	blank	print ' ' if positive
 *
 *	z	signed hexadecimal
 *	r	signed, 'radix'
 *	n	unsigned, 'radix'
 *
 *	D,U,O,Z	same as corresponding lower-case versions
 *		(compatibility)
 */

#define isdigit(d) ((d) >= '0' && (d) <= '9')
#define Ctod(c) ((c) - '0')

#define MAXBUF (sizeof(long int) * 8)		 /* enough for binary */

static void
printnum(register unsigned long	u,	/* number to print */
	 register int		base,
	 void			(*outc)(void *, int),
	 void			*outc_arg)
{
	char	buf[MAXBUF];	/* build number here */
	register char *	p = &buf[MAXBUF-1];
	static char digs[] = "0123456789abcdef";

	do {
	    *p-- = digs[u % base];
	    u /= base;
	} while (u != 0);

	while (++p != &buf[MAXBUF])
	    (*outc)(outc_arg, *p);
}

static void
_doprnt(register const char *fmt,
	va_list		args,
	int		radix,			/* default radix - for '%r' */
 	void		(*outc)(void *, int),	/* character output */
	void		*outc_arg)		/* argument for outc */
{
	int		length;
	int		prec;
	boolean_t	ladjust;
	char		padc;
	long		n;
	unsigned long	u;
	int		plus_sign;
	int		sign_char;
	boolean_t	altfmt;
	int		base;
	char		c;

	while (*fmt != '\0') {
	    if (*fmt != '%') {
		(*outc)(outc_arg, *fmt++);
		continue;
	    }

	    fmt++;

	    length = 0;
	    prec = -1;
	    ladjust = FALSE;
	    padc = ' ';
	    plus_sign = 0;
	    sign_char = 0;
	    altfmt = FALSE;

	    while (TRUE) {
		if (*fmt == '#') {
		    altfmt = TRUE;
		    fmt++;
		}
		else if (*fmt == '-') {
		    ladjust = TRUE;
		    fmt++;
		}
		else if (*fmt == '+') {
		    plus_sign = '+';
		    fmt++;
		}
		else if (*fmt == ' ') {
		    if (plus_sign == 0)
			plus_sign = ' ';
		    fmt++;
		}
		else
		    break;
	    }

	    if (*fmt == '0') {
		padc = '0';
		fmt++;
	    }

	    if (isdigit(*fmt)) {
		while(isdigit(*fmt))
		    length = 10 * length + Ctod(*fmt++);
	    }
	    else if (*fmt == '*') {
		length = va_arg(args, int);
		fmt++;
		if (length < 0) {
		    ladjust = !ladjust;
		    length = -length;
		}
	    }

	    if (*fmt == '.') {
		fmt++;
		if (isdigit(*fmt)) {
		    prec = 0;
		    while(isdigit(*fmt))
			prec = 10 * prec + Ctod(*fmt++);
		}
		else if (*fmt == '*') {
		    prec = va_arg(args, int);
		    fmt++;
		}
	    }

	    if (*fmt == 'l')
		fmt++;	/* need it if sizeof(int) < sizeof(long) */

	    switch(*fmt) {
		case 'b':
		case 'B':
		{
		    register const char *p;
		    boolean_t	  any;
		    register int  i;

		    u = va_arg(args, unsigned long);
		    p = va_arg(args, const char *);
		    base = *p++;
		    printnum(u, base, outc, outc_arg);

		    if (u == 0)
			break;

		    any = FALSE;
		    while (i = *p++) {
			if (*p <= 32) {
			    /*
			     * Bit field
			     */
			    register int j;
			    if (any)
				(*outc)(outc_arg, ',');
			    else {
				(*outc)(outc_arg, '<');
				any = TRUE;
			    }
			    j = *p++;
			    for (; (c = *p) > 32; p++)
				(*outc)(outc_arg, c);
			    printnum(( (u>>(j-1)) & ((2<<(i-j))-1)),
					base, outc, outc_arg);
			}
			else if (u & (1<<(i-1))) {
			    if (any)
				(*outc)(outc_arg, ',');
			    else {
				(*outc)(outc_arg, '<');
				any = TRUE;
			    }
			    for (; (c = *p) > 32; p++)
				(*outc)(outc_arg, c);
			}
			else {
			    for (; *p > 32; p++)
				continue;
			}
		    }
		    if (any)
			(*outc)(outc_arg, '>');
		    break;
		}

		case 'c':
		    c = va_arg(args, int);
		    (*outc)(outc_arg, c);
		    break;

		case 's':
		{
		    register const char *p;
		    register const char *p2;

		    if (prec == -1)
			prec = 0x7fffffff;	/* MAXINT */

		    p = va_arg(args, char *);

		    if (p == (char *)0)
			p = "";

		    if (length > 0 && !ladjust) {
			n = 0;
			p2 = p;

			for (; *p != '\0' && n < prec; p++)
			    n++;

			p = p2;

			while (n < length) {
			    (*outc)(outc_arg, ' ');
			    n++;
			}
		    }

		    n = 0;

		    while (*p != '\0') {
			if (++n > prec)
			    break;

			(*outc)(outc_arg, *p++);
		    }

		    if (n < length && ladjust) {
			while (n < length) {
			    (*outc)(outc_arg, ' ');
			    n++;
			}
		    }

		    break;
		}

		case 'o':
		case 'O':
		    base = 8;
		    goto print_unsigned;

		case 'd':
		case 'D':
		case 'i':
		    base = 10;
		    goto print_signed;

		case 'u':
		case 'U':
		    base = 10;
		    goto print_unsigned;

		case 'p':
		    padc = '0';
		case 'x':
		case 'X':
		    base = 16;
		    goto print_unsigned;

		case 'z':
		case 'Z':
		    base = 16;
		    goto print_signed;

		case 'r':
		case 'R':
		    base = radix;
		    goto print_signed;

		case 'n':
		    base = radix;
		    goto print_unsigned;

		print_signed:
		    n = va_arg(args, long);
		    if (n >= 0) {
			u = n;
			sign_char = plus_sign;
		    }
		    else {
			u = -n;
			sign_char = '-';
		    }
		    goto print_num;

		print_unsigned:
		    u = va_arg(args, unsigned long);
		    goto print_num;

		print_num:
		{
		    char	buf[MAXBUF];	/* build number here */
		    register char *	p = &buf[MAXBUF-1];
		    static char digits[] = "0123456789abcdef";
		    const char *prefix = 0;

		    if (u != 0 && altfmt) {
			if (base == 8)
			    prefix = "0";
			else if (base == 16)
			    prefix = "0x";
		    }

		    do {
			*p-- = digits[u % base];
			u /= base;
		    } while (u != 0);

		    length -= (&buf[MAXBUF-1] - p);
		    if (sign_char)
			length--;
		    if (prefix)
			length -= strlen(prefix);

		    if (padc == ' ' && !ladjust) {
			/* blank padding goes before prefix */
			while (--length >= 0)
			    (*outc)(outc_arg, ' ');
		    }
		    if (sign_char)
			(*outc)(outc_arg, sign_char);
		    if (prefix)
			while (*prefix)
			    (*outc)(outc_arg, *prefix++);
		    if (padc == '0') {
			/* zero padding goes after sign and prefix */
			while (--length >= 0)
			    (*outc)(outc_arg, '0');
		    }
		    while (++p != &buf[MAXBUF])
			(*outc)(outc_arg, *p);

		    if (ladjust) {
			while (--length >= 0)
			    (*outc)(outc_arg, ' ');
		    }
		    break;
		}

		case '\0':
		    fmt--;
		    break;

		default:
		    (*outc)(outc_arg, *fmt);
	    }
	fmt++;
	}
}

mach_port_t console_port;
static security_token_t null_security_token;

void
printf_init(mach_port_t device_server_port)
{
	(void) device_open(device_server_port, MACH_PORT_NULL, D_WRITE,
			   null_security_token, (char *)"console",
			   &console_port);
}

#define	PRINTF_BUFMAX	128

int printf_bufmax = PRINTF_BUFMAX;

struct printf_state {
	char buf[PRINTF_BUFMAX + 1]; /* extra for '\r\n' */
	unsigned int index;
	unsigned int total;
};

/* Try to obtain the console port.  This is not guaranteed to work (we
   may not have a valid bootstrap port) so it is only called when we would
   otherwise have to drop a printf(). */
static void
get_console_port(void)
{
	kern_return_t kr;
	mach_port_t device_server_port, junk_port;
	mach_msg_type_number_t count;
	security_token_t token;

	if (bootstrap_port == MACH_PORT_NULL) {
	    kr = task_get_special_port(mach_task_self(), TASK_BOOTSTRAP_PORT,
				       &bootstrap_port);
	    if (kr != KERN_SUCCESS)
		return;
	}
	kr = bootstrap_ports(bootstrap_port, &junk_port, &device_server_port,
			     &junk_port, &junk_port, &junk_port);
	if (kr != KERN_SUCCESS)
	    return;
	count = TASK_SECURITY_TOKEN_COUNT;
	kr = task_info(mach_task_self(), TASK_SECURITY_TOKEN,
		       (task_info_t) &token, &count);
	if (kr != KERN_SUCCESS)
	    return;
	kr = device_open(device_server_port, MACH_PORT_NULL, D_READ|D_WRITE,
			 token, (char *) "console", &console_port);
}

static void
flush(struct printf_state *state)
{
	io_buf_len_t amt;
	int offset, count;
	kern_return_t kr;

	/*
	 * It is likely that not all characters will be written if
	 * the console is a slow serial device. Make sure we look at
	 * the 'amt' return value and make sure all characters have
	 * been written. 
	 */
	offset = 0;
	count = state->index;
	while (count) {
	    kr = device_write_inband(console_port, 0, 0,
				     &state->buf[offset], count, &amt);
	    if (kr != D_SUCCESS) {
		if (console_port == MACH_PORT_NULL) {
		    /* If the console port is null it's probably because
		       printf_init() hasn't been called.  This can happen
		       for instance if the thread-package startup function
		       needs to print an error message.  Rather than drop
		       the message on the floor, we attempt to fabricate a
		       console port and only if that fails do we discard
		       the message.  */
		    get_console_port();
		    if (console_port != MACH_PORT_NULL)
			continue;
		}
		break;
	    }
	    count -= amt;
	    offset += amt;
	    if (count) {
		/*
		 * We were unable to send all the data. Sleep for a little
		 * while, then try to send the rest.
		 */
		usleep(USEC_PER_SEC/4);  /* .25 second sleep */ 
	    }
	}
#ifdef	TTY_DRAIN
	{
		int word;
		/* flush console output */
		(void) device_set_status(console_port, TTY_DRAIN, &word, 0);
	}
#endif	/* TTY_DRAIN */
	state->total += state->index;
	state->index = 0;
}

static void
outchar(void *arg, int c)
{
	struct printf_state *state = (struct printf_state *) arg;

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
int
vprintf(const char *fmt, va_list args)
{
	struct printf_state state;

	state.index = state.total = 0;
	_doprnt(fmt, args, 0, outchar, (char *) &state);

	if (state.index != 0)
	    flush(&state);
	return (state.total);
}

/*VARARGS1*/
int
printf(const char *fmt, ...)
{
	int ret;
	va_list	args;

	va_start(args, fmt);
	ret = vprintf(fmt, args);
	va_end(args);
	return (ret);
}

static void
savechar(void *arg, int c)
{
	*(*(char **)arg)++ = c;
}

int
vsprintf(char *s, const char *fmt, va_list args)
{
	char *ss = s;
	_doprnt(fmt, args, 0, savechar, &s);
	*s = 0;
	return (s - ss);
}

/*VARARGS2*/
int
sprintf(char *s, const char *fmt, ...)
{
	int ret;
	va_list	args;

	va_start(args, fmt);
	ret = vsprintf(s, fmt, args);
	va_end(args);
	return (ret);
}

int
getchar(void)
{
	unsigned char ch;
	mach_msg_type_number_t count;

	/* initialise count so as not to cause problems in mig */
	count = sizeof(ch);
	(void) device_read_inband(console_port, 0, 0,
				  sizeof(ch), (char *)&ch, &count);
	return((int)ch);
}
