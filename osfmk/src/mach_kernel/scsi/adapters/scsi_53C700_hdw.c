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
 *  (c) Copyright 1992 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */
/*
 * MkLinux
 */

/* CMU_HIST */
/*
 * Revision 2.1.2.1  92/03/28  10:14:07  jeffreyh
 * 	Pick up changes from MK71
 * 	[92/03/20  13:30:41  jeffreyh]
 * 
 * Revision 2.2  92/02/23  22:43:42  elf
 * 	Created, from the NCR specs:
 * 	"NCR 53C700 SCSI I/O Processor Data Manual, Rev 2.5"
 * 	NCR Microelectronics Division, Colorado Spring, 2/90
 * 	[91/08/27            af]
 * 
 * Revision 2.1  91/08/28  12:03:53  af
 * Created.
 * 
 */

/* hpsgc/scsi_ncr53c700.c HISTORY */
/* $Log: scsi_53C700_hdw.c,v $
 * Revision 1.1.19.1  1996/12/19  09:36:22  bruel
 * 	use more generic byte swap function.
 * 	[96/12/19            bruel]
 *
 * Revision 1.1.18.2  1996/12/19  09:31:00  bruel
 * 	use more generic byte swap function.
 * 	[96/12/19            bruel]
 *
 * Revision 1.1.17.2  1996/09/17  16:28:47  bruel
 * 	use standalone includes only
 * 	[1996/09/17  15:55:32  bruel]
 *
 * Revision 1.1.19.2  1996/09/17  15:55:32  bruel
 * 	use standalone includes only
 *
 * Revision 1.1.17.1  1996/09/02  09:54:28  bruel
 * 	Call the controller at splbio in ncr53c700_go.
 * 	[96/09/02            bruel]
 *
 * Revision 1.1.16.2  1996/09/02  09:41:52  bruel
 * 	Call the controller at splbio in ncr53c700_go.
 * 	[96/09/02            bruel]
 *
 * Revision 1.1.7.3  1995/11/08  11:11:01  bruel
 * 	removed hard define DEBUG.
 * 	[95/11/08            bruel]
 *
 * Revision 1.1.11.2  1995/11/08  11:01:13  bruel
 * 	removed hard define DEBUG.
 * 	[95/11/08            bruel]
 *
 * Revision 1.1.7.2  1995/11/02  14:59:52  bruel
 * 	hpsgc/ files moved to hp_pa/HP700
 * 	[95/09/04            bernadat]
 * 	Replaced code with hpsgc/scsi_ncr53c700.c one
 * 	Adapted to mach scsi layered driver.
 * 	Changes for fwscsi.
 * 	[95/09/01            bernadat]
 * 	[95/10/23            bruel]
 *
 * Revision 1.1.11.2  1995/10/24  08:55:46  bruel
 * 	hpsgc/ files moved to hp_pa/HP700
 * 	[95/09/04            bernadat]
 * 	Replaced code with hpsgc/scsi_ncr53c700.c one
 * 	Adapted to mach scsi layered driver.
 * 	Changes for fwscsi.
 * 	[95/09/01            bernadat]
 * 	[95/10/23            bruel]
 *
 * Revision 1.1.10.1  1995/09/08  07:13:42  bernadat
 * 	hpsgc/ files moved to hp_pa/HP700
 * 	[95/09/04            bernadat]
 *
 * 	Replaced code with hpsgc/scsi_ncr53c700.c one
 * 	Adapted to mach scsi layered driver.
 * 	Changes for fwscsi.
 * 	[95/09/01            bernadat]
 *
 * Revision 1.1.9.2  1995/09/07  14:41:26  bernadat
 * 	hpsgc/ files moved to hp_pa/HP700
 * 	[95/09/04            bernadat]
 *
 * 	Replaced code with hpsgc/scsi_ncr53c700.c one
 * 	Adapted to mach scsi layered driver.
 * 	Changes for fwscsi.
 * 	[95/09/01            bernadat]
 *
 * Revision 1.1.15.3  1995/04/07  18:55:03  barbou
 * 	Fixed prototype for ncr53c700_minphys.
 * 	[95/03/31            barbou]
 *
 * Revision 1.1.15.2  1995/03/31  15:51:45  bruel
 * 	zalloc cannot be used in interrupts, use zget instead.
 * 	[95/03/31            bruel]
 * 
 * Revision 1.1.15.1  1995/03/15  17:46:08  bruel
 * 	hppa merge
 * 	[1995/03/15  09:44:50  bruel]
 * 
 * Revision 1.1.4.3  1995/01/11  09:42:12  bruel
 * 	Utah merge: fwscsi, new HP copyright.
 * 	[95/01/06            bruel]
 * 
 * Revision 1.1.4.2  1994/09/15  10:40:36  bruel
 * 	code cleanup.
 * 	[94/09/15            bruel]
 * 
 * Revision 1.1.4.1  1994/08/11  16:43:57  bruel
 * 	zinit change.
 * 	[94/08/11            bruel]
 * 
 * Revision 1.1.2.2  1994/03/22  16:20:55  bruel
 * 	Added prototypes.
 * 	[94/02/22            bruel]
 * 
 * Revision 1.1.2.1  1994/02/11  13:59:45  bruel
 * 	zones non-pageable.
 * 	[93/11/29            bruel]
 * 
 */

/*
 * NCR 53C700 SCSI device driver bottom layer
 * Some hp_pa dependencies remain and should be cleaned up some day
 */

#define	NEISA	0

#include "ncr.h"

#include <string.h>
#include <mach_assert.h>

#include <cputypes.h>
#include <types.h>
#include <device/device_types.h>
#include <device/buf.h>
#include <machine/asp.h>
#include <hp_pa/machparam.h>
#include <hp_pa/iomod.h>
#include <kern/spl.h>
#include <scsi/scsi_defs.h>
#include <scsi/scsi.h>
#include <device/disk_status.h>
#include <vm/vm_kern.h>
#include <scsi/adapters/scsi_53C700.h>

#include <kern/misc_protos.h>
#include <hp_pa/misc_protos.h>
#include <hp_pa/trap.h>

#if NEISA > 0
#include <hp_pa/HP700/atbus.h>
#endif


u_long	SCRIPT[] = {
	0x41010000,	0x00000140,
	0x830B0000,	0x00000098,
	0x9E020000,	0x0000FF01,
	0x06000000,	0x00000000,
	0x860B0000,	0x00000018,
	0x830A0000,	0x00000098,
	0x870A0000,	0x000000D8,
	0x9A020000,	0x0000FF02,
	0x02000000,	0x00000000,
	0x870B0000,	0x000000D8,
	0x830A0000,	0x00000098,
	0x890A0000,	0x00000070,
	0x880A0000,	0x00000078,
	0x98080000,	0x0000FF03,
	0x98080000,	0x0000FF27,
	0x98080000,	0x0000FF28,
	0x830B0000,	0x00000098,
	0x870A0000,	0x000000D8,
	0x98080000,	0x0000FF05,
	0x0B000001,	0x00000000,
	0x9F030000,	0x0000FF04,
	0x07000001,	0x00000000,
	0x98040000,	0x0000FF26,
	0x80000000,	0x00000000,
	0x60000040,	0x00000000,
	0x48000000,	0x00000000,
	0x98080000,	0x0000FF31,
	0x0F000001,	0x00000000,
	0x800C0001,	0x00000100,
	0x800C0004,	0x00000120,
	0x98080000,	0x0000FF06,
	0x80080000,	0x00000020,
	0x60000040,	0x00000000,
	0x0F000000,	0x00000001,
	0x98080000,	0x0000FF07,
	0x80080000,	0x00000020,
	0x80000000,	0x00000000,
	0x60000040,	0x00000000,
	0x48000000,	0x00000000,
	0x98080000,	0x0000FF09,
	0x50000000,	0x00000190,
	0x9F030000,	0x0000FF18,
	0x0F000001,	0x00000000,
	0x60000040,	0x00000000,
	0x990B0000,	0x0000FF19,
	0x980A0000,	0x0000FF20,
	0x9F0A0000,	0x0000FF21,
	0x9B0A0000,	0x0000FF22,
	0x9E0A0000,	0x0000FF23,
	0x98080000,	0x0000FF24,
	0x98080000,	0x0000FF25
};

#define	A_sts	0x00000000
u_long	A_sts_Used[] = {
	0x00000027
};

#define	A_cdb	0x00000000
u_long	A_cdb_Used[] = {
	0x00000011
};

#define	A_msg	0x00000000
u_long	A_msg_Used[] = {
	0x00000007
};

#define	A_msgi	0x00000000
u_long	A_msgi_Used[] = {
	0x0000002b,	0x00000037,
	0x00000043,	0x00000055
};

#define	A_id	0x00000000
u_long	A_id_Used[] = {
	0x00000000
};

#define	A_msg_size	0x00000000
u_long	A_msg_size_Used[] = {
	0x00000006
};

#define	A_msgi_size	0x00000000
u_long	A_msgi_size_Used[] = {
	0x00000042
};

#define	A_cdb_size	0x00000000
u_long	A_cdb_size_Used[] = {
	0x00000010
};

u_long	A_720_patch_Used[] = {
	0x0000002f,	0x00000049
};

#define	A_720_patch_size \
	(sizeof(A_720_patch_Used)/sizeof(A_720_patch_Used[0]))


#define MASK_SCNTL_INST	0x7C027F00	/* MOVE SCNTL2 & 0x7F TO SCNTL2 */


#define	A_err1	0x0000FF01
#define	A_err2	0x0000FF02
#define	A_err3	0x0000FF03
#define	A_err4	0x0000FF04
#define	A_err5	0x0000FF05
#define	A_err6	0x0000FF06
#define	A_err7	0x0000FF07
#define	A_err9	0x0000FF09
#define	A_err18	0x0000FF18
#define	A_err19	0x0000FF19
#define	A_err20	0x0000FF20
#define	A_err21	0x0000FF21
#define	A_err22	0x0000FF22
#define	A_err23	0x0000FF23
#define	A_err24	0x0000FF24
#define	A_err25	0x0000FF25
#define	A_err26	0x0000FF26
#define	A_err27	0x0000FF27
#define	A_err28	0x0000FF28
#define	A_err31	0x0000FF31

#define	Ent_scsi_sio		0x00000000
#define	Ent_scsi_msg_out	0x00000018
#define	Ent_scsi_cmd		0x00000040
#define	Ent_scsi_msg_in		0x000000D8
#define	Ent_scsi_status		0x00000098
#define	Ent_scsi_data_in	0x00000070
#define	Ent_scsi_data_out	0x00000078
#define	Ent_scsi_data_done	0x00000080
#define	Ent_scsi_resel		0x00000140


u_long	LABELPATCHES[] = {
	0x00000001,	0x00000003,
	0x00000009,	0x0000000b,
	0x0000000d,	0x00000013,
	0x00000015,	0x00000017,
	0x00000019,	0x00000021,
	0x00000023,	0x00000039,
	0x0000003b,	0x0000003f,
	0x00000047,	0x00000051
};

u_long	INSTRUCTIONS	= 0x00000033;
u_long	PATCHES		= 0x00000010;

extern unsigned long dcache_line_size;


#define SCSI_INITIATOR 		7	/* Target ID of SCSI initiator  */
#define SCSI_MAX_MSG_LEN        12      /* Maximum message size accepted */
#define SCSI_MAX_CMD_LEN        256     /* Maximum cmd size accepted */

/* scsi_status values */

#define NCR_SCSI_GOOD				(SCSI_ST_GOOD<<1)
#define NCR_SCSI_CHECK_CONDITION		(SCSI_ST_CHECK_CONDITION<<1)
#define NCR_SCSI_CONDITION_MET			(SCSI_ST_CONDITION_MET<<1)
#define NCR_SCSI_BUSY				(SCSI_ST_BUSY<<1)
#define NCR_SCSI_INTERMEDIATE_GOOD		(SCSI_ST_INT_GOOD<<1)
#define NCR_SCSI_INTERMEDIATE_CONDITION_MET	(SCSI_ST_INT_MET<<1)
#define NCR_SCSI_RESERVATION_CONFLICT		(SCSI_ST_RES_CONFLICT<<1)
#define NCR_SCSI_QUEUE_FULL			(SCSI_ST2_QUEUE_FULL<<1)

/* reserved values */

#define NCR_SCSI_DATA_DIRECTION_ERROR       	0xFB

#define NCR_SCSI_DMA_UNDERRUN               	0xFC
#define NCR_SCSI_DMA_OVERRUN                	0xFD
#define NCR_SCSI_DEVICE_NA                  	0xFE    /* Device not
							   available */
#define NCR_SCSI_UNDEF_STAT			0xFF    /* Undefined status */

/* Symbols for emsg bytes       */

#define SCSI_EMSG_ID            0               
#define SCSI_EMSG_LEN           1
#define SCSI_EMSG_CODE          2
#define SCSI_EMSG_XFER_PERIOD   3 
#define SCSI_EMSG_XFER_WIDTH	3
#define SCSI_EMSG_REQ_ACK_OFF   4

/* Scsi manager command queueing states - used for handling SCSI-2 command
   queueing */
#define SCSI_CQ_UNKNOWN         0x00    /* Attempt to use command queueing */
#define SCSI_CQ_YES             0x01    /* Use command queueing message */
#define SCSI_CQ_NO              0x02    /* Not using command queueing */
#define SCSI_CQ_MSGSENT         0x04    /* We sent a queue message */

/* 
 * Scsi manager abort/reset states
 * - used for handling abort and reset in a multitude of conditions
 */
#define SCSI_AR_NA	0x00    /* Not performing abort or reset */
#define SCSI_AR_SEL	0x01    /* Must select target to do abort or reset */
#define SCSI_AR_ATN     0x02    /* We want to send an abort or reset
				   and we raised ATN */
#define SCSI_AR_SENT    0x03    * We sent an abort or reset message */

/* Scsi manager transfer information states
   - used for establishing sync vs async data transfer */

#define SCSI_XT_UNKNOWN         0x00	/* Not known yet - must send sync data
					   transfer request message */
#define SCSI_XT_WMSGSENT        0x01    /* We sent a wide data 
					   request message */
#define SCSI_XT_SMSGSENT        0x02    /* We sent a sync data
					   request message */
#define SCSI_XT_SYNC            0x03    /* We are synchronous */
#define SCSI_XT_ASYNC           0x04    /* We are asynchrounous */

/* Scsi manager software states - */
#define SCSI_SS_DATA_OUT        0x00    /* Data out state */
#define SCSI_SS_DATA_IN         0x01    /* Data in state */
#define SCSI_SS_CMD             0x02    /* Command state */
#define SCSI_SS_STAT            0x03    /* Status state */
#define SCSI_SS_RES0            0x04    /* Dont mess */
#define SCSI_SS_RES1            0x05    /* with these */
#define SCSI_SS_MSG_OUT         0x06    /* Message out state */
#define SCSI_SS_MSG_IN          0x07    /* Message in state */
#define SCSI_SS_DIS             0x08    /* Disconnected */
#define SCSI_SS_ACC             0x09    /* Accepting message */
#define SCSI_SS_RES             0x0a    /* Reselected */
#define SCSI_SS_ASM             0x0b    /* Accepting status (command complete)
					   message */
#define SCSI_SS_BUS_FREE        0x0c    /* Bus Free */
                                                                

/* macro for defining controller flags */
#define SCSI_CONTROLLER_FLAGS	0xff000000	/* mask of flags that
						   controllers may use */

#define SCSI_CONTROLLER_FLAG(b) 					\
	(((b)<<24)&SCSI_CONTROLLER_FLAGS)


#ifdef DEBUG
#define private
int ncr53c700_debug = 0x0;
#define C700_DEBUG_ENTRY	0x0001		/* print routine entry */
#define C700_DEBUG_INIT		0x0002		/* print initialization
						   info */
#define C700_DEBUG_SDP		0x0004		/* print save data pointers
						   info */
#define C700_DEBUG_DMA		0x0008		/* print data inout info */
#define C700_DEBUG_UDC		0x0010		/* print udc info */
#define C700_DEBUG_INTS		0x0020		/* print interrupt status
						   regs when read */
#define C700_DEBUG_CSR		0x0040		/* print CSR at start_ctlr
						   time */
#else
#define private static
#endif

#define C700_MAX_OFFSET         8		/* maximum sync data offset */
#define C700_MIN_TP_CLOCKS      4       	/* minimum sync data transfer
						   period clocks */
#define C700_MAX_TP_CLOCKS      11      	/* maximum sync data transfer
						   period clocks */

#if	hp_pa
#define CORE_OFFSET        0x100
#define ZALON_OFFSET       0x800 
#endif	/* hp_pa */

/* control */
#define EISA_CNTL_ENABLE   0x01                         /* enable card */
#define EISA_CNTL_IOCHKERR 0x02                         /* io chk error */
#define EISA_CNTL_IOCHKRST 0x04                         /* io chk reset */

