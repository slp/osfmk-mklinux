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
/* CMU_HIST */
/*
 * Revision 2.7  91/06/19  11:57:24  rvb
 * 	File moved here from mips/PMAX since it is now "MI" code, also
 * 	used by Vax3100 and soon -- the omron luna88k.
 * 	[91/06/04            rvb]
 * 
 * Revision 2.6  91/05/14  17:28:17  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/05/13  06:33:17  af
 * 	Define bus phases here.  More defines for return data from
 * 	various commands that are now actually used.
 * 
 * 	Added def of max addressable block for short read/write.
 * 	Added missing sense key values (they were on the back of
 * 	the page, oops).  Added extended sense data defs.
 * 	Added mode sense defs.
 * 
 * Revision 2.4.2.1  91/05/12  16:22:46  af
 * 	Define bus phases here.  More defines for return data from
 * 	various commands that are now actually used.
 * 
 * 	Added def of max addressable block for short read/write.
 * 	Added missing sense key values (they were on the back of
 * 	the page, oops).  Added extended sense data defs.
 * 	Added mode sense defs.
 * 
 * Revision 2.4  91/02/05  17:44:46  mrt
 * 	Added author notices
 * 	[91/02/04  11:18:12  mrt]
 * 
 * 	Changed to use new Mach copyright
 * 	[91/02/02  12:16:54  mrt]
 * 
 * Revision 2.3  90/12/05  23:34:41  af
 * 	Decls of status byte was upside down.
 * 	[90/12/03  23:38:44  af]
 * 
 * Revision 2.1.1.1  90/11/01  03:38:31  af
 * 	Created, from the SCSI specs:
 * 	"Small Computer Systems Interface (SCSI)", ANSI Draft
 * 	X3T9.2/82-2 - Rev 17B December 1985
 * 	[90/09/03            af]
 */
/* CMU_ENDHIST */

/* INTEL_HIST */
/*
 * Revision 1.9  1994/04/20  22:15:18  richardg
 *  Reviewer: Jerrie Coffman
 *  Risk: low
 *  Benefit or PTS #: added for 3480 Tape Library support - LOAD DISPLAY.
 *  Testing: tape EAT's and costom code to exercise the ioctl
 *  Module(s):
 *
 * Revision 1.8  1994/03/08  17:33:23  richardg
 *  Reviewer: Jerrie Coffman
 *  Risk: low
 *  Benefit or PTS #: add 3480 tape library support
 *  Testing: tape EAT's, a short program that uses the MTLOCATE ioctl
 *  Module(s): added the define for the SCSI LOCATE command
 *
 * Revision 1.7  1993/10/13  17:52:57  richardg
 * added support for media changer devices. SCSI_JUKEBOX
 *
 * Revision 1.6  1993/09/24  17:28:02  jerrie
 * Added definitions for the data compression characteristics and
 * device configuration mode pages to support tape data compression.
 *
 * Revision 1.5  1993/06/30  22:53:51  dleslie
 * Adding copyright notices required by legal folks
 *
 * Revision 1.4  1993/04/27  20:47:58  dleslie
 * Copy of R1.0 sources onto main trunk
 *
 * Revision 1.2.6.2  1993/04/22  18:53:26  dleslie
 * First R1_0 release
 *
*/
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 */
/*
 *	File: scsi.h
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	9/90
 *
 *	Definitions of the SCSI-1 Standard
 */

#ifndef	_SCSI_SCSI_H_
#define	_SCSI_SCSI_H_

#include <scsi/scsi_endian.h>

/*
 * Bus phases
 */

#define	SCSI_IO		0x01		/* Input/Output */
#define	SCSI_CD		0x02		/* Command/Data */
#define	SCSI_MSG	0x04		/* Message */

#define	SCSI_PHASE_MASK	0x07
#define	SCSI_PHASE(x)	((x)&SCSI_PHASE_MASK)

#define	SCSI_PHASE_DATAO	0x00				/* 0 */
#define	SCSI_PHASE_DATAI	SCSI_IO				/* 1 */
#define	SCSI_PHASE_CMD		SCSI_CD				/* 2 */
#define	SCSI_PHASE_STATUS	(SCSI_CD|SCSI_IO)		/* 3 */
								/* 4..5 ANSI reserved */
#define	SCSI_PHASE_MSG_OUT	(SCSI_MSG|SCSI_CD)		/* 6 */
#define	SCSI_PHASE_MSG_IN	(SCSI_MSG|SCSI_CD|SCSI_IO)	/* 7 */

/*
 * Single byte messages
 *
 * originator:	I-nitiator T-arget
 * T-support:	M-andatory O-ptional
 */

