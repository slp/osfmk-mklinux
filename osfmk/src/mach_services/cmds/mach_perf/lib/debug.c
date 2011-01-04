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

#define MAX_DEBUGS 10
#define MAX_TRACES 10


struct debug_info debug_infos[MAX_DEBUGS];
struct trace_info trace_infos[MAX_DEBUGS];

int 	ndebugs = 0;		/* how many debug options are specified  ? */
int	 ntraces = 0;		/* how many trace options are specified  ? */
int 	debug_all = 0;		/* debug all modules */
int 	trace_all = 0;		/* trace all modules */
int	debug_resources = 0;	/* show resource consumptions */


int
get_debug_level(char *file)
{
	register i;
	char *s, c;
	int l;

	for (i=0; i < ndebugs; i++) {
	  	c = *debug_infos[i].string;
		l = strlen(debug_infos[i].string);
		for (s = strchr(file, c); s; s = strchr(s, c))
			if (strncmp(s, debug_infos[i].string, l) == 0)
				return(debug_infos[i].level);
			else if (!*++s)
				break;
	}
	return(debug_all);
}

int
is_traced(char *file)
{
	register i;
	char *s, c;
	int l;

	if (trace_all)
		return(1);
	for (i=0; i < ntraces; i++) {
	  	c = *trace_infos[i].string;
		l = strlen(trace_infos[i].string);
		for (s = strchr(file, c); s; s = strchr(s, c))
			if (strncmp(s, trace_infos[i].string, l) == 0)
				return(1);
			else if (!*++s)
				break;
	}
	return(0);
}

void
add_debug_string(char *string, int level)
{
	if (ndebugs > MAX_DEBUGS) {
		printf("Too many debug strings, dropping %s\n", string);
		return;
	}
	debug_infos[ndebugs].string = string;
	debug_infos[ndebugs].level = level;
	ndebugs++;
}
 
void
add_trace_string(char *string)
{
	if (ntraces > MAX_TRACES) {
		printf("Too many trace strings, dropping %s\n", string);
		return;
	}
	trace_infos[ntraces].string = string;
	ntraces++;
}
 
char *cur_traced_file;
      
mach_call_print(char *func, char *file, int line, kern_return_t ret)
{
	if (ntraces && is_traced(file)) {
		if (cur_traced_file  != file) {
			printf("\nIn %s\n", file);
			cur_traced_file = file;
		}
		printf("At line %d %s()", line, func);
		if (debug_resources && resource_check_on)
	  		check_resources();
		printf("\n");
	}

	if (ret != KERN_SUCCESS) {
		verbose++;
		mach_call_failed(func, file, line, ret);
	} 
}

mach_func_print(char *func, char *file, int line, int ret)
{
	if (ntraces && is_traced(file)) {
		if (cur_traced_file  != file) {
			printf("\nIn %s\n", file);
			cur_traced_file = file;
		}
		printf("At line %d %s()", line, func);
		if (debug_resources && resource_check_on)
	  		check_resources();
		printf("\n");
	}

	if (!ret) {
		verbose++;
		mach_func_failed(func, file, line);
	} 
}

struct vm_space 	debug_vm_space_before;
struct vm_space 	debug_vm_space_after;
struct ipc_space 	debug_ipc_space_before;
struct ipc_space 	debug_ipc_space_after;

check_resources()
{
	ipc_resources(mach_task_self(), &debug_ipc_space_after);
	vm_resources(mach_task_self(), &debug_vm_space_after);
	vm_space_use(&debug_vm_space_before, &debug_vm_space_after);
	ipc_space_use(&debug_ipc_space_before, &debug_ipc_space_after);
	vm_space_xchg(&debug_vm_space_before, &debug_vm_space_after);
	ipc_space_xchg(&debug_ipc_space_before, &debug_ipc_space_after);
}

#define INIT_DEBUG_IPC_NAMES(ipc_space)					\
 	if (!ipc_space.names)						\
		MACH_CALL(vm_allocate, (mach_task_self(),		\
					(vm_offset_t *)&ipc_space.names,\
					vm_page_size,			\
					TRUE));

#define INIT_DEBUG_VM_REGIONS(vm_space)					\
 	if (!vm_space.regions)						\
		MACH_CALL(vm_allocate, (mach_task_self(),		\
					(vm_offset_t *)&vm_space.regions,\
					vm_page_size,			\
					TRUE));

debug_resources_reset()
{
	if (!debug_resources) 
		return;
	INIT_DEBUG_IPC_NAMES(ipc_space_before);
	INIT_DEBUG_IPC_NAMES(ipc_space_after);
	INIT_DEBUG_VM_REGIONS(vm_space_before);
	INIT_DEBUG_VM_REGIONS(vm_space_after);
	INIT_DEBUG_IPC_NAMES(debug_ipc_space_before);
	INIT_DEBUG_IPC_NAMES(debug_ipc_space_after);
	INIT_DEBUG_VM_REGIONS(debug_vm_space_before);
	INIT_DEBUG_VM_REGIONS(debug_vm_space_after);
	ipc_resources(mach_task_self(), &debug_ipc_space_before);
	vm_resources(mach_task_self(), &debug_vm_space_before);
}






