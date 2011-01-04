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
 
/*
 *   Copyright 1988, 1989 by Intel Corporation, Santa Clara, California.
 * 
 * 		All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appears in all
 * copies and that both the copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Intel
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.
 * 
 * INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * disk.h
 */

#define PART_DISK       2               // do I need this?

#define V_NUMPAR        16              /* maximum number of partitions */

/* driver ioctl() commands */
#define V_REMOUNT       _IO('v',2)    		/* Remount Drive */
#define V_GETPARMS      _IOR('v',4,struct disk_parms)   /* Get drive/partition parameters */
#define V_FORMAT        _IOW('v',5,union io_arg)/* Format track(s) */
#define V_ABS		_IOW('v',9,int)		/* set a sector for an absolute addr */
#define V_RDABS		_IOW('v',10,struct absio)/* Read a sector at an absolute addr */
#define V_WRABS		_IOW('v',11,struct absio)/* Write a sector to absolute addr */
#define V_VERIFY	_IOWR('v',12,union vfy_io)/* Read verify sector(s) */
#define V_SETPARMS	_IOW('v',14,int)	/* Set drivers parameters */
#define V_EJECT		_IO('v',15)		/* Eject disk */

union   io_arg {
	struct  {
		unsigned short	ncyl;	/* number of cylinders on drive */
		unsigned char	nhead;	/* number of heads/cyl */
		unsigned char	nsec;	/* number of sectors/track */
		unsigned short	secsiz;	/* number of bytes/sector */
		} ia_cd;		/* used for Configure Drive cmd */
	struct  {
		unsigned short	flags;	/* flags (see below) */
		long	bad_sector;	/* absolute sector number */
		long	new_sector;	/* RETURNED alternate sect assigned */
		} ia_abs;		/* used for Add Bad Sector cmd */
	struct  {
		unsigned short	start_trk; /* first track # */
		unsigned short	num_trks; /* number of tracks to format */
		unsigned short	intlv;	/* interleave factor */
		} ia_fmt;		/* used for Format Tracks cmd */
	struct	{
		unsigned short	start_trk; /* first track	*/
		char	*intlv_tbl;	/* interleave table */
		} ia_xfmt;		/* used for the V_XFORMAT ioctl */
};

/*
 * Data structure for the V_VERIFY ioctl
 */
union	vfy_io	{
	struct	{
		long	abs_sec;	/* absolute sector number        */
		unsigned short	num_sec; /* number of sectors to verify   */
		unsigned short	time_flg; /* flag to indicate time the ops */
		}vfy_in;
	struct	{
		long	deltatime;	/* duration of operation */
		unsigned short	err_code; /* reason for failure    */
		}vfy_out;
};
#define BAD_BLK         0x80                    /* needed for V_VERIFY */

/* data structure returned by the Get Parameters ioctl: */
struct  disk_parms {
	char	dp_type;		/* Disk type (see below) */
	unsigned char	dp_heads;	/* Number of heads */
	unsigned short	dp_cyls;	/* Number of cylinders */
	unsigned char	dp_sectors;	/* Number of sectors/track */
	unsigned short	dp_secsiz;	/* Number of bytes/sector */
					/* for this partition: */
	unsigned short	dp_ptag;	/* Partition tag */
	unsigned short	dp_pflag;	/* Partition flag */
	long	dp_pstartsec;		/* Starting absolute sector number */
	long	dp_pnumsec;		/* Number of sectors */
	unsigned char	dp_dosheads;	/* Number of heads */
	unsigned short	dp_doscyls;	/* Number of cylinders */
	unsigned char	dp_dossectors;	/* Number of sectors/track */
};

/* Disk types for disk_parms.dp_type: */
#define DPT_WINI        1               /* Winchester disk */
#define DPT_FLOPPY      2               /* Floppy */
#define DPT_OTHER       3               /* Other type of disk */
#define DPT_NOTDISK     0               /* Not a disk device */

/* Data structure for V_RDABS/V_WRABS ioctl's */
struct absio {
	long	abs_sec;		/* Absolute sector number (from 0) */
	char	*abs_buf;		/* Sector buffer */
};

#if	MACH_KERNEL

#include <device/disk_status.h>
/*
 * ppc/POWERMAC/at386_scsi.c
 */

extern io_return_t	scsi_rw_abs(
				dev_t				dev,
				dev_status_t			data,
				int				rw,
				int				sec,
				int				count);

extern io_return_t	ata_rw_abs(
				dev_t				dev,
				dev_status_t			data,
				int				rw,
				int				sec,
				int				count);

#endif /* MACH_KERNEL */
