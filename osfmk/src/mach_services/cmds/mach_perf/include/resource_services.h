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

#ifndef _RESOURCE_SERVICES_H
#define _RESOURCE_SERVICES_H

extern	int use_opt;		/* User specified is own mem size */
extern  int mem_use;		/* % of free memory we use */
#define MEM_USE_DEFAULT 60
extern  int zone_debug;		/* show zone usage */

struct ipc_name {
	mach_port_t		name;
	mach_port_type_t	type;
};

struct ipc_space {
	int 			total;
	int 			send;
	int 			receive;
	int 			send_once;
	int 			port_set;
	int 			dead;
	int 			other;
	struct ipc_name		*names;
};

struct vm_region {
	vm_offset_t		offset;
	unsigned short		pages;
	unsigned short		prot;
};

struct vm_space {
	int 			total;
	int 			pages;
	int 			rw;
	int 			r;
	int 			inh_share;
	struct vm_region	*regions;
};

struct vm_stat {
	int real;
	int free;
	int active;
	int inactive;
	int wired;
	int zero_fill;
	int pagein;
	int pageout;
	int fault;
	int cow;
	int lookup;
};

extern boolean_t resource_check_on;

extern struct vm_space	vm_space_before;	
extern struct vm_space	vm_space_after;
extern struct ipc_space	ipc_space_before;	
extern struct ipc_space	ipc_space_after;

#define vm_space_diff(before, after)				\
	bcmp((char *)after,					\
	     (char *)before,					\
	     sizeof(struct vm_space)-sizeof(struct vm_region *))

#define ipc_space_diff(before, after)				\
	bcmp((char *)after,					\
	     (char *)before,					\
	     sizeof(struct ipc_space)-sizeof(struct ipc_name *))

extern int resources_start();
extern int resources_stop();
extern int resources_use(char *message);
extern int print_vm_stats();
extern int threads_init();
extern int get_vm_region_info(
                        mach_port_t     task,
                        vm_offset_t     *addr,
                        vm_size_t       *size,
                        vm_prot_t       *prot,
                        vm_inherit_t    *inh,
                        mach_port_t     *name);

#endif /* _RESOURCE_SERVICES_H */
	







