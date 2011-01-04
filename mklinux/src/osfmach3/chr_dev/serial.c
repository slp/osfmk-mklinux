/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 * linux/drivers/char/serial_osfmach3_ppc.c
 *
 * This is the OSFMACH3 Driver for MkLinux/PPC
 */

/*
 *  linux/drivers/char/serial.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Extensively rewritten by Theodore Ts'o, 8/16/92 -- 9/14/92.  Now
 *  much more extensible to support other serial cards based on the
 *  16450/16550A UART's.  Added support for the AST FourPort and the
 *  Accent Async board.  
 *
 *  set_serial_info fixed to set the flags, custom divisor, and uart
 * 	type fields.  Fix suggested by Michael K. Johnson 12/12/92.
 *
 * This module exports the following rs232 io functions:
 *
 *	long rs_init(long);
 * 	int  rs_open(struct tty_struct * tty, struct file * filp)
 */

#include <device/tty_status.h>
#include <osfmach3/device_utils.h>
#include <osfmach3/device_reply_hdlr.h>
#include <osfmach3/assert.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/server_thread.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/mach_init.h>

#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_reg.h>
#include <linux/config.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>
#include <linux/mm.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>
#include <asm/bitops.h>

static char *serial_version = "1.01";

DECLARE_TASK_QUEUE(tq_serial);

struct tty_driver serial_driver, callout_driver;
static int serial_refcount;

/* serial subtype definitions */
#define SERIAL_TYPE_NORMAL	1
#define SERIAL_TYPE_CALLOUT	2

/* number of characters left in xmit buffer before we ask for more */
#define WAKEUP_CHARS 256

/*
 * Serial driver configuration section.  Here are the various options:
 *
 * CONFIG_HUB6
 *		Enables support for the venerable Bell Technologies
 *		HUB6 card.
 *
 * SERIAL_PARANOIA_CHECK
 * 		Check the magic number for the async_structure where
 * 		ever possible.
 */

#define SERIAL_PARANOIA_CHECK
#define	CONFIG_SERIAL_NOPAUSE_IO
#define SERIAL_DO_RESTART

#undef	SERIAL_DEBUG_INTR
#undef	SERIAL_DEBUG_OPEN

#define _INLINE_

/*
 * IRQ_timeout		- How long the timeout should be for each IRQ
 * 				should be after the IRQ has been active.
 */

extern void *serial_read_thread(void *arg);
static void change_speed(struct async_struct *info);
static void serial_close_device(struct async_struct *info);
kern_return_t serial_write_reply(char *, kern_return_t, char *, 
					unsigned int data_count);
	
kern_return_t seriaL_read_reply(char *, kern_return_t, char *,
				unsigned int data_count);
static void serial_send_next_write(struct async_struct *info);

#define NR_PORTS	2

static struct tty_struct *serial_table[NR_PORTS];
static struct termios *serial_termios[NR_PORTS];
static struct termios *serial_termios_locked[NR_PORTS];

struct async_struct rs_table[NR_PORTS] = { 
	{ 0 }
};

#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

static inline int serial_paranoia_check(struct async_struct *info,
					kdev_t device, const char *routine)
{
#ifdef SERIAL_PARANOIA_CHECK
	static const char *badmagic =
		"Warning: bad magic number for serial struct (%s) in %s\n";
	static const char *badinfo =
		"Warning: null async_struct for (%s) in %s\n";

	if (!info) {
		printk(badinfo, kdevname(device), routine);
		return 1;
	}
	if (info->magic != SERIAL_MAGIC) {
		printk(badmagic, kdevname(device), routine);
		return 1;
	}
#endif
	return 0;
}

/*
 * This is used to figure out the divisor speeds and the timeouts
 */
static int baud_table[] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
	9600, 19200, 38400, 57600, 115200, 230400 };

/* Dummy routines */

int register_serial(struct serial_struct *req)
{
	return -1;
}

void unregister_serial(int line)
{
	return;
}

/*
 * ------------------------------------------------------------
 * rs_stop() and rs_start()
 *
 * This routines are called before setting or resetting tty->stopped.
 * They enable or disable transmitter interrupts, as necessary.
 * ------------------------------------------------------------
 */
static void rs_stop(struct tty_struct *tty)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;

	serial_paranoia_check(info, tty->device, "rs_stop");

	/* NOP for OSFMACH3 */
}

static void rs_start(struct tty_struct *tty)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;
	
	if (serial_paranoia_check(info, tty->device, "rs_start"))
		return;
}

/*
 * This routine is used by the interrupt handler to schedule
 * processing in the software interrupt portion of the driver.
 */
static _INLINE_ void rs_sched_event(struct async_struct *info,
				  int event)
{
	info->event |= 1 << event;
	queue_task_irq_off(&info->tqueue, &tq_serial);
	mark_bh(SERIAL_BH);
}

static _INLINE_ void transmit_chars(struct async_struct *info)
{
	
	if ((info->xmit_cnt <= 0) || info->tty->stopped ||
	    info->tty->hw_stopped || info->write_busy) {
		return;
	}
	
	serial_send_next_write(info);
}

