/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */


#ifndef _ASM_OSFMACH3_MACHINE_ATOMIC_H
#define _ASM_OSFMACH3_MACHINE_ATOMIC_H

typedef int atomic_t;
#define atomic_add(i, v)       (*(v) += i)
#define atomic_sub(i, v)       (*(v) -= i)
#define atomic_inc(v)          (*(v) += 1)
#define atomic_dec(v)          (*(v) -= 1)
#define atomic_dec_and_test(v) ((*(v) -= 1) == 0)

#define atomic_dec_return      atomic_dec
#define atomic_inc_return      atomic_inc

#endif	/* _ASM_OSFMACH3_MACHINE_ATOMIC_H */
