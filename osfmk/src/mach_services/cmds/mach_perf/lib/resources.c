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

#include <mach_perf.h>
#include <mach_debug/mach_debug.h>


int 	mem_use = MEM_USE_DEFAULT;	/* % of free memory used */
int  	use_opt = 0;

#define 	max_names  (vm_page_size / sizeof(struct ipc_name))
#define		max_regions (vm_page_size / sizeof(struct vm_region))

extern	void	flush_reaper_thread(void);

ipc_resources(task, ipc_space)
mach_port_t task;
struct ipc_space *ipc_space;
{
	mach_port_array_t names;
	mach_port_type_array_t types;
	mach_msg_type_number_t count;
	register i;
	mach_port_type_t port_type;

	MACH_CALL_NO_TRACE(mach_port_names, (task,
				    &names,
				    &count,
				    &types,
				    &count));

	ipc_space->send = 0;
	ipc_space->receive = 0;
	ipc_space->send_once = 0;
	ipc_space->port_set = 0;
	ipc_space->dead = 0;
	ipc_space->other = 0;
	ipc_space->total = count;
	for (i=0 ; i<count; i++) {
		port_type = *(types + i);
		if (port_type & MACH_PORT_TYPE_SEND)
			ipc_space->send++;
		if (port_type & MACH_PORT_TYPE_RECEIVE)
			ipc_space->receive++;
		if (port_type & MACH_PORT_TYPE_SEND_ONCE)
			ipc_space->send_once++;
		if (port_type & MACH_PORT_TYPE_PORT_SET)
			ipc_space->port_set++;
		if (port_type & MACH_PORT_TYPE_DEAD_NAME)
			ipc_space->dead++;
		if (!(port_type & (MACH_PORT_TYPE_SEND|
			      MACH_PORT_TYPE_RECEIVE|
			      MACH_PORT_TYPE_SEND_ONCE|
			      MACH_PORT_TYPE_PORT_SET|
			      MACH_PORT_TYPE_DEAD_NAME)))
			ipc_space->other++;
		if (ipc_space->names) {
			struct ipc_name *name;

			if (i >= max_names)
				test_error("ipc_resources",
					   "too many port names");
			name = ipc_space->names+i;
			name->name = *(names+i);
			name->type = port_type;
		}
		if (debug)
			printf("port name %x, type %x\n", *(names+i), port_type);
	}

	MACH_CALL_NO_TRACE(vm_deallocate, (mach_task_self(),
				  (vm_offset_t) names,
				  count * sizeof(*names)));

	MACH_CALL_NO_TRACE(vm_deallocate, (mach_task_self(),
				  (vm_offset_t) types,
				  count * sizeof(*types)));

	if (debug) {
	  printf("ipc space:\n");
	  printf("Names	Send	Receive	Sonce	Pset	dead	other\n");
	  printf("%d	%d	%d	%d	%d	%d	%d\n",
		 ipc_space->total,
		 ipc_space->send,
		 ipc_space->receive,
		 ipc_space->send_once,
		 ipc_space->port_set,
		 ipc_space->dead,
		 ipc_space->other);
	}
}

struct ipc_space ipc_space_before;	
struct ipc_space ipc_space_after;

struct ipc_type {
	mach_port_type_t type;
	char *sym;
} ipc_types[] = {
	MACH_PORT_TYPE_SEND, "S",
	MACH_PORT_TYPE_RECEIVE, "R",
	MACH_PORT_TYPE_SEND_ONCE, "SO",
	MACH_PORT_TYPE_PORT_SET, "SET",
	MACH_PORT_TYPE_DEAD_NAME, "D",
	0, "?"
};

char *get_ipc_type_sym(mach_port_type_t type)
{
	struct ipc_type *it = ipc_types;
	while (it->type && it->type != type)
		it++;
	return(it->sym);
}

