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

#include <stdlib.h>

int (*_mach_init_routine)(void);
int (*_thread_init_routine)(void);
void (*_thread_exit_routine)(void *);

extern int main(int, char **, char **);
void _start(unsigned, char **, char **);

void
_start(register unsigned argc,
       register char **argv,
       register char **envp)
{
	if (_mach_init_routine)
		(*_mach_init_routine)();

	if (_thread_init_routine) {
		int	new_sp;
		new_sp = (*_thread_init_routine)();
		if (new_sp) {
			__asm__ volatile("ldo 7(%0),%0" : : "g"(new_sp));
			__asm__ volatile("depi 0,31,3,%0" : : "g" (new_sp));
			__asm__ volatile("copy %0,%%r30" : : "g" (new_sp): "r30");
			__asm__ volatile("ldo 48(%%r30),%%r30" : : :"r30");
		}
	}

	argc = main(argc, argv, envp);

	if (_thread_exit_routine)
		(*_thread_exit_routine)((void *)argc);

	exit(argc);
}


