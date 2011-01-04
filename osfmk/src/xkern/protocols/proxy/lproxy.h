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
 * lproxy.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.11
 * Date: 1993/09/16 22:02:19
 */


kern_return_t	do_lproxy_dumpXObj(
				mach_port_t,
				xkern_return_t *,
				xobj_ext_id_t,
				xk_xobj_dump_t *
		);

kern_return_t	do_lproxy_ping(
				mach_port_t
		);

kern_return_t	do_lproxy_xCallDemux(
				mach_port_t,
				mach_port_t,
				xkern_return_t *,
				xobj_ext_id_t,
				xk_msg_data_t,
				mach_msg_type_number_t,
				int,
				xk_large_msg_data_t,
				mach_msg_type_number_t,
				xk_msg_data_t,
				mach_msg_type_number_t *,
				xk_large_msg_data_t *,
				mach_msg_type_number_t *,
				xk_msg_attr_t,
				mach_msg_type_number_t
		);

kern_return_t	do_lproxy_xDemux(
				mach_port_t,
				mach_port_t,
				xkern_return_t *,
				xobj_ext_id_t,
				xk_msg_data_t,
				mach_msg_type_number_t,
				int,
				xk_large_msg_data_t,
				mach_msg_type_number_t,
				xk_msg_attr_t,
				mach_msg_type_number_t
		);

kern_return_t	do_lproxy_xOpenDone(
				mach_port_t,
				mach_port_t,
				xkern_return_t *,
				xobj_ext_id_t,
				xobj_ext_id_t,
				xobj_ext_id_t,
				xobj_ext_id_t,
				xk_path_t
		);


