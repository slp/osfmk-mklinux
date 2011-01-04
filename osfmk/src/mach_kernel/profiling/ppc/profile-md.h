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
 * Types of the profil counter.
 */

typedef unsigned short	HISTCOUNTER;		/* profil */
typedef prof_cnt_t	LHISTCOUNTER;		/* lprofil */

struct profile_stats {			/* Debugging counters */
	prof_uptrint_t major_version;	/* major version number */
	prof_uptrint_t minor_version;	/* minor version number */
};

struct profile_md {
	int major_version;		/* major version number */
	int minor_version;		/* minor version number */
};

#define PROFILE_MAJOR_VERSION 1
#define PROFILE_MINOR_VERSION 1

#endif /* _PROFILE_MD_H */






