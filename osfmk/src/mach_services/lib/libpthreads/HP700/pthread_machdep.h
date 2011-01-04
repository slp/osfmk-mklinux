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

/* Machine-dependent definitions for pthread internals. */

/* PA-RISC interlocked instructions require an integer operand aligned on a
   16-byte boundary.  The only compiler-portable way to arrange for this to
   work is to reserve four words and do the lock on whichever one of those
   words is correctly aligned.  Gcc's __attribute__((aligned(16))) would
   allow us to avoid wasting this extra space and time in the usual case
   where we can have a correctly aligned variable, but then the internals
   of structures containing locks would have to be visible to pthread users
   since they can have variables of those structure types.  */

typedef struct {
    int word[4];
} pthread_lock_t;

#define LOCK_INIT(l)	\
	((l).word[0] = (l).word[1] = (l).word[2] = (l).word[3] = -1)
#define LOCK_INITIALIZER {{-1, -1, -1, -1}}

#define STACK_GROWS_UP 1

/* Prototypes for functions specific to this architecture.  */
extern int _dp(void);
