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

#ifndef _PROF_SERVICES_H
#define _PROF_SERVICES_H

struct sample_buf {
	unsigned free_count;
	unsigned *next_sample;
	struct sample_buf *next;
	unsigned samples[1];
};

typedef struct sample_buf *sample_buf_t;

extern sample_buf_t 	sample_bufs;
extern int		sample_count;
extern int		samples_per_buf;
#define PROF_ALLOC_UNIT (4*vm_page_size)


struct symbol {
	vm_offset_t	offset;
	char		*name;
	unsigned int	count;
};

typedef struct symbol *symbol_t;

extern float		prof_trunc;

extern	char		prof_command[];

extern	struct symbol 	kernel_symbols[];
extern	unsigned int	kernel_symbols_count;
extern  char		*kernel_symbols_version;

extern int prof_reset();
extern int prof_drop();
extern int prof_save();

extern void (*prof_print_routine)();
#define prof_print() {			\
       if (prof_print_routine)		\
	       (*prof_print_routine)();	\
}

extern int prof_init();
extern int prof_terminate();

extern mach_port_t	prof_cmd_port;

#endif /* _PROF_SERVICES_H */