/* rsrc */
#define EISA_RSRC_IRQ_NONE 0x00                         /* no irq assigned */
#define EISA_RSRC_IRQ3     0x01
#define EISA_RSRC_IRQ4     0x02
#define EISA_RSRC_IRQ5     0x03
#define EISA_RSRC_IRQ7     0x04
#define EISA_RSRC_IRQ8     0x05
#define EISA_RSRC_IRQ10    0x06
#define EISA_RSRC_IRQ11    0x07
#define EISA_RSRC_NABRT    0x20
#define EISA_RSRC_ROM_DIS  0xc0

/* determine script index given offset */
#define SCRIPT_INDEX(off)               				\
	((off)/sizeof(union instruction))

/* determine VA of script given offset */
#define SCRIPT_ADDR(cdata, off)						\
	((unsigned char *)((cdata)->script) + (off))

/* determine VA of script given pc */
#define SCRIPT_VA(cdata, pc)						\
	((unsigned char *)((cdata)->script) +				\
	 ((unsigned char *)(pc) -					\
	  (unsigned char *)((cdata)->inst_ioaddr)))

/* relocate VA in script to io address */
#define RELOC(cdata, pc)						\
	((unsigned long)((unsigned char *)(pc) -			\
			 (unsigned char *)((cdata)->script) +		\
			 (unsigned char *)((cdata)->inst_ioaddr)))

/* start the controller going */
#ifdef DEBUG
#define START_CONTROLLER(cdata, tdata) 					\
	ncr53c700_start_controller((cdata),(tdata))
#else
#define START_CONTROLLER(cdata, tdata) 					\
	((cdata)->csr->rw.dsp = BYTESWAP((tdata)->pc));
#endif





struct block_move {
        unsigned int   type:   2,       /* instruction type */
                       ind_adr:1,       /* intdirect addressing */
                       opcode: 2,       /* opcode */
                       state:  3,       /* bus state */
                       bcount: 24;      /* data count */
        unsigned long  data_addr;       /* data xfer address start */
};


struct io {
        unsigned int   type:   2,       /* instruction type */
                       opcode: 3,       /* opcode */
                       mbz:    2,       /* reserved, must be zero */
                       sel_atn:1,       /* select w/atn/ */
                       sid:    8,       /* scsi id (1<<tid) */
                       mbz4:   6,
                       set_trg:1,       /* set target role */
                       mbz3:   2,
                       ass_ack:1,       /* assert ack */
                       mbz1:   2,
                       ass_atn:1,       /* assert atn */
                       mbz0:   3;
        unsigned long  jump_addr;       /* jump address */
};

struct  xfer_control {
        unsigned int   type:   2,       /* instruction type */
                       opcode: 3,       /* opcode */
                       state:  3,       /* bus state */
                       mbz:    4,
                       tf:     1,       /* jump if true (1), false (0) */
                       cmp_dat:1,       /* compare data */
                       cmp_ph: 1,       /* compare phase */
                       wait:   1,       /* wait for valid phase */
                       mask:   8,       /* mask for compare */
                       data:   8;       /* data to be compared */
        unsigned long  jump_addr;       /* jump address */
};


/* script instruction */

union   instruction {
        struct block_move	block_move;
        struct io		io;
        struct xfer_control	xfer_control;
        unsigned long           l[2];   	/* instruction as longwords */
};

/* instruction types */
#define INST_BMOVE              0x00
#define INST_IO                 0x01
#define INST_TRANSFER           0x02

/* block_move opcodes */
#define INST_BM_MOVE            0x000		/* block move */
#define INST_BM_WMOVE           0x001           /* wait block move */

/* io opcodes */
#define INST_IO_SELECT          0x000           /* select */
#define INST_IO_WAIT_DIS        0x001           /* wait for disconnect */
#define INST_IO_WAIT_RESEL      0x002           /* wait for reselect */
#define INST_IO_SET             0x003           /* set (e.g. set atn) */
#define INST_IO_CLEAR           0x004           /* clear (e.g. clear ack) */

/* transfer_control opcodes */
#define INST_TRANSFER_JUMP      0x000           /* jump */
#define INST_TRANSFER_CALL      0x001           /* call */ 
#define INST_TRANSFER_RETURN    0x002           /* return from call */
#define INST_TRANSFER_INT       0x003           /* interrupt */


/* Pseudo-DMA controller control structure */
struct  pdma {
        unsigned long	pc;		/* current pc    */
        unsigned long   addr;           /* current instruction io address */
        unsigned long   cnt;            /* current instruction transfer
					   length */
        unsigned        valid:1;        /* true when dma engine saved */
};


/* The various generic formats of comand descriptor blocks */
union scsi_cdb {
	struct {
		unsigned	opcode:8;
		unsigned	lun:3;
		unsigned	block_addr:21;
		unsigned	length:8;
		unsigned	control:8;
	} c6;
	struct {
		unsigned	opcode:8;
		unsigned	lun:3;
		unsigned	:4;
		unsigned	reladr:1;
		unsigned	block_addr_hi:16;
		unsigned	block_addr_lo:16;
		unsigned	:8;
		unsigned	length_hi:8;
		unsigned	length_lo:8;
		unsigned	control:8;
	} c10;
	struct {
		unsigned	opcode:8;
		unsigned	lun:3;
		unsigned	:4;
		unsigned	reladr:1;
		unsigned	block_addr_hi:16;
		unsigned	block_addr_lo:16;
		unsigned	length_hi:16;
		unsigned	length_lo:16;
		unsigned	:8;
		unsigned	control:8;
	} c12;
	struct {
		unsigned	group:3;
		unsigned	command:5;
	} code;
	unsigned char	command[12];
};

struct xfer_data {
    unsigned long	pc;			/* current pc    */
    vm_offset_t         ioaddr;                 /* ioaddr (for iomap case) */
    unsigned long       npages;                 /* number of iomap pages */
    unsigned long       cnt;                    /* number of instruction */
    union instruction   inst[C700_MAX_DMA_INST];/* instructions */
};

struct  target_dma_data {
    union instruction	inst[C700_MAX_DMA_INST];
    union scsi_cdb	cdb;                    /* CDB bytes */
    unsigned char       msg[SCSI_MAX_MSG_LEN];  /* Message to send when
						   message out phase comes */
    unsigned char       msgi[SCSI_MAX_MSG_LEN]; /* Message received from
						   target */
    unsigned char       sts[SCSI_MAX_MSG_LEN];  /* Scsi status message */
    unsigned char       msg_size;               /* Size of message to send */
    unsigned char       msg_index;              /* Index for receiving
						   multiple byte messages */
    unsigned char       msgi_size;              /* Size of message received */
    unsigned char       cdb_size;               /* Size of CDB to send */
};


struct  target_data {
        int			id;		/* target id */
        struct target_dma_data  *dma_data;	/* Per-target DMA area */
	struct xfer_data	xfer_data;	
        unsigned long           pc;             /* Controller PC
						   (Big Endian form!) */
        unsigned char           stp;            /* Sync transfer period */
        unsigned char           sof;            /* Sync offset          */
	unsigned char		sxfer;		/* Proto sxfer reg value */
	unsigned char		sbcl;		/* Proto sbcl/scntl3
						   reg value */
	unsigned char		width;		/* Transfer
						   width: 0=8, 1=16 */
        unsigned char           tstate;         /* Target state */
        unsigned char           cq_state;       /* State of command
						   queueing */
        unsigned char           xt_state;       /* State of transfer type
						   information   */
        unsigned char           ar_state;       /* State of abort or reset
						   operations   */
        unsigned char           ar_msg;         /* Holds abort or reset
						   message */
        unsigned char           lmsg[SCSI_MAX_MSG_LEN];
						/* Last message received
						   from target    */
        unsigned char           lmsg_size;      /* Size of last message
						   received from target    */
        unsigned int            active:1;       /* True when target is
						   active       */
};


#ifdef DEBUG
/* interrupt trace data */
struct itrace {
        struct  target_data *tdata;		/* target info */
        unsigned long  icount;                  /* interrupt counter */
        unsigned long  pc_entry, pc_exit;       /* on entry, exit pc */
        unsigned short code, isaved;            /* interrupt code, interrupts
						   saved (loop counter
						   for handler) */
        unsigned short tid_entry, tid_exit;     /* target id on entry/exit */
        unsigned char  istat;                   /* controller registers */
        unsigned char  dstat;
        unsigned char  sstat0;
	unsigned char  restart;			/* indicated target
						   reselected and restarted */
};

#define ITRACE_NENTRIES         64
#endif

struct  controller_data {
        struct target_data      *target_data[MAX_SCSI_TARGETS];
						/* Per target info */

        /* controller status/info */
	
        struct pdma             dma;   		/* Current target DMA 
						   engine */                 
	union csr               *csr;     	/* controller csr */
        unsigned int            maxoff;         /* Maximum synchronous offset
						   supported by controller */
        unsigned int            maxperiod;      /* Maximum time between 
						   leading edges of REQ or 
						   ACK pulses (ns) */
        unsigned int            minperiod;      /* Minimum time between 
						   leading edges of REQ or 
						   ACK pulses (ns) */
        unsigned int            set_atn:1;      /* True when want to set ATN 
						   (want to send a message) */
        unsigned int            active:1;       /* True when controller 
						   is active  */
        unsigned int            cmess:1;        /* True when command complete 
						   message received */
        unsigned int            tcp;            /* Input clock period (ns) */
        unsigned char           ctid;           /* Current Target ID */
        unsigned char           ctype;          /* Controller type */
	unsigned char		bigend;		/* Controller is big endian */

        /* script management */

        vm_offset_t             inst_ioaddr;    /* Instruction DMA area for 
						   script & target dma data 
						   areas */
        union  instruction      *script;        /* Controllers copy of script
						   (allocated) */
        vm_offset_t             script_phys;    /* Physical address of script
						   (allocated) */

        /* chip "features" */

        unsigned long           sel_resel;      /* resel after WAIT DISCONNECT
						   before INT */
        unsigned long           sel_int;	/* resel after WAIT DISCONNECT
						   during INT */
        unsigned long           restart_resel;  /* resel before INT
						   instruction in command
						   state */
        unsigned long           ill_resel;      /* resel during WAIT
						   DISCONNECT instruction */

        /* tracing */

        unsigned long           it_icount;      /* number of interrupts */
        unsigned long           it_ihandled;    /* number of interrupts 
						   handled */
        unsigned long           it_ispur;       /* number of spurious 
						   interrupts */
#if	DEBUG
        unsigned long           it_index;       /* current interrupt trace 
						   entry */
        unsigned long           it_nentries;    /* actual number of entries */
        struct itrace           *it;            /* trace buffer */
#endif	/* DEBUG */
};

#if	DEBUG
static void	display_status(
			       struct controller_data  *cdata);
#endif


/* H/W dependant controller flags */
#define CMD_ACTIVE	SCSI_CONTROLLER_FLAG(0x01)	/* command is active */
#define CMD_IOVA	SCSI_CONTROLLER_FLAG(0x02)	/* command has
							   allocated iomap
							   space */

unsigned int   r;                       /* discard for reads */
extern int     isgecko;

/*
 * Per target scsi command
 */

struct scsi_cmd {
	unsigned long		scsi_status;
	vm_offset_t		data_addr;
	unsigned long		data_len;
	target_info_t		*tgt;		/* H/W independent scsi info */
	boolean_t		queued;		/* cmd is queued */
};

#define cdb_len	tgt->transient_state.cmd_count

struct scsi_controller {
	struct watchdog_t	wd;		/* Must be first element */
        int             	bustype;        /* bustype */
	union csr 		*bus_addr;	/* address of csr registers */
	struct controller_data 	*info;		/* controller information */
	scsi_softc_t		*sc;		/* HBA-indep info */
	struct scsi_cmd		*scsi_cmd[MAX_SCSI_TARGETS];
						/* per target command */
};

int ncr53c700_controller_init = 0;

struct scsi_controller ncr53c700_controller[NNCR] = {
  { BUSTYPE_NOSUCHBUS,
      (vm_offset_t)0,
      (vm_offset_t)0,
  },
};

/* forward */

int 		ncr53c700_attach(
				 hp_ctlrinfo_t *);

int 		ncr53c700_setsync(
			  struct scsi_controller *,
			  struct target_data *,
			  unsigned int,
			  unsigned int);
extern void	scsi_add(
			 int bus,
			 struct scsi_controller *scp);

boolean_t	ncr53c700_probe_target(
				  target_info_t		*tgt,
				  io_req_t		ior);

void		ncr53c700_reset_scsibus(
			   	struct scsi_controller *scp);

void		ncr53c700_go(
			target_info_t		*tgt,
			unsigned int		cmd_count,
			unsigned int		in_count,
			boolean_t		cmd_only);

void		ncr53c700_probe_devices(
				   struct scsi_controller *scp);

void 		ncr53c700_mkinst(
				 struct scsi_controller*,
				 struct scsi_cmd *);

void 		ncr53c700_start(
				struct scsi_controller *);

void 		ncr53c700_sel_ar(
				 struct scsi_controller*,
				 struct target_data *);

void 		ncr53c700_sel_cmd(
				  struct  scsi_controller *,
				  struct  target_data *);

void 		ncr53c700_patch_script(
				       struct scsi_controller *,
				       struct  target_data *,
				       int tid);

void 		ncr53c700_start_controller(
					   struct controller_data*,
					   struct target_data *);

int 		ncr53c700_process_int(
				      struct scsi_controller*,
				      struct target_data*,
				      struct target_dma_data *,
				      struct scsi_cmd *,
				      unsigned long); 

void 		ncr53c700_process_message(
					  struct scsi_controller*,
					  struct target_data*,
					  struct scsi_cmd*,
					  unsigned char*);

void 		ncr53c700_save_dma(
				   struct  scsi_controller *,
				   struct  target_data *,
				   struct scsi_cmd *, unsigned char);

void 		ncr53c700_next_phase(
				     struct scsi_controller *,
				     struct target_data *,
				     struct scsi_cmd *,
				     unsigned char);

void 		ncr53c700_iodone(
				 struct scsi_controller*,
				 struct target_data *);

void 		ncr53c700_save_data_pointers(
					     struct scsi_controller *,
					     struct scsi_cmd *);

void 		ncr53c700_transfer_pad(
				       struct scsi_controller*,
				       struct target_data*,
				       struct scsi_cmd *);

void 		ncr53c700_data_inout(
				     struct scsi_controller*,
				     struct target_data *,
				     struct scsi_cmd*);

void		ncr53c700_intr(
			       struct scsi_controller*);

boolean_t	ncr53c700_target_alloc(
				       struct scsi_controller 	*scp,
				       target_info_t		*tgt);

void		ncr53c700_target_dealloc(
				       struct scsi_controller 	*scp,
				       target_info_t		*tgt);

vm_offset_t	kalloc_aligned(
			       vm_size_t	size,
			       vm_size_t	alignment);

void		kfree_aligned(
			      vm_offset_t	offset,
			      vm_size_t		size,
			      vm_size_t		alignment);

#if hp_pa
void		zalon_init(
			   struct controller_data  *cdata);
#endif	/* hp_pa */

#if	hp_pa
#define	cache_round(size) \
        ((size + (dcache_line_size-1)) & ~(dcache_line_size-1))
#else
#define	cache_round(size) 
	(size)
#endif	/* hp_pa */

#ifdef	DEBUG

/*
 *      ncr53c700_start_controller - Start controller at a given pc.
 */
void
ncr53c700_start_controller(
			   struct  controller_data *cdata,
			   struct  target_data     *tdata)
{
        unsigned long bpc;

        bpc = BYTESWAP(tdata->pc);

	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_start_controller(0x%x, 0x%x), bpc = 0x%x\n",
		       cdata, tdata, bpc);

        if (tdata->xt_state == SCSI_XT_ASYNC &&
	    (READBYTEREG(cdata->csr->rw.sxfer) & 0xf)) {
                panic("ncr53c700_start_controller: async device with non-zero sync offset");
        }
	if (ncr53c700_debug & C700_DEBUG_CSR)
		display_status(cdata);

        cdata->csr->rw.dsp = bpc;
}

#endif	/* DEBUG */

/*
 *      ncr53c700_mkinst - Build dma instructions for transfers.
 */
void
ncr53c700_mkinst(
		 struct  scsi_controller *scp,
		 struct  scsi_cmd        *cmd)
{
        struct  controller_data *cdata;
        struct  target_data     *tdata;
        struct  target_dma_data *tdma;
        struct  xfer_data       *xfer;
        union   instruction     *ip, inst;
	int			tid;
	vm_offset_t		ioaddr;
	vm_offset_t		vaddr;
	vm_map_t		map;
	vm_size_t		psize;
	vm_offset_t		page;
        unsigned long           bcount, data_addr, len;
	unsigned long           ii, npages, offset;

