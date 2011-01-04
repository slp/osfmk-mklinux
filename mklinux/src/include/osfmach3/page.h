/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _OSFMACH3_PAGE_H
#define _OSFMACH3_PAGE_H

#ifdef	PAGE_SHIFT
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))
#else	/* PAGE_SHIFT */
/*
 * Generic values: exported by the micro-kernel through libmach.
 * We might want to use machine-specific values (defined in
 * asm-???/page.h) for performance, though.
 */
extern vm_size_t	page_size;
extern int		page_shift;
extern vm_size_t	page_mask;

#define PAGE_SHIFT	page_shift
#define PAGE_SIZE	page_size
#define PAGE_MASK	(~page_mask)
#endif	/* PAGE_SHIFT */

#ifdef __KERNEL__

#define STRICT_MM_TYPECHECKS

#ifdef STRICT_MM_TYPECHECKS
/*
 * These are used to make use of C type-checking..
 */
typedef struct { unsigned long pgprot; } pgprot_t;
#define pgprot_val(x)	((x).pgprot)
#define __pgprot(x)	((pgprot_t) { (x) } )
#else
/*
 * .. while these make it easier on the compiler
 */
typedef unsigned long pgprot_t;
#define pgprot_val(x)	(x)
#define __pgprot(x)	(x)
#endif

/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)
#define PAGE_TRUNC(addr)	((addr)&PAGE_MASK)

/* This handles the memory map.. */
extern unsigned long __mem_base;
#define PAGE_OFFSET (__mem_base)
#define MAP_NR(addr) (((unsigned long)(addr) - PAGE_OFFSET) >> PAGE_SHIFT)

#endif /* __KERNEL__ */

#endif /* _OSFMACH3_PAGE_H */

