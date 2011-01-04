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

#ifndef _PROFILE_MD_H
#define _PROFILE_MD_H

/*
 * Define the interfaces between the assembly language profiling support
 * that is common between the kernel, mach servers, and user space library.
 */

/*
 * Integer types used.
 */

typedef	long		prof_ptrint_t;	/* hold either pointer or signed int */
typedef	unsigned long	prof_uptrint_t;	/* hold either pointer or unsigned int */
typedef	long		prof_lock_t;	/* lock word type */
typedef unsigned char	prof_flag_t;	/* type for boolean flags */

/*
 * Double precision counter.
 */

typedef struct prof_cnt_t {
	prof_uptrint_t	low;		/* low 32 bits of counter */
	prof_uptrint_t	high;		/* high 32 bits of counter */
} prof_cnt_t;

#define PROF_CNT_INC(cnt)	((++((cnt).low) == 0) ? ++((cnt).high) : 0)
#define PROF_CNT_ADD(cnt,val)	(((((cnt).low + (val)) < (val)) ? ((cnt).high++) : 0), ((cnt).low += (val)))
#define PROF_CNT_LADD(cnt,val)	(PROF_CNT_ADD(cnt,(val).low), (cnt).high += (val).high)
#define PROF_CNT_SUB(cnt,val)	(((((cnt).low - (val)) > (cnt).low) ? ((cnt).high--) : 0), ((cnt).low -= (val)))
#define PROF_CNT_LSUB(cnt,val)	(PROF_CNT_SUB(cnt,(val).low), (cnt).high -= (val).high)

#define PROF_ULONG_TO_CNT(cnt,val)	(((cnt).high = 0), ((cnt).low = val))
#define	PROF_CNT_OVERFLOW(cnt,high,low)	(((high) = (cnt).high), ((low) = (cnt).low))
#define PROF_CNT_TO_ULONG(cnt)		(((cnt).high == 0) ? (cnt).low : 0xffffffffu)
#define PROF_CNT_TO_LDOUBLE(cnt)	((((long double)(cnt).high) * 4294967296.0L) + (long double)(cnt).low)
#define PROF_CNT_TO_DECIMAL(buf,cnt)	_profile_cnt_to_decimal(buf, (cnt).low, (cnt).high)
#define PROF_CNT_EQ_0(cnt)		(((cnt).high | (cnt).low) == 0)
#define PROF_CNT_NE_0(cnt)		(((cnt).high | (cnt).low) != 0)
#define PROF_CNT_EQ(cnt1,cnt2)		((((cnt1).high ^ (cnt2).high) | ((cnt1).low ^ (cnt2).low)) == 0)
#define PROF_CNT_NE(cnt1,cnt2)		((((cnt1).high ^ (cnt2).high) | ((cnt1).low ^ (cnt2).low)) != 0)
#define PROF_CNT_GT(cnt1,cnt2)		(((cnt1).high > (cnt2).high) || ((cnt1).low > (cnt2).low))
#define PROF_CNT_LT(cnt1,cnt2)		(((cnt1).high < (cnt2).high) || ((cnt1).low < (cnt2).low))

/* max # digits + null to hold prof_cnt_t values (round up to multiple of 4) */
#define PROF_CNT_DIGITS			24

/*
 * Types of the profil counter.
 */

typedef unsigned short	HISTCOUNTER;		/* profil */
typedef prof_cnt_t	LHISTCOUNTER;		/* lprofil */

#define LPROF_ULONG_TO_CNT(cnt,val)	PROF_ULONG_TO_CNT(cnt,val)
#define LPROF_CNT_INC(lp)		PROF_CNT_INC(lp)
#define LPROF_CNT_ADD(lp,val)		PROF_CNT_ADD(lp,val)
#define LPROF_CNT_LADD(lp,val)		PROF_CNT_LADD(lp,val)
#define LPROF_CNT_SUB(lp,val)		PROF_CNT_SUB(lp,val)
#define LPROF_CNT_LSUB(lp,val)		PROF_CNT_LSUB(lp,val)
#define	LPROF_CNT_OVERFLOW(lp,high,low)	PROF_CNT_OVERFLOW(lp,high,low)
#define LPROF_CNT_TO_ULONG(lp)		PROF_CNT_TO_ULONG(lp)
#define LPROF_CNT_TO_LDOUBLE(lp)	PROF_CNT_TO_LDOUBLE(lp)
#define LPROF_CNT_TO_DECIMAL(buf,cnt)	PROF_CNT_TO_DECIMAL(buf,cnt)
#define LPROF_CNT_EQ_0(cnt)		PROF_CNT_EQ_0(cnt)
#define LPROF_CNT_NE_0(cnt)		PROF_CNT_NE_0(cnt)
#define LPROF_CNT_EQ(cnt1,cnt2)		PROF_CNT_EQ(cnt1,cnt2)
#define LPROF_CNT_NE(cnt1,cnt2)		PROF_CNT_NE(cnt1,cnt2)
#define LPROF_CNT_GT(cnt1,cnt2)		PROF_CNT_GT(cnt1,cnt2)
#define LPROF_CNT_LT(cnt1,cnt2)		PROF_CNT_LT(cnt1,cnt2)
#define LPROF_CNT_DIGITS		PROF_CNT_DIGITS

/*
 *  fraction of text space to allocate for histogram counters
 */

#define HISTFRACTION    4

/*
 * Fraction of text space to allocate for from hash buckets.
 */

#define HASHFRACTION	HISTFRACTION

/*
 * Prof call count, external format.
 */

