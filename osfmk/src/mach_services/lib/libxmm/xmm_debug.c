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
 *	File:	norma/xmm_debug.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Xmm layer providing debugging output.
 */

#ifdef	MACH_KERNEL
#include <norma/xmm_obj.h>
#include <sys/varargs.h>
#else	/* MACH_KERNEL */
#include "xmm_obj.h"
#include <varargs.h>
#endif	/* MACH_KERNEL */

struct mobj {
	struct xmm_obj	obj;
	int		id;
	int		kobj_id_counter;
	int		kobj_count;
};

struct kobj {
	struct xmm_obj	obj;
	struct mobj *	mobj;
	vm_size_t	page_size;
	int		id;
};

kern_return_t m_debug_init();
kern_return_t m_debug_terminate();
kern_return_t m_debug_copy();
kern_return_t m_debug_data_request();
kern_return_t m_debug_data_unlock();
kern_return_t m_debug_data_write();
kern_return_t m_debug_lock_completed();
kern_return_t m_debug_supply_completed();
kern_return_t m_debug_data_return();

kern_return_t k_debug_data_provided();
kern_return_t k_debug_data_unavailable();
kern_return_t k_debug_get_attributes();
kern_return_t k_debug_lock_request();
kern_return_t k_debug_data_error();
kern_return_t k_debug_set_attributes();
kern_return_t k_debug_destroy();
kern_return_t k_debug_data_supply();

xmm_decl(debug_mclass,
	/* m_init		*/	m_debug_init,
	/* m_terminate		*/	m_debug_terminate,
	/* m_copy		*/	m_debug_copy,
	/* m_data_request	*/	m_debug_data_request,
	/* m_data_unlock	*/	m_debug_data_unlock,
	/* m_data_write		*/	m_debug_data_write,
	/* m_lock_completed	*/	m_debug_lock_completed,
	/* m_supply_completed	*/	m_debug_supply_completed,
	/* m_data_return	*/	m_debug_data_return,

	/* k_data_provided	*/	k_invalid_data_provided,
	/* k_data_unavailable	*/	k_invalid_data_unavailable,
	/* k_get_attributes	*/	k_invalid_get_attributes,
	/* k_lock_request	*/	k_invalid_lock_request,
	/* k_data_error		*/	k_invalid_data_error,
	/* k_set_attributes	*/	k_invalid_set_attributes,
	/* k_destroy		*/	k_invalid_destroy,
	/* k_data_supply	*/	k_invalid_data_supply,

	/* name			*/	"debug.m",
	/* size			*/	sizeof(struct mobj)
);

xmm_decl(debug_kclass,
	/* m_init		*/	m_invalid_init,
	/* m_terminate		*/	m_invalid_terminate,
	/* m_copy		*/	m_invalid_copy,
	/* m_data_request	*/	m_invalid_data_request,
	/* m_data_unlock	*/	m_invalid_data_unlock,
	/* m_data_write		*/	m_invalid_data_write,
	/* m_lock_completed	*/	m_invalid_lock_completed,
	/* m_supply_completed	*/	m_invalid_supply_completed,
	/* m_data_return	*/	m_invalid_data_return,

	/* k_data_provided	*/	k_debug_data_provided,
	/* k_data_unavailable	*/	k_debug_data_unavailable,
	/* k_get_attributes	*/	k_debug_get_attributes,
	/* k_lock_request	*/	k_debug_lock_request,
	/* k_data_error		*/	k_debug_data_error,
	/* k_set_attributes	*/	k_debug_set_attributes,
	/* k_destroy		*/	k_debug_destroy,
	/* k_data_supply	*/	k_debug_data_supply,

	/* name			*/	"debug.k",
	/* size			*/	sizeof(struct kobj)
);

int mobj_id_counter = 1;
#ifdef	MACH_KERNEL
int xmm_debug_allowed = 0;
#else
int xmm_debug_allowed = 1;
#endif

