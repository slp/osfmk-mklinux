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
#include <kern/misc_protos.h>
#include <device/driver_lock.h>

/* defines for testing and timing */
/* #define	FDDITEST */

/* board id - manufacturer id + product id + release #  */
#define	 INP_SEAHAWK	0x25D00100

/* All registers are in the EISA slot specific address space */

/************************************************************************/
/* Configuration registers - from the hardware spec of Interphase */
/* references to THE book implies Motorola 68840 FDDI User's Manual */
/* Note board id and board control registers are in eisa_common.h */
/* these are 8 bit registers */

#define EISA_MEM_HIGH		0xC88
#define EISA_MEM_LOW		0xC8E

#define  FSI_A_PORT_ADDR	0xC91
#define  STATIC_RAM_ADDR	0xC93
#define  MAC_ELM_ADDR		0xC97

#define  FSI_A_PORT_MASK	0xC99
#define  STATIC_RAM_MASK	0xC9B
#define  MAC_ELM_MASK		0xC9F

#define  MEM_ADDR_DECODE	0xCA2
#define  FSI_A_PORT_DECODE	0xCA3
#define  STATIC_RAM_DECODE	0xCA3
#define  MAC_ELM_DECODE		0xCA6

#define	FACTORY_TEST_OUTPUT_LATCH	0xCA8
#define EISA_MASTER_BURST_ENABLE		0x08  

#define	FSI_MAC_ELM_INTERRUPT_CONFIG	0xCA9 
#define is_fsi_intr(sp)	(*(sp->hwaddr + FSI_MAC_ELM_INTERRUPT_CONFIG) & 0x80)

#define	TIMER_CPU_INTERRUPT_CONFIG	0xCAA
#define is_cpu_intr(sp)	(*(sp->hwaddr + TIMER_CPU_INTERRUPT_CONFIG) & 0x80)

#define ENABLE_EISA_INTERRUPT			0x08  
#define EISA_IRQ_ASSIGNMENT			0x03  

#define	MISCELLANEOUS_INPUT_PORT	0xCAC

#define	MISCELLANEOUS_OUTPUT_LATCH	0xCAD

#define	ADDRESS_DECODE_ENABLE		0xCAE

/************************************************************************/
/* FSI registers - 32 bit */
#define	FSI_B_PORT		0x000

/* The FSI Status Register 1  and associated values*/
#define	FSI_STATUS_REG1		0x004
#define	FSI_COMMAND_DONE		0x80000000
#define	FSI_FCR_FREE			0x40000000
#define	MAX_FCR_TRIES			100

/* Interrupt mask register */
#define	FSI_INTR_MASK1		0x014


#define	FSI_COMMAND_REG		0x024
#define	FSI_COMMAND_EXT_REG	0x02C
#define	MAX_CMR_TRIES			5

/* The FSI control register in used as a pointer to a bunch of other
   registers - see THE book Chapter 7 pages 16-36
*/
#define	FSI_CONTROL_REG		0x03C
/* the following are to initialize the FSI thru FCR. see THE book
	Appendix B.1  */
#define	RING_0	0x00000000
#define	RING_1	0x01000000
#define	RING_2	0x02000000
#define	RING_3	0x03000000
#define	RING_4	0x04000000
#define	RING_5	0x05000000
#define	RING_6	0x05000000
#define	RING_7	0x07000000

/* port A control register setup as 32 bit, no parity, big endian and
   the port is enabled */
#define	FCR_PCRA_INIT		0x0000015E
/* port B control register setup as 32 bit, no parity, little endian and
   the port is enabled */
#define	FCR_PCRB_INIT		0x0000115F
		
/* port A memory page with 16 bytes page size (see Interphase doc page 27) */
#define FCR_PMPA_INIT		0x00000046
/* port B memory page with 1K page size (see Interphase doc page 26) */
#define FCR_PMPB_INIT		0x00000F47

/* transmit commands to be read from port B */
#define FCR_CMD_PARAM_INIT	0x00001037

/* Four rings - rings 0 & 1 for transmit and 4 & 5 for receive */

