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
 * OLD HISTORY
 * Revision 1.1.4.4  1996/01/25  11:32:35  bernadat
 * 	__main is defined in libsa_mach for some platforms.
 * 	Moved from standalone misc.c to here.
 * 	[95/12/26            bernadat]
 * 
 * Revision 1.1.4.3  1995/01/10  08:08:03  devrcs
 * 	mk6 CR801 - copyright marker not _FREE
 * 	[1994/12/01  21:35:42  dwm]
 * 
 * Revision 1.1.4.1  1994/06/18  18:44:53  bolinger
 * 	Import NMK17.2 sources into MK6.
 * 	[1994/06/18  18:35:42  bolinger]
 * 
 * Revision 1.1.2.1  1994/04/14  13:54:46  bernadat
 * 	Initial revision.
 * 	[94/04/14            bernadat]
 * 
 */

#include <mach_perf.h>
#include <norma_services.h>

struct test_dir machine_test_dir [] = {
	TEST(set_def_pager, 0),
	TEST(get_def_pager, 0),
	TEST(node, 0),
	0, 0, 0
};

__main() {}	/* GCC 2 C++ init routine */
