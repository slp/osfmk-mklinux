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
 * path.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * path.c,v
 * Revision 1.1.2.3.1.2  1994/09/07  04:18:16  menze
 * OSF modifications
 *
 * Revision 1.1.2.3  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.1.2.2  1994/09/01  18:48:08  menze
 * Meta-data allocations now use Allocators and Paths
 * Subsystem initialization functions can fail
 * Default path uses separate message and meta-data allocators
 * *AdjustMin* becomes *Reserve*
 *
 * Revision 1.1  1994/07/22  19:39:25  menze
 * Initial revision
 *
 */

#include <xkern/include/xk_path.h>
#include <xkern/include/romopt.h>
#include <xkern/include/xk_debug.h>

#define PATH_WC_ID_MAX	0xfffffffe
#define PATH_WC_ID_MIN	0x80000000

#define ALLOC_MAP_SIZE	8
#define PATH_MAP_SIZE	8

#define POOL_MAX_DRIVERS	10

typedef struct {
    Path_s path;
    /* 
     * Drivers which this path has told to establish input pools.
     */
    u_int driverMask;
} PathInfo;

#define setBit(mask, i)		do { (mask) |= (1 << (i)); } while(0)
#define isBitSet(mask, i) 	((mask) & (1 << (i)))

typedef struct {
    XObj		obj;
    DevInputFunc	inputFunc;
    DevPoolFunc		poolFunc;
} DriverInfo;

DriverInfo drivers[POOL_MAX_DRIVERS + 1];

static	Map	idMap, nameMap;
static	Path_s	bsPath;

int	tracepath;
Path	xkDefaultPath;
Path	xkSystemPath = &bsPath;


/* 
 * path create NAME ID [ MSG_ALLOC ] [ MD_ALLOC ]
 */
static xkern_return_t
readPath( char **str, int nFields, int line, void *arg )
{
    xk_u_int32	id = 0;
    int		n;
    Path	p;
    Allocator	a;

    n = atoi(str[3]);
    if ( n < 0 ) {
	return XK_FAILURE;
    }
    id = (xk_u_int32)n;
    if ( (p = pathCreate(id, str[2])) == 0 ) {
	xTrace2(path, TR_ERRORS,
		"couldn't create path %s (%d) from rom entry",
		str[2], id);
	return XK_FAILURE;
    }
    if ( nFields > 4 ) {
	if ( (a = getAllocByName(str[4])) == 0 ) {
	    return XK_FAILURE;
	}
	if ( pathSetAlloc(p, MSG_ALLOC, a) == XK_FAILURE ) {
	    xTrace2(path, TR_ERRORS,
		    "couldn't set allocator %s for path %s", str[4], str[2]);
	    return XK_FAILURE;
	}
    }
    if ( nFields > 5 ) {
	if ( (a = getAllocByName(str[5])) == 0 ) {
	    return XK_FAILURE;
	}
	if ( pathSetAlloc(p, MD_ALLOC, a) == XK_FAILURE ) {
	    xTrace2(path, TR_ERRORS,
		    "couldn't set allocator %s for path %s", str[5], str[2]);
	    return XK_FAILURE;
	}
    }
    return XK_SUCCESS;
}


/* 
 * path input_pool PATH_NAME [BUFFERS [THREADS [INTERFACE_OBJ_NAME]]]
 */
