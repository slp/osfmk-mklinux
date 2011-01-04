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

#ifndef _mig_log_
#define _mig_log_

typedef enum {
	MACH_MSG_LOG_USER,
	MACH_MSG_LOG_SERVER
} mig_who_t;

typedef enum {
	MACH_MSG_REQUEST_BEING_SENT,
	MACH_MSG_REQUEST_BEING_RCVD,
	MACH_MSG_REPLY_BEING_SENT,
	MACH_MSG_REPLY_BEING_RCVD
} mig_which_event_t;

typedef enum {
	MACH_MSG_ERROR_WHILE_PARSING,
	MACH_MSG_ERROR_UNKNOWN_ID
} mig_which_error_t;

extern void MigEventTracer
#if     defined(__STDC__)
(
	mig_who_t who,	
	mig_which_event_t what,
	mach_msg_id_t msgh_id,
	unsigned int size,
	unsigned int kpd,
	unsigned int retcode,
	unsigned int ports,
	unsigned int oolports,
	unsigned int ool,
	char *file,
	unsigned int line
);
#else  	/* !defined(__STDC__) */
();
#endif  /* !defined(__STDC__) */

extern void MigEventErrors
#if     defined(__STDC__)
(
	mig_who_t who,	
	mig_which_error_t what,
	void *par,
	char *file,
	unsigned int line
);
#else  	/* !defined(__STDC__) */
();
#endif  /* !defined(__STDC__) */

extern int mig_errors;
extern int mig_tracing;

#define LOG_ERRORS      if (mig_errors)  MigEventErrors
#define LOG_TRACE       if (mig_tracing) MigEventTracer

#endif  /* _mach_log_ */

