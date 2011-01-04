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
 * Copyright 1992 by Open Software Foundation,
 * Cambridge, Massachusetts USA
 *
 * 		All Rights Reserved
 * 
 *   Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both the copyright notice and this permission notice appear in
 * supporting documentation, and that the name of OSF or Open Software
 * Foundation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.
 * 
 *   OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * CYCTM05 timer board driver.
 *
 * Board implements a set of 5 16 bit timers that can be configured in a
 * variety of ways.  In particular, the timers can be set to tick at a number
 * of intervals and be cascaded together to implement larger counters.
 * We will be using 4 timers for a 64 bit ticker incrementing at 1Mhz
 * (1,000,000/sec), and in the future, using the last timer as a high
 * resolution clock available through the new clock facility.
 */

#include <cyctm.h>
#if	NCYCTM > 0

#include <sys/types.h>
#include <kern/clock.h>
#include <kern/assert.h>
#include <kern/spl.h>
#include <kern/misc_protos.h>
#include <mach/clock_types.h>
#include <i386/pio.h>
#include <i386/AT386/cyctm05.h>
#include <i386/AT386/cyctm05_entries.h>

#define DEBUG

/*
 * Clock ops structure in case we use the interrupting capabilities to
 * generate a high resolution clock.
 */
struct clock_ops  cyctm05_ops = {
	cyctm05_config,	cyctm05_init,	cyctm05_gettime,cyctm05_settime,
	0,		0,		0,		0,
};

int		cyctm05_raw = 1;
boolean_t	cyctm05_initialized;

#define	LOCK_CYCTM05()		sploff()
#define	UNLOCK_CYCTM05(s)	splon(s)

/* Forward */

static int		cyctm05_step(
				int		counter,
				int		step_count);
static kern_return_t	get_clock(
				struct t64	*t64p);
static kern_return_t	set_clock(
				struct t64	*t64p);
static void		program_counter(
				int		counter,
				unsigned int	cmd);
extern void		cyctm05_stamp(
				struct t64	*t64p);
static kern_return_t	diff_clock(
				struct t64	*start,
				struct t64	*end,
				struct t64	*diff);
extern void		cyctm05_diff(
				struct t64	*start,
				struct t64	*end,
				struct t64	*diff);

/*
 * Device Interface
 */

/*
 * Structure if it was memory mapped.
 *
 * The amd9513 timer chip access is through the command/status and data
 * registers.  Internally, the chip has many registers, so commands
 * manipulate an internal data pointer register and then move the data to
 * that register.
 */

union cyctm05_reg {
	struct {
		volatile unsigned char	data;		/* data from 9513 */
		volatile unsigned char	status;		/* status from 9513 */
		volatile unsigned char	dig_input;
		volatile unsigned char	unused;
	} read;
	struct {
		volatile unsigned char	data;		/* data to 9513 */
		volatile unsigned char	command;	/* command to 9513 */
		volatile unsigned char	unused;
		volatile unsigned char	dig_output;
	} write;
};

#define BASE_PORT	0x300		/* XXX hard coded base I/O address */

/* Read regs */
#define DATA_PORT	(BASE_PORT+0)	/* 9513 data reg (read/write) */
#define STAT_PORT	(BASE_PORT+1)	/* 9513 status reg (read) */
#define CMD_PORT	(BASE_PORT+1)	/* 9513 command reg (write) */
#define DIG_IN_PORT	(BASE_PORT+2)	/* CYCTM digital input reg (read) */
#define DIG_OUT_PORT	(BASE_PORT+3)	/* CYCTM digital output reg (write) */

/*
 * All counter registers are 16 bits, but are set and read with 2 8 bit accesses.
 */

#define LSB(x)		((x) & 0xff)
#define MSB(x)		(((x) >> 8) & 0xff)

/*
 * Number of counters we need for our 64 bit ticker
 */
#define NCOUNTERS	5

/* Status reg definitions */

/* Command reg definitions:
 *
 * All commands are 1-byte.  Breakdown is
 *
 * Top 3 bits are command value
 * Bottom 5 bits are interpreted for each command
 *
 * For commands that operate on the counters, the bottom 5 bits are usually
 * a bit mask representing each counter that the command should affect.
 */

#define CMD_LDP		(0x00 << 5)	/* Load data pointer register */

#define CMD_GP_ILL0	(0x0)		/* Illegal Group */
/*
 * Counter group values
 */
