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
 * upi_inline.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.9.1 $
 * $Date: 1995/02/23 15:27:24 $
 */

#ifndef upi_inline_h
#define upi_inline_h

#include <xkern/include/platform.h>

/*
 * definitions of xPop and xCallPop
 *
 * If XK_USE_INLINE is defined, these are inline functions.  If not,
 * these are the prototypes for regular functions.  One source file
 * should define UPI_INLINE_INSTANTIATE before including this file.
 * This will cause the actual functions to be instantiated in that
 * source file.
 */

#ifdef XK_USE_INLINE
#  define FUNC_TYPE_XKR  static __inline__ xkern_return_t
#  define FUNC_TYPE_XOBJ static __inline__ XObj
#else
#  define FUNC_TYPE_XKR  xkern_return_t
#  define FUNC_TYPE_XOBJ XObj

FUNC_TYPE_XKR	xPop( XObj, XObj, Msg, void * );
FUNC_TYPE_XKR	xCallPop( XObj, XObj, Msg, void *, Msg );
FUNC_TYPE_XOBJ  xGetDown( XObj, int);

#endif /* XK_USE_INLINE */


#if defined(XK_USE_INLINE) || defined(UPI_INLINE_INSTANTIATE)

/*
 * OP_COUNTS controls whether session reference counts are raised and
 * lowered around each operation in order to count the number of
 * operations "outstanding" on that session
 */
#define OP_COUNTS	


#ifdef OP_COUNTS
#define INC_REF_COUNT(sessn, func) 					\
  {									\
    (sessn)->rcnt++;							\
    xTrace4(protocol, TR_MORE_EVENTS,					\
	"%s increased ref count of %x[%s] to %d",			\
	(func), sessn, (sessn)->myprotl->name, (sessn)->rcnt);		\
  }
#else
#define INC_REF_COUNT(s, func) 1
#endif /* OP_COUNTS */
  	

#define DEC_REF_COUNT_UNCOND(sessn, func)				\
  {									\
    if (--(sessn)->rcnt <= 0) {						\
	xTrace4(protocol, TR_MORE_EVENTS,				\
		"%s -- ref count of %x[%s] is %d, calling close",	\
		(func), s, (sessn)->myprotl->name, (sessn)->rcnt);	\
	(*(sessn)->close)(sessn);					\
    } else {								\
	xTrace4(protocol, TR_MORE_EVENTS,				\
		"%s -- decreased ref count of %x[%s] to %d",		\
		(func), s, (sessn)->myprotl->name, (sessn)->rcnt);	\
    }									\
  }


#ifdef OP_COUNTS
#define DEC_REF_COUNT(s, func) DEC_REF_COUNT_UNCOND((s), (func))
#else
#define DEC_REF_COUNT(s, func) 1
#endif   /* OP_COUNTS */


FUNC_TYPE_XKR
xPop(
     XObj 	s,
     XObj	ds,
     Msg	msg,
     void	*hdr)
{
    xkern_return_t retVal;
    
    xAssert(!(int)ds || xIsXObj(ds));
    INC_REF_COUNT(s, "xPop");
    xTrace2(protocol, TR_EVENTS,
	"Calling pop[%s], %d bytes", s->myprotl->name, msgLen(msg));
    retVal = (*s->pop)(s, ds, msg, hdr);
    DEC_REF_COUNT(s, "xPop");
    return retVal;
}


FUNC_TYPE_XKR
xCallPop(
	 XObj 	s,
	 XObj 	ds,
	 Msg 	msg,
	 void	*hdr,
	 Msg 	replyMsg)
{
    xkern_return_t retVal;
    
    xAssert(!(int)ds || xIsXObj(ds));
    INC_REF_COUNT(s, "xCallPop");
    xTrace2(protocol, TR_EVENTS,
	"Calling callpop[%s], %d bytes", s->myprotl->name, msgLen(msg));
    retVal =  (*s->callpop)(s, ds, msg, hdr, replyMsg);
    xTrace2(protocol, TR_EVENTS,
	"callpop[%s] returns %d bytes", s->myprotl->name, msgLen(replyMsg));
    DEC_REF_COUNT(s, "xCallPop");
    return retVal;
}

FUNC_TYPE_XOBJ
xGetDown(
	 XObj	s,
	 int	i)
{
    xAssert( xIsXObj(s) );
    if (i + 1 > s->numdown) {
	return ERR_XOBJ;
    }
    return (i < STD_DOWN) ? s->down[i] : s->downlist[i - STD_DOWN];
}

#endif /* defined(XK_USE_INLINE) || defined(UPI_INLINE_INSTANTIATE) */

#endif /* ! upi_inline_h */