static void serial_send_next_write(struct async_struct *info)
{
	int			line;
	mach_msg_type_number_t	data_count;
	kern_return_t		kr;

	line = MINOR(info->tty->device) - info->tty->driver.minor_start;

	data_count = SERIAL_XMIT_SIZE - info->xmit_tail;
	if (data_count > info->xmit_cnt)
		data_count = info->xmit_cnt;
	if (data_count > IO_INBAND_MAX)
		data_count = IO_INBAND_MAX;

	info->write_busy = TRUE;

	kr = serv_device_write_async(info->device_port,
			info->reply_port, 0,
			0, &info->xmit_buf[info->xmit_tail],
			data_count, TRUE);

	if (kr != D_SUCCESS) {
		if (info->tty != NULL) {
			MACH3_DEBUG(1, kr,
				    ("transmit_chars(dev 0x%x): "
				     "serv_device_write_async",
				     info->tty->device));
		}
		info->xmit_cnt = 0;
		info->write_busy = FALSE;
	}
	
	if (info->xmit_cnt < WAKEUP_CHARS)
		rs_sched_event(info, RS_EVENT_WRITE_WAKEUP);

#ifdef SERIAL_DEBUG_INTR
	printk("THRE...");
#endif
}

kern_return_t
serial_write_reply(char *handle, kern_return_t return_code,
        char *data,  unsigned int data_count)
{
	struct async_struct *info = (struct async_struct *) handle;

	info->write_busy = FALSE;

	if (info->tty == NULL) 	 /* Lost tty.. */
		return KERN_SUCCESS;

	/* TODO -- maybe handle errors some how? */
	if (return_code != KERN_SUCCESS) {
		MACH3_DEBUG(1, return_code,
			("serial_write_reply failed: %d", return_code));
	} else {
		info->xmit_tail += data_count;
		info->xmit_tail &= SERIAL_XMIT_SIZE-1;
		info->xmit_cnt -= data_count;
	}

	transmit_chars(info);	/* Transmit next block .. */

	return KERN_SUCCESS;
}

kern_return_t
serial_read_reply(char *handle, kern_return_t return_code,
        char *data,  unsigned int data_count)
{
	panic("serial_read_reply called!");
}

/*
 * This routine is used to handle the "bottom half" processing for the
 * serial driver, known also the "software interrupt" processing.
 * This processing is done at the kernel interrupt level, after the
 * rs_interrupt() has returned, BUT WITH INTERRUPTS TURNED ON.  This
 * is where time-consuming activities which can not be done in the
 * interrupt driver proper are done; the interrupt driver schedules
 * them using rs_sched_event(), and they get done here.
 */
static void do_serial_bh(void)
{
	run_task_queue(&tq_serial);
}

static void do_softint(void *private_)
{
	struct async_struct	*info = (struct async_struct *) private_;
	struct tty_struct	*tty;
	
	tty = info->tty;
	if (!tty)
		return;

	if (clear_bit(RS_EVENT_WRITE_WAKEUP, &info->event)) {
		if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
		    tty->ldisc.write_wakeup)
			(tty->ldisc.write_wakeup)(tty);
		wake_up_interruptible(&tty->write_wait);
	}
}

/*
 * This routine is called from the scheduler tqueue when the interrupt
 * routine has signalled that a hangup has occurred.  The path of
 * hangup processing is:
 *
 * 	serial interrupt routine -> (scheduler tqueue) ->
 * 	do_serial_hangup() -> tty->hangup() -> rs_hangup()
 * 
 */
static void do_serial_hangup(void *private_)
{
	struct async_struct	*info = (struct async_struct *) private_;
	struct tty_struct	*tty;
	
	tty = info->tty;
	if (!tty)
		return;

	tty_hangup(tty);
}

/*
 * ---------------------------------------------------------------
 * Low level utility subroutines for the serial driver:  routines to
 * figure out the appropriate timeout for an interrupt chain, routines
 * to initialize and startup a serial port, and routines to shutdown a
 * serial port.  Useful stuff like that.
 * ---------------------------------------------------------------
 */

static int startup(struct async_struct * info)
{
	unsigned long flags;
	unsigned long page;

	page = get_free_page(GFP_KERNEL);
	if (!page)
		return -ENOMEM;


	save_flags(flags); cli();

	if (info->flags & ASYNC_INITIALIZED) {
		free_page(page);
		restore_flags(flags);
		return 0;
	}

	if (info->xmit_buf)
		free_page(page);
	else
		info->xmit_buf = (unsigned char *) page;

#ifdef SERIAL_DEBUG_OPEN
	printk("starting up ttys%d (irq %d)...", info->line, info->irq);
#endif

	if (info->tty)
		clear_bit(TTY_IO_ERROR, &info->tty->flags);
	info->xmit_cnt = info->xmit_head = info->xmit_tail = 0;

	/*
	 * and set the speed of the serial port
	 */
	change_speed(info);

	info->flags |= ASYNC_INITIALIZED;
	restore_flags(flags);
	return 0;
}

/*
 * This routine will shutdown a serial port; interrupts are disabled, and
 * DTR is dropped if the hangup on close termio flag is on.
 */
static void shutdown(struct async_struct * info)
{
	unsigned long	flags;

	if (!(info->flags & ASYNC_INITIALIZED))
		return;

#ifdef SERIAL_DEBUG_OPEN
	printk("Shutting down serial port %d (irq %d)....", info->line,
	       info->irq);
#endif
	
	save_flags(flags); cli(); /* Disable interrupts */
	

	if (info->xmit_buf) {
		free_page((unsigned long) info->xmit_buf);
		info->xmit_buf = 0;
	}

	if (info->tty)
		set_bit(TTY_IO_ERROR, &info->tty->flags);
	
	info->flags &= ~ASYNC_INITIALIZED;
	restore_flags(flags);
}

/*
 * This routine is called to set the UART divisor registers to match
 * the specified baud rate for a serial port.
 */
