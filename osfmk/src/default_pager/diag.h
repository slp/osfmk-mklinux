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

#ifdef ASSERTIONS
#define assert(cond)	\
	if (!(cond)) panic("%sassertion: %s", my_name, # cond)
#else
#define assert(cond)
#endif

#define Panic(aargh) panic("%s[%d]%s: %s", my_name, dp_thread_id(), here, aargh)

extern char	my_name[];

#define VSTATS_ACTION(l, stmt)	\
	do { VSTATS_LOCK(l); stmt; VSTATS_UNLOCK(l); } while (0)

#if	!defined(VAGUE_STATS) || (VAGUE_STATS > 0)
#define VSTATS_LOCK_DECL(name)
#define VSTATS_LOCK(l)
#define VSTATS_UNLOCK(l)
#define VSTATS_LOCK_INIT(l)
#else
#define VSTATS_LOCK_DECL(name)	struct mutex name;
#define VSTATS_LOCK(l)		mutex_lock(l)
#define VSTATS_UNLOCK(l)	mutex_unlock(l)
#define VSTATS_LOCK_INIT(l)	mutex_init(l)
#endif	/* VAGUE_STATS */

