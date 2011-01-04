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
# include <time.h>
#endif

# include <mach.h>
# include <device/device_types.h>
# include <device/device.h>
# include <file_system.h>

# define HFS_BLOCKSZ	512
# define HFS_MAX_FLEN	31
# define HFS_MAX_VLEN	27

#define NODEVLOCKS

#ifndef ENOENT
#define ENOENT FS_NO_ENTRY
#endif
#ifndef ENOTDIR
#define ENOTDIR FS_NOT_DIRECTORY
#endif
#ifndef EISTDIR
#define EISDIR FS_NO_ENTRY
#endif
#ifndef ENOTDIR
#define ENOTDIR FS_NOT_DIRECTORY
#endif
#ifndef EROFS
#define EROFS FS_INVALID_PARAMETER
#endif
#ifndef EEXIST
#define EEXIST FS_INVALID_PARAMETER
#endif
#ifndef ENOSPC
#define ENOSPC FS_INVALID_PARAMETER
#endif
#ifndef ENOTEMPTY
#define ENOTEMPTY FS_INVALID_PARAMETER
#endif
#ifndef ENAMETOOLONG
#define ENAMETOOLONG FS_NAME_TOO_LONG
#endif

#ifndef SEEK_SET
#define SEEK_SET 1
#define SEEK_CUR 2
#define SEEK_END 3
#endif

typedef struct _hfsvol_  hfsvol;
typedef struct _hfsfile_ hfsfile;
typedef struct _hfsdir_  hfsdir;

typedef struct {
  char name[HFS_MAX_VLEN + 1];	/* name of volume */
  int flags;			/* volume flags */
  unsigned long totbytes;	/* total bytes on volume */
  unsigned long freebytes;	/* free bytes on volume */
  time_t crdate;		/* volume creation date */
  time_t mddate;		/* last volume modification date */
} hfsvolent;

typedef struct {
  char name[HFS_MAX_FLEN + 1];	/* catalog name */
  int flags;			/* bit flags */
  long cnid;			/* catalog node id (CNID) */
  long parid;			/* CNID of parent directory */
  time_t crdate;		/* date of creation */
  time_t mddate;		/* date of last modification */
  unsigned long dsize;		/* size of data fork */
  unsigned long rsize;		/* size of resource fork */
  char type[5];			/* file type code (plus null) */
  char creator[5];		/* file creator code (plus null) */
  short fdflags;		/* Macintosh Finder flags */
} hfsdirent;

# define HFS_ISDIR		0x01
# define HFS_ISLOCKED		0x02

# define HFS_CNID_ROOTPAR	1
# define HFS_CNID_ROOTDIR	2
# define HFS_CNID_EXT		3
# define HFS_CNID_CAT		4
# define HFS_CNID_BADALLOC	5

# define HFS_FNDR_ISONDESK		(1 <<  0)
# define HFS_FNDR_COLOR			0x0e
# define HFS_FNDR_COLORRESERVED		(1 <<  4)
# define HFS_FNDR_REQUIRESSWITCHLAUNCH	(1 <<  5)
# define HFS_FNDR_ISSHARED		(1 <<  6)
# define HFS_FNDR_HASNOINITS		(1 <<  7)
# define HFS_FNDR_HASBEENINITED		(1 <<  8)
# define HFS_FNDR_RESERVED		(1 <<  9)
# define HFS_FNDR_HASCUSTOMICON		(1 << 10)
# define HFS_FNDR_ISSTATIONERY		(1 << 11)
# define HFS_FNDR_NAMELOCKED		(1 << 12)
# define HFS_FNDR_HASBUNDLE		(1 << 13)
# define HFS_FNDR_ISINVISIBLE		(1 << 14)
# define HFS_FNDR_ISALIAS		(1 << 15)

extern char *hfs_error;
extern unsigned char hfs_charorder[];

hfsvol *hfs_mount(struct device *);
int hfs_flush(hfsvol *);
void hfs_flushall(void);
int hfs_umount(hfsvol *);
void hfs_umountall(void);
hfsvol *hfs_getvol(char *);
void hfs_setvol(hfsvol *);

int hfs_vstat(hfsvol *, hfsvolent *);
int hfs_format(char *, int, char *);

int hfs_chdir(hfsvol *, char *);
long hfs_getcwd(hfsvol *);
int hfs_setcwd(hfsvol *, long);
int hfs_dirinfo(hfsvol *, long *, char *);

hfsdir *hfs_opendir(hfsvol *, char *);
int hfs_readdir(hfsdir *, hfsdirent *);
int hfs_closedir(hfsdir *);

hfsfile *hfs_open(hfsvol *, char *);
int hfs_setfork(hfsfile *, int);
int hfs_getfork(hfsfile *);
long hfs_read(hfsfile *, void *, unsigned long);
long hfs_write(hfsfile *, void *, unsigned long);
int hfs_truncate(hfsfile *, unsigned long);
long hfs_lseek(hfsfile *, long, int);
int hfs_close(hfsfile *);

int hfs_stat(hfsvol *, char *, hfsdirent *);
int hfs_fstat(hfsfile *, hfsdirent *);
int hfs_setattr(hfsvol *, char *, hfsdirent *);
int hfs_fsetattr(hfsfile *, hfsdirent *);

int hfs_mkdir(hfsvol *, char *);
int hfs_rmdir(hfsvol *, char *);

int hfs_create(hfsvol *, char *, char *, char *);
int hfs_delete(hfsvol *, char *);

int hfs_rename(hfsvol *, char *, char *);

char *mystrchr(char *, char);
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