static void change_speed(struct async_struct *info)
{
	unsigned cflag;
	int			line;
	mach_port_t		device_port;
	kern_return_t		kr;
	struct tty_status	ttstat;
	mach_msg_type_number_t	ttstat_count;

	if (!info->tty || !info->tty->termios)
		return;

	cflag = info->tty->termios->c_cflag;

	line = MINOR(info->tty->device) - info->tty->driver.minor_start;
	device_port = info->device_port;
	ttstat_count = TTY_STATUS_COUNT;
	if (device_port != MACH_PORT_NULL) {
		kr = device_get_status(device_port,
				       TTY_STATUS_NEW,
				       (dev_status_t) &ttstat,
				       &ttstat_count);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("change_speed(dev 0x%x): "
				     "device_get_status(TTY_STATUS_NEW)",
				     info->tty->device));
			return;
		}
	}
	/* Mach break handling is obsolete with "new" out-of-band. */
	ttstat.tt_breakc = 0;
	ttstat.tt_flags &= ~(TF_ODDP | TF_EVENP | TF_LITOUT | TF_NOHANG |
			     TF_8BIT | TF_READ | TF_HUPCLS | TF_OUT_OF_BAND | 
			     TF_INPCK | TF_CRTSCTS);
	ttstat.tt_flags |= TF_OUT_OF_BAND;	/* we always want OUT_OF_BAND */

	/* Convert from POSIX to MACH speed */

	if ((cflag & CBAUD) < (sizeof(baud_table)/sizeof(baud_table[0])))
		ttstat.tt_ispeed = ttstat.tt_ospeed = 
			baud_table[cflag & CBAUD];
	else	/* Largest possible baud rate (for the moment) */
		ttstat.tt_ispeed = ttstat.tt_ospeed = 230400;

	if (cflag & CRTSCTS) {
		info->flags |= ASYNC_CTS_FLOW;
	} else
		info->flags &= ~ASYNC_CTS_FLOW;

	if (cflag & CLOCAL)
		info->flags &= ~ASYNC_CHECK_CD;
	else {
		info->flags |= ASYNC_CHECK_CD;
	}

#define RELEVANT_IFLAG(iflag) (iflag & (IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK))

	if (I_INPCK(info->tty))
		info->read_status_mask |= UART_LSR_FE | UART_LSR_PE;

	if (I_BRKINT(info->tty) || I_PARMRK(info->tty))
		info->read_status_mask |= UART_LSR_BI;
	
	info->ignore_status_mask = 0;
	if (I_IGNPAR(info->tty)) {
		info->ignore_status_mask |= UART_LSR_PE | UART_LSR_FE;
		info->read_status_mask |= UART_LSR_PE | UART_LSR_FE;
	}
	if (I_IGNBRK(info->tty)) {
		info->ignore_status_mask |= UART_LSR_BI;
		info->read_status_mask |= UART_LSR_BI;
		/*
		 * If we're ignore parity and break indicators, ignore 
		 * overruns too.  (For real raw support).
		 */
		if (I_IGNPAR(info->tty)) {
			info->ignore_status_mask |= UART_LSR_OE;
			info->read_status_mask |= UART_LSR_OE;
		}
	}
	
	if (cflag & PARENB) {
		ttstat.tt_flags |= TF_INPCK;
		ttstat.tt_flags |= (cflag & PARODD) ? TF_ODDP : TF_EVENP;
	}

	if ((cflag & CSIZE) != CS7) { 	/* assume CS8 */
		ttstat.tt_flags |= TF_LITOUT | TF_8BIT;
	}

	if (cflag & CLOCAL)
		ttstat.tt_flags |= TF_NOHANG;

	if (cflag & CREAD)
		ttstat.tt_flags |= TF_READ;

	if (cflag & HUPCL)
		ttstat.tt_flags |= TF_HUPCLS;

	if (cflag & CRTSCTS)
		ttstat.tt_flags |= TF_CRTSCTS;

	if (device_port != MACH_PORT_NULL) {
		kr = device_set_status(device_port,
				       TTY_STATUS_NEW,
				       (dev_status_t) &ttstat,
				       ttstat_count);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("change_speed(dev 0x%x): "
				     "device_set_status(TTY_STATUS_NEW)",
				     info->tty->device));
		}
	}
}

static void rs_put_char(struct tty_struct *tty, unsigned char ch)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;
	unsigned long flags;

	if (serial_paranoia_check(info, tty->device, "rs_put_char"))
		return;

	if (!tty || !info->xmit_buf)
		return;

	save_flags(flags); cli();
	if (info->xmit_cnt >= SERIAL_XMIT_SIZE - 1) {
		restore_flags(flags);
		return;
	}

	info->xmit_buf[info->xmit_head++] = ch;
	info->xmit_head &= SERIAL_XMIT_SIZE-1;
	info->xmit_cnt++;
	restore_flags(flags);
}

static void rs_flush_chars(struct tty_struct *tty)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;
	unsigned long flags;
				
	if (serial_paranoia_check(info, tty->device, "rs_flush_chars"))
		return;

	if (info->xmit_cnt <= 0 || tty->stopped || tty->hw_stopped ||
	    !info->xmit_buf)
		return;

	save_flags(flags); cli();
	transmit_chars(info);
	restore_flags(flags);
}

static int rs_write(struct tty_struct * tty, int from_user,
		    const unsigned char *buf, int count)
{
	int	c, total = 0;
	struct async_struct *info = (struct async_struct *)tty->driver_data;
	unsigned long flags;
				
	if (serial_paranoia_check(info, tty->device, "rs_write"))
		return 0;

	if (!tty || !info->xmit_buf)
		return 0;
	    
	save_flags(flags);
	while (1) {
		cli();		
		c = MIN(count, MIN(SERIAL_XMIT_SIZE - info->xmit_cnt - 1,
				   SERIAL_XMIT_SIZE - info->xmit_head));
		if (c <= 0)
			break;

		if (from_user) 
                        memcpy_fromfs(info->xmit_buf + info->xmit_head, buf, c);
                else
			memcpy(info->xmit_buf + info->xmit_head, buf, c);

		info->xmit_head = (info->xmit_head + c) & (SERIAL_XMIT_SIZE-1);
		info->xmit_cnt += c;
		restore_flags(flags);
		buf += c;
		count -= c;
		total += c;
	}

	if (info->xmit_cnt && !tty->stopped && !tty->hw_stopped) {
		transmit_chars(info);
	}

	restore_flags(flags);
	return total;
}

