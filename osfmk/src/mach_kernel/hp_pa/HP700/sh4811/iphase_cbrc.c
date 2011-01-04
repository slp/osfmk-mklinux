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
#include <types.h>
#include <mach/boolean.h>
#include <mach/mach_types.h>

#include <kern/spl.h>
#include <mach/machine/vm_types.h>
#include <mach/vm_param.h>
#include <kern/zalloc.h>
#include <hp_pa/pmap.h>
#include <device/if_hdr.h>
#include <device/io_req.h>
#include <device/net_io.h>
#include <device/if_fddi.h>
#include <device/driver_lock.h>

#include <hp_pa/HP700/eisa_common.h>
#include <hp_pa/HP700/sh4811/iphase_cbrc.h>
#include <hp_pa/HP700/sh4811/sh4811_ring.h>
#include <hp_pa/HP700/sh4811/if_sh4811.h>

#include <device/if_fddi.h>
#include <ipc/ipc_kmsg.h>


static 	int sh4811_hostseg=0;
#ifdef DEBUG
int	fddidebug;
#endif

int
iphase_sram_info(sh4811_softc_t *sp)
{
	register int loopcount=IPHASE_MAX_LOOPS;
	volatile register int *iaddr;

	iaddr=(int *)sp->sram_vaddr;
	*iaddr;
	while(loopcount && ((*iaddr & 0x000000FF) != CB_HERALD)) {
		delay(100);
		--loopcount;
		*iaddr;
	}
	if(!loopcount) 
		return(0);
	bcopy((char *)iaddr, (char *)&sp->sram_info, sizeof(int));

	sp->sram_info.cb_cmd_start *= 4; /* offset is in longwords */
	FDDIPRINTF(FDDIDEBUG_INFO,
		   ("SRAM at addr %x - size %x, command offset %x, sign %x\n",
		    iaddr,
		    sp->sram_info.size, 
		    sp->sram_info.cb_cmd_start,
		    sp->sram_info.signature));

	return(iphase_cmd_ok((volatile unsigned int *)(sp->sram_vaddr + sp->sram_info.cb_cmd_start)));
}
	
int
iphase_cmd_ok(volatile unsigned int *addr)
{
	register int loopcount=IPHASE_MAX_LOOPS;
	register int i;

	while(loopcount && *addr != CB_OK) {
		FDDIPRINTF(FDDIDEBUG_INFO,
			   ("CSW at addr %x is %x\n", addr, *addr));
		delay(1);
		--loopcount;
	}
	return(loopcount);
}

int
iphase_board_init(sh4811_softc_t *sp)
{
	unsigned char boot_cmd_str[128]; 
	volatile unsigned char *addr;
	register int len=0;
	ccb	*cmd;
	volatile unsigned int *iaddr;

	bzero((char *)boot_cmd_str, (vm_size_t)128);
	addr=boot_cmd_str+sizeof(int); /* leave space for "len" */
	
	/* allocate the board to host segment memory directive */
	cmd = (ccb *)addr;
	cmd->cmd_len=sizeof(ccb)+(6 * sizeof(int));
	cmd->cmd_id=RC_ALLOC_B2H_SEG;
	addr += sizeof(ccb);
	iaddr=(unsigned int *)addr;
	*iaddr++ = 0; /* transfer options */
	*iaddr++ = RC_SEG_LEN; /* segment len */

	/* physical and virtual address of first B2H segment */
	sp->B2Hchan_paddr = (unsigned char *)(sp->sram_paddr + B2H_SEG_OFFSET);
	sp->B2Hchan_vaddr = sp->sram_vaddr + B2H_SEG_OFFSET;
	*iaddr++ = (unsigned int)sp->B2Hchan_paddr;
	*iaddr++ = (unsigned int)sp->B2Hchan_vaddr;
	/* physical and virtual address of second B2H segment */
	*iaddr++ = (unsigned int)sp->B2Hchan_paddr + RC_SEG_LEN;
	*iaddr++ = (unsigned int)(sp->B2Hchan_vaddr + RC_SEG_LEN);
	len += cmd->cmd_len;
	addr=(unsigned char *)iaddr;
	
	/* create the board to host channel */
	cmd = (ccb *)addr;
	cmd->cmd_len=sizeof(ccb)+(2*sizeof(int));
	cmd->cmd_id=RC_B2H_CREATE;
	addr += sizeof(ccb);

	iaddr=(unsigned int *)addr;
	*iaddr++ = 0; /* response channel id */
	iaddr++; /* command tag */
	len += cmd->cmd_len;
	addr=(unsigned char *)iaddr;

	/* create the host to board channel */
	cmd = (ccb *)addr;
	cmd->cmd_len=sizeof(ccb)+(5*sizeof(int));
	cmd->cmd_id=RC_H2B_CREATE;
	addr += sizeof(ccb);
	iaddr=(unsigned int *)addr;
	*iaddr++ = 0; /* response channel id */
	iaddr++; /* command tag */
	*iaddr++ = 0; /* transfer options */
	sp->H2Bchan_paddr = (unsigned char *)(sp->sram_paddr + H2B_SEG_OFFSET);
	sp->H2Bchan_vaddr = sp->sram_vaddr + H2B_SEG_OFFSET;
	*iaddr++ = (unsigned int)sp->H2Bchan_paddr; /* phys addr of first H2B segment */
	*iaddr++ = RC_SEG_LEN; /* length of first H2B segment */

	addr=(unsigned char *)iaddr;
	len += cmd->cmd_len;
	/* stuff "len" into first word of boot_cmd_str */
	*(unsigned int *)boot_cmd_str=len;  

	/* do byte swapping */
	iaddr=(volatile unsigned int *)(sp->sram_vaddr + sp->sram_info.cb_cmd_start);
	copy_swap_long(boot_cmd_str, (unsigned char *)(iaddr+2), 
			len+sizeof(int));

	FDDIPRINTF(FDDIDEBUG_INFO,
		   ("board_init - booting the interface %x, cmdlen %d\n", 
		    sp->sram_vaddr + sp->sram_info.cb_cmd_start, len));

	/* boot the RC interface */
	*(iaddr+1)=0;  
	*iaddr=CB_BOOT;

	sp->H2Bread = sp->H2Bwrite = sp->B2Hread = sp->B2Hwrite = 0;
	*(int *)(sp->sram_vaddr+4)=0;
	delay(1000000);
	/*dump_SRAMcmd(sp);*/
	
	return(iphase_cmd_ok((volatile unsigned int *)(sp->sram_vaddr + sp->sram_info.cb_cmd_start)));
}

