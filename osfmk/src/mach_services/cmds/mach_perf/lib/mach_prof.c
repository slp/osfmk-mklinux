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
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
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
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <mach_perf.h>


sample_cmp(s1, s2)
unsigned *s1, *s2;
{

	return(*s1 - *s2);
}

count_cmp(n1, n2)
symbol_t n1, n2;
{
	return(n1->count - n2->count);
}

sym_cmp(s1, s2)
symbol_t s1, s2;
{
	return(s1->offset - s2->offset);
}

unsigned int *
get_samples()
{
  	unsigned int *samples, *sample_pt;
	register i, count;
	sample_buf_t buf;

	if (debug > 1)
		printf("get_samples\n");


	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *)&samples,
				sample_count*(sizeof (unsigned int)),
				TRUE));

	buf = sample_bufs;
	sample_pt = samples;
	
	for (i = sample_count; i; i -= count) {
		count = samples_per_buf - buf->free_count;
		bcopy((char *)buf->samples,
		      (char *)sample_pt,
		      count * sizeof(unsigned int));
		buf = buf->next;
		sample_pt += count;
	}
	return(samples);		
}


free_samples(samples)
{
	if (debug > 1)
		printf("free_samples\n");


	MACH_CALL(vm_deallocate, (mach_task_self(),
				samples,
				sample_count*(sizeof (unsigned int))));
}

assign_samples(unsigned int *samples, symbol_t syms, unsigned nsyms)
{
	unsigned int *sample = 0;

	register i;
	symbol_t sym;

	if (debug > 1)
		printf("assign_samples\n");

	/* reset counters */

	for (sym = syms; sym->name; sym++)
		sym->count = 0;

	qsort(samples, sample_count, sizeof(*sample), sample_cmp);

	if (debug)
		printf("assigning samples\n");

	sample = samples;
	sym = syms;
	for (i=0; i < sample_count; i++) {
		while(sym < syms+nsyms && sym->offset <= *sample)
			sym++;
		if (debug) 
			printf("sample %x: %s\n",
			       *sample,
			       (sym-1)->name ? (sym-1)->name : "?");
		(sym-1)->count++;
		sample++;
	}
}

mach_prof_print()
{
	register i;
	symbol_t sym;
        symbol_t syms = (symbol_t) 0;
	float f, elapsed, cpu_time;
	int hz;
	float tot_perc;
	unsigned int *samples, *sample;
	unsigned int nsyms;
	kernel_version_t version;
	boolean_t use_kernel_symbols = TRUE;

	if (debug)
		printf("prof_print\n");

	if (!prof_opt)
		return;

	MACH_CALL(host_kernel_version, (mach_host_self(),
					&version[0]));

	samples = get_samples();

	if (!kernel_symbols_count) {
		printf("\nNo kernel symbols\n");
		use_kernel_symbols = FALSE;
	} else if (strcmp(version, kernel_symbols_version)) {
		printf("\nKernel symbol table does not match kernel version\n");
		use_kernel_symbols = FALSE;
	}		

	if (use_kernel_symbols) {
	  	/*
		 * we have a valid symbol table
		 */
	  	syms = kernel_symbols;
		nsyms = kernel_symbols_count;
	} else if (sample_count) {
	  	/*
		 * We dont have a valid dymbol table,
		 * create one that will contain at most
		 * one element per existing sample.
		 */
	  	if (!prof_trunc)
			prof_trunc = 5.0;
		MACH_CALL(vm_allocate, (mach_task_self(),
					(vm_offset_t *)&syms,
					sample_count*(sizeof (struct symbol)),
					TRUE));
		for (i=0, sample = samples, sym = syms;
		     i < sample_count;
		     i++, sample++, sym++)
	        	sym->offset = *sample;
		nsyms = sample_count;
	}
	
	if (nsyms) {
		qsort(syms, nsyms, sizeof(struct symbol), sym_cmp);
		assign_samples(samples, syms, nsyms);
	}

#if	i860	/* XXX clock_getattr not implemeted yet */
	hz = 100;
#else
	hz = hertz();
#endif
	free_samples(samples);

	elapsed = client_stats.total.xsum/1000000;

	cpu_time = 1;
	cpu_time /= hz;
	cpu_time *= sample_count;

	if (nsyms)
		qsort(syms, nsyms, sizeof(struct symbol), count_cmp);

	printf("\nTotal elapsed time (seconds): %10.3f", elapsed);
	printf("\nNumber of samples:            %10d", sample_count);
	printf("\nSampling rate (HZ):           %10d", hz);
	printf("\nEstimated cpu time (seconds): %10.3f", cpu_time);
	printf("\nCPU utilisation (%%):          %10.2f", (cpu_time/elapsed)*100);
	printf("\n\n     %%    seconds\n");
	tot_perc = 0;
	i = nsyms;
 	sym = &syms[nsyms-1];
	while(i-- && sym->count) {
		f = sym->count;
		f /= sample_count;
		if (f*100 < prof_trunc)
			break;
		tot_perc += f;
		printf("%6.2f %10.3f", f*100, cpu_time * f);
		if (sym->name) 
			printf("  %s\n", sym->name);
		else
			printf("  0x%8x\n", sym->offset);
		sym--;
	}
	if (tot_perc < 1 && sample_count)
		printf("%6.2f %10.3f  (others)\n", (1-tot_perc)*100, cpu_time * (1-tot_perc));

	printf("------ ----------\n%6.2f %10.3f  total\n\n", (float)100, (float)cpu_time);
	if ((!use_kernel_symbols) && syms) {
		MACH_CALL(vm_deallocate,
			  (mach_task_self(),
			   (vm_offset_t)syms,
			   sample_count*(sizeof (struct symbol))));
	}		
}
