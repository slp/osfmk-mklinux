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
 * Compatibility with hpux file systems.
 * All defines that differ from those of osf1 have been renamed
 * with a HPUX_ or hpux_ prefix.
 */

/*
 * @(#)fs.h: $Revision: 1.1.2.2 $ $Date: 1996/10/07 07:19:48 $
 * $Locker:  $
 * 
 */

/* @(#) $Revision: 1.1.2.2 $ */     
#ifndef _SYS_FS_INCLUDED /* allows multiple inclusion */
#define _SYS_FS_INCLUDED
/*
 * Each disk drive contains some number of file systems.
 * A file system consists of a number of cylinder groups.
 * Each cylinder group has inodes and data.
 *
 * A file system is described by its super-block, which in turn
 * describes the cylinder groups.  The super-block is critical
 * data and is replicated in each cylinder group to protect against
 * catastrophic loss.  This is done at mkfs time and the critical
 * super-block data does not change, so the copies need not be
 * referenced further unless disaster strikes.
 *
 * For file system fs, the offsets of the various blocks of interest
 * are given in the super block as:
 *	[fs->fs_sblkno]		Super-block
 *	[fs->fs_cblkno]		Cylinder group block
 *	[fs->fs_iblkno]		Inode blocks
 *	[fs->fs_dblkno]		Data blocks
 * The beginning of cylinder group cg in fs, is given by
 * the ``cgbase(fs, cg)'' macro.
 *
 * The first boot and super blocks are given in absolute disk addresses.
 */

#include <types.h>

#ifdef	OSF
#define HPUX_BBSIZE	8192
#define	HPUX_BBLOCK	((daddr_t)(0))
#define	HPUX_SBLOCK	((daddr_t)(HPUX_BBLOCK + HPUX_BBSIZE / DEV_BSIZE))
#else	/* OSF */
#define BBSIZE		8192
#define SBSIZE		8192
#define	BBLOCK		((daddr_t)(0))
#define	SBLOCK		((daddr_t)(BBLOCK + BBSIZE / DEV_BSIZE))
#endif	/* OSF */

/*
 * Addresses stored in inodes are capable of addressing fragments
 * of `blocks'. File system blocks of at most size MAXBSIZE can 
 * be optionally broken into 2, 4, or 8 pieces, each of which is
 * addressible; these pieces may be DEV_BSIZE, or some multiple of
 * a DEV_BSIZE unit.
 *
 * Large files consist of exclusively large data blocks.  To avoid
 * undue wasted disk space, the last data block of a small file may be
 * allocated as only as many fragments of a large block as are
 * necessary.  The file system format retains only a single pointer
 * to such a fragment, which is a piece of a single large block that
 * has been divided.  The size of such a fragment is determinable from
 * information in the inode, using the ``blksize(fs, ip, lbn)'' macro.
 *
 * The file system records space availability at the fragment level;
 * to determine block availability, aligned fragments are examined.
 *
 */

/*
 * Cylinder group related limits.
 *
 * For each cylinder we keep track of the availability of blocks at different
 * rotational positions, so that we can lay out the data to be picked
 * up with minimum rotational latency.  NRPOS is the number of rotational
 * positions which we distinguish.  With NRPOS 8 the resolution of our
 * summary information is 2ms for a typical 3600 rpm drive.
 */
#define	NRPOS		8	/* number distinct rotational positions */

/*
 * MAXIPG bounds the number of inodes per cylinder group, and
 * is needed only to keep the structure simpler by having the
 * only a single variable size element (the free bit map).
 *
 * N.B.: MAXIPG must be a multiple of INOPB(fs).
 */
#define	MAXIPG		2048	/* max number inodes/cyl group */

/*
 * MINBSIZE is the smallest allowable block size.
 * In order to insure that it is possible to create files of size
 * 2^32 with only two levels of indirection, MINBSIZE is set to 4096.
 * MINBSIZE must be big enough to hold a cylinder group block,
 * thus changes to (struct cg) must keep its size within MINBSIZE.
 * MAXCPG is limited only to dimension an array in (struct cg);
 * it can be made larger as long as that structures size remains
 * within the bounds dictated by MINBSIZE.
 * Note that super blocks are always of size MAXBSIZE,
 * and that MAXBSIZE must be >= MINBSIZE.
 */
