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
 * allocator.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * allocator.c,v
 * Revision 1.2.1.4.1.2  1994/09/07  04:18:22  menze
 * OSF modifications
 *
 * Revision 1.2.1.4  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.2.1.3  1994/09/01  04:29:08  menze
 * Interior allocator uses platform-specific allocator
 * Fixed some rom-option problems
 * *AdjustMin* becomes *Reserve*
 * Added allocGetwithSize and modified allocGet
 * Meta-data allocations now use Allocators and Paths
 *
 * Revision 1.2.1.2  1994/07/27  19:51:36  menze
 * Allocators no longer have ID numbers
 *
 * Revision 1.2.1.1  1994/07/21  23:49:46  menze
 * Allocators present only a "raw memory" interface.  All Msg and MNode
 * references have been removed.  Msg-related rom configuration is now in
 * the message tool.
 *
 * Revision 1.2  1994/05/17  18:28:00  menze
 * Call-backs are working
 * Interior alloc interface is now more block-oriented
 * Added default version of minUltimate
 * Creates default allocators
 * New allocators: block and guaranteed
 *
 * Revision 1.1  1994/03/22  00:25:06  menze
 * Initial revision
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/intern_alloc.h>

/*
 * alloc	create		ALLOC_NAME 	TYPE	[BACKING_ALLOCATOR]
 * alloc	reserve		ALLOC_NAME	N	SIZE
 * alloc	limit		ALLOC_NAME	N 
 * alloc	blocks		ALLOC_NAME	B1 B2 ...
 */

#define DEFAULT_ALLOC_ID		0
#define DEFAULT_INTERIOR_ALLOC_ID	20000


#define ALLOC_MAX_DRIVERS	10

typedef struct {
    struct allocator	alloc;
    Map		registrants;	/* void * -> AllocNotifyFunc */
    int		callBacksInProgress;
    Event	ev;
} AllocatorInfo;
#define REG_MAP_SIZE	20


typedef struct {
    char	*name;
    u_int	blocks[ALLOC_MAX_BLOCKS];
    int		numBlocks;
} PreBuildOptInfo;


static xkern_return_t	readMemRom( char **, int, int, void * );
static xkern_return_t	readNameRom( char **, int, int, void * );
static xkern_return_t	readBlockRom( char **, int, int, void * );
static xkern_return_t	readReserveRom( char **, int, int, void * );

#if XK_DEBUG
int	tracealloc;
#endif 

static struct allocator	bsAlloc;

Allocator	xkBootStrapAllocator = &bsAlloc;
Allocator	xkDefaultAllocator;
Allocator	xkDefaultMsgAllocator;
Allocator	xkSystemAllocator;

static Map	nameMap;	/* name to AllocatorInfo */
static Map	registrantMap;	/* void * -> AllocNotifyFunc */
#define ID_MAP_SIZE		50
#define NAME_MAP_SIZE		50
#define REGISTRANT_MAP_SIZE	50


#define MAX_ALLOC_TYPES	10

typedef struct {
    char		*type;
    AllocInitFunc	*f;
} AllocType;


/* 
 * Platform-specific types may be added via allocAddType
 */
static AllocType allocTypes[MAX_ALLOC_TYPES + 1];

static AllocType defaultTypes[] = {
    { "budget", budgetAlloc_init },
    { "block", blockAlloc_init },
    { "guaranteed", guaranteedAlloc_init },
    { "interior", simpleInternalAlloc_init }
};


typedef struct {
    char	*name;
    char	*type;
    char	*lower;
    enum {
	NOT_BUILT_S = 0,
	BUILDING_S,
	BUILT_S,
	ERROR_S
    } 		state;
} AllocConfig;

#define MAX_ALLOC_CONFIGS	30

/* 
 * More initial allocators might be added through ROM options.
 * Allocators subsequently created via allocCreate will not show up in
 * this list of configurations.
 */
static AllocConfig allocConfigs[MAX_ALLOC_CONFIGS + 1];

AllocConfig	defaultConfigs[] = {
    { "default_interior", "interior", 0 },
    { "default", "budget", "default_interior" },
    { "default_msg", "budget", "default_interior" },
    { "system", "budget", "default_interior" }
};


