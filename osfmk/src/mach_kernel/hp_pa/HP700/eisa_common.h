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

#ifndef	_MACHINE_EISA_COMMON_H_
#define	_MACHINE_EISA_COMMON_H_

#include <machine/iomod.h>

/* EISA IO map addresses for HP workstations */
#define	EISA_BASE_ADDR		0xFC000000 
#define	EISA_IOMAP_ADDR		0xFC100000 
#define	EISA_MONGOOSE_REG	0xFC010000 
#define	EISA_INTR_ACK		0xFC01F000 

#define	EISA_WAX_LOCK		0xFC010001
#define	EISA_WAX_FIFO		0xFC011001
#define	EISA_WAX_BUS		0xFC012001
#define	EISA_WAX_CONVERTER	0xFC01E001

/***************************************************************************/
/* some useful EISA defines */
#define	MAX_EISA_SLOTS		8 
#define	EISA_SLOT_SIZE		0x1000 

#define EISA_BOARD_ID		0xc80
#define	EISA_BOARD_CONTROL	0xc84

/***************************************************************************/
/* EISA system board initializations */

/* interrupts */
/* taken from EISA spec and the values of HP specific values from
   HPUX  source code */
/* INT1 is the master interrupt controller and INT2 the slave interrupt
controller */

/* Interrupt related */
#define	EISA_INT1_BASE		0x20
#define	EISA_INT1_MASK		0x21
#define	EISA_INT2_BASE		0xA0
#define	EISA_INT2_MASK		0xA1
#define	EISA_INT1_CONTROL	0x4D0
#define	EISA_INT2_CONTROL	0x4D1


#define	EISA_NMI_STATUS		0x61
#define	EISA_NMI_ENABLE		0x70

/* values to be loaded into registers */
#define	ICW1			0x10	
#define	ICW4_NEEDED		0x01	
#define	ICW2_MASTER_VECTOR	0x00	
#define	ICW2_SLAVE_VECTOR	0x08
#define	ICW3_MASTER_DEVICE	0x04	
#define	ICW3_SLAVE_DEVICE	0x02
#define	ICW4_MODE		0x11 /* special fully nested mode , 
					 no automatic EOI */
#define	OCW3_READ_ISR		0x0B

#define	EISA_DISABLE_INT	0xFF

#define	EISA_BUS_RESET		0x01
#define	DISABLE_PARITY_IOCHK	0xC0
#define	ENABLE_IOCHK		0x40

#define	ENABLE_NMI		0x00

#define	NONSPECIFIC_EOI		0x20
#define	NOOP_EOI		0x40
#define	SPECIFIC_EOI		0x60

/* Timer related */
#define	EISA_TIMER1_CONTROL	0x43
#define	EISA_TIMER2_CONTROL	0x4B
	/* values */
#define DISABLE_TIMER        	0x10
#define SYSTEM_COUNTER 		0x0
#define REFRESH_COUNTER 	0x40

/***************************************************************************/
/* DMA init */
/* 8 DMA channels 0-3 on DMA 1 and 4-7 on DMA 2 */
#define DMA1_CHANNEL0_ADDR	0x0000
#define DMA1_CHANNEL0_COUNT	0x0001
#define	DMA1_COMMAND		0x0008	/* write only */
#define	DMA1_STATUS		0x0008	/* read only */
#define	DMA1_REQUEST		0x0009
#define	DMA1_MASK_ONE		0x000A
#define	DMA1_MODE		0x000B
#define	DMA1_MASTER_CLEAR	0x000D
#define	DMA1_MASK_ALL		0x000F
#define	DMA1_CHAINING_MODE	0x040A
#define	DMA1_EXTENDED_MODE	0x040B

#define DMA1_CHANNEL4_ADDR	0x00C0
#define DMA1_CHANNEL4_COUNT	0x00C2
#define	DMA2_COMMAND		0x00D0	/* write only */
#define	DMA2_STATUS		0x00D0	/* read only */
#define	DMA2_REQUEST		0x00D2
#define	DMA2_MASK_ONE		0x00D4
#define	DMA2_MODE		0x00D6
#define	DMA2_MASTER_CLEAR	0x00DA
#define	DMA2_MASK_ALL		0x00DE
#define	DMA2_CHAINING_MODE	0x04D4
#define	DMA2_EXTENDED_MODE	0x04D6

#define	SELECT_CHANNEL_0	0x0
#define	SELECT_CHANNEL_4	SELECT_CHANNEL_0
#define	SELECT_CHANNEL_1	0x1
#define	SELECT_CHANNEL_5	SELECT_CHANNEL_1
#define	SELECT_CHANNEL_2	0x2
#define	SELECT_CHANNEL_6	SELECT_CHANNEL_2
#define	SELECT_CHANNEL_3	0x3
#define	SELECT_CHANNEL_7	SELECT_CHANNEL_3

#define	STOP_DISABLED		0x80
#define	CASCADE_MODE		0xC0

#define	DMA_DEMAND		0x00
#define	DMA_BURST		0x30
#define	DMA_32BIT		0x08


#define	MASK_CHANNEL_4		0x1


/***************************************************************************/

extern int      geteisa_boardid(struct iomod *hpa, int init);
extern void	eisa_init(void);
extern void	eisa_init_regs(void);
extern void	eisa_intr_init(void);
extern void	init_intr_ctlr(void);
extern void	eisa_dma_init(void);
extern int 	eisa_irq_in_use(int irq);
extern void 	eisa_irq_register(int unit, int irq, void (*handler)(int));
extern void 	eisa_enable_intr(void);
extern void	eisaintr(void);
extern void	eisanmi(void);
extern void 	eisa_irq_enable(int irq);
extern void 	eisa_irq_disable(int irq);
extern boolean_t eisa_eoi(int irq);
extern unsigned int map_host_to_eisa(void *vaddr, int size, int *npages);
extern unsigned int eisa_page_offset(int paddr, boolean_t *found);
extern void 	unmap_host_from_eisa(void *vaddr, int size);
extern int  	map_eisa_to_host(int paddr);
extern void	dump_eisa_map(void);
extern void	eisa_arb_enable(void);
extern void	eisa_arb_disable(void);
extern void	wax_eisa_arb_enable(void);
extern void	wax_eisa_arb_disable(void);
extern void	viper_eisa_arb_enable(void);
extern void	viper_eisa_arb_disable(void);

#endif /*_MACHINE_EISA_COMMON_H_ */
