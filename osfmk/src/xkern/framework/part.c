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
 * part.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:22:51 $
 */

#include <xkern/include/domain.h>
#include <xkern/include/xtype.h>
#include <xkern/include/part.h>
#include <xkern/include/assert.h>
#include <xkern/include/platform.h>
#include <xkern/include/xk_alloc.h>

void
partInit(
	Part_s *p,
	int n)
{
    int i;

    p->len = 0;
    for (i=0; i < n; i++) {
	p[i].stack.top = 0;
	p[i].len = n;
    }
}


void
partStackPush(
	PartStack *s,
	void *data,
	int len)
{
    xAssert(s->top >= 0);
    if (s->top >= PART_MAX_STACK) {
	Kabort("participant stack overflow");
    }
    s->arr[s->top].ptr = data;
    s->arr[s->top].len = len;
    s->top++;
}


void *
partStackPop(
	PartStack *s)
{
    xAssert(s->top >= 0);
    if (s->top == 0 || s->top > PART_MAX_STACK) {
	return 0;
    }
    return s->arr[--s->top].ptr;
}


int
partStackTopByteLen(
    Part_s	p)
{
    PartStack	*s = &p.stack;

    xAssert( s->top >= 0 );
    xAssert( s->top <= PART_MAX_STACK );
    if ( s->top <= 0 || s->top > PART_MAX_STACK ) {
	return -1;
    }
    return  s->arr[s->top - 1].len;
}


/* 
 * Very quick, very dirty ...
 *
 * External participant representation:
 *
 *	number of Participants
 * 		number in part1 stack
 *			stack elem 1 len	stack elements from bottom-to-top
 *			stack elem 1 data
 *			stack elem 2 len	
 *			stack elem 2 data
 *			...
 * 		number in part2 stack
 *			stack elem 1 len	stack elements from bottom-to-top
 *			stack elem 1 data
 *			stack elem 2 len	
 *			stack elem 2 data
 *		...
 */


#define	CHECK_BUF_LEN( _incLen )					\
  	if ((char *)buf + (_incLen) - (char *)bufStart > (*bufLen)) {	\
	    if (mustfree)                                               \
		allocFree(bufStart);					\
	    return XK_FAILURE;						\
	}		

xkern_return_t
partExternalize(
	Part	p,
    	void	*dstBuf,
    	int	*bufLen)	/* length of buffer, number of bytes written */
{
    char	*bufStart, *buf, pcache[PART_CACHE_SIZE];
    int		i, j, len, numParts;
    boolean_t   mustfree = FALSE;

    /*
     * We keep buf and dstBuf separate until the very end (when we
     * copy buf into dstBuf) because some of the pointers in the
     * participant we are externalizing may point into the interior of
     * dstBuf.
     */
    if (*bufLen > PART_CACHE_SIZE) {
	if ( bufStart = xSysAlloc(*bufLen) ) {
	    mustfree = TRUE;
	} else {
	    /* 
	     * Even though we can't allocate a large buffer, we'll try
	     * to proceed with the stack buffer anyway.
	     */
	    bufStart = &pcache[0];
	    *bufLen = PART_CACHE_SIZE;
	}
    } else {
	bufStart = &pcache[0];
    }

    buf = bufStart;
    CHECK_BUF_LEN(sizeof(int));
    *(int *)buf = numParts = partLen(p);
    buf += sizeof(int);
    for (i=0; i < numParts; i++, p++) {
	/* 
	 * For each participant
	 */
	CHECK_BUF_LEN(sizeof(int));
	*(int *)buf = p->stack.top;
	buf += sizeof(int);
	for (j=0; j < p->stack.top; j++) {
	    /* 
	     * For each stack element
	     */
	    CHECK_BUF_LEN(sizeof(int));
	    len = p->stack.arr[j].len;
	    *(int *)buf = len;
	    buf += sizeof(int);
	    if (len) {
		CHECK_BUF_LEN(len);
		bcopy((char *)p->stack.arr[j].ptr, buf, len);
		buf += len;
	    } else {
		/* 
		 * len == 0 indicates a special-valued pointer which
		 * must be preserved. 
		 */
		CHECK_BUF_LEN(sizeof(void *));
		*(void **)buf = p->stack.arr[j].ptr;
		buf += sizeof(void *);
	    }
	    while (!LONG_ALIGNED(buf)) {
		buf++;
	    }
	}
    }
    *bufLen = buf - bufStart;
    bcopy(bufStart, dstBuf, *bufLen);
    if (mustfree) 
	allocFree(bufStart);
    return XK_SUCCESS;
}

void
partInternalize(
	Part	p,
    	void	*dummyBuf)
{
    int		i, j, len, numInStack;
    char	*buf = (char *) dummyBuf;

    partInit(p, *(int *)buf);
    buf += sizeof(int);
    for (i=0; i < partLen(p); i++) {
	/* 
	 * For each participant
	 */
	numInStack = *(int *)buf;
	buf += sizeof(int);
	for (j=0; j < numInStack; j++) {
	    /* 
	     * For each stack element
	     */
	    len = *(int *)buf;
	    buf += sizeof(int);
	    if (len) {
		partPush(p[i], buf, len);
		buf += len;
	    } else {
		partPush(p[i], *(void **)buf, 0);
		buf += sizeof(void *);
	    }
	    while (!LONG_ALIGNED(buf))
	        buf++;
	}
    }
}