static RomOpt	nameOpts[] = {
    { "create", 4, readNameRom },
    { "limit", 0, 0 },
    { "reserve", 0, 0 },
    { "blocks", 0, 0 },
};


static RomOpt	allocOpts[] = {
    { "create", 0, 0 },
    { "limit", 4, readMemRom },
    { "reserve", 5, readReserveRom },
    { "blocks", 0, 0 }
};


/* 
 * Options that need to be set up for each allocator before it is
 * instantiated.  
 */
static RomOpt	preBuildOpts[] = {
    { "create", 0, 0 },
    { "limit", 0, 0 },
    { "reserve", 0, 0 },
    { "blocks", 4, readBlockRom }
};


/* 
 * alloc limit ALLOC_NAME N 
 */
static xkern_return_t
readMemRom( char **str, int nFields, int line, void *arg )
{
    void	*i;
    int		newMax;

    if ( mapVarResolve(nameMap, str[2], strlen(str[2]), &i)
		== XK_FAILURE ) {
	sprintf(errBuf, "Couldn't find allocator %s", str[2]);
	xError(errBuf);
	return XK_FAILURE;
    }
    newMax = atoi(str[3]); if ( newMax < 0 ) return XK_FAILURE;
    xTrace2(alloc, TR_EVENTS, "alloc ROM option: limit %s %d", str[2], newMax);
    allocMaxChanges(&((AllocatorInfo *)i)->alloc, newMax);
    return XK_SUCCESS;
}


/* 
 * alloc block ALLOC_NAME B1 B2 ...
 */
static xkern_return_t
readBlockRom( char **str, int nFields, int line, void *arg )
{
    int		i, j, b;
    PreBuildOptInfo	*info = arg;

    if ( strcmp(str[2], info->name) ) return XK_SUCCESS;
    xTrace1(alloc, TR_EVENTS, "alloc ROM option: block %s", str[2]);
    for ( i=3; i < nFields; i++ ) {
	b = atoi(str[i]);  if ( b <= 0 ) return XK_FAILURE;
	/* 
	 * Block sizes are stored in increasing order
	 */
	if ( info->numBlocks >= ALLOC_MAX_BLOCKS - 1 ) {
	    xTrace1(alloc, TR_ERRORS, "Allocator %s -- too many blocks specified",
		    info->name);
	    return XK_FAILURE;
	}
	for ( j=0; j < info->numBlocks && info->blocks[j] < b; j++ )
	  ;
	for ( ; b; j++ ) {
	    {
		u_int tmp;

		tmp = info->blocks[j];
		info->blocks[j] = b;
		b = tmp;
	    }
	}
	info->numBlocks++;
	xAssert(info->numBlocks < ALLOC_MAX_BLOCKS);
    }
    return XK_SUCCESS;
}


static xkern_return_t
addInstance( char *name, char *type, char *lower )
{
    int	i;

    for ( i=0; i <= MAX_ALLOC_CONFIGS; i++ ) {
	if ( ! allocConfigs[i].name || 
	     ! strcmp(allocConfigs[i].name, name) ) {
	    break;
	}
    }
    if ( i > MAX_ALLOC_CONFIGS ) {
	xTrace0(alloc, TR_ERRORS, "too many allocators specified");
	return XK_FAILURE;
    }
    allocConfigs[i].name = name;
    allocConfigs[i].type = type;
    allocConfigs[i].lower = lower;
    return XK_SUCCESS;
}	


/* 
 * alloc name ALLOC_NAME TYPE [BACKING_ALLOCATOR]
 */
static xkern_return_t
readNameRom( char **str, int nFields, int line, void *arg )
{
    if ( strlen(str[2]) > MAX_ALLOC_NAME_LEN ||
	 (nFields > 4 && strlen(str[4]) > MAX_ALLOC_NAME_LEN) ) {
	return XK_FAILURE;
    }
    return addInstance(str[2], str[3], (nFields > 4) ? str[4] : 0);
}


/* 
 * alloc reserve ALLOC_NAME N SIZE
 */
