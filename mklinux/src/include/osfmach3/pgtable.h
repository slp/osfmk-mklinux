/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _OSFMACH3_PGTABLE_H
#define _OSFMACH3_PGTABLE_H

/* We don't have any memory cache */
#define flush_cache_all()			do { } while (0)
#define flush_cache_mm(mm)			do { } while (0)
#define flush_cache_range(mm, start, end)	do { } while (0)
#define flush_cache_page(vma, vmaddr)		do { } while (0)
#define flush_page_to_ram(page)			do { } while (0)

#define flush_tlb_all()				do { } while (0)
#define flush_tlb_mm(mm)			do { } while (0)
#define flush_tlb_page(vma, addr)		do { } while (0)
#define flush_tlb_range(mm, start, end)		do { } while (0)
#define local_flush_tlb()			do { } while (0)

#define _PAGE_PRESENT	0x001
#define _PAGE_READ	0x002
#define _PAGE_WRITE	0x004
#define _PAGE_EXEC	0x008
#define _PAGE_COW	0x010

#define _PAGE_TABLE	(_PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE)
#define _PAGE_CHG_MASK	(PAGE_MASK)

#define PAGE_NONE	__pgprot(_PAGE_PRESENT)
#define PAGE_SHARED	__pgprot(_PAGE_PRESENT|_PAGE_READ|_PAGE_WRITE)
#define PAGE_COPY	__pgprot(_PAGE_PRESENT|_PAGE_COW)
#define PAGE_READONLY	__pgprot(_PAGE_PRESENT)

/* Do we need to distinguish EXEC permission ? */
#define __P000	PAGE_NONE
#define __P001	PAGE_READONLY
#define __P010	PAGE_COPY
#define __P011	PAGE_COPY
#define __P100	PAGE_READONLY
#define __P101	PAGE_READONLY
#define __P110	PAGE_COPY
#define __P111	PAGE_COPY

#define __S000	PAGE_NONE
#define __S001	PAGE_READONLY
#define __S010	PAGE_SHARED
#define __S011	PAGE_SHARED
#define __S100	PAGE_READONLY
#define __S101	PAGE_READONLY
#define __S110	PAGE_SHARED
#define __S111	PAGE_SHARED

/*
 * We've replaced the "pgd" field with a pointer to the task in "mm_struct".
 */
#define swapper_pg_dir	(&init_osfmach3_task)

#endif /* _OSFMACH3_PGTABLE_H */

