/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 * Handler for device read and write replies.  Simulates an
 * interrupt handler.
 */

#include <cthreads.h>
#include <mach/mig_errors.h>
#include <mach/mach_traps.h>

#include <osfmach3/mach_init.h>
#include <osfmach3/device_reply_hdlr.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/queue.h>
#include <osfmach3/assert.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/server_thread.h>

#include <linux/kernel.h>
#include <linux/malloc.h>
#include <linux/mm.h>

extern kern_return_t io_done_queue_wait(
	mach_port_t		queue,
	io_done_result_t	*result);

#if	! DEV_REPLY_MESSAGE
mach_port_t	device_reply_queue;	/* Device reply queue handle */
unsigned int	device_reply_id = 0;	/* Incremented for each request */
#endif

struct dev_reply_hash_elt {
	queue_head_t queue;
	int count;
};

struct dev_reply_entry {
#if	! DEV_REPLY_PORT_ALIAS && DEV_REPLY_MESSAGE
	queue_chain_t	chain;
#endif
	mach_port_t	port;
	char		*object;
	dev_reply_t	read_reply;
	dev_reply_t	write_reply;
};

mach_port_t	dev_reply_port_set = MACH_PORT_NULL;
void		*device_reply_thread(void *arg);	/* forward */

#if	! DEV_REPLY_PORT_ALIAS
/*
 *	We add the low 8 bits to the high 24 bits, because
 *	this interacts well with the way the IPC implementation
 *	allocates port nms.
 */
#define	DEV_REPLY_HASH(port)	\
	((((port) & 0xff) + ((port) >> 8)) & (DEV_REPLY_HASH_SIZE - 1))
#define	DEV_REPLY_HASH_SIZE	64
struct dev_reply_hash_elt \
	dev_reply_hash_table[DEV_REPLY_HASH_SIZE];
#endif	/* ! DEV_REPLY_PORT_ALIAS */

void
device_reply_register(
	mach_port_t	*portp,
	char		*object,
	dev_reply_t	read_reply,
	dev_reply_t	write_reply)
{
	struct dev_reply_entry *entry;
#if	DEV_REPLY_PORT_ALIAS
	kern_return_t kr;
#endif
#if	!DEV_REPLY_MESSAGE && DEV_REPLY_MESSAGE
	struct dev_reply_hash_elt *elt;
#endif

	/* 
	 * Use GFP_ATOMIC here because we might be called from sys_bdflush
	 * and we don't want to have to block for a page since we're 
	 * supposed to free some...
	 */
	entry = (struct dev_reply_entry *)
		kmalloc(sizeof (struct dev_reply_entry), GFP_ATOMIC);
	if (entry == NULL) {
		panic("device_reply_register: can't allocate entry\n");
	}
	entry->object = object;
	entry->read_reply = read_reply;
	entry->write_reply = write_reply;

#if	DEV_REPLY_PORT_ALIAS || ! DEV_REPLY_MESSAGE

#if	DEV_REPLY_PORT_ALIAS
	kr = mach_port_allocate_name(mach_task_self(),
				     MACH_PORT_RIGHT_RECEIVE,
				     (mach_port_t)entry);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("device_reply_register: "
			     "mach_port_allocate_name(0x%x)",
			     entry));
		panic("device_reply_register: can't allocate reply port");
	}
#endif

	*portp = entry->port = (mach_port_t)entry;

#else	/* DEV_REPLY_PORT_ALIAS || ! DEV_REPLY_MESSAGE */

	entry->port = mach_reply_port();
	if (! MACH_PORT_VALID(entry->port)) {
		printk("mach_reply_port() returned port=0x%x\n", *portp);
		panic("device_reply_register: mach_reply_port()");
	}
	*portp = entry->port;

	elt = &dev_reply_hash_table[DEV_REPLY_HASH(entry->port)];
	queue_enter(&elt->queue, entry, struct dev_reply_entry *, chain);
	elt->count++;

#endif	/* DEV_REPLY_PORT_ALIAS */

#if	DEV_REPLY_MESSAGE

	kr = mach_port_move_member(mach_task_self(),
				   entry->port, dev_reply_port_set);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("device_reply_register: "
			     "mach_port_move_member(0x%x, 0x%x)",
			     entry->port, dev_reply_port_set));
		panic("device_reply_register: can't add port to set");
	}

#endif	/* DEV_REPLY_MESSAGE */

}