static xkern_return_t
readPoolRom( char **str, int nFields, int line, void *arg )
{
    int		nBuffs, nThreads;
    PathInfo	*pi;
    int		i = *(int *)arg;
    DriverInfo	*di = &drivers[i];

    xAssert(xIsXObj(di->obj));
    if ( nFields > 5 &&
	 strcmp(di->obj->fullName, str[5]) &&
	 strcmp(di->obj->name, str[5]) ) {
	/* 
	 * object name mismatch
	 */
	return XK_SUCCESS;
    }
    if ( mapVarResolve(nameMap, str[2], strlen(str[2]), (void **)&pi)
		== XK_FAILURE ) {
	xTrace1(path, TR_ERRORS, "readPoolRom: Couldn't find path %s", str[2]);
	return XK_FAILURE;
    }
    nBuffs = (nFields > 3) ? atoi(str[3]) : 0;
    nThreads = ( nFields > 4 ) ? atoi(str[4]) : 0;
    if ( nBuffs < 0 || nThreads < 0 ) {
	return XK_FAILURE;
    }
    if ( di->poolFunc(di->obj, &pi->path, nBuffs, nThreads, 0) == XK_FAILURE ) {
	return XK_FAILURE;
    }
    setBit(pi->driverMask, i);
    return XK_SUCCESS;
}


/* 
 * msg input PATH_NAME N SIZE [INTERFACE_OBJ_NAME]
 */
static xkern_return_t
readInputRom( char **str, int nFields, int line, void *arg )
{
    PathInfo	*pi;
    int		numMsgs;
    int		msgSize;
    int		i = *(int *)arg;
    DriverInfo	*di = &drivers[i];

    xAssert(xIsXObj(di->obj));
    if ( nFields > 5 &&
	 strcmp(di->obj->fullName, str[5]) &&
	 strcmp(di->obj->name, str[5]) ) {
	/* 
	 * object name mismatch
	 */
	return XK_SUCCESS;
    }
    if ( mapVarResolve(nameMap, str[2], strlen(str[2]), (void **)&pi)
		== XK_FAILURE ) {
	xTrace1(path, TR_ERRORS, "readBuffersRom: Couldn't find path %s", str[2]);
	return XK_FAILURE;
    }
    if ( ! isBitSet(pi->driverMask, i) ) {
	return XK_SUCCESS;
    }
    numMsgs = atoi(str[3]);
    msgSize = atoi(str[4]);
    if ( msgSize < 0 ) return XK_FAILURE;
    xTrace3(alloc, TR_EVENTS, "msg ROM option: input_msgs %s %d %d",
	    str[2], msgSize, numMsgs);
    return di->inputFunc(di->obj, &pi->path, msgSize, numMsgs);
}


static RomOpt	pathOpts[] = {
    {"input_pool", 0, 0},
    {"create", 4, readPath}
};


static RomOpt	inputOpts[] = {
    {"input_pool", 3, readPoolRom},
    {"create", 0, 0}
};


static RomOpt	msgOpts[] = {
    {"input", 5, readInputRom},
    {"", 0, 0}
};



Path
pathCreate( xk_u_int32 id, char *name )
{
    PathInfo		*pi;
    static xk_u_int32	nextId = PATH_WC_ID_MIN;
    xk_u_int32		firstId;
    xkern_return_t	xkr;

    if ( id == PATH_ANY_ID ) {
	firstId = nextId;
	do {
	    id = nextId;
	    if ( nextId >= PATH_WC_ID_MAX ) {
		nextId = PATH_WC_ID_MIN;
	    } else {
		nextId++;
	    }
	    xkr = mapResolve(idMap, &id, 0);
	} while ( xkr == XK_FAILURE && nextId != firstId );
	if ( xkr == XK_FAILURE ) {
	    xTrace0(path, TR_SOFT_ERRORS,
		    "pathCreate: wildcard namespace full!");
	    return 0;
	}
    }
    if ( mapResolve(idMap, &id, 0) == XK_SUCCESS ||
	 mapVarResolve(idMap, name, strlen(name), 0) == XK_SUCCESS ) {
	xTrace2(path, TR_SOFT_ERRORS, "id/name for path %s (%d) not unique",
		name, id);
	return 0;
    }
    if ( ! (pi = xSysAlloc(sizeof(PathInfo))) ) {
	return 0;
    }
    bzero((char *)pi, sizeof(PathInfo));
    pi->path.id = id;
    xAssert(xkDefaultAllocator);
    pi->path.sysAlloc[MSG_ALLOC] = xkDefaultMsgAllocator;
    pi->path.sysAlloc[MD_ALLOC] = xkDefaultAllocator;
    xAssert(idMap && nameMap);
    if ( mapBind(idMap, &id, pi, 0) != XK_SUCCESS ) {
	xTrace1(path, TR_ERRORS, "error binding path %s", name);
	allocFree(pi);
	return 0;
    }
    if ( mapVarBind(nameMap, name, strlen(name), pi, 0) != XK_SUCCESS ) {
	xTrace1(path, TR_ERRORS, "error binding path %s", name);
	mapUnbind(idMap, &id);
	allocFree(pi);
	return 0;
    }
    return &pi->path;
}

