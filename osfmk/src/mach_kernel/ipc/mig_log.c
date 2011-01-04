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

#ifdef MACH_KERNEL
#include <mig_debug.h>
#endif

#include <mach/message.h>
#include <mach/mig_log.h>

int mig_tracing, mig_errors, mig_full_tracing;

/*
 * Tracing facilities for MIG generated stubs.
 *
 * At the moment, there is only a printf, which is
 * activated through the runtime switch:
 * 	mig_tracing to call MigEventTracer
 * 	mig_errors to call MigEventErrors
 * For this to work, MIG has to run with the -L option, 
 * and the mig_debug flags has to be selected
 *
 * In the future, it will be possible to collect infos
 * on the use of MACH IPC with an application similar
 * to netstat.
 * 
 * A new option will be generated accordingly to the
 * kernel configuration rules, e.g
 *	#include <mig_log.h>
 */ 

void
MigEventTracer(
	mig_who_t		who,
	mig_which_event_t	what,
	mach_msg_id_t		msgh_id,
	unsigned int		size,
	unsigned int		kpd,
	unsigned int		retcode,
	unsigned int		ports,
	unsigned int		oolports,
	unsigned int		ool,
	char			*file,
	unsigned int		line)
{
    printf("%d|%d|%d", who, what, msgh_id); 
    if (mig_full_tracing)
	printf(" -- sz%d|kpd%d|ret(0x%x)|p%d|o%d|op%d|%s, %d", 
	    size, kpd, retcode, ports, oolports, ool, file, line); 
    printf("\n");
}

void
MigEventErrors(
	mig_who_t		who,
	mig_which_error_t	what,
	void			*par,
	char			*file,
	unsigned int		line)
{
    if (what == MACH_MSG_ERROR_UNKNOWN_ID)
	printf("%d|%d|%d -- %s %d\n", who, what, *(int *)par, file, line); 
    else
	printf("%d|%d|%s -- %s %d\n", who, what, (char *)par, file, line); 
}
