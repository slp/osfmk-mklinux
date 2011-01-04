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
 * select_i.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * select_i.h,v
 * Revision 1.4.1.2.1.2  1994/09/07  04:18:45  menze
 * OSF modifications
 *
 * Revision 1.4.1.2  1994/09/01  04:05:26  menze
 * XObj initialization functions return xkern_return_t
 *
 * Revision 1.4.1.1  1994/07/26  23:57:37  menze
 * Uses Path-based message interface and UPI
 *
 */

#ifndef select_i_h
#define select_i_h


#define SEL_OK 1
#define SEL_FAIL 2

typedef struct  {
    long	id;
    long	status;		/* only used in the RPC realm */
} SelHdr;

typedef struct {
    SelHdr	hdr;
} SState;

typedef struct {
    Map passiveMap;
    Map activeMap;
} PState;


typedef struct {
    long id;
    XObj lls;
} ActiveKey;
    
typedef long PassiveKey;

extern int	traceselectp;


xkern_return_t	selectCommonInit( XObj );
XObj		selectCommonOpen( XObj, XObj, XObj, Part, long, Path );
xkern_return_t	selectCommonOpenDisable( XObj, XObj, XObj, Part, long );
xkern_return_t 	selectCommonOpenEnable( XObj, XObj, XObj, Part, long );
int		selectControlProtl( XObj, int, char *, int );
int		selectCommonControlSessn( XObj, int, char *, int );
xkern_return_t	selectCallDemux( XObj, XObj, Msg, Msg );
xkern_return_t	selectDemux( XObj, XObj, Msg );

#endif	/* ! select_i_h */


