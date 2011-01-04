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
 * Copyright (c) 1994 Open Software Foundation
 *
 * Modelled after vsize_i.h, which is
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.1.7.1 $
 * $Date: 1995/02/23 15:31:36 $
 */

typedef  struct {
	Map	activeMap;
	Map	passiveMap;
} PSTATE;


typedef struct {
	Msg_s	reass;		/* reassembly "queue" */
	int	length;		/* extracted from msg */
} SSTATE;

typedef	XObj	PassiveId;

#define DG_MAGIC	0xDeadBeef	/* footer pattern */