        /* template for building instructions */
        static struct block_move dma_temp = {
                INST_BMOVE,             /* type */
                FALSE,                  /* addr ind */
                INST_BM_WMOVE,          /* opcode */
                SCSI_PHASE_DATAI,        /* state (modified) */
                0,                      /* bcount (modified) */
                0                       /* data_addr (modified) */
        };

        static struct xfer_control jump_temp = {
                INST_TRANSFER,          /* type */
                INST_TRANSFER_JUMP,     /* opcode */
                SCSI_PHASE_DATAI,        /* state (good enough) */
                0,                      /* mbz */
                TRUE,                   /* tf, jump if true */
                FALSE,                  /* cmp_dat */
                FALSE,                  /* cmp_ph */
		FALSE,			/* wait */
                0,                      /* mask */
                0,                      /* data */
                0                       /* jump_addr (modified) */
        };

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_mkinst(0x%x, 0x%x) target %d\n",
		       scp, cmd, cmd->tgt->target_id);
#endif

	if (cmd->data_len > C700_MAXPHYS) {
                panic("ncr53c700_mkinst: data length too large");
        }
	else if (cmd->data_len != 0) {
                tid = cmd->tgt->target_id;
                cdata = scp->info;
                tdata = cdata->target_data[tid];
                tdma = tdata->dma_data;
                xfer = &tdata->xfer_data;

#if NEISA > 0
		if (scp->bustype == BUSTYPE_EISA)
			psize = ATBUS_IOMAP_PAGE_SIZE;
		else
#endif
		psize = NBPG;

		/* find out how much iospace we need */
		offset = cmd->data_addr & (psize-1);
		npages = (offset + cmd->data_len + psize - 1) / psize;
#if NEISA > 0
		if (scp->bustype == BUSTYPE_EISA) {
			ioaddr = atbus_iomap_allocate(ATBUS_IOMAP_MASTER,
						      ATBUS_IOMAP_ANYWHERE,
						      npages);
			if (ioaddr == ATBUS_IOMAP_NOSPACE)
				panic("ncr53c700_mkinst: no more iomap space");
			cmd->flags |= CMD_IOVA;
		} else
#endif
		{
			ioaddr = 0;
		}

		xfer->ioaddr = ioaddr;
		xfer->npages = npages;

#ifdef DEBUG
	        if (ncr53c700_debug & C700_DEBUG_DMA)
			printf("ncr53c700_data_mkinst: data_addr = 0x%x, data_len = 0x%x, offset = 0x%x\n",
			       cmd->data_addr,
			       cmd->data_len, offset);
#endif

		/* map our pages into the iospace */
		map = kernel_map;

		xfer->pc = RELOC(cdata, tdma->inst);

		inst.block_move = dma_temp;
		if (cmd->tgt->transient_state.out_count)
			inst.block_move.state = SCSI_PHASE_DATAO;
		else
			inst.block_move.state = SCSI_PHASE_DATAI;

		/* build move instructions */
		ip = xfer->inst;
		len = cmd->data_len;

		for (ii = 0, vaddr = cmd->data_addr;
		     ii < npages;
		     ii++, vaddr += psize) {

			page = pmap_extract(vm_map_pmap(map),
					    vaddr & ~(psize-1));
                        if (page == 0)
                                panic("ncr53c700_mkinst: page not mapped");

#if NEISA > 0
			if (cmd->flags & CMD_IOVA) {
				/* map the page and use io address
				   as controller address */
	                        atbus_iomap_map(ioaddr, &page, 1);
			} else
#endif
			{
				/* get aligned controller address */
				ioaddr = page;
			}
#ifdef DEBUG
	                if (ncr53c700_debug & C700_DEBUG_DMA)
			        printf("ncr53c700_data_mkinst: vaddr = 0x%x, page = 0x%x\n", vaddr, page);
#endif
			*ip = inst;			/* copy template */
			bcount = MIN(psize - offset, len);
			data_addr = ioaddr + offset;
                        offset = 0;			/* offset good first
							   time through loop */
#ifdef DEBUG
	                if (ncr53c700_debug & C700_DEBUG_DMA)
			        printf("ncr53c700_data_mkinst: data_addr = 0x%x, ioaddr = 0x%x\n", data_addr, ioaddr);
#endif
#if NEISA > 0
			if (cmd->flags & CMD_IOVA)
	                        ioaddr += psize;
#endif
			len -= bcount;
			ip->block_move.bcount = bcount;
			ip->block_move.data_addr = data_addr;
			SWAPINST(ip);
			ip++;
		}

		ip->xfer_control = jump_temp;
		if (cmd->tgt->transient_state.out_count)
			ip->xfer_control.state = SCSI_PHASE_DATAO;
		else
			ip->xfer_control.state = SCSI_PHASE_DATAI;

		ip->xfer_control.jump_addr =
		  	RELOC(cdata,
			      SCRIPT_ADDR(cdata, Ent_scsi_data_done));
                SWAPINST(ip);
		xfer->cnt = ip - xfer->inst + 1;
	}
}


/*
 *      ncr53c700_patch_script - Patch scsi script.
 */
void
ncr53c700_patch_script(
		       struct  scsi_controller *scp,
		       struct  target_data     *tdata,
		       int                     tid)
{

/* patch a script location:
 *      script value = original script value (relative offset)
 *		       + relocated data area address
 */

#define PATCH(loc, sloc, ioloc)            (loc) = BYTESWAP((sloc) + (ioloc));

        struct  controller_data *cdata;
        struct  target_dma_data *tdma;
        union   instruction     *ip, inst;
        unsigned long                  *s;
        int                     x;

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_patch_script(0x%x, 0x%x, 0x%x)\n",
		       scp, tdata, tid);
#endif
        cdata = scp->info;
        tdma = tdata->dma_data;

        /*
         * Instructions are byteswapped on a longword basis.
         */

        s = (unsigned long *)cdata->script;
        for (x = 0; x < sizeof(A_sts_Used)/sizeof(A_sts_Used[0]); ++x) {
                PATCH(s[A_sts_Used[x]],
		      SCRIPT[A_sts_Used[x]],
		      RELOC(cdata, tdma->sts));
        }
        for (x = 0; x < sizeof(A_cdb_Used)/sizeof(A_cdb_Used[0]); ++x) {
                PATCH(s[A_cdb_Used[x]],
		      SCRIPT[A_cdb_Used[x]],
		      RELOC(cdata, tdma->cdb.command));
        }
        for (x = 0; x < sizeof(A_msg_Used)/sizeof(A_msg_Used[0]); ++x) {
                PATCH(s[A_msg_Used[x]],
		      SCRIPT[A_msg_Used[x]],
		      RELOC(cdata, tdma->msg));
        }
        for (x = 0; x < sizeof(A_msgi_Used)/sizeof(A_msgi_Used[0]); ++x) {
                PATCH(s[A_msgi_Used[x]],
		      SCRIPT[A_msgi_Used[x]],
		      RELOC(cdata, tdma->msgi));
        }

        /* id */
        for (x = 0; x < sizeof(A_id_Used)/sizeof(A_id_Used[0]); ++x) {
                ip = (union instruction *)&s[A_id_Used[x]];
                inst.l[0] = BYTESWAP(ip->l[0]);
		if (cdata->ctype == CTYPE_720)
			inst.io.sid = tid;
		else
			inst.io.sid = 1 << tid;
                ip->l[0] = BYTESWAP(inst.l[0]);
        }

        /* msg_size */
        for (x = 0;
	     x < sizeof(A_msg_size_Used)/sizeof(A_msg_size_Used[0]);
	     ++x) {
                ip = (union instruction *)&s[A_msg_size_Used[x]];
                inst.l[0] = BYTESWAP(ip->l[0]);
                inst.block_move.bcount = tdma->msg_size;
                ip->l[0] = BYTESWAP(inst.l[0]);
        }
        /* msgi_size */
        for (x = 0;
	     x < sizeof(A_msgi_size_Used)/sizeof(A_msgi_size_Used[0]);
	     ++x) {
                ip = (union instruction *)&s[A_msgi_size_Used[x]];
                inst.l[0] = BYTESWAP(ip->l[0]);
                inst.block_move.bcount = sizeof(tdma->msgi)-1;
                ip->l[0] = BYTESWAP(inst.l[0]);
        }

        /* cdb_size */
        for (x = 0;
	     x < sizeof(A_cdb_size_Used)/sizeof(A_cdb_size_Used[0]);
	     ++x) {
                ip = (union instruction *)&s[A_cdb_size_Used[x]];

                inst.l[0] = BYTESWAP(ip->l[0]);
                inst.block_move.bcount = tdma->cdb_size;
                ip->l[0] = BYTESWAP(inst.l[0]);
        }

        pmap_flush_range(vm_map_pmap(kernel_map),
			 (vm_offset_t)cdata->script, sizeof(SCRIPT));
        pmap_flush_range(vm_map_pmap(kernel_map),
			 (vm_offset_t)tdma, sizeof(*tdma));
}

/*
 *      ncr53c700_scsi_sel_ar - Select target and send abort or reset message.
 *
 *      This routine is passed pointers to a controller structure and a target
 *	structure.  It sets up to send an abort or reset message to the target
 *	and selects the target.
 */
void
ncr53c700_sel_ar(
		 struct  scsi_controller *scp,
		 struct  target_data     *tdata)
{
        struct  controller_data *cdata;
        struct  target_dma_data *tdma;
        union   csr             *csr;
        struct  scsi_cmd        *cmd;
        int     tid;

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_sel_ar(0x%x, 0x%x)\n", scp, tdata);
#endif
        cdata = scp->info;
        tid = tdata->id;
        tdma = tdata->dma_data;

        /* Get the first command off the list. 
	   If none there then forget it. */
        cmd = scp->scsi_cmd[tid];
        if (!cmd->queued) {
                printf("ncr53c700_sel_ar: no commands on list\n");
                return;
        }
        cmd->tgt->flags |= CMD_ACTIVE;

        csr = scp->bus_addr;

        /* Put the ID message in the message buffer */
        tdma->msg[0] = (SCSI_IDENTIFY|SCSI_IFY_ENABLE_DISCONNECT |
			cmd->tgt->lun);
        tdma->msg_size = 1;

        /* Load transfer period & sync offset for this target */
	SET_SXFER(cdata, csr, tdata);

        /* make sure controller sees messages & command */
        pmap_flush_range(vm_map_pmap(kernel_map),
			 (vm_offset_t)tdma, sizeof(*tdma));

        /* Mark this controller as active. 
	   Mark this target as active and current.
	   Place software state to command state. */
        cdata->active = TRUE;
        cdata->ctid = tid;
        cdata->cmess = FALSE;
        cdata->dma.valid = FALSE;
        tdata->active = TRUE;
        tdata->tstate = SCSI_PHASE_CMD;
        tdata->lmsg[0] = SCSI_NOP;
        tdata->lmsg_size = 1;
        tdata->pc = RELOC(cdata, SCRIPT_ADDR(cdata, Ent_scsi_sio));
        ncr53c700_patch_script(scp, tdata, tid);
#ifdef DEBUG
        if ((tdata->xt_state == SCSI_XT_ASYNC) && tdata->sof) {
                panic("ncr53c700_sel_ar: async device with non 0 transfer period");
        }
#endif
        START_CONTROLLER(cdata, tdata);
}

#ifdef DEBUG
struct {
	long nodata;
	long writes;
	long alignedreads;
	long unalignedreads;
} scsicstats;
#endif

/*
 *      ncr53c700_sel_cmd - Begin SCSI command.
 *
 *      This routine is passed pointers to a controller structure and
 *	a target structure.  It starts the first request on the list by
 *	setting up a command that will select and send a command to
 *      the appropiate target.
 */
void
ncr53c700_sel_cmd(
		  struct  scsi_controller *scp,
		  struct  target_data     *tdata)
{
        struct  controller_data *cdata;
        struct  target_dma_data *tdma;
        union   csr             *csr;
        struct  scsi_cmd        *cmd;
        int     t, x, tid;

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_sel_cmd(0x%x, 0x%x)\n", scp, tdata);
#endif
        cdata = scp->info;
        tid = tdata->id;
        tdma = tdata->dma_data;

        /* Get the first command off the list. 
	   If none there then forget it. */
        cmd = scp->scsi_cmd[tid];
        if (!cmd->queued) {
                printf("ncr53c700_sel_cmd: no commands on list\n");
                return;
        }

        cmd->tgt->flags |= CMD_ACTIVE;
        cmd->scsi_status = NCR_SCSI_UNDEF_STAT;

        csr = scp->bus_addr;

        /* ID message */
        x = 0;
        tdma->msg[x++] = (SCSI_IDENTIFY|SCSI_IFY_ENABLE_DISCONNECT |
			  cmd->tgt->lun);

        /* Now, let's see if we've established transfer type yet        */
	switch (tdata->xt_state) {
	case SCSI_XT_UNKNOWN:
                /*
		 * For 720 negotiate wide transfer first
		 */
		if (cdata->ctype == CTYPE_720) {
			tdma->msg[x++] = SCSI_EXTENDED_MESSAGE;
			tdma->msg[x++] = 2;
			tdma->msg[x++] = SCSI_WIDE_XFER_REQUEST;
			tdma->msg[x++] = 1;
			tdata->xt_state = SCSI_XT_WMSGSENT;
#ifdef DEBUG
			if (ncr53c700_debug & C700_DEBUG_INIT)
				printf("sel_cmd: WDTR XMSG: tid=%d\n", tid);
#endif
			break;
		}
		/* else fall into ... */
	case SCSI_XT_WMSGSENT:
		/*
		 * Send sync data transfer request
		 *
		 * More funny arithmetic here for rounding - see comment in
		 * ncr53c700_init.
		 */
		t = ((cdata->minperiod * 10) / 4);
		t = (t + 5) / 10;
		tdma->msg[x++] = SCSI_EXTENDED_MESSAGE;
		tdma->msg[x++] = 3;
		tdma->msg[x++] = SCSI_SYNC_XFER_REQUEST;
		tdma->msg[x++] = t;
		tdma->msg[x++] = cdata->maxoff;
		tdata->xt_state = SCSI_XT_SMSGSENT;
#ifdef DEBUG
		if (ncr53c700_debug & C700_DEBUG_INIT)
			printf("sel_cmd: SDTR XMSG: tid=%d, minper=%d, maxoff=%d, state=0x%x, stp=%d, sof=%d\n",
			       tid, t, cdata->maxoff,
			       tdata->xt_state, tdata->stp, tdata->sof);
#endif
	default:
		break;
	}
        tdma->msg_size = x;

#ifdef DEBUG
        if (ncr53c700_debug & C700_DEBUG_ENTRY)
                for (x = 0; x < tdma->msg_size; ++x)
                        printf("ncr53c700_sel_cmd: msg[%d] = 0x%x\n",
			       x, tdma->msg[x]);
#endif
        /* Now let's stuff the cmd buffer with the cdb */
        for (x = 0; x < cmd->cdb_len; ++x)
                tdma->cdb.command[x] = cmd->tgt->cmd_ptr[x];

        tdma->cdb_size = cmd->cdb_len;
        tdma->msgi[0] = SCSI_NOP;
        tdma->msgi_size = 1;

        /* Load transfer period & sync offset for this target */
	SET_SXFER(cdata, csr, tdata);

#ifdef DEBUG
        if (ncr53c700_debug & C700_DEBUG_ENTRY)
                for (x = 0; x < tdma->cdb_size; ++x)
                        printf("ncr53c700_sel_cmd: cmd[%d] = 0x%x\n",
			       x, tdma->cdb.command[x]);
#endif

        /* make sure controller sees messages & command */
        pmap_flush_range(vm_map_pmap(kernel_map),
			 (vm_offset_t)tdma, sizeof(*tdma));

	/*
	 * For a data transfer we need to flush/purge the affected 
	 * cache lines.
	 */
	if (cmd->data_addr) {
		vm_map_t map = kernel_map;

		/*
		 * Write operation, must flush so SCSI see correct data.
		 */
		if (cmd->tgt->transient_state.out_count) {
			pmap_flush_range(vm_map_pmap(map),
					 cmd->data_addr,
					 cmd->data_len);
#ifdef DEBUG
			scsicstats.writes++;
#endif
		}
		/*
		 * Read operation with start or length not cache line
		 * aligned.  We must flush (not purge) to ensure unrelated
		 * data in cache lines are committed to memory.  Note that
		 * we could just flush the one or two unaligned lines and
		 * purge the rest but this case is rare enough that we
		 * don't bother.
		 */
		else if ((cmd->data_addr & (dcache_line_size-1)) ||
			 (cmd->data_len & (dcache_line_size-1))) {
			pmap_flush_range(vm_map_pmap(map),
					 cmd->data_addr,
					 cmd->data_len);
#ifdef DEBUG
			scsicstats.unalignedreads++;
#endif
		}
		/*
		 * Cache aligned read operation.  We purge before the
		 * operation not after.  If we did not purge now, dirty
		 * lines might get displaced from the cache while the
		 * operation is in effect, overwriting incoming data.
		 *
		 * XXX one can make a security argument for flushing rather
		 * than purging.  If the range included some dirty cache
		 * lines and the read aborts for some reason, we have
		 * exposed the previous contents of those lines to the
		 * caller.  Change it to a flush (and collapse all these
		 * cases) if you care.
		 */
		else {
			pmap_purge_range(vm_map_pmap(map),
					 cmd->data_addr,
					 cmd->data_len);
#ifdef DEBUG
			scsicstats.alignedreads++;
#endif
		}
	}
#ifdef DEBUG
	else
		scsicstats.nodata++;
#endif