ipc_space_use(before, after)
struct ipc_space *before, *after;
{
	if (ipc_space_diff(before, after)) {
		if (debug_resources && before->names && after->names) {
			struct ipc_name	*br = before->names;
			struct ipc_name	*ar = after->names;
			unsigned int	bc = before->total;
			unsigned int	ac = after->total;

			while (bc || ac) {
				if (ac == 0 || (bc && br->name < ar->name)) {
					printf(" -%x:%s",
					       br->name,
					       get_ipc_type_sym(br->type));
					if (bc) {
						bc--; br++;
					}
				} else if (bc == 0 || (ac && br->name > ar->name)) {
					printf(" +%x:%s",
					       ar->name,
					       get_ipc_type_sym(ar->type));
					if (ac) {
						ac--; ar++;
					}
				} else if (br->type != ar->type) {
					printf(" -%x:%s",
					       br->name,
					       get_ipc_type_sym(br->type));
					printf(" +%x:%s",
					       ar->name,
					       get_ipc_type_sym(ar->type));
					ac--; ar++;
					bc--; br++;
				} else {
					ac--; ar++;
					bc--; br++;
				}
			}
		} else {
		  	printf("consumed:\n");
			printf("Names	Send	Receive	Sonce	Pset	dead	other\n");
			printf("%d	%d	%d	%d	%d	%d	%d\n",
			       after->total - before->total,
			       after->send - before->send,
			       after->receive - before->receive,
			       after->send_once - before->send_once,
			       after->port_set - before->port_set,
			       after->dead - before->dead,
			       after->other - before->other);
		}
	  	return(1);
	}
	return(0);
}

ipc_space_xchg(struct ipc_space *is1, struct ipc_space *is2)
{
	struct ipc_space is;
	is = *is1;
	*is1 = *is2;
	*is2 = is;
}

int	thread_cnt_before;
int	thread_cnt_after;
int	task_cnt_before;
int	task_cnt_after;

vm_resources(task, vm_space)
mach_port_t task;
struct vm_space *vm_space;
{
        task_port_t     thistask=mach_task_self();
	vm_offset_t	addr;
	vm_size_t	size;
        vm_prot_t       prot;
        vm_inherit_t    inh;
	mach_port_t	name;
	int		page_count;

	vm_space->total = 0;
	vm_space->pages = 0;
	vm_space->rw = 0;
	vm_space->r = 0;
	vm_space->inh_share = 0;

	for (addr = VM_MIN_ADDRESS;
	     addr < VM_MAX_ADDRESS;
	     addr += size) {
		if (get_vm_region_info (thistask,
				      &addr,
				      &size,
			              &prot,
			              &inh,
				      &name) != KERN_SUCCESS)
			break;
		vm_space->total++;
		page_count = size/vm_page_size;
		vm_space->pages += page_count;
		
		if (prot & VM_PROT_WRITE)
			vm_space->rw++;
		else if (prot & VM_PROT_READ)
			vm_space->r++;
		if (inh & VM_INHERIT_SHARE)
			vm_space->inh_share++;
		if (vm_space->regions) {
			struct vm_region *region;

			if (vm_space->total >= max_regions)
				test_error("vm_resources",
					   "too many vm regions");
			region = vm_space->regions+vm_space->total-1;
			region->offset = addr;
			region->pages = page_count;
			region->prot = prot;
		}
		if (debug)
			printf("address %x, %d pages , prot %x\n",
			       addr, page_count, prot);
		mach_port_deallocate (mach_task_self(), name);
	}

	if (debug) {
	  printf("vm space:\n");
	  printf("vm_regs	Pages	R/W	RO	Inh_share\n");
	  printf("%d	%d	%d	%d	%d\n",
		 vm_space->total,
		 vm_space->pages,
		 vm_space->rw,
		 vm_space->r,
		 vm_space->inh_share);
	}
}