void
iphase_set_intr(sh4811_softc_t *sp)
{
	short len, offset;
	ccb	*cmd;
	volatile unsigned char *addr;
	volatile unsigned int *iaddr;
	unsigned int intr_timer, tmval=0xF;
	unsigned char rc_cmd_str[MAX_RC_CMD_SIZE]; 

	int_swap(tmval, intr_timer);
        *(volatile unsigned int *)(sp->sram_vaddr+B2H_CHAN_OFFSET) = intr_timer;

	bzero((char *)rc_cmd_str, MAX_RC_CMD_SIZE);
	addr=rc_cmd_str;

	cmd = (ccb *)addr;
	len=cmd->cmd_len=sizeof(ccb)+(3 * sizeof(int));
	cmd->cmd_id=RC_SET_INTR;
	addr+=sizeof(ccb);
	iaddr=(volatile unsigned int *)addr;
	*iaddr++=B2H_CHAN_OFFSET;	/* channel id */
	*iaddr++=sp->irq;	/* interrupt level */
	*iaddr++=0;		/* interrupt vector */

	offset=H2B_cmd_offset(sp,len);
	FDDIPRINTF(FDDIDEBUG_INFO, ("board set interrupt - "));
	FDDIPRINTF(FDDIDEBUG_INFO, ("addr %x, offset %x, len %x\n",
			       sp->H2Bchan_vaddr, offset, len));
	copy_swap_long(rc_cmd_str, sp->H2Bchan_vaddr+offset, len); 
	H2B_offset_update(sp, len);

        sp->configured = TRUE; /* inform interrupt handler we are ready */
	/* acknowledge interrupt */
	*(volatile short *)(sp->sram_vaddr+6)=0;
}

void
iphase_req_stat(sh4811_softc_t *sp)
{
	short len, offset;
	ccb	*cmd;
	volatile unsigned char *addr;
	volatile unsigned int *iaddr;
	unsigned char rc_cmd_str[MAX_RC_CMD_SIZE]; 

	bzero((char *)rc_cmd_str, MAX_RC_CMD_SIZE);
	addr=rc_cmd_str;

	cmd = (ccb *)addr;
	len = cmd->cmd_len=sizeof(ccb)+(2 * sizeof(int));
	cmd->cmd_id=RC_GET_STAT;
	addr+=sizeof(ccb);
	iaddr=(unsigned int *)addr;
	*iaddr++=B2H_CHAN_OFFSET;	/* channel id */
	*iaddr++=(unsigned int)sp;

	offset=H2B_cmd_offset(sp,len);
	FDDIPRINTF(FDDIDEBUG_INFO, ("board request statistics - "));
	FDDIPRINTF(FDDIDEBUG_INFO, ("addr %x, offset %x, len %x\n",
				    sp->H2Bchan_vaddr, offset, len));
	copy_swap_long(rc_cmd_str, sp->H2Bchan_vaddr+offset, len); 
	H2B_offset_update(sp, len);
}