static int rs_write_room(struct tty_struct *tty)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;
	int	ret;
				
	if (serial_paranoia_check(info, tty->device, "rs_write_room"))
		return 0;
	ret = SERIAL_XMIT_SIZE - info->xmit_cnt - 1;
	if (ret < 0)
		ret = 0;
	return ret;
}

static int rs_chars_in_buffer(struct tty_struct *tty)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;
				
	if (serial_paranoia_check(info, tty->device, "rs_chars_in_buffer"))
		return 0;
	return info->xmit_cnt;
}

static void rs_flush_buffer(struct tty_struct *tty)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;
				
	if (serial_paranoia_check(info, tty->device, "rs_flush_buffer"))
		return;
	cli();
	info->xmit_cnt = info->xmit_head = info->xmit_tail = 0;
	sti();
	wake_up_interruptible(&tty->write_wait);
	if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
	    tty->ldisc.write_wakeup)
		(tty->ldisc.write_wakeup)(tty);
}

/*
 * ------------------------------------------------------------
 * rs_throttle()
 * 
 * This routine is called by the upper-layer tty layer to signal that
 * incoming characters should be throttled.
 * ------------------------------------------------------------
 */
static void rs_throttle(struct tty_struct * tty)
{
	/*struct async_struct *info = (struct async_struct *)tty->driver_data;*/

	return;
}

static void rs_unthrottle(struct tty_struct * tty)
{
	/*struct async_struct *info = (struct async_struct *)tty->driver_data;*/

	return;
}

/*
 * ------------------------------------------------------------
 * rs_ioctl() and friends
 * ------------------------------------------------------------
 */

static int
get_modem_info(struct async_struct *info, unsigned int *result)
{
	mach_msg_type_number_t	count;
	kern_return_t		kr;
	unsigned long		value, status = 0;
	
	count = TTY_MODEM_COUNT;
	kr = device_get_status(info->device_port,
			       TTY_MODEM,
			       (dev_status_t)&status,
			       &count);
	if (kr != D_SUCCESS)
		return -EIO;

	value = 0;
	if (status & TM_RTS)
		value |= TIOCM_RTS;

	if (status & TM_DTR)
		value |= TIOCM_DTR;

	if (status & TM_CAR)
		value |= TIOCM_CAR;

	put_user(value, result);
	return 0;
}

static int set_modem_info(struct async_struct * info, unsigned int cmd,
			  unsigned int *value)
{
	int error;
	unsigned int arg;
	long	result = 0, cur;
	mach_msg_type_number_t	count;
	kern_return_t		kr;

	error = verify_area(VERIFY_READ, value, sizeof(int));
	if (error)
		return error;
	arg = get_user(value);

	if (arg & TIOCM_RTS)
		result |= TM_RTS;

	if (arg & TIOCM_DTR)
		result |= TM_DTR;

	count = TTY_MODEM_COUNT;
	kr = device_get_status(info->device_port,
			       TTY_MODEM,
			       (dev_status_t)&cur,
			       &count);
	if (kr != D_SUCCESS)
		return	-EIO;

	switch (cmd) {
	case TIOCMBIS: 
		cur = cur | result;
		break;

	case TIOCMBIC:
		cur = cur & ~result;
		break;

	case TIOCMSET:
		cur = result;
		break;

	default:
		return -EINVAL;
	}

	kr = device_set_status(info->device_port,
			       TTY_MODEM,
			       (dev_status_t)&cur,
			       TTY_MODEM_COUNT);
	if (kr != D_SUCCESS) 
		return	-EIO;

	return 0;
}

/*
 * This routine sends a break character out the serial port.
 */
static void send_break(	struct async_struct * info, int duration)
{
	mach_port_t	device_port = info->device_port;

	current->state = TASK_INTERRUPTIBLE;
	current->timeout = jiffies + duration;
	cli();
	device_set_status(device_port, TTY_SET_BREAK, 0, 0);
	schedule();
	device_set_status(device_port, TTY_CLEAR_BREAK, 0, 0);
	sti();
}