#ifndef	OSF
#define MINBSIZE	4096
#endif	/* OSF */
#define	MAXCPG		32	/* maximum fs_cpg */

/* MAXFRAG is the maximum number of fragments per block */
#ifndef	OSF
#define MAXFRAG		8
#endif	/* OSF */

#ifndef	NBBY
#define NBBY		8	/* number of bits in a byte	*/
				/* NOTE: this is also defined	*/
				/* in param.h.  So if NBBY gets	*/
				/* changed, change it in	*/
				/* param.h also			*/
#endif

/*
 * The path name on which the file system is mounted is maintained
 * in fs_fsmnt. MAXMNTLEN defines the amount of space allocated in 
 * the super block for this name.
 * The limit on the amount of summary information per file system
 * is defined by MAXCSBUFS. It is currently parameterized for a
 * maximum of two million cylinders.
 */
#ifndef	OSF
#define MAXMNTLEN 512
#define MAXCSBUFS 32

/*
 * Per cylinder group information; summarized in blocks allocated
 * from first cylinder group data blocks.  These blocks have to be
 * read in from fs_csaddr (size fs_cssize) in addition to the
 * super block.
 *
 * N.B. sizeof(struct csum) must be a power of two in order for
 * the ``fs_cs'' macro to work (see below).
 */
struct csum {
	long	cs_ndir;	/* number of directories */
	long	cs_nbfree;	/* number of free blocks */
	long	cs_nifree;	/* number of free inodes */
	long	cs_nffree;	/* number of free frags */
};

#endif	/* OSF */

/*
 * Super block for a file system.
 */
#ifndef	OSF
#define	FS_MAGIC	0x011954
#endif	/* OSF */

/*
 * Magic number for file system allowing long file names.
 */
#define	FS_MAGIC_LFN	0x095014
#ifdef	OSF
#define	HPUX_FS_MAGIC	FS_MAGIC_LFN	
#endif	/* OSF */

/*
 * Magic number for file systems which have their fs_featurebits field
 * set up.
 */
#define FD_FSMAGIC	0x195612

/*
 * Flags for fs_featurebits field.
 */
#define FSF_LFN		0x1	/* long file names */
#define FSF_KNOWN	(FSF_LFN)
#define FSF_UNKNOWN(bits) ((bits) & ~(FSF_KNOWN))

/*
 * Quick check to see if inode is in a file system allowing
 * long file names.
 */
#define IS_LFN_FS(ip) \
    (((ip)->i_fs->fs_magic == FS_MAGIC_LFN) || \
    ((ip)->i_fs->fs_featurebits & FSF_LFN))

#ifdef	OSF
#define	HPUX_FS_CLEAN	0x17
#else	/* OSF */
#define	FS_CLEAN	0x17
#endif	/* OSF */
#define	FS_OK		0x53
#define	FS_NOTOK	0x31

/* fs_flags fields */
#define FS_INSTALL 	0x80
#define FS_QCLEAN 	0x01
#define FS_QOK	 	0x02
#define FS_QNOTOK 	0x03
#define FS_QMASK 	0x03
#define FS_QFLAG(p)	((p)->fs_flags & FS_QMASK)
#define FS_QSET(p,val)	((p)->fs_flags &= ~FS_QMASK, (p)->fs_flags |= (val))

/* Mirstate describes the mirror states of the root and primary swap */
/* devices.  This information is only recorded in the super block of */
/* the root file system.  If root and swap devices are mirrored, the */
/* bootup code will configure their states based on mirstate.        */

struct mirinfo {
        struct mirstate {               /* mirror states for root and swap */
                u_int   root:4,         /* root mirror states */
                        rflag:1,        /* root clean/unconf flag */
                        swap:4,         /* swap mirror states */
                        sflag:1,        /* swap clean/unconf flag */
                        spare:22;       /* spare bits */
        } state;
        long mirtime;                   /* mirror time stamp */
};

