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

#include <platforms.h>
#include <cpus.h>

#include <mach/vm_param.h>
#include <kern/macro_help.h>
#include <kern/misc_protos.h>
#include <intel/pmap.h>
#include <i386/pio.h>
#include <i386/AT386/mp/mp.h>
#include <i386/ipl.h>

#if NCPUS > 1

#if CBUS
#include <busses/cbus/cbus.h>
#endif /* CBUS */

#if MBUS
#include <busses/mbus/mbus.h>
#endif	/* MBUS */

#if MP_V1_1
#include <i386/AT386/mp/mp_v1_1.h>
#endif	/* MP_V1_1 */

#endif	/* NCPUS > 1 */

/*
 * Type of IOBUS values
 */
typedef	unsigned short		IOBUS_TYPE_T;
typedef	volatile i386_ioport_t	IOBUS_ADDR_T;
typedef	volatile unsigned char	IOBUS_VAL8_T;
typedef	volatile unsigned short	IOBUS_VAL16_T;
typedef	volatile unsigned int	IOBUS_VAL32_T;
typedef	volatile vm_offset_t	IOBUS_SHMEM_T;

/*
 * IOBUS synchronization
 */
#define	IOBUS_LOCK_STATE()		at386_io_lock_state()
#if MBUS && NCPUS > 1
#define	IOBUS_LOCK(addr, op)		mbus_dev_lock((addr), (op))
#define	IOBUS_UNLOCK(addr)		mbus_dev_unlock(addr)
#define	IOBUS_LOCK_DECL(attr, name)	attr struct mp_dev_lock name;
#define	IOBUS_LOCK_INIT(addr, num, start, intr, timeo, callb)\
MACRO_BEGIN						\
    simple_lock_init(&(addr)->lock, ETAP_IO_DEVINS);	\
    (addr)->unit = (num);				\
    (addr)->pending_ops = 0;				\
    (addr)->op[MP_DEV_OP_START] = (start);		\
    (addr)->op[MP_DEV_OP_INTR] = (intr);		\
    (addr)->op[MP_DEV_OP_TIMEO] = (timeo);		\
    (addr)->op[MP_DEV_OP_CALLB] = (callb);		\
MACRO_END

#else /*  MBUS && NCPUS > 1 */
#define	IOBUS_LOCK(addr, op)		at386_io_lock(op)
#define	IOBUS_UNLOCK(addr)		at386_io_unlock()
#define	IOBUS_LOCK_DECL(attr, name)
#define	IOBUS_LOCK_INIT(addr, unit, start, intr, timeo, callb)
#endif /*  MBUS && NCPUS > 1 */

#define	IOBUS_LOCK_START		MP_DEV_OP_START
#define	IOBUS_LOCK_INTR			MP_DEV_OP_INTR
#define	IOBUS_LOCK_TIMEO		MP_DEV_OP_TIMEO
#define	IOBUS_LOCK_CALLB		MP_DEV_OP_CALLB
#define	IOBUS_LOCK_WAIT			MP_DEV_WAIT

/*
 * IOBUS SPLs
 */
#define	IOBUS_SPLIMP			SPLIMP

/*
 * AT386 I/O addresses are directly mapped in the i386 I/O space
 */
#define	IOBUS_ADDR(type, addr)		(addr)

#define	IOBUS_IN8(type, addr)		inb(IOBUS_ADDR((type), (addr)))
#define	IOBUS_IN16(type, addr)		inw(IOBUS_ADDR((type), (addr)))
#define	IOBUS_IN32(type, addr)		inl(IOBUS_ADDR((type), (addr)))

#define	IOBUS_OUT8(type, addr, val)	outb(IOBUS_ADDR((type), (addr)), (val))
#define	IOBUS_OUT16(type, addr, val)	outw(IOBUS_ADDR((type), (addr)), (val))
#define	IOBUS_OUT32(type, addr, val)	outl(IOBUS_ADDR((type), (addr)), (val))

#define	IOBUS_COPYIN8(type, from, to, len)			 	\
		bcopy_io_in8((const char *)IOBUS_ADDR((type), (from)),	\
			     (char *)(to), 	 			\
			     (vm_size_t)(len))

#define	IOBUS_COPYOUT8(type, from, to, len)				\
		bcopy_io_out8((const char *)(from), 			\
			      (char *)IOBUS_ADDR((type), (to)),		\
			      (vm_size_t)(len))

#define	IOBUS_COPYIN16(type, from, to, len)				\
		bcopy_io_in16((const char *)IOBUS_ADDR((type), (from)),	\
			     (char *)(to), 				\
			     (vm_size_t)(len))

#define	IOBUS_COPYOUT16(type, from, to, len)				\
		bcopy_io_out16((const char *)(from),			\
			      (char *)IOBUS_ADDR((type), (to)),		\
			      (vm_size_t)(len))

#define	IOBUS_COPYIN32(type, from, to, len)			 	\
		bcopy_io_in32((const char *)IOBUS_ADDR((type), (from)),	\
			     (char *)(to), 				\
			     (vm_size_t)(len))

#define	IOBUS_COPYOUT32(type, from, to, len)				\
		bcopy_io_out32((const char *)(from),			\
			      (char *)IOBUS_ADDR((type), (to)),		\
			      (vm_size_t)(len))

/*
 * IOBUS_COPYTO copies bytes to onboard shared memory
 * IOBUS_COPYFROM copies bytes from onboard shared memory
 * IOBUS_ZERO zeroes onboard shared memory bytes
 */
#define	IOBUS_COPYTO(from, to, len)	\
		bcopy((const char *)(from), (char *)(to), (vm_size_t)(len))
#define	IOBUS_COPYFROM(from, to, len)	\
		bcopy((const char *)(from), (char *)(to), (vm_size_t)(len))
#define	IOBUS_ZERO(to, len)		\
		bzero((char *)(to), (vm_size_t)(len))

/*
 * IOBUS_FLUSH_DCACHE makes memory coherent with cache
 * IOBUS_PURGE_DCACHE invalidates cache without previous flushed
 *
 * N.B. For the i386, the cache is always coherent with the memory, so these
 *	functions are not defined.
 */
#define	IOBUS_FLUSH_DCACHE(type, addr, len)
#define	IOBUS_PURGE_DCACHE(type, addr, len)

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
