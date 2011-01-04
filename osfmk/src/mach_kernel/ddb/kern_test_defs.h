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

/*
 * Definitions for in-kernel tests.
 */

/*
 * These constants are used as indices into various arrays, and so should
 * be assigned values 0, 1, 2, 3, ...
 */
#define PAGING_LOAD	0
#define VM_TEST		1
#define SCSIT_TEST	2
#define KKT_TEST	3
#define MSG_TEST	4
#define DIPC_TEST	5
#define MAX_TEST	5

/*
 * These constants are used to test a particular bit, and so should
 * be assigned values 1, 2, 4, 8, ...
 */
#define PAGING_LOAD_BIT	1
#define VM_TEST_BIT	2
#define SCSIT_TEST_BIT	4
#define KKT_TEST_BIT	8
#define MSG_TEST_BIT	16
#define DIPC_TEST_BIT	32
#define MAX_TEST_BIT	32

extern unsigned int kern_test_intr_control[];

extern unsigned int kern_test_enable;
extern unsigned int kern_test_intr_enable;

/* external function prototypes */
extern void kern_test_start_paging(void);
extern void kern_test_stop_paging(void);
extern void kernel_test_intr(void);
extern void kernel_test_thread(void);


