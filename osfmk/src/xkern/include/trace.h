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
 * trace.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:27:13 $
 */

#define TR_NEVER		100 /* we'll never use this */
#define TR_FULL_TRACE		25  /* every subroutine entry (sometimes exit, too */
#define TR_DETAILED		 9  /* all functions plus dumps of data structures at strategic points */
#define TR_FUNCTIONAL_TRACE	 7  /* all the functions of the module and their parameters */
#define TR_MORE_EVENTS		 6  /* probably should have used 7, instead */
#define TR_EVENTS		 5  /* more detail than major_events */
#define TR_SOFT_ERRORS		 4  /* mild warning */
#define TR_SOFT_ERROR TR_SOFT_ERRORS
#define TR_MAJOR_EVENTS		 3  /* open, close, etc. */
#define TR_GROSS_EVENTS		 2  /* probably should have used 3, instead */
#define TR_ERRORS		 1  /* serious non-fatal errors, low-level trace (init, closesessn, etc. */
#define TR_ALWAYS		 0  /* normally only used during protocol development */

