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

/*
 * This is the Mach standalone startup.  It has been written from
 * scratch to only depend upon interfaces present in a standalone
 * Mach environment.  It contains no OS personality code.
 *
 * General flow structure:
 *
 *	call mach library initialization (if present)
 *	get arguments and environment
 *	call thread library initialization (if present)
 *	call main
 *	call thread library exit (if present) - called with main return value
 *	call exit - called with main return value
 */

#include <mach_init.h>
#include <mach/mach_interface.h>
#include <mach/bootstrap.h>


void (*_mach_init_routine)(void);
int  (*_threadlib_init_routine)(void);
void (*_threadlib_exit_routine)(int);
#if	OLD_COMPAT
int (*_thread_init_routine)(void);
#endif

static char *__nullarg = 0;
static char **__argv = &__nullarg;
static int __argc = 0;
char **__environment = &__nullarg;

extern int main(int, char **);
extern void exit(int);

static void __setup_ptrs(vm_offset_t, vm_size_t, char ***, int *);
static void __get_arguments(void);
static void __get_environment(void);

#if	OLD_COMPAT
#if	defined(i386) || defined (i860)
void __start_mach(int);

void
__start_mach(int kframe)
#endif	/* defined(i386) || defined (i860) */
#if	defined(hp_pa) || defined(ppc)
void __c_start_mach(int, char**);

void __c_start_mach(int kargc, char **kargv)
#endif	/* defined(hp_pa) || defined(ppc) */
#else
void __start_mach(void);

void
__start_mach(void)
#endif
{
	int retval;
#if	OLD_COMPAT
#if	defined(i386) || defined (i860)
	struct kframe {
		int	kargc;
		char	*kargv[1];	/* size depends on kargc */
	};
	struct kframe *kfp = (struct kframe *) (&kframe - 1);

	if (kfp->kargv[0]) {
	    __argc = kfp->kargc;
	    __argv = kfp->kargv;
#endif	/* defined(i386) || defined (i860) */
#if	defined(hp_pa) || defined(ppc)
	if (kargc) {
	    __argc = kargc;
	    __argv = kargv;
#endif	/* defined(hp_pa) || defined(ppc) */

	    if (*_mach_init_routine)
		(*_mach_init_routine)();

	    if (_thread_init_routine) {
		int new_sp;

		new_sp = (*_thread_init_routine)();
		if (new_sp)
#if	defined(i386)
		    __asm__ volatile("movl %0, %%esp" : : "g" (new_sp) );
#endif	/* defined(i386) */
#if	defined(i860)
		__setstack(new_sp);
#endif	/* defined(i386) */
#if     defined(hp_pa)
		{
			__asm__ volatile("ldo 7(%0),%0" : : "g"(new_sp));
			__asm__ volatile("depi 0,31,3,%0" : : "g" (new_sp));
			__asm__ volatile("copy %0,%%r30" : : "g" (new_sp): "r30");
			__asm__ volatile("ldo 48(%%r30),%%r30" : : :"r30");
		}
#endif
#if	defined(ppc)
		{
			/* Macros have been expanded by hand to avoid asm.h
			 * is there any reason not to include
			 * asm.h in this file?
			 * DUPLICATE OF BELOW
			 */
			__asm__ volatile("mr 1,%0" : : "g"(new_sp));
			__asm__ volatile("clrrwi 1, 1, 4");
			__asm__ volatile("subi 1,1,16");
			__asm__ volatile("li 0, 0");
			__asm__ volatile("stw 0,0(1)");
		}
#endif
	    }
	} else {
#endif

	if (*_mach_init_routine)
		(*_mach_init_routine)();

	__get_arguments();
	__get_environment();

	if (*_threadlib_init_routine) {
		int new_sp;

		new_sp = (*_threadlib_init_routine)();

		if (new_sp)
#if	defined(i386)
			__asm__ volatile("movl %0, %%esp" : : "g" (new_sp) );
#elif	defined(i860)
		__setstack(new_sp);
#elif	defined(hp_pa)
	{
		__asm__ volatile("ldo 7(%0),%0" : : "g"(new_sp));
		__asm__ volatile("depi 0,31,3,%0" : : "g" (new_sp));
		__asm__ volatile("copy %0,%%r30" : : "g" (new_sp): "r30");
		__asm__ volatile("ldo 48(%%r30),%%r30" : : :"r30");
	}
#elif	defined(ppc)
	{
		/* Macros have been expanded by hand to avoid asm.h
                 * is there any reason not to include asm.h in this file?
		 * DUPLICATE OF ABOVE
                 */
		__asm__ volatile("mr 1,%0" : : "g"(new_sp));
		__asm__ volatile("clrrwi 1, 1, 4");
		__asm__ volatile("subi 1,1,16");
		__asm__ volatile("li 0, 0");
		__asm__ volatile("stw 0,0(1)");
	}
#else
#error Need code to initialise stack pointer on this architecture.
#endif
	}

#if	OLD_COMPAT
	}
#endif

	retval = main(__argc, __argv);

	if (*_threadlib_exit_routine)
		(*_threadlib_exit_routine)(retval);

	exit(retval);
}

#if	defined(i860)
__setstack_holder(
	int	new_stack)
{
	asm("	.align	4		");
	asm("___setstack::		");
	asm("	bri	r1		");
	asm("	 mov	r16,sp		");

	return;
}
#endif	/* defined(i386) */

static void
__setup_ptrs(
	vm_offset_t strs,
	vm_size_t strs_size,
	char ***ptrs_ptr,
	int *nptrs_ptr)
{
	kern_return_t kr;
	vm_offset_t ptrs;
	int nptrs;
	char *ptr;
	char *endptr;
	int i;

	nptrs = 0;
	ptr = (char *)strs;
	endptr = ptr + strs_size;
	while (ptr < endptr) {
		nptrs++;
		while (*ptr++)
			;
	}
	kr = vm_allocate(mach_task_self(), &ptrs, (nptrs+1) * sizeof(char *),
			 TRUE);
	if (kr != KERN_SUCCESS) {
		vm_deallocate(mach_task_self(), strs, strs_size);
		return;
	}
	ptr = (char *)strs;
	*ptrs_ptr = (char **)ptrs;
	*nptrs_ptr = nptrs;
	for (i = 0; i < nptrs; i++) {
		(*ptrs_ptr)[i] = ptr;
		while (*ptr++)
			;
	}
}

static void
__get_arguments(void)
{
	kern_return_t kr;
	vm_offset_t arguments;
	mach_msg_type_number_t arguments_size;

	kr = bootstrap_arguments(bootstrap_port,
				 mach_task_self(),
				 &arguments,
				 &arguments_size);
	if (kr != KERN_SUCCESS || arguments_size == 0)
		return;
	__setup_ptrs(arguments, arguments_size, &__argv, &__argc);
}

static void
__get_environment(void)
{
	kern_return_t kr;
	vm_offset_t environment;
	mach_msg_type_number_t environment_size;
	int nptrs;

	kr = bootstrap_environment(bootstrap_port,
				   mach_task_self(),
				   &environment,
				   &environment_size);
	if (kr != KERN_SUCCESS || environment_size == 0)
		return;
	__setup_ptrs(environment, environment_size, &__environment, &nptrs);
}
