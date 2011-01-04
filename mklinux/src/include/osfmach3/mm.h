/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef	_OSFMACH3_MM_H
#define _OSFMACH3_MM_H

#include <linux/sched.h>
#include <linux/mm.h>

/*
 * Back-end routines to interface Mach VM from Linux VM.
 */

extern void osfmach3_mem_init(unsigned long start_mem,
			      unsigned long end_mem);
extern unsigned long osfmach3_paging_init(unsigned long start_mem,
					  unsigned long end_mem);

extern void osfmach3_exit_mmap(struct mm_struct *mm);
extern void osfmach3_insert_vm_struct(struct mm_struct *mm,
				      struct vm_area_struct *vmp);
extern void osfmach3_remove_shared_vm_struct(struct vm_area_struct *mpnt);
extern int osfmach3_split_vm_struct(unsigned long addr,
				    size_t len);
extern void osfmach3_mprotect_fixup(struct vm_area_struct *vmp,
				    unsigned long start,
				    unsigned long end,
				    unsigned int newflags);
extern int osfmach3_msync(struct vm_area_struct *vmp,
			  unsigned long address,
			  size_t size,
			  unsigned int flags);
extern unsigned long osfmach3_mremap(struct vm_area_struct *vma,
				     unsigned long addr,
				     unsigned long old_len,
				     unsigned long new_len,
				     unsigned long flags);
extern void osfmach3_mlock_fixup(struct vm_area_struct *vma,
				 unsigned int newflags);

extern mach_port_t	osfmach3_get_default_pager_port(void);

#endif	/* _OSFMACH3_MM_H */
