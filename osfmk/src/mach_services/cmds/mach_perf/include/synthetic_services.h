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

/* This service deals with the execution of multiple benchmarks in parallel,
 * in order to try and measure effects of different types of kernel
 * loading upon a given benchmark. Multiple benchmarks are executed,
 * there being a single foreground test which is measured whilst all
 * the other tests are executing.
 */

/*
 * MkLinux
 */

#ifndef _SYNTHETIC_SERVICES_H
#define _SYNTHETIC_SERVICES_H

typedef struct sync_vec {
    void (*start_time)(void);
    int (*filter)();
} sync_vec;

extern void synthetic_init(void);

/* the maximum size of the messages coming back from bg tasks */

#define BG_DATA_SIZE 256

#endif /* _SYNTHETIC_SERVICES_H */
