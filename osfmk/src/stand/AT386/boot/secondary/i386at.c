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
 * File : i386at.c
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains i386at specific procedures of the Network bootstrap.
 */

#include <secondary/net.h>

#define	TICKS_PER_10SEC	182

static unsigned	i386at_timeout;
static unsigned	i386at_random;

/*
 * Calibration delay counts.
 */
unsigned int	delaycount = 10;

#ifdef DEBUG
extern int debug;
#endif

void
timerinit()
{
	read_ticks(&i386at_random);
}

void
settimeout(unsigned msecs)
{
	set_ticks(0);
	i386at_timeout = TICKS_PER_10SEC * msecs;
#ifdef	DEBUG
	if (debug)
		printf("Set new timeout: 0x%x\n", i386at_timeout);
#endif
}

unsigned
getelapsed(void)
{
	unsigned val;

	read_ticks(&val);
	return ((val * 10000) / TICKS_PER_10SEC);
}

int
isatimeout(void)
{
	unsigned val;

	read_ticks(&val);
	return (val * 10000 >= i386at_timeout);
}

unsigned
rand(void)
{
	i386at_random = i386at_random * 1103515245 + 12345;
	return (i386at_random);
}


/*
 * measure_delay(microseconds)
 *
 * Measure elapsed time for delay calls
 * Returns microseconds.
 * 
 */

/* The following should come from <i386/pit.h> */
/* Definitions for 8254 Programmable Interrupt Timer ports on AT 386 */
#define PITCTR0_PORT	0x40		/* counter 0 port */	
#define PITCTL_PORT	0x43		/* PIT control port */

/* Following are used for Timer 0 */
#define PIT_C0          0x00            /* select counter 0 */
#define PIT_LOADMODE	0x30		/* load least significant byte followed
					 * by most significant byte */
#define PIT_NDIVMODE	0x04		/* divide by N counter */

/*
 * Clock speed for the timer in hz divided by the constant HZ
 * (defined in param.h)
 */
#define CLKNUM		1193167

int
measure_delay(
	int us)
{
	unsigned int	val, lsb;

	outb(PITCTL_PORT, PIT_C0|PIT_NDIVMODE|PIT_LOADMODE);
	outb(PITCTR0_PORT, 0xff);	/* set counter to max value */
	outb(PITCTR0_PORT, 0xff);
	delay(us);
	outb(PITCTL_PORT, PIT_C0);
	lsb = inb(PITCTR0_PORT);
	val = (inb(PITCTR0_PORT) << 8) | lsb;
	val = 0xffff - val;
	val *= 1000000;
	val /= CLKNUM;

	return(val);
}

/*
 * calibrate_delay(void)
 *
 * Adjust delaycount.
 */

void
calibrate_delay(void)
{
	unsigned 	val;
	register	i;

	for (i=0; i<10; i++) {
 		val = measure_delay(50);	/* take the time for 50us */
		if (val == 0) {
			delaycount <<=1;	/* insufficient delaycount */
		} else {
			delaycount *= 50;
			delaycount += val-1; 	/* round up to upper us */
			delaycount /= val;
		}
		if (delaycount <= 0)
			delaycount = 1;
	}
}

void
delayed_outb(int d, int a)
{
	outb(d,a);
	delay(5);	/* 5 usecs */
}

