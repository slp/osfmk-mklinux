/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <cthreads.h>

#include <osfmach3/parent_server.h>
#include <osfmach3/console.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/server_thread.h>
#include <osfmach3/device_utils.h>
#include <osfmach3/char_dev.h>
#include <osfmach3/mach3_debug.h>

#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/termios.h>
#include <linux/tty_driver.h>
#include <linux/major.h>
#include <linux/tty.h>
#include <linux/errno.h>

extern void register_console(void (*proc)(const char *));

extern struct tty_driver console_driver;
static int osfmach3_console_refcount;
static struct tty_struct *osfmach3_console_table[MAX_NR_CONSOLES];
static struct termios *osfmach3_console_termios[MAX_NR_CONSOLES];
static struct termios *osfmach3_console_termios_locked[MAX_NR_CONSOLES];

int osfmach3_con_refs[MAX_NR_CONSOLES] = {0, };

memory_object_t	osfmach3_video_memory_object;
vm_address_t	osfmach3_video_physaddr;
vm_address_t	osfmach3_video_map_base;
vm_size_t	osfmach3_video_map_size = 0;
unsigned long	osfmach3_video_offset = 0;
mach_port_t	osfmach3_video_port = MACH_PORT_NULL;

/*
 * Use Mach console instead of a video adapter + raw keyboard ?
 * This is useful when using the remote Mach console on a serial line,
 * to have the Linux console on the serial line too.
 */
int osfmach3_use_mach_console = 0;

int osfmach3_con_open(struct tty_struct *tty, struct file *filp)
{
	int result;

	result = osfmach3_tty_open(tty);
	if (result)
		return result;

#ifdef	PPC
	/*
	 * XXX
	 * This is mostly a hack to get the console size.
	 */
	{
		struct vc_info {
			unsigned long   v_height;
			unsigned long   v_width;
			unsigned long   v_depth;
			unsigned long   v_rowbytes;
			unsigned long   v_baseaddr;
			unsigned long   v_type;
			char            v_name[32];
			unsigned long   v_reserved[9];
		} vi;
#define VC_GETINFO	(('v'<<16)|1)
		kern_return_t		kr;
		mach_msg_type_number_t	count;

		count = sizeof vi / sizeof (int);
		kr = device_get_status(osfmach3_console_port,
				       VC_GETINFO,
				       (dev_status_t) &vi,
				       &count);
		if (kr == D_SUCCESS) {
			struct tty_struct *tty;

			tty = console_driver.table[0];
			/* the following assumes a 8x16 character size */
			tty->winsize.ws_row = vi.v_height / 16;
			tty->winsize.ws_col = vi.v_width / 8;
		}
	}
#endif	/* PPC */
	return 0;
}

int osfmach3_con_write(struct tty_struct * tty, int from_user,
	      const unsigned char *buf, int count)
{
	int c, n = 0;

	while (!tty->stopped && count) {
		c = from_user ? get_fs_byte(buf) : *buf;
		buf++; n++; count--;

		cnputc(c);
	}
	cnflush();

	return n;
}

int osfmach3_con_write_room(struct tty_struct *tty)
{
	if (tty->stopped)
		return 0;
	return 4096;	/* No limit, really: we're not buffering */
}

int osfmach3_con_chars_in_buffer(struct tty_struct *tty)
{
	return 0;	/* we're not buffering */
}

void osfmach3_con_stop(struct tty_struct *tty)
{
}

void osfmach3_con_start(struct tty_struct *tty)
{
#if 0
	if (! parent_server) {
		osfmach3_tty_start(tty);
	}
#endif
}

void osfmach3_con_throttle(struct tty_struct *tty)
{
}

void osfmach3_con_unthrottle(struct tty_struct *tty)
{
}

int osfmach3_vt_ioctl(struct tty_struct *tty, struct file * file,
		      unsigned int cmd, unsigned long arg)
{
	return -ENOIOCTLCMD;
}

