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
 * prottbl.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * prottbl.c,v
 * Revision 1.40.1.3.1.2  1994/09/07  04:18:20  menze
 * OSF modifications
 *
 * Revision 1.40.1.4  1994/09/06  21:52:22  menze
 * One of the mapBinds in protTblAddBinding had disappeared in the
 * meta-data transformation.
 *
 * Revision 1.40.1.3  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.40.1.2  1994/09/01  18:49:59  menze
 * Meta-data allocations now use Allocators and Paths
 * Subsystem initialization functions can fail
 *
 * Revision 1.40  1994/02/05  00:08:18  menze
 *   [ 1994/01/28          menze ]
 *   assert.h renamed to xk_assert.h
 *
 * Revision 1.39  1993/12/15  23:32:33  menze
 * Modifications from UMass:
 *
 *   [ 93/07/27          yates ]
 *     added relProtNumById(hlpId, llp)
 *
 * Revision 1.38  1993/12/11  00:23:44  menze
 * fixed #endif comments
 *
 * Revision 1.37  1993/12/10  20:39:20  menze
 * Added unique_hlps ROM option
 *
 * Cleaned up initialization interaction with prottbl_parse.
 * prottbl_init and ROM parsing now live here.
 *
 * Revision 1.36  1993/12/10  18:15:34  menze
 * A protocol can now have multiple protocols bound to the same hlpNum
 */

/*
 * Management of the "protocol id / relative number" table
 */

#include <xkern/include/domain.h>
#include <xkern/include/prottbl.h>
#include <xkern/include/idmap.h>
#include <xkern/include/xk_debug.h>
#include <xkern/include/assert.h>
#include <xkern/include/compose.h>
#include <xkern/include/romopt.h>
#include <xkern/include/xk_path.h>

typedef PtblEntry	Entry;

int	traceptbl;

#define PTBL_MAP_SIZE		101
#define PTBL_NAME_MAP_SIZE	101
#define PTBL_ID_MAP_SIZE	101
#define PTBL_HLP_MAP_SIZE	10

char *	protIdToStr( long );

static int		backPatchBinding( void *, void * , void * );
static int		checkEntry( void *, void *, void * );
static xkern_return_t	configFileOpt( char **, int, int, void * );
static void		error( char * );
static void		mkKey( char *, char * );
static xkern_return_t	uniqueHlpNumOpt( char **, int, int, void * );

#if	XK_DEBUG

static int	dispHlp( void *, void *, void * );
static int	dispEntry( void *, void *, void * );

#endif /* XK_DEBUG */


Map	ptblNameMap = 0;	/* strings to Entry structures */
static Map	idMap = 0;
static int	errorOccurred = 0;
static int	ptblMapIsEmpty = 1;
static int	uniqueHlpNums = 0;
static char *	romTables[100];

static RomOpt romOptions[] = {
    { "unique_hlps", 3, uniqueHlpNumOpt },
    { "", 2, configFileOpt }
};


#define MAX_PROT_NAME	16
/* 
 * Error messages
 */
#define MULT_PROT_MSG   "prot table: multiple protocols with id %d declared"
#define NO_ID_MSG     "prot table: %s (declared as hlp to %s) has no id number"
#define NAME_REPEATED_MSG  "prot table: protocol %s is declared with different id's"
#define BIND_REPEATED_MSG  "prot table: binding for %s above %s defined multiple times"
#define NO_ENTRY_MSG "prot table: No entry exists for llp %s"
#define BIND_ERROR_MSG  "prot table: map binding error"

#define NUM_BOUND_MSG "prot table: relative number %d is already bound for llp %s"

static void
error(
    char	*msg)
{
    xError(msg);
    errorOccurred = 1;
}


static void
mkKey(
    char	*key,
    char	*name)
{
    bzero(key, MAX_PROT_NAME);
    strncpy(key, name, MAX_PROT_NAME);
}



