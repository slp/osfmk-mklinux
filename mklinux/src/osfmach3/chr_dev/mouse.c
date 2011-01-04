/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <osfmach3/server_thread.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/device_utils.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/mach_init.h>

#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/malloc.h>
#include <linux/miscdevice.h>

mach_port_t	mouse_port = MACH_PORT_NULL;
int		mouse_refs;


/* Note: MOUSE_BUF_SIZE must be a power of 2 */
#define MOUSE_BUF_SIZE	2048
struct mouse_queue_struct {
	unsigned long	head;
	unsigned long	tail;
	struct wait_queue	*proc_list;
	unsigned char		buf[MOUSE_BUF_SIZE];
};
struct mouse_queue_struct *mouse_queue;

unsigned int get_from_mouse_queue(void)
{
	unsigned int result;

	result = mouse_queue->buf[mouse_queue->tail];
	mouse_queue->tail = (mouse_queue->tail + 1) & (MOUSE_BUF_SIZE-1);
	return result;
}

static inline int mouse_queue_empty(void)
{
	return mouse_queue->head == mouse_queue->tail;
}

void *
mouse_input_thread(
	void	*mouse_port_handle)
{
	struct server_thread_priv_data	priv_data;
	kern_return_t			kr;
	io_buf_ptr_inband_t		inbuf;	/* 128 chars */
	mach_msg_type_number_t 		count;
	int				head, maxhead;
	int 				i;
	
	cthread_set_name(cthread_self(), "mouse input");
	server_thread_set_priv_data(cthread_self(), &priv_data);

	uniproc_enter();

	/* interactive response is important, therefore high priority */
	server_thread_priorities(BASEPRI_SERVER-2, BASEPRI_SERVER-2);

	for (;;) {
		count = sizeof inbuf;
		server_thread_blocking(FALSE);
		kr = device_read_inband(mouse_port, 0, 0,
					sizeof inbuf, inbuf, &count);
		server_thread_unblocking(FALSE);
		if (kr != D_SUCCESS) {
			if (kr == D_DEVICE_DOWN ||
			    kr == MACH_SEND_INVALID_DEST ||
			    kr == MACH_SEND_INTERRUPTED ||
			    kr == MACH_RCV_INTERRUPTED) {
				/* device has been closed */
				uniproc_exit();
				cthread_detach(cthread_self());
				cthread_exit((void *) 0);
			}
			MACH3_DEBUG(0, kr,
				    ("mouse_input_thread: device_read_inband"));
			panic("mouse_input_thread: input error");
		}

		head = mouse_queue->head;
		maxhead = (mouse_queue->tail-1) & (MOUSE_BUF_SIZE-1);
		for (i = 0; i < count; i++) {
			mouse_queue->buf[head] = inbuf[i];
			if (head != maxhead) {
				head++;
				head &= MOUSE_BUF_SIZE-1;
			}
		}
		mouse_queue->head = head;
		wake_up_interruptible(&mouse_queue->proc_list);
	}
	/*NOTREACHED*/
}

int mouse_open(
	struct inode	*inode,
	struct file	*file)
{
	kern_return_t	kr;
	char		*mach_mouse_name;

	if (mouse_refs++ > 0) {
		return 0;
	}

	ASSERT(mouse_port == MACH_PORT_NULL);
	switch (MINOR(inode->i_rdev)) {
	    case BUSMOUSE_MINOR:
	    case MS_BUSMOUSE_MINOR:
	    case ATIXL_BUSMOUSE_MINOR:
	    case PSMOUSE_MINOR:
		mach_mouse_name = "mouse148";
		break;
#if 0
	    case PSMOUSE_MINOR:
		mach_mouse_name = "mouse144";
		break;
#endif
	    default:
		return -ENODEV;
	}
	kr = device_open(device_server_port,
			 MACH_PORT_NULL,
			 D_READ | D_WRITE,
			 server_security_token,
			 mach_mouse_name,
			 &mouse_port);
	if (kr != KERN_SUCCESS) {
		if (kr != D_NO_SUCH_DEVICE) {
			MACH3_DEBUG(1, kr,
				    ("mouse_open(0x%x): "
				     "device_open(\"mouse0\")",
				     inode->i_rdev));
		}
		return -ENODEV;
	}
	mouse_queue->head = mouse_queue->tail = 0;
	server_thread_start(mouse_input_thread, (void *) 0);

	return 0;
}

void mouse_release(
	struct inode	*inode,
	struct file	*file)
{
	kern_return_t	kr;

	if (--mouse_refs == 0) {
		kr = device_close(mouse_port);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("mouse_release(0x%x): device_close(0x%x)",
				     inode->i_rdev, mouse_port));
			panic("mouse_release: device_close failed");
		}
		kr = mach_port_destroy(mach_task_self(), mouse_port);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("mouse_release(0x%x): "
				     "mach_port_destroy(0x%x)",
				     inode->i_rdev, mouse_port));
			panic("mouse_release: mach_port_destroy failed");
		}
		mouse_port = MACH_PORT_NULL;
	}
}

int mouse_read(
	struct inode	*inode,
	struct file	*file,
	char		*buffer,
	int		count)
{
	struct wait_queue wait = { current, NULL };
	int i = count;
	unsigned char c;

	if (mouse_queue_empty()) {
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;
		add_wait_queue(&mouse_queue->proc_list, &wait);
	    repeat:
		current->state = TASK_INTERRUPTIBLE;
		if (mouse_queue_empty() &&
		    !(current->signal & ~current->blocked)) {
			schedule();
			goto repeat;
		}
		current->state = TASK_RUNNING;
		remove_wait_queue(&mouse_queue->proc_list, &wait);
	}
	while (i > 0 && !mouse_queue_empty()) {
		c = get_from_mouse_queue();
		put_fs_byte(c, buffer++);
		i--;
	}
	if (count - i) {
		inode->i_atime = CURRENT_TIME;
		return count - i;
	}
	if (current->signal & ~current->blocked)
		return -ERESTARTSYS;
	return 0;
}

int mouse_write(
	struct inode	*inode,
	struct file	*file,
	const char	*buffer,
	int		count)
{
	return -EINVAL;
}

int mouse_select(
	struct inode	*inode,
	struct file	*file,
	int		sel_type,
	select_table	*wait)
{
	if (sel_type != SEL_IN)
		return 0;
	if (!mouse_queue_empty())
		return 1;
	select_wait(&mouse_queue->proc_list, wait);
	return 0;
}

struct file_operations mouse_fops = {
        NULL,		/* seek */
	mouse_read,
	mouse_write,
	NULL,		/* readdir */
	mouse_select,
	NULL,		/* ioctl */
	NULL,		/* mmap */
        mouse_open,
        mouse_release
};

int mouse_init(void)
{
	mouse_queue = (struct mouse_queue_struct *)
		kmalloc(sizeof (*mouse_queue), GFP_KERNEL);
	if (mouse_queue == NULL)
		panic("mouse_init: can't allocate mouse_queue");
	mouse_queue->head = mouse_queue->tail = 0;
	mouse_queue->proc_list = NULL;
	mouse_refs = 0;

	return 0;
}
