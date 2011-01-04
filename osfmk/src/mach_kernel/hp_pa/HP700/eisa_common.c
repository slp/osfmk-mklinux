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

#include <eisaha.h>
#if	 NEISAHA > 0
#include <types.h>
#include <mach/vm_param.h>
#include <machine/asp.h>
#include <kern/spl.h>
#include <hp_pa/pmap.h>

#include <kern/misc_protos.h>
#include <hp_pa/trap.h>
#include <hp_pa/psw.h>
#include <hp_pa/viper.h>
#include <hp_pa/c_support.h>
#include <hp_pa/HP700/eisa_common.h>

/* structure to store mapping of eisa bus on HP machines */
struct mongoose_reg {
        unsigned char pad;
        volatile unsigned char bus_lock;
        unsigned char liowait;
        unsigned char bus_speed;
};

struct eisabus {
	volatile unsigned char *base_addr;
	volatile unsigned char *iomap_addr;
	volatile unsigned char *intr_ack;
	volatile struct mongoose_reg *mongoose_regs;
	void 	(*eisa_arb_enable)(void);
	void 	(*eisa_arb_disable)(void);
} ;

struct eisabus eisa_bus;

/* This array is used to store & check the IRQs currently in use */
struct eisa_irq {
	int	unit;
	int	irqno;
	void 	(*handler)(int);
} eisa_irqs[MAX_EISA_SLOTS];
static int eisa_index=0;

int
geteisa_boardid(struct iomod *hpa, int init)
{
	register unsigned char first_byte;
	register unsigned int id;
	volatile unsigned char *addr;

	addr = (unsigned char *)hpa+EISA_BOARD_ID;
        /* prime the bus line capacitors */
	if(init)
        	*addr = 0xff;

        /* attempt to read the id */
        first_byte = *addr;

        /* see if they responded with a not-ready */
        if (init && (first_byte & 0xf0) == 0x70) {
                /* wait for 1 millisecond for card to respond */
                delay(1000);

                first_byte = *addr;

                /* if still not ready, bail out */
                if ((first_byte & 0xf0) == 0x70)
                        return(0);
        }

        /* bail out if we read bus line values or MSB is 1 */
        if (first_byte == 0xff || first_byte & 0x80)
                return(0);

        /* construct the board id - most significant byte first */
        id = *addr++ << 24;
        id += *addr++ << 16;
        id += *addr++ << 8;
        id += *addr;

        return(id);
}

void
eisa_init(void)
{
	extern  int waxix;

	/* initialize eisa bus structure */
	eisa_init_regs();

	/* initialize mongoose registers */
	eisa_bus.mongoose_regs->liowait=1;	/* no liowait delays */
	eisa_bus.mongoose_regs->bus_lock=1;	/* unlock bus */

	/* reset system bus */
	*(eisa_bus.base_addr+EISA_NMI_STATUS) = EISA_BUS_RESET;
	delay(1000);	/* hold reset */
	*(eisa_bus.base_addr+EISA_NMI_STATUS) = 0;
	delay(1000);

	eisa_intr_init();
	init_intr_ctlr();

	eisa_dma_init();

	if(waxix >= 0) {
		eisa_bus.eisa_arb_enable=wax_eisa_arb_enable;
		eisa_bus.eisa_arb_disable=wax_eisa_arb_disable;
        	waxitab(INT_EISA, SPLEISA, (void (*)(int))eisaintr, 0);
		*((volatile unsigned char *)EISA_WAX_FIFO) = 1; /* enable FIFO*/
	}
	else {
		eisa_bus.eisa_arb_enable=viper_eisa_arb_enable;
		eisa_bus.eisa_arb_disable=viper_eisa_arb_disable;
        	aspitab(INT_EISA, SPLEISA, (void (*)(int))eisaintr, 0);
        	aspitab(INT_EISA_NMI, SPLEISA, (void (*)(int))eisanmi, 0);
	}
	eisa_arb_enable();
	eisa_enable_intr();
}