xkern_return_t
pathSetAlloc( Path p, PathAllocId id, Allocator a )
{
    Allocator		oldAlloc;
    xkern_return_t	xkr;

    if ( id < LAST_SYS_ALLOC ) {
	p->sysAlloc[id] = a;
	return XK_SUCCESS;
    }
    if ( ! p->allocMap ) {
	if ( p->sysAlloc[MD_ALLOC] == 0 ) {
	    return XK_FAILURE;
	}
	p->allocMap = mapCreate(ALLOC_MAP_SIZE, sizeof(PathAllocId), p);
	if ( ! p->allocMap ) {
	    xTrace0(path, TR_SOFT_ERRORS, "pathSetAlloc couldn't allocate map");
	    return XK_FAILURE;
	}
    }
    if ( mapResolve(p->allocMap, &id, &oldAlloc) == XK_SUCCESS ) {
	if ( oldAlloc == a ) {
	    return XK_SUCCESS;
	}
	xkr = mapUnbind(p->allocMap, &id);
	xAssert( xkr == XK_SUCCESS );
    }
    return (mapBind(p->allocMap, &id, a, 0) == XK_SUCCESS) ?
      		XK_SUCCESS : XK_FAILURE;
}


Allocator
pathGetAlloc_lookup( Path p, PathAllocId id )
{
    Allocator	alloc;

    if ( ! p->allocMap ) {
	return 0;
    }
    if ( mapResolve(p->allocMap, &id, &alloc) == XK_FAILURE ) {
	return 0;
    } 
    return alloc;
}


xkern_return_t
pathBootStrap( void )
{
    bsPath.sysAlloc[MSG_ALLOC] = xkSystemAllocator;
    bsPath.sysAlloc[MD_ALLOC] = xkSystemAllocator;
    return XK_SUCCESS;
}


xkern_return_t
pathInit( void )
{
    bsPath.sysAlloc[MSG_ALLOC] = xkSystemAllocator;
    bsPath.sysAlloc[MD_ALLOC] = xkSystemAllocator;
    idMap = mapCreate(PATH_MAP_SIZE, sizeof(xk_u_int32), xkSystemPath);
    nameMap = mapCreate(PATH_MAP_SIZE, -1, xkSystemPath);
    if ( ! idMap || ! nameMap ) {
	xTrace0(path, TR_ERRORS, "x-kernel pathInit couldn't create maps");
	return XK_FAILURE;
    }
    if ( (xkDefaultPath = pathCreate(0, "default")) == 0 ) {
	xTrace0(path, TR_ERRORS, "pathInit couldn't create default path");
	return XK_FAILURE;
    }
    findRomOpts("path", pathOpts, sizeof(pathOpts)/sizeof(RomOpt), 0);
    return XK_SUCCESS;
}


Path
getPathByName( char *n )
{
    void	*pi;

    if ( ! n || ! *n ) {
	return xkDefaultPath;
    }
    if ( mapVarResolve(nameMap, n, strlen(n), &pi) == XK_FAILURE ) {
	return 0;
    }
    return &((PathInfo *)pi)->path;
}


Path
getPathById( xk_u_int32	id )
{
    Path	pi;

    if ( mapResolve(idMap, &id, &pi) == XK_FAILURE ) {
	return 0;
    }
    return &((PathInfo *)pi)->path;
}


