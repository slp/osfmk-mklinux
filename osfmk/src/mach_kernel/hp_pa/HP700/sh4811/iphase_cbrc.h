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

/* byte swap macros */
#define	swap_long(to, from) { 					\
		unsigned int _i = *(unsigned int *)(from);	\
		*(to) = (((_i & 0xFF000000) >> 24)|	 	\
			 ((_i & 0x00FF0000) >> 8) |	 	\
			 ((_i & 0x0000FF00) << 8) |		\
			 ((_i & 0x000000FF) << 24));		\
}

#define short_swap(in, out) ((out) = ((((in) & 0xFF00) >> 8) |	\
				      (((in) & 0x00FF) << 8)))

#define int_swap(in, out) ((out) = (((in) & 0xFF000000) >> 24)|	\
				   (((in) & 0x00FF0000) >> 8) | \
				   (((in) & 0x0000FF00) << 8) | \
				   (((in) & 0x000000FF) << 24))


/* Boot definitions */
#define MAKE_LONG(a,b,c,d)      ((d << 24) | (c << 16) | (b << 8) | a)

/* common boot commands */
#define CB_BOOT			MAKE_LONG('B','O','O','T')
#define CB_DIAG                 MAKE_LONG('D','I','A','G')
#define CB_FILL                 MAKE_LONG('F','I','L','L')
#define CB_FLSH                 MAKE_LONG('F','L','S','H')
#define CB_GRNL                 MAKE_LONG('G','R','N','L')
#define CB_JUMP                 MAKE_LONG('J','U','M','P')
#define CB_PEEK                 MAKE_LONG('P','E','E','K')
#define CB_POKE                 MAKE_LONG('P','O','K','E')
#define CB_REDL                 MAKE_LONG('R','E','D','L')
#define CB_STUF                 MAKE_LONG('S','T','U','F')

/* common boot command status */
#define CB_OK			MAKE_LONG('C','B','O','K')
#define CB_FAIL			MAKE_LONG('F','A','I','L')


/* RC requests and directives */
/* requests have responses. directives do not */
/* H2B - host to board. B2H - board to host */
#define	RC_SET_DMA_BURST	0x1 /* directive to set DMA burst */
#define	RC_SET_BUS_TIMEOUT	0x2 /* directive to set bus timeout */
#define	RC_GET_STAT		0x3 /* req to get statistics */
#define	RC_SET_INTR		0x4 /* directive to set interrupts*/
#define	RC_H2B_CREATE		0x5 /* req to create H2B channel */
#define	RC_B2H_CREATE		0x6 /* req to create B2H channel */
#define	RC_H2B_LINK		0x7 /* directive to link H2B channel */
#define	RC_ALLOC_B2H_SEG	0x9 /* directive to set B2H segments */

/* RC responses and indications */
#define	RC_GET_STATRESP		RC_GET_STAT+128 
#define	RC_LINK_SEG_IND		0x86

/* Miscellaneous defines */

#define	IPHASE_MAX_LOOPS	3
#define	MAX_RC_CMD_SIZE		32
#define	MAX_RC_REPLY_SIZE	32

#define	TRANSMIT_CHAN		0	/* host to board */
#define	RCV_CHAN		1	/* board to host */

/* layout of onboard shared memory */
#define	B2H_INTR_ACK		6
#define	H2B_CHAN_OFFSET		16
#define	B2H_CHAN_OFFSET		32
#define	RC_SEG_LEN		512
#define	RC_COUNTERS		4
#define	H2B_SEG_OFFSET		RC_SEG_LEN+(RC_COUNTERS*sizeof(int))
#define RC_SEG_PER_CHAN		2 /* number of segments per channel */
#define	B2H_SEG_OFFSET		H2B_SEG_OFFSET+(RC_SEG_PER_CHAN * RC_SEG_LEN)


/* read & write pointer manipulation */
#define	SIZE_LINK_SEG_DIRECTIVE		20
#define	SIZE_LINK_SEG_INDICATION	16
#define	H2B_cmd_offset(sp, len)	(				\
		((sp->H2Bwrite+len+SIZE_LINK_SEG_DIRECTIVE) < RC_SEG_LEN) ? \
			sp->H2Bwrite : link_cmd_offset(sp,len)  \
)

#define	B2H_interrupt_timer(sp, timer)	\
	(int_swap(*(volatile unsigned int *)((sp)->sram_vaddr+B2H_CHAN_OFFSET), \
		timer))
#define	B2H_reply_offset(sp)	(sp->B2Hread)
#define	B2H_board_offset(sp, offset)	\
	(int_swap(*(volatile unsigned int *)((sp)->sram_vaddr+B2H_CHAN_OFFSET+8), \
		offset))
#define	B2H_get_reply_len(addr, replen)	\
	(int_swap((((ccb *)addr)->cmd_len), replen))

#define	H2B_offset_update(sp, len) {			\
	unsigned int swap_len;				\
	unsigned long oldpri;				\
	unsigned int tmp;			\
							\
        oldpri = splimp();                              \
	sp->H2Bwrite += len;				\
	tmp = sh4811_hostseg|sp->H2Bwrite;		\
	int_swap(tmp, swap_len);	\
	*(volatile unsigned int *)(sp->sram_vaddr+H2B_CHAN_OFFSET+8) \
		= swap_len; 				\
	splx(oldpri);					\
}