/* ring 0,4 descriptors and data are on port B, no parity check.
   rings contain upto 8 entries. */
#define FCR_RING0_INIT		0x00003030
#define FCR_RING4_INIT		0x00003034
/* ring 1,5 descriptors and data are on port A, no parity check.
   rings contain upto 8 entries. */
#define FCR_RING1_INIT		0x00000031
#define FCR_RING5_INIT		0x00000035
/* ring 6 idescriptors and data are on port B, no parity check.
   rings contain upto 16 entries. This is used for commands */
/*
#define FCR_RING6_INIT		0x36340000
*/

/* FIFO watermarks. ring 4&5 have 32 blocks and rings 0&1 have 20 blocks.
   Each block is 32 bytes. */
#define FCR_RING0_FIFO		0x00001420
#define FCR_RING1_FIFO		0x00001421
#define FCR_RING4_FIFO		0x00002024
#define FCR_RING5_FIFO		0x00002520

/* receive frame type. ring 4 deals with LLC and ring 5 with SMT */
#define FCR_RING4_RCV_TYPE	0x0000180C
#define FCR_RING5_RCV_TYPE	0x0000020D

/* maximum receive memory space - 2K for rings 4 & 5 */
#define FCR_RING4_MAX_RCV	0x00004064
#define FCR_RING5_MAX_RCV	0x00004065

/* MACIF transmit control - enable transmit */
#define MACIF_TRANSMIT_ENABLE	0x00000108

/* MACIF receive control - enable receive for rings 4 & 5 */
#define MACIF_RECEIVE_ENABLE	0x00000309

/* Ring Ready */
#define FCR_RING_READY		0x00000000
#define FCR_RING_READY0		FCR_RING_READY|RING_0
#define FCR_RING_READY1		FCR_RING_READY|RING_1
#define FCR_RING_READY4		FCR_RING_READY|RING_4

/* Ring status */
#define FCR_RING_STATUS		0xF0000000
#define FCR_RING_STATUS0	FCR_RING_STATUS|RING_0
#define FCR_RING_STATUS1	FCR_RING_STATUS|RING_1
#define FCR_RING_STATUS4	FCR_RING_STATUS|RING_4

/************************************************************************/
/*  MAC registers - 16 bit */

#define	MAC_CTRL_A		0x400
#define	MAC_ON				0x0080
#define	MAC_CTRL_B		0x402

#define	SHORT_ADDRESS 		0x420
#define	LONG_ADDRESS_A 		0x422
#define	LONG_ADDRESS_B 		0x424
#define	LONG_ADDRESS_C 		0x426

#define	REQ_TTRT		0x428 	
#define	REQ_TTRT_VALUE			0x3CFF 	
#define	INIT_MAX_TTRT		0x42A
#define	INIT_MAX_TTRT_VALUE		0xE080

#define	FRAME_COUNT		0x440
#define	LOST_COUNT		0x442

#define	INT_EVENT_A		0x444
#define	INT_EVENT_B		0x446
#define	INT_EVENT_C		0x426

#define	RECEIVE_STATUS 		0x448
#define	TRANSMIT_STATUS 	0x44A

/************************************************************************/
/* ELM registers - 16 bit */
#define	ELM_CTRL_A		0x480
#define	SAS_CTRL_A_VALUE		0x0000
#define	ELM_CTRL_B		0x482
#define	SAS_CTRL_B_VALUE		0x8000
#define	ELM_INTR_MASK		0x484
#define	ELM_INTR_MASK_VALUE		0x6400

#define	PCM_ACQ_MAX		0x48C
#define	PCM_ACQ_MAX_VALUE		0xF6FF /* 0.2 MS */
#define	PCM_LSCHANGE_MAX	0x48E
#define	PCM_LSCHANGE_MAX_VALUE		0xFFFF /* 0.025 MS */
#define	PCM_BREAK_MIN		0x490
#define	PCM_BREAK_MIN_VALUE		0x0CFF /* 5 MS */
#define	PCM_SIG_TIMEOUT		0x492
#define	PCM_SIG_TIMEOUT_VALUE		0xEDEC /* 100 MS */
#define	PCM_LC_SHORT		0x496
#define	PCM_LC_SHORT_VALUE		0x76F6 /* 50 MS */
#define	PCM_SCRUB		0x498
#define	PCM_SCRUB_VALUE			0x55FF /* 3.5 MS */
#define	PCM_NOISE		0x49A
#define	PCM_NOISE_VALUE			0x22F0 /* 1.3 MS */