#define SCSI_COMMAND_COMPLETE		0x00	/* M T   */
#define SCSI_EXTENDED_MESSAGE		0x01	/*   IT  */
#define SCSI_SAVE_DATA_POINTER		0x02	/* O T   */
#define SCSI_RESTORE_POINTERS		0x03	/* O T   */
#define SCSI_DISCONNECT			0x04	/* O T   */
#define SCSI_I_DETECTED_ERROR		0x05	/* M I   */
#define SCSI_ABORT			0x06	/* M I   */
#define SCSI_MESSAGE_REJECT		0x07	/* M IT  */
#define SCSI_NOP			0x08	/* M I   */
#define SCSI_MSG_PARITY_ERROR		0x09	/* M I   */
#define SCSI_LNKD_CMD_COMPLETE		0x0a	/* O T   */
#define SCSI_LNKD_CMD_COMPLETE_F	0x0b	/* O T   */
#define SCSI_BUS_DEVICE_RESET		0x0c	/* M I   */
					/* 0x0d..0x11 scsi2 */
					/* 0x12..0x1f reserved */
#define SCSI_IDENTIFY			0x80	/* IT */
#	define SCSI_IFY_ENABLE_DISCONNECT	0x40	/* I  */
#	define SCSI_IFY_LUNTAR			0x20	/* IT */
#	define SCSI_IFY_LUN_MASK		0x07	/* IT */


/* Message codes 0x30..0x7f are reserved */

/*
 * Extended messages, codes and formats
 */

#define SCSI_MODIFY_DATA_PTR		0x00	/* T  */
typedef struct {
	unsigned char	xtn_msg_tag;		/* const 0x01 */
	unsigned char	xtn_msg_len;		/* const 0x05 */
	unsigned char	xtn_msg_code;		/* const 0x00 */
	unsigned char	xtn_msg_arg_1000;	/* MSB, signed 2cmpl */
	unsigned char	xtn_msg_arg_0200;
	unsigned char	xtn_msg_arg_0030;
	unsigned char	xtn_msg_arg_0004;	/* LSB */
} scsi_mod_ptr_t;

#define SCSI_SYNC_XFER_REQUEST		0x01	/* IT */
typedef struct {
	unsigned char	xtn_msg_tag;		/* const 0x01 */
	unsigned char	xtn_msg_len;		/* const 0x03 */
	unsigned char	xtn_msg_code;		/* const 0x01 */
	unsigned char	xtn_msg_xfer_period;	/* times 4nsecs */
	unsigned char	xtn_msg_xfer_offset;	/* pending ack window */
#define SCSI_SYNCH_XFER_OFFANY	0xff		/* T  unlimited */
} scsi_synch_xfer_req_t;

#define SCSI_XTN_IDENTIFY		0x02	/* IT -2 */
typedef struct {
	unsigned char	xtn_msg_tag;		/* const 0x01 */
	unsigned char	xtn_msg_len;		/* const 0x02 */
	unsigned char	xtn_msg_code;		/* const 0x02 */
	unsigned char	xtn_msg_sublun;
} scsi_xtn_identify_t;

					/* 0x03..0x7f reserved */

#define SCSI_XTN_VENDOR_UQE		0x80	/* vendor unique bit */
typedef struct {
	unsigned char	xtn_msg_tag;		/* const 0x01 */
	unsigned char	xtn_msg_len;		/* args' len+1 (0-->256)*/
	unsigned char	xtn_msg_code;		/* const 0x80..0xff */
	unsigned char	xtn_msg_args[1];	/* 0..255 bytes */
} scsi_xtn_vedor_unique_t;


/*
 * Commands, generic structures
 */

/* SIX byte commands */
typedef struct {
	unsigned char	scsi_cmd_code;		/* group(7..5) and command(4..1) */
#define SCSI_CODE_GROUP		0xe0
#define SCSI_CODE_CMD		0x1f
	unsigned char	scsi_cmd_lun_and_lba1;	/* lun(7..5) and block# msb[20..16] */
#define SCSI_LUN_MASK		0xe0
#define SCSI_LBA_MASK		0x1f
	unsigned char	scsi_cmd_lba2;		/* block#[15.. 8] */
	unsigned char	scsi_cmd_lba3;		/* block#[ 7.. 0] */
	unsigned char	scsi_cmd_xfer_len;	/* if required */
	unsigned char	scsi_cmd_ctrl_byte;	/* contains: */
#define	SCSI_CTRL_VUQ		0xc0		/* vendor unique bits */
#define SCSI_CTRL_RESVD		0x3c		/* reserved, mbz */
#define SCSI_CTRL_FLAG		0x02		/* send a complete_with_flag at end */
#define SCSI_CTRL_LINK		0x01		/* link this command with next */
} scsi_command_group_0;