static int rs_ioctl(struct tty_struct *tty, struct file * file,
		    unsigned int cmd, unsigned long arg)
{
	int error;
	struct async_struct * info = (struct async_struct *)tty->driver_data;
	int retval;

	if (serial_paranoia_check(info, tty->device, "rs_ioctl"))
		return -ENODEV;

	if ((cmd != TIOCGSERIAL) && (cmd != TIOCSSERIAL) &&
	    (cmd != TIOCSERCONFIG) && (cmd != TIOCSERGWILD)  &&
	    (cmd != TIOCSERSWILD) && (cmd != TIOCSERGSTRUCT) &&
	    (cmd != TIOCMIWAIT) && (cmd != TIOCGICOUNT)) {
		if (tty->flags & (1 << TTY_IO_ERROR))
		    return -EIO;
	}
	
	switch (cmd) {
		case TCSBRK:	/* SVID version: non-zero arg --> no break */
			retval = tty_check_change(tty);
			if (retval)
				return retval;
			tty_wait_until_sent(tty, 0);
			if (!arg)
				send_break(info, HZ/4);	/* 1/4 second */
			return 0;
		case TCSBRKP:	/* support for POSIX tcsendbreak() */
			retval = tty_check_change(tty);
			if (retval)
				return retval;
			tty_wait_until_sent(tty, 0);
			send_break(info, arg ? arg*(HZ/10) : HZ/4);
			return 0;
		case TIOCGSOFTCAR:
			error = verify_area(VERIFY_WRITE, (void *) arg,sizeof(long));
			if (error)
				return error;
			put_fs_long(C_CLOCAL(tty) ? 1 : 0,
				    (unsigned long *) arg);
			return 0;
		case TIOCSSOFTCAR:
			error = verify_area(VERIFY_READ, (void *) arg, sizeof(long));
			if (error)
				return error;
			arg = get_fs_long((unsigned long *) arg);
			tty->termios->c_cflag =
				((tty->termios->c_cflag & ~CLOCAL) |
				 (arg ? CLOCAL : 0));
			return 0;
		case TIOCMGET:
			error = verify_area(VERIFY_WRITE, (void *) arg,
				sizeof(unsigned int));
			if (error)
				return error;
			return get_modem_info(info, (unsigned int *) arg);
		case TIOCMBIS:
		case TIOCMBIC:
		case TIOCMSET:
			return set_modem_info(info, cmd, (unsigned int *) arg);
		default:
			return -ENOIOCTLCMD;
		}
	return 0;
}

static void rs_set_termios(struct tty_struct *tty, struct termios *old_termios)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;

	if (   (tty->termios->c_cflag == old_termios->c_cflag)
	    && (   RELEVANT_IFLAG(tty->termios->c_iflag) 
		== RELEVANT_IFLAG(old_termios->c_iflag)))
	  return;

	change_speed(info);

	if ((old_termios->c_cflag & CRTSCTS) &&
	    !(tty->termios->c_cflag & CRTSCTS)) {
		tty->hw_stopped = 0;
		rs_start(tty);
	}
}

/*
 * ------------------------------------------------------------
 * rs_close()
 * 
 * This routine is called when the serial port gets closed.  First, we
 * wait for the last remaining data to be sent.  Then, we unlink its
 * async structure from the interrupt chain if necessary, and we free
 * that IRQ if nothing is left in the chain.
 * ------------------------------------------------------------
 */
static void rs_close(struct tty_struct *tty, struct file * filp)
{
	struct async_struct * info = (struct async_struct *)tty->driver_data;
	unsigned long flags;
	int		line;

	if (!info || serial_paranoia_check(info, tty->device, "rs_close")) {
		return;
	}
	
	save_flags(flags); cli();
	
	if (tty_hung_up_p(filp)) {
		restore_flags(flags);
		return;
	}
	
#ifdef SERIAL_DEBUG_OPEN
	printk("rs_close ttys%d, count = %d\n", info->line, info->count);
#endif
	if ((tty->count == 1) && (info->count != 1)) {
		/*
		 * Uh, oh.  tty->count is 1, which means that the tty
		 * structure will be freed.  Info->count should always
		 * be one in these conditions.  If it's greater than
		 * one, we've got real problems, since it means the
		 * serial port won't be shutdown.
		 */
		printk("rs_close: bad serial port count; tty->count is 1, "
		       "info->count is %d\n", info->count);
		info->count = 1;
	}
	if (--info->count < 0) {
		printk("rs_close: bad serial port count for ttys%d: %d\n",
		       info->line, info->count);
		info->count = 0;
	}
	if (info->count) {
		restore_flags(flags);
		return;
	}
	info->flags |= ASYNC_CLOSING;
	/*
	 * Save the termios structure, since this port may have
	 * separate termios for callout and dialin.
	 */
	if (info->flags & ASYNC_NORMAL_ACTIVE)
		info->normal_termios = *tty->termios;
	if (info->flags & ASYNC_CALLOUT_ACTIVE)
		info->callout_termios = *tty->termios;
	/*
	 * Now we wait for the transmit buffer to clear; and we notify 
	 * the line discipline to only process XON/XOFF characters.
	 */
	tty->closing = 1;
	if (info->closing_wait != ASYNC_CLOSING_WAIT_NONE)
		tty_wait_until_sent(tty, info->closing_wait);

	shutdown(info);
	if (tty->driver.flush_buffer)
		tty->driver.flush_buffer(tty);
	if (tty->ldisc.flush_buffer)
		tty->ldisc.flush_buffer(tty);
	tty->closing = 0;
	info->event = 0;
	info->tty = 0;

	line = MINOR(tty->device) - tty->driver.minor_start;

	serial_close_device(info);

	if (info->blocked_open) {
		if (info->close_delay) {
			current->state = TASK_INTERRUPTIBLE;
			current->timeout = jiffies + info->close_delay;
			schedule();
		}
		wake_up_interruptible(&info->open_wait);
	}
	info->flags &= ~(ASYNC_NORMAL_ACTIVE|ASYNC_CALLOUT_ACTIVE|
			 ASYNC_CLOSING);
	wake_up_interruptible(&info->close_wait);
	restore_flags(flags);
}

/*
 * rs_hangup() --- called by tty_hangup() when a hangup is signaled.
 */
void rs_hangup(struct tty_struct *tty)
{
	struct async_struct * info = (struct async_struct *)tty->driver_data;
	
	if (serial_paranoia_check(info, tty->device, "rs_hangup"))
		return;
	
	rs_flush_buffer(tty);
	shutdown(info);
	info->event = 0;
	info->count = 0;
	info->flags &= ~(ASYNC_NORMAL_ACTIVE|ASYNC_CALLOUT_ACTIVE);
	info->tty = 0;

	serial_close_device(info);

	wake_up_interruptible(&info->open_wait);
}

