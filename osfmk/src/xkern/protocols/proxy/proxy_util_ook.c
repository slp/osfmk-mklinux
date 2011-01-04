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
 * proxy_util_ook.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.1
 * Date: 1993/07/16 00:28:13
 */

#include <mach.h>
#include <xkern/include/xkernel.h>
#include <xkern/framework/msg_internal.h>
#include <xkern/protocols/proxy/proxy_util.h>
#include <xkern/protocols/proxy/xk_mig_sizes.h>


extern int	traceproxy;




/* 
 * Deallocate the virtual memory region containing 'p'.  
 */
void
oolFree(
    void	*p,
    int		len,
    void        *unused)
{
    kern_return_t	kr;

    xTrace2(proxy, TR_EVENTS, "proxy oobFree deallocates %d bytes starting at %x",
	    len, p);
    kr = vm_deallocate(mach_task_self(), (vm_address_t)p, len);
    if ( kr != KERN_SUCCESS ) {
	sprintf(errBuf,
		"vm_deallocate fails in  x-kernel proxy oobFree, error: %s",
		mach_error_string(kr));
	xError(errBuf);
	return;
    }
}


static void
call_allocFree(
    void	*p,
    int		len)
{
    xTrace2(proxy, TR_FUNCTIONAL_TRACE, "calling xFree with region %x, length %d",
	    (u_int)p, len);
    allocFree(p);
}


static void
call_msgDestroy(
    void	*p,
    int		len)
{
    xTrace2(proxy, TR_FUNCTIONAL_TRACE, "calling msgDestroy with msg %x (length %d)",
	    (u_int)p, len);
    msgDestroy((Msg)p);
    allocFree((void *)p);
}


#define msgTop(_m) ((_m)->stackHeadPtr)


/* 
 * Convert the data in the given message to an out-of-line region
 * suitable for passing directly as a MIG out-of-line parameter.
 */
xkern_return_t
msgToOol(
    Msg		m,
    char	**oolPtr,
    DeallocFunc	*func,
    void	**arg)
{
    /* 
     * In the case where we need to copy, it's unclear how best to
     * allocate the transmission buffer.  Either vm_allocate or
     * xMalloc will work.  vm_allocate is probably more expensive, but
     * is less likely to result in another copy due to a copy-on-write
     * fault. 
     */

    /* 
     * Avoid copying the data if it's contiguous
     */
    if ( msgIsContiguous(m) ) {
	Msg_s	*mCopy;

	xTrace2(proxy, TR_FUNCTIONAL_TRACE,
		"msgToOol: message is contiguous, avoiding copy",
		msgLen(m), (u_int)*oolPtr );
	if ( ! (mCopy = pathAlloc(msgGetPath(m), sizeof(Msg_s))) ) {
	    return XK_NO_MEMORY;
	}
	msgConstructCopy(mCopy, m);
	*arg = (void *)mCopy;
	*func = call_msgDestroy;
	*oolPtr = msgTop(m);
    } else {
	*oolPtr = allocGet(pathGetAlloc(msgGetPath(m), MSG_ALLOC), msgLen(m));
	if ( ! *oolPtr ) {
	    return XK_NO_MEMORY;
	}
	xTrace2(proxy, TR_FUNCTIONAL_TRACE,
		"msgToOol allocating %d bytes at %x",
		msgLen(m), (u_int)*oolPtr );
	msg2buf(m, *oolPtr);
	*func = call_allocFree;
	*arg = *oolPtr;
    }
    return XK_SUCCESS;
}


/* 
 * Construct an x-kernel message from an incoming region of
 * out-of-line memory.  
 */
xkern_return_t
oolToMsg(
    char	*oolPtr,
    Path   	path,
    u_int 	len,
    Msg		msg)
{
    xTrace1(proxy, TR_DETAILED, "xCall VM region starts at %x", (u_int)oolPtr);
    return msgConstructInPlace(msg, path, len, 0, oolPtr, oolFree, 0);
}


void
lingeringMsgSave(
    DeallocFunc		f,
    void		*arg,
    int			len)
{
    ProxyThreadInfo	*info;

    info = (ProxyThreadInfo *)cthread_data(cthread_self());
    xAssert(info);
    if ( info->lm.valid ) {
	lingeringMsgClear();
    }
    if ( f ) {
	info->lm.valid = TRUE;
	info->lm.func = f;
	info->lm.arg = arg;
	info->lm.len = len;
    }
}


void
lingeringMsgClear()
{
    ProxyThreadInfo	*info;
    LingeringMsg	*lMsg;

    info = (ProxyThreadInfo *)cthread_data(cthread_self());
    xAssert(info);
    lMsg = &info->lm;
    if ( lMsg->valid ) {
	xTrace0(proxy, TR_DETAILED, "clearing lingering message");
	if ( lMsg->func ) {
	    lMsg->func(lMsg->arg, lMsg->len);
	}
	lMsg->valid = FALSE;
    } else {
	xTrace0(proxy, TR_DETAILED, "no lingering message to clear");
    }
}