#define ELM_INT_EVENT		0x4AE
#define ELM_VIOLATE_SYM		0x4B0
#define	ELM_MIN_IDLE		0x4B2
#define	ELM_LINK_ERROR		0x4B4

/************************************************************************/
/* timer registers */

/************************************************************************/
/* Nonvolatile memory I/O map */
/* 	zC00 - zC7F  128 bytes  R/W    User storage area */
/* 	zC80 - zC83  4   bytes  Read   EISA Bus identification */
/* 	zC84 - zCBF  60  bytes  R/W    EISA configuration registers */
/* 	zCC0 - zCC3  4   bytes  None   Factory write EISA Bus id */
/* 	zCC4 - zCC9  6   bytes  Read   Factory Node id */
/* 	zCCA - zCFF  54  bytes  None   Reserved */
#define	SH4811_NODE_ADDR	0xCC4

/************************************************************************/

typedef struct {
	unsigned short	flags;
	unsigned short	size; 	/* size of buffer */
	void	*addr;	/* addr of buffer */
} sh4811_descriptor;

struct sh4811_stats {
        unsigned long txt; /* successful transmits */
        unsigned long rcv; /* successful receives */
        unsigned long txtsz; /* size of packets transmitted */
        unsigned long rcvsz; /* size of packets received */
        unsigned short txterr; /* errors in transmit */
        unsigned short rcverr; /* errors in receive */
#ifdef  FDDITEST
        unsigned long txttime; /* time for packets transmitted */
#endif
} ;

/* These defines refer to the number of entries in each descriptor ring */
#define	NUM_TXT_LLC	1
#define	NUM_TXT_SMT	1
#define	NUM_RCV_LLC	1
#define	NUM_RCV_SMT	1

/* define ring related */
#define	DEFINE_RCV_RING		0xC0000000
/**/
#define	FSI4KPAGE		0x00000800
/**/
/* 8K pages */
/**
#define	FSI4KPAGE		0x00001000
**/
#define	DEFINE_NORMAL_RING	0xBE000000
#define	RING_READY_STATE	0x00080000

#define FDDI_ADDR_SIZE 6

typedef struct {
        struct  ifnet	ds_if;          /* generic interface header */
	int		seated;
	int		configured;
	int		status;
	unsigned char	slot;		/* EISA slot */
	unsigned char	irq;	
        unsigned char	noncan_mac_addr[FDDI_ADDR_SIZE]; 
        unsigned char	can_mac_addr[FDDI_ADDR_SIZE]; 
        unsigned char	*hwaddr;	/* pointer to hpa */
        int     	flags;
	/* board/host communication thru' shared memory */
	int		sram_paddr;	/* physical address of sram */
        unsigned char	*sram_vaddr;	/* virtual address of sram */
	sram_header	sram_info;
	unsigned int	indication;	/* RC indication waited for */

        queue_head_t    multicast_queue;/* multicast hardware addr list */
	
	unsigned int 	H2Bread;
	unsigned int 	H2Bwrite;
	unsigned char	*H2Bchan_paddr;
	unsigned char	*H2Bchan_vaddr;

	unsigned int 	B2Hread;
	unsigned int 	B2Hwrite;
	unsigned char	*B2Hchan_paddr;
	unsigned char	*B2Hchan_vaddr;
	
	ring_hdr	txt_ring; /* transmit ring descriptor header */
	ring_hdr	rcv_ring; /* receive ring descriptor header */
	list_hdr	txt_list; /* transmit buffer list header */
	list_hdr	rcv_list; /* receive  buffer list header */
	unsigned char	rcvind[RC_SEG_LEN];
	net_pool_t  	rcv_msg_pool; /* pool rcv_kmsg allocated from */

	char		*txt_hdr; /* list of free txt headers */
	char		*txt_hdr_mem; /* address of txt headers */
	unsigned	txt_hdr_map; /* EISA mapped address of txt headers */

	int     timer;
	int     tbusy;
	struct  sh4811_stats	stats; /* board statistics */
	unsigned int		callback_missed; /* receive buffer missed */
	struct net_callback	callback; /* receive buffer callback */
	driver_lock_decl(,lock) /* driver lock */
} sh4811_softc_t;