static void
serial_close_device(struct async_struct *info)
{
	if (info->device_port != MACH_PORT_NULL) {
		device_close(info->device_port);
		mach_port_destroy(mach_task_self(), info->device_port);

		device_reply_deregister(info->reply_port);

		info->reply_port = MACH_PORT_NULL;
		info->device_port = MACH_PORT_NULL;
	}
}
		
/*
 * ------------------------------------------------------------
 * rs_open() and friends
 * ------------------------------------------------------------
 */
static int block_til_ready(struct tty_struct *tty, struct file * filp,
			   struct async_struct *info)
{
	struct wait_queue wait = { current, NULL };
	int		retval;
	int		do_clocal = 0;

	/*
	 * If the device is in the middle of being closed, then block
	 * until it's done, and then try again.
	 */
	if (tty_hung_up_p(filp) ||
	    (info->flags & ASYNC_CLOSING)) {
		if (info->flags & ASYNC_CLOSING)
			interruptible_sleep_on(&info->close_wait);
#ifdef SERIAL_DO_RESTART
		if (info->flags & ASYNC_HUP_NOTIFY)
			return -EAGAIN;
		else
			return -ERESTARTSYS;
#else
		return -EAGAIN;
#endif
	}

	/*
	 * If this is a callout device, then just make sure the normal
	 * device isn't being used.
	 */
	if (tty->driver.subtype == SERIAL_TYPE_CALLOUT) {
		if (info->flags & ASYNC_NORMAL_ACTIVE)
			return -EBUSY;
		if ((info->flags & ASYNC_CALLOUT_ACTIVE) &&
		    (info->flags & ASYNC_SESSION_LOCKOUT) &&
		    (info->session != current->session))
		    return -EBUSY;
		if ((info->flags & ASYNC_CALLOUT_ACTIVE) &&
		    (info->flags & ASYNC_PGRP_LOCKOUT) &&
		    (info->pgrp != current->pgrp))
		    return -EBUSY;
		info->flags |= ASYNC_CALLOUT_ACTIVE;
		return 0;
	}
	
	/*
	 * If non-blocking mode is set, or the port is not enabled,
	 * then make the check up front and then exit.
	 */
	if ((filp->f_flags & O_NONBLOCK) ||
	    (tty->flags & (1 << TTY_IO_ERROR))) {
		if (info->flags & ASYNC_CALLOUT_ACTIVE)
			return -EBUSY;
		info->flags |= ASYNC_NORMAL_ACTIVE;
		return 0;
	}

	if (info->flags & ASYNC_CALLOUT_ACTIVE) {
		if (info->normal_termios.c_cflag & CLOCAL)
			do_clocal = 1;
	} else {
		if (tty->termios->c_cflag & CLOCAL)
			do_clocal = 1;
	}
	
	/*
	 * Block waiting for the carrier detect and the line to become
	 * free (i.e., not in use by the callout).  While we are in
	 * this loop, info->count is dropped by one, so that
	 * rs_close() knows when to free things.  We restore it upon
	 * exit, either normal or abnormal.
	 */
	retval = 0;
	add_wait_queue(&info->open_wait, &wait);
#ifdef SERIAL_DEBUG_OPEN
	printk("block_til_ready before block: ttys%d, count = %d\n",
	       info->line, info->count);
#endif
	cli();
	if (!tty_hung_up_p(filp)) 
		info->count--;
	sti();
	info->blocked_open++;
	while (1) {
		current->state = TASK_INTERRUPTIBLE;
		if (tty_hung_up_p(filp) ||
		    !(info->flags & ASYNC_INITIALIZED)) {
#ifdef SERIAL_DO_RESTART
			if (info->flags & ASYNC_HUP_NOTIFY)
				retval = -EAGAIN;
			else
				retval = -ERESTARTSYS;	
#else
			retval = -EAGAIN;
#endif
			break;
		}

		if (!(info->flags & ASYNC_CALLOUT_ACTIVE) &&
		    !(info->flags & ASYNC_CLOSING) &&
		    (do_clocal || (info->flags & ASYNC_DCD_PRESENT)) )
			break;

		if (current->signal & ~current->blocked) {
			retval = -ERESTARTSYS;
			break;
		}
#ifdef SERIAL_DEBUG_OPEN
		printk("block_til_ready blocking: ttys%d, count = %d\n",
		       info->line, info->count);
#endif
		schedule();
	}
	current->state = TASK_RUNNING;
	remove_wait_queue(&info->open_wait, &wait);
	if (!tty_hung_up_p(filp))
		info->count++;
	info->blocked_open--;
#ifdef SERIAL_DEBUG_OPEN
	printk("block_til_ready after blocking: ttys%d, count = %d\n",
	       info->line, info->count);
#endif
	if (retval)
		return retval;
	info->flags |= ASYNC_NORMAL_ACTIVE;
	return 0;
}	

/*
 * This routine is called whenever a serial port is opened.  It
 * enables interrupts for a serial port, linking in its async structure into
 * the IRQ chain.   It also performs the serial-specific
 * initialization for the tty structure.
 */