#define CMD_GP_CNT1	(0x1)		/* Counter #1 */
#define CMD_GP_CNT2	(0x2)		/* Counter #2 */
#define CMD_GP_CNT3	(0x3)		/* Counter #3 */
#define CMD_GP_CNT4	(0x4)		/* Counter #4 */
#define CMD_GP_CNT5	(0x5)		/* Counter #5 */

/*
 * Element Pointer values for the Above counter Group
 */
#define CMD_EP_MODE	(0x0 << 3)	/* Mode register */
#define CMD_EP_LOAD	(0x1 << 3)	/* Load register */
#define CMD_EP_HOLD	(0x2 << 3)	/* Hold register */
/*
 * the HOLDCY element selection allows the Counter group to cycle instead
 * of the Element Pointer, enabling easy access to the saved values in the
 * Hold register
 */
#define CMD_EP_HOLDCY	(0x3 << 3)	/* Hold register, hold cycle increment */

#define CMD_GP_ILL6	(0x6)		/* Illegal Group */

#define CMD_GP_CNTL	(0x7)		/* Control Group */
/*
 * Element Pointer Values for Above Control Group
 */
#define CMD_EP_ALARM1	(0x0 << 3)	/* Alarm 1 */
#define CMD_EP_ALARM2	(0x1 << 3)	/* Alarm 2 */
#define CMD_EP_MM	(0x2 << 3)	/* Master Mode */
#define CMD_EP_STATUS	(0x3 << 3)	/* Status, no increment */

/*
 * Low order 5 bits for the following commands select counters as bit mask
 */
#define CMD_ARM		(0x1 << 5)	/* Arm counting registers */
#define CMD_LOAD	(0x2 << 5)	/* Load counting registers */
#define CMD_LOAD_ARM	(0x3 << 5)	/* Load & Arm counting registers */
#define CMD_DISARM_SAVE	(0x4 << 5)	/* Disarm & Save counting registers */
#define CMD_SAVE	(0x5 << 5)	/* Save counting registers */
#define CMD_DISARM	(0x6 << 5)	/* Disarm counting registers */


#define CMD_COUNTER_MASK	0x1f	/* Counter mask for above commands */
#define COUNTER64_MASK		0x0f	/* For our 64 bit counter, use low 4 counters */

/*
 * Low order 3 bits select a particular counter (valid counters = 0x0 to 0x5)
 */
#define CMD_CLR_TOG_OUT	(0x1c << 3)
#define CMD_SET_TOG_OUT	(0x1d << 3)
#define CMD_STEP	(0x1e << 3)

/*
 * Master chip reset
 */
#define CMD_DIS_DP_SEQ	0xe8		/* Enable data pointer sequencing */
#define CMD_GOFF_FOUT	0xed		/* Gate Off FOUT */
#define CMD_16BIT_BUS	0xef		/* 16 bit bus */
#define CMD_ENA_DP_SEQ	0xe0		/* Enable data pointer sequencing */
#define CMD_GON_FOUT	0xe6		/* Gate On FOUT */
#define CMD_8BIT_BUS	0xe7		/* 8 bit bus */
#define CMD_ENA_PREFCH	0xe0		/* Enable Prefetch on Write */
#define CMD_DIS_PREFCH	0xf0		/* Disable Prefetch on Write */
#define CMD_RESET	0xff		/* Master reset */

/*
 * Master Mode definitions
 */

#define MM_TOD_DISABLE		(0x00 << 0)	/* TOD disable */
#define MM_CMP1_ENABLE		(0x01 << 2)	/* Counter 1 comparator enable */
#define MM_CMP2_ENABLE		(0x01 << 3)	/* Counter 2 comparator enable */

/* FOUT source */
#define MM_FOUTSRC_E1		(0x00 << 4)
#define MM_FOUTSRC_SRC1		(0x01 << 4)
#define MM_FOUTSRC_SRC2		(0x02 << 4)
#define MM_FOUTSRC_SRC3		(0x03 << 4)
#define MM_FOUTSRC_SRC4		(0x04 << 4)
#define MM_FOUTSRC_SRC5		(0x05 << 4)
#define MM_FOUTSRC_GATE1	(0x06 << 4)
#define MM_FOUTSRC_GATE2	(0x07 << 4)
#define MM_FOUTSRC_GATE3	(0x08 << 4)
#define MM_FOUTSRC_GATE4	(0x09 << 4)
#define MM_FOUTSRC_GATE5	(0x0a << 4)
#define MM_FOUTSRC_F1		(0x0b << 4)
#define MM_FOUTSRC_F2		(0x0c << 4)
#define MM_FOUTSRC_F3		(0x0d << 4)
#define MM_FOUTSRC_F4		(0x0e << 4)
#define MM_FOUTSRC_F5		(0x0f << 4)