void 
eisa_init_regs(void)
{
        eisa_bus.base_addr=(volatile unsigned char *)EISA_BASE_ADDR;
        eisa_bus.iomap_addr=(unsigned char *)EISA_IOMAP_ADDR;
        eisa_bus.mongoose_regs=(struct mongoose_reg *)EISA_MONGOOSE_REG;
        eisa_bus.intr_ack=(unsigned char *)EISA_INTR_ACK;
}

void
eisa_intr_init(void)
{
	/* init interrupt controller 1 - master */ 
	*(eisa_bus.base_addr+EISA_INT1_BASE)=ICW1|ICW4_NEEDED;
	*(eisa_bus.base_addr+EISA_INT1_BASE+1)=ICW2_MASTER_VECTOR;
	*(eisa_bus.base_addr+EISA_INT1_BASE+1)=ICW3_MASTER_DEVICE;
	*(eisa_bus.base_addr+EISA_INT1_BASE+1)=ICW4_MODE;
	*(eisa_bus.base_addr+EISA_INT1_MASK)=EISA_DISABLE_INT;

	/* init interrupt controller 2 - slave */ 
	*(eisa_bus.base_addr+EISA_INT2_BASE)=ICW1|ICW4_NEEDED;
	*(eisa_bus.base_addr+EISA_INT2_BASE+1)=ICW2_SLAVE_VECTOR;
	*(eisa_bus.base_addr+EISA_INT2_BASE+1)=ICW3_SLAVE_DEVICE;
	*(eisa_bus.base_addr+EISA_INT2_BASE+1)=ICW4_MODE;
	*(eisa_bus.base_addr+EISA_INT2_MASK)=EISA_DISABLE_INT;

	*(eisa_bus.base_addr+EISA_INT1_CONTROL)=0;
	*(eisa_bus.base_addr+EISA_INT2_CONTROL)=0;

	/* intialize NMI - enable IOCHK */
	*(eisa_bus.base_addr+EISA_NMI_STATUS)=DISABLE_PARITY_IOCHK;
	*(eisa_bus.base_addr+EISA_NMI_STATUS)=ENABLE_IOCHK;
	*(eisa_bus.base_addr+EISA_NMI_ENABLE)=0x08/*ENABLE_NMI*/;


}

void
init_intr_ctlr()
{
	int hit=0,i=0;
	volatile unsigned char *int1addr=eisa_bus.base_addr+EISA_INT1_BASE;
	unsigned int irq;

	/* read iack reg until we get three 7's */
	while(hit < 3) {
                irq = *eisa_bus.intr_ack;
                if (irq == 7) hit++;
	        /* perform eoi */
		(void)eisa_eoi(irq);
		*int1addr = NONSPECIFIC_EOI;
                if (i++ >= 100) {
                        printf("EISA: Bad EISA Interrupt value %d\n", irq);
			break;
		}
	}
#ifdef  DEBUG
	printf("EISA Interrupt Hardware initialized after %d tries\n", i);	
#endif
}

