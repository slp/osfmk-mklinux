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

#define	STATIC

STATIC void qsort_swap(
	register int	*a,
	register int	*b,
	register int	size);

STATIC void qsort_rotate(
	register int	*a,
	register int	*b,
	register int	*c,
	register int	size);

STATIC void qsort_recur(
	char	*left,
	char	*right,
	int	eltsize,
	int	(*compfun)(char *, char *));

STATIC void qsort_checker(
	char	*table,
	int	nbelts,
	int	eltsize,
	int	(*compfun)(char *, char *));

STATIC int qsort_check = 0;

void
qsort(
	char	*table,
	int	nbelts,
	int	eltsize,
	int	(*compfun)(char *, char *))
{
	if (nbelts <= 0 || eltsize <= 0 || compfun == 0) {
		printf("qsort: invalid parameters\n");
		return;
	}
	qsort_recur(table, table + nbelts * eltsize, eltsize, compfun);

	if (qsort_check)
		qsort_checker(table, nbelts, eltsize, compfun);
}

STATIC void
qsort_swap(
	register int	*a,
	register int	*b,
	register int	size)
{
	register int temp;
	char *aa, *bb;
	char ctemp;

	for (; size >= sizeof (int); size -= sizeof (int), a++, b++) {
		temp = *a;
		*a = *b;
		*b = temp;
	}
	aa = (char *)a;
	bb = (char *)b;
	for (; size > 0; size--, aa++, bb++) {
		ctemp = *aa;
		*aa = *bb;
		*bb = ctemp;
	}
}

/*
 *	rotate the three elements to the left
 */
STATIC void
qsort_rotate(
	register int	*a,
	register int	*b,
	register int	*c,
	register int	size)
{
	register int temp;
	char *aa, *bb, *cc;
	char ctemp;

	for (; size >= sizeof (int); size -= sizeof (int), a++, b++, c++) {
		temp = *a;
		*a = *c;
		*c = *b;
		*b = temp;
	}
	aa = (char *)a;
	bb = (char *)b;
	cc = (char *)c;
	for (; size > 0; size--, aa++, bb++, cc++) {
		ctemp = *aa;
		*aa = *cc;
		*cc = *bb;
		*bb = ctemp;
	}
}

STATIC void
qsort_recur(
	char	*left,
	char	*right,
	int	eltsize,
	int	(*compfun)(char *, char *))
{
	char *i, *j;
	char *sameleft, *sameright;

    top:
	if (left + eltsize - 1 >= right) {
		return;
	}

	/*
	 *	partition element (reference for "same"ness
	 */
	sameleft = left + (((right - left) / eltsize) / 2) * eltsize;
	sameright = sameleft;

	i = left;
	j = right - eltsize;

    again:
    	while (i < sameleft) {
		int comp;

		comp = (*compfun)(i, sameleft);
		if (comp == 0) {
			/*
			 * Move to the "same" partition.
			 */
			/*
			 * Shift the left part of the "same" partition to
			 * the left, so that "same" elements stay in their
			 * original order.
			 */
			sameleft -= eltsize;
			qsort_swap((int *) i, (int *) sameleft, eltsize);
		} else if (comp < 0) {
			/*
			 * Stay in the "left" partition.
			 */
			i += eltsize;
		} else {
			/*
			 * Should be moved to the "right" partition.
			 * Wait until the next loop finds an appropriate
			 * place to store this element.
			 */
			break;
		}
	}

	while (j > sameright) {
		int comp;

		comp = (*compfun)(sameright, j);
		if (comp == 0) {
			/*
			 * Move to the right of the "same" partition.
			 */
			sameright += eltsize;
			qsort_swap((int *) sameright, (int *) j, eltsize);
		} else if (comp > 0) {
			/*
			 * Move to the "left" partition.
			 */
			if (i == sameleft) {
				/*
				 * Unfortunately, the "left" partition
				 * has already been fully processed, so
				 * we have to shift the "same" partition
				 * to the right to free a "left" element.
				 * This is done by moving the leftest same
				 * to the right of the "same" partition.
				 */
				sameright += eltsize;
				qsort_rotate((int *) sameleft, (int*) sameright,
					     (int *) j, eltsize);
				sameleft += eltsize;
				i = sameleft;
			} else {
				/*
				 * Swap with the "left" partition element
				 * waiting to be moved to the "right"
				 * partition.
				 */
				qsort_swap((int *) i, (int *) j, eltsize);
				j -= eltsize;
				/*
				 * Go back to the 1st loop.
				 */
				i += eltsize;
				goto again;
			}
		} else {
			/*
			 * Stay in the "right" partition.
			 */
			j -= eltsize;
		}
	}
			
	if (i != sameleft) {
		/*
		 * The second loop completed (the"right" partition is ok),
		 * but we have to go back to the first loop, and deal with
		 * the element waiting for a place in the "right" partition.
		 * Let's shift the "same" zone to the left.
		 */
		sameleft -= eltsize;
		qsort_rotate((int *) sameright, (int *) sameleft, (int *) i,
			     eltsize);
		sameright -= eltsize;
		j = sameright;
		/*
		 * Go back to 1st loop.
		 */
		goto again;
	}

	/*
	 * The partitions are correct now. Recur on the smallest side only.
	 */
	if (sameleft - left >= right - (sameright + eltsize)) {
		qsort_recur(sameright + eltsize, right, eltsize, compfun);
		/*
		 * The "right" partition is now completely sorted.
		 * The "same" partition is OK, so...
		 * Ignore them, and start the loops again on the
		 * "left" partition.
		 */
		right = sameleft;
		goto top;
	} else {
		qsort_recur(left, sameleft, eltsize, compfun);
		/*
		 * The "left" partition is now completely sorted.
		 * The "same" partition is OK, so ...
		 * Ignore them, and start the loops again on the
		 * "right" partition.
		 */
		left = sameright + eltsize;
		goto top;
	}
}

STATIC void
qsort_checker(
	char	*table,
	int	nbelts,
	int	eltsize,
	int	(*compfun)(char *, char *))
{
	char *curr, *prev, *last;

	prev = table;
	curr = prev + eltsize;
	last = table + (nbelts * eltsize);

	while (prev < last) {
		if ((*compfun)(prev, curr) > 0) {
			printf("**** qsort_checker: error between 0x%x and 0x%x!!!\n", prev, curr);
			break;
		}
		prev = curr;
		curr += eltsize;
	}
	printf("qsort_checker: OK\n");
}
