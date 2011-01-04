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
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile: crt0.c,v $ $Revision: 1.3.10.2 $ (OSF) $Date: 1995/01/26 22:12:43 $";
#endif

/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <machine/asm.h>

#define MACH3

char **environ;
char **_auxv;

int errno;

void (*_malloc_init_routine)(void);
void (*_mach_init_routine)(void);
void (*_pthread_init_routine)(void);
void (*_pthread_exit_routine)(int);
void (*_xti_tli_init_routine)(void);
#ifdef	MACH3
int  (*_cthread_init_routine)();
void (*_cthread_exit_routine)();
#endif	/* MACH3 */

int _ldr_crt0_request = 1;
int _ldr_present;
void (*_ldr_jump_func)();

extern char etext;

static int __inline__ Entry_sp(void)
{
	int sp;
	__asm__ volatile ("leal 4(%%ebp), %0" : "=r" (sp));
	return (sp);
}

__start()
{
	struct kframe {
		int	kargc;
		char	*kargv[1];	/* size depends on kargc */
		char	kargstr[1];	/* size varies */
		char	kenvstr[1];	/* size varies */
		char	k_auxv[1];	/* size varies */
	};
	/*
	 *	ALL REGISTER VARIABLES!!!
	 */
	register int r11;		/* needed for init */
	register struct kframe *kfp;	/* r10 */
	register char **targv;
	register char **argv;
	register int argc;
	register char **argcp;

	kfp = (struct kframe *)Entry_sp();

	argcp = (char **)kfp;		

	for (argv = targv = &kfp->kargv[0]; *targv++; /* void */)
		/* void */ ;
	environ = targv;

        /* _auxv starts after environ */
        for (targv = environ; *targv++; /* void */)
                /* void */ ;
        _auxv = targv;

#ifdef GCRT0
	_moninit = _gprof_moninit;
	_monstop = _gprof_monstop;
	_moncontrol = _gprof_moncontrol;
	_monstartup = _gprof_monstartup;
#endif

#ifdef MCRT0
	_moninit = _prof_moninit;
	_monstop = _prof_monstop;
	_moncontrol = _prof_moncontrol;
	_monstartup = _prof_monstartup;
#endif

#if defined(MCRT0) || defined(GCRT0)
        (*_monstartup)((char *)__start, (char *)&etext);
#endif

	if (_mach_init_routine)
		(*_mach_init_routine)();

	if (_malloc_init_routine)		/* for malloc leak tracing */
		(*_malloc_init_routine)();

#ifdef	MACH3
	if (_cthread_init_routine) {
		int	new_sp;
		new_sp = (*_cthread_init_routine)();
		if (new_sp) {
			__asm__ volatile("movl %0, %%esp" : : "g" (new_sp) );
		}
	}
#endif	/* MACH3 */

	if (_pthread_init_routine)
		(*_pthread_init_routine)();

	if (_xti_tli_init_routine)
		(*_xti_tli_init_routine)();

	errno = 0;
	if (_ldr_present) {
		void (*entry_pt)();

		entry_pt = (void ((*)()))main(kfp->kargc, argv, environ, _auxv);
		(*_ldr_jump_func)(entry_pt, argcp);
		/*NOTREACHED*/
	} else
		argc = (int)main(kfp->kargc, argv, environ, _auxv);

#ifdef	MACH3
	if (_cthread_exit_routine)
		(*_cthread_exit_routine)(argc);
#endif	/* MACH3 */

	if (_pthread_exit_routine)
		(*_pthread_exit_routine)(argc);

	exit(argc);

}
