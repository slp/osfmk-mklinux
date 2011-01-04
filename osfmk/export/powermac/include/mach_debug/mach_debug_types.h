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
/* CMU_HIST */
/*
 * Revision 2.7.3.2  92/04/08  15:45:14  jeffreyh
 * 	Revert back to Revision 2.7. Back out mailline changes.
 * 	[92/04/07  10:31:29  jeffreyh]
 * 
 * Revision 2.7  91/07/31  17:56:07  dbg
 * 	Add symtab_name_t.
 * 	[91/07/30  17:12:01  dbg]
 * 
 * Revision 2.6  91/05/14  17:03:54  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/02/05  17:38:07  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:29:06  mrt]
 * 
 * Revision 2.4  91/01/08  15:47:01  rpd
 * 	Added <mach_debug/hash_info.h>.
 * 	[91/01/02            rpd]
 * 
 * Revision 2.3  90/06/02  15:00:45  rpd
 * 	Added <mach_debug/vm_info.h>.
 * 	[90/04/20            rpd]
 * 	Converted to new IPC.
 * 	[90/03/26  22:43:44  rpd]
 * 
 * Revision 2.2  90/05/03  15:48:53  dbg
 * 	Remove callout_statistics; add zone_info, page_info.
 * 	[90/04/06            dbg]
 * 
 * Revision 2.1  89/08/03  16:55:16  rwd
 * Created.
 * 
 * Revision 2.3  89/02/25  18:43:46  gm0w
 * 	Changes for cleanup.
 * 
 * Revision 2.2  89/01/12  08:00:53  rpd
 * 	Created.
 * 	[89/01/12  04:23:42  rpd]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 */
/*
 *	Mach kernel debugging interface type declarations
 */

#ifndef	_MACH_DEBUG_MACH_DEBUG_TYPES_H_
#define _MACH_DEBUG_MACH_DEBUG_TYPES_H_

#include <mach_debug/ipc_info.h>
#include <mach_debug/vm_info.h>
#include <mach_debug/zone_info.h>
#include <mach_debug/page_info.h>
#include <mach_debug/hash_info.h>

typedef	char	symtab_name_t[32];

#endif	/* _MACH_DEBUG_MACH_DEBUG_TYPES_H_ */