boolean_t
device_reply_lookup(
	mach_port_t	port,
	dr_select_t	which,
	char		**object,	/* OUT */
	dev_reply_t	*func)		/* OUT */
{
#if	!DEV_REPLY_PORT_ALIAS && DEV_REPLY_MESSAGE
	struct dev_reply_hash_elt *elt;
#endif
	struct dev_reply_entry *entry;

#if	DEV_REPLY_PORT_ALIAS || ! DEV_REPLY_MESSAGE

#if	DEV_REPLY_PORT_ALIAS
	if (port == MACH_PORT_DEAD) {
		printk("device_reply_lookup: dead reply port!!\n");
		return FALSE;
	}
#endif
	entry = (struct dev_reply_entry *) port;
	ASSERT(entry->port == port);
	*object = entry->object;
	*func   = (which == DR_WRITE) ? entry->write_reply : entry->read_reply;

	return TRUE;

#else	/* DEV_REPLY_PORT_ALIAS || ! DEV_REPLY_MESSAGE */

	elt = &dev_reply_hash_table[DEV_REPLY_HASH(port)];
	for (entry = (struct dev_reply_entry *)queue_first(&elt->queue);
	     !queue_end(&elt->queue, (queue_entry_t)entry);
	     entry = (struct dev_reply_entry *)queue_next(&entry->chain)) {
		if (entry->port == port) {
			*object = entry->object;
			*func   = (which == DR_WRITE) ? entry->write_reply
						      : entry->read_reply;
			return TRUE;
		}
	}
	return FALSE;

#endif	/* DEV_REPLY_PORT_ALIAS */
}

void
device_reply_deregister(
	mach_port_t	port)
{
#if	!DEV_REPLY_PORT_ALIAS && DEV_REPLY_MESSAGE
	struct dev_reply_hash_elt *elt;
#endif
	struct dev_reply_entry *entry;

#if	DEV_REPLY_MESSAGE

	kern_return_t kr;

	kr = mach_port_move_member(mach_task_self(), port, MACH_PORT_NULL);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("device_reply_deregister: "
			     "mach_port_move_member(0x%x, 0)", port));
	}

	kr = mach_port_destroy(mach_task_self(), port);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr, 
			    ("device_reply_register: mach_port_destroy(0x%x)",
			     port));
	}

#endif	/* DEV_REPLY_MESSAGE */

#if	DEV_REPLY_PORT_ALIAS || ! DEV_REPLY_MESSAGE

	entry = (struct dev_reply_entry *) port;
	ASSERT(entry->port == port);

#else	/* DEV_REPLY_PORT_ALIAS || ! DEV_REPLY_MESSAGE */

	elt = &dev_reply_hash_table[DEV_REPLY_HASH(port)];
	for (entry = (struct dev_reply_entry *) queue_first(&elt->queue);
	     !queue_end(&elt->queue, (queue_entry_t) entry);
	     entry = (struct dev_reply_entry *)queue_next(&entry->chain)) {
		if (entry->port == port) {
			queue_remove(&elt->queue, entry,
				     struct dev_reply_entry *, chain);
			elt->count--;
			break;
		}
	}

#endif	/* DEV_REPLY_PORT_ALIAS */

	if (entry != NULL && entry->port == port) {
		kfree(entry);
	} else {
		printk("device_reply_deregister: can't find entry.\n");
	}
}

kern_return_t
device_open_reply(
	mach_port_t	reply_port,
	kern_return_t	return_code,
	mach_port_t	device_port)
{
	printk("device_open_reply: called !!\n");
	return FALSE;
}

kern_return_t
device_write_reply(
	mach_port_t	reply_port,
	kern_return_t	return_code,
	int		bytes_written)
{
	char		*object;
	dev_reply_t	func;

	if (!device_reply_lookup(reply_port, DR_WRITE, &object, &func))
		return FALSE;

	return ((*func)(object, return_code, NULL, bytes_written));
}

kern_return_t
device_write_reply_inband(
	mach_port_t	reply_port,
	kern_return_t	return_code,
	int		bytes_written)
{
	char		*object;
	dev_reply_t	func;

	if (!device_reply_lookup(reply_port, DR_WRITE, &object, &func))
		return FALSE;

	return ((*func)(object, return_code, NULL, bytes_written));
}

kern_return_t
device_read_reply(
	mach_port_t	reply_port,
	kern_return_t	return_code,
	io_buf_ptr_t	data,
	unsigned int	data_count)
{
	char		*object;
	dev_reply_t	func;

	if (!device_reply_lookup(reply_port, DR_READ, &object, &func))
		return FALSE;

	return ((*func)(object, return_code, data, data_count));
}

kern_return_t
device_read_reply_inband(
	mach_port_t	reply_port,
	kern_return_t	return_code,
	io_buf_ptr_t	data,
	unsigned int	data_count)
{
	char		*object;
	dev_reply_t	func;

	if (!device_reply_lookup(reply_port, DR_READ, &object, &func))
		return FALSE;

	return ((*func)(object, return_code, data, data_count));
}

int device_overwrite = 1;	/* use device_overwrite interfaces */

