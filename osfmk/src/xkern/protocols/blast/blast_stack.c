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
/* 
 * blast_stack.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * blast_stack.c,v
 * Revision 1.3.1.2.1.2  1994/09/07  04:18:26  menze
 * OSF modifications
 *
 * Revision 1.3.1.2  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.3  1994/02/05  00:02:54  menze
 *   [ 1994/01/28          menze ]
 *   assert.h renamed to xk_assert.h
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/blast/blast_stack.h>

int	tracestack;

Stack
stackCreate( int size, Path path )
{
    Stack	s;

    xAssert(size >= 0);
    if ( ! (s = pathAlloc(path, sizeof(struct stack))) ) {
	return 0;
    }
    if ( ! (s->a = pathAlloc(path, size * sizeof(void *))) ) {
	pathFree(s);
	return 0;
    }
    s->max = size;
    s->top = 0;
    return s;
}


void *
stackPop( s )
    Stack	s;
{
    xAssert(s);
    xAssert(s->top >= 0);
    xAssert(s->max >= 0);
    xTrace1(stack, TR_DETAILED, "stack pop -- size == %d", s->top);
    if ( s->top ) {
	xTrace1(stack, TR_DETAILED, "stack pop returning %x", s->a[s->top-1]);
	return s->a[--s->top];
    } else {
	xTrace0(stack, TR_DETAILED, "stack pop -- stack is empty");
	return 0;
    }
}


int
stackPush( s, v )
    Stack	s;
    void	*v;
{
    xAssert(s);
    xAssert(s->top >= 0);
    xAssert(s->max >= 0);
    xTrace1(stack, TR_DETAILED, "stack push -- size == %d", s->top);
    if ( s->top >= s->max ) {
	xTrace0(stack, TR_DETAILED, "stack push -- stack is full");
	return -1;
    }
    xTrace1(stack, TR_DETAILED, "stack push storing %x", v);
    s->a[s->top++] = v;
    return 0;
}


void
stackDestroy( s )
    Stack	s;
{
    xAssert(s);
    xAssert(s->a);
    pathFree((char *)s->a);
    pathFree((char *)s);
}