#ifdef	OSF
struct	hpux_fs
#else	/* OSF */
struct	fs
#endif	/* OSF */
{
	struct	fs *fs_link;		/* linked list of file systems */
	struct	fs *fs_rlink;		/*     used for incore super blocks */
	daddr_t	fs_sblkno;		/* addr of super-block in filesys */
	daddr_t	fs_cblkno;		/* offset of cyl-block in filesys */
	daddr_t	fs_iblkno;		/* offset of inode-blocks in filesys */
	daddr_t	fs_dblkno;		/* offset of first data after cg */
	long	fs_cgoffset;		/* cylinder group offset in cylinder */
	long	fs_cgmask;		/* used to calc mod fs_ntrak */
	time_t 	fs_time;    		/* last time written */
	long	fs_size;		/* number of blocks in fs */
	long	fs_dsize;		/* number of data blocks in fs */
	long	fs_ncg;			/* number of cylinder groups */
	long	fs_bsize;		/* size of basic blocks in fs */
	long	fs_fsize;		/* size of frag blocks in fs */
	long	fs_frag;		/* number of frags in a block in fs */
/* these are configuration parameters */
	long	fs_minfree;		/* minimum percentage of free blocks */
	long	fs_rotdelay;		/* num of ms for optimal next block */
	long	fs_rps;			/* disk revolutions per second */
/* these fields can be computed from the others */
	long	fs_bmask;		/* ``blkoff'' calc of blk offsets */
	long	fs_fmask;		/* ``fragoff'' calc of frag offsets */
	long	fs_bshift;		/* ``lblkno'' calc of logical blkno */
	long	fs_fshift;		/* ``numfrags'' calc number of frags */
/* these are configuration parameters */
	long	fs_maxcontig;		/* max number of contiguous blks */
	long	fs_maxbpg;		/* max number of blks per cyl group */
/* these fields can be computed from the others */
	long	fs_fragshift;		/* block to frag shift */
	long	fs_fsbtodb;		/* fsbtodb and dbtofsb shift constant */
	long	fs_sbsize;		/* actual size of super block */
	long	fs_csmask;		/* csum block offset */
	long	fs_csshift;		/* csum block number */
	long	fs_nindir;		/* value of NINDIR */
	long	fs_inopb;		/* value of INOPB */
	long	fs_nspf;		/* value of NSPF */
	long	fs_id[2];		/* file system id */
	struct  mirinfo fs_mirror;      /* mirror states of root/swap */
	long	fs_featurebits;		/* feature bit flags */
	long	fs_optim;		/* optimization preference - see below */
/* sizes determined by number of cylinder groups and their sizes */
	daddr_t fs_csaddr;		/* blk addr of cyl grp summary area */
	long	fs_cssize;		/* size of cyl grp summary area */
	long	fs_cgsize;		/* cylinder group size */
/* these fields should be derived from the hardware */
	long	fs_ntrak;		/* tracks per cylinder */
	long	fs_nsect;		/* sectors per track */
	long  	fs_spc;   		/* sectors per cylinder */
/* this comes from the disk driver partitioning */
	long	fs_ncyl;   		/* cylinders in file system */
/* these fields can be computed from the others */
	long	fs_cpg;			/* cylinders per group */
	long	fs_ipg;			/* inodes per group */
	long	fs_fpg;			/* blocks per group * fs_frag */
/* this data must be re-computed after crashes */
	struct	csum fs_cstotal;	/* cylinder summary information */
/* these fields are cleared at mount time */
	char   	fs_fmod;    		/* super block modified flag */
	char   	fs_clean;    		/* file system is clean flag */
	char   	fs_ronly;   		/* mounted read-only flag */
	char   	fs_flags;   		/* currently unused flag */
	char	fs_fsmnt[MAXMNTLEN];	/* name mounted on */
/* these fields retain the current block allocation info */
	long	fs_cgrotor;		/* last cg searched */
/* Dummy union for compatibility with OSF/1 ufs/fs.h, which does:
   "#define fs_csp fs_u.fsu_s.fsu_csp".  Our union/struct allows us to
   continue to refer to fs->fs_csp.  */
	union {
		struct {
			struct	csum *fsu_csp[MAXCSBUFS];
			/* list of fs_cs info buffers */
		} fsu_s;
	} fs_u;
	long	fs_cpc;			/* cyl per cycle in postbl */
	short	fs_postbl[MAXCPG][NRPOS];/* head of blocks for each rotation */
	long	fs_magic;		/* magic number */
	char	fs_fname[6];		/* file system name */
	char	fs_fpack[6];		/* file system pack name */
	u_char	fs_rotbl[1];		/* list of blocks for each rotation */
/* actually longer */
};
/*
 * Preference for optimization.
 */
