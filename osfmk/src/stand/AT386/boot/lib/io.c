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
/* CMU_HIST */
/*
 * Revision 2.1.2.1  92/04/30  11:53:47  bernadat
 * 	Copied from main line
 * 	[92/04/21            bernadat]
 * 
 * Revision 2.2  92/04/04  11:35:57  rpd
 * 	Fixed for IBM L40's A20 initialization.
 * 	[92/03/30            rvb]
 * 
 * 	Created.
 * 	[92/03/30            mg32]
 * 
 */
/* CMU_ENDHIST */
/*
 * Mach Operating System
 * Copyright (c) 1992, 1991 Carnegie Mellon University
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
 */

#include "boot.h"
#include <stdarg.h>

#include "bios.h"

#define K_RDWR 		0x60		/* keyboard data & cmds (read/write) */
#define K_STATUS 	0x64		/* keyboard status */
#define K_CMD	 	0x64		/* keybd ctlr command (write-only) */

#define K_OBUF_FUL 	0x01		/* output buffer full */
#define K_IBUF_FUL 	0x02		/* input buffer full */

#define KC_CMD_WIN	0xd0		/* read  output port */
#define KC_CMD_WOUT	0xd1		/* write output port */
#define KB_A20		0x9f		/* enable A20,
					   enable output buffer full interrupt
					   enable data line
					   disable clock line */

/*
 * Should be imported with comreg.h
 */

#define TXRX(addr)	(addr + 0)
#define BAUD_LSB(addr)	(addr + 0)
#define BAUD_MSB(addr)	(addr + 1)
#define INTR_ENAB(addr)	(addr + 1)
#define INTR_ID(addr)	(addr + 2)
#define LINE_CTL(addr)	(addr + 3)
#define MODEM_CTL(addr)	(addr + 4)
#define LINE_STAT(addr)	(addr + 5)
#define MODEM_STAT(addr)(addr + 6)

#define COM0_ADDR 0x3f8

/* line status register */
#define		iDR	0x01		/* data ready */
#define		iOR	0x02		/* overrun error */
#define		iPE	0x04		/* parity error */
#define		iFE	0x08		/* framing error */
#define		iBRKINTR 0x10		/* a break has arrived */
#define		iTSRE	0x40		/* tx shift reg is now empty */
#define		iTHRE	0x20		/* tx hold reg is now empty */

/* modem control register */
#define		iDTR		0x01	/* data terminal ready */
#define		iRTS		0x02	/* request to send */
#define		iOUT1		0x04	/* COM aux line -not used */
#define		iOUT2		0x08	/* turns intr to 386 on/off */	
#define		iLOOP		0x10	/* loopback for diagnostics */

#define CNTRL(x)	(x & 037)
#define INTR_CHAR 	CNTRL('c')
#define BELL_CHAR 	CNTRL('g')

static void 	do_putc(int);
static int 	do_getc(void);
static int 	is_com_char(void);
static int 	is_char(void);

void	putchar(int);
int 	cons_is_com;
int 	com_enabled;
int 	com_initialized;
int	prompt;			/* prompt, no automatic boot */
int	delayprompt;

/*
 * Gate A20 for high memory
 */
void
gateA20(void)
{
	while (inb(K_STATUS) & K_IBUF_FUL);
	while (inb(K_STATUS) & K_OBUF_FUL)
		(void)inb(K_RDWR);

	outb(K_CMD, KC_CMD_WOUT);
	while (inb(K_STATUS) & K_IBUF_FUL);
	outb(K_RDWR, KB_A20);
	while (inb(K_STATUS) & K_IBUF_FUL);
}

/*
 * printf
 *
 * only handles %d as decimal, %x as hex, %c as char, and %s as string
 */

