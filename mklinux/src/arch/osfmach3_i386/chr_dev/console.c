/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 *
 */
/*
 * MkLinux
 */

#include <mach/mach_interface.h>

#include <osfmach3/mach_init.h>
#include <osfmach3/device_utils.h>
#include <osfmach3/console.h>
#include <osfmach3/parent_server.h>
#include <osfmach3/mach3_debug.h>

#include <linux/kernel.h>

extern void osfmach3_physmem_init(void);
extern mach_port_t osfmach3_physmem_memory_object;

boolean_t
osfmach3_con_probe(void)
{
	kern_return_t kr;

	if (parent_server || osfmach3_use_mach_console) {
		/* no virtual consoles */
		return FALSE;
	}

	/*
	 * Try mapping the physical memory for the VGA.
	 */
	osfmach3_physmem_init();
	if (osfmach3_physmem_memory_object != MACH_PORT_NULL) {
		osfmach3_video_physaddr = 0xa0000;	/* XXX */
		osfmach3_video_map_size = 0x100000 - 0xa0000;	/* XXX */
		osfmach3_video_map_base = (vm_address_t) 0;
		kr = vm_map(mach_task_self(),
			    &osfmach3_video_map_base,	/* address */
			    osfmach3_video_map_size,	/* XXX size */
			    0,				/* mask */
			    TRUE,			/* anywhere */
			    osfmach3_physmem_memory_object, /* mem object */
			    0xa0000,			/* offset */
			    FALSE,			/* copy */
			    VM_PROT_READ | VM_PROT_WRITE, /* cur_prot */
			    VM_PROT_READ | VM_PROT_WRITE, /* max_prot */
			    VM_INHERIT_NONE);		/* inheritance */
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(0, kr,
				    ("osfmach3_con_probe: "
				     "vm_map(obj=0x%x, size=0x%x)",
				     osfmach3_physmem_memory_object,
				     osfmach3_video_map_size));
			printk("osfmach3_con_probe: "
			       "can't map display from physmem.\n");
		} else {
			osfmach3_video_port = 1;	/* XXX */
			return TRUE;
		}
	}

	/*
	 * Map the VGA memory.
	 */
	kr = device_open(device_server_port,
			 MACH_PORT_NULL,
			 D_READ | D_WRITE,
			 server_security_token,
			 "vga0",
			 &osfmach3_video_port);
	if (kr != D_SUCCESS) {
		MACH3_DEBUG(2, kr,
			    ("osfmach3_con_probe: device_open(\"vga0\")"));
		printk("osfmach3_con_probe: can't map display. "
		       "Using dumb console.\n");
		return FALSE;
	}

	osfmach3_video_map_size = 0x100000 - 0xa0000;	/* XXX */
	kr = device_map(osfmach3_video_port,
			VM_PROT_READ | VM_PROT_WRITE,
			0,
			osfmach3_video_map_size,
			&osfmach3_video_memory_object,
			0);
	if (kr != D_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("osfmach3_con_probe: "
			     "device_map(port=0x%x, size=0x%x)",
			     osfmach3_video_port, osfmach3_video_map_size));
		panic("osfmach3_con_probe: can't device_map display\n");
	}

	osfmach3_video_map_base = (vm_address_t) 0;
	kr = vm_map(mach_task_self(),
		    &osfmach3_video_map_base,		/* address */
		    osfmach3_video_map_size,		/* XXX size */
		    0,					/* mask */
		    TRUE,				/* anywhere */
		    osfmach3_video_memory_object,	/* memory object */
		    0,					/* offset */
		    FALSE,				/* copy */
		    VM_PROT_READ | VM_PROT_WRITE,	/* cur_protection */
		    VM_PROT_READ | VM_PROT_WRITE,	/* max_protection */
		    VM_INHERIT_NONE);			/* inheritance */
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("osfmach3_con_probe: vm_map(obj=0x%x, size=0x%x)",
			     osfmach3_video_memory_object,
			     osfmach3_video_map_size));
		panic("osfmach3_con_probe: can't map display.\n");
	}

	return TRUE;
}