xkern_return_t
pathReserve(
    Path	p,
    int		nBuffs,
    u_int 	size)
{
    xAssert(p->sysAlloc[MD_ALLOC]);
    return allocReserve(p->sysAlloc[MD_ALLOC], nBuffs, &size);
}
	    


void 
pathRegisterDriver( XObj obj, DevPoolFunc poolFunc, DevInputFunc inFunc )
{
    int	i;
    
    xAssert(xIsXObj(obj));
    for ( i=0; drivers[i].obj; i++ ) {
	if ( drivers[i].obj == obj ) {
	    xTrace1(path, TR_SOFT_ERRORS,
		    "driver %s already registered with path module",
		    obj->fullName);
	    return;
	}
	if ( i >= POOL_MAX_DRIVERS ) {
	    xError("Too many drivers in pathRegisterDriver");
	    return;
	}
    }
    drivers[i].obj = obj;
    drivers[i].inputFunc = inFunc;
    drivers[i].poolFunc = poolFunc;
    drivers[i+1].obj = 0;
    poolFunc(obj, xkDefaultPath, 0, 0, 0);
    findRomOpts("path", inputOpts, sizeof(inputOpts)/sizeof(RomOpt), &i);
    /* 
     * The "msg input" option is here because we need to process it
     * after the drivers have registered, and the only way to do that
     * right now is to do it for each driver, one at a time, after it
     * registers.  And since driver initialization is done in the path
     * module, processing of the "msg input" option is in the path
     * module as well.
     *
     * Separating driver initialization from that of other protocols
     * (and allowing for hooks like processing input config options
     * after the driver initialization) would probably be a good thing.
     */
    findRomOpts("msg", msgOpts, sizeof(msgOpts)/sizeof(RomOpt), &i);
}


xkern_return_t
pathEstablishPool(
    Path		path,
    u_int 		nBuffs,
    u_int 		nThreads,
    XkThreadPolicy	policy,
    char 		*dev )
{
    PathInfo 		*pi;
    xkern_return_t 	xkr = XK_SUCCESS;
    int			i;
    DriverInfo		*di;

    pi = (PathInfo *)(((char *)path) - offsetof(PathInfo, path));
    for ( i=0, di = drivers; di->obj; di++, i++ ) {
	if ( dev && strcmp(di->obj->fullName, dev) &&
	    	    strcmp(di->obj->name, dev) ) {
	    continue;
	}
	if ( di->poolFunc(di->obj, path, nBuffs, nThreads, policy)
	    	== XK_FAILURE ) {
	    xkr = XK_FAILURE;
	}
	setBit(pi->driverMask, i);
    }
    return xkr;
}


xkern_return_t
pathReserveInput( Path path, int nMsgs, u_int msgSize, char *dev )
{
    int		i;
    DriverInfo	*di;
    PathInfo	*pi;

    pi = (PathInfo *)(((char *)path) - offsetof(PathInfo, path));
    for ( i=0, di = drivers; di->obj; di++, i++ ) {
	if ( dev && strcmp(di->obj->fullName, dev) &&
	    	    strcmp(di->obj->name, dev) ) {
	    continue;
	}
	if ( ! isBitSet(pi->driverMask, i) ) {
	    continue;
	}
	if ( di->inputFunc(di->obj, &pi->path, msgSize, nMsgs) == XK_FAILURE ) {
	    for ( i--, di--; i > 0; i--, di-- ) {
		if ( dev && strcmp(di->obj->fullName, dev) &&
		    strcmp(di->obj->name, dev) ) {
		    continue;
		}
		if ( ! isBitSet(pi->driverMask, i) ) {
		    continue;
		}
		di->inputFunc(di->obj, &pi->path, msgSize, - nMsgs);
	    }
	    return XK_FAILURE;
	}
    }
    return XK_SUCCESS;
}