kern_return_t
get_vm_region_info(task, addr, size, prot, inh, name)
mach_port_t     task;
vm_offset_t     *addr;
vm_size_t       *size;
vm_prot_t       *prot;
vm_inherit_t    *inh;
mach_port_t     *name;
{
        kern_return_t   kern_ret;
        vm_region_basic_info_data_t vm_info;
        unsigned int    count = sizeof(vm_info);

        kern_ret = vm_region(task, addr, size, VM_REGION_BASIC_INFO,
                        (vm_region_info_t) &vm_info, &count, name);
        *prot = vm_info.protection;
        *inh = vm_info.inheritance;
        return(kern_ret) ;
}

struct vm_space vm_space_before;	
struct vm_space vm_space_after;
boolean_t	resource_check_on = FALSE;

vm_space_use(before, after)
struct vm_space *before, *after;
{
	if (vm_space_diff(before, after)) {
		if (debug_resources && before->regions && after->regions) {
			struct vm_region	*br = before->regions;
			struct vm_region 	*ar = after->regions;
			unsigned int	bc = before->total;
			unsigned int	ac = after->total;

			while (bc || ac) {
				if (ac == 0 || (bc && br->offset < ar->offset)) {
					printf(" -%x:%x",
					       br->offset,
					       br->offset+br->pages*vm_page_size);
					if (bc) {
						bc--; br++;
					}
				} else if (bc == 0 || (ac && br->offset > ar->offset)) {
					printf(" +%x:%x",
					       ar->offset,
					       ar->offset+ar->pages*vm_page_size);
					if (ac) {
						ac--; ar++;
					}
				} else if (br->pages < ar->pages) {
					printf(" +%x:%x",
					       br->offset+br->pages*vm_page_size,
					       ar->offset+ar->pages*vm_page_size);
					ac--; ar++;
					bc--; br++;
				} else if (br->pages > ar->pages) {
					printf(" -%x:%x",
					       ar->offset+ar->pages*vm_page_size,
					       br->offset+br->pages*vm_page_size);
					ac--; ar++;
					bc--; br++;
				} else {
					ac--; ar++;
					bc--; br++;
				}
			}
		} else {
			printf("consumed: \n");
			printf("vm_regs	Pages	R/W	RO	Inh_share\n");
			printf("%d	%d	%d	%d	%d\n",
			       after->total - before->total,
			       after->pages - before->pages,
			       after->rw - before->rw,
			       after->r - before->r,
			       after->inh_share - before->inh_share);
		}
		return(1);
	}
	return(0);
}

vm_space_xchg(struct vm_space *vs1, struct vm_space *vs2)
{
	struct vm_space vs;
	vs = *vs1;
	*vs1 = *vs2;
	*vs2 = vs;
}

int zone_debug = 0;

struct zone_use {
	zone_name_t 	name_buf[1024];
	zone_info_t 	info_buf[1024];
	int		cnt;
	unsigned 	int totalsize;
	unsigned 	int totalused;
};       

typedef struct zone_use *zone_use_t;
	
extern void	get_zones(zone_use_t zones);
extern void	zone_use(zone_use_t before, zone_use_t after, char *text);

struct zone_use zone_before;
struct zone_use zone_after;

boolean_t	zone_info_disabled = FALSE;

resources_start()
{
	debug_resources_reset();
	ipc_resources(mach_task_self(), &ipc_space_before);
	vm_resources(mach_task_self(), &vm_space_before);
	task_cnt_before = get_task_cnt();
	thread_cnt_before = get_thread_cnt();
	if (zone_debug || is_master_task)
		get_zones(&zone_before);
	resource_check_on = TRUE;
}
 
