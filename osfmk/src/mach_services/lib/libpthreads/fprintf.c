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
 * 
 */
/*
 * MkLinux
 */

#include <stdio.h>
#include <stdarg.h>
#include <mach.h>
#include <mach/sync_policy.h>
#include <mach/sync.h>

#ifndef MACH_CALL
#define MACH_CALL(expr, ret) (ret) = (expr)
#endif

#if 0 /* Uses LOCK SET */

static int _init = 0;
static lock_set_port_t lock;

int
fprintf(FILE *fp, const char *fmt, ...)
{
	kern_return_t kern_res;
	va_list ap;
	if (!_init)
	{
		_init++;
		MACH_CALL(lock_set_create(mach_task_self(), 
					  &lock, 
					  1,
					  SYNC_POLICY_FIFO),
			  kern_res);
	}
	MACH_CALL(lock_acquire(lock, 0), kern_res);
	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	va_end(ap);
	MACH_CALL(lock_release(lock, 0), kern_res);
}

#else /* Uses SEMAPHORE */

#if defined(DEFINE_REENTRANT_PRINTF)
/* should avoid mach calls */

static int _init = 0;
static mach_port_t lock;

#if 0
int
fprintf(FILE *fp, const char *fmt, ...)
{
	kern_return_t kern_res;
	va_list ap;
	if (!_init)
	{
		_init++;
		MACH_CALL(semaphore_create(mach_task_self(), 
					   &lock, 
					   SYNC_POLICY_FIFO,
					   1),
			  kern_res);
	}
	MACH_CALL(semaphore_wait(lock), kern_res);
	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	va_end(ap);
	MACH_CALL(semaphore_signal(lock), kern_res);
}
#endif

int
printf(const char *fmt, ...)
{
	kern_return_t kern_res;
	va_list ap;
	if (!_init)
	{
		_init++;
		MACH_CALL(semaphore_create(mach_task_self(), 
					   &lock, 
					   SYNC_POLICY_FIFO,
					   1),
			  kern_res);
	}
	MACH_CALL(semaphore_wait(lock), kern_res);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	MACH_CALL(semaphore_signal(lock), kern_res);
	if (kern_res == KERN_SUCCESS)
		return 0;
	else
		return -1;
}
#endif /* defined(DEFINE_REENTRANT_PRINTF) */
#endif /* Uses SEMAPHORE */