#ifndef	OSF
#define FS_OPTTIME	0	/* minimize allocation time */
#define FS_OPTSPACE	1	/* minimize disk fragmentation */

/*
 * Convert cylinder group to base address of its global summary info.
 *
 * N.B. This macro assumes that sizeof(struct csum) is a power of two.
 */
#define fs_cs(fs, indx) \
	fs_csp[(indx) >> (fs)->fs_csshift][(indx) & ~(fs)->fs_csmask]

#endif	/* OSF */

/*
 * MAXBPC bounds the size of the rotational layout tables and
 * is limited by the fact that the super block is of size SBSIZE.
 * The size of these tables is INVERSELY proportional to the block
 * size of the file system. It is aggravated by sector sizes that
 * are not powers of two, as this increases the number of cylinders
 * included before the rotational pattern repeats (fs_cpc).
 * Its size is derived from the number of bytes remaining in (struct fs)
 */
#define	MAXBPC	(SBSIZE - sizeof (struct fs))

#ifndef	OSF
/*
 * Cylinder group block for a file system.
 */
#define	CG_MAGIC	0x090255
struct	cg
{
	struct	cg *cg_link;		/* linked list of cyl groups */
	struct	cg *cg_rlink;		/*     used for incore cyl groups */
	time_t	cg_time;		/* time last written */
	long	cg_cgx;			/* we are the cgx'th cylinder group */
	short	cg_ncyl;		/* number of cyl's this cg */
	short	cg_niblk;		/* number of inode blocks this cg */
	long	cg_ndblk;		/* number of data blocks this cg */
	struct	csum cg_cs;		/* cylinder summary information */
	long	cg_rotor;		/* position of last used block */
	long	cg_frotor;		/* position of last used frag */
	long	cg_irotor;		/* position of last used inode */
	long	cg_frsum[MAXFRAG];	/* counts of available frags */
	long	cg_btot[MAXCPG];	/* block totals per cylinder */
	short	cg_b[MAXCPG][NRPOS];	/* positions of free blocks */
	char	cg_iused[MAXIPG/NBBY];	/* used inode map */
	long	cg_magic;		/* magic number */
	u_char	cg_free[1];		/* free block map */
/* actually longer */
};
#endif	/* OSF */

/*
 * MAXBPG bounds the number of blocks of data per cylinder group,
 * and is limited by the fact that cylinder groups are at most one block.
 * Its size is derived from the size of blocks and the (struct cg) size,
 * by the number of remaining bits.
 */
#define	MAXBPG(fs) \
	(fragstoblks((fs), (NBBY * ((fs)->fs_bsize - (sizeof (struct cg))))))

/*
 * Turn file system block numbers into disk block addresses.
 * This maps file system blocks to device size blocks.
 */
#ifndef	OSF
#define fsbtodb(fs, b)	((b) << (fs)->fs_fsbtodb)
#define	dbtofsb(fs, b)	((b) >> (fs)->fs_fsbtodb)

/*
 * Cylinder group macros to locate things in cylinder groups.
 * They calc file system addresses of cylinder group data structures.
 */
#define	cgbase(fs, c)	((daddr_t)((fs)->fs_fpg * (c)))
#define cgstart(fs, c) \
	(cgbase(fs, c) + (fs)->fs_cgoffset * ((c) & ~((fs)->fs_cgmask)))
