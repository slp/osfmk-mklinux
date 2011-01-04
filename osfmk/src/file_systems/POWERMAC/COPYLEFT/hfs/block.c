/*
 * hfsutils - tools for reading and writing Macintosh HFS volumes
 * Copyright (C) 1996, 1997 Robert Leslie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if 0
# include <unistd.h>
# include <time.h>
#endif
# include <types.h>
# include <errno.h>

# include "hfs/internal.h"
# include "hfs/data.h"
# include "hfs/block.h"
# include "hfs/low.h"

#define 	HFS_CACHE_SIZE	(64*1024)

/*
 * NAME:	block->readlb()
 * DESCRIPTION:	read a logical block from a volume
 */
int b_readlb(hfsvol *vol, unsigned long num, block *bp)
{
  int bytes;
  vm_offset_t data;
  mach_msg_type_number_t retcount;
  int fc;
  unsigned long blknum, offset, adjblk;

#if 0
  if (lseek(vol->fd, (vol->vstart + num) * HFS_BLOCKSZ, SEEK_SET) < 0)
    {
      ERROR(errno, "error seeking device");
      return -1;
    }
  bytes = read(vol->fd, bp, HFS_BLOCKSZ);
#endif

  /* use mach for device access */

  blknum = (vol->vstart + num );

again:
  if (blknum >= vol->cache_start && blknum < vol->cache_start + vol->cache_size) {
	memcpy((char *) bp,
		vol->cache_data+((blknum - vol->cache_start) * HFS_BLOCKSZ),
		HFS_BLOCKSZ);
	return 0;
  }

  adjblk = ((blknum * HFS_BLOCKSZ) / vol->f_dev.rec_size);

  fc = device_read(vol->f_dev.dev_port, 0,
		(recnum_t)adjblk,
	      HFS_CACHE_SIZE, (char **)&data, &retcount);

  if (fc == KERN_SUCCESS) {
	if (vol->cache_data)
		vm_deallocate(mach_task_self(), (vm_offset_t) vol->cache_data,
				vol->cache_size * HFS_BLOCKSZ);
	vol->cache_data = (char *)data;
	vol->cache_start = (adjblk * vol->f_dev.rec_size) / HFS_BLOCKSZ;
	vol->cache_size = retcount / HFS_BLOCKSZ;
	goto again;
  }

  /* Fall back to reading a single block.. */


  fc = device_read(vol->f_dev.dev_port, 0, (recnum_t)adjblk, 
	      HFS_BLOCKSZ < vol->f_dev.rec_size ? vol->f_dev.rec_size : HFS_BLOCKSZ, (char **)&data, &retcount);


  if (fc) 
    return -1;

  /* copy to bp & free the mach allocation */

  offset = (blknum * HFS_BLOCKSZ) - (adjblk * vol->f_dev.rec_size);
  memcpy((char *)bp, ((char *)data)+offset,
		retcount > HFS_BLOCKSZ ? HFS_BLOCKSZ : (unsigned) retcount);

  (void) vm_deallocate(mach_task_self(), data, retcount);

  bytes = retcount;

  if (bytes < 0)
    {
      ERROR(errno, "error reading from device");
      return -1;
    }
  else if (bytes == 0)
    {
      ERROR(EIO, "read EOF on volume");
      return -1;
    }
  else if (bytes != HFS_BLOCKSZ)
    {
      ERROR(EIO, "read incomplete block");
      return -1;
    }

  return 0;
}

/*
 * NAME:	block->writelb()
 * DESCRIPTION:	write a logical block to a volume
 */
int b_writelb(hfsvol *vol, unsigned long num, block *bp)
{
  return 0;
}

/*
 * NAME:	block->readab()
 * DESCRIPTION:	read a block from an allocation block from a volume
 */
int b_readab(hfsvol *vol,
	     unsigned int anum, unsigned int index, block *bp)
{
  /* verify the allocation block exists and is marked as in-use */

  if (anum >= vol->mdb.drNmAlBlks)
    {
      ERROR(EIO, "read nonexistent block");
      return -1;
    }
  else if (vol->vbm && ! BMTST(vol->vbm, anum))
    {
      ERROR(EIO, "read unallocated block");
      return -1;
    }

  return b_readlb(vol, vol->mdb.drAlBlSt + anum * vol->lpa + index, bp);
}

/*
 * NAME:	b->writeab()
 * DESCRIPTION:	write a block to an allocation block to a volume
 */
int b_writeab(hfsvol *vol,
	      unsigned int anum, unsigned int index, block *bp)
{
  /* verify the allocation block exists and is marked as in-use */

  if (anum >= vol->mdb.drNmAlBlks)
    {
      ERROR(EIO, "write nonexistent block");
      return -1;
    }
  else if (vol->vbm && ! BMTST(vol->vbm, anum))
    {
      ERROR(EIO, "write unallocated block");
      return -1;
    }

  vol->mdb.drAtrb &= ~HFS_ATRB_UMOUNTED;
  vol->mdb.drLsMod = d_tomtime(/*time*/(0));
  ++vol->mdb.drWrCnt;

  vol->flags |= HFS_UPDATE_MDB;

  return b_writelb(vol, vol->mdb.drAlBlSt + anum * vol->lpa + index, bp);
}
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
