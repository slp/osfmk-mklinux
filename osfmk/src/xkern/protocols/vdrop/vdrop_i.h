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
 * vdrop_i.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * 1.4
 * 1993/02/12 23:13:47
 */


#define	VDROP_ACT_MAP_SZ	11
#define	VDROP_PAS_MAP_SZ	11

/* 
 * VDROP_MAX_INTERVAL is the upper bound of the initial interval
 * value.  The actual interval value is randomly selected between 2
 * and VDROP_MAX_INTERVAL.  User-specified intervals may exceed
 * VDROP_MAX_INTERVAL. 
 */
#define VDROP_MAX_INTERVAL	20


typedef struct {
    Map		activeMap;
    Map 	passiveMap;
} PState;

typedef struct {
    int		interval;
    int		count;
} SState;