/* VARARGS */
m_printf(fmt, va_alist)
	char *fmt;
	va_dcl
{
	va_list adx;
	char c, buf[1024], fmt0[3], *bp;
	int i, page_size = 1;
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	xmm_obj_t robj;

	if (! xmm_debug_allowed) {
		return;
	}
	va_start(adx);
	fmt0[0] = '%';
	fmt0[2] = '\0';
	for (;;) {
		bp = buf;
		while ((c = *fmt++) && c != '%') {
			*bp++ = c;
		}
		*bp++ = '\0';
		printf("%s", buf);
		if (c == '\0') {
			break;
		}
		switch (c = *fmt++) {
		case 'M':
			mobj = va_arg(adx, xmm_obj_t);
			kobj = va_arg(adx, xmm_obj_t);
			page_size = KOBJ->page_size;
			printf("k_%s.%d -> m_%s.%d",
			       kobj->k_kobj->class->c_name,
			       KOBJ->id,
			       mobj->m_mobj->class->c_name,
			       MOBJ->id);
			break;

		case 'K':
			kobj = va_arg(adx, xmm_obj_t);
			mobj = (xmm_obj_t) KOBJ->mobj;
			page_size = KOBJ->page_size;
			printf("m_%s.%d -> k_%s.%d",
			       mobj->m_mobj->class->c_name,
			       MOBJ->id,
			       kobj->k_kobj->class->c_name,
			       KOBJ->id);
			break;

		case 'R':
			robj = va_arg(adx, xmm_obj_t);
			if (robj == mobj) {
				printf("reply");
			} else if (robj == XMM_OBJ_NULL) {
				printf("!reply");
			} else {
				printf("?reply");
			}
			break;

		case 'Z':
			i = va_arg(adx, int);
			printf("%d", i / page_size);
			if (i % page_size) {
				printf(".%d", i % page_size);
			}
			break;

		case 'P':
			i = va_arg(adx, int);
			printf("%c%c%c",
			       ((i & VM_PROT_READ) ? 'r' : '-'),
			       ((i & VM_PROT_WRITE) ? 'w' : '-'),
			       ((i & VM_PROT_EXECUTE)? 'x' : '-'));
			break;

		case 'N':
			if (! va_arg(adx, int)) {
				printf("!");
			}
			break;

		case 'C':
			i = va_arg(adx, int);
			if (i == MEMORY_OBJECT_COPY_NONE) {
				bp = "copy_none";
			} else if (i == MEMORY_OBJECT_COPY_CALL) {
				bp = "copy_call";
			} else if (i == MEMORY_OBJECT_COPY_DELAY) {
				bp = "copy_delay";
			} else {
				bp = "copy_???";
			}
			printf("%s", bp);
			break;

		default:
			fmt0[1] = c;
			printf(fmt0, va_arg(adx, long));
			break;
		}
	}
	va_end(adx);
}

