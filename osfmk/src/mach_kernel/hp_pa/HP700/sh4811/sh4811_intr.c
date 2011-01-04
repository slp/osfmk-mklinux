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

#include <mach/vm_param.h>
#include <hp_pa/pmap.h>
#include <device/if_hdr.h>
#include <device/net_status.h>
#include <ipc/ipc_kmsg.h>
#include <device/net_io.h>
#include <device/io_req.h>
#include <device/ds_routines.h>
#include <device/if_llc.h>
#include <device/if_fddi.h>
#include <device/if_ether.h>

#include <hp_pa/HP700/driver_lock.h>
#include <hp_pa/HP700/eisa_common.h>
#include <hp_pa/HP700/sh4811/iphase_cbrc.h>
#include <hp_pa/HP700/sh4811/sh4811_ring.h>
#include <hp_pa/HP700/sh4811/if_sh4811.h>
#include <hp_pa/HP700/device.h>
#include <hp_pa/HP700/sh4811/if_sh4811var.h>

static int sh4811_boardseg=0;

int
sh4811_rc_intr(sh4811_softc_t *sp)
{
        unsigned int replen, boffset ;
        unsigned char reply[MAX_RC_REPLY_SIZE];
	unsigned char *brep_addr;
	unsigned int intr_timer, tm=0x00000001;
	unsigned long oldpri;
	int bseg;
	int count;

	B2H_board_offset(sp, boffset);
	/* to take care of change of segment on the B2H channel */
	count = 0;
	while((boffset & 0x0000FFFF) == 0x0000FFFF) {
		if(count++ > 100) {
			printf("fddi board still busyon offset\n");
			return(0);
		}
		B2H_board_offset(sp, boffset);
	}
	bseg = boffset & 0xFFFF0000;
	if(bseg != sh4811_boardseg << 16) {
		sh4811_boardseg = (sh4811_boardseg ? 0 : 1);
		sp->B2Hread=0;
	}
	boffset &= 0x0000FFFF;
	brep_addr=(sp->sram_vaddr+B2H_SEG_OFFSET+sh4811_boardseg*RC_SEG_LEN)
			+B2H_reply_offset(sp);
	FDDIPRINTF(FDDIDEBUG_INTR,
		   ("In sh4811_rc_intr - addr %x, seg %x, start %x, end %x\n",
		    brep_addr, bseg, B2H_reply_offset(sp), boffset));

        while(B2H_reply_offset(sp) < boffset) {
		B2H_get_reply_len(brep_addr, replen);
		FDDIPRINTF(FDDIDEBUG_INTR, ("next reply len %x at addr %x\n",
					    replen, brep_addr));
                if(!replen) {
			printf("sh4811_rc_intr: Got invalid header in reply\n");
			break;
                }
                if(!process_reply(sp, brep_addr, sp->rcvind, replen)) {
			B2H_board_offset(sp, boffset);
			bseg = boffset & 0xFFFF0000;
			sh4811_boardseg = (sh4811_boardseg ? 0 : 1);
			boffset &= 0x0000FFFF;
			brep_addr=(sp->sram_vaddr+B2H_SEG_OFFSET+
					sh4811_boardseg*RC_SEG_LEN);
			continue;
		}
		brep_addr += replen;
		B2H_read_update(sp, bseg, replen);
		B2H_board_offset(sp, boffset);
		boffset &= 0x0000FFFF;
        }
	/* acknowledge interrupt */
        int_swap(tm, intr_timer);
        *(volatile unsigned int *)(sp->sram_vaddr+B2H_CHAN_OFFSET) = intr_timer;
	*(volatile short *)(sp->sram_vaddr+6)=0;
        return(1);
}

