/*
 * PowerPC atomic operations
 */

#ifndef _ASM_PPC_ATOMIC_H_ 
#define _ASM_PPC_ATOMIC_H_

typedef int atomic_t;
void atomic_add(int, atomic_t *);
void atomic_sub(int, atomic_t *);
void atomic_inc(atomic_t *);
void atomic_dec(atomic_t *);
int atomic_dec_and_test(atomic_t *);
int atomic_inc_return(atomic_t *);
int atomic_dec_return(atomic_t *);
#endif