/* FOUT divider */
#define MM_FOUTDIV_BY16		(0x00 << 8)
#define MM_FOUTDIV_BY1		(0x01 << 8)
#define MM_FOUTDIV_BY2		(0x02 << 8)
#define MM_FOUTDIV_BY3		(0x03 << 8)
#define MM_FOUTDIV_BY4		(0x04 << 8)
#define MM_FOUTDIV_BY5		(0x05 << 8)
#define MM_FOUTDIV_BY6		(0x06 << 8)
#define MM_FOUTDIV_BY7		(0x07 << 8)
#define MM_FOUTDIV_BY8		(0x08 << 8)
#define MM_FOUTDIV_BY9		(0x09 << 8)
#define MM_FOUTDIV_BY10		(0x0a << 8)
#define MM_FOUTDIV_BY11		(0x0b << 8)
#define MM_FOUTDIV_BY12		(0x0c << 8)
#define MM_FOUTDIV_BY13		(0x0d << 8)
#define MM_FOUTDIV_BY14		(0x0e << 8)
#define MM_FOUTDIV_BY15		(0x0f << 8)

#define MM_FOUTGATE_OFF		(0x0 << 12)	/* FOUT Gate off */
#define MM_FOUTGATE_ON		(0x1 << 12)	/* FOUT Gate on */

#define MM_DATABUS_SIZE8	(0x0 << 13)	/* Data bus width 8 bits */
#define MM_DATABUS_SIZE16	(0x1 << 13)	/* Data bus width 16 bits */

#define MM_DATAPTR_INCR		(0x0 << 14)	/* Disable data pointer auto incrementing */
#define MM_DATAPTR_NOINCR	(0x1 << 14)	/* Enable data pointer auto incrementing */

#define MM_SCALER_BIN		(0x0 << 15)	/* Binary scaler div (power of 2) */
#define MM_SCALER_BCD		(0x1 << 15)	/* BCD scaler div (power of 10) */	

/* Counter Mode */

/* Output Control */
#define CM_OUT_CNTL_INACTLOW	0x0
#define CM_OUT_CNTL_ACTHI_TC	0x1
#define CM_OUT_CNTL_TCTOGGLE	0x2
#define CM_OUT_CNTL_ILL3	0x3
#define CM_OUT_CNTL_INACTHI	0x4
#define CM_OUT_CNTL_ACTLOW_TC	0x5
#define CM_OUT_CNTL_ILL6	0x6
#define CM_OUT_CNTL_ILL7	0x7

#define CM_COUNT_DOWN		(0x0 << 3)
#define CM_COUNT_UP		(0x1 << 3)

#define CM_COUNT_BIN		(0x0 << 4)
#define CM_COUNT_BCD		(0x1 << 4)

#define CM_COUNT_ONCE		(0x0 << 5)
#define CM_COUNT_REP		(0x1 << 5)

#define CM_RELOAD_FROM_LOAD	(0x0 << 6)
#define CM_RELOAD_FROM_L_H	(0x1 << 6)

#define CM_DISABLE_SPEC_GATE	(0x0 << 7)
#define CM_ENABLE_SPEC_GATE	(0x1 << 7)

/* Count source */
#define CM_TCN_M1		(0x0 << 8)	/* Use TC of counter-1 */
#define CM_SRC1			(0x1 << 8)
#define CM_SRC2			(0x2 << 8)
#define CM_SRC3			(0x3 << 8)
#define CM_SRC4			(0x4 << 8)
#define CM_SRC5			(0x5 << 8)
#define CM_GATE1		(0x6 << 8)
#define CM_GATE2		(0x7 << 8)
#define CM_GATE3		(0x8 << 8)
#define CM_GATE4		(0x9 << 8)
#define CM_GATE5		(0xa << 8)
#define CM_F1			(0xb << 8)
#define CM_F2			(0xc << 8)
#define CM_F3			(0xd << 8)
#define CM_F4			(0xe << 8)
#define CM_F5			(0xf << 8)

