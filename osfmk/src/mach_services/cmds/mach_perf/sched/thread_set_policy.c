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

#define EXPORT_BOOLEAN
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/message.h>
#include <mach/notify.h>
#include <mach/mach_types.h>
#include <mach/mig_errors.h>
#include <mach/msg_type.h>
/* LINTLIBRARY */

extern mach_port_t mig_get_reply_port();
extern void mig_dealloc_reply_port();

#ifndef	mig_internal
#define	mig_internal	static
#endif

#ifndef	mig_external
#define mig_external
#endif

#ifndef	TypeCheck
#define	TypeCheck 1
#endif

#ifndef	UseExternRCSId
#ifdef	hc
#define	UseExternRCSId		1
#endif
#endif

#ifndef	UseStaticMsgType
#if	!defined(hc) || defined(__STDC__)
#define	UseStaticMsgType	1
#endif
#endif

#define msgh_request_port	msgh_remote_port
#define msgh_reply_port		msgh_local_port


/* Routine thread_set_policy */
mig_external kern_return_t thread_set_policy
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t thread,
	int policy,
	int data,
	mach_port_t pset
)
#else
	(thread, policy, data, pset)
	mach_port_t thread;
	int policy;
	int data;
	mach_port_t pset;
#endif
{
	typedef struct {
		mach_msg_header_t Head;
		mach_msg_type_t policyType;
		int policy;
		mach_msg_type_t dataType;
		int data;
		mach_msg_type_t psetType;
		mach_port_t pset;
	} Request;

	typedef struct {
		mach_msg_header_t Head;
		mach_msg_type_t RetCodeType;
		kern_return_t RetCode;
	} Reply;

	union {
		Request In;
		Reply Out;
	} Mess;

	register Request *InP = &Mess.In;
	register Reply *OutP = &Mess.Out;

	mach_msg_return_t msg_result;

#if	UseStaticMsgType
	static mach_msg_type_t policyType = {
		/* msgt_name = */		2,
		/* msgt_size = */		32,
		/* msgt_number = */		1,
		/* msgt_inline = */		TRUE,
		/* msgt_longform = */		FALSE,
		/* msgt_deallocate = */		FALSE,
		/* msgt_unused = */		0
	};
#endif	/* UseStaticMsgType */

#if	UseStaticMsgType
	static mach_msg_type_t dataType = {
		/* msgt_name = */		2,
		/* msgt_size = */		32,
		/* msgt_number = */		1,
		/* msgt_inline = */		TRUE,
		/* msgt_longform = */		FALSE,
		/* msgt_deallocate = */		FALSE,
		/* msgt_unused = */		0
	};
#endif	/* UseStaticMsgType */

#if	UseStaticMsgType
	static mach_msg_type_t psetType = {
		/* msgt_name = */		19,
		/* msgt_size = */		32,
		/* msgt_number = */		1,
		/* msgt_inline = */		TRUE,
		/* msgt_longform = */		FALSE,
		/* msgt_deallocate = */		FALSE,
		/* msgt_unused = */		0
	};
#endif	/* UseStaticMsgType */

#if	UseStaticMsgType
	static mach_msg_type_t RetCodeCheck = {
		/* msgt_name = */		MACH_MSG_TYPE_INTEGER_32,
		/* msgt_size = */		32,
		/* msgt_number = */		1,
		/* msgt_inline = */		TRUE,
		/* msgt_longform = */		FALSE,
		/* msgt_deallocate = */		FALSE,
		/* msgt_unused = */		0
	};
#endif	/* UseStaticMsgType */

#if	UseStaticMsgType
	InP->policyType = policyType;
#else	/* UseStaticMsgType */
	InP->policyType.msgt_name = 2;
	InP->policyType.msgt_size = 32;
	InP->policyType.msgt_number = 1;
	InP->policyType.msgt_inline = TRUE;
	InP->policyType.msgt_longform = FALSE;
	InP->policyType.msgt_deallocate = FALSE;
	InP->policyType.msgt_unused = 0;
#endif	/* UseStaticMsgType */

	InP->policy = policy;

#if	UseStaticMsgType
	InP->dataType = dataType;
#else	/* UseStaticMsgType */
	InP->dataType.msgt_name = 2;
	InP->dataType.msgt_size = 32;
	InP->dataType.msgt_number = 1;
	InP->dataType.msgt_inline = TRUE;
	InP->dataType.msgt_longform = FALSE;
	InP->dataType.msgt_deallocate = FALSE;
	InP->dataType.msgt_unused = 0;
#endif	/* UseStaticMsgType */

	InP->data = data;

#if	UseStaticMsgType
	InP->psetType = psetType;
#else	/* UseStaticMsgType */
	InP->psetType.msgt_name = 19;
	InP->psetType.msgt_size = 32;
	InP->psetType.msgt_number = 1;
	InP->psetType.msgt_inline = TRUE;
	InP->psetType.msgt_longform = FALSE;
	InP->psetType.msgt_deallocate = FALSE;
	InP->psetType.msgt_unused = 0;
#endif	/* UseStaticMsgType */

	InP->pset = pset;

	InP->Head.msgh_bits = MACH_MSGH_BITS_COMPLEX|
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = thread;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_seqno = 0;
	InP->Head.msgh_id = 2689;

	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, sizeof(Request), sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	if (msg_result != MACH_MSG_SUCCESS) {
		if ((msg_result == MACH_SEND_INVALID_REPLY) ||
		    (msg_result == MACH_RCV_INVALID_NAME))
			mig_dealloc_reply_port();
		return msg_result;
	}

	if (OutP->Head.msgh_id != 2789) {
		if (OutP->Head.msgh_id == MACH_NOTIFY_SEND_ONCE)
		return MIG_SERVER_DIED;
		else
		return MIG_REPLY_MISMATCH;
	}

#if	TypeCheck
	if ((OutP->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    (OutP->Head.msgh_size != 32))
		return MIG_TYPE_ERROR;
#endif	/* TypeCheck */

#if	TypeCheck
#if	UseStaticMsgType
	if (* (int *) &OutP->RetCodeType != * (int *) &RetCodeCheck)
#else	/* UseStaticMsgType */
	if ((OutP->RetCodeType.msgt_inline != TRUE) ||
	    (OutP->RetCodeType.msgt_longform != FALSE) ||
	    (OutP->RetCodeType.msgt_name != MACH_MSG_TYPE_INTEGER_32) ||
	    (OutP->RetCodeType.msgt_number != 1) ||
	    (OutP->RetCodeType.msgt_size != 32))
#endif	/* UseStaticMsgType */
		return MIG_TYPE_ERROR;
#endif	/* TypeCheck */

	return OutP->RetCode;
}
