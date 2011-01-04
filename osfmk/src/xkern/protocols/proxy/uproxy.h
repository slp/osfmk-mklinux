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
 * uproxy.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.9
 * Date: 1993/09/21 00:23:23
 */


kern_return_t	do_uproxy_abort( PORT_TYPE,
				PORT_TYPE);

kern_return_t	do_uproxy_dumpXObj(PORT_TYPE,
				   xkern_return_t *,
				   xobj_ext_id_t,
				   xk_xobj_dump_t *);

kern_return_t	do_uproxy_ping( PORT_TYPE );

kern_return_t	do_uproxy_pingtest( PORT_TYPE,
				   PORT_TYPE,
				   int );

kern_return_t	do_uproxy_xClose( PORT_TYPE,
				 xkern_return_t *,
				 xobj_ext_id_t );

kern_return_t	do_uproxy_xControl( PORT_TYPE,
				   xobj_ext_id_t,
				   int,
				   xk_ctl_buf_t,
				   int *);

kern_return_t	do_uproxy_xDuplicate( PORT_TYPE,
				     PORT_TYPE,
				     xkern_return_t *,
				     xobj_ext_id_t,
				     xobj_ext_id_t );

kern_return_t	do_uproxy_xOpenEnable( PORT_TYPE,
				      PORT_TYPE,
				      xkern_return_t *,
				      xobj_ext_id_t,
				      xobj_ext_id_t,
				      xobj_ext_id_t,
				      xk_part_t );

kern_return_t	do_uproxy_xOpenDisable( PORT_TYPE,
				       PORT_TYPE,
				       xkern_return_t *,
				       xobj_ext_id_t,
				       xobj_ext_id_t,
				       xobj_ext_id_t,
				       xk_part_t );

kern_return_t	do_uproxy_xOpen( PORT_TYPE,
				PORT_TYPE,
				xobj_ext_id_t *,
				xobj_ext_id_t,
				xobj_ext_id_t,
				xobj_ext_id_t,
				xk_part_t,
				xk_path_t );

kern_return_t	do_uproxy_xGetProtlByName( PORT_TYPE,
					  xk_string_t,
					  xobj_ext_id_t * );

kern_return_t	do_uproxy_xPush( PORT_TYPE,
				xmsg_handle_t *,
				xobj_ext_id_t,
				char *,
				mach_msg_type_number_t,
				char *,
				mach_msg_type_number_t,
/*
				boolean_t,
*/
				xk_msg_attr_t,
				mach_msg_type_number_t,
				xk_path_t );

kern_return_t	do_uproxy_xCall( PORT_TYPE,
				xkern_return_t *,
				xobj_ext_id_t,
				char *,
				mach_msg_type_number_t,
				char *,
				mach_msg_type_number_t,
/*
				boolean_t,
*/
				char *,
				mach_msg_type_number_t *,
				char **,
				mach_msg_type_number_t *,
				xk_msg_attr_t,
				mach_msg_type_number_t,
				xk_path_t,
				xk_path_t *);