void
eisa_dma_init(void)
{
	/* dma channels 0 to 3 */
	*(eisa_bus.base_addr+DMA1_COMMAND)=0;
	*(eisa_bus.base_addr+DMA1_MASTER_CLEAR)=0;

	*(eisa_bus.base_addr+DMA1_MODE)=SELECT_CHANNEL_0;
	*(eisa_bus.base_addr+DMA1_MODE)=SELECT_CHANNEL_1;
	*(eisa_bus.base_addr+DMA1_MODE)=SELECT_CHANNEL_2;
	*(eisa_bus.base_addr+DMA1_MODE)=SELECT_CHANNEL_3;

	*(eisa_bus.base_addr+DMA1_EXTENDED_MODE)=SELECT_CHANNEL_0|STOP_DISABLED;
	*(eisa_bus.base_addr+DMA1_EXTENDED_MODE)=SELECT_CHANNEL_1|STOP_DISABLED;
	*(eisa_bus.base_addr+DMA1_EXTENDED_MODE)=SELECT_CHANNEL_2|STOP_DISABLED;
	*(eisa_bus.base_addr+DMA1_EXTENDED_MODE)=SELECT_CHANNEL_3|STOP_DISABLED;

	*(eisa_bus.base_addr+DMA1_MASK_ALL)=0xFF; /* mask all channels */

	*(eisa_bus.base_addr+DMA1_CHAINING_MODE)=SELECT_CHANNEL_0;
	*(eisa_bus.base_addr+DMA1_CHAINING_MODE)=SELECT_CHANNEL_1;
	*(eisa_bus.base_addr+DMA1_CHAINING_MODE)=SELECT_CHANNEL_2;
	*(eisa_bus.base_addr+DMA1_CHAINING_MODE)=SELECT_CHANNEL_3;

	/* dma channels 4 to 7 */
	*(eisa_bus.base_addr+DMA2_COMMAND)=0;
	*(eisa_bus.base_addr+DMA2_MASTER_CLEAR)=0;

	*(eisa_bus.base_addr+DMA2_MODE)=SELECT_CHANNEL_4|CASCADE_MODE;
	*(eisa_bus.base_addr+DMA2_MODE)=SELECT_CHANNEL_5;
	*(eisa_bus.base_addr+DMA2_MODE)=SELECT_CHANNEL_6;
	*(eisa_bus.base_addr+DMA2_MODE)=SELECT_CHANNEL_7;

	*(eisa_bus.base_addr+DMA2_EXTENDED_MODE)=SELECT_CHANNEL_4|STOP_DISABLED;
	*(eisa_bus.base_addr+DMA2_EXTENDED_MODE)=SELECT_CHANNEL_5|STOP_DISABLED;
	*(eisa_bus.base_addr+DMA2_EXTENDED_MODE)=SELECT_CHANNEL_6|STOP_DISABLED;
	*(eisa_bus.base_addr+DMA2_EXTENDED_MODE)=SELECT_CHANNEL_7|STOP_DISABLED;

	*(eisa_bus.base_addr+DMA2_MASK_ALL) &= ~MASK_CHANNEL_4;	/* unmask channel 4 */

	*(eisa_bus.base_addr+DMA2_CHAINING_MODE)=SELECT_CHANNEL_4;
	*(eisa_bus.base_addr+DMA2_CHAINING_MODE)=SELECT_CHANNEL_5;
	*(eisa_bus.base_addr+DMA2_CHAINING_MODE)=SELECT_CHANNEL_6;
	*(eisa_bus.base_addr+DMA2_CHAINING_MODE)=SELECT_CHANNEL_7;
}

int
eisa_irq_in_use(int irq)
{
	register int i;
	
	for(i=0; i<MAX_EISA_SLOTS; i++)
		if(eisa_irqs[i].irqno == irq)
			return(irq);
	return(0);
}

void
eisa_irq_register(int unit, int irq, void (*handler)(int))
{
	if(eisa_index >= MAX_EISA_SLOTS)
		panic("Too many EISA boards");
#ifdef	DEBUG
	printf("Registering eisa irq %d for unit %d\n", irq, eisa_index);
#endif
	eisa_irqs[eisa_index].unit = unit;
	eisa_irqs[eisa_index].irqno = irq;
	eisa_irqs[eisa_index].handler = handler;
	eisa_index++;
}

void
eisa_enable_intr(void)
{
	register int i, irq, tmpint;
	unsigned short intrmask;
	
	intrmask=0xFFFB;
	for(i=0; i<MAX_EISA_SLOTS; i++) {
		irq=eisa_irqs[i].irqno;
		if(!irq)
			continue;
		tmpint = irq & 0xF;
		intrmask &= ~(1 << tmpint);
	}
	*(eisa_bus.base_addr+EISA_INT1_MASK)=intrmask & 0xFF;
	*(eisa_bus.base_addr+EISA_INT2_MASK)=intrmask >> 8;
#ifdef	DEBUG
	printf("interrupt mask 1 - %x, 2 - %x\n", 
			intrmask & 0xFF, intrmask >> 8);
#endif
}


static int
getirqindex(int irq)
{
	register int i;
	
	for(i=0; i<MAX_EISA_SLOTS; i++)
		if(eisa_irqs[i].irqno == irq)
			return(i);
	return(-1);
}

void
eisanmi()
{
	printf("In eisanmi\n");
}

