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

#define	_simple_lock_xchg_(lock, new_val) \
    ({	register int _old_val_; \
	asm volatile("xchgl %0, %2" \
		    : "=r" (_old_val_) \
		    : "0" (new_val), "m" (*(lock)) \
		    ); \
	_old_val_; \
    })

#define	_simple_lock_init(l) \
	((l)->lock_data = 0)

#define	_simple_lock(l) \
    ({ \
	while(_simple_lock_xchg_(l, 1)) \
	    while (*(volatile int *)&(l)->lock_data) \
		continue; \
	0; \
    })

#define	_simple_unlock(l) \
	(_simple_lock_xchg_(l, 0))

#define	_simple_lock_try(l) \
	(!_simple_lock_xchg_(l, 1))

simple_lock(l) 
struct slock *l;
{
	while (!_simple_lock_try(l))
		MACH_CALL(thread_switch, (mach_thread_self(),
					  SWITCH_OPTION_NONE,
					  0))
}

simple_unlock(l) 
struct slock *l;
{
	_simple_unlock(l);
}