static xkern_return_t
readReserveRom( char **str, int nFields, int line, void *arg )
{
    int		n, size;
    void	*i;
    
    if ( mapVarResolve(nameMap, str[2], strlen(str[2]), &i)
		== XK_FAILURE ) {
	sprintf(errBuf, "Couldn't find allocator %s", str[2]);
	xError(errBuf);
	return XK_FAILURE;
    }
    n = atoi(str[3]);
    size = atoi(str[4]);
    if ( n < 0 || size < 0 ) return XK_FAILURE;
    return allocReserve(&((AllocatorInfo *)i)->alloc, n, (u_int *)&size);
}




/* 
 * Create an internal binding for an allocator with this name and id.
 */
Allocator
allocCreate( char *name, char *type, Allocator lower )
{
    AllocatorInfo	*i;
    Allocator		a;
    xkern_return_t	xkr;

    if ( mapVarResolve(nameMap, name, strlen(name), 0) == XK_SUCCESS ) {
	xTrace1(alloc, TR_ERRORS, "allocator name %s already bound", name);
	return 0;
    }
    if ( strlen(name) > MAX_ALLOC_NAME_LEN ) {
	xTrace1(alloc, TR_ERRORS, "allocator name %s too long", name);
	return 0;
    }
    if ( (i = xSysAlloc(sizeof(AllocatorInfo))) == 0 ) {
	return 0;
    }
    bzero((char *)i, sizeof(AllocatorInfo));
    if ( ! (i->ev = evAlloc(xkSystemPath)) ) {
	allocFree(i);
	return 0;
    }
    a = &i->alloc;
    a->lower = lower;
    i->registrants = mapCreate(REG_MAP_SIZE, sizeof(XObj), xkSystemPath);
    if ( i->registrants == 0 ) {
	xTrace0(alloc, TR_ERRORS, "allocCreate, map creation fails");
	allocFree(i);
	return 0;
    }
    strcpy(a->name, name);
    {
	/* 
	 * Look through the ROM entries for configuration stuff that's
	 * easier to hand the protocol at initialization time than later. 
	 */
	PreBuildOptInfo	info;
	AllocType	*t;
	
	bzero((char *)&info, sizeof(info));
	info.name = name;
	xkr = findRomOpts("alloc", preBuildOpts,
			  sizeof(preBuildOpts)/sizeof(RomOpt), &info);
	if ( xkr != XK_SUCCESS ) {
	    goto bailout;
	}
	if ( info.blocks[0] == 0 && a->lower &&
	     allocBlockSizes(a->lower, info.blocks) != XK_SUCCESS ) {
		 goto bailout;
	}
	for ( t=allocTypes; t->type; t++ ) {
	    if ( ! strcmp(t->type, type) ) {
		break;
	    }
	}
	if ( t->type ) {
	    xkr = t->f(a, info.blocks);
	} else {
	    xTrace1(alloc, TR_ERRORS, "Allocator type %s not recognized",
		    type);
	    xkr = XK_FAILURE;
	}
    }
bailout:
    if ( xkr == XK_FAILURE ||
	 	(lower && allocConnect(lower, a) == XK_FAILURE) || 
	 	mapVarBind(nameMap, name, strlen(name), i, 0) != XK_SUCCESS ) {
	mapClose(i->registrants);
	allocFree((char *)i);
	return 0;
    }
    return a;
}


xkern_return_t
allocAddType( char *type, AllocInitFunc f )
{
    int	i;

    if ( ! type ) {
	return XK_FAILURE;
    }
    for ( i=0; i <= MAX_ALLOC_TYPES; i++ ) {
	if ( ! allocTypes[i].type || ! strcmp(allocTypes[i].type, type) ) {
	    break;
	}
    }
    if ( i > MAX_ALLOC_TYPES ) {
	xTrace0(alloc, TR_ERRORS, "too many allocators specified");
	return XK_FAILURE;
    }
    allocTypes[i].type = type;
    allocTypes[i].f = f;
    /* 
     * Special case for the ALLOC_PLATFORM_TYPE allocator ... cause the
     * default_interior allocator to point to use an instance of it.
     */
    if ( ! strcmp(type, ALLOC_PLATFORM_TYPE) ) {
	if ( addInstance(ALLOC_PLATFORM_TYPE "_instance",
			 ALLOC_PLATFORM_TYPE, 0) != XK_SUCCESS ) {
	    return XK_FAILURE;
	}
	if ( addInstance("default_interior", "interior",
			 ALLOC_PLATFORM_TYPE "_instance") != XK_SUCCESS ) {
	    return XK_FAILURE;
	}
    }
    return XK_SUCCESS;
}


