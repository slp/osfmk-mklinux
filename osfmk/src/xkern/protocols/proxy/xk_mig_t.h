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
 */
/*
 * MkLinux
 */
/*     
 * xk_mig_t.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.8
 * Date: 1993/09/21 00:24:00
 */

#ifndef XK_MIG_T_H
#define XK_MIG_T_H

#include <xkern/include/part.h>
#include <xkern/include/msg.h>
#include <xkern/protocols/proxy/xk_mig_sizes.h>


#ifdef MACH_KERNEL
#  define PORT_TYPE	ipc_port_t
#  include <ipc/ipc_port.h>
#else
#  define PORT_TYPE	mach_port_t
#endif


typedef	char		xk_string_80_t[80];

typedef xk_string_80_t 	xk_string_t;

typedef int		xobj_ext_id_t;

typedef	xk_u_int32	xk_path_t;

typedef struct {
    int			type;
    xobj_ext_id_t	hlpRcv;
    xobj_ext_id_t	hlpType;
    xobj_ext_id_t	myprotl;
    xk_path_t		pathId;
    xk_string_t		name;
    xk_string_t		instName;
} xk_xobj_dump_t;

typedef int		xk_part_t[PART_EXT_BUF_LEN];

typedef struct {
    int	type;
    int	len;
} xk_msg_info_t;

typedef	char	*xk_msg_data_t;
typedef	char	*xk_msg_attr_t;
typedef	char	*xk_large_msg_data_t;

typedef struct {
    void	*machMsg;
    Msg		xkMsg;
} xk_and_mach_msg_t;

typedef char	xk_ctl_buf_t[XK_MAX_CTL_BUF_LEN];

#ifndef offsetof
#  define offsetof(type, mem) ((int) \
			((char *)&((type *) 0)->mem - (char *)((type *) 0)))
#endif

#endif /* ! XK_MIG_T_H */