unsigned long osfmach3_con_init(unsigned long kmem_start)
{
	int ret;

	memset(&console_driver, 0, sizeof (struct tty_driver));
	console_driver.magic = TTY_DRIVER_MAGIC;
	console_driver.name = "tty";
	console_driver.name_base = 1;
	console_driver.major = TTY_MAJOR;
	console_driver.minor_start = 1;
	console_driver.num = MAX_NR_CONSOLES;
	console_driver.type = TTY_DRIVER_TYPE_CONSOLE;
	console_driver.init_termios = tty_std_termios;
	console_driver.flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_RESET_TERMIOS;
	console_driver.refcount = &osfmach3_console_refcount;
	console_driver.table = osfmach3_console_table;
	console_driver.termios = osfmach3_console_termios;
	console_driver.termios_locked = osfmach3_console_termios_locked;

	console_driver.open = osfmach3_con_open;
	console_driver.write = osfmach3_con_write;
	console_driver.write_room = osfmach3_con_write_room;
	console_driver.chars_in_buffer = osfmach3_con_chars_in_buffer;
	console_driver.ioctl = osfmach3_vt_ioctl;
	console_driver.stop = osfmach3_con_stop;
	console_driver.start = osfmach3_con_start;
	console_driver.throttle = osfmach3_con_throttle;
	console_driver.unthrottle = osfmach3_con_unthrottle;
	
	if (tty_register_driver(&console_driver))
		panic("Couldn't register console driver\n");
	
	ret = osfmach3_register_chrdev(console_driver.major,
				       console_driver.name,
				       console_driver.minor_start +
				       console_driver.num,
				       osfmach3_tty_dev_to_name,
				       osfmach3_con_refs);
	if (ret) {
		printk("con_init: osfmach3_register_chrdev failed %d\n", ret);
		panic("con_init: can't register Mach device");
	}

	if (parent_server) {
		printk("Console: parent process's terminal\n");
	} else {
		printk("Console: Mach console\n");
	}

#if 0
	/* already done in osfmach3_console_init() */
	register_console(console_print);
#endif

	return kmem_start;
}


void *
console_read_thread(
	void 	*tty_handle)
{
	struct server_thread_priv_data	priv_data;
	struct tty_struct	*tty;
	int			ret;
	unsigned char 		c;
	struct tty_struct	**ttytab;

	cthread_set_name(cthread_self(), "console read");
	server_thread_set_priv_data(cthread_self(), &priv_data);

	tty = (struct tty_struct *) tty_handle;
	ttytab = console_driver.table;
	uniproc_enter();

	for (;;) {
		ret = cngetc();
		if (ret == -1)
			panic("console_read_thread: cngetc returned -1!\n");
		c = (unsigned char) ret;
		if (tty_handle == NULL) {
			/*
			 * Virtual console: find the active one.
			 */
			tty = ttytab[fg_console];
		}
		tty->ldisc.receive_buf(tty, &c, 0, 1);
	}
	/*NOTREACHED*/
		
}

void
osfmach3_launch_console_read_thread(
	void *tty_handle)
{
	static boolean_t launched = FALSE;

	if (launched || osfmach3_keyboard_port != MACH_PORT_NULL) {
		/*
		 * There is already a console_read_thread running,
		 * or we don't need one since we use the raw keyboard driver
		 * directly.
		 */
		return;
	}
	launched = TRUE;
	server_thread_start(console_read_thread, tty_handle);
}

/*
 * Console output.
 */

mach_port_t	osfmach3_console_port;

/*
 * device_write_inband() can't deal with more than 128 characters at
 * a time. It will return MIG_ARRAY_TOO_LARGE otherwise...
 * Check the file {mk_release_directory}/include/device/device.defs for
 * the MiG stub, and device_types.defs for the argument type and its length
 * restriction.
 */
#define	CONS_BUF_SIZE	128
struct {
	char		chars[CONS_BUF_SIZE];
	int		pos;
	int		initialized;
} cons_buf;

int
cnputc(
	int	c)
{
	cons_buf.chars[cons_buf.pos++] = c;
	if (cons_buf.pos >= CONS_BUF_SIZE ||
	    c == '\n' || c == '\r') {
		cnflush();
	}

	return 0;
}