int
process_reply(sh4811_softc_t *sp, unsigned char *source, unsigned char *dest, unsigned int len)
{
        ccb     *cmd;
	unsigned char *ucresp;

	copy_swap_long(source, dest, len);
        cmd = (ccb *)dest;
        ucresp=dest+sizeof(ccb);

	switch(cmd->cmd_id) {
		case RC_GET_STATRESP:
			{
			rc_stat_resp *resp=(rc_stat_resp *)ucresp;
			rc_stat_resp *unmodresp=
					(rc_stat_resp *)(source+sizeof(ccb));

			FDDIPRINTF(FDDIDEBUG_STAT,
				   ("Interphase SeaHawk EISA/FDDI\n"));
			FDDIPRINTF(FDDIDEBUG_STAT,
				   ("Firmware id %s, release date %s\n",
				    unmodresp->firmware_id,
				    unmodresp->rel_date));
			FDDIPRINTF(FDDIDEBUG_STAT,
				   ("buffer size %d, SRAM size %d Kbytes\n",
				    resp->buffersize/1024,
				    resp->sramsize/1024));
			break;
			}
		case FSI_GET_ADDR_RESP:
			{
			register int i;
			unsigned char *resp=
					(unsigned char *)(source+sizeof(ccb));
			unsigned char *addr;

			resp+=4;
			FDDIPRINTF(FDDIDEBUG_STAT, ("Mac addr canonical - "));
			addr=sp->can_mac_addr;
			for(i=0;i<6;i++) {
				*addr++=*resp++;
				FDDIPRINTF(FDDIDEBUG_STAT,
					   ("%x ", sp->can_mac_addr[i]));
			}
			FDDIPRINTF(FDDIDEBUG_STAT, ("\n"));
			FDDIPRINTF(FDDIDEBUG_STAT,
				   ("Mac addr noncanonical - "));
			addr=sp->noncan_mac_addr;
			for(i=0;i<6;i++) {
				*addr++=*resp++;
				FDDIPRINTF(FDDIDEBUG_STAT,
					   ("%x ", sp->noncan_mac_addr[i]));
			}
			FDDIPRINTF(FDDIDEBUG_STAT, ("\n"));
			
			break;
			}
		case FDDI_TRACE_IND:
			{
			rc_stat_resp *resp=(rc_stat_resp *)ucresp;
                        register int i;
                        unsigned int *iaddr;
			FDDIPRINTF(FDDIDEBUG_STAT, ("fddi SMT event\n"));
                        iaddr=(unsigned int *)ucresp;
                        for(i=0; i<(len-sizeof(ccb))/sizeof(int);i++,iaddr++)
                                FDDIPRINTF(FDDIDEBUG_STAT, (" %x ", *iaddr));
                        FDDIPRINTF(FDDIDEBUG_STAT, ("\n"));
			break;
			}
		case FSI_CREATE_RING_RESP:
			{
			create_ring_resp *resp=(create_ring_resp *)ucresp;

			FDDIPRINTF(FDDIDEBUG_STAT,
				   ("Transmit ring - status %x, size %x\n",
				    resp->async_txt_status,
				    resp->async_txt_size));
			FDDIPRINTF(FDDIDEBUG_STAT,
				   ("Receive ring - status %x, size %x\n",
				    resp->llc_rcv_status,
				    resp->llc_rcv_size));

			/*
			 * There is a bug in the InterPhase firmware, since
			 * the maximum number of packets in the onboard memory
			 * is limited to the size of the area / 4K. But, FDDI
			 * packets may be > 4K long, thus leading to packet
			 * overwriting, hopefully detected by a higher level
			 * checksum control. The correction id to decrease the
			 * limit by 1 packet in order to avoid overwriting.
			 *
			 * Currently, this is not done, because it is linked to
			 * the size allocated in the onbard memory. The command
			 * for the receiving area of 32K limited to 7 packets:
			 * fsi_cr_cmd(sp, 0x2C060000);
			 */
			break;
			}
		case FDDI_RING_INDICATION:
			{
			unsigned int status=*(unsigned int *)ucresp;
			unsigned int mask, *iaddr;
			volatile unsigned char *addr;
			
			if(status == SH4811_RING_CONNECT) {
        			addr = sp->hwaddr+FSI_MAC_ELM_INTERRUPT_CONFIG;
        			*addr |= 0x28;
			        addr = sp->hwaddr+TIMER_CPU_INTERRUPT_CONFIG;
			        *addr |= 0x38;
                		iaddr = (unsigned int *)(sp->hwaddr+FSI_INTR_MASK1);
                		mask=FSI_INT_MASK; 
                		swap_long(iaddr, &mask);
				sp->status=SH4811_RING_CONNECT;
				sp->ds_if.if_flags |= (IFF_UP | IFF_RUNNING);
				sp->tbusy=0;
				FDDIPRINTF(FDDIDEBUG_STAT,
					   ("FDDI connected to ring and FSI interrupts enabled\n"));
				/*
				 * Start transmitting in case there are
				 * waiting packets
				 */
				sh4811start(sp->ds_if.if_unit);
			}
			else if(status == SH4811_RING_DISCONNECT) {
				FDDIPRINTF(FDDIDEBUG_STAT,
					   ("FDDI not connected to ring anymore\n"));
				sp->status=SH4811_RING_DISCONNECT;
				sp->ds_if.if_flags &= ~(IFF_UP | IFF_RUNNING);
			}
			break;
			}
		case RC_LINK_SEG_IND:
			{
			unsigned int *iaddr=(unsigned int *)ucresp;

			/* reset read ptr to beginning of new segment */
			sp->B2Hread = 0;
			/* reset board seg */
			sh4811_boardseg = (sh4811_boardseg ? 0 : 1);
			FDDIPRINTF(FDDIDEBUG_STAT,
				   ("fddi - Link to next segment %x\n",
				    *iaddr));
			return(0);
			}
		case FDDI_SET_CAM_RESP:
			{
			unsigned int *statusp=(unsigned int *)ucresp;

			statusp++;
			FDDIPRINTF(FDDIDEBUG_STAT,
				   ("fddi CAM status %d\n", *statusp));
			break;
			}
		default:
			{
			register int i;
			unsigned int *iaddr;
			FDDIPRINTF(FDDIDEBUG_STAT,
				   ("fddi - unknown resp/ind id %x\n",
				    cmd->cmd_id));
			iaddr=(unsigned int *)ucresp;
			for(i=0; i<(len-sizeof(ccb))/sizeof(int);i++,iaddr++)
				FDDIPRINTF(FDDIDEBUG_STAT, (" %x ", *iaddr));
			FDDIPRINTF(FDDIDEBUG_STAT, ("\n"));
			break;
			}
	}

	if (sp->indication == cmd->cmd_id) {
		assert(sp->indication != 0);
		sp->indication = 0;
		thread_wakeup((event_t)&sp->indication);
	}
	return(1);
}