void
eisaintr()
{
	int irq, index;
        volatile unsigned char *intaddr;
	spl_t s; 

	do {
		switch (irq = *eisa_bus.intr_ack) {
		default:
			break;
		case 7:
			intaddr = eisa_bus.base_addr + EISA_INT1_BASE;
			*intaddr = OCW3_READ_ISR;
			if ((*intaddr & 0x80) == 0)
                    		return;
			break;
		case 15:
			intaddr = eisa_bus.base_addr + EISA_INT2_BASE;
			*intaddr = OCW3_READ_ISR;
			if ((*intaddr & 0x80) == 0)
                    		return;
			break;
		}
		if((index = getirqindex(irq)) < 0) {
#ifdef	DEBUG
		    printf("Invalid eisa intr %x\n", irq);
#endif
		    return;
		}
		/* call handler */
		s = splimp();
		eisa_irqs[index].handler(eisa_irqs[index].unit);	
		splx(s);

		/* perform eoi */
	} while (eisa_eoi(irq));
}

void
eisa_irq_enable(int irq)
{
        if(irq >= 8 && irq <= 15)
		*(eisa_bus.base_addr+EISA_INT2_MASK) &= ~(1 << (irq-8));
        else
		*(eisa_bus.base_addr+EISA_INT1_MASK) &= ~(1 << irq);
}

void
eisa_irq_disable(int irq)
{
        if(irq >= 8 && irq <= 15)
		*(eisa_bus.base_addr+EISA_INT2_MASK) |= (1 << (irq-8));
        else
		*(eisa_bus.base_addr+EISA_INT1_MASK) |= (1 << irq);
}

boolean_t
eisa_eoi(int irq)
{
	volatile unsigned char *maddr, *saddr;

	maddr=eisa_bus.base_addr+EISA_INT1_BASE;
	saddr=eisa_bus.base_addr+EISA_INT2_BASE;

	if(irq >= 8 && irq <= 15) { /* slave */
		/* specific eoi to slave */
		*saddr = NOOP_EOI | ((irq - 8) & 0x7);
		*saddr = SPECIFIC_EOI | ((irq - 8) & 0x7);
		/* if no more interrupts, nonspecific eoi to master */
		*saddr = OCW3_READ_ISR;
		if(*saddr)
			return (TRUE);

		/* eoi to master */
		*maddr = NOOP_EOI | ICW3_SLAVE_DEVICE;
		*maddr = SPECIFIC_EOI | ICW3_SLAVE_DEVICE;
	}
	else {
		/* specific eoi to master */
		*maddr = NOOP_EOI | (irq & 0x7);
		*maddr = SPECIFIC_EOI | (irq & 0x7);
	}
	return (FALSE);
}

/* pointers to last mapped entries in eisa space */
static int eisa_mapped_pages=0;

#define	MAX_EISA_MAP	256
struct eisa_map {
	int	paddr;
	short	refcount;
} eisa_map[MAX_EISA_MAP];

/* 
	These routines map/unmap host memory for EISA board access.
*/

unsigned int 
map_host_to_eisa(void *vaddr, int size, int *mapbytes)
{
	unsigned int paddr=0;
	unsigned int epage=0;
	unsigned int left_this_page=0;
	boolean_t	found;
	volatile unsigned int eisaoff=(eisa_bus.iomap_addr - eisa_bus.base_addr);
	volatile unsigned int *eisa_iomap_addr=
				(volatile unsigned int *)eisa_bus.iomap_addr; 

	if(eisa_mapped_pages >= MAX_EISA_MAP-1) {
		panic("Out of EISA map space");
	}
	left_this_page = PAGE_SIZE - ((int)vaddr & PAGE_MASK);
	if(size <= left_this_page)
		*mapbytes=size;
	else
		*mapbytes = left_this_page;
	
	paddr = kvtophys((vm_offset_t)vaddr) >> PAGE_SHIFT;
	epage = eisa_page_offset(paddr, &found);
	eisaoff += epage*PAGE_SIZE;
	if(found)  {
		eisa_map[epage].refcount++;
		return(eisaoff+((int)vaddr & PAGE_MASK));
	}

	eisa_iomap_addr += ((epage*PAGE_SIZE)/sizeof(int));
	*eisa_iomap_addr = paddr; 

	eisa_map[epage].paddr=paddr;
	eisa_map[epage].refcount=1;
	eisa_mapped_pages++;

	return(eisaoff+((int)vaddr & PAGE_MASK));
}

