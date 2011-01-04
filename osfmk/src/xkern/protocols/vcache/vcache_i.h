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
 * vcache_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision 1.8.1.1  1994/07/08  05:07:03  menze
 * Added maps and session state to support storage of participants used
 * to open sessions.  This prevents having to re-open lower sessions.
 *
 */

#define VCACHE_ALL_SESSN_MAP_SZ	511	/* sessn keyed (all vcache sessns) */
#define	VCACHE_HLP_MAP_SZ	11	/* hlp keyed */
#define VCACHE_COLLECT_INTERVAL	15	/* seconds */
#define PART_BUF_SZ		500

#include <xkern/include/dlist.h>

typedef struct {
    Map 	passiveMap;

    /* 
     * idleMap:  { lls -> sessn }
     */
    Map 	idleMap;

    /* 
     * idlePartMap:  { Participants -> list of sessns }
     *
     * The idlePartMap is used to satisfy active open requests, and so
     * vcache sessions are placed in the idlePartMap only if they were
     * returned as the result of an active open.  This prevents us
     * from returning server sessions as client sessions, if the lower
     * protocol makes such distinctions. 
     *
     * The list of sessions is necessary to support caching above
     * protocols that return distinct sessions for each xOpen with the
     * same participants (e.g., CHAN).
     */
    Map 	idlePartMap;

    /* 
     * activeLlsMap:  { lls -> sessn }
     */
    Map		activeMap;

} PState;

typedef struct SState {
    struct dlist	e;
    XObj		self;
    IPhost		remote_host;
    void		*partBuf;
    int			partBufLen;
} SState;



extern int	tracevcachep;