#define	B2H_read_update(sp, boardseg, len) {	\
	unsigned int swap_len;			\
	unsigned int tmp;			\
						\
	sp->B2Hread += len;			\
	tmp = boardseg|sp->B2Hread;		\
	int_swap(tmp, swap_len);		\
	*(volatile unsigned int *)(sp->sram_vaddr+B2H_CHAN_OFFSET+4) = swap_len;  \
}

/* structures used */

typedef struct {
	unsigned short	size; /* shared memory size */
	unsigned char	cb_cmd_start; /* offset where commands are issued */
#define	CB_HERALD	0xCB
	unsigned char	signature;
	unsigned short	reserved;
} sram_header;

typedef	struct {
	unsigned int cmd_len;	/* command length */
	unsigned int cmd_id;	/* command id */
} ccb;

/*
** channels are used to communicate between the host and the board 
** There are two channels - host_to_board(H2B) and board_to_host(B2H).
** H2B write pointers are updated by the host and B2H write pointers 
** by the board and inversely for the read pointers.
** each channel needs to have a minimum of 2 segments.
** we have thus 2 segments for H2B and 2 segments for B2H 
*/

typedef	struct {
	unsigned short reserved1;
	unsigned short intr_timer;	/* interrupt timer */
	unsigned short rd_seg;		/* read segment (0 or 1) */
	unsigned short rd_off;		/* read offset */
	unsigned short wr_seg;		/* write segment (0 or 1) */
	unsigned short wr_off;		/* write offset */
	unsigned int reserved2;	
} rc_channel_hdr;

typedef	struct {
	unsigned int tag;
	unsigned char firmware_id[16];
	unsigned char rel_date[16];
	unsigned int buffersize;
	unsigned int sramsize;
} rc_stat_resp;


/* adding a receive buffer descriptor */
#define	INC_WRITE_PTR(ring)			\
	(ring).write++;				\
	if((ring).write == (ring).stop) {	\
		(ring).write=(ring).start;	\
	}

#define	INC_READ_PTR(ring)			\
	(ring).read++;				\
	if((ring).read == (ring).stop) {	\
		(ring).read = (ring).start;	\
	}

#define	NEXT_READ_PTR(ring)			\
	((ring).read + 1 == (ring).stop ? (ring).start : (ring).read + 1)

#define ADD_RCV_BUF(rcv_ring, rcv_list, net_buf, pkt_addr, io_addr) { \
	unsigned int _tmp;					\
								\
	rcv_list.write->buf_ptr = (unsigned char *)net_buf;     \
	rcv_list.write->io_ptr = (unsigned char *)pkt_addr;     \
	INC_WRITE_PTR(rcv_list);				\
	_tmp=(unsigned int)io_addr;				\
	eisa_arb_disable();					\
	pdcache(HP700_SID_KERNEL, 				\
		(vm_offset_t)rcv_ring.write, sizeof (fsi_cmd));	\
	int_swap(_tmp, rcv_ring.write->cer);			\
	_tmp=(DEFINE_RCV_RING|FSI4KPAGE);			\
	int_swap(_tmp, rcv_ring.write->cmr);			\
	fdcache(HP700_SID_KERNEL, 				\
		(vm_offset_t)rcv_ring.write, sizeof (fsi_cmd));	\
	eisa_arb_enable();					\
	INC_WRITE_PTR(rcv_ring);				\
}


#define ADD_TXT_BUF(txt_ring, txt_list, net_buf, ior, cmd, io_addr, nbytes) { \
	unsigned int _tmp;					\
								\
	txt_list.write->buf_ptr = (unsigned char *)ior;		\
	txt_list.write->io_ptr = (unsigned char *)net_buf;	\
	txt_list.write->size = nbytes;				\
	INC_WRITE_PTR(txt_list);				\
	_tmp=(unsigned int)io_addr;			        \
	pdcache(HP700_SID_KERNEL, 				\
		(vm_offset_t)txt_ring.write, sizeof(fsi_cmd));	\
	int_swap(_tmp, txt_ring.write->cer);			\
	int_swap(cmd, txt_ring.write->cmr);			\
	fdcache(HP700_SID_KERNEL, 				\
		(vm_offset_t)txt_ring.write, sizeof(fsi_cmd));	\
	INC_WRITE_PTR(txt_ring);				\
}

/* debug routine */
#ifdef	DEBUG
extern int fddidebug;
#define	FDDIDEBUG_INTR	0x0001
#define	FDDIDEBUG_EOT	0x0002
#define	FDDIDEBUG_RCV	0x0004
#define	FDDIDEBUG_START	0x0008
#define	FDDIDEBUG_INFO	0x0010
#define	FDDIDEBUG_DUMP	0x0020
#define	FDDIDEBUG_STAT	0x0040
#define	FDDIDEBUG_DEBUG	0x0080
#define FDDIPRINTF(level, args)	{	\
	if (fddidebug & (level)) 	\
		printf args;		\
}
#else
#define FDDIPRINTF(level, args)
#endif

#define	FSICMD_PER_PKT	3	/* Maximum number of FSI cmd per txt packet */

/*
 * Prototypes.
 */
extern int iphase_cmd_ok(volatile unsigned int *addr);
extern void copy_swap_long(unsigned char *source, unsigned char *dest, int nbytes);
extern void sh4811_copyhdr(struct fddi_header *dst, struct fddi_header *src);
#endif
