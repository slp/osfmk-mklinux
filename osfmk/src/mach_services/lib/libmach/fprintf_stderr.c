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
 * 
 */
/*
 * MkLinux
 */

#include <mach.h>
#include <mach_init.h>
#include <stdio.h>
#include <stdarg.h>

int (*vprintf_stderr_func)(const char *format, va_list ap);


/* This function allows the writing of a mach error message to an
 * application-controllable output method, the default being to
 * use printf if no other method is specified by the application.
 *
 * To override, set the global (static) function pointer vprintf_stderr to
 * a function which takes the same parameters as vprintf.
 */

int fprintf_stderr(const char *format, ...)
{
        va_list args;
	int retval;

	va_start(args, format);
	if (vprintf_stderr_func == NULL)
		retval = vprintf(format, args);
	else
		retval = (*vprintf_stderr_func)(format, args);
	va_end(args);

	return retval;
}
