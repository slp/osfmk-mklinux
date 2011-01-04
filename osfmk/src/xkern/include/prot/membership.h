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

#ifndef membership_h
#define membership_h

#include <xkern/include/dlist.h>

typedef u_bit32_t group_id;

typedef enum {
    FULL,
    DELTA,
    MASTERCHANGE
} membershipXchangeType;

typedef struct {
    group_id              group;        /* IN */
    u_bit32_t             token;        /* IN */
    u_bit32_t             hlpNum;       /* IN */
    xkern_return_t        xkr;          /* OUT */
    membershipXchangeType type;         /* IN */
    union {
	u_bit32_t         absolute;     /* IN/OUT */
	struct {
	    u_bit8_t      additions;    /* IN/OUT */
	    u_bit8_t      deletions;    /* IN/OUT */
	} rel;
    } update_count;
} membershipXchangeHdr;

#define NULL_XCHANGE_HDR        ((membershipXchangeHdr *)0)

/*
 * Elementary unit for the Membership Database.
 * If group is ALL_NODES_GROUP, the unit is
 * used in full (cross-reference functions).
 * The opaque area can be used by protocols
 * which piggyback on membership's maps.
 * The sequencer does use the opaque section
 * as its own scoreboard.
 */

#define EXTRA_BYTES_PER_ELEMENT  24

typedef struct {
    struct dlist dl;                    /* ALL_NODES_GROUP only */
    XObj         session;               /* creator */
    u_long       timestamp;             /* when?   */
    boolean_t    touched;
    u_bit8_t     opaque[EXTRA_BYTES_PER_ELEMENT];
} node_element;

/*
 * ALL_NODES_GROUP is a super set of any other GROUP
 */
#define ALL_NODES_GROUP  0
#define ANY_GROUP        ANY_HOST

/*
 * Control Operations
 */
#define MEMBERSHIP_SETNOTIFICATION    ( MEMBERSHIP_CTL * MAXOPS + 0 )
#define MEMBERSHIP_GETNOTIFICATION    ( MEMBERSHIP_CTL * MAXOPS + 1 )

#endif /* membership_h */