/* 
 * This function is called for each entry in a lower protocol's temp
 * map (binding higher protocol name strings to relative numbers.)
 * We find the hlp's id number and bind it to the relative number,
 * signalling an error if we can't find the hlp's id number.
 */
static int
backPatchBinding(
    void	*key,
    void	*relNum,
    void	*arg)
{
    Entry	*llpEntry = (Entry *)arg;
    Entry	*hlpEntry;
    
    if ( mapResolve(ptblNameMap, key, &hlpEntry) == XK_FAILURE ) {
	sprintf(errBuf, NO_ID_MSG, (char *)key, llpEntry->name);
	error(errBuf);
    } else {
	xTrace3(ptbl, TR_MAJOR_EVENTS,
		"Backpatching binding of %s->%s using %d",
		hlpEntry->name, llpEntry->name, (int)relNum);
	if ( mapBind(llpEntry->idMap, &hlpEntry->id, relNum, 0)
	    	!= XK_SUCCESS ) {
	    error(BIND_ERROR_MSG);
	}
    }
    return MFE_CONTINUE;
}


/* 
 * This function is called for each entry (lower protocol) in the name
 * map.  If there are entries in the llp's tmpMap (higher protocols
 * which were bound with a string instead of an id), we backpatch the
 * id binding.
 */
static int
checkEntry(
    void	*key,
    void	*value,
    void	*arg)
{
    Entry	*llpEntry = (Entry *)value;

    /* 
     * If there is anything in this entry's tmpMap, transfer it to the
     * idMap.  If we don't know the id number of the hlp in the
     * tmpMap, signal an error.
     */
    if ( llpEntry->tmpMap ) {
	mapForEach(llpEntry->tmpMap, backPatchBinding, llpEntry);
	mapClose(llpEntry->tmpMap);
	llpEntry->tmpMap = 0;
    }
    return MFE_CONTINUE;
}


int
protTblBuild(
    char	*filename)
{
    xTrace1(ptbl, TR_GROSS_EVENTS, "protTblBuild( %s )", filename);
    xAssert( ptblNameMap );
    if ( (protTblParse(filename)) != 0 || errorOccurred ) {
        xTrace1(ptbl, TR_ERRORS,
		"protTblBuild: problems building input file %s", filename);
	return -1;
    }
    xTrace1(ptbl, TR_GROSS_EVENTS,
	"protTblBuild( %s ) checking map consistency", filename);
    /* 
     * Check the consistency of the protocol map.  
     */
    mapForEach(ptblNameMap, checkEntry, 0);
    xIfTrace(ptbl, TR_ERRORS) {
	if (errorOccurred)
	    printf("prottbl: protTblBuild: Consistency error in file %s",
		    filename);
    }
    return errorOccurred;
}


long
protTblGetId(
    char	*protocolName)
{
    Entry	*e;
    char	key[MAX_PROT_NAME];
    
    xAssert(ptblNameMap);
    mkKey(key, protocolName);
    if ( mapResolve(ptblNameMap, key, &e) == XK_FAILURE ) {
	xTrace0(ptbl, TR_ERRORS, "protTblGetId failed to find key");
	if ( ptblMapIsEmpty ) {
	    xError("No protocol tables have been loaded");
	}
	return -1;
    }
    return e->id;
}


char *
protIdToStr(
    long	id)
{
    Entry	*e;

    if ( mapResolve(idMap, &id, &e) == XK_FAILURE ) {
	return 0;
    }
    return e->name;
}


