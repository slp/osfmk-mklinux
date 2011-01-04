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
 * Revision 2.1.2.1  92/04/30  11:54:25  bernadat
 * 	Fixed code to use mk includes only (not sys/dir.h ....).
 * 	Let the kernel name be optional in case user specifies unit to boot
 * 	Change args parsing to detect server name ans server args
 * 	New syntax: hd(0,a)/kernel:server [-h|-s|-d|-a|-<digit>] <server args>
 * 	Add boot options to be compatible with old boot -h, -<digit> (ncpus)
 * 	Copied from main line
 * 	[92/03/19            bernadat]
 * 
 * Revision 2.2  92/04/04  11:36:34  rpd
 * 	Fabricated from 3.0 bootstrap and scratch.
 * 	[92/03/30            mg32]
 * 
 */
/* CMU_ENDHIST */
/*
 * Mach Operating System
 * Copyright (c) 1992, 1991 Carnegie Mellon University
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

#include "boot.h"
#include <dirent.h>
#include <sys/reboot.h>

struct inode inode;
static char fsbuf[SBSIZE];
static char iobuf[MAXBSIZE];

static int block_map(int);
static int find(const char *);
static int ioread(void *, int, int);

int
xread(void *addr, int count)
{
	return(ioread(addr, count, 1));
}

int
read(void *addr, int count)
{
	return(ioread(addr, count, 0));
}

static int
ioread(void *addr, int count, int phys)
{
	int logno, off, size;

	while (count) {
		off = blkoff(fs, poff);
		logno = lblkno(fs, poff);
		cnt = size = blksize(fs, &inode, logno);
		bnum = block_map(logno);
		if (bnum == -1)
			return(1);
		bnum = fsbtodb(fs, bnum) + boff;
		size -= off;
		if (size > count)
			size = count;
		if (disk_read(bnum, cnt, (vm_offset_t)iobuf))
			return(1);
		if (phys)
			pcpy(iobuf+off,addr,size);
		else
			bcopy(iobuf+off,addr,size);
		addr = (char *)addr + size;
		count -= size;
		poff += size;
	}
	return(0);
}

int
find(const char *path)
{
	const char *rest;
	int ch;
	int block, off, loc, ino = ROOTINO;
	struct dirent *dp;

	for (;;) {
		cnt = fs->fs_bsize;
		bnum = fsbtodb(fs,itod(fs,ino)) + boff;
		if (disk_read(bnum, cnt, (vm_offset_t)iobuf))
			return 0;
		bcopy((char *)&((struct dinode *)iobuf)[ino % fs->fs_inopb],
		      (char *)&inode.i_di,
		      sizeof (struct dinode));
		if (!*path)
			return 1;
		while (*path == '/')
			path++;
		if (!inode.i_size || ((inode.i_mode&IFMT) != IFDIR))
			return 0;
		for (rest = path; (ch = *rest) && ch != '/'; rest++) ;
		loc = 0;
		for (;;) {
			if (loc >= inode.i_size)
				return 0;
			if (!(off = blkoff(fs, loc))) {
				block = lblkno(fs, loc);
				cnt = blksize(fs, &inode, block);
				bnum = fsbtodb(fs, block_map(block)) + boff;
				if (disk_read(bnum, cnt, (vm_offset_t)iobuf))
					return 0;
			}
			dp = (struct dirent *)(iobuf + off);
			loc += dp->d_reclen;
			if (dp->d_ino == 0)
				continue;
			if (strncmp(path, dp->d_name, rest - path) == 0 &&
			    dp->d_name[rest - path] == 0)
				break;
		}
		ino = dp->d_ino;
		path = rest;
	}
}

static char mapbuf[MAXBSIZE];
static int mapblock;

int
block_map(int file_block)
{
	if (file_block < NDADDR)
		return(inode.i_db[file_block]);
	if ((bnum=fsbtodb(fs, inode.i_ib[0])+boff) != mapblock) {
		cnt = fs->fs_bsize;
		if (disk_read(bnum, cnt, (vm_offset_t)mapbuf))
			return(-1);
		mapblock = bnum;
	}
	return (((int *)mapbuf)[(file_block - NDADDR) % NINDIR(fs)]);
}

int
openrd(const char *name, const char **basename)
{
	const char *cp = name;

	while (*cp && *cp!='(')
		cp++;
	if (!*cp)
		cp = name;
	else {
		if (cp++ != name) {
			if (name[1] != 'd' ||
			    (name[0] != 'h' && name[0] != 'f' && name[0] != 's')) {
				printf("Unknown device\n");
				return 1;
			}
			dev = name[0];
		}
		if (*cp >= '0' && *cp <= '9')
			if ((unit = *cp++ - '0') > 1) {
				printf("Bad unit\n");
				return 1;
			}
		if (!*cp || (*cp == ',' && !*++cp))
			return 1;
		if (*cp >= 'a' && *cp <= 'p')
			part = *cp++ - 'a';
		while (*cp && *cp++!=')') ;
	}
	if (disk_open()) {
	  	printf("Can't open device.\n");
		return 1;
	}
	cnt = SBSIZE;
	bnum = SBLOCK + boff;
	if (disk_read(bnum, cnt, (vm_offset_t)fsbuf)) {
		printf("Cant read super block\n");
		return 1;
	}
	fs = (struct fs *)fsbuf;
	if (fs->fs_magic != FS_MAGIC) {
		printf("bad FS_MAGIC\n");
		return 1;
	}
	if (!*cp || *cp == ' ' || !find(cp))
		return 1;
	poff = 0;
	if (basename)
		*basename = cp;
	return 0;
}

int
devfs(void)
{
	if (disk_open()) {
	  	printf("Can't open device.\n");
		return 1;
	}
	cnt = SBSIZE;
	bnum = SBLOCK + boff;
	if (disk_read(bnum, cnt, (vm_offset_t)fsbuf)) {
		printf("Cant read super block\n");
		return 1;
	}
	fs = (struct fs *)fsbuf;
	return (fs->fs_magic != FS_MAGIC);
}

