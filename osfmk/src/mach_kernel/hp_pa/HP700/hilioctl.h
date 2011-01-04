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
 * Copyright (c) 1990-1994, The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 *	Utah $Hdr: hilioctl.h 1.11 94/12/14$
 */

struct _hilbell {
	unsigned char	duration;
	unsigned char	frequency;
};

struct _hilbuf16 {
	unsigned char	string[16];
};

struct _hilbuf11 {
	unsigned char	string[11];
};

struct _hilbuf5 {
  	unsigned char  string[5];
};

struct _hilbuf4 {
  	unsigned char  string[4];
};

struct _hilbuf2 {
	unsigned char	string[2];
};

/*
 * HPUX ioctls (here for the benefit of the HIL driver).
 * Named as they are under HPUX.
 * The first set are loop device ioctls.
 * The second set are ioctls for the 8042.
 * Note that some are not defined as in HPUX
 * due to the difference in the definitions of IOC_VOID.
 */
#ifdef hp300
#define _IOHpux(x,y)	(IOC_IN|((x)<<8)|y)	/* IOC_IN is IOC_VOID */
#else
#define _IOHpux(x,y)	_IO(x,y)
#endif

#define HILID	_IOR('h',0x03,struct _hilbuf11)	/* Identify & describe */
#define HILRN	_IOR('h',0x30,struct _hilbuf16)	/* Report name */
#define HILRS	_IOR('h',0x31,struct _hilbuf16)	/* Report status */
#define HILED	_IOR('h',0x32,struct _hilbuf16)	/* Extended describe*/
#define HILSC	_IOR('h',0x33,struct _hilbuf16)	/* Security code */
#define HILDKR  _IOHpux('h',0x3D)		/* Disable autorepeat */
#define HILER1  _IOHpux('h',0x3E)		/* Autorepeat 1/30 */
#define HILER2  _IOHpux('h',0x3F)		/* Autorepeat 1/60 */
#define HILP1	_IOHpux('h',0x40)		/* Prompt 1 */
#define HILP2	_IOHpux('h',0x41)		/* Prompt 2 */
#define HILP3	_IOHpux('h',0x42)		/* Prompt 3 */
#define HILP4	_IOHpux('h',0x43)		/* Prompt 4 */
#define HILP5	_IOHpux('h',0x44)		/* Prompt 5 */
#define HILP6	_IOHpux('h',0x45)		/* Prompt 6 */
#define HILP7	_IOHpux('h',0x46)		/* Prompt 7 */
#define HILP	_IOHpux('h',0x47)		/* Prompt */
#define HILA1	_IOHpux('h',0x48)		/* Acknowledge 1 */
#define HILA2	_IOHpux('h',0x49)		/* Acknowledge 2 */
#define HILA3	_IOHpux('h',0x4A)		/* Acknowledge 3 */
#define HILA4	_IOHpux('h',0x4B)		/* Acknowledge 4 */
#define HILA5	_IOHpux('h',0x4C)		/* Acknowledge 5 */
#define HILA6	_IOHpux('h',0x4D)		/* Acknowledge 6 */
#define HILA7	_IOHpux('h',0x4E)		/* Acknowledge 7 */
#define HILA	_IOHpux('h',0x4F)		/* Acknowledge */

#define EFTRCC  _IOR('H',0x11,char)		/* Read configuration code. */
#define EFTRLC  _IOR('H',0x12,char)		/* Read the language code. */
#define EFTSRD  _IOW('H',0xa0,char)		/* Set the repeat delay. */
#define EFTSRR  _IOW('H',0xa2,char)		/* Set the repeat rate. */
#ifdef hp300
#define EFTSBI  _IOW('H',0xa3,struct _hilbuf2)	/* Set the bell information. */
#else
#define EFTSBI  _IOW('H',0xa3,char)		/* Do the beep thing. */
#endif
#define EFTSRPG _IOW('H',0xa6,char)		/* Set RPG interrupt rate. */
#define EFTSBP  _IOW('H',0xc4,struct _hilbuf4)	/* Send data to the beeper. */
#define EFTRRT  _IOR('H',0x31,struct _hilbuf5)	/* Read the real time. */
#define EFTRT   _IOR('H',0xf4,struct _hilbuf4)	/* Read timers for 4 voices. */

#define	BELLDUR		80	/* tone duration in msec (10 - 2560) */
#define	BELLFREQ	8	/* tone frequency (0 - 63) */

/*
 * HIL input queue.
 * This is the circular queue (allocated by HILIOCALLOC) shared by kernel
 * and user.  It consists of a sixteen byte header followed by space for
 * NBPG-16 bytes of input data packets (24 bytes each).  The kernel adds
 * packets at tail.  The user is expected to remove packets from head.
 * This is the only field in the header that the user should modify.
 */
typedef struct hil_packet {
	long	tstamp;		/* time stamp */
	unsigned char	size;		/* total packet size */
	unsigned char	dev;		/* loop device packet was generated by */
	unsigned char	phdr;		/* poll header */
	unsigned char	data[14];	/* device data */
	unsigned char	pad[3];		/* pad to LW boundary--wastes space on 68k */
} hil_packet;

typedef struct hil_eventqueue {
	int	qnum;
	int	size;
	int	head;
	int	tail;
} hil_eventqueue;

typedef union hilqueue {
	char	hqu_size[4096];
	struct	q_data {
		hil_eventqueue	h_eventqueue;
		hil_packet	h_event[1];
	} q_data;
#define hil_evqueue	q_data.h_eventqueue
#define hil_event	q_data.h_event
} HILQ;

#define HEVQSIZE	\
	((sizeof(HILQ) - sizeof(hil_eventqueue)) / sizeof(hil_packet))
