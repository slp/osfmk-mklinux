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
 * eth_i.h 
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * eth_i.h,v
 * Revision 1.11.1.3.1.2  1994/09/07  04:18:41  menze
 * OSF modifications
 *
 * Revision 1.11.1.3  1994/09/01  18:56:38  menze
 * VCI_ETH_NET_TYPE stored rather than recalculated
 * Use same VCI_ETH_NET_TYPE as OSF
 *
 * Revision 1.11.1.2  1994/04/05  22:19:14  menze
 * Added ETH_HDR_LEN constant
 *
 * Revision 1.11.1.1  1994/03/31  17:15:58  menze
 * Added VCI support
 *
 * Revision 1.11  1994/01/08  21:33:31  menze
 *   [ 1994/01/03          menze ]
 *   Removed declarations for the obsolete eth protocol-driver interface.
 *
 */

/*
 * Information shared between the ethernet protocol and the driver
 */

#ifndef eth_i_h
#define eth_i_h

#include <xkern/include/prot/eth.h>

extern int traceethp;
extern int tracevci;

/*
 * range of legal data sizes
 */

#define MIN_ETH_DATA_SZ		64

/*
 * Ethernet "types"
 *
 * Ether-speak for protocol numbers is "types."
 * Unfortunately, ether "types" are unsigned shorts,
 * while xkernel PROTLs are ints.
 */

typedef unsigned short		ETHtype, ethType_t;

typedef struct {
    ETHhost	dst;
    ETHhost	src;
    ethType_t	type;
} ETHhdr;

#define ETH_HDR_LEN	14

#define XK_VCI TRUE

#define VCI_ETH_TYPE		0x3ccc
extern	ETHtype			vciEthNetType;

typedef xk_u_int16		VciType;

typedef struct {
    VciType     vci;
    ETHtype     ethType;
} VCIhdr;
#define VCI_HDR_LEN		4

#endif  /* ! eth_i_h */
