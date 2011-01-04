/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <mach/port.h>
#include <mach/mach_host.h>
#include <mach/default_pager_object.h>

#include <osfmach3/mach_init.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/assert.h>
#include <osfmach3/block_dev.h>

#include <linux/errno.h>

#include <asm/page.h>

extern mach_port_t	privileged_host_port;

mach_port_t	default_pager_port = MACH_PORT_NULL;

mach_port_t
osfmach3_get_default_pager_port(void)
{
	kern_return_t kr;

	if (default_pager_port == MACH_PORT_NULL) {
		kr = host_default_memory_manager(privileged_host_port,
						 &default_pager_port,
						 0);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr, ("osfmach3_get_default_pager_port: "
					    "host_default_memory_manager"));
		}
	}
	return default_pager_port;
}

int
osfmach3_swapon(
	kdev_t		dev,
	int		size,
	mach_port_t	*swap_backing_storep)
{
	int		ret;
	mach_port_t	device_port;
	mach_port_t	backing_store;
	kern_return_t	kr;

	ret = block_dev_to_mach_dev(dev, &device_port);
	if (ret != 0) {
		printk("osfmach3_swapon: can't get Mach dev port for dev %s\n",
		       kdevname(dev));
		return -ENODEV;
	}

	kr = default_pager_backing_store_create(osfmach3_get_default_pager_port(),
						-1,  /* default priority */
						0,   /* default cluster size */
						&backing_store);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("osfmach3_swapon(dev=0x%x): "
			     "default_pager_backing_store_create",
			     dev));
		return -EINVAL;
	}

	kr = default_pager_add_segment(backing_store,
				       device_port,
				       PAGE_SIZE / OSFMACH3_DEV_BSIZE,/*offset*/
				       size / OSFMACH3_DEV_BSIZE, /* #blocks */
				       OSFMACH3_DEV_BSIZE); /* blockksize */
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("osfmach3_swapon(dev=%s)"
			     "default_pager_add_segment(dev=0x%x,offset=0x%lx,"
			     "count=0x%x,record_size=0x%x)",
			     kdevname(dev),
			     device_port,
			     PAGE_SIZE / OSFMACH3_DEV_BSIZE,
			     size / OSFMACH3_DEV_BSIZE,
			     OSFMACH3_DEV_BSIZE));
		/* default_pager_backing_store_delete(...) */
		return -EINVAL;
	}

	*swap_backing_storep = backing_store;
	return 0;
}

int
osfmach3_swapoff(
	mach_port_t	swap_backing_store)
{
	kern_return_t	kr;

	kr = default_pager_backing_store_delete(swap_backing_store);
	if (kr != KERN_SUCCESS) {
		if (kr == KERN_RESOURCE_SHORTAGE) {
			/* not enough virtual memory */
			return -ENOMEM;
		}
		if (kr == KERN_FAILURE) {
			/* the default pager doesn't support this operation */
			return -ENOSYS;
		}
		if (kr == KERN_INVALID_ARGUMENT) {
			/* invalid backing_store port */
			return -EINVAL;
		}
		MACH3_DEBUG(1, kr,
			    ("osfmach3_swapoff: "
			     "default_pager_backing_store_delete(0x%x)",
			     swap_backing_store));
		return -EINVAL;
	}

	kr = mach_port_destroy(mach_task_self(), swap_backing_store);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("osfmach3_swapoff: mach_port_destroy(0x%x)",
			     swap_backing_store));
	}

	return 0;
}

int
osfmach3_si_swapinfo(
	mach_port_t	backing_store_port,
	unsigned long	*totalswapp,
	unsigned long	*freeswapp)
{
	unsigned int			count;
	struct backing_store_basic_info	info;
	kern_return_t			kr;

	ASSERT(MACH_PORT_VALID(backing_store_port));

	count = BACKING_STORE_BASIC_INFO_COUNT;
	kr = default_pager_backing_store_info(backing_store_port,
					      BACKING_STORE_BASIC_INFO,
					      (backing_store_info_t) &info,
					      &count);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("osfmach3_si_swapinfo: "
			     "default_pager_backing_store_info(port=%x)",
			     backing_store_port));
		return -EINVAL;
	}
	ASSERT(count == BACKING_STORE_BASIC_INFO_COUNT);

	*totalswapp = info.bs_pages_total;
	*freeswapp = info.bs_pages_free;

	return 0;
}
