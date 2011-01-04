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
 * Revision 2.5  91/05/14  17:48:31  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:57:53  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:31:29  mrt]
 * 
 * Revision 2.3  90/05/29  18:38:36  rwd
 * 	Picked up rfr changes.
 * 	[90/04/12  13:46:56  rwd]
 * 
 * Revision 2.2  90/01/11  11:47:31  dbg
 * 	Added changes from mainline:
 * 		Remember bitmap size, for safer deallocation.
 * 		[89/10/16  15:32:58  af]
 * 		Declare vm_external_destroy().
 * 		[89/08/08            mwyoung]
 * 
 * Revision 2.1  89/08/03  16:44:46  rwd
 * Created.
 * 
 * Revision 2.3  89/04/18  21:24:53  mwyoung
 * 	Created.
 * 	[89/04/18            mwyoung]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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

#ifndef	VM_VM_EXTERNAL_H_
#define VM_VM_EXTERNAL_H_

#include <mach/boolean.h>
#include <mach/machine/vm_types.h>

/*
 *	External page management hint technology
 *
 *	The data structure exported by this module maintains
 *	a (potentially incomplete) map of the pages written
 *	to external storage for a range of virtual memory.
 */

typedef char	*vm_external_map_t;
#define	VM_EXTERNAL_NULL	((char *) 0)

/*
 *	The states that may be recorded for a page of external storage.
 */

typedef int	vm_external_state_t;
#define	VM_EXTERNAL_STATE_EXISTS		1
#define	VM_EXTERNAL_STATE_UNKNOWN		2
#define	VM_EXTERNAL_STATE_ABSENT		3

/*
 * Useful macros
 */
#define stob(s)	((atop((s)) + 07) >> 3)

/*
 *	Routines exported by this module.
 */
					/* Initialize the module */
extern void			vm_external_module_initialize(void);


extern vm_external_map_t	vm_external_create(
					/* Create a vm_external_map_t */
					vm_offset_t		size);

extern void			vm_external_destroy(
					/* Destroy one */
					vm_external_map_t	map,
					vm_size_t		size);

extern vm_size_t		vm_external_map_size(
					/* Return size of map in bytes */
					vm_offset_t	size);

extern void			vm_external_copy(
					/* Copy one into another */
					vm_external_map_t	old_map,
					vm_size_t		old_size,
					vm_external_map_t	new_map);

extern void			vm_external_state_set(	
					/* Set state of a page to
					 * VM_EXTERNAL_STATE_EXISTS */
					vm_external_map_t	map,
					vm_offset_t		offset);

#define	vm_external_state_get(map, offset)			  	\
			(((map) != VM_EXTERNAL_NULL) ? 			\
			  _vm_external_state_get((map), (offset)) :	\
			  VM_EXTERNAL_STATE_UNKNOWN)
					/* Retrieve the state for a
					 * given page, if known.  */

extern vm_external_state_t	_vm_external_state_get(
					/* HIDDEN routine */
					vm_external_map_t	map,
					vm_offset_t		offset);

boolean_t			vm_external_within(
					/* Check if new object size
					 * fits in current map */
					vm_size_t	new_size, 
					vm_size_t	old_size);
#endif	/* VM_VM_EXTERNAL_H_ */