int rs_open(struct tty_struct *tty, struct file * filp)
{
	struct async_struct	*info;
	int 			retval, line;

	line = MINOR(tty->device) - tty->driver.minor_start;
	if ((line < 0) || (line >= NR_PORTS))
		return -ENODEV;
	info = rs_table + line;
	if (serial_paranoia_check(info, tty->device, "rs_open"))
		return -ENODEV;

#ifdef SERIAL_DEBUG_OPEN
	printk("rs_open %s%d, count = %d\n", tty->driver.name, info->line,
	       info->count);
#endif
	if (!(info->flags & ASYNC_INITIALIZED)) {
		char		device_name[25];
		mach_port_t	device_port;
		dev_mode_t	device_mode;
		kern_return_t	kr;
		int		word, count;

		sprintf(device_name, "com%d", line);

		device_mode = D_READ | D_WRITE;

		kr = device_open(device_server_port,
				 MACH_PORT_NULL,
				 device_mode,
				 server_security_token,
				 device_name,
				 &device_port);

		if (kr != D_SUCCESS) {
			if (kr != D_NO_SUCH_DEVICE) {
				MACH3_DEBUG(2, kr,
					    ("rs_open(dev 0x%x): "
					     "device_open(\"%s\")",
					     tty->device,
					     device_name));
			}
			return -ENODEV;
		}

		info->flags &= ~ASYNC_DCD_PRESENT;

		count = TTY_MODEM_COUNT;
		kr = device_get_status(device_port, 
				TTY_MODEM, &word, &count);

		if (kr == D_SUCCESS && (word & TM_CAR) != 0) 
			info->flags |= ASYNC_DCD_PRESENT;
		else
			info->flags &= ~ASYNC_DCD_PRESENT;

		/* Flush any garbage data which might be left over.. */
		count = TTY_FLUSH_COUNT;
		word = D_READ|D_WRITE;
		kr = device_set_status(device_port, TTY_FLUSH, &word, count);

		info->device_port = device_port;
		tty->flip.char_buf_ptr = tty->flip.char_buf;
		tty->flip.flag_buf_ptr = tty->flip.flag_buf;
		tty->flip.count = 0;

		/* Now register a few asynchronous routines.. */
		device_reply_register(&info->reply_port,
               			(char *) info, serial_read_reply,
				serial_write_reply);

	}
	ASSERT(MACH_PORT_VALID(info->device_port));

	info->count++;
	tty->driver_data = info;
	info->tty = tty;

	/*
	 * Start up serial port
	 */
	retval = startup(info);
	if (retval)
		return retval;

	if (info->count == 1)
		server_thread_start(serial_read_thread, (void *) line);

	retval = block_til_ready(tty, filp, info);
	if (retval) {
#ifdef SERIAL_DEBUG_OPEN
		printk("rs_open returning after block_til_ready with %d\n",
		       retval);
#endif
		return retval;
	}

	if ((info->count == 1) && (info->flags & ASYNC_SPLIT_TERMIOS)) {
		if (tty->driver.subtype == SERIAL_TYPE_NORMAL)
			*tty->termios = info->normal_termios;
		else 
			*tty->termios = info->callout_termios;
		change_speed(info);

	}

	info->session = current->session;
	info->pgrp = current->pgrp;


#ifdef SERIAL_DEBUG_OPEN
	printk("rs_open ttys%d successful...", info->line);
#endif
	return 0;
}

/*
 * ---------------------------------------------------------------------
 * rs_init() and friends
 *
 * rs_init() is called at boot-time to initialize the serial driver.
 * ---------------------------------------------------------------------
 */

/*
 * This routine prints out the appropriate serial driver version
 * number, and identifies which options were configured into this
 * driver.
 */
static void show_serial_version(void)
{
	printk(KERN_INFO "MkLinux Serial Driver version %s\n", serial_version);
}

/*
 * The serial driver boot-time initialization code!
 */
int rs_init(void)
{
	int i;
	struct async_struct * info;
	
	init_bh(SERIAL_BH, do_serial_bh);
	timer_table[RS_TIMER].expires = 0;

	show_serial_version();

	/* Initialize the tty_driver structure */
	
	memset(&serial_driver, 0, sizeof(struct tty_driver));
	serial_driver.magic = TTY_DRIVER_MAGIC;
	serial_driver.name = "ttyS";
	serial_driver.major = TTY_MAJOR;
	serial_driver.minor_start = 64;
	serial_driver.num = NR_PORTS;
	serial_driver.type = TTY_DRIVER_TYPE_SERIAL;
	serial_driver.subtype = SERIAL_TYPE_NORMAL;
	serial_driver.init_termios = tty_std_termios;
	serial_driver.init_termios.c_cflag =
		B9600 | CS8 | CREAD | HUPCL | CLOCAL;
	serial_driver.flags = TTY_DRIVER_REAL_RAW;
	serial_driver.refcount = &serial_refcount;
	serial_driver.table = serial_table;
	serial_driver.termios = serial_termios;
	serial_driver.termios_locked = serial_termios_locked;

	serial_driver.open = rs_open;
	serial_driver.close = rs_close;
	serial_driver.write = rs_write;
	serial_driver.put_char = rs_put_char;
	serial_driver.flush_chars = rs_flush_chars;
	serial_driver.write_room = rs_write_room;
	serial_driver.chars_in_buffer = rs_chars_in_buffer;
	serial_driver.flush_buffer = rs_flush_buffer;
	serial_driver.ioctl = rs_ioctl;
	serial_driver.throttle = rs_throttle;
	serial_driver.unthrottle = rs_unthrottle;
	serial_driver.set_termios = rs_set_termios;
	serial_driver.stop = rs_stop;
	serial_driver.start = rs_start;
	serial_driver.hangup = rs_hangup;

	/*
	 * The callout device is just like normal device except for
	 * major number and the subtype code.
	 */
	callout_driver = serial_driver;
	callout_driver.name = "cua";
	callout_driver.major = TTYAUX_MAJOR;
	callout_driver.subtype = SERIAL_TYPE_CALLOUT;

	if (tty_register_driver(&serial_driver))
		panic("Couldn't register serial driver\n");
	if (tty_register_driver(&callout_driver))
		panic("Couldn't register callout driver\n");
	
	for (i = 0, info = rs_table; i < NR_PORTS; i++,info++) {
		info->magic = SERIAL_MAGIC;
		info->line = i;
		info->tty = 0;
		info->close_delay = 5*HZ/10;
		info->closing_wait = 30*HZ;
		info->event = 0;
		info->count = 0;
		info->blocked_open = 0;
		info->tqueue.routine = do_softint;
		info->tqueue.data = info;
		info->tqueue_hangup.routine = do_serial_hangup;
		info->tqueue_hangup.data = info;
		info->callout_termios =callout_driver.init_termios;
		info->normal_termios = serial_driver.init_termios;
		info->open_wait = 0;
		info->close_wait = 0;
	}

	return 0;
}

