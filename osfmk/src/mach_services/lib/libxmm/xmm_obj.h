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
 * Mach Operating System
 * Copyright (c) 1991 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
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
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 * MkLinux
 */
/*
 *	File:	norma/xmm_obj.h
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Common definitions for xmm system.
 */

#ifdef	MACH_KERNEL
#include <mach/std_types.h>	/* For pointer_t */
#include <mach/mach_types.h>
#include <mach/kern_return.h>
#include <mach/boolean.h>
#include <mach/vm_prot.h>
#include <mach/message.h>
#include <kern/zalloc.h>
#else	/* MACH_KERNEL */
#include <mach.h>
#include "xmm_hash.h"
#include "zalloc.h"
#endif	/* MACH_KERNEL */

/*
 * Mach 2.5 (old ipc) compatibility
 */
#ifndef	MACH_KERNEL
#ifndef	MACH_PORT_NULL
#define	MACH_PORT_NULL		PORT_NULL
#define	mach_port_t		port_t
#undef	task_self
#define	task_self		mach_task_self
#endif
#endif

/*
 * Support for obsolete compilers
 */
#ifndef	__STDC__
#define	void	int
#endif	/* __STDC__ */

typedef kern_return_t		(*kern_routine_t)();

typedef struct xmm_class	*xmm_class_t;
typedef struct xmm_obj		*xmm_obj_t;

#define	XMM_CLASS_NULL		((xmm_class_t) 0)
#define	XMM_OBJ_NULL		((xmm_obj_t) 0)

#define	MOBJ			((struct mobj *) mobj)
#define	KOBJ			((struct kobj *) kobj)

struct xmm_class {
	kern_routine_t	m_init;
	kern_routine_t	m_terminate;
	kern_routine_t	m_copy;
	kern_routine_t	m_data_request;
	kern_routine_t	m_data_unlock;
	kern_routine_t	m_data_write;
	kern_routine_t	m_lock_completed;
	kern_routine_t	m_supply_completed;
	kern_routine_t	m_data_return;
	kern_routine_t	m_change_completed;

	kern_routine_t	k_data_provided;
	kern_routine_t	k_data_unavailable;
	kern_routine_t	k_get_attributes;
	kern_routine_t	k_lock_request;
	kern_routine_t	k_data_error;
	kern_routine_t	k_set_attributes;
	kern_routine_t	k_destroy;
	kern_routine_t	k_data_supply;

	char *		c_name;
	int		c_size;
	zone_t		c_zone;
};

#define xmm_decl(D1,D2,D3,D4,D5,D6,D7,D8,D9,Da,Db,Dc,Dd,De,Df,Dg,Dh,Di,Dj,Dk)\
struct xmm_class D1 = {					\
	/* m_init		*/	D2,		\
	/* m_terminate		*/	D3,		\
	/* m_copy		*/	D4,		\
	/* m_data_request	*/	D5,		\
	/* m_data_unlock	*/	D6,		\
	/* m_data_write		*/	D7,		\
	/* m_lock_completed	*/	D8,		\
	/* m_supply_completed	*/	D9,		\
	/* m_data_return	*/	Da,		\
	/* m_change_completed	*/	0,		\
							\
	/* k_data_provided	*/	Db,		\
	/* k_data_unavailable	*/	Dc,		\
	/* k_get_attributes	*/	Dd,		\
	/* k_lock_request	*/	De,		\
	/* k_data_error		*/	Df,		\
	/* k_set_attributes	*/	Dg,		\
	/* k_destroy		*/	Dh,		\
	/* k_data_supply	*/	Di,		\
							\
	/* name			*/	Dj,		\
	/* size			*/	Dk,		\
	/* zone			*/	ZONE_NULL,	\
}


struct xmm_obj {
	xmm_class_t	class;
	int		refcount;
	xmm_obj_t	m_mobj;
	xmm_obj_t	m_kobj;
	xmm_obj_t	k_mobj;
	xmm_obj_t	k_kobj;
};

kern_return_t	m_interpose_init();
kern_return_t	m_interpose_terminate();
kern_return_t	m_interpose_copy();
kern_return_t	m_interpose_data_request();
kern_return_t	m_interpose_data_unlock();
kern_return_t	m_interpose_data_write();
kern_return_t	m_interpose_lock_completed();
kern_return_t	m_interpose_supply_completed();
kern_return_t	m_interpose_data_return();
kern_return_t	k_interpose_data_provided();
kern_return_t	k_interpose_data_unavailable();
kern_return_t	k_interpose_get_attributes();
kern_return_t	k_interpose_lock_request();
kern_return_t	k_interpose_data_error();
kern_return_t	k_interpose_set_attributes();
kern_return_t	k_interpose_destroy();
kern_return_t	k_interpose_data_supply();

