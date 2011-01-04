/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */
/*
 * Miscellaneous device-related functions.
 */

#include <device/device_request.h>

#include <osfmach3/assert.h>
#include <osfmach3/mach_init.h>
#include <osfmach3/device_reply_hdlr.h>
#include <osfmach3/device_utils.h>
#include <osfmach3/queue.h>

#include <linux/fs.h>
#include <linux/malloc.h>
#include <linux/mm.h>

/*
 * Device to object hashing.
 */

#define DEV_TO_OBJECT_HASH_SIZE	8
#define DEV_TO_OBJECT_HASH(dev, type)	\
	((MAJOR((dev)) ^ MINOR((dev)) ^ (type)) & (DEV_TO_OBJECT_HASH_SIZE - 1))
struct dev_to_object_hash_elt {
	queue_head_t	queue;
	int		count;
#if	HASH_STATS
	unsigned int	counts_count;
	unsigned int	cumulated_counts;
#endif
} dev_to_object_hash_table[DEV_TO_OBJECT_HASH_SIZE];

struct dev_to_object_hash_entry {
	queue_chain_t	chain;
	kdev_t		dev;
	int		type;
	char		*object;
};

void
dev_to_object_init(void)
{
	register int i;

	for (i = 0; i < DEV_TO_OBJECT_HASH_SIZE; i++) {
		queue_init(&dev_to_object_hash_table[i].queue);
		dev_to_object_hash_table[i].count = 0;
#if	HASH_STATS
		dev_to_object_table[i].counts_count = 0;
		dev_to_object_table[i].cumulated_counts = 0;
#endif
	}
}

void
dev_to_object_register(
	kdev_t		dev,
	int		type,
	char		*object)
{
	struct dev_to_object_hash_elt *elt;
	struct dev_to_object_hash_entry *entry;

	ASSERT(dev_to_object_lookup(dev, type) == NULL);

	entry = (struct dev_to_object_hash_entry *)
		kmalloc(sizeof (struct dev_to_object_hash_entry), GFP_KERNEL);
	entry->dev = dev;
	entry->type = type;
	entry->object = object;  

	elt = &dev_to_object_hash_table[DEV_TO_OBJECT_HASH(dev, type)];
	enqueue_tail(&elt->queue, (queue_entry_t) entry);
	elt->count++;
}

char *
dev_to_object_lookup(
	kdev_t		dev,
	int		type)
{
	struct dev_to_object_hash_elt *elt;
	struct dev_to_object_hash_entry *entry;
	char *object;

	object = NULL;
	elt = &dev_to_object_hash_table[DEV_TO_OBJECT_HASH(dev, type)];

#if	HASH_STATS
	if (elt->cumulated_counts + elt->count < old_cumulated_counts) {
		if (elt->count != 0) {
			printk("dev_to_object_lookup: hash[%d] stats overflow. "
			       "avg = %d. resetting.\n", 
			       elt - &dev_to_object_hash_table[0], 
			       elt->cumulated_counts / elt->count);
		}
		elt->counts_count = 0;
		elt->cumulated_counts = 0;
	} else {
		elt->counts_count++;
		elt->cumulated_counts += elt->count;
	}
#endif

	for (entry = ((struct dev_to_object_hash_entry *)
		      queue_first(&elt->queue));
	     !queue_end(&elt->queue, (queue_entry_t) entry);
	     entry = ((struct dev_to_object_hash_entry *)
		      queue_next(&entry->chain))) {
		if (entry->dev == dev && entry->type == type) {
			object = entry->object;
			break;
		}
	}

	return object;
}

void
dev_to_object_deregister(
	kdev_t		dev,
	int		type)
{
	struct dev_to_object_hash_elt *elt;
	struct dev_to_object_hash_entry *entry;

	elt = &dev_to_object_hash_table[DEV_TO_OBJECT_HASH(dev, type)];
	for (entry = ((struct dev_to_object_hash_entry *)
		      queue_first(&elt->queue));
	     !queue_end(&elt->queue, (queue_entry_t) entry);
	     entry = ((struct dev_to_object_hash_entry *)
		      queue_next(&entry->chain))) {
		if (entry->dev == dev && entry->type == type) {
			queue_remove(&elt->queue,
				     entry,
				     struct dev_to_object_hash_entry *,
				     chain);
			elt->count--;
			kfree(entry);
			break;
		}
	}
	ASSERT(entry != NULL && entry->dev == dev && entry->type == type);
}

/*
 * The data and count parameters are reversed as compared with
 * device_read_overwrite.  The latter would surely be consistent with
 * device_write if it had not been added much later.  We retain
 * consistency between serv_device_{read,write}_async at the risk of
 * possible confusion when changing from direct Mach interfaces to
 * these functions.
 */
kern_return_t
serv_device_read_async(
	mach_port_t		device,
	mach_port_t		reply_port,
	dev_mode_t		mode,
	recnum_t		recnum,
	caddr_t			data,
	mach_msg_type_number_t	count,
	boolean_t		inband)
{
	if (data) {
		ASSERT(!inband);
#if	DEV_REPLY_MESSAGE
		return device_read_overwrite_request
		       (device, reply_port, mode, recnum, count, data);
#else	/* !DEV_REPLY_MESSAGE */
		return device_read_overwrite_async
		       (device, device_reply_queue, reply_port, mode, recnum,
			count, (vm_address_t) data);
#endif	/* !DEV_REPLY_MESSAGE */
	}
#if	DEV_REPLY_MESSAGE
	return (inband ? device_read_request_inband : device_read_request)
	       (device, reply_port, mode, recnum, count);
#else	/* !DEV_REPLY_MESSAGE */
	return (inband ? device_read_async_inband : device_read_async)
	       (device, device_reply_queue, reply_port, mode, recnum, count);
#endif	/* !DEV_REPLY_MESSAGE */
}

kern_return_t
serv_device_write_async(
	mach_port_t		device,
	mach_port_t		reply_port,
	dev_mode_t		mode,
	recnum_t		recnum,
	caddr_t			data,
	mach_msg_type_number_t	count,
	boolean_t		inband)
{
#if	DEV_REPLY_MESSAGE
	return (inband ? device_write_request_inband : device_write_request)
	       (device, reply_port, mode, recnum, data, count);
#else	/* !DEV_REPLY_MESSAGE */
	return (inband ? device_write_async_inband : device_write_async)
	       (device,
		(reply_port == MACH_PORT_NULL ?
		 MACH_PORT_NULL : device_reply_queue),
	        reply_port, mode, recnum, data, count);
#endif	/* !DEV_REPLY_MESSAGE */
}
