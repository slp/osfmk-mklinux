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

#include <xkern/include/xkernel.h>
#include <xkern/include/xk_debug.h>
#include <xkern/include/xk_alloc.h>
#include <xkern/include/intern_alloc.h>
#include <vm/vm_kern.h>
#include <machine/machlimits.h>

/* 
 * Wrapper putting an x-kernel allocator interface on top of zalloc
 * x-kernel zones are non-pageable, non-collectable,
 * and are preloaded, so allocations will not block.  Allocations for
 * sizes larger than the max zone will fail.
 */

#if     MACH_KDB || MACH_KGDB
void
db_show_net_memory(void);

vm_offset_t plat_alloc_zones;
#endif /* MACH_KDB || MACH_KGDB */ 

#if XKSTATS
u_int plat_alloc_failures;
#endif /* XKSTATS */

#define MAX_ZNAME_LEN	80

typedef struct {
    struct blockInfo {
	u_int	size;
	zone_t	zone;
	char	name[MAX_ZNAME_LEN + 1];
    } blocks[ALLOC_MAX_BLOCKS];
} AState;

typedef struct {
    char	*name;
    u_int	size;
    u_int	max;
} ZoneInfo;


static ZoneInfo
zInfo[] = {
    { "xk_zone_64", 64, 2000 },		/* MNodes, etc.	*/
    { "xk_zone_256", 256, 1000 },	/* XObjs, etc. 	*/
    { "xk_zone_mtu", 1750, 750, },	/* Eth MTU	*/
    { "xk_zone_4k", 4 * 1024, 50 },
    { "xk_zone_8k", 8 * 1024, 10 },
    { 0, 0, 0 }
};

#define MAX_REPORTED_BLOCK_SIZE	(4 * 1024)


static AllocMethods	kInt_methods;

/* 
 * We currently assume that the zone sizes never change.
 */
static u_int
getMax( Allocator self, Allocator client )
{
    AState	*as = self->state;
    u_int	i, max;

    max = 0;
    for ( i=0; zInfo[i].name; i++ ) {
	max += zInfo[i].size * zInfo[i].max;
    }
    return max;
}

static void *
get( Allocator self, Allocator client, u_int *reqSize )
{
    AState	*as = self->state;
    int		i;

    xTrace1(alloc, TR_EVENTS, "plat_alloc get %d bytes", *reqSize);
    for ( i=0; *reqSize > as->blocks[i].size; i++ )
      ;
    if ( as->blocks[i].zone ) {
	xTrace1(alloc, TR_MORE_EVENTS, "plat_alloc using zone %d",
		as->blocks[i].size);
	{
	    vm_offset_t ptr;
	    ptr = zget(as->blocks[i].zone);
	    if ( ! ptr ) {
#if XKSTATS
		plat_alloc_failures++;
#endif /* XKSTATS */
		xTrace1(alloc, TR_SOFT_ERRORS, "zget fails (zone %d)",
			as->blocks[i].size);
	    }
	    return (void *)ptr;
	}
    } else {
	/* 
	 * Request is too big.
	 */
	xTrace1(alloc, TR_SOFT_ERRORS, "plat_alloc request (%d) too big",
		*reqSize);
	return 0;
    }
}


static void
put( Allocator self, Allocator client, void *ptr, u_int len )
{
    AState	*as = self->state;
    int		i;

    for ( i=0; len > as->blocks[i].size; i++ )
      ;
    xAssert(as->blocks[i].zone);
    if ( as->blocks[i].zone ) {
	zfree(as->blocks[i].zone, (vm_offset_t)ptr);
    } 
}


static xkern_return_t
blockSizes( Allocator self, u_int buf[ALLOC_MAX_BLOCKS] )
{
    AState	*as = self->state;
    int		i;

    for ( i=0; (buf[i] = as->blocks[i].size) <= MAX_REPORTED_BLOCK_SIZE; i++ )
      ;
    buf[i] = 0;
    return XK_SUCCESS;
}


static xkern_return_t
xk_zalloc_init( Allocator self, u_int blocks[ALLOC_MAX_BLOCKS] )
{
    vm_offset_t min, max;
    vm_size_t	size;
    AState	*as;
    int		i;
    
    if ( kInt_methods.u.i.get == 0 ) {
	kInt_methods.u.i.get = get;
	kInt_methods.u.i.put = put;
	kInt_methods.u.i.blockSizes = blockSizes;
	kInt_methods.u.i.getMax = getMax;
    }
    if ( sizeof(zInfo) / sizeof(ZoneInfo) >= ALLOC_MAX_BLOCKS ) {
	return XK_FAILURE;
    }
    self->class = ALLOC_INTERIOR;
    self->methods = &kInt_methods;
    as = self->state = xSysAlloc(sizeof(AState));
#if MACH_KDB || MACH_KGDB
    plat_alloc_zones = (vm_offset_t)as;
#endif /* MACH_KDB || MACH_KGDB */
    bzero((char *)as, sizeof(AState));
    xk_thread_checkout(FALSE);
    for ( i=0; zInfo[i].name; i++ ) {
	struct blockInfo *bi;

	bi = &as->blocks[i];
	bi->size = zInfo[i].size;
	xAssert(strlen(zInfo[i].name) + strlen(self->name) < MAX_ZNAME_LEN - 2);
	sprintf(bi->name, "%s_%s", zInfo[i].name, self->name);
	bi->zone = zinit(zInfo[i].size, zInfo[i].size * zInfo[i].max,
			 0, bi->name);
	zone_change(bi->zone, Z_COLLECT, FALSE);
	if ( bi->zone == ZONE_NULL ) {
	    xTrace1(alloc, TR_ERRORS, "%s: zinit failure", self->name);
	    xk_thread_checkin();
	    return XK_FAILURE;
	}
	if ( zfill(bi->zone, zInfo[i].max) < zInfo[i].max ) {
	    xTrace1(alloc, TR_ERRORS, "%s: zfill failure", self->name);
	    xk_thread_checkin();
	    return XK_FAILURE;
	}
    }
    xk_thread_checkin();
    /* 
     * To detect allocations that are too large.
     */
    as->blocks[i].size = UINT_MAX;
    return XK_SUCCESS;
}



/* 
 * Platform-specific allocators can be established by making calls to
 * allocAddType here.
 */
xkern_return_t
platformAllocators()
{
    if ( allocAddType(ALLOC_PLATFORM_TYPE, xk_zalloc_init) != XK_SUCCESS ||
	 allocAddType("kern_zalloc", xk_zalloc_init) != XK_SUCCESS ) {
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}

#if     MACH_KDB
#include <ddb/db_output.h>

void
db_show_net_memory(void)
{
    AState      *as;
    int         i;

    as = (AState *)plat_alloc_zones;
    iprintf("Size\t\tzone\t\tcount\t\tout of\t\tname\n");
    
    for (i = 0; i < ALLOC_MAX_BLOCKS && as->blocks[i].zone; i++) {
	iprintf("%d\t\t%x\t%d\t\t%d\t\t%s\n",
		as->blocks[i].size, 
		as->blocks[i].zone,
		as->blocks[i].zone->count, 
		as->blocks[i].zone->max_size / as->blocks[i].zone->elem_size,
		as->blocks[i].name);
    }
}
#endif /* MACH_KDB */ 