        /* Mark this controller as active. 
	 * Mark this target as active and current.
         * Place software state to command state. */
        cdata->active = TRUE;
        cdata->ctid = tid;
        cdata->cmess = FALSE;
        cdata->dma.valid = FALSE;
        tdata->active = TRUE;
        tdata->ar_state = SCSI_AR_NA;
        tdata->tstate = SCSI_PHASE_CMD;
        tdata->lmsg[0] = SCSI_NOP;
        tdata->lmsg_size = 1;
        tdata->pc = RELOC(cdata, SCRIPT_ADDR(cdata, Ent_scsi_sio));
        ncr53c700_patch_script(scp, tdata, tid);
        START_CONTROLLER(cdata, tdata);
}

/*
 * ncr53c700_start - Start a SCSI command if one available
 *
 * This routine is called when the controller is inactive.  It searches data
 * structures for an idle target with waiting cdbs.  If found, it starts a
 * request.  This routine is passed a pointer to the per-controller structure
 * of the controller that is inactive.
 *
 * This routine also has an important role in aborting or reseting devices.
 * Targets that are active but need to be reselected to send abort or reset
 *  messages have higher priority than idle targets.
 */
void
ncr53c700_start(struct  scsi_controller *scp)
{
        struct  controller_data *cdata;
        struct  target_data     *tdata, *tp;
        int     tid;

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_start(0x%x)\n", scp);
#endif
        cdata = scp->info;
        if (cdata->active)      /* Already busy */
                return;

        /* Now loop down all targets to find one that is inactive and
	 * has outstanding commands. Give higher priority to targets
	 * that have to be selected and aborted or reset
	 */
        tp = (struct target_data *) 0;
        for (tid = 0; tid < MAX_SCSI_TARGETS-1; ++tid) {
	  	if (! (tdata = cdata->target_data[tid]))
			continue;
	  	assert(scp->scsi_cmd[tid]);
		if (tdata->active == FALSE && scp->scsi_cmd[tid]->queued) {
                        tp = tdata;
                } else if (tdata->active && tdata->ar_state ==
			   SCSI_AR_SEL) {
                        tp = tdata;
                        break;
                }
        }


        /* If tp still NULL, then we didn't find one.  If we did find one,
	   start processing   */
        if (tp != (struct target_data *) 0) {
                if (tp->active)
                        /* This target must be selected to send abort
			   or reset message  */
                        ncr53c700_sel_ar(scp, tp);
                else
                        ncr53c700_sel_cmd(scp, tp);
        }
}

/*
 *      ncr53c700_save_data_pointers - Save state of transfer.
 *
 *      This routine is called when the transfer state of the command must
 *	be saved.  The count of the number of bytes transferred is updated,
 *	and the dma controller is turned off.
 */
void
ncr53c700_save_data_pointers(
			     struct  scsi_controller *scp,
			     struct  scsi_cmd        *cmd)
{
        struct  controller_data *cdata;
        struct  target_dma_data *tdma;
        struct  xfer_data       *xfer;
        union   instruction     *ip, inst;

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_save_data_pointers(0x%x, 0x%x)\n", scp, cmd);
#endif
        cdata = scp->info;
        tdma = cdata->target_data[cdata->ctid]->dma_data;

        /* If a dma underrun or direction error has happened, just return. */
        if ((cmd->scsi_status == NCR_SCSI_DMA_UNDERRUN) ||
	    (cmd->scsi_status == NCR_SCSI_DATA_DIRECTION_ERROR))
                return;

        /* We only do this for commands that transfer data */
        if (cmd->data_len != 0 && cdata->dma.valid) {
                xfer = &cdata->target_data[cdata->ctid]->xfer_data;
                ip = xfer->inst +
		  ((unsigned long)cdata->dma.pc -
		   RELOC(cdata, tdma->inst))/sizeof(union instruction);

                /* Patch instruction */
                inst = *ip;
                SWAPINST(&inst);
#ifdef DEBUG
                if (ncr53c700_debug & C700_DEBUG_SDP)
                        printf("ncr53c700_save_data_pointers: inst = 0x%x, old addr 0x%0x, old byte count 0x%0x\n",
                               ip, inst.block_move.data_addr,
			       inst.block_move.bcount);
#endif

                inst.block_move.bcount = cdata->dma.cnt;
                inst.block_move.data_addr = cdata->dma.addr;
#ifdef DEBUG
                if (ncr53c700_debug & C700_DEBUG_SDP)
                        printf("ncr53c700_save_data_pointers: new addr 0x%0x, new byte count 0x%0x\n",
                               inst.block_move.data_addr,
			       inst.block_move.bcount);
#endif
                SWAPINST(&inst);
                *ip = inst;

                /* save away restart pc */
                xfer->pc = cdata->dma.pc;
        }
}

/*
 *      ncr53c700_save_dma - Save state of DMA engine.
 *
 */
void
ncr53c700_save_dma(
		   struct  scsi_controller *scp,
		   struct  target_data     *tdata,
		   struct  scsi_cmd        *cmd,
		   unsigned char           phase)
{
        struct  controller_data *cdata;
        union   csr             *csr;
	unsigned char           dfifo, dfifo_mask, sstat1;
        unsigned long           fifo_bytes, dbc, dbc1, dnad;
        int                     tid;

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_save_dma(0x%x, 0x%x, 0x%x, 0x%x)\n",
		       scp, tdata, cmd, phase);
#endif
        cdata = scp->info;
        csr = scp->bus_addr;

        /* If a dma underrun or direction error has happened,
	   just clear the FIFOs */
        if ((cmd->scsi_status == NCR_SCSI_DMA_UNDERRUN) ||
	    (cmd->scsi_status == NCR_SCSI_DATA_DIRECTION_ERROR)) {
		C700_CLF(cdata, csr);
                return;
        }

	if (cdata->ctype == CTYPE_720)
		sstat1 = READBYTEREG(csr->rw.sstat0);
	else
		sstat1 = READBYTEREG(csr->rw.sstat1);
	dfifo = READBYTEREG(csr->rw.dfifo);
        /* since this is a 24 bit register byte-swapping gets screwed up */
        dbc = BYTESWAP(csr->rw.d.dbc) & 0xffffff;
        dnad = BYTESWAP(csr->rw.dnad);

        /* check for any data in the fifo */
        switch (phase) {
        case SCSI_PHASE_DATAI:
                fifo_bytes = 0;
                break;        /* all data has gone to memory on reads */

        case SCSI_PHASE_DATAO:
		if (cdata->ctype == CTYPE_700)
			dfifo_mask = C700_DFIFO_BO_MASK;
		else
			dfifo_mask = C710_DFIFO_BO_MASK;
                fifo_bytes = ((dfifo & dfifo_mask) -
			      (dbc & dfifo_mask)) & dfifo_mask;
		if (sstat1 & C700_SSTAT1_OLF)
			fifo_bytes++;
                if (tdata->xt_state == SCSI_XT_SYNC &&
		    (sstat1 & C700_SSTAT1_ORF))
			fifo_bytes++;
		/*
		 * check the MSBs for wide SCSI
		 */
		if (cdata->ctype == CTYPE_720) {
			sstat1 = READBYTEREG(csr->rw.sstat2);
			if (sstat1 & C720_SSTAT2_OLF1)
				fifo_bytes++;
			if (tdata->xt_state == SCSI_XT_SYNC &&
			    (sstat1 & C720_SSTAT2_ORF1))
				fifo_bytes++;
		}
                break;

        default:
                panic("ncr53c700_save_dma: illegal phase");
        }

        cdata->dma.cnt = dbc + fifo_bytes;
        cdata->dma.addr = dnad - fifo_bytes;
        cdata->dma.pc = tdata->pc - sizeof(union instruction);
        cdata->dma.valid = TRUE;
#ifdef DEBUG
        if (ncr53c700_debug & C700_DEBUG_SDP)
                printf("ncr53c700_save_dma: pc = 0x%x, addr = 0x%x, cnt = 0x%x\n", cdata->dma.pc, cdata->dma.addr, cdata->dma.cnt);
#endif
}

/*
 *      ncr53c700_process_message - Process message read from target.
 *
 *      This routine is called when a message has been passed to be processed.
 *	It is passed pointers to controller data structure, target data
 *	structure, and command structure, as well as the message itself.
 *      The message is processed.  Sometimes this means simply accepting
 *	the message, others involve actions,
 *      others involve sending messages back, still others are combinations.
 */
void
ncr53c700_process_message(
			  struct	scsi_controller	*scp,
			  struct	target_data	*tdata,
			  struct	scsi_cmd	*cmd,
			  unsigned char			*msg)
{
        struct  controller_data *cdata;
        struct  target_dma_data *tdma;
        union   csr             *csr;
        int     reject = FALSE;                 /* Reject message when true     */
        unsigned int   t_per, t_off;            /* Transfer params from target  */
        int     x, tid, size;

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_process_message(0x%x, 0x%x, 0x%x, 0x%x)\n",
		       scp, tdata, cmd, msg);
#endif
        cdata = scp->info;
        csr = scp->bus_addr;
        tid = tdata->id;
        tdma = tdata->dma_data;
	size = 1;

        /* See if this is an identify message.  If so then just accept it. */
        if ((msg[0] & SCSI_IDENTIFY) == SCSI_IDENTIFY)
                tdata->tstate = SCSI_SS_ASM;
        else {
                /* Switch on message    */
                switch (msg[SCSI_EMSG_ID]) {
                case SCSI_COMMAND_COMPLETE:
                        /* Command complete.  Set tstate to signify that
			   we're accepting a command complete     */
                        tdata->tstate = SCSI_SS_ASM;
                        break;

                case SCSI_SAVE_DATA_POINTER:
                        /* Save data pointers.  Call routine that does
			   this and set normal state        */
                        ncr53c700_save_data_pointers(scp, cmd);
                        tdata->tstate = SCSI_SS_ACC;
                        break;

                case SCSI_NOP:
                        /* Nothing needs to be done for these messages  */
			panic("ncr53c700_process_message: NOP message!");
                        tdata->tstate = SCSI_SS_ACC;
                        break;

                case SCSI_LNKD_CMD_COMPLETE:
                case SCSI_LNKD_CMD_COMPLETE_F:
                case SCSI_DISCONNECT:
                case SCSI_RESTORE_POINTERS:
                        /* Nothing needs to be done for these messages  */
                        tdata->tstate = SCSI_SS_ACC;
                        break;

                case SCSI_EXTENDED_MESSAGE:
                        /* Check out extended message   */
                        size = msg[SCSI_EMSG_LEN];
                        switch (msg[SCSI_EMSG_CODE]) {
			case SCSI_WIDE_XFER_REQUEST:
			    if (cdata->ctype != CTYPE_720)
				panic("ncr53c700_process_message: unexpected WDTR message");
#ifdef DEBUG
			    if (ncr53c700_debug & C700_DEBUG_INIT)
			        printf("process_msg: tid=%d, width=%d\n",
				       tid,
				       msg[SCSI_EMSG_XFER_WIDTH]);
#endif
			    if (msg[SCSI_EMSG_XFER_WIDTH] > 1)
			        reject = TRUE;
			    else
			        tdata->width = msg[SCSI_EMSG_XFER_WIDTH];
			    /*
			     * Target has gone to async mode, we must also
			     */
			    (void)ncr53c700_setsync(scp, tdata, 0,
						    cdata->maxperiod+1);
			    SET_SXFER(cdata, csr, tdata);
			    break;
                        case SCSI_SYNC_XFER_REQUEST:
			    /* This is a response to a sync data transfer
			       message that we sent.     */
#ifdef DEBUG
			    if (ncr53c700_debug & C700_DEBUG_INIT)
			        printf("process_msg: tid=%d, rcvoff=%d, rcvper=%d(%d)\n",
				       tid,
				       msg[SCSI_EMSG_REQ_ACK_OFF],
				       msg[SCSI_EMSG_XFER_PERIOD],
				       msg[SCSI_EMSG_XFER_PERIOD]*4,
				       tdata->xt_state,
				       tdata->stp, tdata->sof);
#endif
			    t_off = 0;
			    /* First look at REQ/ACK offset */
			    if (msg[SCSI_EMSG_REQ_ACK_OFF] == 0)
			        /* Stay async   */
			        tdata->xt_state = SCSI_XT_ASYNC;
			    else if (msg[SCSI_EMSG_REQ_ACK_OFF] > cdata->maxoff)
			        /* They sent an offset that is larger
				   then ours, forget it      */
			        reject = TRUE;
			    else
			        /* Their offset is less than or equal
				   to ours, let's use it     */
			        t_off = msg[SCSI_EMSG_REQ_ACK_OFF];

			    /* Now look at transfer period - remember, it's encoded */
			    t_per = msg[SCSI_EMSG_XFER_PERIOD] * 4;
			    if (!ncr53c700_setsync(scp, tdata, t_off, t_per)) {
			        reject = TRUE;
			    } else {
			        tdata->xt_state = SCSI_XT_SYNC;
				SET_SXFER(cdata, csr, tdata);
			    }
#ifdef DEBUG
			    if (ncr53c700_debug & C700_DEBUG_INIT)
			        printf("  rej=%d, stp=%d, sof=%d, sbcl=0x%x, sxfer=0x%x\n",
				       reject, tdata->stp, tdata->sof,
				       tdata->sbcl, tdata->sxfer);
#endif
			    break;

			default:
                                reject = TRUE;
                                panic("ncr53c700_process_message: unknown_message (1)");
                                break;
                        }
                        tdata->tstate = SCSI_SS_ACC;
                        break;

                case SCSI_MESSAGE_REJECT:
			/*
			 * If it rejected a wide transfer request message,
			 * we still need to try to negotiate sync.
			 */
			if (tdata->xt_state == SCSI_XT_WMSGSENT) {
#ifdef DEBUG
			    if (ncr53c700_debug & C700_DEBUG_INIT)
				printf("process_msg: tid=%d, WDTR rejected\n",
				       tid);
#endif
			}
			/*
			 * If we sent a sync data transfer request message,
			 * then the drive is async.
			 */
			else if (tdata->xt_state == SCSI_XT_SMSGSENT) {
#ifdef DEBUG
			    if (ncr53c700_debug & C700_DEBUG_INIT)
			        printf("process_msg: tid=%d, SDTR rejected\n",
				       tid);
#endif
			    tdata->xt_state = SCSI_XT_ASYNC;
			}
                        /* If we sent a queue message, then the drive
			   does not support command queueing. */
                        else if (tdata->cq_state == SCSI_CQ_MSGSENT)
                                tdata->cq_state = SCSI_CQ_NO;
                        /* We're in trouble */
                        else {
                                reject = TRUE;
                                panic("ncr53c700_process_message: unknown_message (2)");
                        }
                        tdata->tstate = SCSI_SS_ACC;
                        break;

                /* Placeholders for now */
                case SCSI_CLEAR_QUEUE:
                case SCSI_INITIATE_RECOVERY:
                case SCSI_RELEASE_RECOVERY:
                case SCSI_SIMPLE_QUEUE_TAG:
                case SCSI_HEADOF_QUEUE_TAG:
                case SCSI_ORDERED_QUEUE_TAG:
		case SCSI_IGNORE_WIDE_RESIDUE:

                case SCSI_I_DETECTED_ERROR:
                case SCSI_ABORT:
                case SCSI_BUS_DEVICE_RESET:
                case SCSI_MSG_PARITY_ERROR:
                default:
                        reject = TRUE;
                        panic("ncr53c700_process_message: unknown_message (3)");
                        tdata->tstate = SCSI_SS_ACC;
                }
        }

        /* Save message in tinfo structure in case we want to look
	   at it later  */
        for (x = 0; x < size; ++x)
                tdata->lmsg[x] = msg[x];
        tdata->lmsg_size = size;
        if (reject) {
                /* If rejecting, set reject message in controller
		   structure and set boolean     */
                tdma->msg[0] = SCSI_MESSAGE_REJECT;
                tdma->msg_size = 1;
                cdata->set_atn = TRUE;
        } else {
                /* Otherwise set msg to default */
                cdata->set_atn = FALSE;
                tdma->msgi[0] = SCSI_NOP;
                tdma->msgi_size = 1;
        }

        /* If we want to send a message, here is where we start it. */
        if (cdata->set_atn) {
                cdata->set_atn = FALSE;
                tdata->pc = RELOC(cdata, SCRIPT_ADDR(cdata, Ent_scsi_msg_out));
        }

        /* acknowledge the message (clear ACK/, ATN/) */
	if (cdata->ctype == CTYPE_720)
		CLRBYTEREG(csr->c720.socl, C700_SOCL_ACK|C700_SOCL_ATN);
	else
		CLRBYTEREG(csr->rw.socl, C700_SOCL_ACK|C700_SOCL_ATN);

        pmap_flush_range(vm_map_pmap(kernel_map),
			 (vm_offset_t)tdma, sizeof(*tdma));
}