int
link_cmd_offset(sh4811_softc_t *sp, int len)
{
	short linklen, offset;
	ccb	*cmd;
	volatile unsigned char *addr;
	volatile unsigned int *iaddr;
	unsigned char rc_cmd_str[MAX_RC_CMD_SIZE]; 


	/* writing FFFF to offset tells the board that we are
	   moving onto next segment */
	*(volatile unsigned short *)(sp->sram_vaddr+H2B_CHAN_OFFSET+10) = 0xFFFF;
	
        /* link the host to board segment */
	bzero((char *)rc_cmd_str, MAX_RC_CMD_SIZE);
	addr=rc_cmd_str;
        cmd = (ccb *)addr;
        linklen =cmd->cmd_len=sizeof(ccb)+(3*sizeof(int));
        cmd->cmd_id=RC_H2B_LINK;
        addr += sizeof(ccb);
        iaddr=(volatile unsigned int *)addr;
        *iaddr++ = 0; /* transfer options */
	if(!sh4811_hostseg) {
        	sp->H2Bchan_paddr = (unsigned char *)(sp->sram_paddr +
						H2B_SEG_OFFSET+RC_SEG_LEN);
        	sp->H2Bchan_vaddr = sp->sram_vaddr+H2B_SEG_OFFSET+RC_SEG_LEN;
	}
	else {
        	sp->H2Bchan_paddr = (unsigned char *)(sp->sram_paddr + 
							H2B_SEG_OFFSET);
        	sp->H2Bchan_vaddr = sp->sram_vaddr + H2B_SEG_OFFSET;
	}
        *iaddr++ = (unsigned int)sp->H2Bchan_paddr; /* phys addr of current H2B segment */
        *iaddr++ = RC_SEG_LEN; /* length of current H2B segment */

	offset = sp->H2Bwrite;
	FDDIPRINTF(FDDIDEBUG_INFO, ("link host-to-board segment - "));
	FDDIPRINTF(FDDIDEBUG_INFO, ("addr %x, offset %x, len %x\n", 
				    sp->H2Bchan_vaddr, offset, linklen));
	copy_swap_long(rc_cmd_str, sp->H2Bchan_vaddr+offset, linklen); 
	/* intialize host seg and write offset in next segment */
	if(!sh4811_hostseg)
		sh4811_hostseg=1<<sizeof(short);
	else
		sh4811_hostseg=0;
	sp->H2Bwrite = 0;
	H2B_offset_update(sp, 0);
	return(0);
}

zone_t	sh4811_ring_zone;

int
sh4811_alloc_rings(sh4811_softc_t *sp)
{
	char *zptr;
	int i, nbytes;

	/* allocate memory for transmit and receive rings */

/*
	sh4811_ring_zone = zinit(PAGE_SIZE, PAGE_SIZE, PAGE_SIZE, 
				"sh4811_zone");
	zptr = (char *)zalloc(sh4811_ring_zone);
*/
	zptr = (char *)kalloc(PAGE_SIZE);
	bzero(zptr, PAGE_SIZE);
        fdcache(HP700_SID_KERNEL, (vm_offset_t)zptr, PAGE_SIZE);

	sp->txt_ring.start=(fsi_cmd *)zptr;
	sp->txt_ring.read=(fsi_cmd *)zptr;
	sp->txt_ring.write=(fsi_cmd *)zptr;
	sp->txt_ring.stop=(fsi_cmd *)(zptr+(2 * RC_SEG_LEN));

	zptr += (4 * RC_SEG_LEN);
	sp->rcv_ring.start=(fsi_cmd *)zptr;
	sp->rcv_ring.read=(fsi_cmd *)zptr;
	sp->rcv_ring.write=(fsi_cmd *)zptr;
	sp->rcv_ring.stop=(fsi_cmd *)(zptr+(2 * RC_SEG_LEN));

	/*
	 * Compute the maximum number of non-transmitted packets we want
	 * to put into the txt ring
	 */
	i = ((2 * RC_SEG_LEN) / sizeof (fsi_cmd)) / FSICMD_PER_PKT;
	if (i * sizeof (struct fddi_header) > PAGE_SIZE)
		i = PAGE_SIZE / sizeof (struct fddi_header);
	sp->txt_hdr_mem = zptr = (char *)kalloc(PAGE_SIZE);

	while (i-- > 0) {
		*(char **)zptr = (char *)sp->txt_hdr;
		sp->txt_hdr = zptr;
		zptr += sizeof (struct fddi_header);
	}

	zptr = (char *)kalloc(PAGE_SIZE);
	bzero(zptr, PAGE_SIZE);

	sp->txt_list.start=(fsi_list *)zptr;
	sp->txt_list.read=(fsi_list *)zptr;
	sp->txt_list.write=(fsi_list *)zptr;
	sp->txt_list.stop=(fsi_list *)(zptr+(4 * RC_SEG_LEN));

	zptr += (4 * RC_SEG_LEN);
	sp->rcv_list.start=(fsi_list *)zptr;
	sp->rcv_list.read=(fsi_list *)zptr;
	sp->rcv_list.write=(fsi_list *)zptr;
	sp->rcv_list.stop=(fsi_list *)(zptr+(4 * RC_SEG_LEN));

	return(1);
}

void
iphase_board_close(sh4811_softc_t *sp)
{
	unsigned char *addr;

	/* stop FSI interrupts */
	addr = sp->hwaddr + FSI_MAC_ELM_INTERRUPT_CONFIG;
	*addr &= ~0x08;

	/* disconnect from ring */
	sh4811_connect_ring(sp, 0);

	/* free memory allocated */
	sh4811_free_resources(sp);
}

