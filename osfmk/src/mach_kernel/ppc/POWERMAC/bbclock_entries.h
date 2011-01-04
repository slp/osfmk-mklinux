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
/* OLD PPC HISTORY
 * Revision 1.1.7.1  1996/04/11  09:07:31  emcmanus
 * 	Copied from mainline.ppc.
 * 	[1996/04/10  17:02:54  emcmanus]
 *
 * 	change marker to not FREE
 * 	[1994/09/22  21:15:26  ezf]
 *
 * Revision 1.1.5.1  1995/11/23  17:41:11  stephen
 * 	first powerpc checkin to mainline.ppc
 * 	[1995/11/23  16:56:34  stephen]
 * 
 * Revision 1.1.3.1  1995/10/10  15:08:54  stephen
 * 	return from apple
 * 	[1995/10/10  14:35:35  stephen]
 * 
 * 	powerpc merge
 * 
 * Revision 1.1.1.2  95/09/05  17:58:12  stephen
 * 	File created - copied from AT386 branch
 * 
 * END_OLD_PPC_HISTORY
 */

extern kern_return_t	bbc_gettime(
				tvalspec_t		* curtime);
extern kern_return_t	bbc_settime(
				tvalspec_t		* curtime);

#define	NO_MAPTIME	(kern_return_t (*)(ipc_port_t * pager))0
#define	NO_SETALRM	(void (*) (tvalspec_t * alarm_time))0