void
cnflush(void)
{
	if (parent_server)
		parent_server_write(1, cons_buf.chars, cons_buf.pos);
	else
	{
		unsigned int cw;
		int start_pos;
		kern_return_t kr;

		kr = D_SUCCESS;
		start_pos = 0;
		while (start_pos < cons_buf.pos) {
			kr = device_write_inband(osfmach3_console_port,
						 0, 0,
						 &cons_buf.chars[start_pos],
						 cons_buf.pos - start_pos,
						 &cw);
			start_pos += cw;
		}
		if (kr != D_SUCCESS) {
			char *text = "panic: cnflush.\n\r";
			(void) device_write_inband(osfmach3_console_port, 0, 0,
						   text, strlen(text), &cw);
			kr /= 0;	/* suicide, since panic may fail */
		}
		start_pos += cw;
	}
	cons_buf.pos = 0;
}

void
osfmach3_console_print(const char *b)
{
	char c;

	while ((c = *(b++)) != 0) {
		cnputc(c);
		if (c == '\n') {
			cnputc('\r');
			cnflush();
		}
	}
	cnflush();
}

int osfmach3_console_initialized = 0;

void
osfmach3_console_init(void)
{
	kern_return_t	kr;

	if (osfmach3_console_initialized != 0)
		return;
	osfmach3_console_initialized = 1;

	if (parent_server) {
		parent_server_grab_console();
	} else {

		/*
		 * Open the console
		 */
		kr = device_open(device_server_port, MACH_PORT_NULL,
				 D_READ|D_WRITE, server_security_token,
				 "console", &osfmach3_console_port);
		if (kr != KERN_SUCCESS)
			panic("console_init");
	}

	register_console(osfmach3_console_print);
}

int
cngetc(void)
{
	char c;
	int n;
	kern_return_t kr;

	if (parent_server) {
		server_thread_blocking(FALSE);
		n = parent_server_read(0, &c, sizeof c);
		server_thread_unblocking(FALSE);
		if (n <= 0) {
			if (n < 0)
				printk("cngetc: read error %d\n",
				       parent_server_errno);
			else
				printk("cngetc: EOF\n");
			return -1;
		}
		return (int) c;
	}
	
	n = sizeof c;
	server_thread_blocking(FALSE);
	kr = device_read_inband(osfmach3_console_port, 0, 0, sizeof c, &c, &n);
	server_thread_unblocking(FALSE);
	if (kr != D_SUCCESS) {
		MACH3_DEBUG(0, kr, ("cngetc: read"));
		return -1;
	}
	if (n <= 0) {
		printk("cngetc: read returned %d bytes", n);
		return -1;
	}
	return ((int) c) & 0xff;
}

/* Get string with minimal line editing.  Based on kernel/i386/rdb/gets.c.
   Note that there is no checking for overflow of the buffer.  Also, on the
   second server our attempts at echoing and editing are probably for naught
   since the second console is most likely still in cooked mode when we are
   called.  But the only visible effect in that case is that the entered
   line gets printed in its entirety once you hit RETURN, since we get and
   echo all the entered characters at once.  */
char *gets(
	char buf[GETS_MAX])
{
	int c, i;

	i = 0;
	for (;;) {
		c = cngetc();
		switch (c) {
		case '\n':
		case '\r':
			cnputc('\n');
			cnputc('\r');
			cnflush();
			buf[i] = '\0';
			return(buf);
		case '\b':
		case '#':
			if (i > 0) {
				cnputc('\b');
				cnputc(' ');
				cnputc('\b');
				cnflush();
				i--;
			}
			continue;
		case '@':
		case 'u' & 037:
			i = 0;
			cnputc('\n');
			cnputc('\r');
			cnflush();
			continue;
		case -1:
		case '\04':		/* ^D - return null pointer */
			cnputc('\n');
			cnputc('\r');
			cnflush();
			return((char *) 0);
		default:
			if (i >= GETS_MAX - 1)
				cnputc('\a');
			else {
				cnputc(c);
				buf[i++] = c;
			}
			cnflush();
		}
	}
}
