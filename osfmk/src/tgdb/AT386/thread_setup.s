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
 * 
 */
/*
 * MkLinux
 */

#include <i386/asm.h>

ENTRY(get_pc)
        movl    4(%ebp), %eax           / fetch pc of caller
        ret
END(get_pc)


/*
 * boolean_t spin_try_lock(int *m)
 */
ENTRY(spin_try_lock)
        movl    4(%esp),%ecx            / point at mutex
        movl    $1,%eax                 / set locked value in acc
        xchg    %eax,(%ecx)             / swap with mutex
                                        / xchg with memory is automatically
                                        / locked
        xorl    $1,%eax                 / 1 (locked) => FALSE
                                        / 0 (locked) => TRUE
        ret
END(spin_try_lock)

/*
 * void spin_unlock(int *m)
 */
ENTRY(spin_unlock)
        movl    4(%esp),%ecx            / point at mutex
        xorl    %eax,%eax               / set unlocked value in acc
        xchg    %eax,(%ecx)             / swap with mutex
                                        / xchg with memory is automatically
                                        / locked
        ret
END(spin_unlock)



