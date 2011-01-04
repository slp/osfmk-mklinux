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

struct slock {
	int		lock_data;	/* in general 1 bit is sufficient */
};

#define	_simple_lock_xchg_(lock, old_val, new_val) \
	__asm__("xchgl %0, %2" \
		    : "=r" (old_val) \
		    : "0" (new_val), "m" (*(lock)) \
		    )

#define	simple_lock_init(l) \
	((l)->lock_data = 0)

#define	simple_lock(l) \
    do { \
	register int _old_val_; \
	while(1) { \
		_simple_lock_xchg_(l, _old_val_, 1); \
		if (!_old_val_) \
			break; \
	    while (*(volatile int *)&(l)->lock_data) \
			continue; \
    } \
	} \
	while (0)

#define	simple_unlock(l) \
    do { \
		register int _old_val_; \
		_simple_lock_xchg_(l, _old_val_, 0); \
	} \
	while (0)

#define	simple_lock_try(l, old_val) \
		(_simple_lock_xchg_(l, old_val, 1), !old_val)

lock_loop()
{
	register unsigned i = loops;
	struct slock l;

	simple_lock_init(&l);
	start_time();
	while(i--) {
		simple_lock(&l);
		simple_unlock(&l);
	}
	stop_time();
}

locks_loop(nlocks)
{
	register unsigned i = loops;
	struct slock l;

	simple_lock_init(&l);
	start_time();
	while(i--) {
		simple_lock(&l);
		simple_unlock(&l);
	}
	stop_time();
}