/* TEN byte commands */
typedef struct {
	unsigned char	scsi_cmd_code;		/* group(7..5) and command(4..1) */
	unsigned char	scsi_cmd_lun_and_relbit;/* lun(7..5) and RelAdr(0) */
#define SCSI_RELADR		0x01
	unsigned char	scsi_cmd_lba1;		/* block#[31..24] */
	unsigned char	scsi_cmd_lba2;		/* block#[23..16] */
	unsigned char	scsi_cmd_lba3;		/* block#[15.. 8] */
	unsigned char	scsi_cmd_lba4;		/* block#[ 7.. 0] */
	unsigned char	scsi_cmd_xxx;		/* reserved, mbz */
	unsigned char	scsi_cmd_xfer_len_1;	/* if required */
	unsigned char	scsi_cmd_xfer_len_2;	/* if required */
	unsigned char	scsi_cmd_ctrl_byte;	/* see above */
} scsi_command_group_1,
  scsi_command_group_2;

/* TWELVE byte commands */
typedef struct {
	unsigned char	scsi_cmd_code;		/* group(7..5) and command(4..1) */
	unsigned char	scsi_cmd_lun_and_relbit;/* lun(7..5) and RelAdr(0) */
	unsigned char	scsi_cmd_lba1;		/* block#[31..24] */
	unsigned char	scsi_cmd_lba2;		/* block#[23..16] */
	unsigned char	scsi_cmd_lba3;		/* block#[15.. 8] */
	unsigned char	scsi_cmd_lba4;		/* block#[ 7.. 0] */
	unsigned char	scsi_cmd_xfer_len_1;	/* if required */
	unsigned char	scsi_cmd_xfer_len_2;	/* if required */
	unsigned char	scsi_cmd_xfer_len_3;	/* if required */
	unsigned char	scsi_cmd_xfer_len_4;	/* if required */
	unsigned char	scsi_cmd_xxx1;		/* reserved, mbz */
	unsigned char	scsi_cmd_ctrl_byte;	/* see above */
} scsi_command_group_5;


/*
 * Commands, codes and aliases
 */

				/* GROUP 0 */
#define SCSI_CMD_TEST_UNIT_READY	0x00	/* O all 2M all */
#define	scsi_cmd_test_unit_ready_t	scsi_command_group_0

#define SCSI_CMD_REZERO_UNIT		0x01	/* O disk worm rom */
#define SCSI_CMD_REWIND			0x01	/* M tape */
#define	scsi_cmd_rewind_t	scsi_command_group_0
#define	scsi_cmd_rezero_t	scsi_command_group_0
#	define SCSI_CMD_REW_IMMED		0x01

					/* 0x02 vendor unique */

#define SCSI_CMD_REQUEST_SENSE		0x03	/* M all */
#define	scsi_cmd_request_sense_t	scsi_command_group_0
#	define scsi_cmd_allocation_length	scsi_cmd_xfer_len

#define SCSI_CMD_FORMAT_UNIT		0x04	/* M disk O prin */
#define	scsi_cmd_format_t	scsi_command_group_0
#	define SCSI_CMD_FMT_FMTDATA		0x10
#	define SCSI_CMD_FMT_CMPLIST		0x08
#	define SCSI_CMD_FMT_LIST_TYPE		0x07
#	define scsi_cmd_intleave1		scsi_cmd_lba3
#	define scsi_cmd_intleave2		scsi_cmd_xfer_len

#define SCSI_CMD_READ_BLOCK_LIMITS	0x05	/* E tape */
#define	scsi_cmd_block_limits_t	scsi_command_group_0

#define SCSI_CMD_LOAD_DISPLAY		0x06  /* vendor unique 3480 tape */
#define	scsi_cmd_load_display_t		scsi_command_group_0
#	define SCSI_CMD_LD_LEN		0x11		/* 17 bytes */

#define SCSI_CMD_REASSIGN_BLOCKS	0x07	/* O disk worm */
#define	scsi_cmd_reassign_blocks_t	scsi_command_group_0

#define SCSI_CMD_READ			0x08	/* M disk tape O worm rom */
#define	SCSI_CMD_RECEIVE		0x08	/* O proc */
#define	scsi_cmd_read_t	scsi_command_group_0
#	define SCSI_CMD_TP_FIXED		0x01		/* tape */
#	define scsi_cmd_tp_len1			scsi_cmd_lba2
#	define scsi_cmd_tp_len2			scsi_cmd_lba3
#	define scsi_cmd_tp_len3			scsi_cmd_xfer_len
					/* largest addressable blockno */
#define	SCSI_CMD_READ_MAX_LBA		((1 << 21) - 1)

					/* 0x09 vendor unique */

#define SCSI_CMD_WRITE			0x0a	/* M disk tape O worm */
#define SCSI_CMD_PRINT			0x0a	/* M prin */
#define SCSI_CMD_SEND			0x0a	/* M proc */
#define	scsi_cmd_write_t	scsi_command_group_0

