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
/* CMU_HIST */
/*
 * Revision 2.5  91/05/14  16:10:48  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:12:45  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:35:36  mrt]
 * 
 * Revision 2.3  90/08/27  21:57:15  dbg
 * 	Use new segment definitions.
 * 	[90/07/25            dbg]
 * 
 * Revision 2.2  90/05/03  15:33:38  dbg
 * 	Created.
 * 	[90/02/15            dbg]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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

/*
 * "Local" descriptor table.  At the moment, all tasks use the
 * same LDT.
 */
#include <i386/seg.h>
#include <mach/i386/vm_types.h>
#include <mach/i386/vm_param.h>

extern int	syscall(void);
extern int	mach_rpc(void);

struct fake_descriptor	ldt[LDTSZ] = {
/*007*/	{ (unsigned int)&syscall,
	  KERNEL_CS,
	  0,				/* no parameters */
	  ACC_P|ACC_PL_U|ACC_CALL_GATE
	},				/* call gate for system calls */
/*00F*/	{ (unsigned int)&mach_rpc,
          KERNEL_CS,
          0,                            /* no parameters */
          ACC_P|ACC_PL_U|ACC_CALL_GATE
        },				/* call gate for mach rpc */
/*017*/	{ 0,
	  (VM_MAX_ADDRESS-VM_MIN_ADDRESS-1)>>12,
	  SZ_32|SZ_G,
	  ACC_P|ACC_PL_U|ACC_CODE_R
	},				/* user code segment */
/*01F*/	{ 0,
	  (VM_MAX_ADDRESS-VM_MIN_ADDRESS-1)>>12,
	  SZ_32|SZ_G,
	  ACC_P|ACC_PL_U|ACC_DATA_W
	},				/* user data segment */
};