/*
 *
 */
int
ncr53c700_setsync(struct scsi_controller *scp, struct target_data *tdata,
		  unsigned int t_off, unsigned int t_per)
{
        struct controller_data *cdata;
	int shift;

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_setsync(0x%x, 0x%x, 0x%x, 0x%x)\n",
		       scp, tdata, t_off, t_per);
#endif
        cdata = scp->info;

	/*
	 * Period out of range, do async
	 * XXX what do with do with already negotiated WDTR here?
	 */
	if (t_per > cdata->maxperiod || t_per < cdata->minperiod) {
		tdata->stp = tdata->sof = 0;
		tdata->sxfer = 0;
		if (cdata->ctype == CTYPE_720) {
			tdata->sbcl = C720_SCNTL3_CFDIV2;
			if (tdata->width)
				tdata->sbcl |= C720_SCNTL3_EWS;
		} else
			tdata->sbcl = 0;
		return(0);
	}

	/*
	 * Determine synchronous divisor.
	 */
	switch (cdata->ctype) {
	case CTYPE_700:
		tdata->sbcl = C700_SBCL_DIV_DCNTL;
		shift = 4;
		break;
	case CTYPE_710:
		if (t_per < 200) {
			tdata->sbcl = C710_SBCL_DIV1;
			t_per *= 2;
		} else
			tdata->sbcl = C700_SBCL_DIV_DCNTL;
		shift = 4;
		break;
	case CTYPE_720:
		tdata->sbcl = C720_SCNTL3_CFDIV2;
		if (tdata->width)
			tdata->sbcl |= C720_SCNTL3_EWS;
		if (t_per < 200) {
			tdata->sbcl |= (C720_SCNTL3_CFDIV1 << 4);
			t_per *= 2;
		} else
			tdata->sbcl |= (C720_SCNTL3_CFDIV2 << 4);
		shift = 5;
		break;
	}

	/*
	 * Compute transfer period and maximum offset.
	 * (See manual for this calculation.)
	 */
	t_per = ((t_per * 100) / cdata->tcp) - 400;
	tdata->stp = (t_per / 100) & 7;
#ifdef DEBUG
	if (scp->bustype == BUSTYPE_EISA && tdata->stp == 0)
		panic("ncr53c700_setsync: bad sync transfer period");
#endif
	tdata->sof = t_off & 0xf;
	tdata->sxfer = (tdata->stp << shift) | tdata->sof;
	return(1);
}

/*
 *      ncr53c700_transfer_pad - Null out data in/out phase.
 *
 *      This routine is called when a user error has caused a data in/out
 *	phase that cannot be completed with the paramaters given.
 *	This command send a transfer pad command to the controller.
 */
void
ncr53c700_transfer_pad(
		       struct	scsi_controller	*scp,
		       struct	target_data	*tdata,
		       struct	scsi_cmd	*cmd)
{
        struct  controller_data *cdata;
        union   csr             *csr;

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_transfer_pad(0x%x, 0x%x, 0x%x)\n",
		       scp, tdata, cmd);
#endif
        cdata = scp->info;
        csr = scp->bus_addr;

        /* Set new software state and return    */
        if (cmd->tgt->transient_state.out_count)
                tdata->tstate = SCSI_SS_DATA_OUT;
        else
                tdata->tstate = SCSI_SS_DATA_IN;

        panic("ncr53c700_transfer_pad: entered");
}

/*
 *      ncr53c700_data_inout - Set up to perform I/O.
 *
 *      This routine is called to enter the data in/out phase. 
 *	It sets up the (software) DMA controller and
 *      scsi bus controller for data transfer.
 */
void
ncr53c700_data_inout(
		     struct	scsi_controller	*scp,
		     struct	target_data	*tdata,
		     struct	scsi_cmd	*cmd)
{
        struct  controller_data *cdata;
        struct  target_dma_data *tdma;
        union   csr             *csr;
        struct  xfer_data       *xfer;
        int     x, tid;

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_data_inout(0x%x, 0x%x, 0x%x)\n",
		       scp, tdata, cmd);
#endif
        cdata = scp->info;
        csr = scp->bus_addr;
        tid = tdata->id;
        xfer = &tdata->xfer_data;
        tdma = tdata->dma_data;

        /* We are going to set up for DMA mode transfer.
	   First make sure we have data to send. */
        if (cmd->data_len == 0 )  {
                /* Set error in structure, call transfer pad, and return */
                cmd->scsi_status = NCR_SCSI_DMA_UNDERRUN;
                ncr53c700_transfer_pad(scp, tdata, cmd);
                return;
        }

#ifdef DEBUG
        if (ncr53c700_debug & C700_DEBUG_DMA)
	        printf("ncr53c700_data_inout: xfer inst = 0x%x, tdma inst 0x%x, xfer pc = 0x%x, count = 0x%x\n",
			xfer->inst, tdma->inst, xfer->pc, xfer->cnt);
#endif
        /* Copy in target instructions */
        for (x = 0; x < xfer->cnt; ++x) {
                tdma->inst[x] = xfer->inst[x];
#ifdef DEBUG
                if (ncr53c700_debug & C700_DEBUG_DMA)
		        printf("ncr53c700_data_inout: inst[%d] = 0x%x 0x%x\n",
                               x, BYTESWAP(tdma->inst[x].l[0]),
			       BYTESWAP(tdma->inst[x].l[1]));
#endif
        }

        /* Set new software state and return    */
        if (cmd->tgt->transient_state.out_count)
                tdata->tstate = SCSI_SS_DATA_OUT;
        else
                tdata->tstate = SCSI_SS_DATA_IN;

        tdata->pc = xfer->pc;
        pmap_flush_range(vm_map_pmap(kernel_map),
			 (vm_offset_t)tdma,
			 sizeof(*tdma));
}

/*
 *      ncr53c700_iodone - I/O complete.
 *
 *      This routine is called when a command is finished on a target.
 *	It is passed to the pointer to the
 *      controller and target structures in question. 
 *	This routine wakes up the waiting process.
 */
void
ncr53c700_iodone(
		 struct	scsi_controller	*scp,
		 struct	target_data	*tdata)
{
        struct  controller_data *cdata;
        struct	scsi_cmd	*cmd;
        int                     tid;
	target_info_t		*tgt;

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_iodone(0x%x, 0x%x)\n", scp, tdata);
#endif

        cdata = scp->info;
        tid = tdata->id;

        /* the first command in the queue is done */
        cmd = scp->scsi_cmd[tid];
        if (!cmd->queued)
                panic("ncr53c700_iodone: got a command complete, but none on queue");

	tgt = cmd->tgt;
        if ((tgt->flags & CMD_ACTIVE) == 0)
                panic("ncr53c700_iodone: command not active");

        /* unlink command */
        cmd->queued = FALSE;

        tgt->flags &= ~(CMD_ACTIVE);

#if NEISA > 0
        if (cmd->flags & CMD_IOVA) {

                struct  xfer_data *xfer;

		xfer = &tdata->xfer_data;

                /* free iomap space */
                atbus_iomap_free(xfer->ioaddr, xfer->npages);
                cmd->flags &= ~(CMD_IOVA);
        }
#endif

        /*
	 * Purge affected cache lines on read operations.
	 *
	 * XXX we really shouldn't need to do this since we purged/flushed
	 * before the operation.  However, I don't know if we can trust all
	 * applications to not touch their IO buffer while a read is taking
	 * place.  If they do touch it and we didn't purge, they could mask
	 * some of the incoming data.
	 */
        if (cmd->data_addr && tgt->transient_state.in_count)
                pmap_purge_range(vm_map_pmap(kernel_map),
				 cmd->data_addr,
				 cmd->data_len);

	if (cmd->scsi_status == NCR_SCSI_GOOD)
		tgt->done = SCSI_RET_SUCCESS;
	else if (cmd->scsi_status == NCR_SCSI_DEVICE_NA)
		tgt->done = SCSI_RET_DEVICE_DOWN;
	else {
		scsi_error(tgt, SCSI_ERR_STATUS, cmd->scsi_status, 0);
		tgt->done = (cmd->scsi_status == NCR_SCSI_BUSY) ?
		  	    SCSI_RET_RETRY : SCSI_RET_NEED_SENSE;
	}

	if (scp->wd.nactive-- == 1)
		scp->wd.watchdog_state = SCSI_WD_INACTIVE;
	if (tgt->ior) {
		(*cmd->tgt->dev_ops->restart)( tgt, TRUE);
	}
}

/*
 *      ncr53c700_next_phase - Switch out to next phase.
 *
 *      This routine is called from the interrupt handler to dispatch control
 *	to one of the NCR53c700 scsi phase routines.
 *      It is passed pointers to the current controller data structure,
 *	target data structure, and command data structure
 *      for this bus/target/command, as well as the next scsi bus phase.
 */
void
ncr53c700_next_phase(
		     struct  scsi_controller *scp,
		     struct  target_data     *tdata,
		     struct  scsi_cmd        *cmd,
		     unsigned char            np)
{
	struct  controller_data *cdata;
#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_next_phase(0x%x, 0x%x, 0x%x, 0x%x)\n",
		       scp, tdata, cmd, np);
#endif
        cdata = scp->info;

        switch (np) {
        case SCSI_PHASE_CMD :
                /* Command Phase        */
                tdata->tstate = SCSI_PHASE_CMD;
                tdata->pc = RELOC(cdata, SCRIPT_ADDR(cdata, Ent_scsi_cmd));
                break;

        case SCSI_PHASE_DATAO :
                /* Check direction and call phase handler       */
                if (!cmd->tgt->transient_state.out_count) {
                        /* Set error and call transfer pad routine      */
                        cmd->scsi_status = NCR_SCSI_DATA_DIRECTION_ERROR;
                        ncr53c700_transfer_pad(scp, tdata, cmd);
                } else
                        ncr53c700_data_inout(scp, tdata, cmd);
                break;

        case SCSI_PHASE_DATAI :
                /* Check direction and call phase handler       */
                if (!cmd->tgt->transient_state.in_count) {
                        /* Set error and call transfer pad routine      */
                        cmd->scsi_status = NCR_SCSI_DATA_DIRECTION_ERROR;
                        ncr53c700_transfer_pad(scp, tdata, cmd);
                } else
                        ncr53c700_data_inout(scp, tdata, cmd);
                break;

        case SCSI_PHASE_STATUS :
                /* Status Phase */
                tdata->tstate = SCSI_SS_STAT;
                tdata->pc = RELOC(cdata, SCRIPT_ADDR(cdata, Ent_scsi_status));
                break;

        case SCSI_PHASE_MSG_IN :
                /* Message in phase     */
                tdata->tstate = SCSI_SS_MSG_IN;
                tdata->pc = RELOC(cdata, SCRIPT_ADDR(cdata, Ent_scsi_msg_in));
                break;

        case SCSI_PHASE_MSG_OUT :
                /* Message out phase */
                tdata->tstate = SCSI_SS_MSG_OUT;
                tdata->pc = RELOC(cdata, SCRIPT_ADDR(cdata, Ent_scsi_msg_out));
                break;

        default :
                panic("ncr53c700_next_phase: illegal phase\n");
                break;
        }
}

/*
 *      ncr53c700_process_int - Process script interrupt instruction.
 *
 *      Returns indication of whether target should be restarted.
 */
int
ncr53c700_process_int(
		      struct  scsi_controller *scp,
		      struct  target_data     *tdata,
		      struct  target_dma_data *tdma,
		      struct  scsi_cmd        *cmd,
		      unsigned long           code)
{
        struct controller_data *cdata;
        union csr *csr;
	unsigned char sstat2;

        csr = scp->bus_addr;
        cdata = scp->info;

        switch (code) {
        case A_err2:
		/*
		 * This case used to unconditionally panic.
		 * However, this case would happen with the HP MO drive.
		 * I noticed that the phase it was in was DATA_IN so on a
		 * lark I let it go as though this case were normal and
		 * everything worked!  I have never seen the panic on any
		 * other drive so I feel this is fairly safe.
		 */
		sstat2 = C700_GETBP(cdata, csr);
		if (sstat2 == SCSI_PHASE_DATAI || sstat2 == SCSI_PHASE_DATAO) {
			ncr53c700_next_phase(scp, tdata, cmd, sstat2);
			break;
		}
		/* else fall into... */
        case A_err1:
        case A_err3:
        case A_err4:
        case A_err5:
        case A_err24:
                printf("ncr53c700_process_int: illegal bus phase, bp = 0x%x, code = 0x%x\n", C700_GETBP(cdata, csr), code);
                panic("ncr53c700_process_int: illegal bus phase");
                break;

        case A_err6:       /* std message other than disconnect */
        case A_err7:       /* ext message */
                /* insure we get to see the message */
                pmap_purge_range(vm_map_pmap(kernel_map),
				 (vm_offset_t)tdma->msgi, sizeof(tdma->msgi));
                ncr53c700_process_message(scp, tdata, cmd, tdma->msgi);
                break;

        case A_err9:       /* disconnect */
                tdata->tstate = SCSI_SS_DIS;
		cdata->dma.valid = FALSE;
                cdata->active = FALSE;
                break;

        case A_err18:
                panic("ncr53c700_process_int: illegal reselect");
                break;

        case A_err21:
                tdata->tstate = SCSI_SS_MSG_IN;
                tdata->pc = RELOC(cdata, SCRIPT_ADDR(cdata, Ent_scsi_msg_in));
                break;

        case A_err22:
                tdata->tstate = SCSI_SS_STAT;
                tdata->pc = RELOC(cdata, SCRIPT_ADDR(cdata, Ent_scsi_status));
                break;

        case A_err23:
                tdata->tstate = SCSI_SS_MSG_OUT;
                tdata->pc = RELOC(cdata, SCRIPT_ADDR(cdata,
						     Ent_scsi_msg_out));
                break;

        case A_err25:
                panic("ncr53c700_process_int: host selected as target\n");
                break;

        case A_err26:
                panic("ncr53c700_process_int: illegal message after status phase\n");
                break;

        case A_err19:
        case A_err27:
                ncr53c700_next_phase(scp, tdata, cmd, SCSI_PHASE_DATAI);
                break;

        case A_err20:
        case A_err28:
                ncr53c700_next_phase(scp, tdata, cmd, SCSI_PHASE_DATAO);
                break;

        case A_err31:      /* command completely done */
                /* insure we get to see the status */
                pmap_purge_range(vm_map_pmap(kernel_map),
				 (vm_offset_t)tdma->sts, sizeof(tdma->sts));
                cmd->scsi_status = tdma->sts[0];
#ifdef DEBUG
	        if (ncr53c700_debug & C700_DEBUG_ENTRY)
        		printf("ncr53c700_process_int: command done, sts = 0x%x\n", cmd->scsi_status);
#endif
                tdata->tstate = SCSI_SS_DIS;
                tdata->active = FALSE;
                tdata->pc = 0;
                ncr53c700_iodone(scp, tdata);
                /* don't restart controller */
                cdata->active = FALSE;
		cdata->dma.valid = FALSE;
                break;

        default:
                panic("ncr53c700_process_int: unknown script interrupt code");
                break;
        }
        return(cdata->active);
}

/*
 *      ncr53c700_intr - Process controller interrrupts.
 */
