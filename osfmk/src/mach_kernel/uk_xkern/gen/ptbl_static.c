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
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 */

#include <xk_debug.h>
#include <xkern/include/prottbl.h>

typedef struct {
    char	*name;
    long	relNum;
} MapEntry;


typedef struct {
    char 	*name;
    long	id;
    MapEntry	*map;
} Entry;


char *
protocolTables[2] = {
    "in-kernel", 0	/* dummy name */
};


#include <uk_xkern/ptblData.c>


ErrorCode
protTblParse(
    char	*s)
{
    static int	done = 0;
    Entry 	*e = entries;
    MapEntry	*mapEnt;

    printf("protTblParse called\n");
    if ( ! done ) {
	done = 1;
	for ( e = entries; e->name; e++ ) {
	    protTblAddProt(e->name, e->id);
	    if ( e->map ) {
		for ( mapEnt = e->map; mapEnt->name; mapEnt ++ ) {
		    protTblAddBinding(e->name, mapEnt->name, mapEnt->relNum);
		}
	    }
	}
    }
    return NO_ERROR;
}