unsigned int
eisa_page_offset(register int paddr, boolean_t *found)
{
	register int i, hole=-1;
	register struct eisa_map *emap;

	*found=FALSE;
	for(i=0,emap=&eisa_map[0];i<MAX_EISA_MAP;i++, emap++) {
		if(emap->paddr == paddr) {
			*found=TRUE;
			return((unsigned int)i);
		}
		if(hole < 0 && !emap->paddr)
			hole=i;
	}
	return((unsigned int)hole);
}

void
unmap_host_from_eisa(void *vaddr, int size)
{
	register struct eisa_map *emap;
	unsigned int paddr;
        unsigned int offset;
	boolean_t found;
	int mapped_bytes;

	paddr = kvtophys((vm_offset_t)vaddr) >> PAGE_SHIFT;
#ifdef	DEBUG2
	printf("unmap - vaddr %x, paddr %x,  ", vaddr, paddr);
#endif

        offset = eisa_page_offset(paddr, &found);
#ifdef	DEBUG2
	printf("offset %x, ref %x\n", offset, eisa_map[offset].refcount);
#endif
	if(found) {
		emap=&eisa_map[offset];
		emap->refcount--;
		if(emap->refcount <= 0) {
			emap->paddr=emap->refcount=0;
			eisa_mapped_pages--;
		}
	} else
	    printf("unmap_host_from_eisa: Unknown map\n");
	mapped_bytes = PAGE_SIZE - ((unsigned int)vaddr & PAGE_MASK);
	vaddr = (void *)((unsigned int)vaddr + mapped_bytes);
	if(size >  mapped_bytes) 
		unmap_host_from_eisa(vaddr, size-mapped_bytes);
}

void
dump_eisa_map()
{
	volatile unsigned int *eisa_addr=(volatile unsigned int *)
						eisa_bus.iomap_addr; 
	register int i;

	printf("EISA IOMAP - ");
	for(i=0;i<eisa_mapped_pages;i++) {
		printf("%x %x, %x - ", eisa_addr, *eisa_addr, 
				eisa_map[i].paddr);
		eisa_addr += (PAGE_SIZE/sizeof(int));
	}
	printf("\n");
}

int
map_eisa_to_host(int paddr)
{
	return((int)(eisa_bus.base_addr + paddr));
}

void
eisa_arb_enable()
{
	eisa_bus.eisa_arb_enable();
}

void
eisa_arb_disable()
{
	eisa_bus.eisa_arb_disable();
}

void
wax_eisa_arb_enable()
{
	eisa_bus.mongoose_regs->bus_lock=1;	/* unlock bus */
}

void
wax_eisa_arb_disable()
{
	volatile int tmp;
 	eisa_bus.mongoose_regs->bus_lock=0;       /* lock bus */
	tmp = *(volatile unsigned int *)eisa_bus.iomap_addr;
	*((volatile unsigned char *)EISA_WAX_FIFO) = 1; /* flush FIFO */
}

#define	VIPER_CONTROL_COPY	0xDC
#define	EISA_ARBITRATION	0x01

void
viper_eisa_arb_enable()
{
	spl_t s = splhigh();
	int psw = rsm(PSW_D);
	int tmp;

	*(int *)VIPER_CONTROL_COPY &= ~EISA_ARBITRATION;
	VI_TRS->vi_control = *(int *)VIPER_CONTROL_COPY;
	tmp = VI_TRS->vi_control;
	if (psw & PSW_D)
		ssm(PSW_D);
	splx(s);
}

void
viper_eisa_arb_disable()
{
	spl_t s = splhigh();
	int psw = rsm(PSW_D);
	int tmp;

	*(int *)VIPER_CONTROL_COPY |= EISA_ARBITRATION;
	VI_TRS->vi_control = *(int *)VIPER_CONTROL_COPY;
	tmp = VI_TRS->vi_control;
	if (psw & PSW_D)
		ssm(PSW_D);
	splx(s);
}
#endif	/* NEISAHA > 0 */
