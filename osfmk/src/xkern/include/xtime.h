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
 * xtime.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:27:57 $
 */

/* Same time structure as Unix */

#ifndef xtime_h
#define xtime_h

typedef struct {
  long sec;
  long usec;
} XTime;

/* set *t to the current time of day */
void xGetTime(XTime *t);

/* 
 * xAddtime -- Sets the XTime value _res to the sum of _t1 and _t2.
 * Assumes _t1 and _t2 are in standard time format (i.e., does not
 * check for integer overflow of the usec value.)
 */
#define xAddTime( _res, _t1, _t2 )			\
do {							\
    (_res)->sec = (_t1).sec + (_t2).sec;		\
    (_res)->usec = (_t1).usec + (_t2).usec;		\
    if ( (_res)->usec >= (1000 * 1000)) {		\
	(_res)->sec += (_res)->usec / (1000 * 1000);	\
	(_res)->usec %= (u_int)(1000 * 1000);		\
    }							\
} while (0)



/* 
 * xSubTime -- Sets _res to the difference of _t1 and _t2.  The resulting
 * value may be negative.
 */
#define xSubTime( _res, _t1, _t2 )		\
do {						\
    (_res)->sec = (_t1).sec - (_t2).sec;	\
    (_res)->usec = (_t1).usec - (_t2).usec;	\
    if ( (_res)->usec < 0 ) {			\
	(_res)->usec += (1000 * 1000);		\
	(_res)->sec -= 1;			\
    }						\
} while (0)



#endif /* ! xtime.h */
