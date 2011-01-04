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
/*     
 * romopt.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * romopt.c,v
 * Revision 1.8.1.1  1994/07/22  19:40:31  menze
 * Allow null function pointers
 * Changed message on error return to not imply a format error
 *
 * Revision 1.8  1994/02/04  23:37:17  menze
 *   [ 1994/01/13          menze ]
 *   Need to include rom.h to get rom declarations
 *
 */

/* 
 * Library support for processing romfile options
 */


#include <xkern/include/xkernel.h>
#include <xkern/include/romopt.h>

int	tracerom;

#define ROM_MAX_LINES 	100 	/* Up to 100 table entries */
#define ROM_MAX_FIELDS 	20

static	char *rom[ROM_MAX_LINES + 1][ROM_MAX_FIELDS + 1];  


typedef struct {
    char	*name;
    int		minFields;
    void	*func;
} GenericRomOpt;


static xkern_return_t
	findOptions( char *, char *, GenericRomOpt *, int, void *, XObj );


static void
reportError(
	    char *errorString,
	    int	 i)
{
    int	j;

    xAssert(sizeof(errBuf) >= ROM_OPT_MAX_LEN);
    sprintf(errBuf, "Rom error: %s, resulting from entry:", errorString);
    xError(errBuf);
    strcpy(errBuf, "\t");
    for ( j=0; rom[i][j]; j++ ) {
	if ( j ) {
	    strcat(errBuf, " ");
	} 
	strcat(errBuf, rom[i][j]);
    }
    xError(errBuf);
}


static xkern_return_t
findOptions(
    char		*name,
    char		*fullName,
    GenericRomOpt	*opts,
    int			numOpts,
    void		*arg,
    XObj		obj)
{
    int			line, j, nFields;
    xkern_return_t	xkr;
    GenericRomOpt	*opt = 0;

    for ( line=0; rom[line][0]; line++ ) {
	if ( ! strcmp(rom[line][0], name) || ! strcmp(rom[line][0], fullName) ) {
	    for ( j=0; j < numOpts; j++ ) {
		opt = &opts[j];
		if ( strlen(opt->name) == 0 ) {
		    /*
		     * a default option -- issue warning if it's not
		     * the last option 
		     */
		    if ( j+1 != numOpts ) {
			sprintf(errBuf,
				"xkernel findRomOpts WARNING: "
				"%s options after default are ignored",
				fullName);
			xError(errBuf);
		    }
		    break;
		}
		if ( rom[line][1] && ! strcmp(opt->name, rom[line][1]) ) {
		    break;
		}
	    }
	    if ( j < numOpts ) {
		xAssert(opt);
		if ( ! opt->func ) {
		    continue;
		}
		for ( nFields=1; rom[line][nFields] != 0; nFields++ )
		  ;
		if ( nFields < opt->minFields ) {
		    reportError("not enough fields", line);
		    return XK_FAILURE;
		}
		if ( obj ) {
		    xkr = ((XObjRomOptFunc)opt->func)(obj, rom[line], nFields,
						      line+1, arg);
		} else {
		    xkr = ((RomOptFunc)opt->func)(rom[line], nFields, line+1,
						  arg);
		}
		if ( xkr == XK_FAILURE ) {
		    reportError("option handler failure", line);
		    return XK_FAILURE;
		}
	    } else {
		reportError("unhandled option", line);
		return XK_FAILURE;
	    }
	}
    }
    return XK_SUCCESS;
}


xkern_return_t
findXObjRomOpts(
    XObj	obj,
    XObjRomOpt 	*opt,
    int 	numOpts,
    void 	*arg)
{    
    return findOptions( obj->name, obj->fullName, (GenericRomOpt *)opt,
		        numOpts, arg, obj );
}



xkern_return_t
findRomOpts(
    char	*str,
    RomOpt 	*opt,
    int 	numOpts,
    void 	*arg)
{    
    return findOptions( str, str, (GenericRomOpt *)opt, numOpts, arg, 0 );
}


/* 
 * Save all but the last character (the newline)
 */
static char *
savestr(
	const char *s )
{
    char 	*r;

    if ( r = (char *) xSysAlloc(strlen(s) + 1) ) {
	strcpy(r, s);
    } else {
	xTrace0(rom, TR_SOFT_ERRORS, "rom allocation error");
    }
    return r;
}


#ifndef isspace
#  define isspace(c) ((c) == ' ' || (c) == '\t' || (c) == '\n')
#endif


xkern_return_t
addRomOpt( buf )
    const char	*buf;
{
    static int	i=0;
    char	*p;
    int		j;

    xTrace1(rom, TR_EVENTS, "adding rom option %s", buf);
    if ( i > ROM_MAX_LINES  ) {
	xTrace1(rom, TR_ERRORS, "ROM file has too many lines (max %d)",
		ROM_MAX_LINES);
	return XK_FAILURE;
    }
    if ( strlen(buf) > ROM_OPT_MAX_LEN ) {
	xTrace2(rom, TR_ERRORS,
		"ROM entry in line %d is too long (max %d chars)",
		i, ROM_OPT_MAX_LEN);
	return XK_FAILURE;
    }
    if ( ! (p = savestr(buf)) ) {
	return XK_NO_MEMORY;
    }
    /* 
     * Clear out initial white space
     */
    while ( *p && isspace(*p) ) {
	p++;
    }
    if ( strlen(p) == 0 ) {
	rom[i][0] = "";
    } else {
	/* 
	 * Put a '\0' after each field and set the rom array to these
	 * fields 
	 */
	for ( j=0; *p; j++ ) {
	    if ( j >= ROM_MAX_FIELDS ) {
		xTrace2(rom, TR_ERRORS,
			"ROM entry on line %d has too many fields (max %d)",
			i, ROM_MAX_FIELDS);
		return XK_FAILURE;
	    }
	    if ( *p == '#' ) {
		/* 
		 * Comments follow until end of line.  Make sure this
		 * doesn't look like the final line.
		 */
		if ( j == 0 ) {
		    rom[i][0] = p;	/* bogus, but non-null */
		    rom[i][1] = 0;
		} else {
		    rom[i][j] = 0;
		}
		break;
	    }
	    rom[i][j] = p;
	    /* 
	     * Find and mark the end of this field
	     */
	    while ( *p && ! isspace(*p) ) {
		p++;
	    }
	    if ( *p ) {
		*p++ = 0;
		/* 
		 * Find the start of the next field
		 */
		while ( *p && isspace(*p) ) {
		    p++;
		}
	    }
	}
    }
    i++;
    return XK_SUCCESS;
}
