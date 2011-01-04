/*
 * linux/fs/hfs/bitops.c
 *
 * Copyright (C) 1996  Paul H. Hargrove
 * This file may be distributed under the terms of the GNU Public License.
 *
 * This file contains functions to handle bitmaps in "left-to-right"
 * bit-order such that the MSB of a 32-bit big-endian word is bit 0.
 * (This corresponds to bit 7 of a 32-bit little-endian word.)
 *
 * I have tested and confirmed that the results are identical on the
 * Intel x86, PowerPC and DEC Alpha processors.
 *
 * "XXX" in a comment is a note to myself to consider changing something.
 */

#include "hfs.h"

/*================ Global functions ================*/

/*
 * hfs_find_first_zero_bit()
 *
 * Description:
 *   Given the address of a block of memory find the first (by
 *   left-to-right ordering) zero bit in the first 'size' bits.
 */
hfs_u32 hfs_find_first_zero_bit(const void * addr, hfs_u32 size)
{
	const hfs_u32 *start = (const hfs_u32 *)addr;
	const hfs_u32 *end   = start + ((size+31) >> 5);
	const hfs_u32 *curr  = start;
	int bit = 0;
	
	while (curr < end) {
		if (*curr == ~((hfs_u32)0)) {
			++curr;
		} else {
			while (hfs_test_bit(bit, curr)) {
			       ++bit;
			}
			break;
		}
	}
	return ((curr-start)<<5) | bit;
}

/*
 * hfs_find_next_zero_bit()
 *
 * Description:
 *   Given the address of a block of memory find the next (by
 *   left-to-right bit-ordering) zero bit in the first 'size' bits.
 */
hfs_u32 hfs_find_next_zero_bit(const void * addr, hfs_u32 size, hfs_u32 offset)
{
	const hfs_u32 *start = (const hfs_u32 *)addr;
	const hfs_u32 *end   = start + ((size+31) >> 5);
	const hfs_u32 *curr  = start + (offset>>5);
	int bit = offset % 32;
	
	if (size < offset) {
		return size;
	}

	/* finish up first partial hfs_u32 */
	if (bit != 0) {
		do {
			if (!hfs_test_bit(bit, curr)) {
				goto done;
			}
			++bit;
		} while (bit < 32);
		bit = 0;
		++curr;
	}
	
	/* skip fully set hfs_u32 */
	while (*curr == ~((hfs_u32)0)) {
		if (curr == end) {
			goto done;
		} else {
			++curr;
		}
	}

	/* check final partial hfs_u32 */
	do {
		if (!hfs_test_bit(bit, curr)) {
			goto done;
		}
		++bit;
	} while (bit<32);

done:
	return ((curr-start)<<5) | bit;
}

/*
 * hfs_count_zero_bits()
 *
 * Description:
 *  Given a block of memory, its length in bits, and a starting bit number,
 *  determine the number of consecutive zero bits (in left-to-right ordering)
 *  starting at that location, up to 'max'.
 */
hfs_u32 hfs_count_zero_bits(const void *addr, hfs_u32 size,
			    hfs_u32 offset, hfs_u32 max)
{
	const hfs_u32 *curr = (const hfs_u32 *)addr + (offset>>5);
	const hfs_u32 *end = (const hfs_u32 *)addr + ((size+31)>>5);
	int bit = offset % 32;
	hfs_u32 count = 0;

	/* fix up max to avoid running off end */
	if (max > (size - offset)) {
		max = size - offset;
	}

	/* finish up first partial hfs_u32 */
	if (bit != 0) {
		do {
			if (hfs_test_bit(bit, curr)) {
				goto done;
			}
			++count;
			++bit;
		} while (bit<32);
		++curr;
	}
	
	/* eat up any full hfs_u32s of zeros */
	if (size > (offset + max)) {
		size = offset + max;
	}
	while (*curr == ((hfs_u32)0)) {
		if (curr == end) {
			goto done;
		} else {
			++curr;
			count += 32;
		}
	}
	
	/* count bits in the last hfs_u32 */
	bit = 0;
	do {
		if (hfs_test_bit(bit, curr)) {
			goto done;
		}
		++count;
		++bit;
	} while (bit<32);

done:
	if (count > max) {
		count = max;
	}
	return count;
}