void
sh4811_free_resources(sh4811_softc_t *sp)
{
	register int i;
	fsi_list *lp;

	/* release net_kmsg buffers */
	lp=sp->rcv_list.start;
	for(i=0;i<16;i++) {
		net_kmsg_put_buf(sp->rcv_msg_pool, (ipc_kmsg_t)lp->buf_ptr);
		if(++lp == sp->rcv_list.stop)
			break;
	}
	/* release memory allocated for rings and lists */
	if(sp->txt_ring.start != (fsi_cmd *)NULL)
		kfree((vm_offset_t)sp->txt_ring.start, PAGE_SIZE);
	if(sp->txt_list.start != (fsi_list*)NULL)
		kfree((vm_offset_t)sp->txt_list.start, PAGE_SIZE);

	/* release resources allocated for the headers */
	if (sp->txt_hdr_map != 0) {
		unmap_host_from_eisa((void *)sp->txt_hdr_map, PAGE_SIZE);
		sp->txt_hdr_map = 0;
	}
	if (sp->txt_hdr_mem != (char *)0) {
		kfree((vm_offset_t)sp->txt_hdr_mem, PAGE_SIZE);
		sp->txt_hdr = sp->txt_hdr_mem = (char *)0;
	}
}

void
sh4811_create_rings(sh4811_softc_t *sp)
{
	create_ring *cr;
	short len, offset;
	int nbytes=0;
	unsigned char rc_cmd_str[MAX_RC_CMD_SIZE*2];

	/*
	 * Map header page
	 */
	sp->txt_hdr_map = map_host_to_eisa((void *)sp->txt_hdr_mem,
					   PAGE_SIZE, &nbytes);
	assert(nbytes == PAGE_SIZE);

        bzero((char *)rc_cmd_str, MAX_RC_CMD_SIZE);
        cr=(create_ring *)rc_cmd_str;

	len=cr->cmd.cmd_len=sizeof(create_ring);
	cr->cmd.cmd_id=FSI_CREATE_RING_REQ;
	
	cr->tag=(unsigned int)sp;

	/* ratio 217 - 0 for sync, 32 for async and 64 for receive */
	/* ratio 161 - 0 for sync, 16 for async and 32 for receive */
	/* ratio 112 - 0 for sync, 16 for async and 16 for receive */
	/* ratio  56 - 0 for sync,  8 for async and  8 for receive */
	cr->ratio=161; 

	/* not using sync */
	cr->sync_txt_size=0;
	cr->sync_txt_addr=0;

	cr->async_txt_size=2 * RC_SEG_LEN;
	cr->async_txt_addr=map_host_to_eisa((void *)sp->txt_ring.start, 
			4 * RC_SEG_LEN, &nbytes);
	assert(nbytes == 4 * RC_SEG_LEN);

	cr->llc_rcv_size=2 * RC_SEG_LEN;
	cr->llc_rcv_addr=map_host_to_eisa((void *)sp->rcv_ring.start, 
			4 * RC_SEG_LEN, &nbytes);
	assert(nbytes == 4 * RC_SEG_LEN);

	offset=H2B_cmd_offset(sp,len);
	FDDIPRINTF(FDDIDEBUG_INFO, ("addr %x, offset %x, len %x\n",
				    sp->H2Bchan_vaddr, offset, len));
	FDDIPRINTF(FDDIDEBUG_INFO,
		   ("create ring : nbytes %d, Txt - vaddr %x, eisa addr %x.",
		    nbytes, sp->txt_ring.start, cr->async_txt_addr));
	FDDIPRINTF(FDDIDEBUG_INFO, ("Rcv - vaddr %x, eisa addr %x.\n",
				    sp->rcv_ring.start, cr->llc_rcv_addr));
	copy_swap_long(rc_cmd_str, sp->H2Bchan_vaddr+offset, len); 
	H2B_offset_update(sp, len);
}


void
sh4811_connect_ring(sh4811_softc_t *sp, int flag)
{
	short len, offset;
	ccb	*cmd;
	volatile unsigned char *addr;
	volatile unsigned int *iaddr;
	unsigned char rc_cmd_str[MAX_RC_CMD_SIZE]; 

	bzero((char *)rc_cmd_str, MAX_RC_CMD_SIZE);
	addr=rc_cmd_str;

	cmd = (ccb *)addr;
	len=cmd->cmd_len=sizeof(ccb)+(1 * sizeof(int));
	cmd->cmd_id=FDDI_RING_CONNECT;
	addr+=sizeof(ccb);
	iaddr=(unsigned int *)addr;
	if(flag)
		*iaddr++=SH4811_RING_CONNECT;
	else
		*iaddr++=SH4811_RING_DISCONNECT;

	offset=H2B_cmd_offset(sp,len);
	FDDIPRINTF(FDDIDEBUG_INFO, ("connect ring - "));
	FDDIPRINTF(FDDIDEBUG_INFO, ("addr %x, offset %x, len %x\n",
				    sp->H2Bchan_vaddr, offset, len));
	copy_swap_long(rc_cmd_str, sp->H2Bchan_vaddr+offset, len); 
	H2B_offset_update(sp, len);
}

