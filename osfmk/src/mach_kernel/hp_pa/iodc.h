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
 * Copyright (c) 1990,1991 The University of Utah and
 * the Center for Software Science (CSS).
 * All rights reserved.

 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * Copyright (c) 1990 mt Xinu, Inc.
 * This file may be freely distributed in any form as long as
 * this copyright notice is included.
 * MTXINU, THE UNIVERSITY OF UTAH, AND CSS PROVIDE THIS SOFTWARE ``AS
 * IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: iodc.h 1.4 91/09/25$
 *	Author: Dave Slattengren (mtXinu), Jeff Forys (CSS)
 */

/*
 * Definitions for talking to IODC (I/O Dependent Code).
 *
 * The PDC is used to load I/O Dependent Code from a particular module.
 * I/O Dependent Code is module-type dependent software which provides
 * a uniform way to identify, initialize, and access a module (and in
 * some cases, their devices).
 */

#ifndef	_IODC_
#define _IODC_
#ifndef ASSEMBLER
#include <types.h>
#endif	/* ASSEMBLER */

/*
 * IODC Entry Indices and their arguments...
 */

#define	IODC_DATA	0	/* get the 16-byte IODC structure (see below) */

#define	IODC_INIT	3	/* initialize (see options below) */
#define	IODC_INIT_FIRST		2	/* find first device on module */
#define	IODC_INIT_NEXT		3	/* find subsequent devices on module */
#define	IODC_INIT_ALL		4	/* initialize module and device */
#define	IODC_INIT_DEV		5	/* initialize device */
#define	IODC_INIT_MOD		6	/* initialize module */
#define	IODC_INIT_MSG		9	/* return error message(s) */

#define	IODC_IO		4	/* perform I/O (see options below) */
#define	IODC_IO_BOOTIN		0	/* read from boot device */
#define	IODC_IO_BOOTOUT		1	/* write to boot device */
#define	IODC_IO_CONSIN		2	/* read from console */
#define	IODC_IO_CONSOUT		3	/* write to conosle */
#define	IODC_IO_MSG		9	/* return error message(s) */

#define	IODC_SPA	5	/* get extended SPA information */
#define	IODC_SPA_DFLT		0	/* return SPA information */

#define	IODC_TEST	8	/* perform self tests */
#define	IODC_TEST_INFO		0	/* return test information */
#define	IODC_TEST_STEP		1	/* execute a particular test */
#define	IODC_TEST_TEST		2	/* describe a test section */
#define	IODC_TEST_MSG		9	/* return error message(s) */


/*
 * The following structure defines what a particular IODC returns when
 * given the IODC_DATA argument.
 */

#ifndef	ASSEMBLER
struct iodc_data {
	u_int	iodc_model: 8,		/* hardware model number */
		iodc_revision:8,	/* software revision */
		iodc_spa_io: 1,		/* 0:memory, 1:device */
		iodc_spa_pack:1,	/* 1:packed multiplexor */
		iodc_spa_enb:1,		/* 1:has an spa */
		iodc_spa_shift:5,	/* power of two # bytes in SPA space */
		iodc_more: 1,		/* iodc_data is: 0:8-byte, 1:16-byte */
		iodc_word: 1,		/* io_dc_data is: 0:byte, 1:word */
		iodc_pf: 1,		/* 1:supports powerfail */
		iodc_type: 5;		/* see below */
	u_int	iodc_sv_rev: 4,		/* software version revision number */
		iodc_sv_model:20,	/* software interface model # */
		iodc_sv_opt: 8;		/* type-specific options */
	u_char	iodc_rev;		/* revision number of IODC code */
	u_char	iodc_dep;		/* module-dependent information */
	u_char	iodc_rsv[2];		/* reserved */
	u_short	iodc_cksum;		/* 16-bit checksum of whole IODC */
	u_short	iodc_length;		/* number of entry points in IODC */
		/* IODC entry points follow... */
};
#endif	/* !ASSEMBLER */