#define FDDI_HEADER_SIZE        16
#define SH_NUM_DESC             16
#define FDDI_MAX_BUFSIZ      (4096+1024)

struct seahawk_rcv_msg {
        mach_msg_header_t msg_hdr;
        NDR_record_t NDR;
        char            header[NET_HDW_HDR_MAX];
        mach_msg_type_number_t packetCnt;
        char            packet[FDDI_MAX_BUFSIZ];
        mach_msg_format_0_trailer_t trailer;
};

typedef struct seahawk_rcv_msg *seahawk_rcv_msg_t ;
#define sh4811_net_kmsg(kmsg)          ((seahawk_rcv_msg_t)&(kmsg)->ikm_header)
#ifdef	oldpool
#define sh4811_net_kmsg(kmsg)          net_kmsg(kmsg)
#endif

/************************************************************************/

extern int  sh4811output(dev_t dev, io_req_t ior);
extern int  sh4811_txt(sh4811_softc_t *sp, io_req_t ior);
extern int  iphase_sram_info(sh4811_softc_t *sp);
extern int  iphase_board_init(sh4811_softc_t *sp);
extern int  sh4811_board_init(sh4811_softc_t *sp);
extern void sh4811intr(int unit);
extern int  sh4811Fintr(sh4811_softc_t *sp);
extern int  sh4811Cintr(sh4811_softc_t *sp);
extern void sh4811_create_rings(sh4811_softc_t *sp);
extern int  sh4811_alloc_rings(sh4811_softc_t *sp);
extern int  process_reply(sh4811_softc_t *sp, unsigned char *source, 
		unsigned char *dest, unsigned int len);


extern void iphase_set_intr(sh4811_softc_t *sp);
extern void iphase_req_stat(sh4811_softc_t *sp);
extern void sh4811_callback_receive(int unit);
extern int  sh4811_setup_receive(sh4811_softc_t *sp, int numdesc);
extern void sh4811_get_addr(sh4811_softc_t *sp);
extern void sh4811_connect_ring(sh4811_softc_t *sp, int flag);
extern void sh4811_trace_smt(sh4811_softc_t *sp, int flag);
extern void  sh4811_set_cam(sh4811_softc_t *sp, unsigned char *cam_addr,
					unsigned int flag);
extern int  sh4811_rc_intr(sh4811_softc_t *sp);
extern int  sh4811_fsi_intr(sh4811_softc_t *sp);
extern int  link_cmd_offset(sh4811_softc_t *sp, int len);
extern void iphase_board_close(sh4811_softc_t *sp);
extern void sh4811_free_resources(sh4811_softc_t *sp);

extern void dump_SRAMcmd(sh4811_softc_t *sp);
extern void dump_SRAMrcv(sh4811_softc_t *sp);
extern int  fsi_cr_cmd(sh4811_softc_t *sp, unsigned int command);
extern int  fsi_cr_read(sh4811_softc_t *sp, unsigned int command);
extern int  fsi_command(sh4811_softc_t *sp, int cmr, int cer);
extern int  dump_fddi_frame(ipc_kmsg_t kmsg, int len);

extern int  endof_transmit(sh4811_softc_t *sp);
extern int  fddi_pkt_rcv(sh4811_softc_t *sp, unsigned int status);
extern int  wait_for_end(sh4811_softc_t *sp);

#ifdef  FDDITEST
extern int sh4811_test_send(sh4811_softc_t *sp);
extern void sh4811FN(void);
extern int mfctl(int);
#endif
#endif