#define SCSI_CMD_SEEK			0x0b	/* O disk worm rom */
#define SCSI_CMD_TRACK_SELECT		0x0b	/* O tape */
#define SCSI_CMD_SLEW_AND_PRINT		0x0b	/* O prin */
#define	scsi_cmd_seek_t	scsi_command_group_0
#	define SCSI_CMD_SLW_CHANNEL		0x01
#	define scsi_cmd_tp_trackno		scsi_cmd_xfer_len
#	define scsi_cmd_slew_value		scsi_cmd_lba2

					/* 0x0c..0x0e vendor unique */

#define SCSI_CMD_READ_REVERSE		0x0f	/* O tape */
#define	scsi_cmd_rev_read_t	scsi_command_group_0

#define SCSI_CMD_WRITE_FILEMARKS	0x10	/* M tape */
#	define SCSI_CMD_WF_IMMED	0x01

#define SCSI_CMD_FLUSH_BUFFER		0x10	/* M prin */
#define	scsi_cmd_write_fil_t	scsi_command_group_0

#define SCSI_CMD_SPACE			0x11	/* O tape */
#define	scsi_cmd_space_t	scsi_command_group_0
#	define SCSI_CMD_SP_BLOCKS		0x00
#	define SCSI_CMD_SP_FIL			0x01
#	define SCSI_CMD_SP_SEQ_FIL		0x02
#	define SCSI_CMD_SP_EOT			0x03

#define SCSI_CMD_INQUIRY		0x12	/* E all (2M all) */
#define	scsi_cmd_inquiry_t	scsi_command_group_0
#	define SCSI_CMD_INQ_EVPD		0x01	/* 2 */
#	define scsi_cmd_page_code		scsi_cmd_lba2	/* 2 */

#define SCSI_CMD_VERIFY_0		0x13	/* O tape */
#define	scsi_cmd_verify_t	scsi_command_group_0
#	define SCSI_CMD_VFY_BYTCMP		0x02

#define SCSI_CMD_RECOVER_BUFFERED_DATA	0x14	/* O tape prin */
#define	scsi_cmd_recover_buffer_t	scsi_command_group_0

#define SCSI_CMD_MODE_SELECT		0x15	/* O disk tape prin worm rom */
#	define SCSI_CMD_MSL_PF			0x10
#	define SCSI_CMD_MSL_SP			0x01
#define	scsi_cmd_mode_select_t	scsi_command_group_0

#define SCSI_CMD_RESERVE		0x16	/* O disk tape prin worm rom */
#define	scsi_cmd_reserve_t	scsi_command_group_0
#	define SCSI_CMD_RES_3RDPTY		0x10
#	define SCSI_CMD_RES_3RDPTY_DEV		0x0e
#	define SCSI_CMD_RES_EXTENT		0x01
#	define scsi_cmd_reserve_id		scsi_cmd_lba2
#	define scsi_cmd_extent_llen1		scsi_cmd_lba3
#	define scsi_cmd_extent_llen2		scsi_cmd_xfer_len

#define SCSI_CMD_RELEASE		0x17	/* O disk tape prin worm rom */
#define	scsi_cmd_release_t	scsi_command_group_0

#define SCSI_CMD_COPY			0x18	/* O all */
#define	scsi_cmd_copy_t	scsi_command_group_0
#	define SCSI_CMD_CPY_PAD			0x01	/* 2 */
#	define scsi_cmd_paraml_len0		scsi_cmd_lba2
#	define scsi_cmd_paraml_len1		scsi_cmd_lba3
#	define scsi_cmd_paraml_len2		scsi_cmd_xfer_len

#define SCSI_CMD_ERASE			0x19	/* O tape */
#define	scsi_cmd_erase_t	scsi_command_group_0
#	define SCSI_CMD_ER_LONG			0x01

#define SCSI_CMD_MODE_SENSE		0x1a	/* O disk tape prin worm rom */
#define	scsi_cmd_mode_sense_t	scsi_command_group_0
#	define scsi_cmd_ms_pagecode		scsi_cmd_lba2

#define SCSI_CMD_START_STOP_UNIT	0x1b	/* O disk prin worm rom */
#define SCSI_CMD_LOAD_UNLOAD		0x1b	/* O tape */
#define	scsi_cmd_start_t	scsi_command_group_0
#	define SCSI_CMD_SS_IMMED		0x01
#	define scsi_cmd_ss_flags		scsi_cmd_xfer_len
#	define SCSI_CMD_SS_START		0x01
#	define SCSI_CMD_SS_RETEN		0x02
#	define SCSI_CMD_SS_RETAIN		0x01
#	define SCSI_CMD_SS_EJECT		0x02