#define	cgsblock(fs, c)	(cgstart(fs, c) + (fs)->fs_sblkno)	/* super blk */
#define	cgtod(fs, c)	(cgstart(fs, c) + (fs)->fs_cblkno)	/* cg block */
#define	cgimin(fs, c)	(cgstart(fs, c) + (fs)->fs_iblkno)	/* inode blk */
#define	cgdmin(fs, c)	(cgstart(fs, c) + (fs)->fs_dblkno)	/* 1st data */

/*
 * Give cylinder group number for a file system block.
 * Give cylinder group block number for a file system block.
 */
#define	dtog(fs, d)	((d) / (fs)->fs_fpg)
#define	dtogd(fs, d)	((d) % (fs)->fs_fpg)

/*
 * Extract the bits for a block from a map.
 * Compute the cylinder and rotational position of a cyl block addr.
 */
#define blkmap(fs, map, loc) \
    (((map)[(loc) / NBBY] >> ((loc) & (NBBY-1))) & (0xff >> (NBBY - (fs)->fs_frag)))
#define cbtocylno(fs, bno) \
	((bno) * NSPF(fs) / (fs)->fs_spc)
#define cbtorpos(fs, bno) \
	((bno) * NSPF(fs) % (fs)->fs_nsect * NRPOS / (fs)->fs_nsect)
#else	/* OSF */
#define hpux_cbtorpos(fs, bno) \
	((bno) * NSPF(fs) % (fs)->fs_nsect * NRPOS / (fs)->fs_nsect)
#endif	/* OSF */

/*
 * The following macros optimize certain frequently calculated
 * quantities by using shifts and masks in place of divisions
 * modulos and multiplications.
 */
#ifndef	OSF
#define blkoff(fs, loc)		/* calculates (loc % fs->fs_bsize) */ \
	((loc) & ~(fs)->fs_bmask)
#define fragoff(fs, loc)	/* calculates (loc % fs->fs_fsize) */ \
	((loc) & ~(fs)->fs_fmask)
#define lblkno(fs, loc)		/* calculates (loc / fs->fs_bsize) */ \
	((loc) >> (fs)->fs_bshift)
#define numfrags(fs, loc)	/* calculates (loc / fs->fs_fsize) */ \
	((loc) >> (fs)->fs_fshift)
#define blkroundup(fs, size)	/* calculates roundup(size, fs->fs_bsize) */ \
	(((size) + (fs)->fs_bsize - 1) & (fs)->fs_bmask)
#define fragroundup(fs, size)	/* calculates roundup(size, fs->fs_fsize) */ \
	(((size) + (fs)->fs_fsize - 1) & (fs)->fs_fmask)
#define fragstoblks(fs, frags)	/* calculates (frags / fs->fs_frag) */ \
	((frags) >> (fs)->fs_fragshift)
#define blkstofrags(fs, blks)	/* calculates (blks * fs->fs_frag) */ \
	((blks) << (fs)->fs_fragshift)
#define fragnum(fs, fsb)	/* calculates (fsb % fs->fs_frag) */ \
	((fsb) & ((fs)->fs_frag - 1))
#define blknum(fs, fsb)		/* calculates rounddown(fsb, fs->fs_frag) */ \
	((fsb) &~ ((fs)->fs_frag - 1))

/*
 * Determine the number of available frags given a
 * percentage to hold in reserve
 */
#define freespace(fs, percentreserved) \
	(blkstofrags((fs), (fs)->fs_cstotal.cs_nbfree) + \
	(fs)->fs_cstotal.cs_nffree - ((fs)->fs_dsize * (percentreserved) / 100))

/*
 * Determining the size of a file block in the file system.
 */
#define blksize(fs, ip, lbn) \
	(((lbn) >= NDADDR || (ip)->i_size >= ((lbn) + 1) << (fs)->fs_bshift) \
	    ? (fs)->fs_bsize \
	    : (fragroundup(fs, blkoff(fs, (ip)->i_size))))
#define dblksize(fs, dip, lbn) \
	(((lbn) >= NDADDR || (dip)->di_size >= ((lbn) + 1) << (fs)->fs_bshift) \
	    ? (fs)->fs_bsize \
	    : (fragroundup(fs, blkoff(fs, (dip)->di_size))))

/*
 * Number of disk sectors per block; assumes DEV_BSIZE byte sector size.
 */