int
sh4811_fsi_intr(sh4811_softc_t *sp)
{
	unsigned int *addr;
	unsigned int status;
	boolean_t restart;

	FDDIPRINTF(FDDIDEBUG_INTR, ("Received FSI interrupt\n"));
	do {
		restart = FALSE;
		addr=(unsigned int *)(sp->hwaddr + FSI_STATUS_REG1);
		swap_long(&status, addr);

		if((status & FSTAT_ROV4) == FSTAT_ROV4) {
			FDDIPRINTF(FDDIDEBUG_INTR,
				   ("fddi receive overrun - %x\n", status));
			sp->ds_if.if_ierrors++;
			fddi_pkt_rcv(sp, FSTAT_RXC4 | FSTAT_RNR4 | FSTAT_ROV4);
			restart = TRUE;
		} else if((status & FSTAT_RXC4) == FSTAT_RXC4) {
			fddi_pkt_rcv(sp, FSTAT_RXC4 | FSTAT_RNR4);
			restart = TRUE;
		}
		if((status & FSTAT_RCC1) == FSTAT_RCC1) {
			endof_transmit(sp);
			restart = TRUE;
		}
	} while (restart);

        return(1);
}

int
wait_for_end(sh4811_softc_t *sp)
{
	unsigned int cmr;
	register int count=1000;

        while(count--) {
                pdcache(HP700_SID_KERNEL,
                        (vm_offset_t)sp->txt_ring.write, sizeof(fsi_cmd));
                swap_long(&cmr, &sp->txt_ring.write->cmr);
                if((cmr & HOST_OWNED))
                        break;
	}
	return(count); 
}