void
ncr53c700_intr(struct scsi_controller *scp)
{
        struct  controller_data *cdata;
        struct  target_data     *tdata;
        struct  target_dma_data *tdma;
        union   csr             *csr;
        struct  scsi_cmd        *cmd;
        union   instruction     *ip;
        int                     bit_position, tid, stid;
        unsigned char           istat, dstat, sstat0, phase;
        unsigned long           code, isaved;

#ifdef DEBUG
        struct itrace           *it = (struct itrace *)0;
#endif
#define C700_MAX_INTR_LOOP	20

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_intr(0x%x)\n", scp);
#endif
        isaved = 0;

	if (scp->wd.nactive)
		scp->wd.watchdog_state = SCSI_WD_ACTIVE;

        tdata = (struct target_data *) 0;

        csr = scp->bus_addr;
        cdata = scp->info;

        cdata->it_icount++;

        /* See if there is another interrupt that we can service */
        while (1) {
	    if (cdata->ctype == CTYPE_720)
		    istat = READBYTEREG(csr->c720.istat);
	    else
		    istat = READBYTEREG(csr->rw.istat);
	    if ((istat & (C700_ISTAT_DIP|C700_ISTAT_SIP|C700_ISTAT_ABRT)) == 0)
		    break;
	    isaved++;
	    if (isaved > C700_MAX_INTR_LOOP)
                    panic("ncr53c700_intr: looping");
#ifdef DEBUG
	    if (ncr53c700_debug & C700_DEBUG_INTS)
		    printf("ncr53c700_intr: istat=0x%x\n", istat);
	    it = &cdata->it[cdata->it_index];
	    if (it) {
                bzero((char *)it, sizeof(struct itrace)); 
		/* trace interrupt counts */ 
		it->icount = cdata->it_icount;
		it->isaved = isaved;
		it->istat = istat;
            }
#endif
	    if (cdata->active) {
                tid = cdata->ctid;
		tdata = cdata->target_data[tid];
		tdma = tdata->dma_data;

		cmd = scp->scsi_cmd[tid];
                if (!cmd->queued)
                        panic("ncr53c700_intr: no current command (interrupt)");

		/* controller is frozen, save restart point */
		tdata->pc = BYTESWAP(csr->rw.dsp);
#ifdef DEBUG
		if (it) {
		    it->tdata = tdata;
		    it->tid_entry = tid;
		    it->pc_entry = tdata->pc;
		}
#endif
            } else {
                cmd = (struct scsi_cmd *) 0;
		tdata = (struct target_data *) 0;
		tdma = (struct target_dma_data *) 0;
            }

	    /* if we got an abort interrupt turn off the abort bit */
	    if (istat & (C700_ISTAT_ABRT|C700_ISTAT_DIP))
		if (cdata->ctype == CTYPE_720)
		    WRITEBYTEREG(csr->c720.istat, 0);
		else
		    WRITEBYTEREG(csr->rw.istat, 0);

	    if (istat & C700_ISTAT_DIP) {
                /* dma or controller related interrupts */
	        dstat = READBYTEREG(csr->rw.dstat);
#ifdef DEBUG
		if (ncr53c700_debug & C700_DEBUG_INTS)
		    printf("ncr53c700_intr: dstat=0x%x\n", dstat);
		if (it) it->dstat = dstat;
#endif
		if (dstat & C700_DSTAT_SIR) {
                    /* script interrupt instruction */
		    code = BYTESWAP(csr->rw.dsps);
		    /* clear instruction & code so subsequent
		       reselections don't see it! */
		    csr->rw.dsps = csr->rw.d.dbc = 0;
#ifdef DEBUG
		    if (it) it->code = code;
#endif
		    if (!ncr53c700_process_int(scp,tdata, tdma, cmd, code))
                                        /* don't restart target */
                        tdata = (struct target_data *)0;

                }
		if (dstat & C700_DSTAT_ABRT) {
                    panic("ncr53c700_intr: abort interrupt");
                }
		if (dstat & C700_DSTAT_SPI) {
                    panic("ncr53c700_intr: script pipeline interrupt");
                }
		if (dstat & C700_DSTAT_WTD) {
                    panic("ncr53c700_intr: watchdog timeout");
                }
		if (dstat & C700_DSTAT_IID) {
                    /* check for WAIT DISCONNECT problem */
                    if (READBYTEREG(csr->rw.d.d.dcmd) == 0x48) {
                     	cdata->ill_resel++;        /* count it */
			ip = (union instruction *)SCRIPT_VA(cdata, tdata->pc);
			code = BYTESWAP(ip->xfer_control.jump_addr);
#ifdef DEBUG
			if (it) it->code = code;
#endif
			if (ncr53c700_process_int(scp, tdata, tdma, cmd, code))
                            panic("ncr53c700_intr: illegal reselection");
                        } else
                            panic("ncr53c700_intr: illegal script instruction");
                }
            }

	    r = csr->rw.temp;	/* read delay for chip */

	    if (istat & C700_ISTAT_SIP) {
                /* scsi related interrupt */
	        if (cdata->ctype == CTYPE_720) {
		    unsigned char sist0, sist1;

		    sist0 = READBYTEREG(csr->c720.sist0);
		    r = csr->rw.temp;	/* delay */
		    sist1 = READBYTEREG(csr->c720.sist1);

		    /* XXX make it look like a 700/710 mask */
		    sstat0 = sist0 & ~(C720_SIEN_SEL|C720_SIEN_RSL);
		    if (sist0 & (C720_SIEN_SEL|C720_SIEN_RSL))
		        sstat0 |= C700_SSTAT0_SEL;
		    if (sist1 & C720_SIEN_STO)
		        sstat0 |= C700_SSTAT0_STO;
		} else
		    sstat0 = READBYTEREG(csr->rw.sstat0);
#ifdef DEBUG
		if (ncr53c700_debug & C700_DEBUG_INTS)
		    printf("ncr53c700_intr: sstat=0x%x\n", sstat0);
		if (it) it->sstat0 = sstat0;
#endif
		if (sstat0 & C700_SSTAT0_RST) {
		    panic("ncr53c700_intr: scsi bus reset detected");
                } else if (sstat0 & C700_SSTAT0_PAR) {
                    panic("ncr53c700_intr: scsi bus parity error");
                } else if (sstat0 & C700_SSTAT0_SGE) {
                    panic("ncr53c700_intr: gross error");
                }
		if (sstat0 & C700_SSTAT0_MA) {
		    /* target hopped out of the state it was in */
		    phase = C700_DCMD_BP(READBYTEREG(csr->rw.d.d.dcmd));
		    switch (phase) {
		    case SCSI_PHASE_MSG_OUT:
		        /* target is probably going to reject our
			   extended message (e.g. async targets on
			   sync messages) */
		    case SCSI_PHASE_CMD:
		        /* target is probably going to reject the
			   cdb as unknown */
		        break;

		    case SCSI_PHASE_MSG_IN:
			tdata->tstate = SCSI_SS_MSG_IN;
			/* insure we get to see the message */
			pmap_purge_range(vm_map_pmap(kernel_map),
					 (vm_offset_t)tdma->msgi,
					 sizeof(tdma->msgi));
			ncr53c700_process_message(scp, tdata, cmd, tdma->msgi);
			break;

		    case SCSI_PHASE_DATAI:
		    case SCSI_PHASE_DATAO:
			if (phase == SCSI_PHASE_DATAI)
			    tdata->tstate = SCSI_SS_DATA_IN;
			else
			    tdata->tstate = SCSI_SS_DATA_OUT;
			ncr53c700_save_dma(scp, tdata, cmd, phase);
			break;

		    default:
			panic("ncr53c700_intr: invalid phase");
			break;
		    }

		    /* now clear FIFOs */
		    C700_CLF(cdata, csr);

		    /* determine where we are going */
		    ncr53c700_next_phase(scp, tdata, cmd,
					 C700_GETBP(cdata, csr));
		}
		/*
		 * Order is important here.  On target selection if there is NO
		 * device, then we will receive a selection timeout (STO) and
		 * an unexpected disconnect (UDC), but not necessarily on the
		 * same sstat0 read.  We must therefore prepare for the UDC
		 * and possibly a relesection by another target on the SAME
		 * sstat0 read, thus the order here is STO then UDC then SEL,
		 * and all must be checked on the same read.
		 */
		if (sstat0 & C700_SSTAT0_STO) {
		    /* selection timeout, target is N/A */
		    cmd->scsi_status = NCR_SCSI_DEVICE_NA;
		    tdata->tstate = SCSI_SS_DIS;
		    tdata->active = FALSE;
		    tdata->pc = 0;
		    /* order is important in this iodone sequence */
		    ncr53c700_iodone(scp, tdata);
		    /* don't restart controller */
		    tdata = (struct target_data *) 0;
		    cdata->active = FALSE;
		}
		if (sstat0 & C700_SSTAT0_UDC) {
#ifdef DEBUG
		    if (ncr53c700_debug & C700_DEBUG_UDC)
		        printf("ncr53c700_intr: UDC dsp=0x%x, dsps=0x%x\n",
			       csr->rw.dsp, csr->rw.dsps);
#endif
		    /* clear FIFOs */
		    C700_CLF(cdata, csr);
		    /*
		     * if we have an active target, this is no good, otherwise,
		     * it probably arrived along with a selection timeout,
		     * so we will ignore it.
		     */
		    if (tdata)
		        panic("ncr53c700_intr: unexpected disconnect");
		}
		if (sstat0 & C700_SSTAT0_SEL) {
		    /*
		     * selected or reselected
		     *
		     * If we've got a current software target, check to see if
		     * we've been reselected when working on another target.
		     */
		    if (tdata) {
		        /* Check to see if we were in the process of
			   interrupting after disconnect */
		        if (READBYTEREG(csr->rw.d.d.dcmd) == 0x98) {
			    cdata->sel_int++;        /* count it */
			    /* script interrupt instruction */
			    code = BYTESWAP(csr->rw.dsps);
			    /* clear instruction & code so subsequent
			       reselections don't see it! */
			    csr->rw.dsps = csr->rw.d.dbc = 0;
#ifdef DEBUG
			    if (it) it->code = code;
#endif
			    if (ncr53c700_process_int(scp, tdata, 
						      tdma, cmd, code))
			        panic("ncr53c700_intr: illegal reselection");
			}
			/* check for WAIT DISCONNECT problem */
			else if (READBYTEREG(csr->rw.d.d.dcmd) == 0x48) {
			    cdata->sel_resel++;        /* count it */
			    ip = (union instruction *)SCRIPT_VA(cdata,
								tdata->pc);
			    code = BYTESWAP(ip->xfer_control.jump_addr);
#ifdef DEBUG
			    if (it) it->code = code;
#endif
			    if (ncr53c700_process_int(scp,tdata, tdma,
						      cmd, code))
			        panic("ncr53c700_intr: illegal reselection");
			} else if (tdata->tstate == SCSI_PHASE_CMD) {
			    /*
			     * We've been reselected when selecting.
			     * Reset active state for this target.
			     */
			    tdata->active = FALSE;
			    cdata->restart_resel++;        /* count it */
#ifdef DEBUG
			    if (it) it->restart = TRUE;
#endif
		        } else
			    panic("ncr53c700_intr: reselected while not in software command state");
		    }

		    /* determine target */
		    stid = READBYTEREG(csr->rw.sfbr);
		    stid &= ~(1<<SCSI_INITIATOR);
		    bit_position = 1;
		    for (tid = 0; tid < MAX_SCSI_TARGETS; ++tid)
		        if (stid & bit_position)
			    break;
			else
			    bit_position <<= 1;

		    cdata->ctid = tid;
		    cmd = scp->scsi_cmd[tid];
		    if (!cmd->queued)
		        panic("ncr53c700_intr: no current command (reselection)");

		    tdata = cdata->target_data[tid];
		    tdata->tstate = SCSI_SS_RES;
		    tdata->active = TRUE;
		    tdata->pc = RELOC(cdata,
				      SCRIPT_ADDR(cdata, Ent_scsi_resel));

		    tdma = tdata->dma_data;

		    /* some targets reselect and go into MSG_IN,
		       send them a NOP there */
		    tdma->msg[0] = SCSI_NOP;
		    tdma->msg_size = 1;

		    /* Load transfer period & sync offset for this target */
		    SET_SXFER(cdata, csr, tdata);

		    /* patch script for this target */
		    ncr53c700_patch_script(scp, tdata, tid);
		    cdata->active = TRUE;
		}
		/* sstat0 & C700_SSTAT0_CMP - don't care condition */
	    }
#ifdef DEBUG
	    if (it && (++cdata->it_index >= cdata->it_nentries))
	        cdata->it_index = 0;
#endif
	}

        /* If we still are operating on a target and its still active,
	   restart it */
        if (tdata && tdata->active && tdata->tstate != SCSI_SS_DIS) {
#ifdef	DEBUG
	    if (it) {
	        it->pc_exit = tdata->pc;
		it->tid_exit = tid;
	    }
#endif
	    START_CONTROLLER(cdata, tdata);
	}
        /* See if there's more work to do */
        else if (cdata->active == FALSE) {
	    ncr53c700_start(scp);
        }
        cdata->it_ihandled += isaved;
        if (isaved == 0)
	    cdata->it_ispur++;
}


