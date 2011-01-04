/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef __OSFMACH3_PROCESSOR_H
#define __OSFMACH3_PROCESSOR_H

#define EISA_bus__is_a_macro	/* for versions in ksyms.c */
#define MCA_bus__is_a_macro	/* for versions in ksyms.c */

#define wp_works_ok 1
#define wp_works_ok__is_a_macro	/* for versions in ksyms.c */

struct thread_struct {
	unsigned long	wchan;
	unsigned long	saved_pc;
};

#define INIT_MMAP { &init_mm, VM_MIN_ADDRESS, VM_MAX_ADDRESS, PAGE_SHARED, VM_READ | VM_WRITE | VM_EXEC }

#define INIT_TSS { 							\
	0, 		/* wchan */					\
	0 		/* saved_pc */					\
}

extern void start_thread(struct pt_regs *regs,
			 unsigned long pc,
			 unsigned long sp);

/*
 * Return saved PC of a blocked thread.
 */
extern inline unsigned long thread_saved_pc(struct thread_struct *t)
{
	return t->saved_pc;
}

extern kern_return_t osfmach3_thread_set_state(mach_port_t thread_port,
					       struct pt_regs *regs);

#endif	/* __OSFMACH3_PROCESSOR_H */