kern_return_t
device_read_reply_overwrite(
	mach_port_t	reply_port,
	kern_return_t	return_code,
	unsigned int	data_count)
{
	char		*object;
	dev_reply_t	func;

	if (!device_overwrite) {
		panic("device_read_reply_overwrite: called !\n");
		return FALSE;
	}
	if (!device_reply_lookup(reply_port, DR_READ, &object, &func))
		return FALSE;

	return ((*func)(object, return_code, 0, data_count));
}

void *
device_reply_thread(
	void	*arg)
{
	struct server_thread_priv_data	priv_data;
	kern_return_t			kr;

#if	DEV_REPLY_MESSAGE
	union reply_msg {
		mach_msg_header_t	hdr;
		char		space[8192];
	} *reply_msg;
#endif	/* DEV_REPLY_MESSAGE */

	cthread_set_name(cthread_self(), "device reply thread");
	server_thread_set_priv_data(cthread_self(), &priv_data);

	uniproc_enter();

#if	DEV_REPLY_MESSAGE

	/* Allocate the reply message.  Don't put it on the stack,
	 * because we want to be able to wire small stacks when running
         * in kernel mode.
	 */
	kr = vm_allocate(mach_task_self(), (vm_address_t *) &reply_msg,
			 sizeof(union reply_msg), TRUE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr, ("device_reply_thread: vm_allocate"));
		panic("device_reply_thread alloc");
	}

#endif	/* DEV_REPLY_MESSAGE */

#if 0
	server_thread_priorities(BASEPRI_SERVER+2, BASEPRI_SERVER+2);
#else
	server_thread_priorities(BASEPRI_SERVER-1, BASEPRI_SERVER-1);
#endif

#if	DEV_REPLY_MESSAGE

	for (;;) {
		/*
	 	 * We KNOW that none of these messages have replies...
	 	 */
		mig_reply_error_t	rep_rep_msg;

		server_thread_blocking(FALSE);
		kr = mach_msg(&reply_msg->hdr, MACH_RCV_MSG,
			      0, sizeof *reply_msg, dev_reply_port_set,
			      MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
		server_thread_unblocking(FALSE);
		if (kr == MACH_MSG_SUCCESS) {
			if (device_reply_server(&reply_msg->hdr,
						&rep_rep_msg.Head)) {
				/*
				 * None of these messages need replies
				 */
			} else {
				printk("device_reply_thread: invalid msg.\n");
			}
		}
	}

#else	/* ! DEV_REPLY_MESSAGE */

	for (;;) {
		io_done_result_t	result;

		server_thread_blocking(FALSE);
		kr = io_done_queue_wait(device_reply_queue, &result);
		server_thread_unblocking(FALSE);
		if (kr == KERN_SUCCESS) {
			switch (result.qd_type) {
			    case IO_DONE_WRITE:
				(void) device_write_reply(
						result.qd_reqid, 
						result.qd_code,
						result.qd_count);
				break;
			    case IO_DONE_READ:
				device_read_reply(
						result.qd_reqid, 
						result.qd_code,
						result.qd_data,
						result.qd_count);
				break;
			    case IO_DONE_OVERWRITE:
				(void) device_read_reply_overwrite(
						result.qd_reqid, 
						result.qd_code,
						result.qd_count);
				break;
			    default:
				printk("device_reply_thread: invalid reply.\n");
				break;
			}
		}
		else {
		    MACH3_DEBUG(1, kr,
				("device_reply_thread: io_done_queue_wait"));
		}
	}

#endif	/* DEV_REPLY_MESSAGE */
}

void
device_reply_hdlr(void)
{
#if	!DEV_REPLY_PORT_ALIAS && DEV_REPLY_MESSAGE
	int	i;
#endif
	kern_return_t	kr;

#if	! DEV_REPLY_PORT_ALIAS && DEV_REPLY_MESSAGE
	for (i = 0; i < DEV_REPLY_HASH_SIZE; i++) {
		queue_init(&dev_reply_hash_table[i].queue);
		dev_reply_hash_table[i].count = 0;
	}
#endif

#if	DEV_REPLY_MESSAGE

	kr = mach_port_allocate(mach_task_self(),
				MACH_PORT_RIGHT_PORT_SET,
				&dev_reply_port_set);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr, ("device_reply_hdlr: mach_port_allocate"));
		panic("device_reply_hdlr: can't allocate port set");
	}

#else	/* ! DEV_REPLY_MESSAGE */

	kr = io_done_queue_create(mach_host_self(), &device_reply_queue);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr, ("device_reply_hdlr: io_done_queue_create"));
		panic("device_reply_hdlr: can't create reply queue");
	}

#endif	/* DEV_REPLY_MESSAGE */

	(void) server_thread_start(device_reply_thread, (void *) 0);
}