int
ncr53c700_attach(hp_ctlrinfo_t *ctlrinfop)
{
	struct scsi_controller  *scp;
        union  csr              *csr;
        struct controller_data  *cdata;
        struct target_data      *tdata;
        struct target_dma_data  *tdma;
        unsigned char           *dma;
        unsigned long           *s;
        vm_offset_t             vaddr;
        unsigned int            tcpl;           /* Long tcp for computation     */
        unsigned int            tid;            /* Target ID */
	unsigned int		bus_minperiod;
        unsigned long           dma_size, script_size, target_dma_size, page_roundup;
        int                     ii, id, epc;
	int			ctype, isbigend;
	struct controller_data	tmpcdata;	/* XXX */
	extern unsigned long	scsi_clock_freq;

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_attach(0x%x)\n", ctlrinfop);
#endif
        if (ctlrinfop->ctlrnum >= NNCR)
                return(0);

        /* init per controller structures */
	if (!ncr53c700_controller_init) {
                for (ii = 1; ii < NNCR; ii++)
                        ncr53c700_controller[ii] = ncr53c700_controller[0];
                ncr53c700_controller_init = 1;
        }

        /* Setup per controller data */
	scp = &ncr53c700_controller[ctlrinfop->ctlrnum];
        scp->bustype = ctlrinfop->parent_ctlrinfop->ctlr_businfop->bustype;

	/* XXX determine type and endianness */
	if (ctlrinfop->ctlrnum == 0) {
		if (isgecko) {
			ctype = CTYPE_710;
			isbigend = 1;
		} else {
			ctype = CTYPE_700;
			isbigend = 0;
		}
	} else {
		ctype = CTYPE_720;
		isbigend = 1;
	}

#ifdef DEBUG
	printf("SCSI NCR53C7%1d0 processor\n", ctype);
#endif

	/* XXX hack, hack, hack: for various macros til cdata is allocated */
	tmpcdata.bigend = isbigend;
	cdata = &tmpcdata;

#if NEISA > 0
        if (scp->bustype == BUSTYPE_EISA) {
                unsigned char irq;

	        csr = (union csr *)ctlrinfop->vcsrs[0];

		/* see if id, EPC already set by EISA IODC */
		id = READBYTEREG(csr->rw.scid);
		if (id != (1<<SCSI_INITIATOR))
			printf("scsi%d: id %x, expected %x\n",
			       ctlrinfop->ctlrnum, id,
			       1 << SCSI_INITIATOR);
		epc = READBYTEREG(csr->rw.scntl0) & C700_SCNTL0_EPC;

                /* configure eisa card regs */
                switch (ctlrinfop->irq) {
                case 3:  irq = EISA_RSRC_IRQ3; break;
                case 4:  irq = EISA_RSRC_IRQ4; break;
                case 5:  irq = EISA_RSRC_IRQ5; break;
                case 7:  irq = EISA_RSRC_IRQ7; break;
                case 8:  irq = EISA_RSRC_IRQ8; break;
                case 10: irq = EISA_RSRC_IRQ10; break;
                case 11: irq = EISA_RSRC_IRQ11; break;
                default:
                        panic("ncr53c700_attach: invalid interrupt request");
                }
                WRITEBYTEREG(csr->eisa.control, EISA_CNTL_ENABLE);
                WRITEBYTEREG(csr->eisa.rsrc, irq|EISA_RSRC_NABRT|EISA_RSRC_ROM_DIS);

                /*
                 * Now, reset the whole chip by toggling reset.
                 * This will NOT affect mode setting done above.
                 */
                WRITEBYTEREG(csr->rw.istat, C710_ISTAT_RST);
                DELAY(100);
                WRITEBYTEREG(csr->rw.istat, 0);

                /* Set burst length to 8 bytes */
                WRITEBYTEREG(csr->c710.dmode, C700_DMODE_B8);

                /* Init snoop control */
		WRITEBYTEREG(csr->rw.ctest7, C710_CTEST7_SC1);
		WRITEBYTEREG(csr->c710.ctest8, C710_CTEST8_SM);

                /* Disable cache bursts & selection timer,
		   enable differential */
                WRITEBYTEREG(csr->rw.ctest7,
			     C710_CTEST7_CDIS|C710_CTEST7_NOTIME
			     |C710_CTEST7_DIFF);
                page_roundup = ATBUS_IOMAP_PAGE_SIZE;
        } else
#endif
	{
		csr = scp->bustype == BUSTYPE_ZALON ?
			(union csr *)(ctlrinfop->vcsrs[0] + ZALON_OFFSET) :
			  (union csr *)(ctlrinfop->vcsrs[0] + CORE_OFFSET);

		if (ctype == CTYPE_720) {
#if	hp_pa
		    if (scp->bustype == BUSTYPE_ZALON) {
			/* XXX for various macros til cdata is allocated */
			tmpcdata.csr = csr;
			
			zalon_init(cdata);
			/* Set the burst length	to 4 bytes */
			WRITEBYTEREG(csr->c700.dmode, C700_DMODE_B4);
		   } else
#endif	/* hp_pa */
		   { 
			/* Enable ack (must be the first access) */
			WRITEBYTEREG(csr->rw.dcntl, C720_DCNTL_EA);

			/* Reset the chip */
			WRITEBYTEREG(csr->c720.istat, C710_ISTAT_RST);
			DELAY(100);
			WRITEBYTEREG(csr->c720.istat, 0);

			/* Set the burst length	to 8 bytes */
			WRITEBYTEREG(csr->c720.dmode, C720_DMODE_B8);

			/* Set burst length on cutoff */
			((struct core_fwscsi *)ctlrinfop->vcsrs[0])->numxfers =
				C720_DMODE_B8 >> 6;

		   }

		    /* Enable differential */
		    WRITEBYTEREG(csr->c720.stest2, C720_STEST2_DIF);
		    
		    /* Set selection timeout interval */
		    WRITEBYTEREG(csr->c720.stime0, C720_STIME0_SEL204);

		    /* Set our reselection ID (7) */
		    WRITEBYTEREG(csr->c720.respid0, C720_RESPID0_ID7);
		    WRITEBYTEREG(csr->c720.respid1, 0);
		} else if (ctype == CTYPE_710) {
			/* Reset the chip */
			WRITEBYTEREG(csr->rw.istat, C710_ISTAT_RST);
			DELAY(100);
			WRITEBYTEREG(csr->rw.istat, 0);

			/* Set the burst length	to 8 bytes */
			WRITEBYTEREG(csr->c710.dmode, C700_DMODE_B8);

			/* Init snoop control */
			WRITEBYTEREG(csr->rw.ctest7, C710_CTEST7_SC1);
			WRITEBYTEREG(csr->c710.ctest8, C710_CTEST8_SM);

			/* Disable selection timer */
			WRITEBYTEREG(csr->rw.ctest7, C710_CTEST7_NOTIME);
		} else {
			/* Reset the chip */
			WRITEBYTEREG(csr->rw.dcntl, C700_DCNTL_RST);
			DELAY(100);
			WRITEBYTEREG(csr->rw.dcntl, 0);

			/* Set the burst length	to 4 bytes */
			WRITEBYTEREG(csr->c700.dmode, C700_DMODE_B4);
		}

                page_roundup = NBPG;
	}

        scp->bus_addr = csr;

        /* Disable all interrupts */
	if (ctype == CTYPE_720) {
		WRITEBYTEREG(csr->c720.sien0, 0);
	} else {
		WRITEBYTEREG(csr->rw.sien, 0);
	}

        /* Reset scsi bus (toggle rst/) */
        WRITEBYTEREG(csr->rw.scntl1, C700_SCNTL1_RST);
        DELAY(C700_BUS_RESET_DELAY);            /* wait for bus reset */
        WRITEBYTEREG(csr->rw.scntl1, 0);

        /* Enable reselection */
	if (ctype != CTYPE_720) {
		SETBYTEREG(csr->rw.scntl1, C700_SCNTL1_ESR);
	}

#if NEISA > 0
        /* insert extra clock cycle to prevent parity errors */
        if (scp->bustype == BUSTYPE_EISA) {
                SETBYTEREG(csr->rw.scntl1, C700_SCNTL1_EXC);
        }
#endif

	/* enable active negation */
	if (ctype == CTYPE_710) {
		SETBYTEREG(csr->rw.ctest0, C710_CTEST0_EAN);
	} else if (ctype == CTYPE_720) {
		WRITEBYTEREG(csr->c720.stest3, C720_STEST3_TE);
	}

        /* XXX remove pending interrupt (from bus reset) */
	if (ctype == CTYPE_720) {
		r = READBYTEREG(csr->c720.sist0);
		r = csr->rw.temp;
		r = READBYTEREG(csr->c720.sist1);
	} else {
		r = READBYTEREG(csr->rw.sstat0);
	}
	
        /* 
         * Set up scntl 0 - other bits are controlled by siop
         * Enable parity checking, parity generation, and full arbitration 
         * Don't get parity int unless AAP set.
         */
#if NEISA > 0
        if (scp->bustype == BUSTYPE_EISA) {
                WRITEBYTEREG(csr->rw.scntl0, C700_SCNTL0_EPG|C700_SCNTL0_FULLARB|
			     C700_SCNTL0_AAP);
		SETBYTEREG(csr->rw.scntl0, (id ? epc : C700_SCNTL0_EPC));

                SETBYTEREG(csr->rw.dcntl, C700_DCNTL_CFDIV2);
                tcpl = (1000000*100)/(50000000/2000);
		bus_minperiod = 100;	/* 10 MB/s */
        } else
#endif
	if (ctype == CTYPE_720) {
		WRITEBYTEREG(csr->c720.scntl3, C720_SCNTL3_CFDIV2);
		tcpl = (1000000*100)/(40000000/2000);
#if 	hp_pa
	    	if (scp->bustype == BUSTYPE_ZALON)
                	WRITEBYTEREG(csr->rw.scntl0,
				     C700_SCNTL0_EPC|C700_SCNTL0_FULLARB);
	    	else
#endif	/* hp_pa */
                	WRITEBYTEREG(csr->rw.scntl0,
				     C700_SCNTL0_EPC|C700_SCNTL0_EPG
				     |C700_SCNTL0_FULLARB|C700_SCNTL0_AAP);
		bus_minperiod = 100;	/* 10 MB/s */
	} else {
                if (scsi_clock_freq > 37500000) {
                        SETBYTEREG(csr->rw.dcntl, C700_DCNTL_CFDIV2);
                        tcpl = (1000000*100)/(scsi_clock_freq/2000);
                }
                else if (scsi_clock_freq > 25000000) {
                        SETBYTEREG(csr->rw.dcntl, C700_DCNTL_CFDIV15);
                        tcpl = (1000000*100)/(scsi_clock_freq/1500);
                }
                else {
                        SETBYTEREG(csr->rw.dcntl, C700_DCNTL_CFDIV1);
                        tcpl = (1000000*100)/(scsi_clock_freq/1000);
                }
                WRITEBYTEREG(csr->rw.scntl0,
			     C700_SCNTL0_EPC|C700_SCNTL0_EPG
			     |C700_SCNTL0_FULLARB|C700_SCNTL0_AAP);
		bus_minperiod = 200;	/* 5 MB/s */
        }

        /* tell chip who we are */
	if (ctype == CTYPE_720) {
		WRITEBYTEREG(csr->rw.scid, C720_SCID_RRE|SCSI_INITIATOR);
	} else {
		WRITEBYTEREG(csr->rw.scid, (1<<SCSI_INITIATOR));
	}

        /* enable scsi & dma interrupts */
	if (ctype == CTYPE_720) {
		/* all but function complete, general purpose, HTH */
		WRITEBYTEREG(csr->c720.sien0, ~0 & ~(C700_SIEN_FCMP));
		WRITEBYTEREG(csr->c720.sien1, C720_SIEN_STO);
	} else {
		/* all but function complete */
		WRITEBYTEREG(csr->rw.sien, ~0 & ~(C700_SIEN_FCMP));
	}
        WRITEBYTEREG(csr->rw.dien,
		     C700_DIEN_SIR|C700_DIEN_WTD|C700_DIEN_IID|C700_DIEN_ABRT);

        /* Allocate space for NCR53C700 specific data structures */
	if ((cdata = (struct controller_data *) kalloc(
				   sizeof(struct controller_data))) == 0) {
                printf("ncr53c700_attach: unable to allocate controller data memory\n");
                return(0);
        }
        scp->info = cdata;
        bzero((char *)cdata, sizeof(*cdata));
        cdata->csr = csr;
	cdata->ctype = ctype;
	cdata->bigend = isbigend;

        /*
         * In order that we don't have 2 targets colliding on dma areas,
	 * cache align them.
         */
        target_dma_size = (sizeof(struct target_dma_data)+(dcache_line_size-1))
	                  & ~(dcache_line_size-1);
        script_size = (sizeof(SCRIPT)+(dcache_line_size-1))
	              & ~(dcache_line_size-1);
        dma_size = (script_size+(target_dma_size*MAX_SCSI_TARGETS)+
		    (page_roundup-1))
	           & ~(page_roundup-1);

        /* Allocate space for NCR53C700 specific dma data structures */
	if (kmem_alloc(kernel_map, (vm_offset_t *)&dma, dma_size)
	    != KERN_SUCCESS) {
                printf("ncr53c700_attach: unable to allocate controller dma area\n");
                return(0);
        }

        bzero((char *)dma, dma_size);
        /*
	 * Initialize the controller's copy of the script.
	 * XXX NOTE: script must be page aligned.
	 * target dma data are allocated at same time to ensure they
	 * are physically contiguous (This might fail is MAX_SCSI_TARGETS
	 * goes to 16 (dma_size will be larger than 1 page )
	 */
        cdata->script = (union instruction *)dma;
        bcopy((const char*)SCRIPT, (char*)cdata->script, sizeof(SCRIPT));
        cdata->script_phys = pmap_extract(vm_map_pmap(kernel_map),
					  (vm_offset_t)cdata->script);

#if NEISA > 0
        if (scp->bustype == BUSTYPE_EISA) {

                vm_offset_t ioaddr;

                /* map script and use mapped address */
                ioaddr = atbus_iomap_allocate(ATBUS_IOMAP_MASTER,
					      ATBUS_IOMAP_ANYWHERE,
					      dma_size/ATBUS_IOMAP_PAGE_SIZE);
                atbus_iomap_map(ioaddr, &cdata->script_phys, 1);
                cdata->inst_ioaddr = ioaddr | 
		                (cdata->script_phys & ATBUS_IOMAP_PAGE_MASK);
        } else
#endif
	{
                /* use physical addresses for SYSTEM bus */
                cdata->inst_ioaddr = cdata->script_phys;
        }
        
#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_INIT)
		printf("ncr53c700_attach: script ioaddr 0x%x\n", cdata->inst_ioaddr);
#endif

        s = (unsigned long *)cdata->script;

	/* Patch script for 720 to expect disconnects */
	if (cdata->ctype == CTYPE_720)
		for (ii = 0; ii < A_720_patch_size; ii++) {
			s[A_720_patch_Used[ii]-1] = MASK_SCNTL_INST;
		}

        /* Relocate script */
        for (ii = 0; ii < PATCHES; ii++)
                s[LABELPATCHES[ii]] += (unsigned long)cdata->inst_ioaddr;

        /* Byteswap all instructions */
        for (ii = 0; ii < INSTRUCTIONS; ii++) {
                cdata->script[ii].l[0] = BYTESWAP(cdata->script[ii].l[0]);
                cdata->script[ii].l[1] = BYTESWAP(cdata->script[ii].l[1]);
        }

        /* 
	 * input clock period, see manual. 
	 * Note funny arithmetic here to avoid overflow and integer truncation.
	 */

        /* Input clock period (nanoseconds) */ 
	cdata->tcp = (tcpl + 50)/100;

	/* Max sync data offset for chip */  
	cdata->maxoff = C700_MAX_OFFSET;    

	/* Min sync transfer period (ns) */                     
        cdata->minperiod = C700_MIN_TP_CLOCKS * cdata->tcp;

	if (cdata->ctype == CTYPE_710) {
		/* Cannot use XFERP 0 on (older?) 53C710 */
		if (scsi_clock_freq > 40000000)
			cdata->minperiod += cdata->tcp;
#if NEISA > 0
		/* Differential */
		if (scp->bustype == BUSTYPE_EISA)
			cdata->minperiod /= 2;
#endif
	} else if (cdata->ctype == CTYPE_720)
		cdata->minperiod /= 2;

	/* Cannot go faster than the bus allows */
	if (cdata->minperiod < bus_minperiod)
		cdata->minperiod = bus_minperiod;

	/* Max sync transfer period (ns) */
        cdata->maxperiod = C700_MAX_TP_CLOCKS * cdata->tcp;

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_INIT)
		printf("ncr53c700_attach: tcpl %d, tcp %d, min %d, max %d\n",
		       tcpl, cdata->tcp, cdata->minperiod, cdata->maxperiod);
#endif
        cdata->set_atn = FALSE;
        cdata->active = FALSE;
        cdata->dma.valid = FALSE;


        /* flush writes out to memory */
        pmap_flush_range(vm_map_pmap(kernel_map),
			 (vm_offset_t)cdata->script, dma_size);

	/* Enable interrupts on the system bus */
#if NEISA > 0
	if (scp->bustype != BUSTYPE_EISA)
#endif
	if (cdata->ctype == CTYPE_720)
		aspitab(INT_FWSCSI, SPLFWSCSI,
			(void (*)(int))ncr53c700_intr,
			(int)scp);
	else
		aspitab(INT_SCSI, SPLSCSI,
			(void (*)(int))ncr53c700_intr,
			(int)scp);


#ifdef	DEBUG
        /* allocate trace buffer */
        cdata->it_nentries = MAX(ITRACE_NENTRIES,
				 (((sizeof(struct itrace)*ITRACE_NENTRIES)+
				   (NBPG-1)) & ~(NBPG-1))/sizeof(struct itrace));
        if (kmem_alloc(kernel_map,
		       (vm_offset_t *)&cdata->it,
		       sizeof(struct itrace)*cdata->it_nentries) != KERN_SUCCESS)
		cdata->it = 0;
        if (!cdata->it) {
                printf("ncr53c700_attach: unable to allocate controller interrupt trace buffer\n");
        }
#endif
	/* hook to scsi interface */
	scsi_add(ctlrinfop->ctlrnum, scp);

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_INIT)
		printf("ncr53c700_attach: initialization done\n");
#endif
        return(1);
}

/*
 *      ncr53c700_probe - Test for c700.
 */
int
ncr53c700_probe(hp_ctlrinfo_t *ctlrinfop)
{
#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_probe(0x%x)\n", ctlrinfop);
#endif
	if (ctlrinfop->numcsrs != 1)
		panic("ncr53c700_probe: unexpected number of control/status register mappings");
	if (!ctlrinfop->vcsrs[0])
		panic("ncr53c700_probe: control registers not mapped");

#if NEISA > 0
	if (ctlrinfop->parent_ctlrinfop->ctlr_businfop->bustype
	    == BUSTYPE_EISA &&
	    ((union csr *)ctlrinfop->vcsrs[0])->eisa.id != 0x22f00c80)
                return(0);
#endif

/* XXX this should be elsewhere...*/
	return(ncr53c700_attach(ctlrinfop));
}

void	
scsi_add(
	 int unit,
	 struct scsi_controller *scp)
{
	scsi_softc_t	*sc;
	sc = scsi_master_alloc(unit, (char *)scp);
	scp->sc = sc;
	sc->go = ncr53c700_go;
	sc->watchdog = 0;
	sc->tgt_setup = (void (*)(target_info_t *))0;
	sc->probe = ncr53c700_probe_target;
	scp->wd.reset = (void (*)(struct watchdog_t *wd))ncr53c700_reset_scsibus;
	sc->max_dma_data = C700_MAXPHYS;
	sc->initiator_id = SCSI_INITIATOR; /* XXX where to get the info from ? */
	ncr53c700_probe_devices(scp);
}

/*
 * Allocate and initialize per target data structures
 */

boolean_t
ncr53c700_target_alloc(
	struct scsi_controller 	*scp,
	target_info_t		*tgt)
{
	struct 	scsi_cmd 	*cmd;
        struct  controller_data *cdata;
        struct target_data      *tdata;
	struct target_dma_data 	*tdma;
	unsigned		char *dma;
        
	assert(!tgt->cmd_ptr);

	if ((cmd = (struct scsi_cmd *)kalloc(sizeof(struct scsi_cmd))) == 0)
	  	goto nomem;
	bzero((char *)cmd, sizeof(struct scsi_cmd));
	cmd->tgt = tgt;
	scp->scsi_cmd[tgt->target_id] = cmd;
	if ((tgt->cmd_ptr = (char *)kalloc_aligned(SCSI_MAX_CMD_LEN, 
						   dcache_line_size)) == 0)
	  	goto nomem;
	cdata = scp->info;
	if ((tdata = (struct target_data *)kalloc(
					   sizeof(struct target_data))) == 0)
	  	goto nomem;
 	bzero((char *)tdata, sizeof(struct target_data));
	tdata->id = tgt->target_id;
        cdata->target_data[tgt->target_id] = tdata;

	/*
	 * target dma data must be physically contiguous and is 
	 * allocated at once at controller initialization
	 */
	dma = (unsigned char *)cdata->script;
	dma += cache_round(sizeof(SCRIPT));
	dma += (tgt->target_id)*cache_round(sizeof(struct target_dma_data));
	tdma = (struct target_dma_data *)dma;

        tdata->dma_data = tdma;
        tdma->msgi[0] = SCSI_NOP;
        tdma->msg[0] = SCSI_NOP;
        tdma->msg_size = 1;

	/* XXX force init for async */
	(void)ncr53c700_setsync(scp, tdata, 0, cdata->maxperiod+1);
	return TRUE;
nomem:
	printf("ncr53c700_target_alloc: no memory\n");
	ncr53c700_target_dealloc(scp, tgt);
	return(FALSE);
}