long
relProtNum(
    XObj	hlp,
    XObj	llp)
{
    Entry	*llpEntry;
    long	res;
    
    xAssert(ptblNameMap);
    if ( ! ( xIsProtocol(hlp) && xIsProtocol(llp) ) ) {
	xIfTrace(ptbl, TR_ERRORS) {
	    if ( !(xIsProtocol(hlp))) printf("prottbl: relProtNum: hlp is not a protocol\n");
	    if ( !(xIsProtocol(llp))) printf("prottbl: relProtNum: llp is not a protocol\n");
	}
	return -1;
    }
    if ( mapResolve(idMap, &llp->id, &llpEntry) == XK_FAILURE ) {
	xTrace1(ptbl, TR_ERRORS, "prottbl: relProtNum: Cannot find llp id %d in protocol table", llp->id);
	return -1;
    }
    if ( llpEntry->idMap == 0 ) {
	/* 
     * The lower protocol does not use relative naming.  Return the
     * absolute id of the upper protocol.
     */
	xTrace1(ptbl, TR_EVENTS, "prottbl: relProtNum: Returning absolute id of upper protocol, %d", hlp->id);
	return hlp->id;
    }
    if ( mapResolve(llpEntry->idMap, &hlp->id, &res) == XK_FAILURE ) {
	res = -1;
    }
    xTrace1(ptbl, TR_EVENTS, "prottbl: relProtNum: Returning %d", res);
    return res;
}


long
relProtNumById(hlpId, llp)
    long hlpId;
    XObj llp;
{
    Entry	*llpEntry;
    long	res;
    
    xAssert(ptblNameMap);
    if ( hlpId < 0 ) {
        xTrace1(ptbl, TR_ERRORS,
	        "prottbl: relProtNumById: hlp id (%d) is invalid", hlpId);
	return -1;
    }
    if ( !xIsProtocol(llp) ) {
        xTrace0(ptbl, TR_ERRORS,
	        "prottbl: relProtNumById: llp is not a protocol");
	return -1;
    }
    if ( mapResolve(idMap, &llp->id, &llpEntry) == XK_FAILURE ) {
        xTrace1(ptbl, TR_ERRORS, "prottbl: relProtNumById: Cannot find llp id %d in protocol table", llp->id);
        return -1;
    }
    if ( llpEntry->idMap == 0 ) {
	/* 
	 * The lower protocol does not use relative naming.  Return the
	 * absolute id of the upper protocol.
	 */
        xTrace1(ptbl, TR_EVENTS, "prottbl: relProtNumById: Returning absolute id of upper protocol, %d", hlpId);
        return hlpId;
    }
    if ( mapResolve(llpEntry->idMap, &hlpId, &res) == XK_FAILURE ) {
	res = -1;
    }
    xTrace1(ptbl, TR_EVENTS, "prottbl: relProtNumById: Returning %d", res);
    return res;
}


xkern_return_t
protTblAddProt(
    char	*name,
    long int	id)
{
    Entry	*e;
    char	key[MAX_PROT_NAME];
    
    xTrace2(ptbl, TR_MAJOR_EVENTS, "protTblAddProt adding %s(%d)", name, id);
    ptblMapIsEmpty = 0;
    mkKey(key, name);
    if ( mapResolve(ptblNameMap, key, &e) == XK_FAILURE ) {
	/* 
	 * Entry does not exist for this name
	 */
	if ( mapResolve(idMap, (void *)&id, 0) == XK_SUCCESS ) {
	    sprintf(errBuf, MULT_PROT_MSG, id);
	    error(errBuf);
	    return XK_FAILURE;
	}
	if ( ! (e = (Entry *)xSysAlloc(sizeof(Entry))) ) {
	    xError("prottbl allocation error");
	    return XK_FAILURE;
	}
	bzero((char *)e, sizeof(Entry));
	if ( ! (e->name = (char *)xSysAlloc(strlen(name) + 1)) ) {
	    xError("prottbl allocation error");
	    allocFree(e);
	    return XK_FAILURE;
	}
	strcpy(e->name, name);
	e->id = id;
	if ( mapBind(ptblNameMap, key, e, 0 ) == XK_SUCCESS ) {
	    if ( mapBind(idMap, &e->id, e, 0) == XK_SUCCESS ) {
		return XK_SUCCESS;
	    }
	    mapUnbind(ptblNameMap, key);
	}
	allocFree(e->name);
	allocFree(e);
	error(BIND_ERROR_MSG);
	return XK_FAILURE;
    } else {
	/* 
	 * Make sure that this declaration has the same id number as
	 * the previous declarations.
	 */
	if ( e->id != id ) {
	    sprintf(errBuf, NAME_REPEATED_MSG, name);
	    error(errBuf);
	    return XK_FAILURE;
	}
    }
    return XK_SUCCESS;
}