int
endof_transmit(sh4811_softc_t *sp)
{
	unsigned int cmr, cer;
	io_req_t ior;
        unsigned int *addr;
        unsigned int status;
	spl_t s;
        struct ifnet   *ifp = &sp->ds_if;

	FDDIPRINTF(FDDIDEBUG_EOT,
		   ("endof_transmit - read %x, write %x, cmr %x\n", 
		    sp->txt_ring.read, sp->txt_ring.write,
		    sp->txt_ring.read->cmr));

	/* clear status register */
        addr=(unsigned int *)(sp->hwaddr + FSI_STATUS_REG1);
	status = FSTAT_RCC1 | FSTAT_RNR1; 
	swap_long(addr, &status);

	while(sp->txt_ring.read != sp->txt_ring.write) {
        	pdcache(HP700_SID_KERNEL,
                	(vm_offset_t)sp->txt_ring.read, sizeof(fsi_cmd));
		s = splhigh();
		eisa_arb_disable();
		swap_long(&cmr, &sp->txt_ring.read->cmr);
		if((cmr & HOST_OWNED) == HOST_OWNED) {
			eisa_arb_enable();
			splx(s);
			break;
		}
		swap_long(&cer, &sp->txt_ring.read->cer);
		eisa_arb_enable();
		splx(s);

		if((cmr & LAST_FRAME) == LAST_FRAME) {
			if((cmr & 0x001F0000) != 0) {
				FDDIPRINTF(FDDIDEBUG_EOT,
					   ("fddi - Error in transmit : %x\n",
					    cmr));
				ifp->if_oerrors++;
			}
			else {
				FDDIPRINTF(FDDIDEBUG_EOT,
					   ("transmit successful %x\n", cer));
				ifp->if_opackets++;
				sp->tbusy--;
			}
#ifdef	FDDITEST
			sp->stats.txtsz += sp->txt_list.read->size;
#else
			assert(sp->txt_list.read->buf_ptr);
			ior = (io_req_t)sp->txt_list.read->buf_ptr;
			ior->io_residual = 0;
                	ior->io_error = 0;
                	iodone(ior);
#endif
		}

		if((cmr & FIRST_FRAME) == FIRST_FRAME) {
			*(char **)sp->txt_list.read->io_ptr = sp->txt_hdr;
			sp->txt_hdr = (char *)sp->txt_list.read->io_ptr;
		} else
			unmap_host_from_eisa((void *)sp->txt_list.read->io_ptr, 
					     sp->txt_list.read->size);

		INC_READ_PTR(sp->txt_ring);
		INC_READ_PTR(sp->txt_list);
	}

	/*
	 * Restart transmitting in case there are waiting packets
	 */
	sh4811start(sp->ds_if.if_unit);
	return(1);
}

