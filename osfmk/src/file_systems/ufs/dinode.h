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
 * Copyright (c) 1982, 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)dinode.h	7.6 (Berkeley) 10/24/89
 */

#ifndef	_UFS_DINODE_H_
#define	_UFS_DINODE_H_

#if 0
#include <sys/secdefines.h>
#endif
#if	SEC_FSCHANGE
#include <sys/security.h>
#endif

/*
 * This structure defines the on-disk format of an inode.
 *
 * Refer to ../sys/inode.h for an explanation of locking constraints
 * when the inode is in-core.
 */

#define	NDADDR	12		/* direct addresses in inode */
#define	NIADDR	3		/* indirect addresses in inode */

#define MAX_FASTLINK_SIZE	((NDADDR + NIADDR) * sizeof (daddr_t))

struct dinode {
	u_bit16_t	di_mode;	/*  0: mode and type of file */
	bit16_t		di_nlink;	/*  2: number of links to file */
	u_bit16_t	di_uid;		/*  4: owner's user id */
	u_bit16_t	di_gid;		/*  6: owner's group id */
	u_bit64_t	di_qsize;	/*  8: number of bytes in file */
	u_bit32_t	di_atime;	/* 16: time last accessed */
	u_bit32_t	di_atspare;
	u_bit32_t	di_mtime;	/* 24: time last modified */
	u_bit32_t	di_mtspare;
	u_bit32_t	di_ctime;	/* 32: last time inode changed */
	u_bit32_t	di_ctspare;
	union {
	    struct {
		daddr_t	Mb_db[NDADDR];	/* 40: disk block addresses*/
		daddr_t	Mb_ib[NIADDR];	/* 88: indirect blocks */
	    } di_Mb;
#define di_db	di_Mun.di_Mb.Mb_db
#define di_ib	di_Mun.di_Mb.Mb_ib
	    char	di_Msymlink[MAX_FASTLINK_SIZE];
					/* 40: symbolic link name */
	} di_Mun;
#define di_symlink	di_Mun.di_Msymlink
	u_bit32_t	di_flags;	/* 100: status */
#define IC_FASTLINK	0x0001		/* Symbolic link in inode */
	u_bit32_t	di_blocks;	/* 104: blocks actually held */
	u_bit32_t	di_gen;		/* 108: generation number */
	u_bit32_t	di_spare[4];	/* 112: reserved, currently unused */
};

#if	SEC_FSCHANGE

/*
 * Security extensions to the on-disk inode format:
 */
struct dinode_sec {
	priv_t  di_gpriv[2];    /* granted privilege vector */
	priv_t  di_ppriv[2];    /* potential privilege vector */
	tag_t   di_tag[SEC_TAG_COUNT];  /* security policy tags */
	ino_t   di_parent;      /* inode number of parent of MLD child */
	u_bit16_t di_type_flags;/* type flags (MLD, 2 person rule, etc.) */
};

/*
 * On-disk inode format for a secure filesystem:
 */
struct sec_dinode {
	struct dinode           di_node;
	struct dinode_sec       di_sec;
};
#endif	/* SEC_FSCHANGE */

#define	di_size		di_qsize._SIG64_BITS	/* see machine/types.h */
#define	di_rdev		di_db[0]

/* file modes */
#define	IFMT		0170000		/* type of file */
#define	IFIFO		0010000		/* fifo (named pipe) */
#define	IFCHR		0020000		/* character special */
#define	IFDIR		0040000		/* directory */
#define	IFBLK		0060000		/* block special */
#define	IFREG		0100000		/* regular */
#define	IFLNK		0120000		/* symbolic link */
#define	IFSOCK		0140000		/* socket */

#define	ISUID		04000		/* set user id on execution */
#define	ISGID		02000		/* set group id on execution */
#define	ISVTX		01000		/* save swapped text even after use */
#define	IREAD		0400		/* read, write, execute permissions */
#define	IWRITE		0200
#define	IEXEC		0100
#endif	/* _UFS_DINODE_H_ */
