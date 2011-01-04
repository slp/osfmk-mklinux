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

extern int 	debug_all;		/* debug level for all modules */
extern int 	trace_all;		/* debug level for all modules */

/*
 * Per module debug
 */

struct debug_info {
	char 	*string;
	int	level;
	int	trace;
};

struct trace_info {
	char 	*string;
};

extern int 	ndebugs;	/* number of per module debug levels */
extern int 	ntraces;	/* number of per module debug levels */
extern int 	get_debug_level(char *file);
extern void	add_debug_string(char *string, int level);
extern char 	*cur_traced_file;
extern int	debug_resources;

#define debug get_debug_level(__FILE__)
