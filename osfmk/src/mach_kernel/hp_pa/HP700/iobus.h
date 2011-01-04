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

#ifndef _MACHINE_IOBUS_H_
#define	_MACHINE_IOBUS_H_

#include <mach_rt.h>
#include <mach/vm_types.h>
#include <kern/misc_protos.h>
#include <kern/macro_help.h>
#include <hp_pa/spl.h>
#include <hp_pa/pmap.h>

/*
 * Type of IOBUS values
 */
typedef unsigned int		IOBUS_TYPE_T;
typedef	volatile unsigned long	IOBUS_ADDR_T;
typedef	volatile unsigned char	IOBUS_VAL8_T;
typedef	volatile unsigned short	IOBUS_VAL16_T;
typedef	volatile unsigned int	IOBUS_VAL32_T;
typedef	volatile vm_offset_t	IOBUS_SHMEM_T;

/*
 * IOBUS synchronization
 */
#include <device/driver_lock.h>

#define	IOBUS_LOCK_STATE()						\
			driver_lock_state()
#define	IOBUS_LOCK(addr, op)						\
			driver_lock((addr), (op), FALSE)
#define	IOBUS_UNLOCK(addr)						\
			driver_unlock(addr)
#define	IOBUS_LOCK_DECL(attr, name)					\
			driver_lock_decl(attr, name)
#define	IOBUS_LOCK_INIT(addr, unit, start, intr, timeo, callb)		\
			driver_lock_init((addr), (unit), (start), \
					 (intr), (timeo), (callb))

#define	IOBUS_LOCK_START		DRIVER_OP_START
#define	IOBUS_LOCK_INTR			DRIVER_OP_INTR
#define	IOBUS_LOCK_TIMEO		DRIVER_OP_TIMEO
#define	IOBUS_LOCK_CALLB		DRIVER_OP_CALLB
#define	IOBUS_LOCK_WAIT			DRIVER_OP_WAIT

/*
 * IOBUS SPLs
 */
#define	IOBUS_SPLIMP			SPLIMP

/*
 * HP700 I/O ISA extension boards addresses:
 *	Each range of 8 consecutive address is mapped into an unique 4K page
 */
#ifdef notdef	/* EISA not yet included */
#define	IOBUS_ADDR(iobus, addr)						\
	(IOBUS_TYPE_BUS(iobus) == IOBUS_TYPE_GSC ?			\
	 (addr) :							\
	 IOBUS_TYPE_BUS(iobus) == IOBUS_TYPE_EISA ?			\
	 (IOBUS_EISA_BASE_ADDR + (addr)) :				\
	 (IOBUS_EISA_BASE_ADDR + (((addr) & ~0x7) << 9) + ((addr) & 0x7)))
#else
#define	IOBUS_ADDR(iobus, addr)	(addr)
#endif

#define	IOBUS_IN8(type, addr)		\
			*(IOBUS_VAL8_T *)IOBUS_ADDR((type), (addr))
#define	IOBUS_IN16(type, addr)		\
			*(IOBUS_VAL16_T *)IOBUS_ADDR((type), (addr))
#define	IOBUS_IN32(type, addr)		\
			*(IOBUS_VAL32_T *)IOBUS_ADDR((type), (addr))

#define	IOBUS_OUT8(type, addr, val)	\
			*(IOBUS_VAL8_T *)IOBUS_ADDR((type), (addr)) = (val)
#define	IOBUS_OUT16(type, addr, val)	\
			*(IOBUS_VAL16_T *)IOBUS_ADDR((type), (addr)) = (val)
#define	IOBUS_OUT32(type, addr, val)	\
			*(IOBUS_VAL32_T *)IOBUS_ADDR((type), (addr)) = (val)

#define	IOBUS_COPYIN8(type, from, to, len)			\
		bcopy((const char *)IOBUS_ADDR((type), (from)),	\
		      (char *)(to), 				\
		      (vm_size_t)(len))

#define	IOBUS_COPYOUT8(type, from, to, len)			\
		bcopy((const char *)(from),			\
		      (char *)IOBUS_ADDR((type), (to)),		\
		      (vm_size_t)(len))

#define	IOBUS_COPYIN16(type, from, to, len)			\
		bcopy((const char *)IOBUS_ADDR((type), (from)),	\
		      (char *)((to) + (offset)), 		\
		      (vm_size_t)(len))

#define	IOBUS_COPYOUT16(type, from, to, len)			\
		bcopy((const char *)(from),			\
		      (char *)IOBUS_ADDR((type), (to)),		\
		      (vm_size_t)(len))

#define	IOBUS_COPYIN32(type, from, to, offset, len)		\
		bcopy((const char *)IOBUS_ADDR((type), (from)),	\
		      (char *)(to),		 		\
		      (vm_size_t)(len))

#define	IOBUS_COPYOUT32(type, from, to, len)			\
		bcopy((const char *)(from),			\
		      (char *)IOBUS_ADDR((type), (to)),		\
		      (vm_size_t)(len))

/*
 * IOBUS_COPYTO copies bytes to onboard shared memory
 * IOBUS_COPYFROM copies bytes from onboard shared memory
 * IOBUS_ZERO zeroes onboard shared memory bytes
 */
#define	IOBUS_COPYTO(from, to, len)	\
		bcopy((const char *)(from), (char *)(to),(vm_size_t)(len))

#define	IOBUS_COPYFROM(from, to, len)	\
		bcopy((const char *)(from), (char *)(to),(vm_size_t)(len))

#define	IOBUS_ZERO(to, len)		\
		bzero((char *)(to),(vm_size_t)(len))

/*
 * IOBUS_FLUSH_DCACHE makes memory coherent with cache
 * IOBUS_PURGE_DCACHE invalidates cache without previous flush
 */
#define	IOBUS_FLUSH_DCACHE(iotype, addr, len)				\
MACRO_BEGIN								\
	if (IOBUS_TYPE_FLAGS(iotype) & IOBUS_TYPE_FLAGS_NOTCOHERENT)	\
	    fdcache(HP700_SID_KERNEL,					\
		    (vm_offset_t)(addr), (vm_size_t)(len));		\
MACRO_END

#define	IOBUS_PURGE_DCACHE(iotype, addr, len)				\
MACRO_BEGIN								\
	if (IOBUS_TYPE_FLAGS(iotype) & IOBUS_TYPE_FLAGS_NOTCOHERENT)	\
	    pdcache(HP700_SID_KERNEL,					\
		    (vm_offset_t)(addr), (vm_size_t)(len));		\
MACRO_END

/*
 * IOBUS_VADDR_MAP map host virtual memory to board memory space
 * IOBUS_PADDR_MAP map host physical memory to board memory space
 * IOBUS_ADDR_UNMAP unmap board memory space
 *
 * N.B. vaddr is the host virtual memory
 */
#define	IOBUS_VADDR_MAP(ioaddr, vaddr, len)
#define	IOBUS_PADDR_MAP(ioaddr, paddr, len)
#define	IOBUS_ADDR_UNMAP(ioaddr, len)

#endif /* _MACHINE_IOBUS_H_ */