#define SCSI_CMD_RECEIVE_DIAG_RESULTS	0x1c	/* O all */
#define	scsi_cmd_receive_diag_t	scsi_command_group_0
#	define scsi_cmd_allocation_length1	scsi_cmd_lba3
#	define scsi_cmd_allocation_length2	scsi_cmd_xfer_len

#define SCSI_CMD_SEND_DIAGNOSTICS	0x1d	/* O all */
#define	scsi_cmd_send_diag_t	scsi_command_group_0
#	define SCSI_CMD_DIAG_SELFTEST		0x04
#	define SCSI_CMD_DIAG_DEVOFFL		0x02
#	define SCSI_CMD_DIAG_UNITOFFL		0x01

#define SCSI_CMD_PREVENT_ALLOW_REMOVAL	0x1e	/* O disk tape worm rom */
#define	scsi_cmd_medium_removal_t	scsi_command_group_0
#	define scsi_cmd_pa_prevent		scsi_cmd_xfer_len /* 0x1 */

					/* 0x1f reserved */

				/* GROUP 1 */
					/* 0x20..0x24 vendor unique */

#define SCSI_CMD_READ_CAPACITY		0x25	/* E disk worm rom */
#define	scsi_cmd_read_capacity_t	scsi_command_group_1
#	define scsi_cmd_rcap_flags		scsi_cmd_xfer_len_2
#	define SCSI_CMD_RCAP_PMI		0x01

					/* 0x26..0x27 vendor unique */

#define SCSI_CMD_LONG_READ			0x28	/* E disk M worm rom */
#define	scsi_cmd_long_read_t	scsi_command_group_1

					/* 0x29 vendor unique */

#define SCSI_CMD_LONG_WRITE			0x2a	/* E disk M worm */
#define	scsi_cmd_long_write_t	scsi_command_group_1

#define SCSI_CMD_LONG_SEEK			0x2b	/* O disk worm rom */
#define SCSI_CMD_LOCATE				0x2b	/* O tape */
#define	scsi_cmd_long_seek_t	scsi_command_group_1
#define	scsi_cmd_locate_t	scsi_command_group_1
#	define SCSI_CMD_LOCATE_BT		0x04

					/* 0x2c..0x2d vendor unique */

#define SCSI_CMD_WRITE_AND_VERIFY	0x2e	/* O disk worm */
#define	scsi_cmd_write_vfy_t	scsi_command_group_1
#	define SCSI_CMD_VFY_BYTCHK		0x02

#define SCSI_CMD_VERIFY_1		0x2f	/* O disk worm rom */
#define	scsi_cmd_verify_long_t	scsi_command_group_1
#	define SCSI_CMD_VFY_BLKVFY		0x04

#define SCSI_CMD_SEARCH_HIGH		0x30	/* O disk worm rom */
#define	scsi_cmd_search_t	scsi_command_group_1
#	define SCSI_CMD_SRCH_INVERT		0x10
#	define SCSI_CMD_SRCH_SPNDAT		0x02

#define SCSI_CMD_SEARCH_EQUAL		0x31	/* O disk worm rom */
#define SCSI_CMD_SEARCH_LOW		0x32	/* O disk worm rom */

#define SCSI_CMD_SET_LIMITS		0x33	/* O disk worm rom */
#define	scsi_cmd_set_limits_t	scsi_command_group_1
#	define SCSI_CMD_SL_RDINH		0x02
#	define SCSI_CMD_SL_WRINH		0x01

					/* 0x34..0x38 reserved */

#define SCSI_CMD_COMPARE		0x39	/* O all */
#define	scsi_cmd_compare_t	scsi_command_group_1
#	define scsi_cmd_1_paraml1		scsi_cmd_lba2
#	define scsi_cmd_1_paraml2		scsi_cmd_lba3
#	define scsi_cmd_1_paraml3		scsi_cmd_lba4

#define SCSI_CMD_COPY_AND_VERIFY	0x3a	/* O all */
#define	scsi_cmd_copy_vfy_t	scsi_command_group_1
#	define SCSI_CMD_CPY_BYTCHK		0x02

					/* 0x3b..0x3f reserved */

				/* GROUP 2 */
					/* 0x40..0x5f reserved */

				/* GROUP 3 */
					/* 0x60..0x7f reserved */

				/* GROUP 4 */
					/* 0x80..0x9f reserved */

				/* GROUP 5 */
					/* 0xa0..0xaf vendor unique */
					/* 0xb0..0xbf reserved */

				/* GROUP 6 */
					/* 0xc0..0xdf vendor unique */

				/* GROUP 7 */
					/* 0xe0..0xff vendor unique */


/*
 * Command-specific results and definitions
 */

