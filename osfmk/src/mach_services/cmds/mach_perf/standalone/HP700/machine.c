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
#include <mach_perf.h>
#include <norma_services.h>

struct test_dir machine_test_dir [] = {
	0, 0, 0
};

/*
 * COPYRIGHT NOTICE
 * Copyright (c) 1990, 1991, 1992, 1993 Open Software Foundation, Inc.
 * ALL RIGHTS RESERVED (OSF/1).  See /usr/include/COPYRIGHT.OSF1 .
 */
/*
 * HISTORY
 * $Log: machine.c,v $
 * Revision 1.1.4.2  1996/01/25  11:32:47  bernadat
 * 	Homogenized copyright headers
 * 	[95/11/30            bernadat]
 *
 * Revision 1.1.5.2  1995/12/20  13:17:19  bernadat
 * 	Homogenized copyright headers
 * 	[95/11/30            bernadat]
 *
 * Revision 1.1.4.1  1995/04/07  19:13:18  barbou
 * 	Merged into mainline.
 * 	[95/03/27            barbou]
 *
 * Revision 1.1.3.2  1995/04/07  18:17:19  barbou
 * 	Merged into mainline.
 * 	[95/03/27            barbou]
 *
 * Revision 1.1.2.1  1994/04/28  17:53:21  bruel
 * 	Created.
 * 	[94/04/28            bruel]
 *
 * Revision 1.1.1.2  1994/04/28  17:38:08  bruel
 * 	Created.
 * 	[94/04/28            bruel]
 *
 * Revision 3.0.2.2  1993/09/15  19:33:41  alan
 * 	replaced copyright marker with copyright text
 * 	[1993/09/09  10:55:18  alan]
 *
 * Revision 3.0  1993/01/01  03:36:01  ede
 * 	Initial revision for OSF/1 R1.3
 * 
 * Revision 2.5.2.2  1992/02/20  23:01:05  damon
 * 	Removed I B M  C O N F I D E N T I A L messages
 * 	[1992/02/14  20:11:30  damon]
 * 
 * Revision 2.5  1990/10/07  19:22:29  devrcs
 * 	Added EndLog Marker.
 * 	[90/09/28  19:08:56  gm]
 * 
 * 	Added rcsid line.
 * 	[90/09/27  07:48:20  kathyg]
 * 
 * Revision 2.4  90/08/24  13:44:10  devrcs
 * 	Make thread safe by elimitating static data. The element and
 * 	comparison function are now passed through the recursion by adding
 * 	a structure pointer to each call.
 * 	[90/08/17  15:18:08  sp]
 * 
 * Revision 2.3  90/03/13  21:15:14  mbrown
 * 	New libc integrated for AIX code.
 * 	[90/03/06  00:45:15  stevem]
 * 
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile: machine.c,v $ $Revision: 1.1.4.2 $ (OSF) $Date: 1996/01/25 11:32:47 $";
#endif
/*
 * FUNCTIONS: qsort
 *
 * (C) COPYRIGHT International Business Machines Corp. 1985, 1989 
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 * Copyright (c) 1984 AT&T	
 * All Rights Reserved  
 *
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	
 * The copyright notice above does not evidence any   
 * actual or intended publication of such source code.
 *
 * qsort.c	1.12  com/lib/c/gen,3.1,8943 10/16/89 09:20:05
 */

#include <stdlib.h>			/* for size_t */

/*
 * FUNCTION:	Qsort sorts a table using the "quicker-sort" algorithm.
 *                                                                    
 * NOTES:	'Base' is the base of the table, 'nmemb' is the number
 *		of elements in it, 'size' is the size of an element and
 *		'compar' is the comparision function to call to compare
 *		2 elements of the table.  'Compar' is called with 2
 *		arguments, each a pointer to an element to be compared.
 *		It must return:
 *			< 0	if the first argument is less than the second
 *			> 0	if the first argument is greater than the second
 *			= 0	if the first argument is equal to the second
 *
 *
 */  

struct qdata {
	size_t	qses;				     /* element size */
	int	(*qscmp)(const void *, const void *); /* comparison function */
};

static void qstexc( char *i, char *j, char *k, struct qdata *qd);
static void qsexc( char *i, char *j, struct qdata *qd);
static void qs1( char *a, char *l, struct qdata *qd);


void 
qsort(void *base, size_t nmemb, size_t size,
	 int(*compar)(const void *, const void *))
{
	struct qdata	qsdata;

	qsdata.qscmp = compar;	/* save off the comparison function	*/
	qsdata.qses = size;	/* save off the element size		*/

	qs1((char *)base, (char *)base + nmemb * size, &qsdata);
}

static void
qs1( char *a, char *l, struct qdata *qsd)
{
	char *i, *j;
	size_t es;
	char	*lp, *hp;
	int	c;
	unsigned n;

	es = qsd->qses;
start:
	if((n=l-a) <= es)
		return;
	n = es * (n / (2*es));
	hp = lp = a+n;
	i = a;
	j = l-es;
	while(1) {
		if(i < lp) {
			if((c = (*(qsd->qscmp))((void*)i, (void*)lp)) == 0) {
				qsexc(i, lp -= es, qsd);
				continue;
			}
			if(c < 0) {
				i += es;
				continue;
			}
		}

loop:
		if(j > hp) {
			if((c = (*(qsd->qscmp))((void*)hp, (void*)j)) == 0) {
				qsexc(hp += es, j, qsd);
				goto loop;
			}
			if(c > 0) {
				if(i == lp) {
					qstexc(i, hp += es, j, qsd);
					i = lp += es;
					goto loop;
				}
				qsexc(i, j, qsd);
				j -= es;
				i += es;
				continue;
			}
			j -= es;
			goto loop;
		}

		if(i == lp) {
			if(lp-a >= l-hp) {
				qs1(hp+es, l, qsd);
				l = lp;
			} else {
				qs1(a, lp, qsd);
				a = hp+es;
			}
			goto start;
		}

		qstexc(j, lp -= es, i, qsd);
		j = hp -= es;
	}
}

static void
qsexc( char *i, char *j, struct qdata *qsd)
{
	char *ri, *rj, c;
	size_t n;

	n = qsd->qses;
	ri = i;
	rj = j;
	do {
		c = *ri;
		*ri++ = *rj;
		*rj++ = c;
	} while(--n);
}

static void
qstexc( char *i, char *j, char *k, struct qdata *qsd)
{
	char *ri, *rj, *rk;
	int c;
	size_t n;

	n = qsd->qses;
	ri = i;
	rj = j;
	rk = k;
	do {
		c = *ri;
		*ri++ = *rk;
		*rk++ = *rj;
		*rj++ = c;
	} while(--n);
}

