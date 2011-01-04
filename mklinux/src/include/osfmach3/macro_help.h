/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 *	File:	osfmach3/macro_help.h
 *
 *	Provide help in making lint-free macro routines
 *
 */

#ifndef	_OSFMACH3_MACRO_HELP_H_
#define _OSFMACH3_MACRO_HELP_H_

#include <mach/boolean.h>

#define		NEVER		FALSE
#define		ALWAYS		TRUE

#define		MACRO_BEGIN	do {
#define		MACRO_END	} while (NEVER)

#define		MACRO_RETURN	if (ALWAYS) return

#endif	/*_OSFMACH3_MACRO_HELP_H_*/