/* Source edge */
#define CM_RISING_EDGE		(0x0 << 12)
#define CM_FALLING_EDGE		(0x1 << 12)

/* Gating control */
#define CM_NO_GATE		(0x0 << 13)
#define CM_ACTHI_TCN_M1		(0x1 << 13)
#define CM_ACTHI_LEVEL_N_P1	(0x2 << 13)
#define CM_ACTHI_LEVEL_N_M1	(0x3 << 13)
#define CM_ACTHI_LEVEL_N	(0x4 << 13)
#define CM_ACTLOW_LEVEL_N	(0x5 << 13)
#define CM_ACTHI_EDGE_N		(0x6 << 13)
#define CM_ACTLOW_EDGE_N	(0x7 << 13)

/*
 * Useful values for timespec_t conversion
 */
#define USEC_PER_WORD		4295		/* microseconds per 32 bit unsigned word */

/*
 * Because we can not do floating point in the kernel, we use
 * scaler arithmetic, but move the decimal point out by 1000 to
 * gain accuracy and then divide it out afterwards.
 */
#define USEC_PER_SEC_X1000	1000000000	/* microseconds per second X 1000 */
#define USEC_PER_WORD_X1000	4295967		/* microseconds per 32 bit unsigned word X 1000 */

#ifdef	DEBUG

/*
 * Last read time
 */
struct t64 cyctm05_lastget;

/*
 * Step counter by requested amount.  Used for debugging.
 */
static int
cyctm05_step(
	int	counter,
	int	step_count)
{
	register int ii;

	/* advance the low order counter */
	for (ii = step_count; ii > 0; ii--)
		outb(CMD_PORT, CMD_STEP|counter);
	return(0);
}
	
#endif	/* DEBUG */

/*
 * Get clock value as 64-bit structure.
 *
 * NOTE: Must be called with clock locked.
 */
static kern_return_t
get_clock(
	struct t64	*t64p)
{
	register unsigned long	low, high;

	/*
	 * Save off current clock values into host register
	 * (counter will keep ticking).
	 */

	outb(CMD_PORT, CMD_SAVE|COUNTER64_MASK);

	/* set up the data pointer to start at the first counter */

	outb(CMD_PORT, CMD_LDP|CMD_EP_HOLDCY|CMD_GP_CNT1);

	low  = (inb(DATA_PORT) & 0xff);
	low += (inb(DATA_PORT) & 0xff) << 8;
	low += (inb(DATA_PORT) & 0xff) << 16;
	low += (inb(DATA_PORT) & 0xff) << 24;

	high  = (inb(DATA_PORT) & 0xff);
	high += (inb(DATA_PORT) & 0xff) << 8;
	high += (inb(DATA_PORT) & 0xff) << 16;
	high += (inb(DATA_PORT) & 0xff) << 24;

	t64p->l = low;
	t64p->h = high;

#ifdef	DEBUG
	cyctm05_lastget = *t64p;
#endif	/* DEBUG */

	return(KERN_SUCCESS);
}

/*
 * Set clock value.
 *
 * NOTE: Must be called with clock locked.
 */
static kern_return_t
set_clock(
	struct t64	*t64p)
{
	return(KERN_FAILURE);
}

/*
 * Program the counter mode (initializing hod and load registers to 0)
 */
static void
program_counter(
	int		counter,
	unsigned int	cmd)
{
	assert(counter >= 1 && counter <= NCOUNTERS);

	/* Setup to program counter */
	outb(CMD_PORT, CMD_LDP|CMD_EP_MODE|counter);

	/* Mode */
	outb(DATA_PORT, LSB(cmd));
	outb(DATA_PORT, MSB(cmd));

	/* Load */
	outb(DATA_PORT, 0);
	outb(DATA_PORT, 0);

	/* Hold */
	outb(DATA_PORT, 0);
	outb(DATA_PORT, 0);
}

/*
 * Configure clock device.
 */