/*
 * Deallocate per target data structures
 */

void
ncr53c700_target_dealloc(
	struct scsi_controller 	*scp,
	target_info_t		*tgt)
{
        struct  controller_data *cdata;
        struct target_data      *tdata;

	if (tgt->cmd_ptr)
	  	kfree_aligned((vm_offset_t)tgt->cmd_ptr,
			      SCSI_MAX_CMD_LEN,
			      dcache_line_size);
	tgt->cmd_ptr = 0;
	if (scp->scsi_cmd[tgt->target_id])
		kfree((vm_offset_t)scp->scsi_cmd[tgt->target_id],
		      sizeof(struct scsi_cmd));
	scp->scsi_cmd[tgt->target_id] = 0;
	cdata = scp->info;
	if (tdata = cdata->target_data[tgt->target_id]) {
		kfree((vm_offset_t)tdata, sizeof(struct target_data));
		cdata->target_data[tgt->target_id] = 0;
	}
}

boolean_t
ncr53c700_probe_target(
	target_info_t		*tgt,
	io_req_t		ior)
{
	boolean_t		newlywed;
	struct scsi_controller 	*scp;
	struct scsi_cmd 	*cmd;
        struct controller_data  *cdata;
        union   csr             *csr;
	char			ret;	

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_probe_target()\n");
#endif
	scp = (struct scsi_controller *)tgt->hw_state;

	newlywed = (tgt->cmd_ptr == 0);
	if (newlywed) {
	  	if (!ncr53c700_target_alloc(scp, tgt))
			return FALSE;
	}

	cdata = scp->info;
        csr = scp->bus_addr;

	if (cdata->ctype == CTYPE_710)
		CLRBYTEREG(csr->rw.ctest7, C710_CTEST7_NOTIME);

	ret = scsi_inquiry(tgt, SCSI_INQ_STD_DATA);

	if (cdata->ctype == CTYPE_710)
		SETBYTEREG(csr->rw.ctest7, C710_CTEST7_NOTIME);

	if (ret == SCSI_RET_DEVICE_DOWN) {
	  	if (newlywed) {
			ncr53c700_target_dealloc(scp, tgt);
		}
		return FALSE;
	} 

	tgt->flags = TGT_ALIVE;
	return TRUE;
}


void
ncr53c700_reset_scsibus(
	struct scsi_controller *scp)
{
	printf("ncr53c700_reset_scsibus()\n");
}

/*
 * Start a SCSI command on a target
 */
void
ncr53c700_go(
	target_info_t		*tgt,
	unsigned int		cmd_count,
	unsigned int		in_count,
	boolean_t		cmd_only)
{
	vm_offset_t		virt;
	struct scsi_cmd		*cmd;
	struct scsi_controller *scp = (struct scsi_controller *)tgt->hw_state;
	boolean_t	passive = (tgt->ior && (tgt->ior->io_op & IO_PASSIVE));
	spl_t	s;

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_go()\n");
#endif
	tgt->transient_state.cmd_count = cmd_count;
	tgt->transient_state.in_count = in_count;
	if ((((tgt->cur_cmd == SCSI_CMD_WRITE) ||
	      (tgt->cur_cmd == SCSI_CMD_LONG_WRITE)) && !passive) ||
	    ((tgt->cur_cmd == SCSI_CMD_RECEIVE) && passive)) {
		io_req_t	ior = tgt->ior;
		register int	len = ior->io_count;

		tgt->transient_state.out_count = len;

	} else {
		tgt->transient_state.out_count = 0;
	}

	/*
	 * Setup tgt->transient_state.dma_offset
	 */
	tgt->done = SCSI_RET_IN_PROGRESS;

	switch (tgt->cur_cmd) {
	    case SCSI_CMD_READ:
	    case SCSI_CMD_LONG_READ:
	    case SCSI_CMD_WRITE:
	    case SCSI_CMD_LONG_WRITE:
		virt = (vm_offset_t)tgt->ior->io_data;
		break;
	    case SCSI_CMD_INQUIRY:
	    case SCSI_CMD_REQUEST_SENSE:
	    case SCSI_CMD_MODE_SENSE:
	    case SCSI_CMD_RECEIVE_DIAG_RESULTS:
	    case SCSI_CMD_READ_CAPACITY:
	    case SCSI_CMD_READ_BLOCK_LIMITS:
		virt = (vm_offset_t)tgt->cmd_ptr;
		break;
	    case SCSI_CMD_MODE_SELECT:
	    case SCSI_CMD_REASSIGN_BLOCKS:
	    case SCSI_CMD_FORMAT_UNIT:
		tgt->transient_state.cmd_count = sizeof(scsi_command_group_0);
		tgt->transient_state.out_count = cmd_count -
		                                 sizeof(scsi_command_group_0);
		virt = (vm_offset_t)tgt->cmd_ptr+sizeof(scsi_command_group_0);
		break;
	    default:
		virt = 0;
	}

	cmd = scp->scsi_cmd[tgt->target_id];
	cmd->data_addr = virt;
	cmd->data_len = cmd->tgt->transient_state.in_count;
	if (!cmd->data_len)
		cmd->data_len = cmd->tgt->transient_state.out_count;

	if (scp->wd.nactive++ == 0)
		scp->wd.watchdog_state = SCSI_WD_ACTIVE;


#ifdef USELEDS
	if (inledcontrol == 0)
		ledcontrol(0, 0, LED_DISK);
#endif

	s = splbio();

        ncr53c700_mkinst(scp, cmd);
	assert(!cmd->queued);
	cmd->queued = TRUE;
	ncr53c700_start(scp);

	splx(s);
}

void
ncr53c700_probe_devices(
		   struct scsi_controller *scp)
{
	scsi_softc_t	*sc;
	target_info_t	*tgt;
	int		target_id;
	struct io_req 	ior;
	spl_t		s;
	struct controller_data *cdata;

        cdata = scp->info;
	s = splhigh();	/* we need interrupts */
	spllo();
    	memset((char *)&ior, 0, sizeof(ior));
	simple_lock_init(&ior.io_req_lock, ETAP_IO_REQ);

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_ENTRY)
		printf("ncr53c700_probe_devices()\n");
#endif
	ior.io_next = 0;
	ior.io_count = 0;
	ior.io_op = IO_INTERNAL;
	ior.io_error = 0;
	sc = scp->sc;

	for (target_id = 0 ; target_id < MAX_SCSI_TARGETS; target_id++) {

		if (target_id ==  SCSI_INITIATOR)	/* done already */
			continue;
	
		/* clear any unit attentions */

#ifdef DEBUG
		if (ncr53c700_debug & C700_DEBUG_INIT)
			printf("probe target %d\n", target_id);
#endif	/* DEBUG */
		if (scsi_probe(sc, &tgt, target_id, 0, &ior))
			printf("\n");      
	}
	splx(s);
}

#if	DEBUG
char *SCNTL0_REG[8] = {"TRG", "AAP", "EPG", "EPC", "WATN", "START", "ARB0", "ARB1"};
char *SCNTL1_REG[8] = {"RCV", "SND", "PAR", "RST", "CON", "ESR", "ADB", "EXC"};
char *SCID_REG[8] = {"ID0", "ID1", "ID2", "ID3", "?", "SRE", "RRE", "?"};
char *SXFER_REG[8] = {"MO0", "MO1", "MO2", "MO3", "TP0", "TP1", "TP2", "DHP"};
char *DSTAT_REG[8] = {"IID", "WTD", "SIR", "SPI", "ABRT", "BF", "?", "DFE"};
char *DFIFO_REG[8] = {"BO0", "BO1", "BO2", "BO3", "BO4", "BO5", "BO6", "?"};
char *DIEN_REG[8] = {"IID", "WTD", "SIR", "SPI", "ABRT", "BF", "?", "?"};
char *DCNTL_REG[8] = {"RST", "?", "STD", "LLM", "SSM", "S16", "CF0", "CF1"};
char *SIEN_REG[8] = {"PAR", "RST", "UDC", "SGE", "SEL", "STO", "FCMP", "MA"};
char *SIEN0_REG[8] = {"PAR", "RST", "UDC", "SGE", "SEL", "STO", "FCMP", "MA"};
char *SIEN1_REG[8] = {"HTH", "GEN", "STO", "?", "?", "?", "?", "?"};
char *SOCL_REG[8] = {"IO", "CD", "MSG", "ATN", "SEL", "BSY", "ACK", "REQ"};
char *SIST0_REG[8] = {"PAR", "RST", "UDC", "SGE", "RSL", "SEL", "CMP", "M/A"};
char *SIST1_REG[8] = {"HTH", "GEN", "STO", "?", "?", "?", "?", "?"};
char *SSTAT0_REG[8] = {"PAR", "RST", "UDC", "SGE", "RSL", "SEL", "CMP", "M/A"};
char *SSTAT1_REG[8] = {"SDP", "RST", "WOA", "LOA", "AIP", "OLF", "ORF", "ILF"};
char *SSTAT2_REG[8] = {"IO", "CD", "MSG", "SDP", "FF0", "FF1", "FF2","FF3"};
char *ISTAT_REG[8] = {"DIP", "SIP", "PRE", "CON", "RES4", "SIGP", "RST", "ABRT"};


#define concatenate_2(x, y)	x##y
#define concatenate(x, y)	concatenate_2(x, y)

#define print_reg(a, b) _print_reg(a, # b, concatenate(b, _REG))

static	void	_print_reg(
			   unsigned char a,
			   char *name,
			   char **values);


void
_print_reg(
	   unsigned char a,
	   char *name,
	   char **values)
{
	register 	i;

	if (!a)
		return;
  	printf("%s:	0x%2x ", name, a);
	for (i=0; i<8; i++, a >>= 1, values++)
		if (a&1)
			printf("%s ", *values);
	printf("\n");
}
			
void
display_status(
	       struct controller_data  *cdata)
{
        union  csr  *csr;

	csr = cdata->csr;
	print_reg(READBYTEREG(csr->rw.scntl0), SCNTL0);
	print_reg(READBYTEREG(csr->rw.scntl1), SCNTL1);
	print_reg(READBYTEREG(csr->rw.scid), SCID);
	print_reg(READBYTEREG(csr->rw.sxfer), SXFER);
	printf("SFBR:	0x%x\n", READBYTEREG(csr->rw.sfbr));
	print_reg(READBYTEREG(csr->rw.dstat), DSTAT);
	print_reg(READBYTEREG(csr->rw.dfifo), DFIFO);
	printf("DCMD:	0x%x\n", READBYTEREG(csr->rw.d.d.dcmd));
	printf("DBC:	0x%x\n", BYTESWAP(csr->rw.d.dbc) & 0xffffff);
	printf("DNAD:	0x%x\n", BYTESWAP(csr->rw.dnad));
	printf("DSP:	0x%x\n", BYTESWAP(csr->rw.dsp));
	printf("DSPS:	0x%x\n", BYTESWAP(csr->rw.dsps));
	print_reg(READBYTEREG(csr->rw.dien), DIEN);
	printf("DWT:	0x%x\n", READBYTEREG(csr->rw.dwt));
	print_reg(READBYTEREG(csr->rw.dcntl), DCNTL);
	switch(cdata->ctype) {
	case CTYPE_700:
	case CTYPE_710:
		printf("SDID:	0x%x\n", READBYTEREG(csr->rw.dwt));
		print_reg(READBYTEREG(csr->rw.sien), SIEN);
		print_reg(READBYTEREG(csr->rw.socl), SOCL);
		print_reg(READBYTEREG(csr->rw.sstat0), SSTAT0);
		print_reg(READBYTEREG(csr->rw.sstat1), SSTAT1);
		print_reg(READBYTEREG(csr->rw.sstat2), SSTAT2);
		print_reg(READBYTEREG(csr->rw.istat), ISTAT);
		break;
	case CTYPE_720:
		printf("SDID:	0x%x\n", READBYTEREG(csr->c720.sdid));
		print_reg(READBYTEREG(csr->c720.sien0), SIEN0);
		print_reg(READBYTEREG(csr->c720.sien1), SIEN1);
		print_reg(READBYTEREG(csr->c720.socl), SOCL);
		print_reg(READBYTEREG(csr->c720.sist0), SIST0);
		print_reg(READBYTEREG(csr->c720.sist1), SIST1);
		print_reg(READBYTEREG(csr->rw.sstat0), SSTAT1);
		print_reg(READBYTEREG(csr->rw.sstat1), SSTAT2);
		print_reg(READBYTEREG(csr->c720.istat), ISTAT);
		break;
	}

}

#endif DEBUG


#if	hp_pa

/*
** PA architecture register bit definitions.
**
*/

#define IOSTATUS_RY             0x40
#define IOSTATUS_FE             0x80
#define IOIIDATA_SMINT5L        0x40000000
#define IOIIDATA_MINT5EN        0x20000000
#define IOIIDATA_PACKEN         0x10000000
#define IOIIDATA_PREFETCHEN     0x08000000
#define IOIIDATA_IOII           0x00000020

void
zalon_init(
	   struct controller_data  *cdata)
{
        union  csr  	*csr;
	struct iomod	*ior;
	extern struct iomod *prochpa;

#ifdef DEBUG
	if (ncr53c700_debug & C700_DEBUG_INIT)
		printf("zalon_init(cdata = %x)\n", cdata);
#endif
	csr = cdata->csr;
	ior = (struct iomod *)(((char *)csr)-ZALON_OFFSET);

	ior->io_eim = (unsigned int)prochpa | ffset(~SPLFWSCSI);
	ior->io_command = CMD_RESET;
	while (!(ior->io_status & IOSTATUS_RY))
		delay(25);

   	/*
	 * Configure IO_II_DATA register (enable mint5_en, pack_en and     
	 * prefetch_en) and return.  NOTE, CMD_RESET has already cleared
	 * the io_ii field (interrupts enable bit). 
	 */
	ior->io_ii_rw |= (IOIIDATA_MINT5EN | IOIIDATA_PACKEN |
			    IOIIDATA_PREFETCHEN);

	/* Enable ack on the NCR */
	
	WRITEBYTEREG(csr->rw.dcntl, C720_DCNTL_EA);

   	/*
	 * In addition I have to set parity the Even Host Parity Bit on NCR in
	 * CTEST0 register as GSC is even parity.
	 */

	WRITEBYTEREG(csr->c720.ctest0, C720_CTEST0_EHP);
	
   	/* 
	 *Now, set Host Bus Multiplex Mode Bit in NCR CTEST4 register.
	 */

   	WRITEBYTEREG(csr->c720.ctest4, C720_CTEST4_MUX | C720_CTEST4_EHPC);

	/* Zalon.  Must clear its interrupt if it is set too.
	 * This will clear the IO_II bit.  If it is set, it will be cleared 
	 * it is written back.  Otherwise, it will remain unset. 
	 */

	ior->io_ii_rw = ior->io_ii_rw;
	
}

#endif /* hp_pa */


vm_offset_t
kalloc_aligned(
	vm_size_t	size,
	vm_size_t	alignment)
{
  	vm_offset_t     *addr, ret, aligned;
	vm_size_t	round_size;

	round_size = (size+sizeof(addr)-1) & ~(sizeof(addr)-1); 
	size = round_size + sizeof(addr) + alignment;
	if ((ret = kalloc(size)) == 0)
		return(0);
	aligned = (ret + alignment - 1) & ~(alignment - 1);
	addr = (vm_offset_t *)(aligned+round_size);
	*addr = ret;
	return(aligned);
}

void
kfree_aligned(
	vm_offset_t	offset,
	vm_size_t	size,
	vm_size_t	alignment)
{
  	vm_offset_t     *addr, ret, aligned;
	vm_size_t	round_size;

	round_size = (size+sizeof(addr)-1) & ~(sizeof(addr)-1); 
	size = round_size + sizeof(addr) + alignment;
	addr = (vm_offset_t *)(offset+round_size);
	kfree(*addr, size);
}
