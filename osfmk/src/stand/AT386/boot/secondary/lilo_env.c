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
 * Lilo environment variables
 */

#include "boot.h"
#include <sys/reboot.h>

#define LILO_PARMS 		(0x9d800-BOOTOFFSET)
#define LILO_PARMS_LENGTH 	(0x9dc00-0x9d800)
#define LILO_ROOTDEV 		(0x90000+0x1fc-BOOTOFFSET)

#define LILO_FLOPPY		0x2
#define LILO_HD			0x3
#define LILO_SCSI		0x8

extern int bootdev;
extern int howto;

/*
 * Retrieve bootdev and howto from lilo environment variables
 */

get_lilo_env()
{
	char *where;
	int maj, min;
	int dev;

	/* Remote console ? */

	if (is_sub_string("serial", 
			  (char *)LILO_PARMS,
			  8*LILO_PARMS_LENGTH))
		howto |= RB_ALTBOOT;

	/* Default boot dev */

	dev = *(unsigned short *)LILO_ROOTDEV;

	/* Can be overwritten */

	if (where = is_sub_string("root=", 
				  (char *)LILO_PARMS,
				  8*LILO_PARMS_LENGTH))
		dev = atoi(where+strlen("root="));
	
	
	if (!dev)
		return;

	maj = dev/100;
	min = dev - maj*100;
	switch(maj) {
	case LILO_FLOPPY:
		bootdev = 1 << B_TYPESHIFT;
		bootdev |= (min&1) << B_UNITSHIFT;
		break;
	case LILO_HD:
		bootdev = min/64 << B_UNITSHIFT;
		bootdev |= (min - (min/64)*64 << B_PARTITIONSHIFT) - 1;
		break;
	case LILO_SCSI:
		bootdev = min/16 << B_UNITSHIFT;
		bootdev |= (min - (min/16)*16 << B_PARTITIONSHIFT) -1;
		break;
	}
}
