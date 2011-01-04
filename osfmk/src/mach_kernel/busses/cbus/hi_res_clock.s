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
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 */

	.file	"clock.c"
/ [created and documented 19 April 91    benjamin]
/
/ Maintains a clock by running a tight loop on a dedicated i386 (number of
/ tight loop iterations is 1st parameter), incrementing a clock (32-bit
/ quantity whose address is 2nd parameter).  This assembly code was created
/ by compiling the C routine below with -O and adding 
/
/  (1) a nop so that clock ticks are more easily mapped into microseconds
/  # not yet# (2) a CLI (mask interrupts) to preserve the regularity of the clock.
/
/ The clock counts consumed by the for loops are detailed below, instruction
/ by instruction.  For INNER LOOP, clock count is 12 * inner_loop_iterations.
/
/ For the OUTER_LOOP (which is the length of a clock tick, clock count is
/
/     20 + ( 12 * loop_iterations_per_tick ).
/
/ Note the addition of a NOP in the outer loop, which brings the outer
/ loop\'s overhead from 17 to 20 cycles.  This permits the tick to be
/ interpreted as a multiple of microseconds on a 16MHz i386. It is also
/ possible that the memory reference to update the clock could suffer
/ bus contention and be delayed, so the clock isn\'t absolutely perfect.
/ But then, what is...
/ PARAMETERS   :  int clock, inner_loop_iterations_per_tick
/ CALL SEQUENCE:  clock_thread_386 (inner_loop_iterations_per_tick, &clock)
/
/     inner_loop      cycles        microseconds
/     iterations       on a         per tick on 
/     per tick         i386         a 16MHz i386
/     ----------      ------        ------------
/        1             32              2 
/        2             44              -
/        3             56              -
/        4             68              -
/        5             80              5
gcc_compiled.:
.text
	.align 2
.globl _clock_thread_386
_clock_thread_386:
	pushl %ebp
	movl %esp,%ebp
	movl 8(%ebp),%ecx  / %ecx <--  inner_loop_iterations
	movl 12(%ebp),%edx 
	movl $0,(%edx)     / (%edx) is the clock, start with 0 ticks
        cli                / keep interrupts out of the loop, but causes boot crash, somehow...
L2:                   / OUTER LOOP BEGINS HERE
	xorl %eax,%eax     / zero loop counter          ** clock count = 2 **
	cmpl %eax,%ecx     / 1st compare of inner loop  ** clock count = 2 **
	jle L9             / never taken                ** clock count = 3 **
L7:                   / INNER LOOP BEGINS HERE
	incl %eax          / bump loop counter          ** clock count = 2 **
	cmpl %eax,%ecx     / finished inner loop?       ** clock count = 2 **
	jg L7              / taken (except last time)   ** clock count = 8 **
                      / INNER LOOP ENDS HERE
L9:                        / fall-through jg was cheap ** clock count = -5 **
	incl (%edx)        / tick the clock             ** clock count = 6 **
        nop / INSERTED BY HAND so tick = X microsecs    ** clock count = 3 **
	jmp L2             / always taken               ** clock count = 9 **
	.align 2      / OUTER LOOP ENDS HERE
	leave
	ret
/
/
/clock_thread_386 (loop_iterations_per_tick,
/                  clock_addr)
/
/             int   loop_iterations_per_tick;  ** input                  **
/    unsigned int  *clock_addr;                ** input, shared memory   **
/
/
/    {
/      int  loop_counter;
/
/      for (*clock_addr = 0 ;; (*clock_addr)++ ) 
/
/           for (loop_counter = 0;
/                loop_counter < loop_iterations_per_tick;
/                loop_counter++) {
/           };
/    }
/           
/ *Since this is a clock, after all, I\'m including a timing analysis
/  of the optimized version of this routine using our gnu C compiler.
/  These times do not reflect bus contention for the clock value.
/
/  Note that I added a nop to the outer loop to allow the clock to
/  count nice multiples of microseconds.  When recompiling, be sure
/  to put it in / adjust for different chip frequencies!
/
/  Of course, they will not be valid with other compilers, non-optimized
/  compiles with this compiler, nor with an i486 cpu.
