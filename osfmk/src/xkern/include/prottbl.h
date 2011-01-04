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
 * prottbl.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * prottbl.h,v
 * Revision 1.9.2.2.1.2  1994/09/07  04:18:03  menze
 * OSF modifications
 *
 * Revision 1.9.2.2  1994/09/01  04:12:48  menze
 * Subsystem initialization functions can fail
 *
 * Revision 1.9  1993/12/15  23:06:15  menze
 * Modifications from UMass:
 *
 *   [ 93/07/27          yates ]
 *   added relProtNumById() function
 *
 */

/* 
 * Interface to the protocol table utility
 */

#ifndef prottbl_h
#define prottbl_h

#include <xkern/include/upi.h>
#include <xkern/include/idmap.h>

typedef enum {
    NO_ERROR = 0,
    ERR_FILE,
    ERR_NAME,
    ERR_ID,
    ERR_NAME_TOO_LARGE
} ErrorCode;


extern	Map	ptblNameMap;

typedef struct {
    char	*name;
    long	id;
    Map		idMap;		/* longs (id's) to relative protocol numbers */
    Map		tmpMap;		/* strings to relative protocol numbers */
    Map		revMap;		/* relative protocol numbers -> 0 */
} PtblEntry;

/* 
 * protTblAddProt -- Create an entry in the protocol map binding 'name'
 * to the given protocol id
 */
extern xkern_return_t	protTblAddProt( char *name, long id );

/* 
 * protTblAddBinding -- Indicate in the map that 'hlp' uses 'relNum' as a
 * protocol number relative to 'llp'.
 */
extern xkern_return_t	protTblAddBinding( char *llp, char *hlp, long relNum );

/* 
 * protTblBuild -- add the entries in the specified file to the table
 * returns non-zero if there were problems
 */
extern int	protTblBuild( char *filename );

/* 
 * protTblParse -- read in 'filename' and add the appropriate entries
 * to the protocol map via protTblAddProt and protTblAddBinding.
 * Returns 0 on a successful parse, non-zero if not.
 */
extern ErrorCode	protTblParse( char *filename );

/* 
 * This is used by the ptblDump utility program.  Not for general
 * consumption. 
 */
char *	protIdToStr( long );

/* 
 * return the protocol id number of the protocol by looking in the table.
 * If no entry for this protocol exists, -1 is returned.
 */
extern long	protTblGetId(char *protocolName);

/* 
 * relProtNum -- return the number of hlp relative to llp.  If this
 * number can not be determined from the table, -1 is returned.  This
 * should be considered an error.
 */
extern long	relProtNum(XObj hlp, XObj llp);

/* 
 * relProtNumById -- return the number of protocol with ID hlpId relative 
 * to llp.  If this number can not be determined from the table, -1 is 
 * returned.  This should be considered an error.
 */
extern long     relProtNumById(long hlpId, XObj llp);

extern xkern_return_t	prottbl_init( void );


#if XK_DEBUG
/* 
 * Display the contents of the protocol table map
 */
void
protTblDisplayMap(void);
#endif /* XK_DEBUG */   

#endif /* !prottbl_h */