static void
serial_put_status_byte(struct async_struct *info, 
			unsigned char	byte)
{
	if (info->tty == NULL)
		return;

	if (info->tty->flip.count >= TTY_FLIPBUF_SIZE)
		return;

	*info->tty->flip.flag_buf_ptr++ = byte;
	*info->tty->flip.char_buf_ptr++ = 0;
	info->tty->flip.count++;
	queue_task_irq_off(&info->tty->flip.tqueue, &tq_timer);
}

static void
serial_handle_oob_event(struct async_struct *info,
			mach_port_t	device_port)
{
	kern_return_t			kr;
	mach_msg_type_number_t	count;
	struct tty_out_of_band	toob;

	count = TTY_OUT_OF_BAND_COUNT;
	kr = device_get_status(device_port, TTY_OUT_OF_BAND,
				(dev_status_t) &toob, &count);

	if (kr != D_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("serial_handle_oob_event(%d):device_get_status",kr ));
		return;
	}

	switch (toob.toob_event) {
	case TOOB_NO_EVENT:
		break;

	case TOOB_BREAK:
		serial_put_status_byte(info, TTY_BREAK);
		break;

	case TOOB_BAD_PARITY:
		serial_put_status_byte(info, TTY_PARITY);
		break;

	case TOOB_FLUSHED:
		break;

	case TOOB_CARRIER:
		if (toob.toob_arg) {
			info->flags |= ASYNC_DCD_PRESENT;
			wake_up_interruptible(&info->open_wait);
		} else {
			info->flags &= ~ASYNC_DCD_PRESENT;

			if (!((info->flags & ASYNC_CALLOUT_ACTIVE) &&
			(info->flags & ASYNC_CALLOUT_NOHUP))) {
#ifdef SERIAL_DEBUG_OPEN
				printk("scheduling hangup...");
#endif
                        	queue_task_irq_off(&info->tqueue_hangup, &tq_scheduler);
			}
		}
		break;

	default:
		printk("serial_handle_oob_event: unknown event 0x%x\n",
		       toob.toob_event);
		break;
	}

	return;
}

void *
serial_read_thread(
	void	*arg)
{
	struct server_thread_priv_data	priv_data;
	kern_return_t			kr;
	struct async_struct	*info;
	int				line, wait_loop, count;
	io_buf_ptr_inband_t		inbuf;		/* 128 chars */
	mach_msg_type_number_t		data_count;
	mach_port_t			device_port;

	cthread_set_name(cthread_self(), "serial read");
	server_thread_set_priv_data(cthread_self(), &priv_data);

	line = (int) arg;
	info = rs_table + line;
	uniproc_enter();
	device_port = info->device_port;

	for (;;) {
		data_count = sizeof inbuf;
		uniproc_exit();
		kr = device_read_inband(device_port,
					0,	/* mode */
					0,	/* recnum */
					sizeof inbuf,
					inbuf,
					&data_count);
		uniproc_enter();

		if (kr == D_OUT_OF_BAND) {
			serial_handle_oob_event(info, device_port);
			continue;
		} else if (kr != D_SUCCESS) {
			/* Something happened.. simply shutdown the line.. */
			if (!((info->flags & ASYNC_CALLOUT_ACTIVE) &&
                           (info->flags & ASYNC_CALLOUT_NOHUP))) 
                       		queue_task_irq_off(&info->tqueue_hangup, &tq_scheduler);
			uniproc_exit();
			cthread_detach(cthread_self());
			cthread_exit((void *) 0);
			/* NEVER REACHED */
		}

		if (data_count <= 0) 
			continue;

		/*
		 * Its very possible with the Power Mac
		 * to overflow the Linux TTY buffers.
		 * (A serial interrupt can present up to
		 * 8K worth of data in one shot)
		 *
		 * The following loops attempts to give the
		 * PPP line disc. a chance to clear the queue
		 * out.
		 */

		for (wait_loop = 0; wait_loop < 6; wait_loop++) {
			/* Check to make sure the tty did not
			 * go away..
			 */

			if (info->tty == NULL)
				break;

			if ((info->tty->flip.count+data_count) < TTY_FLIPBUF_SIZE)
				break;

			/* Try to give another thread some time.. */
			osfmach3_yield();
		}

		if (info->tty == NULL)
			continue;

		count = MIN(TTY_FLIPBUF_SIZE + info->tty->flip.count, data_count);
		if (count <= 0)
			continue;

		info->last_active = jiffies; 
		info->tty->flip.count += count;
		memcpy(info->tty->flip.char_buf_ptr, inbuf, count);
		memset(info->tty->flip.flag_buf_ptr, 0, count);
		info->tty->flip.flag_buf_ptr += count;
		info->tty->flip.char_buf_ptr += count;
		queue_task_irq_off(&info->tty->flip.tqueue, &tq_timer);
	}
}