/* inquiry data */
typedef struct {
	unsigned char	periph_type;
#define	SCSI_DISK	0x00
#define SCSI_TAPE	0x01
#define	SCSI_PRINTER	0x02
#define	SCSI_CPU	0x03
#define	SCSI_WORM	0x04
#define	SCSI_CDROM	0x05
#define	SCSI_JUKEBOX	0x08

	BITFIELD_2( unsigned char,
			device_type : 7,
			rmb : 1);
	BITFIELD_3( unsigned char,
			ansi : 3,
			ecma : 3,
			iso : 2);
	unsigned char	reserved;
	unsigned char	length;
	unsigned char	param[1];
} scsi_inquiry_data_t;

#define	SCSI_INQ_STD_DATA	-1

/* request sense data */
#define SCSI_SNS_NOSENSE	0x0
#define SCSI_SNS_RECOVERED	0x1
#define SCSI_SNS_NOTREADY	0x2
#define SCSI_SNS_MEDIUM_ERR	0x3
#define SCSI_SNS_HW_ERR		0x4
#define SCSI_SNS_ILLEGAL_REQ	0x5
#define SCSI_SNS_UNIT_ATN	0x6
#define SCSI_SNS_PROTECT	0x7
#define	SCSI_SNS_BLANK_CHK	0x8
#define	SCSI_SNS_VUQE		0x9
#define	SCSI_SNS_COPY_ABRT	0xa
#define	SCSI_SNS_ABORTED	0xb
#define	SCSI_SNS_EQUAL		0xc
#define	SCSI_SNS_VOLUME_OVFL	0xd
#define	SCSI_SNS_MISCOMPARE	0xe
#define	SCSI_SNS_RESERVED	0xf

typedef struct {
	BITFIELD_3( unsigned char,
		error_code : 4,
		error_class : 3,
		addr_valid : 1);
#	define	SCSI_SNS_XTENDED_SENSE_DATA	0x7	/* e.g. error_class=7 */
	union {
	    struct {
		BITFIELD_2(unsigned char,
			lba_msb : 5,
			vuqe : 3);
		unsigned char	lba;
		unsigned char	lba_lsb;
	    } non_xtnded;
	    struct {
		unsigned char	segment_number;
		BITFIELD_5(unsigned char,
			sense_key : 4,
			res : 1,
			ili : 1,
			eom : 1,
			fm : 1);
		unsigned char	info0;
		unsigned char	info1;
		unsigned char	info2;
		unsigned char	info3;
		unsigned char	add_len;
		unsigned char	add_bytes[1];/* VARSIZE */
#ifdef	POWERMAC
#define	sense_code	u.xtended.add_bytes[4]
#define	extsense_code	u.xtended.add_bytes[5]
#endif
	    } xtended;
	} u;
} scsi_sense_data_t;


/* mode select params */
typedef struct {
	unsigned char	reserved1;
	unsigned char	medium_type;
	unsigned char	device_spec;	/* device specific mode select data */
	unsigned char	desc_len;
	struct scsi_mode_parm_blockdesc {
		unsigned char	density_code;
		unsigned char	nblocks1;
		unsigned char	nblocks2;
		unsigned char	nblocks3;
		unsigned char	reserved;
		unsigned char	reclen1;
		unsigned char	reclen2;
		unsigned char	reclen3;
	} descs[1];		/* VARSIZE, really */
	/* vuqe data might follow */
} scsi_mode_select_param_t;

/* mode sense data (TAPE) */
typedef struct {
	unsigned char	data_len;
	unsigned char	medium_type;
	unsigned char	device_spec;	/* device specific mode sense data */
	unsigned char	bdesc_len;
	struct {
	    unsigned char	density_code;
	    unsigned char	no_blks_msb;
	    unsigned char	no_blks;
	    unsigned char	no_blks_lsb;
	    unsigned char	reserved;
	    unsigned char	blen_msb;
	    unsigned char	blen;
	    unsigned char	blen_lsb;
	} bdesc[1];	/* VARSIZE */
	/* vuqe data might follow */
} scsi_mode_sense_data_t;

/* DISK device specific mode data */
typedef struct {
	BITFIELD_2(unsigned char,
		reserved : 7,
		wp 	 : 1);
} disk_spec_t;

/* TAPE device specific mode data */
typedef struct {
	BITFIELD_3(unsigned char,
		speed	    : 4,
		buffer_mode : 3,
		wp 	    : 1);
} tape_spec_t;

/* page 1 - read / write error recovery page */
typedef struct {
        BITFIELD_3(unsigned char,
                page_code : 6,
                reserved1 : 1,
                ps        : 1);
        unsigned char   page_length;
        unsigned char   flags;
#       define  PAGE1_AWRE      0x80
#       define  PAGE1_ARRE      0x40
#       define  PAGE1_TB        0x20
#       define  PAGE1_RC        0x10
#       define  PAGE1_EER       0x08
#       define  PAGE1_PER       0x04
#       define  PAGE1_DTE       0x02
#       define  PAGE1_DCR       0x01
        unsigned char   read_retry_count;
        unsigned char   correction_span;
        unsigned char   head_offset_count;
        unsigned char   data_strobe_offset_count;
        unsigned char   reserved2;
        unsigned char   write_retry_count;
        unsigned char   reserved3;
        unsigned char   recovery_time_limit[2];
} scsi_mode_sense_page1_t;