#define	NSPB(fs)	((fs)->fs_nspf << (fs)->fs_fragshift)
#define	NSPF(fs)	((fs)->fs_nspf)
#endif	/* OSF */

#if	OSF
#include <string.h>	/* for memcpy */

#ifndef	ASSERT
#define ASSERT(a)
#endif	/* ASSERT */

/*
 * Macros for structure bases and first field that is different in the
 * two structures.  The first 32 fields of a struct fs and a struct
 * hpux_fs are the same so we can bcopy them across.
 */
#define hpux_fs_base(hpux_fs)	((char *) (hpux_fs))
#define osf1_fs_base(osf1_fs)	((char *) (osf1_fs))
#define hpux_fs_differ(hpux_fs)	((char *) &(hpux_fs)->fs_id)
#define osf1_fs_differ(osf1_fs)	((char *) &(osf1_fs)->fs_optim)

/* 
 * Convert an hpux super block to an osf1 super block
 * hpux_to_osf1_fs (struct hpux_fs *hpux_fs, struct fs *osf1_fs)
 */

#define hpux_to_osf1_fs(hpux_fs, osf1_fs)				      \
{									      \
	ASSERT(hpux_fs_differ(hpux_fs) - hpux_fs_base(hpux_fs) ==	      \
	       osf1_fs_differ(osf1_fs) - osf1_fs_base(osf1_fs));	      \
	       								      \
	memcpy(osf1_fs_base(osf1_fs), hpux_fs_base(hpux_fs),		      \
	       hpux_fs_differ(hpux_fs) - hpux_fs_base(hpux_fs));	      \
	       								      \
	osf1_fs->fs_optim = hpux_fs->fs_optim;				      \
	osf1_fs->fs_npsect = hpux_fs->fs_nsect;				      \
	osf1_fs->fs_interleave = 1;					      \
	osf1_fs->fs_trackskew = 0;					      \
	osf1_fs->fs_headswitch = 0;					      \
	osf1_fs->fs_trkseek = 0;					      \
	       								      \
	ASSERT((char *)&hpux_fs->fs_postbl - (char *)&hpux_fs->fs_csaddr ==   \
	       (char *)&osf1_fs->fs_opostbl - (char *)&osf1_fs->fs_csaddr);   \
	       								      \
	memcpy((char *)&osf1_fs->fs_csaddr, (char *)&hpux_fs->fs_csaddr,      \
	       (char *)&hpux_fs->fs_postbl - (char *)&hpux_fs->fs_csaddr);    \
	       								      \
	memcpy((char *)&osf1_fs->fs_opostbl, (char *)&hpux_fs->fs_postbl,     \
	       sizeof(osf1_fs->fs_opostbl));				      \
	       								      \
	if (hpux_fs->fs_clean == HPUX_FS_CLEAN)				      \
		osf1_fs->fs_clean = FS_CLEAN;				      \
	osf1_fs->fs_qbmask._SIG64_BITS = ~osf1_fs->fs_bmask;		      \
	osf1_fs->fs_qfmask._SIG64_BITS = ~osf1_fs->fs_fmask;		      \
	osf1_fs->fs_postblformat = FS_DYNAMICPOSTBLFMT;			      \
	osf1_fs->fs_nrpos = NRPOS;					      \
	osf1_fs->fs_postbloff = (char *)&osf1_fs->fs_opostbl		      \
	  			- osf1_fs_base(osf1_fs);		      \
	osf1_fs->fs_rotbloff = (char *)&osf1_fs->fs_space		      \
	  			- osf1_fs_base(osf1_fs);		      \
	osf1_fs->fs_magic = HPUX_FS_MAGIC;				      \
	memcpy((char *)&osf1_fs->fs_space, (char *)&hpux_fs->fs_rotbl,	      \
	       SBSIZE - max(						      \
		  (char *)&osf1_fs->fs_space - osf1_fs_base(osf1_fs),	      \
		  (char *)&hpux_fs->fs_rotbl - hpux_fs_base(hpux_fs)));	      \
}

#endif /* OSF */

#endif /* not _SYS_FS_INCLUDED */