resources_stop()
{
	flush_reaper_thread();

	task_cnt_after = get_task_cnt();
	thread_cnt_after = get_thread_cnt();
	
	if (task_cnt_after > task_cnt_before ||
	    thread_cnt_after > thread_cnt_before) {
		int task_cnt = task_cnt_after;
		int thread_cnt = thread_cnt_after;
		do {
	 		task_cnt = task_cnt_after;
	 		thread_cnt = thread_cnt_after;
			mach_sleep(1000);	/* 1000 microseconds */
		} while ((task_cnt_after = get_task_cnt()) < task_cnt ||
			 (thread_cnt_after = get_thread_cnt()) < thread_cnt);
	}
	resource_check_on = FALSE;
	if (zone_debug || is_master_task)
		get_zones(&zone_after);
	ipc_resources(mach_task_self(), &ipc_space_after);
	vm_resources(mach_task_self(), &vm_space_after);
}

resources_use(message)
char *message;
{
	if (ipc_space_diff(&ipc_space_before, &ipc_space_after)) {
		printf("%s", message);
		ipc_space_use(&ipc_space_before, &ipc_space_after);
	}
	if (vm_space_diff(&vm_space_before, &vm_space_after)) {
		printf("%s", message);
		vm_space_use(&vm_space_before, &vm_space_after);
	}
	if (zone_debug)
		zone_use(&zone_before, &zone_after, message);
	if (standalone && is_master_task) {
		if (task_cnt_after != task_cnt_before)
			printf("%s %+d task(s)\n",
			       message,
			       task_cnt_after - task_cnt_before);
		if (thread_cnt_after != thread_cnt_before)
		  	printf("%s %+d thread(s)\n",
			       message, 
			       thread_cnt_after - thread_cnt_before);
	}
}

#define vm_assign(vm_stat, vms)					\
		(vms)->free = (vm_stat)->free_count;		\
		(vms)->active = (vm_stat)->active_count;	\
		(vms)->inactive = (vm_stat)->inactive_count;	\
		(vms)->wired = (vm_stat)->wire_count;		\
		(vms)->zero_fill = (vm_stat)->zero_fill_count;	\
		(vms)->pagein = (vm_stat)->pageins;		\
		(vms)->pageout = (vm_stat)->pageouts;		\
		(vms)->fault = (vm_stat)->faults;		\
		(vms)->cow = (vm_stat)->cow_faults;		\
		(vms)->lookup = (vm_stat)->lookups;

struct vm_stat vm_stat_before, vm_stat_after;

get_vm_stats(vms)
struct vm_stat *vms;
{
	vm_statistics_data_t vm_stats;
	struct host_basic_info basic_info;
	int count;

	bzero((char *)vms, sizeof(*vms));

	count = sizeof(basic_info);
	MACH_CALL(host_info, (mach_host_self(),
			      HOST_BASIC_INFO,
			      &basic_info,
			      &count));
	vms->real = basic_info.memory_size/vm_page_size;

	count = sizeof(vm_stats);
	MACH_CALL( host_statistics, (privileged_host_port,
				     HOST_VM_INFO,
				     &vm_stats,
				     &count));
	vm_assign(&vm_stats, vms);      
}

/*
012345678012345678012345678012345678012345678012345678012345678012345678012345678
vm real   free    inact      act    wired   faults   cows lookups     in/out
12345671234567 +1234567  1234567  1234567  12345671234567123456781234567/1234567
       1234567          +1234567 +1234567
1234567
*/

print_vm_stats()
{
	bcopy((char *)&vm_stat_after,
	      (char *)&vm_stat_before,
	      sizeof(struct vm_stat));
	get_vm_stats(&vm_stat_after);
	if (!vm_opt)
		return;
	printf("vm real   free    inact      act    wired   faults   cows lookups     in/out\n");
	printf("%7d%7d +%7d  %7d  %7d  %7d%7d%8d%7d/%d\n",
	       vm_stat_after.real,
	       vm_stat_after.free, vm_stat_after.inactive,
	       vm_stat_after.active, vm_stat_after.wired,
	       vm_stat_after.fault - vm_stat_before.fault,
	       vm_stat_after.cow - vm_stat_before.cow,
	       vm_stat_after.lookup - vm_stat_before.lookup,
	       vm_stat_after.pagein - vm_stat_before.pagein,
	       vm_stat_after.pageout - vm_stat_before.pageout);
	printf("       %7d          +%7d +%7d\n",
	       vm_stat_after.free + vm_stat_after.inactive,
	       vm_stat_after.active, vm_stat_after.wired);
	printf("%7d\n",
	       vm_stat_after.free + vm_stat_after.inactive +
	       vm_stat_after.active + vm_stat_after.wired);

}

