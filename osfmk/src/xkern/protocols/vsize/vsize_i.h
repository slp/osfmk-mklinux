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
 * vsize_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.7
 * Date: 1993/07/26 22:13:27
 */

#define VSIZEMAXDOWN 8

typedef  struct {
    Map		activeMap;
    Map 	passiveMap;
    /* 
     * If a hard cutoff is specified in configuration, it is stored in
     * protocol state and copied to each session state.  If the cutoff
     * is to be determined dynamically for each session (the preferred
     * method), protocol state cutoff is < 0.
     */
    int		numdown; 
    int		cutoff[VSIZEMAXDOWN];
} PSTATE;


typedef struct {
    int		numdown; 
    int		cutoff[VSIZEMAXDOWN];
} SSTATE;

typedef	XObj	PassiveId;