kern_return_t
xmm_debug_create(old_mobj, new_mobj)
	xmm_obj_t old_mobj;
	xmm_obj_t *new_mobj;
{
	xmm_obj_t mobj;
	kern_return_t kr;

	kr = xmm_obj_allocate(&debug_mclass, old_mobj, &mobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	MOBJ->id = mobj_id_counter++;
	MOBJ->kobj_id_counter = 1;
	MOBJ->kobj_count = 0;
	*new_mobj = mobj;
	return KERN_SUCCESS;
}

m_debug_init(mobj, k_kobj, memory_object_name, page_size)
	xmm_obj_t mobj;
	xmm_obj_t k_kobj;
	mach_port_t memory_object_name;
	vm_size_t page_size;
{
	xmm_obj_t kobj;

	/* XXX should keep a list of kobjs to detect duplicates? */
	xmm_obj_allocate(&debug_kclass, XMM_OBJ_NULL, &kobj);
	k_kobj->m_kobj = kobj;
	kobj->k_kobj = k_kobj;
	KOBJ->id = MOBJ->kobj_id_counter++;
	MOBJ->kobj_count++;
	KOBJ->page_size = page_size;
	KOBJ->mobj = MOBJ;

	m_printf("m_init            (%M, 0x%x, %d)\n",
		 mobj, kobj, memory_object_name, page_size);
	return M_INIT(mobj, kobj, memory_object_name, page_size);
}

m_debug_terminate(mobj, kobj, memory_object_name)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	mach_port_t memory_object_name;
{
	kern_return_t kr;

	m_printf("m_terminate       (%M, 0x%x)\n",
		 mobj, kobj, memory_object_name);
	kr = M_TERMINATE(mobj, kobj, memory_object_name);
	xmm_obj_deallocate(kobj);
	if (--MOBJ->kobj_count == 0) {
		xmm_obj_deallocate(mobj);
	}
}

m_debug_copy(mobj, kobj, offset, length, new_mobj)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	xmm_obj_t new_mobj;
{
	m_printf("m_copy            (%M, %Z, %Z, 0x%x)\n",
		 mobj, kobj, offset, length, new_mobj);
	return M_COPY(mobj, kobj, offset, length, new_mobj);
}

m_debug_data_request(mobj, kobj, offset, length, desired_access)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{
	m_printf("m_data_request    (%M, %Z, %Z, %P)\n",
		 mobj, kobj, offset, length, desired_access);
	return M_DATA_REQUEST(mobj, kobj, offset, length, desired_access);
}

m_debug_data_unlock(mobj, kobj, offset, length, desired_access)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{
	m_printf("m_data_unlock     (%M, %Z, %Z, %P)\n",
		 mobj, kobj, offset, length, desired_access);
	return M_DATA_UNLOCK(mobj, kobj, offset, length, desired_access);
}

m_debug_data_write(mobj, kobj, offset, data, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	char *data;
	int length;
{
	m_printf("m_data_write      (%M, %Z, 0x%x, %Z)\n",
		 mobj, kobj, offset, data, length);
	return M_DATA_WRITE(mobj, kobj, offset, data, length);
}

m_debug_lock_completed(mobj, kobj, offset, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
{
	m_printf("m_lock_completed  (%M, %Z, %Z)\n",
		 mobj, kobj, offset, length);
	return M_LOCK_COMPLETED(mobj, kobj, offset, length);
}

m_debug_supply_completed(mobj, kobj, offset, length, result, error_offset)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	kern_return_t result;
	vm_offset_t error_offset;
{
	m_printf("m_supply_completed(%M, %Z, %Z, 0x%x, %Z)\n",
		 mobj, kobj, offset, length, result, error_offset);
	return M_SUPPLY_COMPLETED(mobj, kobj, offset, length, result,
				  error_offset);
}

m_debug_data_return(mobj, kobj, offset, data, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
{
	m_printf("m_data_return     (%M, %Z, 0x%x, %Z)\n",
		 mobj, kobj, offset, data, length);
	return M_DATA_RETURN(mobj, kobj, offset, data, length);
}

k_debug_data_provided(kobj, offset, data, length, lock_value)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
	vm_prot_t lock_value;
{
	m_printf("k_data_provided   (%K, %Z, %Z, %P)\n",
		 kobj, offset, length, lock_value);
	return K_DATA_PROVIDED(kobj, offset, data, length, lock_value);
}

k_debug_data_unavailable(kobj, offset, length)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
{
	m_printf("k_data_unavailable(%K, %Z, %Z)\n",
		 kobj, offset, length);
	return K_DATA_UNAVAILABLE(kobj, offset, length);
}

k_debug_get_attributes(kobj, object_ready, may_cache, copy_strategy)
	xmm_obj_t kobj;
	boolean_t *object_ready;
	boolean_t *may_cache;
	memory_object_copy_strategy_t *copy_strategy;
{
	m_printf("k_get_attributes  (%K)\n",
		 kobj);
	return K_GET_ATTRIBUTES(kobj, object_ready, may_cache, copy_strategy);
}

k_debug_lock_request(kobj, offset, length, should_clean, should_flush,
		     lock_value, robj)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	boolean_t should_clean;
	boolean_t should_flush;
	vm_prot_t lock_value;
	xmm_obj_t robj;
{
	m_printf("k_lock_request    (%K, %Z, %Z, %Nclean, %Nflush, %P, %R)\n",
		 kobj, offset, length, should_clean, should_flush, lock_value,
		 robj);
	return K_LOCK_REQUEST(kobj, offset, length, should_clean, should_flush,
			      lock_value, robj);
}

k_debug_data_error(kobj, offset, length, error_value)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	kern_return_t error_value;
{
	m_printf("k_data_error      (%K, %Z, %Z, 0x%x)\n",
		 kobj, offset, length, error_value);
	return K_DATA_ERROR(kobj, offset, length, error_value);
}

k_debug_set_attributes(kobj, object_ready, may_cache, copy_strategy)
	xmm_obj_t kobj;
	boolean_t object_ready;
	boolean_t may_cache;
	memory_object_copy_strategy_t copy_strategy;
{
	m_printf("k_set_attributes  (%K, %Nready, %Nmay_cache, %C)\n",
		 kobj, object_ready, may_cache, copy_strategy);
	return K_SET_ATTRIBUTES(kobj, object_ready, may_cache, copy_strategy);
}

k_debug_destroy(kobj, reason)
	xmm_obj_t kobj;
	kern_return_t reason;
{
	m_printf("k_destroy         (%K, 0x%x)\n",
		 kobj, reason);
	return K_DESTROY(kobj, reason);
}

k_debug_data_supply(kobj, offset, data, length, dealloc_data, lock_value,
		    precious, reply_to)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
	boolean_t dealloc_data;
	vm_prot_t lock_value;
	boolean_t precious;
	mach_port_t reply_to;
{
	m_printf("k_data_supply     (%K, %Z, 0x%x, %Z, %Ndealloc, %P, %Nprecious,reply=0x%x)\n",
		 kobj, offset, data, length, dealloc_data, lock_value,
		 precious, reply_to);
	return K_DATA_SUPPLY(kobj, offset, data, length, dealloc_data,
			     lock_value, precious, reply_to);
}
