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
 * icmp_internal.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.11.1 $
 * $Date: 1995/02/23 15:32:06 $
 */

#include <xkern/include/prot/ip.h>
#include <xkern/protocols/ip/ip_i.h>

typedef struct icmp_hdr {
	u_char	icmp_type;	
	u_char	icmp_code;	
	short	icmp_cksum;
} ICMPHeader;

typedef struct common_prefix {
	u_short   icmp_id;
	u_short   icmp_seqnum;
	/* ignoring data part */
} ICMPPrefix, ICMPEcho, ICMPInfoReq, ICMPInfoReply;

typedef struct offending_dg {
	IPheader	icmp_dest;
/*
 * Yes, options might be there, but we don't use them even if they are
 * and their presence in this structure causes msgPop to wretch if they
 * aren't there.
 */
#if 0
	char		icmp_bits[48]; /* includes 40 octets for options */
#endif
} ICMPBadMsg;


typedef struct common_format {
  u_long	icmp_zero;
  ICMPBadMsg	icmp_badmsg;
} ICMPUnreach, ICMPSourceQuench, ICMPTimeout;


typedef struct redirect {
  IPhost	icmp_gw;
  ICMPBadMsg	icmp_badmsg;
} ICMPRedirect;

typedef struct bad_param {
  u_char	icmp_pointer;
  u_char	icmp_zero[3];
  ICMPBadMsg	icmp_badmsg;
} ICMPSyntaxError;

typedef struct timestamps {
  ICMPPrefix	icmp_prefix;
  u_long	icmp_originate;
  u_long	icmp_receive;
  u_long	icmp_transmit;
} ICMPTimeStamp;


typedef struct {
  u_short 	sessId;
  Bind 		bind;
  int		result;
  xkSemaphore	replySem;
  Event		timeoutEvent;
  u_short	seqNum;
} Sstate;

typedef struct {
  Map 		waitMap;
  u_short 	sessionsCreated;
} Pstate;

typedef struct {
  Msg		msg;
  u_short 	sum;
} loadInfo;

typedef struct {
  u_short 	id;
  u_short	seq;
} mapId;

int 	icmpSendEchoReq(XObj s, int msgLength);
void 	icmpPopEchoRep(XObj p, Msg m);
void 	icmpHdrStore(void *hdr, char *netHdr, long len, void *);
void 	icmpEchoStore(void *hdr, char *netHdr, long len, void *);
long 	icmpEchoLoad(void *hdr, char *netHdr, long len, void *);

extern int traceicmpp;

#define REQ_TIMEOUT 15000	/* 15 seconds */

#define ICMP_STACK_LEN	200