xkern_return_t
allocRegister( Allocator a, AllocNotifyFunc f, void *obj )
{
    Map	map;

    if ( a ) {
	AllocatorInfo	*i;

	i = (AllocatorInfo *)((char *)a) - offsetof(AllocatorInfo, alloc);
	map = i->registrants;
    } else {
	map = registrantMap;
    }
    if ( mapBind(map, &obj, f, 0) != XK_SUCCESS ) {
	xTrace0(alloc, TR_ERRORS, "allocRegister -- mapBind fails");
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


xkern_return_t
allocUnregister( Allocator a, void *obj )
{
    Map	map;

    if ( a ) {
	AllocatorInfo	*i;
	
	i = (AllocatorInfo *)((char *)a) - offsetof(AllocatorInfo, alloc);
	map = i->registrants;
    } else {
	map = registrantMap;
    }
    if ( mapUnbind(map, &obj) == XK_FAILURE ) {
	xTrace0(alloc, TR_ERRORS, "allocUnregister -- obj not registered");
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


static int
callBackRegistrant( void *key, void *val, void *arg )
{
    AllocatorInfo 	*i = arg;
    AllocNotifyFunc	f = (AllocNotifyFunc)val;

    xTrace3(alloc, TR_DETAILED,
	    "alloc (%s) callBackRegistrant runs, f (%lx), arg (%lx)",
	    i->alloc.name, (long)f, (long)*(void **)key);
    f(&i->alloc, *(void **)key);
    return MFE_CONTINUE;
}


static int
callBackNonGlobal( void *key, void *val, void *arg )
{
    if ( mapResolve(registrantMap, &key, 0) == XK_FAILURE ) {
	callBackRegistrant(key, val, arg);
    }
    return MFE_CONTINUE;
}


static void
doCallBacks( Event ev, void *arg )
{
    AllocatorInfo *i = arg;

    xTrace0(alloc, TR_DETAILED, "doCallBacks runs");
    mapForEach(i->registrants, callBackNonGlobal, i);
    if ( i->alloc.class == ALLOC_EXTERIOR ) {
	mapForEach(registrantMap, callBackRegistrant, i);
    }
    i->callBacksInProgress = 0;
}


void
allocStartCallBacks( Allocator self )
{
    AllocatorInfo	*i;

    i = (AllocatorInfo *)((char *)self) - offsetof(AllocatorInfo, alloc);
    xTrace1(alloc, TR_DETAILED, "allocStartCallBacks, allocator %s", self->name);
    if ( i->callBacksInProgress ) {
	xTrace0(alloc, TR_EVENTS, "Call-backs are already in progress");
    }
    xTrace1(alloc, TR_EVENTS, "Initiating call-backs for allocator %s", self->name);
    i->callBacksInProgress = 1;
    evSchedule(i->ev, doCallBacks, i, 0);
}


xkern_return_t
allocBootStrap()
{
    if ( bootStrapAlloc_init(xkBootStrapAllocator) == XK_FAILURE ) {
	return XK_FAILURE;
    }
    xkSystemAllocator = xkBootStrapAllocator;
    return XK_SUCCESS;
}


/* 
 * Create an allocator from an AllocConfig, first creating the lower
 * allocator if necessary.
 */
static void
buildConfig( AllocConfig *c )
{
    Allocator	lowerAlloc;

    if ( c->state == BUILDING_S ) {
	xTrace1(alloc, TR_ERRORS, "allocator cycle detected building %s",
		c->name);
	return;
    }
    if ( c->state == ERROR_S || c->state == BUILT_S ) {
	return;
    }
    c->state = BUILDING_S;
    if ( c->lower ) {
	if ( ! (lowerAlloc = getAllocByName(c->lower)) ) {
	    AllocConfig	*lc;

	    for ( lc = allocConfigs; lc->name; lc++ ) {
		if ( ! strcmp(lc->name, c->lower) ) {
		    buildConfig(lc);
		    break;
		}
	    }
	    if ( ! (lowerAlloc = getAllocByName(c->lower)) ) {
		xTrace2(alloc, TR_ERRORS,
			"couldn't find lower alloc %s for allocator %s",
			c->lower, c->name);
		c->state = ERROR_S;
		return;
	    }
	}
    } else {
	lowerAlloc = 0;
    }
    if ( allocCreate(c->name, c->type, lowerAlloc) == 0 ) {
	xTrace1(alloc, TR_ERRORS, "Error creating allocator %s", c->name);
	c->state = ERROR_S;
    } else {
	c->state = BUILT_S;
    }
}


/* 
 * Create all allocators from the allocator graph specified in
 * allocConfigList. 
 */
static void
createConfigs( void )
{
    AllocConfig	*c;

    for ( c = allocConfigs; c->name; c++ ) {
	if ( c->state == NOT_BUILT_S ) {
	    buildConfig(c);
	} 
    }
}


xkern_return_t
allocInit()
{
    xTrace0(alloc, TR_GROSS_EVENTS, "Allocator module initializes");
    if ( (sizeof(defaultTypes)/sizeof(AllocType) > MAX_ALLOC_TYPES) ||
	 (sizeof(defaultConfigs)/sizeof(AllocConfig) > MAX_ALLOC_CONFIGS) ) {
	xError("Internal allocator configuration errors");
	return XK_FAILURE;
    }
    bcopy((char *)defaultTypes, (char *)allocTypes, sizeof(defaultTypes));
    bcopy((char *)defaultConfigs, (char *)allocConfigs, sizeof(defaultConfigs));
    nameMap = mapCreate(NAME_MAP_SIZE, -MAX_ALLOC_NAME_LEN, xkSystemPath);
    registrantMap = mapCreate(REGISTRANT_MAP_SIZE, sizeof(void *),
			      xkSystemPath);
    if ( ! nameMap || ! registrantMap ) {
	xTrace0(alloc, TR_ERRORS, "allocInit: map creation error");
	return XK_FAILURE;
    }
    if ( platformAllocators() != XK_SUCCESS ) {
	return XK_FAILURE;
    }
    /* 
     * Find rom options which specify creation of allocators
     */
    findRomOpts("alloc", nameOpts, sizeof(nameOpts)/sizeof(RomOpt), 0);
    createConfigs();
    xkDefaultAllocator = getAllocByName("default");
    xkDefaultMsgAllocator = getAllocByName("default_msg");
    xkSystemAllocator = getAllocByName("system");
    if ( ! xkDefaultAllocator || ! xkSystemAllocator ||
	 ! xkDefaultMsgAllocator ) {
	xError("Error creating default allocators");
	return XK_FAILURE;
    }
    /* 
     * Find rom options which configure existing allocators
     */
    findRomOpts("alloc", allocOpts, sizeof(allocOpts)/sizeof(RomOpt), 0);
    return XK_SUCCESS;
}


xkern_return_t	
allocReservePartial( Allocator a, int nBuffs, u_int *dataSize, u_int hdrSize,
		     u_int *numBlocks )
{
    if ( a->class != ALLOC_EXTERIOR ) return XK_FAILURE;
    return a->methods->u.e.minPartial(a, nBuffs, dataSize, hdrSize, numBlocks);
}


xkern_return_t	
allocReserve( Allocator a, int nBuffs, u_int *size )
{
    if ( a->class != ALLOC_EXTERIOR ) return XK_FAILURE;
    return a->methods->u.e.min(a, nBuffs, size);
}


xkern_return_t
allocXfer( Allocator from, Allocator to, void *buf, u_int len )
{
    /* 
     * We currently only allow transfers between allocators sharing a
     * common backing allocator.  
     * XXX
     */
    if ( from->lower && from->lower == to->lower ) {
	return from->lower->methods->u.i.handleXfer(from->lower, from, to, buf, len);
    } else {
	return XK_FAILURE;
    }
}


xkern_return_t
allocInteriorAdjustMin( Allocator a, Allocator upper, u_int *size, int N )
{
    if ( a->class != ALLOC_INTERIOR ) return XK_FAILURE;
    return a->methods->u.i.minBlocks(a, upper, size, N);
}



xkern_return_t
allocConnect( Allocator a, Allocator upper )
{
    if ( a->class != ALLOC_INTERIOR ) return XK_FAILURE;
    return a->methods->u.i.connect ? a->methods->u.i.connect(a, upper)
      				   : XK_SUCCESS;
}


xkern_return_t
allocDisconnect( Allocator a, Allocator upper )
{
    if ( a->class != ALLOC_INTERIOR ) return XK_FAILURE;
    return a->methods->u.i.disconnect ? a->methods->u.i.disconnect(a, upper)
      				      : XK_SUCCESS;
}


u_int
allocGetMax( Allocator self ) 
{
    if ( self->class != ALLOC_EXTERIOR ) return 0;
    return self->methods->u.e.getMax(self);
}


u_int
allocInteriorGetMax( Allocator lower, Allocator upper ) 
{
    if ( lower->class != ALLOC_INTERIOR || lower->methods->u.i.getMax == 0 ) {
	return 0;
    }
    return lower->methods->u.i.getMax(lower, upper);
}


xkern_return_t
allocMaxChanges( Allocator a, u_int max )
{
    if ( a->class != ALLOC_INTERIOR ) return 0;
    return a->methods->u.i.maxChanges(a, max);
}


xkern_return_t
allocBlockSizes( Allocator a, u_int buf[ALLOC_MAX_BLOCKS] )
{
    if ( a->methods->u.i.blockSizes(a, buf) ) {
	if ( a->class != ALLOC_INTERIOR ||
	     a->methods->u.i.blockSizes(a, buf) == XK_FAILURE ||
	     buf[ALLOC_MAX_BLOCKS - 1] != 0 ) {
		return XK_FAILURE;
	}
    }
    return XK_SUCCESS;
}


static int
callDumpStats( void *key, void *val, void *arg )
{
    xTrace0(alloc, TR_ALWAYS, "");
    allocDumpStats((Allocator)val);
    return MFE_CONTINUE;
}


void
allocDumpStatsAll()
{
    mapForEach(nameMap, callDumpStats, 0);
}


Allocator
getAllocByName( char *name )
{
    void	*i;

    if ( mapVarResolve(nameMap, name, strlen(name), &i) == XK_FAILURE ) {
	return 0;
    }
    return &((AllocatorInfo *)i)->alloc;
}


xkern_return_t
defaultMinPartial( Allocator a, int nMsgs, u_int *reqSize, u_int hdrSize,
		    u_int *numBlocks )
{
    u_int	fullBuffers, partBufSize, fullSize, totalSize;

    xAssert( a->class == ALLOC_EXTERIOR );
    xAssert(numBlocks);
    xTrace4(alloc, TR_FULL_TRACE,
	    "alloc defaultMinPartial [%s], req %d, hdr %d, %d msgs",
	    a->name, reqSize, hdrSize, nMsgs);
    if ( hdrSize > a->largeBlockSize ) return XK_FAILURE;
    fullBuffers = *reqSize / (a->largeBlockSize - hdrSize);
    xTrace1(alloc, TR_FULL_TRACE, "%d full buffers", fullBuffers);
    if ( fullBuffers ) {
	fullSize = a->largeBlockSize;
	if ( a->methods->u.e.min(a, nMsgs * fullBuffers, &fullSize)
	    		== XK_FAILURE ) {
	    return XK_FAILURE;
	}
	totalSize = (fullSize - hdrSize) * fullBuffers;
    } else {
	totalSize = 0;
    }
    xAssert( *reqSize >= fullBuffers * (a->largeBlockSize - hdrSize) );
    partBufSize = *reqSize - fullBuffers * (a->largeBlockSize - hdrSize);
    if ( partBufSize ) {
	xTrace1(alloc, TR_FULL_TRACE, "partial buffer, size %d", partBufSize);
	partBufSize += hdrSize;
	if ( a->methods->u.e.min(a, nMsgs, &partBufSize) == XK_FAILURE ) {
	    if ( fullBuffers ) {
		a->methods->u.e.min(a, -nMsgs * fullBuffers, &fullSize);
	    }
	    return XK_FAILURE;
	}
	*numBlocks = fullBuffers + 1;
	totalSize += partBufSize - hdrSize;
    } else {
	*numBlocks = fullBuffers;
    }
    *reqSize = totalSize;
    return XK_SUCCESS;
}