void
sh4811_get_addr(sh4811_softc_t *sp)
{
	short len, offset;
	ccb	*cmd;
	volatile unsigned char *addr;
	volatile unsigned int *iaddr;
	unsigned char rc_cmd_str[MAX_RC_CMD_SIZE]; 

	bzero((char *)rc_cmd_str, MAX_RC_CMD_SIZE);
	addr=rc_cmd_str;

	cmd = (ccb *)addr;
	len=cmd->cmd_len=sizeof(ccb)+(2 * sizeof(int));
	cmd->cmd_id=FSI_GET_ADDR_REQ;
	addr+=sizeof(ccb);
        iaddr=(unsigned int *)addr;
        *iaddr++=B2H_CHAN_OFFSET;       /* channel id */
        *iaddr++=(unsigned int)sp;

	offset=H2B_cmd_offset(sp,len);
	FDDIPRINTF(FDDIDEBUG_INFO, ("get mac addr - "));
	FDDIPRINTF(FDDIDEBUG_INFO, ("addr %x, offset %x, len %x\n",
				    sp->H2Bchan_vaddr, offset, len));
	copy_swap_long(rc_cmd_str, sp->H2Bchan_vaddr+offset, len); 
	H2B_offset_update(sp, len);
}

void
sh4811_set_cam(sh4811_softc_t *sp, unsigned char *cam_addr, unsigned int flag)
{
	short len, offset;
	ccb	*cmd;
	volatile unsigned char *addr;
	volatile unsigned int *iaddr, *addr_cam;
	unsigned char rc_cmd_str[MAX_RC_CMD_SIZE]; 

	bzero((char *)rc_cmd_str, MAX_RC_CMD_SIZE);
	addr=rc_cmd_str;

	cmd = (ccb *)addr;
	len=cmd->cmd_len=sizeof(ccb)+(4 * sizeof(int));
	cmd->cmd_id=flag;
	addr+=sizeof(ccb);
        iaddr=(unsigned int *)addr;
        *iaddr++=B2H_CHAN_OFFSET;       /* channel id */
        *iaddr++=(unsigned int)sp;
	/* Interphase indicates that there is a bug that makes this
	   swap neccessary */
	addr_cam=(unsigned int *)cam_addr;
	swap_long(iaddr,addr_cam);
	iaddr++; addr_cam++;
	swap_long(iaddr,addr_cam);

	offset=H2B_cmd_offset(sp,len);
	FDDIPRINTF(FDDIDEBUG_INFO, ("set cam addr - "));
	FDDIPRINTF(FDDIDEBUG_INFO, ("addr %x, offset %x, len %x\n",
				    sp->H2Bchan_vaddr, offset, len));
	copy_swap_long(rc_cmd_str, sp->H2Bchan_vaddr+offset, len); 
	H2B_offset_update(sp, len);
}

void
sh4811_trace_smt(sh4811_softc_t *sp, int flag)
{
	short len, offset;
	ccb	*cmd;
	volatile unsigned char *addr;
	volatile unsigned int *iaddr;
	unsigned char rc_cmd_str[MAX_RC_CMD_SIZE]; 

	bzero((char *)rc_cmd_str, MAX_RC_CMD_SIZE);
	addr=rc_cmd_str;

	cmd = (ccb *)addr;
	len=cmd->cmd_len=sizeof(ccb)+(1 * sizeof(int));
	cmd->cmd_id=FDDI_TRACE_DIR;
	addr+=sizeof(ccb);
	iaddr=(unsigned int *)addr;
	*iaddr++=flag;

	offset=H2B_cmd_offset(sp,len);
	FDDIPRINTF(FDDIDEBUG_INFO, ("smt trace - "));
	FDDIPRINTF(FDDIDEBUG_INFO, ("addr %x, offset %x, len %x\n",
				    sp->H2Bchan_vaddr, offset, len));
	copy_swap_long(rc_cmd_str, sp->H2Bchan_vaddr+offset, len); 
	H2B_offset_update(sp, len);
}

void
copy_swap_long(unsigned char *source, unsigned char *dest, int nbytes)
{
        volatile unsigned int *f, *t;
        register int len=nbytes;

        f=(unsigned int *)source;
        t=(unsigned int *)dest;
        while(len) {
		swap_long(t,f);
		t++; f++;
                len -= sizeof(int);
        }
        fdcache(HP700_SID_KERNEL, (vm_offset_t)dest, nbytes);
}

void
dump_SRAMcmd(sh4811_softc_t *sp)
{
	register int i;
	register unsigned int *myaddr;

	myaddr = (unsigned int *)
	    (sp->sram_vaddr + sp->sram_info.cb_cmd_start);
	FDDIPRINTF(FDDIDEBUG_DEBUG,
		   ("SRAM starting from command address %x\n", myaddr));
        for(i=0;i<16;i++)
                FDDIPRINTF(FDDIDEBUG_DEBUG, ("%x ", *myaddr++));
	FDDIPRINTF(FDDIDEBUG_DEBUG, ("\n"));
}

