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
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */
#ifndef _MAC_LABEL_H_
#define _MAC_LABEL_H_

#include <types.h>

/*
 * APPLE_UNIX_SVR2 partition information
 */

#define MAC_PART_UNIX_SVR2_TYPE "Apple_UNIX_SVR2"
#define MAC_PART_UNIX_SVR2_ROOT "A/UX Root"
#define MAC_PART_UNIX_SVR2_SWAP "Swap"

struct mac_unix_bargs {
	u_int	uxMagic;
	u_char	uxCluster;
	u_char	uxType;
	u_short	uxBadBlocKInode;
	u_short	uxFlags;
	u_short	uxReserved;
	u_int	uxCreationTime;
	u_int	uxMountTime;
	u_int	uxUMountTime;
};
#define MAC_UNIX_BARGS_MAGIC 0xaBadBabe

/* TODO NMGS revise this structure and these values */
#define MAC_UNIX_BARGS_FLAGS_ROOTFS 0x8000
#define MAC_UNIX_BARGS_FLAGS_USRFS  0x4000
#define MAC_UNIX_BARGS_FLAGS_SWAP   0x0003
/*
 * Partition table entry
 */

struct mac_label {
	u_short	pmSig;
	u_short pmSigPad;
	u_int	pmMapBlkCnt;
	u_int	pmPyPartStart;
	u_int	pmPartBlkCnt;
	char	pmPartName[32];
	char	pmPartType[32];
	u_int	pmLgDataStart;
	u_int	pmDataCnt;
	u_int	pmPartStatus;
	u_int	pmLgBootStart;
	u_int	pmBootSize;
	u_int	pmBootLoad;
	u_int	pmBootLoad2;
	u_int	pmBootEntry;
	u_int	pmBootEntry2;
	u_int	pmBootCksum;
	char	pmProcessor[16];
	union	{
		struct mac_unix_bargs unix_bargs;
		char	buffer[128];
	} pmBootArgs;

	char	pad[248];
};

/* Address of mac partition block, in blocks */

#define MAC_PARTITION_BLOCK_OFFSET 1

/*
 * Mac disk partition map magic number.  Valid entries have this
 * in the pmSig field.
 */
#define MAC_PARTITION_MAGIC	0x504d


/* Driver descriptor structure at start of disk */
struct mac_driver_descriptor {
	u_short	sbSig;
	u_short	sbBlockSize;
	u_int	sbBlkCount;
	u_short	sbDevType;
	u_short	sbDevId;
	u_int	sbData;
	u_short	sbDrvrCount;
	u_int	ddBlock;
	u_short	ddSize;
	u_short	ddType;
};
#define MAC_DRIVER_DESCRIPTOR_OFFSET 0
#define MAC_DRIVER_DESCRIPTOR_MAGIC 0x4552

#endif /* _MAC_LABEL_H_ */
