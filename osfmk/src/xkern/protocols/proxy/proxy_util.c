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
 * proxy_util.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.4
 * Date: 1993/07/16 00:45:14
 */

#include <xkern/include/xkernel.h>
#include <xkern/framework/msg_internal.h>
#include <xkern/protocols/proxy/proxy_util.h>
#include <xkern/protocols/proxy/xk_mig_sizes.h>


int	traceproxy = 0;


#define	isContiguous(_m) ( (_m)->state.numNodes == 1 && (_m)->stack == (_m)->tree )

boolean_t
msgIsContiguous(
    Msg m)
{
    return isContiguous(m);
}


boolean_t
msgIsOkToMangle(
    Msg		m,
    char	**machMsg,
    int		offset)
{
#ifdef XK_PROXY_MSG_HACK
    /* 
     * Is the message contiguous
     */
    if ( ! isContiguous(m) ) {
	xTrace0(proxy, TR_DETAILED, "message is non-contiguous, must be copied");
	return FALSE;
    }
    /* 
     * Does this message have the only reference to the stack
     */
    if ( m->stack->refCnt != 1 ) {
	xTrace0(proxy, TR_DETAILED, "msg stack is shared, must be copied");
	return FALSE;
    }
    /* 
     * Is there enough space at the front of the stack to write a mach message header? 
     */
    if ( m->stackHeadPtr <= m->stackTailPtr &&
	 m->stackHeadPtr - m->stack->b.leaf.data >= offset ) {
	*machMsg = m->stackHeadPtr - offset;
    } else {
	xTrace2(proxy, TR_DETAILED, "front of stack is too short (%d < %d)",
		m->stackHeadPtr - m->stack->b.leaf.data, offset);
	xTrace0(proxy, TR_DETAILED, "Msg must be copied");
	return FALSE;
    }
    /* 
     * Is the stack going to be large enough?
     */
    if ( m->stack->b.leaf.data + m->stack->b.leaf.size - *machMsg < XK_MAX_MIG_MSG_LEN ) {
	xTrace1(proxy, TR_DETAILED,
		"Usable stack size (%d) is too small, msg must be copied",
		m->stack->b.leaf.data + m->stack->b.leaf.size - *machMsg);
	return FALSE;
    }
    /* 
     * Is the mach message going to be aligned properly?
     */
    if ( ! LONG_ALIGNED(*machMsg) ) {
	xTrace0(proxy, TR_DETAILED, "Msg has bad alignment, must be copied");
	return FALSE;
    }
    xTrace0(proxy, TR_DETAILED, "Can build mach msg around xkernel msg");
    xTrace2(proxy, TR_DETAILED, "mach msg start: %x, xk msg start: %x",
	    (int)*machMsg, (int)m->stackHeadPtr);
    return TRUE;
#else /* ! XK_PROXY_MSG_HACK */
    return FALSE;
#endif /* XK_PROXY_MSG_HACK */

}