int mem_size()
{
	struct vm_stat vm_stats;
	int free;

	get_vm_stats(&vm_stats);
	free = vm_stats.free + vm_stats.inactive;
	free = (free * mem_use) / 100;
	if (!free) {
		free = vm_stats.real *  mem_use/100;
		printf("No free memory, use %d %% of total memory = %d pages\n\n",
		       mem_use, free);
	} else if (free > vm_stats.free && standalone && !use_opt) {
		printf("%d %% of free+inactive memory (%d) > free memory (%d)\n",
		       mem_use, free, vm_stats.free);
		free = (vm_stats.free * 90)/100;
		printf("Instead use 90 %% of free memory = %d pages\n\n", free);
	} else {
		printf("Use %d %% of free+inactive memory = %d pages\n\n",
		       mem_use, free);
	}
	
	return(free);
}


void
get_zones(
	  zone_use_t zones)
{
	zone_name_t 	*name;
	unsigned int 	nameCnt;
	zone_info_t 	*info;
	unsigned int 	infoCnt;
	int 		i;
	kern_return_t	ret; 

	zones->totalused = 0;
	zones->totalsize = 0;
	infoCnt = 0;

	if (zone_info_disabled)
		return;

	ret = host_zone_info (mach_host_self(),
			      &name,
			      &nameCnt,
			      &info,
			      &infoCnt);
	
	if (ret == MIG_BAD_ID) {
		zone_info_disabled = TRUE;
		return;
	}

	if (ret != KERN_SUCCESS)
		MACH_CALL( host_zone_info, (mach_host_self(),
					    &name,
					    &nameCnt,
					    &info,
					    &infoCnt));

	bcopy(name, zones->name_buf, sizeof (zone_name_t) * nameCnt);
	bcopy(info, zones->info_buf, sizeof (zone_info_t) * infoCnt);
	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t) name,
				  sizeof (zone_name_t) * nameCnt));
	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t) info,
				  sizeof (zone_info_t) * infoCnt));
	zones->cnt = nameCnt;
	for (i=0; i<zones->cnt; i++) {
		zones->totalused += zones->info_buf[i].zi_elem_size *
		  			zones->info_buf[i].zi_count;
		zones->totalsize += zones->info_buf[i].zi_cur_size;
	}
}

void 
zone_diff(
	  char *name,
	  zone_info_t *before,
	  zone_info_t *after,
	  char **text)
{
	int diff = 0;
	if (!before)
		diff = after->zi_count;
	else
		diff = after->zi_count - before->zi_count;
	if (!diff)
		return;
	if (*text)
		printf("%s\n", *text);
	*text = 0;
	printf("	%s: %+d\n", name, diff);
}

void
zone_use(
	 zone_use_t before,
	 zone_use_t after,
	 char *text)
{
	 register i, j;

	 for (i=0; i < after->cnt; i++) {
		for (j=0; j < before->cnt; j++) {
			if (strcmp((char *)&after->name_buf[i],
				   (char *)&before->name_buf[j]) == 0) {
			  	zone_diff((char *)&after->name_buf[i],
					  &before->info_buf[i],
					  &after->info_buf[i],
					  &text);
				break;
			}
		}
		if (j >= before->cnt) 
		  	zone_diff((char *)&after->name_buf[i],
				  0,
				  &after->info_buf[i],
				  &text);
	}
	printf("\nTotal size   = %+d\n",
	       after->totalsize - before->totalsize);
	printf("Total used   = %+d\n",
	       after->totalused - before->totalused);
	printf("Total wasted = %+d\n",
	       (after->totalsize - after->totalused) -
	       (before->totalsize - before->totalused));
}
			

