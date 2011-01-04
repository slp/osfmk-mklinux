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
 * tcp_hdr.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.9.1 $
 * $Date: 1995/02/23 15:38:13 $
 */

/*
 * TCP header store and load functions
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/tcp/tcp_internal.h>

#define CKSUM_TRACE 8

static void	dispPseudoHdr( IPpseudoHdr * );
static void	dispHdr( struct tcphdr * );
static boolean_t	dumpBytes( char *, long, void * );

static void
dispPseudoHdr(h)
    IPpseudoHdr *h;
{
  printf("   IP pseudo header: src: %s  dst: %s  \n   z:  %d  p: %d len: %d\n",
	 ipHostStr(&h->src), ipHostStr(&h->dst), h->zero, h->prot,
	 ntohs(h->len));
}


static void
dispHdr(h)
    struct tcphdr *h;
{
    printf("sp: %d dp: %d ", h->th_sport, h->th_dport);
    printf("seq: %x ack: %x ", h->th_seq, h->th_ack);
    printf("off: %d w: %d ", h->th_off, h->th_win);
    printf("urp: %d fg: %s\n", h->th_urp, tcpFlagStr(h->th_flags));
}


static boolean_t
dumpBytes(buf, len, foo)
    char *buf;
    long int len;
    void *foo;
{
    int i;

    for (i=0; i < len; i++) {
	printf("%x ",*(u_char *)buf++);
	if (i+1 % 50 == 0) {
	    putchar('\n');
	}
    }
    putchar('\n');
    return TRUE;
}



/*
 * tcpHdrStore -- 'arg' should point to an appropriate hdrStore_t 
 * note: '*hdr' and the length field of the pseudoheader in '*arg'
 * will be modified.
 */
void
tcpHdrStore(hdr, dst, len, arg)
    void *hdr;
    char *dst;
    long int len;
    void *arg;
{
    u_short 	sum;
    IPpseudoHdr	*pHdr;

    xAssert(len == sizeof(struct tcphdr));
    xIfTrace(tcpp, 6) {
	printf("Outgoing header\n");
	dispHdr((struct tcphdr *)hdr);
    }
    ((struct tcphdr *)hdr)->th_sport = htons(((struct tcphdr *)hdr)->th_sport);
    ((struct tcphdr *)hdr)->th_dport = htons(((struct tcphdr *)hdr)->th_dport);
    xTrace1(tcpp, 4, "Storing hdr with seq # %d",
	    ((struct tcphdr *)hdr)->th_seq);
    ((struct tcphdr *)hdr)->th_seq   = htonl(((struct tcphdr *)hdr)->th_seq);
    ((struct tcphdr *)hdr)->th_ack   = htonl(((struct tcphdr *)hdr)->th_ack);
    ((struct tcphdr *)hdr)->th_win   = htons(((struct tcphdr *)hdr)->th_win);
    ((struct tcphdr *)hdr)->th_urp   = htons(((struct tcphdr *)hdr)->th_urp);
    ((struct tcphdr *)hdr)->th_sum   = 0;
    bcopy(hdr, dst, len);
    /*
     * Checksum
     */
#define msg (((hdrStore_t *)arg)->m)
    pHdr = ((hdrStore_t *)arg)->h;
    pHdr->len = htons(msgLen(msg));
    xIfTrace(tcpp, CKSUM_TRACE) {
	printf("Sent: ");
	dispPseudoHdr(pHdr);
    }
    sum = inCkSum(msg, (u_short *)pHdr, sizeof(IPpseudoHdr));

    xTrace1(tcpp, CKSUM_TRACE, "Cksum(x): %x", sum);

    bcopy((char *)&sum, (char *)&((struct tcphdr *)dst)->th_sum,
	  sizeof(u_short));

    xIfTrace(tcpp, CKSUM_TRACE) {
	msgForEach(msg, dumpBytes, 0);
    }
    xAssert(inCkSum(msg, (u_short *)pHdr, sizeof(IPpseudoHdr)) == 0);
#undef msg
}


/*
 * tcpHdrLoad -- 'arg' should be a pointer to the message structure
 * The IP pseudoHdr should be attached as an attribute of the message.
 */
long
tcpHdrLoad(hdr, src, len, arg)
    void *hdr;
    char *src;
    long int len;
    void *arg;
{
    u_short	*pHdr;

    xAssert(len == sizeof(struct tcphdr));
    pHdr = (u_short *)msgGetAttr((Msg)arg, 0);
    xAssert(pHdr);
    bcopy(src, hdr, len);
    xIfTrace(tcpp, CKSUM_TRACE) {
	printf("Received: ");
	dispPseudoHdr((IPpseudoHdr *)pHdr);
    }
    xTrace1(tcpp, CKSUM_TRACE, "Incoming cksum: %x",
	    ((struct tcphdr *)hdr)->th_sum);

    ((struct tcphdr *)hdr)->th_sum = inCkSum((Msg)arg, pHdr,
					      sizeof(IPpseudoHdr));

    xIfTrace(tcpp, CKSUM_TRACE) {
	msgForEach((Msg)arg, dumpBytes, 0);
    }
#if 0
    {
	int i;
	u_char *c = src;

	for (i=0; i < len; i++) {
	    printf("%x ", *c++);
	}
	printf("\n");
    }
#endif
    ((struct tcphdr *)hdr)->th_sport = ntohs(((struct tcphdr *)hdr)->th_sport);
    ((struct tcphdr *)hdr)->th_dport = ntohs(((struct tcphdr *)hdr)->th_dport);
    ((struct tcphdr *)hdr)->th_seq   = ntohl(((struct tcphdr *)hdr)->th_seq);
    xTrace1(tcpp, 4, "Loading hdr with seq # %d",
	    ((struct tcphdr *)hdr)->th_seq);
    ((struct tcphdr *)hdr)->th_ack   = ntohl(((struct tcphdr *)hdr)->th_ack);
    ((struct tcphdr *)hdr)->th_win   = ntohs(((struct tcphdr *)hdr)->th_win);
    ((struct tcphdr *)hdr)->th_urp   = ntohs(((struct tcphdr *)hdr)->th_urp);
#if 0
    {
	int i;
	u_char *c = hdr;

	for (i=0; i < len; i++) {
	    printf("%x ", *c++);
	}
	printf("\n");
    }
#endif
    return sizeof(struct tcphdr);
}


/*
 * tcpOptionsStore -- pads the options with zero bytes to a 4-byte boundary
 * 'arg' should point to an integer which will contain the actual number of
 * bytes in 'hdr'
 */
void
tcpOptionsStore(hdr, dst, len, arg)
    void *hdr;
    char *dst;
    long int len;
    void *arg;
{
    int hdrLen;	
    
    hdrLen = *(int *)arg;
    bcopy(hdr, dst, hdrLen);
    if (hdrLen % 4) {
	dst += hdrLen;
	do {
	    *dst++ = 0;
	    hdrLen++;
	} while (hdrLen % 4);
    }
}


long
tcpOptionsLoad(hdr, src, len, arg)
    void *hdr;
    char *src;
    long int len;
    void *arg;
{
    bcopy(src, hdr, len);
    return len;
}


