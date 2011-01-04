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
 * Copyright (c) 1991 Carnegie Mellon University
 * Copyright (c) 1991 Sequent Computer Systems
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON AND SEQUENT COMPUTER SYSTEMS ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND
 * SEQUENT COMPUTER SYSTEMS DISCLAIM ANY LIABILITY OF ANY KIND FOR
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

/* CMU_HIST */
/*
 * Revision 2.3.2.1  92/04/30  12:01:09  bernadat
 * 	Use a boot syntax similar to the i386at one for server args
 * 	[92/04/27            bernadat]
 * 
 * Revision 2.3  91/07/31  18:03:39  dbg
 * 	Changed copyright.
 * 	Moved root_name string out of text segment.
 * 	[91/07/31            dbg]
 * 
 * Revision 2.2  91/05/08  12:58:47  dbg
 * 	Adapted for pure Mach kernel.
 * 	[90/10/09            dbg]
 * 
 */

/*
 * $Header: /u1/osc/rcs/mach_kernel/sqt/setconf.c,v 1.2.10.1 1994/09/23 03:06:42 ezf Exp $
 */

/*
 * Revision 1.1  89/11/14  18:35:23  root
 * Initial revision
 * 
 * Revision 1.2  89/08/16  15:17:31  root
 * balance -> sqt
 * 
 * Revision 1.1  89/07/05  13:17:37  kak
 * Initial revision
 * 
 * Revision 2.2  88/03/20  17:32:11  bak
 * removed xp entry from genericconf structure. xp not supported in symmetry.
 * 
 */
#include <mach/boolean.h>

#include <sqt/cfg.h>
#include <sqt/vm_defs.h>
#include <sys/reboot.h>

char	root_name_string[16] = "\0\0\0\0\0\0\0";	/* at least ddNNp */
char	*root_name = root_name_string;
char	boot_string[BNAMESIZE+1];

#define	MINARGLEN	5

/*
 * The boot syntax is 
 * b [Hex args] zd(?,?)kernel_name [:server_name] [-has] [server_args]
 * this is almost the same syntax as for the i386AT, except the 
 * server name must be separated from the kernel name with a space, as
 * the boot program is not modified.
 *
 * The generic rootdev is passed into the kernel as the
 * 1st argument in the boot name. The argument specifies the device and
 * unit number to use for the rootdev. The actual partitions to be used for
 * a given device is specified in ../conf/conf_generic.c.
 * The argument string is in the form:
 *		XXds		(eg. sd0a)
 *
 *	where	XX is the device type (e.g. sd for scsi disk),
 *		d  is the Dynix device unit number,
 *		s is the Dynix device slice (a-g)
 */

setconf()
{
	register struct genericconf *gc;
	register char *boot_name;
	register char *name;
	register i;
	int	is_server_arg = 0;

	/*
	 * Find Generic rootdev string.
	 * Use boot device name.
	 */

	boot_name = PHYSTOKV(va_CD_LOC->c_boot_name, char *);

	boot_name[BNAMESIZE-1] = '\0';

	/*
	 * Use boot name prefix.
	 * zd(0,0) becomes zd0a.
	 */
	name = boot_name;
	root_name[0] = name[0];
	root_name[1] = name[1];
	if (name[2] != '(')
	  goto error;
	root_name[2] = name[3];
	if (name[4] != ',')
	  goto error;
	root_name[3] = name[5] - '0' + 'a';
	root_name[4] = '\0';

	/*
	 * Force RB_KDB since there is no way to detect x mode on boot file
	 */

	boothowto |= RB_KDB;

	/*
	 * Get boot arguments, 
	 * look for kernel args ( -hasd )
	 */
	
	bcopy(boot_name, boot_string, BNAMESIZE);
	name = boot_string;


	while (name < &boot_string[BNAMESIZE]) {
		switch (*name) {
		case '-':
			while(*++name) switch(*name) {
			case 'h':
				boothowto |= RB_HALT;
				break;
			case 'a':
				boothowto |= (RB_ASKNAME << RB_SHIFT);
				break;
			case 's':
				boothowto |= RB_SINGLE;
				break;
			case 'd':
				boothowto |= RB_KDB;
				break;
			default:
				break;
			}
			break;
		case '\0':
			*name++ = ' ';
			break;
		default:
			name++;
		}
	}

	printf("Root on %s.\n", root_name);
	return;

error:
	printf("Generic root device ");
	if (*name == NULL)
		printf("not specified.\n");
	else
		printf("\"%s\" is incorrect.\n", name);

	printf("Returning to firmware\n");
	return_fw(FALSE);
}

