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
 * $RCSfile: romopt.h,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.3
 * Date: 1993/05/09 00:26:03
 */


#ifndef romopt_h
#define romopt_h

#include <xkern/include/upi.h>

#define ROM_OPT_MAX_LEN	200

typedef xkern_return_t (* XObjRomOptFunc)(
	    XObj, char **, int numFields, int line, void *);

typedef struct {
    char		*name;
    int			minFields;
    XObjRomOptFunc	func;
} XObjRomOpt;



typedef xkern_return_t	(* RomOptFunc)(
	    char **, int numFields, int line, void *);

typedef struct {
    char		*name;
    int			minFields;
    RomOptFunc		func;
} RomOpt;


/* 
 * findXObjRomOpts -- Called by a protocol that has defined an array of
 * RomOpt structures.  If the first field of a romfile line matches
 * either the protocol name or the full instance name of the XObject,
 * the romOpt array is scanned.  If the second field of the line
 * matches the name field of one of the RomOpts, or if the name field
 * of one of the romOpts is the empty string, the RomOptFunc for that
 * option is called with the object, all rom fields on that line, the
 * number of fields on that line, the line number and the
 * user-supplied argument.
 *
 * If the first field of a rom line appears to match the XObject but
 * none of the supplied RomOpts matches the second field, an error
 * message will be printed and XK_FAILURE will be returned.  The rest
 * of the rom entries will not be scanned.  This same behaviour
 * results from the RomOptFunc returning XK_FAILURE.
 *
 * Note that default (empty-string) RomOpts must come last in the
 * RomOpt array.
 */
xkern_return_t	findXObjRomOpts(XObj, XObjRomOpt *, int numOpts, void *arg);


xkern_return_t	findRomOpts(char *, RomOpt *, int numOpts, void *arg);


/* 
 * Add an option to the associative rom table.  An option is a
 * space-separated string of words, with the first word used to
 * indicate the subsystem, protocol, or protocol instance that should
 * interpret the rest of the option.
 *
 * The option must not exceed ROM_OPT_MAX_LEN characters.
 */
xkern_return_t	addRomOpt( 
			  const char *buf );

xkern_return_t	initRom( void );


#endif /* ! romopt_h */