/* page 3 - format device page */
typedef	struct {
	BITFIELD_3(unsigned char,
		page_code : 6,
		reserved1 : 1,
		ps	  : 1);
	unsigned char	page_length;
	unsigned char	tracks_per_zone_msb;
	unsigned char	tracks_per_zone_lsb;
	unsigned char	alt_sectors_per_zone_msb;
	unsigned char	alt_sectors_per_zone_lsb;
	unsigned char	alt_tracks_per_zone_msb;
	unsigned char	alt_tracks_per_zone_lsb;
	unsigned char	alt_tracks_per_unit_msb;
	unsigned char	alt_tracks_per_unit_lsb;
	unsigned char	sectors_per_track_msb;
	unsigned char	sectors_per_track_lsb;
	unsigned char	bytes_per_sector_msb;
	unsigned char	bytes_per_sector_lsb;
	unsigned char	interleave_msb;
	unsigned char	interleave_lsb;
	unsigned char	track_skew_msb;
	unsigned char	track_skew_lsb;
	unsigned char	cylinder_skew_msb;
	unsigned char	cylinder_skew_lsb;
	BITFIELD_5(unsigned char,
		reserved2 : 4,
		surf	  : 1,
		rmb	  : 1,
		hsec	  : 1,
		ssec	  : 1);
	unsigned char	reserved3;
	unsigned char	reserved4;
	unsigned char	reserved5;
} scsi_mode_sense_page3_t;

/* page 4 - rigid disk drive geometry page */
typedef	struct {
	BITFIELD_3(unsigned char,
		page_code : 6,
		reserved1 : 1,
		ps	  : 1);
	unsigned char	page_length;
	unsigned char	number_of_cylinders_msb;
	unsigned char	number_of_cylinders;
	unsigned char	number_of_cylinders_lsb;
	unsigned char	number_of_heads;
	unsigned char	start_cylinder_write_precomp_msb;
	unsigned char	start_cylinder_write_precomp;
	unsigned char	start_cylinder_write_precomp_lsb;
	unsigned char	start_cylinder_reduced_write_msb;
	unsigned char	start_cylinder_reduced_write;
	unsigned char	start_cylinder_reduced_write_lsb;
	unsigned char	drive_step_rate_msb;
	unsigned char	drive_step_rate_lsb;
	unsigned char	landing_zone_cylinder_msb;
	unsigned char	landing_zone_cylinder;
	unsigned char	landing_zone_cylinder_lsb;
	BITFIELD_2(unsigned char,
		rpl	  : 1,
		reserved2 : 7);
	unsigned char	rotational_offset;
	unsigned char	reserved3;
	unsigned char	medium_rotation_rate_msb;
	unsigned char	medium_rotation_rate_lsb;
	unsigned char	reserved4;
	unsigned char	reserved5;
} scsi_mode_sense_page4_t;

/* page 5 - flexible disk drive geometry page */
typedef	struct {
	BITFIELD_3(unsigned char,
		page_code : 6,
		reserved1 : 1,
		ps	  : 1);
	unsigned char	page_length;
	unsigned char	transfer_rate_msb;
	unsigned char	transfer_rate_lsb;
	unsigned char	number_of_heads;
	unsigned char   sectors_per_track;
	unsigned char	bytes_per_sector_msb;
	unsigned char	bytes_per_sector_lsb;
	unsigned char	number_of_cylinders_msb;
	unsigned char	number_of_cylinders_lsb;
	unsigned char	starting_cyl_wpc_msb;
	unsigned char	starting_cyl_wpc_lsb;
	unsigned char	starting_cyl_rwc_msb;
	unsigned char	starting_cyl_rwc_lsb;
	unsigned char	drive_step_rate_msb;
	unsigned char	drive_step_rate_lsb;
	unsigned char	drive_step_pule_width;
	unsigned char	head_settle_delay_msb;
	unsigned char	head_settle_delay_lsb;
	unsigned char	motor_on_delay;
	unsigned char	motor_off_delay;
	BITFIELD_4(unsigned char,
	        reserved2           : 5,
	        motor_on            : 1,
		start_sector_number : 1,
		true_rdy            : 1);
	BITFIELD_2(unsigned char,
		step_pulse_per_cyl  : 4,
	        reserved3           : 4);
	unsigned char	write_precomp_value;
	unsigned char	head_load_delay;
	unsigned char	head_unload_delay;
	BITFIELD_2(unsigned char,
	        pin_2               : 4, 
	        pin_34              : 4);		   
	BITFIELD_2(unsigned char,
	        pin_1               : 4, 
	        pin_4               : 4);		   
	unsigned char	reserved4;
	unsigned char	reserved5;
	unsigned char	reserved6;
	unsigned char	reserved7;
} scsi_mode_sense_page5_t;

