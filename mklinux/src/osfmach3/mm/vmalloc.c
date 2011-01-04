/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <mach/mach_interface.h>

#include <osfmach3/mach3_debug.h>
#include <osfmach3/mach_init.h>

#include <linux/kernel.h>
#include <linux/malloc.h>
#include <linux/mm.h>

#include <asm/page.h>

struct vm_struct {
	unsigned long flags;
	void * addr;
	unsigned long size;
	struct vm_struct * next;
};

static struct vm_struct * vmlist = NULL;

void vfree(void * addr)
{
	struct vm_struct **p, *tmp;
	kern_return_t kr;

	if (!addr)
		return;
	if ((PAGE_SIZE-1) & (unsigned long) addr) {
		printk("Trying to vfree() bad address (%p)\n", addr);
		return;
	}
	for (p = &vmlist; (tmp = *p); p = &tmp->next) {
		if (tmp->addr == addr) {
			*p = tmp->next;
			/* free the memory */
			kr = vm_deallocate(mach_task_self(),
					   (vm_offset_t) tmp->addr,
					   (vm_size_t) tmp->size);
			if (kr != KERN_SUCCESS) {
				MACH3_DEBUG(1, kr,
					    ("vfree: "
					     "vm_deallocate(0x%x,0x%x)",
					     (vm_offset_t) tmp->addr,
					     (vm_size_t) tmp->size));
			}
			kfree(tmp);
			return;
		}
	}
	printk("Trying to vfree() nonexistent vm area (%p)\n", addr);
}

void * vmalloc(unsigned long size)
{
	kern_return_t	kr;
	vm_offset_t	addr;
	struct vm_struct *area, **p, *tmp, *prev;

	size = PAGE_ALIGN(size);
	if (!size || size > (MAP_NR(high_memory) << PAGE_SHIFT))
		return NULL;

	/* allocate a new entry */
	area = kmalloc(sizeof (*area), GFP_KERNEL);
	if (!area)
		return NULL;

	/* allocate some memory to go with it */
	kr = vm_allocate(mach_task_self(), &addr, size, TRUE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("vmalloc: vm_allocate(0x%lx)", size));
		kfree((void *) area);
		return NULL;
	}
	area->addr = (void *) addr;
	area->size = size;
	area->next = NULL;

	/* insert "area" in the vmlist */
	prev = NULL;
	for (p = &vmlist; (tmp = *p); p = &tmp->next, prev = tmp) {
		if ((unsigned long) tmp->addr >=
		    (unsigned long) area->addr + area->size) {
			break;
		}
	}
	if (prev != NULL) {
		area->next = prev->next;
		prev->next = area;
	} else {
		area->next = vmlist;
		vmlist = area;
	}

	return area->addr;
}