struct prof_ext {
	prof_uptrint_t	cvalue;		/* caller address */
	prof_uptrint_t	cncall;		/* # of calls */
};

/*
 * Prof call count, internal format.
 */

struct prof_int {
	struct prof_ext	prof;		/* external prof struct */
	prof_uptrint_t	overflow;	/* # times prof counter overflowed */
};

/*
 * Gprof arc, external format.
 */

struct gprof_arc {
	prof_uptrint_t	 frompc;	/* caller's caller */
	prof_uptrint_t	 selfpc;	/* caller's address */
	prof_uptrint_t	 count;		/* # times arc traversed */
};

/*
 * Gprof arc, internal format.
 */

struct hasharc {
	struct hasharc	*next;		/* next gprof record */
	struct gprof_arc arc;		/* gprof record */
	prof_uptrint_t	 overflow;	/* # times counter overflowed */
};

/*
 * Linked list of all function profile blocks.
 */

#define MAX_CACHE	3		/* # cache table entries */

struct gfuncs {
	struct hasharc **hash_ptr;		/* gprof hash table */
	struct hasharc **unique_ptr; 		/* function unique pointer */
	struct prof_int prof;			/* -p stats for elf */
	struct hasharc *cache_ptr[MAX_CACHE];	/* cache element pointers */
};

/*
 * Profile information which might be written out in ELF {,g}mon.out files.
 */

#define MAX_BUCKETS 9			/* max bucket chain to print out */

struct profile_stats {			/* Debugging counters */
	prof_uptrint_t major_version;	/* major version number */
	prof_uptrint_t minor_version;	/* minor version number */
	prof_uptrint_t stats_size;	/* size of profile_vars structure */
	prof_uptrint_t profil_buckets; 	/* # profil buckets */
	prof_uptrint_t my_cpu;		/* identify current cpu/thread */
	prof_uptrint_t max_cpu;		/* identify max cpu/thread */
	prof_uptrint_t prof_records;	/* # of functions profiled */
	prof_uptrint_t gprof_records;	/* # of gprof arcs */
	prof_uptrint_t hash_buckets;	/* # gprof hash buckets */
	prof_uptrint_t bogus_count;	/* # of bogus functions found in gprof */

	prof_cnt_t cnt;			/* # of calls to _{,g}prof_mcount */
	prof_cnt_t dummy;		/* # of calls to _dummy_mcount */
	prof_cnt_t old_mcount;		/* # of calls to old mcount */
	prof_cnt_t hash_search;		/* # hash buckets searched */
	prof_cnt_t hash_num;		/* # times hash table searched */
	prof_cnt_t user_ticks;		/* # ticks in user space */
	prof_cnt_t kernel_ticks;	/* # ticks in kernel space */
	prof_cnt_t idle_ticks;		/* # ticks in idle mode */
	prof_cnt_t overflow_ticks;	/* # ticks where HISTCOUNTER overflowed */
	prof_cnt_t acontext_locked;	/* # times an acontext was locked */
	prof_cnt_t too_low;		/* # times a histogram tick was too low */
	prof_cnt_t too_high;		/* # times a histogram tick was too high */
	prof_cnt_t prof_overflow;	/* # times a prof count field overflowed */
	prof_cnt_t gprof_overflow;	/* # times a gprof count field overflowed */

					/* allocation statistics */
	prof_uptrint_t num_alloc  [(int)ACONTEXT_MAX];	/* # allocations */
	prof_uptrint_t bytes_alloc[(int)ACONTEXT_MAX];	/* bytes allocated */
	prof_uptrint_t num_context[(int)ACONTEXT_MAX];	/* # contexts */
	prof_uptrint_t wasted     [(int)ACONTEXT_MAX];	/* wasted bytes */
	prof_uptrint_t overhead   [(int)ACONTEXT_MAX];	/* overhead bytes */

	prof_uptrint_t buckets[MAX_BUCKETS+1]; /* # hash indexes that have n buckets */
	prof_cnt_t     cache_hits[MAX_CACHE];  /* # times nth cache entry matched */

	prof_cnt_t stats_unused[64];	/* reserved for future use */
};

#define PROFILE_MAJOR_VERSION 1
#define PROFILE_MINOR_VERSION 1

/*
 * Machine dependent fields.
 */

struct profile_md {
	int major_version;		/* major version number */
	int minor_version;		/* minor version number */
	size_t md_size;			/* size of profile_md structure */
	struct hasharc **hash_ptr;	/* gprof hash table */
	size_t hash_size;		/* size of hash table */
	prof_uptrint_t num_cache;	/* # of cache entries */
	void (*save_mcount_ptr)(void);	/* save for _mcount_ptr */
	void (**mcount_ptr_ptr)(void);	/* pointer to _mcount_ptr */
	struct hasharc *dummy_ptr;	/* pointer to dummy gprof record */
	void *(*alloc_pages)(size_t);	/* pointer to _profile_alloc_pages */
	char num_buffer[PROF_CNT_DIGITS]; /* convert 64 bit ints to string */
	long md_unused[58];		/* add unused fields */
};

/*
 * Record information about each function call.  Specify
 * caller, caller's caller, and a unique label for use by
 * the profiling routines.
 */
extern void _prof_mcount(void);
extern void _gprof_mcount(void);
extern void _dummy_mcount(void);
extern void (*_mcount_ptr)(void);

/*
 * Function in profile-md.c to convert prof_cnt_t to string format (decimal & hex).
 */
extern char *_profile_cnt_to_decimal(char *, prof_uptrint_t, prof_uptrint_t);

#endif /* _PROFILE_MD_H */
