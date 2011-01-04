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

reset_sampling(stats)
struct stats *stats;
{
	stats->x2sum = 0;
	stats->xsum = 0;
	stats->n = 0;
}

collect_sample(stats, x)
struct stats *stats;
double x;
{
  	stats->x2sum += (x * x);
	stats->xsum += x;
	stats->n++;
}

double
average(stats)
struct stats *stats;
{
	return(stats->xsum/stats->n);
}

double sqrt(double);

double
deviation(stats)
struct stats *stats;
{
	double ret;
	ret = stats->x2sum - (stats->xsum * stats->xsum)/stats->n;
	ret /= stats->n;
	if (ret < 0)
		ret = -ret;
	return  sqrt(ret);

}
	
random(n, addr, repeat)
int *addr;
{
	int *int_list, index;
	register i;
	int N = n * (1 + repeat );

	if (debug > 1)
		printf("\nrandom(%d, %x, %d):", n, addr, repeat);

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&int_list,
				  n*sizeof(int),
				  TRUE));
	for (i=0; i<n; i++)
		int_list[i] = 0;
	for (i=0; i<N; i++) {
		index = rand() % n;
		while (int_list[index] > repeat)
			if (++index >= n)
				index = 0;
		int_list[index]++;
		if (debug > 2)
			printf(" %d", index);
		*addr++ = index;
	}
	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t)int_list,
				  n*sizeof(int)));
}

int 
is_infinity(double d)
{
	return (d == d/0.0);

}

int 
is_nan(volatile double d)
{
	volatile double f = d;
	return(d != f);
}

double
modf(double x, volatile double *y)
{
	if (is_nan(x))
		return x;

	if (x == fabs(x))
		*y = floor(x);
	else
		*y = ceil(x);

	return x - *y;
}