void
dump_SRAMrcv(sh4811_softc_t *sp)
{
	register int i;
	register unsigned int *myaddr;
	unsigned int intval;

	FDDIPRINTF(FDDIDEBUG_DEBUG,
		   ("EISA IACK %x, Board IACK %x, IRR1 %x, IRR2 %x\n", 
		    *(unsigned char *)EISA_INTR_ACK,
		    *(unsigned int *)(sp->sram_vaddr + 4),
		    *(unsigned char *)(EISA_BASE_ADDR+EISA_INT1_BASE),
		    *(unsigned char *)(EISA_BASE_ADDR+EISA_INT2_BASE)));

	myaddr = (unsigned int *)(sp->sram_vaddr + H2B_CHAN_OFFSET);
	FDDIPRINTF(FDDIDEBUG_DEBUG, ("SRAM on board channels %x\n", myaddr));
        for(i=0;i<4;i++)
                FDDIPRINTF(FDDIDEBUG_DEBUG, ("%x ", *myaddr++));
	FDDIPRINTF(FDDIDEBUG_DEBUG, ("\t"));
        for(i=0;i<4;i++)
                FDDIPRINTF(FDDIDEBUG_DEBUG, ("%x ", *myaddr++));
	FDDIPRINTF(FDDIDEBUG_DEBUG, ("\n"));

#ifdef notdef
	myaddr = (unsigned int *)sp->H2Bchan_vaddr;
	FDDIPRINTF(FDDIDEBUG_DEBUG,
		   ("SRAM starting from transmit address %x\n", myaddr));
        for(i=0;i<16;i++) 
                FDDIPRINTF(FDDIDEBUG_DEBUG, ("%x ", *myaddr++));
	FDDIPRINTF(FDDIDEBUG_DEBUG, ("\n");

	myaddr = (unsigned int *)sp->B2Hchan_vaddr;
	FDDIPRINTF(FDDIDEBUG_DEBUG,
		   ("SRAM starting from response address %x\n", myaddr));
        for(i=0;i<20;i++) 
                FDDIPRINTF(FDDIDEBUG_DEBUG, ("%x ", *myaddr++));
	FDDIPRINTF(FDDIDEBUG_DEBUG, ("\n"));
#endif
}

int
sh4811_setup_receive(sh4811_softc_t *sp, int numdesc)
{
	register int 	i;
	ipc_kmsg_t              new_kmsg;
        char   *pkt;
	unsigned int 	io_addr, io_addr1, pkt_physaddr;
	int nbytes, nbytes1;

	for(i=0; i<numdesc; i++) {
		new_kmsg = net_kmsg_get_buf(sp->rcv_msg_pool);
		if (new_kmsg == IKM_NULL) {
			if (sp->callback_missed == 0)
				net_kmsg_callback(sp->rcv_msg_pool,
						  &sp->callback);
			sp->callback_missed += numdesc - i;
			FDDIPRINTF(FDDIDEBUG_DEBUG,
				   ("fddi - no more net buffers\n"));
			return(-1);
		}
		pkt = sh4811_net_kmsg(new_kmsg)->packet;
		io_addr = map_host_to_eisa((void *)pkt, PAGE_SIZE, &nbytes);
		pdcache(HP700_SID_KERNEL, (vm_offset_t)pkt, nbytes);
		if(nbytes > PAGE_SIZE || nbytes < 0) {
			FDDIPRINTF(FDDIDEBUG_DEBUG,
				   ("fddi - Error in mapping on receive\n"));
		}
		if(nbytes < PAGE_SIZE) {
			io_addr1 = map_host_to_eisa((void *)(pkt+nbytes),
						    PAGE_SIZE, 
				&nbytes1);
			pdcache(HP700_SID_KERNEL, (vm_offset_t)(pkt+nbytes),
				nbytes1);
			assert ((io_addr & ~PAGE_MASK) + PAGE_SIZE == io_addr1);
		}
		ADD_RCV_BUF(sp->rcv_ring, sp->rcv_list, new_kmsg, 
				pkt, io_addr);
		FDDIPRINTF(FDDIDEBUG_DEBUG,
			   ("rcvbuf - nbytes %d, pkt vaddr %x, eisa %x\n",
			    nbytes, pkt, io_addr));
	} /* end for */

	return(fsi_cr_cmd(sp, FCR_RING_READY4));
}
	
#ifdef	FDDITEST
unsigned char test_pkt[4098] = { 0x30, 0x38, 0x00, 0x50, 
		0x00, 0x00, 0xEE, 0x21, 0x2B, 0xA5,
		0x00, 0x00, 0xEE, 0x21, 0x2B, 0x45,
		0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
} ;

int
sh4811_test_send(sh4811_softc_t *sp)
{
	struct io_req ior;
	unsigned char *pkt=test_pkt;
	register int i,j;

	pkt = test_pkt + 4;
	bcopy((char *)sp->noncan_mac_addr, (char *)pkt, 6);
	if(sp->noncan_mac_addr[5]==0xA5)
		*(test_pkt+9) = 0x45;
	else
		*(test_pkt+9) = 0xA5;
	pkt+=6;
	bcopy((char *)sp->noncan_mac_addr, (char *)pkt, 6);

	ior.io_data=(char *)(&test_pkt[0]);
	ior.io_count=4096;

	sh4811_txt(sp, &ior);
	return(1);
}
#endif

int
sh4811_txt(sh4811_softc_t *sp, io_req_t ior)
{
	char *data;
	unsigned int 	cmr, cer;
	int count, nbytes=0;
	struct fddi_header *fddi_hdr;
	unsigned hcer;
	spl_t s;

	if(sp->status != SH4811_RING_CONNECT)
		return(0);

	/*
	 * Manage fddi header in a different buffer to be able to
	 * bit reverse src and dst address
	 */
	fddi_hdr = (struct fddi_header *)sp->txt_hdr;
	if (fddi_hdr == (struct fddi_header *)0)
		return (0);
	sp->txt_hdr = *(char **)fddi_hdr;
	sh4811_copyhdr(fddi_hdr, (struct fddi_header *)ior->io_data);
	fdcache(HP700_SID_KERNEL,
		(vm_offset_t)fddi_hdr, sizeof (struct fddi_header));
	hcer = sp->txt_hdr_map + ((vm_offset_t)fddi_hdr & PAGE_MASK);
	FDDIPRINTF(FDDIDEBUG_INFO,
		   ("txtbuf - hdr vaddr %x, eisa_addr %x\n", fddi_hdr, hcer));

	/*
	 * Deal with user data
	 */
	data = ior->io_data + sizeof (struct fddi_header);
	count = ior->io_count - sizeof (struct fddi_header);

	fdcache(HP700_SID_KERNEL, (vm_offset_t)data, count);
	cer = map_host_to_eisa(data, count, &nbytes);
	if(nbytes < count) {
		/* We have a data buffer that spans two physical pages */
		int nbytes2;
		char *data2;
		unsigned int cer2;


		data2 = data + nbytes;
		cer2 = map_host_to_eisa(data2, count-nbytes, &nbytes2);
		assert(count == nbytes + nbytes2);
		
		s = splhigh();
		eisa_arb_disable();
#if MACH_ASSERT
		pdcache(HP700_SID_KERNEL,
			(vm_offset_t)sp->txt_ring.write, sizeof(fsi_cmd));
		swap_long(&cmr, &sp->txt_ring.write->cmr);
		assert((cmr & HOST_OWNED) == 0);
#endif /* MACH_ASSERT */
		cmr = HOST_OWNED | VALID_FRAME | FIRST_FRAME |
		    sizeof (struct fddi_header);
		ADD_TXT_BUF(sp->txt_ring, sp->txt_list, (vm_offset_t)fddi_hdr,
			    0, cmr, hcer, sizeof (struct fddi_header));

#if MACH_ASSERT
		pdcache(HP700_SID_KERNEL,
			(vm_offset_t)sp->txt_ring.write, sizeof(fsi_cmd));
		swap_long(&cmr, &sp->txt_ring.write->cmr);
		assert((cmr & HOST_OWNED) == 0);
#endif /* MACH_ASSERT */
		cmr = HOST_OWNED | VALID_FRAME | nbytes ;
		ADD_TXT_BUF(sp->txt_ring, sp->txt_list, data,
			    0, cmr, cer, nbytes);

#if MACH_ASSERT
		pdcache(HP700_SID_KERNEL,
			(vm_offset_t)sp->txt_ring.write, sizeof(fsi_cmd));
		swap_long(&cmr, &sp->txt_ring.write->cmr);
		assert((cmr & HOST_OWNED) == 0);
#endif /* MACH_ASSERT */
		cmr = HOST_OWNED | VALID_FRAME | LAST_FRAME | nbytes2 ; 
		ADD_TXT_BUF(sp->txt_ring, sp->txt_list, data2,
			    ior, cmr, cer2, nbytes2);

		eisa_arb_enable();
		splx(s);

		FDDIPRINTF(FDDIDEBUG_INFO,
			   ("txtbuf - data vaddr %x, eisa_addr %x, size %x\n",
			    data, cer, nbytes));
		FDDIPRINTF(FDDIDEBUG_INFO,
			   ("2 txtbuf - data %x, size %x, cer %x\n",
			    data2, nbytes2, cer2));
	}
	else {
		s = splhigh();
		eisa_arb_disable();
#if MACH_ASSERT
		pdcache(HP700_SID_KERNEL,
			(vm_offset_t)sp->txt_ring.write, sizeof(fsi_cmd));
		swap_long(&cmr, &sp->txt_ring.write->cmr);
		assert((cmr & HOST_OWNED) == 0);
#endif /* MACH_ASSERT */
		cmr = HOST_OWNED | VALID_FRAME | FIRST_FRAME |
		    sizeof (struct fddi_header);
		ADD_TXT_BUF(sp->txt_ring, sp->txt_list, (vm_offset_t)fddi_hdr,
			    0, cmr, hcer, sizeof (struct fddi_header));

#if MACH_ASSERT
		pdcache(HP700_SID_KERNEL,
			(vm_offset_t)sp->txt_ring.write, sizeof(fsi_cmd));
		swap_long(&cmr, &sp->txt_ring.write->cmr);
		assert((cmr & HOST_OWNED) == 0);
#endif /* MACH_ASSERT */
		cmr = HOST_OWNED | VALID_FRAME | LAST_FRAME | count ; 
		ADD_TXT_BUF(sp->txt_ring, sp->txt_list, data,
			    ior, cmr, cer, count);

		eisa_arb_enable();
		splx(s);

		FDDIPRINTF(FDDIDEBUG_INFO,
			   ("txtbuf - data vaddr %x, eisa_addr %x, size %x\n",
			    data, cer, count));
	}
#ifdef DEBUG
	if (fddidebug & FDDIDEBUG_DEBUG) {
		register int i;
		register int *p=(int *)fddi_hdr;
		FDDIPRINTF(FDDIDEBUG_DEBUG, ("txt hdr data - "));
        	for(i=0; i<sizeof(struct fddi_header)/sizeof(int); i++)
			FDDIPRINTF(FDDIDEBUG_DEBUG, ("%x ", *p++));
		FDDIPRINTF(FDDIDEBUG_DEBUG, ("\n"));

		p = (int *)data;
		FDDIPRINTF(FDDIDEBUG_DEBUG, ("txt data - "));
        	for(i=0; i<64/sizeof(int); i++)
			FDDIPRINTF(FDDIDEBUG_DEBUG, ("%x ", *p++));
		FDDIPRINTF(FDDIDEBUG_DEBUG, ("\n"));
	}
#endif /* DEBUG */
	fsi_cr_cmd(sp, FCR_RING_READY1);
	sp->tbusy++;
	return(1);
}

/* command issued to FSI control register. First check if free by reading
   status register.
*/
int
fsi_cr_cmd(sh4811_softc_t *sp, unsigned int command)
{
	volatile unsigned int *addr;
	unsigned int status;
	register int count=MAX_FCR_TRIES;
	unsigned long oldpri;

	while(--count) {
		addr=(volatile unsigned int *)(sp->hwaddr + FSI_STATUS_REG1);
		swap_long(&status, addr);
		if((status & FSI_FCR_FREE) == FSI_FCR_FREE)
			break;
	}
	if(!count) {
		printf("FSI Control register not free\n");
		return(0);
	}
	addr=(volatile unsigned int *)(sp->hwaddr + FSI_CONTROL_REG);
	swap_long(addr, &command);
	return(1);
}

int
fsi_cr_read(sh4811_softc_t *sp, unsigned int command)
{
	volatile unsigned int *addr;
	unsigned int status, retval;
	register int count=MAX_FCR_TRIES;

	while(--count) {
		addr=(volatile unsigned int *)(sp->hwaddr + FSI_STATUS_REG1);
		swap_long(&status, addr);
		if((status & FSI_FCR_FREE) == FSI_FCR_FREE)
			break;
	}
	if(!count) {
		printf("FSI Control register not free\n");
		return(0);
	}
	addr=(volatile unsigned int *)(sp->hwaddr + FSI_CONTROL_REG);
	swap_long(addr, &command);

	count=MAX_FCR_TRIES;
	while(--count) {
		addr=(volatile unsigned int *)(sp->hwaddr + FSI_STATUS_REG1);
		swap_long(&status, addr);
		if((status & FSI_FCR_FREE) == FSI_FCR_FREE)
			break;
	}
	if(!count) {
		printf("FSI Control register not free\n");
		return(0);
	}

	addr=(volatile unsigned int *)(sp->hwaddr + FSI_CONTROL_REG);
	swap_long(&retval, addr);
	return(retval);
}

int
fsi_command(sh4811_softc_t *sp, int cmr, int cer)
{
	volatile unsigned int *addr;
	unsigned int status;
	register int count=MAX_FCR_TRIES;

	addr=(volatile unsigned int *)(sp->hwaddr + FSI_COMMAND_EXT_REG);
	swap_long(addr, &cer);
	addr=(volatile unsigned int *)(sp->hwaddr + FSI_COMMAND_REG);
	swap_long(addr, &cmr);
	return(1);
}

int
dump_fddi_frame(ipc_kmsg_t kmsg, int len)
{
#ifdef DEBUG
	unsigned int *pkt;
	register int 	i;
	
	if ((fddidebug & FDDIDEBUG_DEBUG) == 0)
		return(0);

	pkt = (unsigned int *)(sh4811_net_kmsg(kmsg)->packet);
	FDDIPRINTF(FDDIDEBUG_DEBUG, ("pkt of len %x at %x - ", len, pkt));
	for(i=0; i<32/sizeof(int); i++) {
		FDDIPRINTF(FDDIDEBUG_DEBUG, ("%x ", *pkt++));
	}
	FDDIPRINTF(FDDIDEBUG_DEBUG, ("\n"));
	if(i*sizeof(int) >= len)
		return(0);
	i= (len-32)/sizeof(int);
	for(; i<len/sizeof(int); i++) {
		FDDIPRINTF(FDDIDEBUG_DEBUG, ("%x ", *pkt++));
	}
	FDDIPRINTF(FDDIDEBUG_DEBUG, ("\n"));
#endif /* DEBUG */
	return(0);
}

/*
 * The RC interface does not reverse bits of address, ...
 * The driver is then responsible for doing the work
 */
static unsigned char    bitrevtbl[256] = {
        0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
        0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
        0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
        0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
        0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
        0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
        0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
        0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
        0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
        0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
        0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
        0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
        0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
        0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
        0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
        0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
        0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
        0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
        0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
        0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
        0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
        0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
        0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
        0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
        0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
        0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
        0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
        0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
        0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
        0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
        0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
        0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

void
sh4811_copyhdr(struct fddi_header *dst, struct fddi_header *src)
{
	unsigned int i;

	if (src != dst) {
	    for (i = 0; i < 3; i++)
		dst->fddi_ph[i] = src->fddi_ph[i];
	    dst->fddi_fc = src->fddi_fc;
	}
	for (i = 0; i < 6; i++) {
		dst->fddi_dhost[i] = bitrevtbl[src->fddi_dhost[i]];
		dst->fddi_shost[i] = bitrevtbl[src->fddi_shost[i]];
	}
}
#endif