#ifdef	PARAGON860	/* Tape Compression Support */

/* page 15 (0x0f) - data compression characteristics page */
typedef	struct {
	BITFIELD_3(unsigned char,
		page_code : 6,
		reserved1 : 1,
		ps	  : 1);
	unsigned char	page_length;
	BITFIELD_3(unsigned char,
		reserved2 : 6,
		dcc	  : 1,
		dce	  : 1);
	BITFIELD_3(unsigned char,
		reserved3 : 5,
		red	  : 2,
		dde	  : 1);
	unsigned char	compression_algorithm_msb;
	unsigned char	compression_algorithm_sb1;
	unsigned char	compression_algorithm_sb2;
	unsigned char	compression_algorithm_lsb;
	unsigned char	decompression_algorithm_msb;
	unsigned char	decompression_algorithm_sb1;
	unsigned char	decompression_algorithm_sb2;
	unsigned char	decompression_algorithm_lsb;
	unsigned char	reserved4;
	unsigned char	reserved5;
	unsigned char	reserved6;
	unsigned char	reserved7;
} scsi_mode_sense_page15_t;

/* page 16 (0x10) - device configuration page */
typedef	struct {
	BITFIELD_3(unsigned char,
		page_code : 6,
		reserved1 : 1,
		ps	  : 1);
	unsigned char	page_length;
	BITFIELD_4(unsigned char,
		active_format : 5,
		caf	      : 1,
		cap	      : 1,
		reserved2     : 1);
	unsigned char	active_partition;
	unsigned char	write_buffer_full_ratio;
	unsigned char	read_buffer_full_ratio;
	unsigned char	write_delay_time_msb;
	unsigned char	write_delay_time_lsb;
	BITFIELD_7(unsigned char,
		rew  : 1,
		rbo  : 1,
		socf : 2,
		avc  : 1,
		rsmk : 1,
		bis  : 1,
		dbr  : 1);
	unsigned char	gap_size;
	BITFIELD_4(unsigned char,
		reserved3   : 3,
		sew	    : 1,
		eeg	    : 1,
		eod_defined : 3);
	unsigned char	buffer_size_at_early_warning_msb;
	unsigned char	buffer_size_at_early_warning;
	unsigned char	buffer_size_at_early_warning_lsb;
	unsigned char	select_data_compression_algorithm;
	unsigned char	reserved4;
} scsi_mode_sense_page16_t;

#endif	/* PARAGON860 */	/* Tape Compression Support */

/* read capacity data */
typedef struct {
	unsigned char	lba1;
	unsigned char	lba2;
	unsigned char	lba3;
	unsigned char	lba4;
	unsigned char	blen1;
	unsigned char	blen2;
	unsigned char	blen3;
	unsigned char	blen4;
} scsi_rcap_data_t;

/* defect list(s) */
typedef struct {
	unsigned char	res1;
	unsigned char	res2;
	unsigned char	list_len_msb;
	unsigned char	list_len_lsb;
	struct {
	    unsigned char	blockno_msb;
	    unsigned char	blockno_sb1;
	    unsigned char	blockno_sb2;
	    unsigned char	blockno_lsb;
	} defects[1];	/* VARSIZE */
} scsi_Ldefect_data_t;

/* block limits (tape) */
typedef struct {
	unsigned char	res1;
	unsigned char	maxlen_msb;
	unsigned char	maxlen_sb;
	unsigned char	maxlen_lsb;
	unsigned char	minlen_msb;
	unsigned char	minlen_lsb;
} scsi_blimits_data_t;

/*
 * Status byte (a-la scsi1)
 */

typedef union {
    struct {
	BITFIELD_4( unsigned char,
			scsi_status_vendor_uqe1:1,
			scsi_status_code:4,
			scsi_status_vendor_uqe:2,
			scsi_status_reserved:1);
#	define SCSI_ST_GOOD		0x00		/* scsi_status_code values */
#	define SCSI_ST_CHECK_CONDITION	0x01
#	define SCSI_ST_CONDITION_MET	0x02
#	define SCSI_ST_BUSY		0x04
#	define SCSI_ST_INT_GOOD		0x08
#	define SCSI_ST_INT_MET		0x0a
#	define SCSI_ST_RES_CONFLICT	0x0c
					/* anything else is reserved */
    } st;
    unsigned char bits;
} scsi_status_byte_t;

typedef struct {
	char	messages[SCSI_CMD_LD_LEN];
}  scsi_ld_data_t;
#endif	/* _SCSI_SCSI_H_ */