static struct zone_use zones_zones;

extern void	printzone(zone_name_t *, zone_info_t *);

void
zones(
      int argc,
      char *argv[])
{
  	register i; 

	if (argc == 1) {
		zone_use(&zone_before, &zone_after, "");
		return;
	} else if (argc > 2 ||
		   *argv[1] == '?' ||
		   strcmp(argv[1], "-a") != 0) {
		printf("zones     reports zone usage since last command\n");
		printf("zones -a  reports complete zoned statistics\n");
		return;
	}

	get_zones(&zones_zones);

	if (zone_info_disabled) {
		printf("zone info services disabled from kernel configuration\n");
		return;
	}

	printzoneheader();
	for (i = 0; i < zones_zones.cnt; i++)
	  	printzone(&zones_zones.name_buf[i],
			  &zones_zones.info_buf[i]);

	printf("\n");
	printf("Total size   = %8d\n", zones_zones.totalsize);
	printf("Total used   = %8d\n", zones_zones.totalused);
	printf("Total wasted = %8d\n",
	       zones_zones.totalsize - zones_zones.totalused);
}

void
printzone(
	  zone_name_t *zone_name,
	  zone_info_t *info)
{
	char *name = zone_name->zn_name;
	int j, namewidth, retval;
	unsigned int used, size;
	namewidth = 18;

	name[namewidth]='\0';
	printf("%s", name);
	spaces(namewidth-strlen(name));
	
	printf("%5d", info->zi_elem_size);
	printf("%6dK", info->zi_cur_size/1024);
	if (info->zi_max_size >= 99999 * 1024) {
		printf("   ----");
	} else {
		printf("%6dK", info->zi_max_size/1024);
	}
	printf("%7d", info->zi_cur_size / info->zi_elem_size);
	if (info->zi_max_size >= 99999 * 1024) {
		printf("   ----");
	} else {
		printf("%7d", info->zi_max_size / info->zi_elem_size);
	}
	printf("%6d", info->zi_count);
	printf("%5dK", info->zi_alloc_size/1024);
	printf("%6d", info->zi_alloc_size / info->zi_elem_size);

	used = info->zi_elem_size * info->zi_count;
	size = info->zi_cur_size;
	printf("%7d", size - used);

	printf("%c%c\n",
	       (info->zi_pageable ? 'P' : ' '),
	       (info->zi_collectable ? 'C' : ' '));
}

printzoneheader()
{
	printf("                   elem    cur    max    cur    max%s",
		       "   cur alloc alloc\n");
	printf("zone name          size   size   size  #elts  #elts%s",
		       " inuse  size count wasted\n");
	printf("-------------------------------------------------------------------------------\n");
}

void
flush_reaper_thread(
	void)
{
	mach_port_t thread;
	mach_port_t notification_port;
	mach_port_t prev_port;

	MACH_CALL( mach_port_allocate, (mach_task_self(),
					MACH_PORT_RIGHT_RECEIVE,
					&notification_port));
	MACH_CALL( thread_create, (mach_task_self(), &thread));
	MACH_CALL( mach_port_request_notification,
		  (mach_task_self(),
		   thread,
		   MACH_NOTIFY_DEAD_NAME,
		   0,
		   notification_port,
		   MACH_MSG_TYPE_MAKE_SEND_ONCE,
		   &prev_port));
	MACH_CALL( thread_terminate, (thread));
	WAIT_REPLY( notify_server, notification_port, 128);
	MACH_CALL( mach_port_destroy, (mach_task_self(),
				       thread));
	MACH_CALL( mach_port_destroy, (mach_task_self(),
				       notification_port));
}


