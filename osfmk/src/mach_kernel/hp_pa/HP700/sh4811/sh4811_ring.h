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

#include <fddi.h>
#if	NFDDI > 0

/* defines */

/* FSI status register 1 */
/* error statuses */
#define FSTAT_HER		0x20000000
#define FSTAT_IOE		0x10000000
#define FSTAT_ROV5		0x08000000
#define FSTAT_ROV4		0x04000000
#define FSTAT_POEB		0x02000000
#define FSTAT_POEA		0x01000000
#define FSTAT_CIN		0x00800000
#define FSTAT_STE		0x00400000
#define FSTAT_RER5		0x00200000
#define FSTAT_RER4		0x00100000
#define FSTAT_RER3		0x00080000
#define FSTAT_RER2		0x00040000
#define FSTAT_RER1		0x00020000
#define FSTAT_RER0		0x00010000
#define FSTAT_RXC5		0x00002000
#define FSTAT_RXC4		0x00001000
#define FSTAT_RCC1		0x00000200
#define FSTAT_RNR4		0x00000010
#define FSTAT_RNR1		0x00000002

/* FSI interrupt mask register 1 */

#define FSI_HEE		0x20000000
#define FSI_IEE		0x10000000
#define FSI_RVE4	0x04000000
#define FSI_PEEA	0x01000000
#define FSI_REE4	0x00100000
#define FSI_REE1	0x00020000
#define FSI_REE0	0x00010000
#define FSI_RXE4	0x00001000
#define FSI_RCE1	0x00000200
#define FSI_RCE0	0x00000100
#define FSI_RNR4	0x00000010
#define FSI_RNR1	0x00000002

#define FSI_INT_MASK   ( FSI_HEE  | FSI_IEE  | FSI_RVE4 | FSI_PEEA | \
                         FSI_REE4 | FSI_REE1 | FSI_RXE4 | FSI_RCE1 )

/* ring buffer descriptor and indication */
#define	HOST_OWNED	0x80000000
#define	VALID_FRAME	0x40000000
#define	FIRST_FRAME	0x20000000
#define	LAST_FRAME	0x10000000
#define	ERROR_FRAME	0x04000000
#define	CRC_ERROR_FRAME	0x01000000

/* from the receive indication */
#define	LENGTH_FROM_IND(cmr)	(cmr & 0x00001FFF)	


/* get mac address */
#define	FSI_GET_ADDR_REQ		260
#define	FSI_GET_ADDR_DIR		261
#define	FDDI_RING_CONNECT		262
#define	FDDI_SET_CAM_REQ		266
#define	FDDI_CLEAR_CAM_REQ		267
#define	FDDI_FLUSH_CAM_REQ		268
#define	FDDI_TRACE_DIR			269

#define	FSI_GET_ADDR_RESP		FSI_GET_ADDR_REQ+128
#define	FDDI_RING_INDICATION		FSI_GET_ADDR_RESP+1
#define	FDDI_SET_CAM_RESP		FSI_GET_ADDR_RESP+3
#define	FDDI_TRACE_IND			FSI_GET_ADDR_RESP+5

/* ring create and response */
#define	FSI_CREATE_RING_REQ		1025
#define	FSI_CREATE_RING_RESP		FSI_CREATE_RING_REQ+128


/* ring control */
#define	SH4811_RING_CONNECT	1
#define	SH4811_RING_DISCONNECT	2

/* trace directive values */
#define	SH4811_TRACE_ON		3
#define	SH4811_TRACE_OFF	4


/* structures used */

typedef	struct {
	ccb	cmd;
	unsigned int response_id;
	unsigned int tag;
	unsigned int ratio;
	unsigned int sync_txt_size;
	unsigned int sync_txt_addr;
	unsigned int async_txt_size;
	unsigned int async_txt_addr;
	unsigned int llc_rcv_size;
	unsigned int llc_rcv_addr;
} create_ring;

typedef	struct {
	unsigned int tag;
	unsigned int sync_txt_status;
	unsigned int sync_txt_size;
	unsigned int async_txt_status;
	unsigned int async_txt_size;
	unsigned int llc_rcv_status;
	unsigned int llc_rcv_size;
} create_ring_resp;
	
typedef	struct {
	unsigned int cmr; /* command register for FSI */
	unsigned int cer; /* command extension register for FSI */
} fsi_cmd;

typedef	struct {
	fsi_cmd *start;
	fsi_cmd *read;
	fsi_cmd *write;
	fsi_cmd *stop;
} ring_hdr;

typedef	struct {
	unsigned char *buf_ptr; /* ptr to buffer - net_rcv_msg */
	unsigned char *io_ptr;  /* iobus addr of data in above */
	int size; 		/* size of pkt sent */
	int status; 		/* mapped/unmapped, allocated/free */
} fsi_list;

typedef	struct {
	fsi_list *start;
	fsi_list *read;
	fsi_list *write;
	fsi_list *stop;
} list_hdr;

/*
 * Prototypes.
 */

#endif