int
cyctm05_config(void)
{
	register int ii;
	register int  mm, cmd;

	/*
	 * Reset the device,
	 * Load all the counters,
	 * Set the data pointer to something valid
	 */
	outb(CMD_PORT, CMD_RESET);
	outb(CMD_PORT, CMD_LOAD|CMD_COUNTER_MASK);
	outb(CMD_PORT, CMD_LDP|CMD_EP_MM|CMD_GP_CNTL);

	mm =	MM_TOD_DISABLE |
		MM_FOUTSRC_F1 |
		MM_FOUTDIV_BY1 |
		MM_FOUTGATE_ON |
		MM_DATABUS_SIZE8 |
		MM_DATAPTR_INCR |
		MM_SCALER_BCD;

	/* Initialize the Master Mode register */
	outb(DATA_PORT, LSB(mm));
	outb(DATA_PORT, MSB(mm));

	/* Common counter mode */
	cmd =	CM_COUNT_UP |
		CM_COUNT_BIN |
		CM_COUNT_REP |
		CM_RELOAD_FROM_LOAD | 
		CM_DISABLE_SPEC_GATE |
		CM_RISING_EDGE;

	program_counter(1, cmd|CM_F1);		/* source 1mhz crystal */
	program_counter(2, cmd|CM_TCN_M1);	/* source TC from next lower counter */
	program_counter(3, cmd|CM_TCN_M1);
	program_counter(4, cmd|CM_TCN_M1);
	program_counter(5, cmd|CM_TCN_M1);

	/* Start counters going */
	outb(CMD_PORT, CMD_LOAD_ARM|COUNTER64_MASK);

	printf("cyctm05 clock configured\n");

	cyctm05_initialized = TRUE;

	return(1);
}

/*
 * Initialize clock device.
 */
int
cyctm05_init(void)
{
	return(1);
}

/*
 * Get the current clock time.
 */
kern_return_t
cyctm05_gettime(
	tvalspec_t	*cur_time)	/* OUT */
{
	kern_return_t result;
	struct t64 t64;
	int s;

	t64.l = t64.h = 0;

	/*
	 * Read the 64 bit time value
	 */
	s = LOCK_CYCTM05();
	result = get_clock(&t64);
	UNLOCK_CYCTM05(s);

	if (cyctm05_raw) {
		/*
		 * For debugging, deliver raw 64 bit counter value
		 */
		cur_time->tv_nsec = t64.l;
		cur_time->tv_sec  = t64.h;
	}
	else {
		/*
		 * Convert it to tvalspec format.
		 *
		 * NOTE: we will lose a bit here at some point since we
		 * have more than 32-bits of second resolution...
		 */
		
		cur_time->tv_nsec = (t64.l % USEC_PER_SEC) * NSEC_PER_USEC;
		cur_time->tv_sec  = ((unsigned)(t64.l / USEC_PER_SEC) +
				     ((t64.h * USEC_PER_WORD_X1000) / 1000));
	}

	return(result);
}

/*
 * Set the current clock time.
 */
kern_return_t
cyctm05_settime(
	tvalspec_t	*new_time)
{
	kern_return_t result;
	struct t64 t64;
	int s;

	/*
	 * Convert tvalspec format to 64-bit absolute value
	 */
	t64.l = (new_time->tv_nsec / NSEC_PER_USEC) +
		(new_time->tv_sec * USEC_PER_SEC);
	t64.h = (new_time->tv_sec / USEC_PER_WORD) * USEC_PER_SEC;

	/*
	 * Set chip time value
	 */
	s = LOCK_CYCTM05();
	result = set_clock(&t64);
	UNLOCK_CYCTM05(s);

	return(result);
}

/*
 * Get current time.
 */
void
cyctm05_stamp(
	struct t64	*t64p)
{
	int s;

	s = LOCK_CYCTM05();
	(void) get_clock(t64p);
	UNLOCK_CYCTM05(s);

	return;
}

void
cyctm05_stamp_masked(
	struct t64	*t64p)
{
	(void) get_clock(t64p);
}

/*
 * Calculate difference between two clock values.
 */
static kern_return_t
diff_clock(
	struct t64	*start,
	struct t64	*end,
	struct t64	*diff)
{
	/* perform diff = end - start */
	diff->h = end->h - start->h;
	if (end->l < start->l) {
		diff->l = -(start->l - end->l);
		diff->h--;
	}
	else {
		diff->l = end->l - start->l;
	}

	return(KERN_SUCCESS);
}

/*
 * Compute difference between two time values.
 */
void
cyctm05_diff(
	struct t64	*start,
	struct t64	*end,
	struct t64	*diff)
{
	if (diff_clock(start, end, diff) != KERN_SUCCESS)
		panic("cyctm05_diff: diff_clock failed");
}

#endif	/* if NCYCTM05 > 0 */