void
printf(const char *format, ...)
{
	va_list ap;
	char c;

	va_start(ap, format);
	while (c = *format++) {
		if (c != '%')
			putchar(c);
		else {
			switch (c = *format++) {
			      case 'd': {
				      int num;
				      char buf[10], *ptr = buf;
				      num = va_arg(ap, int);
				      if (num<0) {
					      num = -num;
					      putchar('-');
				      }
				      do
					      *ptr++ = '0'+num%10;
				      while (num /= 10);
				      do
					      putchar(*--ptr);
				      while (ptr != buf);
				      break;
			      }
			      case 'x': {
				      unsigned int num, dig;
				      char buf[8], *ptr = buf;
				      num = va_arg(ap, unsigned int);
				      do
					      *ptr++ = (dig=(num&0xf)) > 9?
							'a' + dig - 10 :
							'0' + dig;
				      while (num >>= 4);
				      do
					      putchar(*--ptr);
				      while (ptr != buf);
				      break;
			      }
			      case 'c': {
				      int ch;
				      ch = va_arg(ap, int);
				      putchar(ch&0xff);
				      break;
			      }
			      case 's': {
				      char *ptr;
				      ptr = va_arg(ap, char *);
				      while (c = *ptr++)
					      putchar(c);
				      break;
			      }
			}
		}
	}
	va_end(ap);
}

void
do_putc(int c)
{
 	putc(c); 
	if (com_enabled) {
		if (!com_initialized ) {
  			com_initialized = 1;
			com_init(COM_CS8|COM_9600); 
			outb(MODEM_CTL(COM0_ADDR),iDTR|iRTS|iOUT2);

		}	
		com_putc(c);
	}
}

int
do_getc(void)
{
	for (;;) {
		if (ischar()) {
			if (com_enabled)
				cons_is_com = 0;
			return(getc());
		} else if (com_enabled && is_com_char()) {
			cons_is_com = 1;
			return(com_getc());
		}
	}
}

int
is_char(void)
{
	return(ischar() || (com_enabled && is_com_char()));
}

void
putchar(int c)
{
	if (c == '\n')
		do_putc('\r');
	do_putc(c);
}

int
getchar(void)
{
	int c;

	if ((c=do_getc()) == '\r')
		c = '\n';
	if (c == '\b') {
		putchar('\b');
		putchar(' ');
	}
	putchar(c);
	return(c);
}



/*
 * detect INTR_CHAR.
 */

int
is_intr_char(void)
{
	if ((ischar() && getc() == INTR_CHAR)
	    || (com_enabled && (is_com_char() && com_getc() == INTR_CHAR))
	    ) {
		printf("^C");
		return(1);
	}
	return(0);
}

#define ONE_SECOND	18	/* Exact value is 18.2 */

void
gets(char *buf)
{
	int	i = 0;
	unsigned int tk;
	char *ptr=buf;

	if (com_enabled)
		cons_is_com = 1;

	set_ticks(0);
	read_ticks(&tk);
	while (prompt || tk < delayprompt * ONE_SECOND) {
		if (i = is_char())
			break;
		read_ticks(&tk);
	}

	if (!i) {
		*ptr = 0;
		putchar('\n');
		return;
	}

	for (;;) {
		switch(*ptr = getchar() & 0xff) {
		case '\n':
		case '\r':
			*ptr = 0;
			return;
		case '\b':
		case 0177:
			if (ptr > buf) ptr--;
			break;
		default:
			ptr++;
			break;
		}
	}
}

int
strncmp(const char *s1, const char *s2, size_t n)
{
	if (n == 0)
		return 0;
	while (*s1 == *s2++) {
		if (*s1++ == 0)
			return 0;
		if (--n == 0)
			return 0;
	}
	return (*s1 - *(s2 - 1));
}

int
strlen(const char *s)
{
	int n = 0;

	while (*s++)
		n++;
	return n;
}

void
bcopy(const char *f, char *t, int len)
{
	const char *from = (const char *) f;
	char *to = (char *) t;

	while (len-- > 0)
		*to++ = *from++;
}

void
com_putc(int c) 
{
	register i;
	for (i=0; (!(inb(LINE_STAT(COM0_ADDR)) & iTHRE)) && (i < 1000); i++);
	outb(TXRX(COM0_ADDR),  c);
}

int
com_getc()
{
	unsigned char	c = 0;

	while (!(inb(LINE_STAT(COM0_ADDR)) & iDR));
	c = inb(TXRX(COM0_ADDR));
	return(c);
}


static int
is_com_char(void)
{
	return (inb(LINE_STAT(COM0_ADDR)) & iDR);
}

void
flush_char(void)
{
	while(is_char())
		getchar();
	com_putc(BELL_CHAR);
	com_putc(BELL_CHAR);
}