static xkern_return_t
mkMaps( Entry *e )
{
    if ( ! e->idMap ) {
	e->idMap = mapCreate(PTBL_HLP_MAP_SIZE, sizeof(long), xkSystemPath);
	if ( ! e->idMap ) return XK_FAILURE;
    }
    if ( ! e->tmpMap ) {
	e->tmpMap = mapCreate(PTBL_HLP_MAP_SIZE, MAX_PROT_NAME, xkSystemPath);
	if ( ! e->tmpMap ) return XK_FAILURE;
    }
    if ( uniqueHlpNums ) {
	if ( ! e->revMap ) {
	    e->revMap = mapCreate(PTBL_HLP_MAP_SIZE, sizeof(long),
				  xkSystemPath);
	    if ( ! e->revMap ) return XK_FAILURE;
	}
    }
    return XK_SUCCESS;
}


xkern_return_t
protTblAddBinding(
    char	*llpName,
    char	*hlpName,
    long int	relNum)
{
    Entry	*llpEntry, *hlpEntry;
    char	key[MAX_PROT_NAME];
    
    xTrace3(ptbl, TR_MAJOR_EVENTS, "protTblAddBinding adding %s->%s uses %d",
	    hlpName, llpName, relNum);
    mkKey(key, llpName);
    if ( mapResolve(ptblNameMap, key, &llpEntry) == XK_FAILURE ) {
	/* 
	 * No entry exists for the lower protocol
	 */
	sprintf(errBuf, NO_ENTRY_MSG, llpName);
	error(errBuf);
	return XK_FAILURE;
    }
    if ( mkMaps(llpEntry) == XK_FAILURE ) {
	xTrace0(ptbl, TR_ERRORS, "protTblAddbinding ... trouble creating maps");
	return XK_FAILURE;
    }
    if ( uniqueHlpNums ) {
	if ( mapResolve(llpEntry->revMap, &relNum, 0) == XK_SUCCESS ) {
	    sprintf(errBuf, NUM_BOUND_MSG, relNum, llpName);
	    error(errBuf);
	    return XK_FAILURE;
	}
    }
    mkKey(key, hlpName);
    if ( mapResolve(ptblNameMap, key, &hlpEntry) == XK_FAILURE ) {
	/* 
	 * Entry for hlp doesn't exist yet.  Add the binding from the
	 * hlp name to the number in tmpMap.  The binding will be
	 * tranferred to idMap after the hlp entry has been added.
	 */
	if ( mapResolve(llpEntry->tmpMap, key, 0) == XK_FAILURE ) {
	    if ( mapBind(llpEntry->tmpMap, key, (void *)relNum, 0)
			!= XK_SUCCESS ) {
		error(BIND_ERROR_MSG);
		return XK_FAILURE;
	    }
	    if ( uniqueHlpNums ) {
		if ( mapBind(llpEntry->revMap, &relNum, 0, 0) != XK_SUCCESS ) {
		    mapUnbind(llpEntry->tmpMap, key);
		    error(BIND_ERROR_MSG);
		    return XK_FAILURE;
		}
	    }
	} else {
	    sprintf(errBuf, BIND_REPEATED_MSG, hlpName, llpName);
	    error(errBuf);
	    return XK_FAILURE;
	}
    } else {
	/* 
	 * Add the entry directly to the idMap
	 */
	if ( mapResolve(llpEntry->idMap, &hlpEntry->id, 0) == XK_FAILURE ) {
	    if ( mapBind(llpEntry->idMap, &hlpEntry->id, (void *)relNum, 0)
			!= XK_SUCCESS ) {
		error(BIND_ERROR_MSG);
		return XK_FAILURE;
	    }
	    if ( uniqueHlpNums ) {
		if ( mapBind(llpEntry->revMap, &relNum, 0, 0) != XK_SUCCESS ) {
		    mapUnbind(llpEntry->idMap, &hlpEntry->id);
		    error(BIND_ERROR_MSG);
		    return XK_FAILURE;
		}
	    }
	} else {
	    sprintf(errBuf, BIND_REPEATED_MSG, hlpName, llpName);
	    error(errBuf);
	    return XK_FAILURE;
	}
    }
    return XK_SUCCESS;
}


