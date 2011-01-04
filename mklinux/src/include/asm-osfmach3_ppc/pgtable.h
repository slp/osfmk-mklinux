/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _ASM_OSFMACH3_PPC_PGTABLE_H
#define _ASM_OSFMACH3_PPC_PGTABLE_H

#include <osfmach3/pgtable.h>

#define SWP_TYPE(entry) (((entry) >> 1) & 0x7f)
#define SWP_OFFSET(entry) ((entry) >> 8)
#define SWP_ENTRY(type,offset) (((type) << 1) | ((offset) << 8))

#define flush_instruction_cache()
extern void flush_instruction_cache_range(struct mm_struct *mm,
					  unsigned long start,
					  unsigned long end);

#endif	/* _ASM_OSFMACH3_PPC_PGTABLE_H */
