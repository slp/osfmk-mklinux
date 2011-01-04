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
 * C version of bit manipulation routines now required by kernel.
 * Should be replaced with assembler versions in any real port.
 *
 * Note that these routines use little-endian numbering for bits (i.e.,
 * the bit number corresponds to the associated power-of-2).
 */
#include <mach/machine/vm_param.h>	/* for BYTE_SIZE */

#define INT_SIZE	(BYTE_SIZE * sizeof (int))

/*
 * Set indicated bit in bit string.
 */
void
setbit(int bitno, int *s)
{
	for ( ; INT_SIZE <= bitno; bitno -= INT_SIZE, ++s)
		;
	*s |= 1 << bitno;
}

/*
 * Clear indicated bit in bit string.
 */
void
clrbit(int bitno, int *s)
{
	for ( ; INT_SIZE <= bitno; bitno -= INT_SIZE, ++s)
		;
	*s &= ~(1 << bitno);
}

/*
 * Find first bit set in bit string.
 */
int
ffsbit(int *s)
{
	int offset, mask;

	for (offset = 0; !*s; offset += INT_SIZE, ++s)
		;
	for (mask = 1; mask; mask <<= 1, ++offset)
		if (mask & *s)
			return (offset);
	/*
	 * Shouldn't get here
	 */
	return (0);
}
