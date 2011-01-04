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
 * Revision 2.6  91/10/07  17:25:00  af
 * 	Removed strlen(), which is now MI.
 * 	[91/10/05            af]
 * 
 * Revision 2.5  91/08/24  11:56:09  af
 * 	Added delay() function, for where too much optim hurts.
 * 	[91/08/02  02:47:21  af]
 * 
 * Revision 2.4  91/05/14  16:11:30  mrt
 * 	Correcting copyright
 * 
 * Revision 2.3  91/02/05  17:13:06  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:35:56  mrt]
 * 
 * Revision 2.2  90/05/03  15:33:52  dbg
 * 	Added strlen.  Removed slave_config.
 * 	[90/02/15            dbg]
 * 
 * Revision 1.7.1.1  89/10/22  11:31:00  rvb
 * 	gets() now in swapgeneric.c
 * 	[89/10/17            rvb]
 * 
 * Revision 1.7  89/09/20  17:26:29  rvb
 * 	Back to zero.  All this will be unnecessary when
 * 	orr's boot parsing gets installed.
 * 	[89/09/20            rvb]
 * 
 * Revision 1.6  89/09/09  15:19:45  rvb
 * 	boot how to -> 3
 * 	[89/09/07            rvb]
 * 
 * Revision 1.5  89/08/08  21:46:32  jsb
 * 	Set boothowto to 0.
 * 	[89/08/03            rvb]
 * 
 * Revision 1.4  89/04/05  12:58:37  rvb
 * 	Add kupfer's ovbcopy, till we recode it.
 * 	[89/04/03            rvb]
 * 
 * 	Temporarily need memcpy for gcc.
 * 	[89/03/06            rvb]
 * 
 * Revision 1.3  89/02/26  12:35:51  gm0w
 * 	Changes for cleanup.
 * 
 * 16-Feb-89  Robert Baron (rvb) at Carnegie-Mellon University
 *	Created
 *
 */ 
/* CMU_ENDHIST */
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
#include <mach_assert.h>

#include <string.h>
#include <mach/boolean.h>
#include <mach/i386/vm_types.h>
#include <kern/kern_types.h>
#include <kern/misc_protos.h>
#include <i386/misc_protos.h>

	/*
	 * Should be rewritten in asm anyway.
	 */
/* 
 * ovbcopy - like bcopy, but recognizes overlapping ranges and handles 
 *           them correctly.
 */

void
ovbcopy(
	const char	*from,
	char		*to,
	vm_size_t	bytes)		/* num bytes to copy */
{
	/* Assume that bcopy copies left-to-right (low addr first). */
	if (from + bytes <= to || to + bytes <= from || to == from)
		bcopy(from, to, bytes);	/* non-overlapping or no-op*/
	else if (from > to)
		bcopy(from, to, bytes);	/* overlapping but OK */
	else {
		/* to > from: overlapping, and must copy right-to-left. */
		from += bytes - 1;
		to += bytes - 1;
		while (bytes-- > 0)
			*to-- = *from--;
	}
}

int bcmp(
	const char	*a,
	const char	*b,
	vm_size_t	len)
{
	if (len == 0)
		return 0;

	do
		if (*a++ != *b++)
			break;
	while (--len);

	return len;
}

#if	MACH_ASSERT

/*
 * Machine-dependent routine to fill in an array with up to callstack_max
 * levels of return pc information.
 */
void machine_callstack(
	natural_t	*buf,
	vm_size_t	callstack_max)
{
}

#endif	/* MACH_ASSERT */