kern_return_t	m_invalid_init();
kern_return_t	m_invalid_terminate();
kern_return_t	m_invalid_copy();
kern_return_t	m_invalid_data_request();
kern_return_t	m_invalid_data_unlock();
kern_return_t	m_invalid_data_write();
kern_return_t	m_invalid_lock_completed();
kern_return_t	m_invalid_supply_completed();
kern_return_t	m_invalid_data_return();
kern_return_t	k_invalid_data_provided();
kern_return_t	k_invalid_data_unavailable();
kern_return_t	k_invalid_get_attributes();
kern_return_t	k_invalid_lock_request();
kern_return_t	k_invalid_data_error();
kern_return_t	k_invalid_set_attributes();
kern_return_t	k_invalid_destroy();
kern_return_t	k_invalid_data_supply();

kern_return_t	xmm_obj_allocate();

#ifndef	MACH_KERNEL
char	*malloc();
int	free();

/*
 * XXX should find or define vm_page_shift
 */
#define	atop(addr)	((addr) / vm_page_size)
#define	ptoa(page)	((page) * vm_page_size)
#endif	/* MACH_KERNEL */

#ifndef lint

#define	M_INIT(MO,MC,MON,PS) \
	(*((MO)->m_mobj->class->m_init))\
		((MO)->m_mobj,MC,MON,PS)

#define	M_TERMINATE(MO,MC,MON) \
	(*((MO)->m_mobj->class->m_terminate))\
		((MO)->m_mobj,(MC)->m_kobj,MON)

#define	M_COPY(MO,MC,O,L,NMO) \
	(*((MO)->m_mobj->class->m_copy))\
		((MO)->m_mobj,(MC)->m_kobj,O,L,NMO)

#define	M_DATA_REQUEST(MO,MC,O,L,DA) \
	(*((MO)->m_mobj->class->m_data_request))\
		((MO)->m_mobj,(MC)->m_kobj,O,L,DA)

#define	M_DATA_UNLOCK(MO,MC,O,L,DA) \
	(*((MO)->m_mobj->class->m_data_unlock))\
		((MO)->m_mobj,(MC)->m_kobj,O,L,DA)

#define	M_DATA_WRITE(MO,MC,O,D,L) \
	(*((MO)->m_mobj->class->m_data_write))\
		((MO)->m_mobj,(MC)->m_kobj,O,D,L)

#if 0
#define	M_LOCK_COMPLETED(RO,MC,O,L) \
	(*((RO)->class->m_lock_completed))\
		(RO,(MC)->m_kobj,O,L)
#else
/* XXX */
#define	M_LOCK_COMPLETED(RO,MC,O,L) \
	(*((RO)->m_mobj->class->m_lock_completed))\
		((RO)->m_mobj,(MC)->m_kobj,O,L)
#endif

#define	M_SUPPLY_COMPLETED(MO,MC,O,L,R,EO) \
	(*((MO)->m_mobj->class->m_supply_completed))\
		((MO)->m_mobj,(MC)->m_kobj,O,L,R,EO)

#define	M_DATA_RETURN(MO,MC,O,D,L) \
	(*((MO)->m_mobj->class->m_data_return))\
		((MO)->m_mobj,(MC)->m_kobj,O,D,L)

#define	M_CHANGE_COMPLETED(MO,MC,C,CS) \
	(*((MO)->m_mobj->class->m_change_completed))\
		((MO)->m_mobj,(MC)->m_kobj,C,CS)

#define	K_DATA_PROVIDED(MC,O,D,L,LV) \
	(*((MC)->k_kobj->class->k_data_provided))((MC)->k_kobj,O,D,L,LV)

#define	K_DATA_UNAVAILABLE(MC,O,L) \
	(*((MC)->k_kobj->class->k_data_unavailable))((MC)->k_kobj,O,L)

#define	K_GET_ATTRIBUTES(MC,OR,MCA,CS) \
	(*((MC)->k_kobj->class->k_get_attributes))((MC)->k_kobj,OR,MCA,CS)

#if 0
#define	K_LOCK_REQUEST(MC,O,L,SC,SF,LV,RO) \
	(*((MC)->k_kobj->class->k_lock_request))((MC)->k_kobj,O,L,SC,SF,LV,RO)
#else
#define	K_LOCK_REQUEST(MC,O,L,SC,SF,LV,RO) \
	(*((MC)->k_kobj->class->k_lock_request))((MC)->k_kobj,O,L,SC,SF,LV,\
			((RO) ? (RO)->k_mobj : XMM_OBJ_NULL))
#endif

#define	K_DATA_ERROR(MC,O,L,EV) \
	(*((MC)->k_kobj->class->k_data_error))((MC)->k_kobj,O,L,EV)

#define	K_SET_ATTRIBUTES(MC,OR,MCA,CS) \
	(*((MC)->k_kobj->class->k_set_attributes))((MC)->k_kobj,OR,MCA,CS)

#define	K_DESTROY(MC,R) \
	(*((MC)->k_kobj->class->k_destroy))((MC)->k_kobj,R)

#define	K_DATA_SUPPLY(MC,O,D,L,DD,LV,P,RT) \
	(*((MC)->k_kobj->class->k_data_supply))((MC)->k_kobj,O,D,L,DD,LV,P,RT)


#define	_K_DESTROY(MC,R) \
	(*((MC)->class->k_destroy))(MC,R)

#endif	/* lint */
