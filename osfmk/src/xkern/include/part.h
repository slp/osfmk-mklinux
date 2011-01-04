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
 * part.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:24:44 $
 */

#ifndef part_h
#define part_h

#include <xkern/include/xtype.h>

/* 
 * Participant library
 */

/********************  private/opaque declarations ****************/

#define	PART_MAX_STACK	10
/*
 * In some cases, we can process the participants on stack,
 * if their size is not greater than PART_CACHE_SIZE
 */
#define PART_CACHE_SIZE 128

typedef struct {
    struct {
	void	*ptr;
	int	len;
    } arr[PART_MAX_STACK];
    int		top;
} PartStack;

typedef struct part {
    int		len;
    PartStack	stack;	/* A stack of void* pointers */
} Part_s;

void		partStackPush( PartStack *s, void *data, int );
void *		partStackPop( PartStack *s );

/********************  public declarations ****************/

#define LOCAL_PART  1
#define REMOTE_PART 0

/* 
 * Initialize a vector of N participants
 */
void		partInit( Part_s *p, int N );

/* 
 * push 'data' onto the stack of participant 'p'.  
 */
#define partPush( p, data, len ) partStackPush( &(p).stack, data, len );

/* 
 * pop off and return the top element of p's stack.  return NULL if
 * there are no more elements
 */
#define partPop( p ) partStackPop( &(p).stack )

#define partLen( partPtr ) (partPtr->len)

xkern_return_t	partExternalize(Part, void *, int *);

void		partInternalize(Part, void *);

#define partExtLen( _bufPtr )	( *(int *)(_bufPtr) )

int		partStackTopByteLen( Part_s );


#define ANY_HOST	((void *) -1)
#define ANY_PORT	-1

#endif /* part_h */