/* iodc_type */
#define	IODC_TP_NPROC	 0	/* native processor */
#define	IODC_TP_MEMORY	 1	/* memory */
#define	IODC_TP_B_DMA	 2	/* Type-B DMA (NIO Transit, Parallel, ... ) */
#define	IODC_TP_B_DIRECT 3	/* Type-B Direct */
#define	IODC_TP_A_DMA	 4	/* Type-A DMA (NIO HPIB, LAN, ... ) */
#define	IODC_TP_A_DIRECT 5	/* Type-A Direct (RS232, HIL, ... ) */
#define	IODC_TP_OTHER	 6	/* other */
#define	IODC_TP_BCPORT	 7	/* Bus Converter Port */
#define	IODC_TP_CIO	 8	/* CIO adapter */
#define	IODC_TP_CONSOLE	 9	/* console */
#define	IODC_TP_FIO	10	/* foreign I/O module */
#define	IODC_TP_BA	11	/* bus adaptor */
#define	IODC_TP_IO_ADAP	12	/* I/O adaptor */
#define	IODC_TP_BRIDGE	13	/* Bus Bridge to foreign Bus */
#define IODC_TP_EISA    30      /* EISA bus devices */
#define	IODC_TP_FAULTY	31	/* broken */

/* iodc_sv_model (IODC_TP_MEMORY) */
#define	SVMOD_MEM_ARCH	0x8	/* architected memory module */
#define	SVMOD_MEM_PDEP	0x9	/* processor-dependent memory module */

/* iodc_sv_model (IODC_TP_OTHER) */
#define	SVMOD_O_SPECFB	0x48	/* hp800 Spectograph frame buffer */
#define	SVMOD_O_SPECCTL	0x49	/* hp800 Spectograph control */

/* iodc_sv_model (IODC_TP_BA) */
#define SVMOD_BA_VME     0x78
#define SVMOD_BA_DIO     0x03
#define SVMOD_BA_SGC     0x05
#define SVMOD_BA_GSC     0x07
#define	SVMOD_BA_ASP	 0x70	
#define	SVMOD_BA_EISA	 0x76	
#define SVMOD_BA_LASI    0x81
#define SVMOD_BA_WAX     0x8e
#define SVMOD_BA_WEISA   0x90
#define SVMOD_BA_UNKNOWN 0x00

/* iodc_sv_model (IODC_TP_FIO) */
#define	SVMOD_FIO_SCSI	0x71	/* hp700 Core SCSI */
#define	SVMOD_FIO_LAN	0x72	/* hp700 Core LAN */
#define	SVMOD_FIO_HIL	0x73	/* hp700 Core HIL */
#define	SVMOD_FIO_CENT	0x74	/* hp700 Core Centronics */
#define	SVMOD_FIO_RS232	0x75	/* hp700 Core RS-232 */
#define	SVMOD_FIO_SGC	0x77	/* hp700 SGC Graphics */
#define	SVMOD_FIO_A1	0x7a	/* hp700 Core audio (type 1) */
#define	SVMOD_FIO_A1NB	0x7e	/* hp700 Core audio (type 1, no beeper) */
#define	SVMOD_FIO_A2	0x7f	/* hp700 Core audio (type 2) */
#define	SVMOD_FIO_A2NB	0x7b	/* hp700 Core audio (type 2, no beeper) */
#define SVMOD_FIO_FWSCSI 0x7c
#define	SVMOD_FIO_FDDI	0x7d	
#define	SVMOD_FIO_HPIB	0x80	/* hp700 Core HPIB (unsupported) */

#define	SVMOD_FIO_GSCSI	0x82	/* hp712 Core SCSI */
#define	SVMOD_FIO_GPCFD	0x83	/* hp712 PC floppy disk */
#define	SVMOD_FIO_GPCIO	0x84	/* hp712 PC keyboard and mouse */
#define	SVMOD_FIO_GGRF	0x85	/* hp712 SGC Graphics */
#define	SVMOD_FIO_GLAN	0x8a	/* hp712 Core LAN */
#define	SVMOD_FIO_GRS232 0x8c	/* hp712 Core RS232 */

#define	SVMOD_FIO_GSCSCSI 0x89

/* iodc_sv_model (IODC_TP_A_DMA) */
#define	SVMOD_ADMA_FWSCSI  0x00089 /* hp700 FW-SCSI */
#define	SVMOD_ADMA_MYRINET 0x00095 /* Hamlyn GSC+ Network Card */

#endif	/* _IODC_ */