static xkern_return_t
uniqueHlpNumOpt( str, nFields, ln, arg )
    char	**str;
    int		nFields;
    int		ln;
    void	*arg;
{
    if ( ! strcmp(str[2], "on") ) {
	uniqueHlpNums = 1;
	return XK_SUCCESS;
    }
    if ( ! strcmp(str[2], "off") ) {
	uniqueHlpNums = 0;
	return XK_SUCCESS;
    }
    return XK_FAILURE;
}


static xkern_return_t
configFileOpt( str, nFields, ln, arg )
    char	**str;
    int		nFields;
    int		ln;
    void	*arg;
{
    static int	i = 0;

    if ( i >= sizeof(romTables) ) {
	xError("Too many protocol tables in ROM file");
    } else {
	romTables[i++] = str[1];
    }
    return XK_SUCCESS;
}


xkern_return_t
prottbl_init(void)
{
    static int	initialized;
    char **tables;

    if ( initialized ) {
	return XK_SUCCESS;
    }
    initialized = 1;
    romTables[0] = 0;
    findRomOpts("prottbl", romOptions, sizeof(romOptions)/sizeof(RomOpt), 0);
    ptblNameMap = mapCreate(PTBL_NAME_MAP_SIZE, MAX_PROT_NAME, xkSystemPath);
    idMap = mapCreate(PTBL_ID_MAP_SIZE, sizeof(long), xkSystemPath);
    if ( ! ptblNameMap || ! idMap ) {
	xTrace0(ptbl, TR_ERRORS, "prottbl_init: map creation failure");
	return XK_FAILURE;
    }
    /* 
     * Config files named in compose files
     */
    for ( tables = protocolTables; *tables != 0; tables++ ) {
	if ( protTblBuild(*tables) ) {
	    sprintf(errBuf, "error building protocol table %s", *tables);
	    xError(errBuf);
	    return XK_FAILURE;
	}
    }
    /* 
     * Config files named in ROM files
     */
    for ( tables = romTables; *tables != 0; tables++ ) {
	if ( protTblBuild(*tables) ) {
	    sprintf(errBuf, "error building protocol table %s", *tables);
	    xError(errBuf);
	    return XK_FAILURE;
	}
    }
    return XK_SUCCESS;
}


#if	XK_DEBUG

static int
dispHlp(
    void	*key,
    void	*relNum,
    void	*arg)
{
    int	*hlpId = (int *)key;

    xTrace2(ptbl, TR_ALWAYS,
	"      hlp id %d uses rel num %d", *hlpId, (int)relNum);
    return MFE_CONTINUE;
}


static int
dispEntry(
    void	*key,
    void	*value,
    void	*arg)
{
    Entry	*e = (Entry *)value;

    xTrace2(ptbl, TR_ALWAYS, "%s (id %d)", e->name, e->id);
    if (e->idMap) {
	xTrace0(ptbl, TR_ALWAYS, "  upper protocols:");
	mapForEach(e->idMap, dispHlp, 0);
    }
    return 1;
}


void
protTblDisplayMap()
{
    xTrace0(ptbl, TR_ALWAYS, "protocol table:");
    mapForEach(ptblNameMap, dispEntry, 0);
}

#endif /* XK_DEBUG */
