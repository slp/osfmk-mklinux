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
 * File : tftp.h
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains TFTP descriptions used for Network bootstrap.
 */

#ifndef __TFTP_H__
#define __TFTP_H__

#define TFTP_ENGINE_START	0
#define TFTP_ENGINE_READ	1
#define TFTP_ENGINE_ABORT	2

#define	TFTP_RETRANSMIT_MAX	15		/* TFTP max # of retransmits */
#define	TFTP_RETRANSMIT		4000		/* TFTP retransmit (4 secs) */
#define	TFTP_MODE		"octet"		/* TFTP request mode */
#define	TFTP_DATA_SIZE		512		/* TFTP max data size */

union frame_tftp {
	struct frame_tftp_1 {
		u16bits	tftp_opcode;		/* Operation (== 1/2) */
		char	tftp_request[1];	/* Request strings */
	} u1;
	struct frame_tftp_3 {
		u16bits	tftp_opcode;		/* Operation (== 3) */
		u16bits	tftp_block;		/* Block number */
		char	tftp_data[512];		/* Block number */
	} u3;
	struct frame_tftp_4 {
		u16bits	tftp_opcode;		/* Operation (== 4) */
		u16bits	tftp_block;		/* Block number */
	} u4;
	struct frame_tftp_5 {
		u16bits	tftp_opcode;		/* Operation (== 5) */
		u16bits	tftp_errorcode;		/* Error code */
		char	tftp_errormsg[512];	/* Error message */
	} u5;
};

#define	TFTP_OPCODE_READ	1		/* Operation == READ */
#define	TFTP_OPCODE_WRITE	2		/* Operation == WRITE */
#define	TFTP_OPCODE_DATA	3		/* Operation == DATA */
#define	TFTP_OPCODE_ACK		4		/* Operation == ACKNOWLEDGE */
#define	TFTP_OPCODE_ERROR	5		/* Operation == ERROR */

#define	TFTP_DATA_MAX		512		/* Max amount of data in a packet */

#define	TFTP_ERROR_UNKNOWN	0		/* Unknown error (cf errmsg) */
#define	TFTP_ERROR_ENOENT	1		/* File not found */
#define	TFTP_ERROR_EACCES	2		/* Access violation */
#define	TFTP_ERROR_ENOSPC	3		/* Disk Full */
#define	TFTP_ERROR_EINVAL	4		/* Invalid TFTP operation */
#define	TFTP_ERROR_EFAULT	5		/* Unknown transfert ID */
#define	TFTP_ERROR_EEXIST	6		/* File already exist */
#define	TFTP_ERROR_EUSER	7		/* No such user */

enum tftp_state {
	TFTP_STATE_FINISHED,		/* File has completely been received */
	TFTP_STATE_START,		/* TFTP has sent a request */
	TFTP_STATE_RUNNING,		/* Request has been accepted */
	TFTP_STATE_ERROR,		/* Error has occured */
	TFTP_STATE_ABORTING
};

extern  u16bits tftp_port;		/* current TFTP client port */

extern int tftp_init(void);
extern int tftp_engine(int, char *, void *, int);
extern int tftp_input(struct udpip_input *);	/* for demux from network layer */

#endif	/* __TFTP_H__ */