int
fddi_pkt_rcv(sh4811_softc_t *sp, unsigned int status)
{
	unsigned int cmr, cmr2;
        struct ifnet   *ifp = &sp->ds_if;
	int len;
	ipc_kmsg_t		rcv_kmsg;
	struct packet_header 	*pkt_hdr;
	unsigned char      *fddi_hdr;
	struct  fddi_header *fh;
	struct llc              *ll;
	int	numdesc;
        unsigned int *addr;
        unsigned int mask;
	boolean_t error;
	spl_t s;

	FDDIPRINTF(FDDIDEBUG_RCV, ("fddi_pkt_rcv - read %x, write %x\n", 
				   sp->rcv_ring.read, sp->rcv_ring.write));

	/* clear status register */
        addr=(unsigned int *)(sp->hwaddr + FSI_STATUS_REG1);
	swap_long(addr, &status);

	while(sp->rcv_ring.read != sp->rcv_ring.write) {
		numdesc = 0;
		error = FALSE;
        	pdcache(HP700_SID_KERNEL,
                	(vm_offset_t)sp->rcv_ring.read, sizeof(fsi_cmd));
		s = splhigh();
		eisa_arb_disable();
		swap_long(&cmr, &sp->rcv_ring.read->cmr);
		if(cmr & HOST_OWNED) {
        		eisa_arb_enable();
			splx(s);
			break;
		}
		assert(cmr & FIRST_FRAME);
		if((cmr & LAST_FRAME) == 0) {
			/*
			 * Validate that there is an indication
			 * in the next frame
			 */
			fsi_cmd *next_read = NEXT_READ_PTR(sp->rcv_ring);
			if (next_read == sp->rcv_ring.write) {
				eisa_arb_enable();
				splx(s);
				break;
			}
			pdcache(HP700_SID_KERNEL,
				(vm_offset_t)next_read, sizeof(fsi_cmd));
			swap_long(&cmr2, &next_read->cmr);
			if(cmr2 & HOST_OWNED) {
				eisa_arb_enable();
				splx(s);
				break;
			}
			if((cmr2 & ERROR_FRAME) ||
			   (cmr2 & CRC_ERROR_FRAME))
				error = TRUE;
			assert((cmr2 & (FIRST_FRAME|LAST_FRAME)) == LAST_FRAME);
			assert((cmr & ERROR_FRAME) == 0);
		} else if((cmr & ERROR_FRAME) ||
			  (cmr & CRC_ERROR_FRAME))
			error = TRUE;
		eisa_arb_enable();
		splx(s);

		numdesc++;
		if(!error) {
			len = LENGTH_FROM_IND(cmr);
			if(len < ifp->if_header_size || 
			   len > ifp->if_mtu+ifp->if_header_size) {
				printf("FDDI: Invalid length = %d\n", len);
				error = TRUE;
			}
		}

		rcv_kmsg = (ipc_kmsg_t)sp->rcv_list.read->buf_ptr;
		if (error)
			ifp->if_rcvdrops++;
		else {
			ifp->if_ipackets++;			
#ifdef	FDDITEST
			sp->stats.rcvsz += len;
#endif
			pkt_hdr = (struct packet_header *)
				(sh4811_net_kmsg(rcv_kmsg)->packet);
			sh4811_copyhdr((struct fddi_header *)pkt_hdr,
				       (struct fddi_header *)pkt_hdr);
			fddi_hdr = (unsigned char *)pkt_hdr;
			ll = (struct llc *)
				(fddi_hdr + sizeof(struct fddi_header));
			if(ll->llc_dsap == LLC_SNAP_LSAP && 
			   ll->llc_ssap == LLC_SNAP_LSAP)
                		pkt_hdr->type = 1;
			else
                		pkt_hdr->type = ll->llc_un.type_snap.ether_type;
			pkt_hdr->length = len + sizeof(struct packet_header);
#ifdef	DEBUG
			dump_fddi_frame(rcv_kmsg, len);
#endif
			/* checking needs to be done for non LLC packets */
			if(pkt_hdr->length < sizeof(struct packet_header)) {
				printf("fddi - Not an LLC packet\n");
				return(0);
			}
			/* check for valid multicast */
			fh = (struct fddi_header *)fddi_hdr;
			if (fh->fddi_dhost[0] == 0x01 &&
			    fh->fddi_dhost[2] == 0x5E &&
			    net_multicast_match(&sp->multicast_queue,
						fh->fddi_dhost, 
						FDDI_ADDR_SIZE)
			    == (net_multicast_t *)0) {
				printf("fddi - Not a valid multicast packet\n");
				continue;
			}
			FDDIPRINTF(FDDIDEBUG_RCV,
				   ("pkt %x, cmr %x, type %x, len %x\n", 
				    pkt_hdr, cmr, pkt_hdr->type,
				    pkt_hdr->length));
		}
		unmap_host_from_eisa((void *)sp->rcv_list.read->io_ptr, 
				     PAGE_SIZE);
		INC_READ_PTR(sp->rcv_ring);
		INC_READ_PTR(sp->rcv_list);

		if((cmr & LAST_FRAME) == 0) {
		/* second of two */
/* Note: the lengths in cmr returned by the board are flaky when there are
   two indications. Ex. we are receiving 5 Kb. We get 2K in the first
   indication and the total length  ie. 5K in the second. In reality
   4K (PAGE_SIZE) has been mapped on the first indication and the rest ie.
   1K in the second.
*/
			int remlen;
			ipc_kmsg_t second_kmsg = (ipc_kmsg_t)
				sp->rcv_list.read->buf_ptr;
			if (!error) {
				len = LENGTH_FROM_IND(cmr2);
				assert(len > 0);
				if(len < ifp->if_header_size || 
				   len > ifp->if_mtu+ifp->if_header_size) {
				    printf("FDDI: Invalid length = %d\n", len);
				    error = TRUE;
				} else {
				    remlen = len - PAGE_SIZE;
				    FDDIPRINTF(FDDIDEBUG_RCV,
					       ("Second - cmr %x, indlen %x, copy len %x\n",
						cmr2, len, remlen));
				    bcopy(sh4811_net_kmsg(second_kmsg)->packet,
					  (char *)(fddi_hdr+PAGE_SIZE), remlen);
				    pkt_hdr->length = len;
				}
			}
			unmap_host_from_eisa((void *)sp->rcv_list.read->io_ptr, 
					     PAGE_SIZE);
			net_kmsg_put_buf(sp->rcv_msg_pool, second_kmsg);
			numdesc++;
			INC_READ_PTR(sp->rcv_ring);
			INC_READ_PTR(sp->rcv_list);
		}
		if (!error) {
			rcv_kmsg->ikm_header.msgh_id = NET_RCV_MSG_ID;
			net_packet_pool(sp->rcv_msg_pool, ifp, 
					rcv_kmsg, pkt_hdr->length, 
					0, 0);
		} else
			net_kmsg_put_buf(sp->rcv_msg_pool, rcv_kmsg);

		/* make sure enough receive descriptors are available */
		(void)sh4811_setup_receive(sp, numdesc);
	}
    
	return(1);
}
#endif /* NFDDI > 0 */
