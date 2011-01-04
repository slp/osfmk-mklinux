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
 * 
 */
/*
 * MkLinux
 */
/*
 * BMAC ethernet driver for PowerMac G3 machines.
 *
 * TODO
 * 	- Figure out WHY the transmit rate is so poor, yet
 *	  the receive rate is very good!
 *
 *      - Verify multicasting works.
 *
 *	- Be able to handle more than one BMAC.
 *
 *	- Finish SMPizing driver.
 *
 * -- Michael Burg, Apple Computer Inc. 1998
 */

#include <bmac.h>

#if NBMAC > 0
#include <debug.h>
#include <mach_kdb.h>

#include	<kern/time_out.h>
#include	<device/device_types.h>
#include	<device/ds_routines.h>
#include	<device/errno.h>
#include	<device/io_req.h>
#include	<device/if_hdr.h>
#include	<device/if_ether.h>
#include	<device/net_status.h>
#include	<device/net_io.h>
#include	<chips/busses.h>
#include	<machine/endian.h> /* for ntohl */
#include	<vm/vm_kern.h>

#include	<ppc/spl.h>
#include	<ppc/proc_reg.h>
#include	<ppc/io_map_entries.h>
#include	<ppc/POWERMAC/io.h>
#include	<ppc/POWERMAC/interrupts.h>
#include	<ppc/POWERMAC/if_bmac.h>
#include	<ppc/POWERMAC/if_bmac_entries.h>
#include	<ppc/POWERMAC/powermac.h>
#include	<ppc/POWERMAC/powermac_pci.h>
#include	<ppc/POWERMAC/dbdma.h>
#include	<ppc/POWERMAC/device_tree.h>

#define	ETHERMINPACKET		64
#define	ETHERCRC		4
#define	ETHERNET_BUF_SIZE	(ETHERMTU+14+ETHERCRC+2)
#define	ETHER_ADDR_SIZE		6

struct enet_command {
	dbdma_command_t		desc_seg[2];
};

typedef struct enet_command enet_command_t;

struct bmac {
    	decl_simple_lock_data(,lock)
	vm_offset_t	ioBaseEnet;
	dbdma_regmap_t	*ioBaseEnetRxDMA;
	dbdma_regmap_t	*ioBaseEnetTxDMA;
	vm_offset_t	ioBaseHeathrow;

	unsigned char		myAddress[ETHER_ADDR_SIZE];
	struct ifnet	networkInterface;

    boolean_t		isPromiscuous;
    boolean_t		multicastEnabled;

    boolean_t		resetAndEnabled;
    unsigned int	enetAddressOffset;

    

    unsigned char	sromAddressBits;

    io_req_t		txNetbuf[TX_RING_LENGTH];
    char		*txDoubleBuffers[TX_RING_LENGTH];

    char		*rxNetbuf[RX_RING_LENGTH];
    
    unsigned int	maxDMACommands;
    unsigned int	txCommandHead;		/* Transmit ring descriptor index */
    unsigned int	txCommandTail;
    unsigned int       	txMaxCommand;		
    unsigned int	rxCommandHead;		/* Receive ring descriptor index */
    unsigned int	rxCommandTail;
    unsigned int       	rxMaxCommand;		

    dbdma_command_t *	dmaCommands;
    dbdma_command_t *	txDMACommands;		/* TX descriptor ring ptr */
    dbdma_command_t *	rxDMACommands;		/* RX descriptor ring ptr */

    unsigned short	hashTableUseCount[64];
    unsigned short	hashTableMask[4];
};


typedef struct bmac bmac_t;

bmac_t	*bmacs[NBMAC];

static void bmac_start(int unit);

static __inline__
void WriteBigMacRegister(vm_offset_t ioEnetBase, unsigned long reg_offset, unsigned short data )
{
  outw_le(ioEnetBase + reg_offset, data);
}


static __inline__
volatile unsigned short ReadBigMacRegister(vm_offset_t ioEnetBase, unsigned long reg_offset )
{
  return inw_le(ioEnetBase + reg_offset); 
}

static void
bmac_resetChip(bmac_t *bmac)
{
    volatile unsigned long  heathrowFCR;
    unsigned long		fcrValue;
	 
    dbdma_reset(bmac->ioBaseEnetRxDMA);
    dbdma_reset(bmac->ioBaseEnetTxDMA);

    heathrowFCR = (unsigned long)((unsigned char *)bmac->ioBaseHeathrow + kHeathrowFCR);

    fcrValue = inl_le(heathrowFCR);

    fcrValue &= kDisableEnet;							// clear out Xvr and Controller Bit
    outl_le(heathrowFCR, fcrValue);
    delay(50000);

    fcrValue |= kResetEnetCell;							// set bit to reset them
    outl_le(heathrowFCR,fcrValue);
    delay(50000);
	
    fcrValue &= kDisableEnet;
    outl_le(heathrowFCR, fcrValue);
    delay(50000);

    fcrValue |= kEnetEnabledBits;
    outl_le(heathrowFCR, fcrValue);
    delay(50000);
	
    outl_le(heathrowFCR, fcrValue);
}

static void
bmac_initRegisters(bmac_t *bmac)
{
    volatile unsigned short		regValue;
    unsigned short			*pWord16;

    WriteBigMacRegister(bmac->ioBaseEnet, kTXRST, kTxResetBit);

    do	
    {
      regValue = ReadBigMacRegister(bmac->ioBaseEnet, kTXRST);		// wait for reset to clear..acknowledge
    } 
    while( regValue & kTxResetBit );

    WriteBigMacRegister(bmac->ioBaseEnet, kRXRST, kRxResetValue);

    WriteBigMacRegister(bmac->ioBaseEnet, kXCVRIF, kClkBit | kSerialMode | kCOLActiveLow);	

    WriteBigMacRegister(bmac->ioBaseEnet, kRSEED, (unsigned short) isync_mfdec());		

    regValue = ReadBigMacRegister(bmac->ioBaseEnet, kXIFC);
    regValue |= kTxOutputEnable;
    WriteBigMacRegister(bmac->ioBaseEnet, kXIFC, regValue);

    ReadBigMacRegister(bmac->ioBaseEnet, kPAREG);

    // set collision counters to 0
    WriteBigMacRegister(bmac->ioBaseEnet, kNCCNT, 0);
    WriteBigMacRegister(bmac->ioBaseEnet, kNTCNT, 0);
    WriteBigMacRegister(bmac->ioBaseEnet, kEXCNT, 0);
    WriteBigMacRegister(bmac->ioBaseEnet, kLTCNT, 0);

    // set rx counters to 0
    WriteBigMacRegister(bmac->ioBaseEnet, kFRCNT, 0);
    WriteBigMacRegister(bmac->ioBaseEnet, kLECNT, 0);
    WriteBigMacRegister(bmac->ioBaseEnet, kAECNT, 0);
    WriteBigMacRegister(bmac->ioBaseEnet, kFECNT, 0);
    WriteBigMacRegister(bmac->ioBaseEnet, kRXCV, 0);

    // set tx fifo information
    WriteBigMacRegister(bmac->ioBaseEnet, kTXTH, 4);					// 4 octets before tx starts

    WriteBigMacRegister(bmac->ioBaseEnet, kTXFIFOCSR, 0);				// first disable txFIFO
    WriteBigMacRegister(bmac->ioBaseEnet, kTXFIFOCSR, kTxFIFOEnable );

    // set rx fifo information
    WriteBigMacRegister(bmac->ioBaseEnet, kRXFIFOCSR, 0);				// first disable rxFIFO
    WriteBigMacRegister(bmac->ioBaseEnet, kRXFIFOCSR, kRxFIFOEnable ); 

    //WriteBigMacRegister(bmac->ioBaseEnet, kTXCFG, kTxMACEnable);			// kTxNeverGiveUp maybe later
    ReadBigMacRegister(bmac->ioBaseEnet, kSTAT);					// read it just to clear it

      WriteBigMacRegister(bmac->ioBaseEnet, kINTDISABLE, kNormalIntEvents);

    // zero out the chip Hash Filter registers
    WriteBigMacRegister(bmac->ioBaseEnet, kHASH3, bmac->hashTableMask[0]); 	// bits 15 - 0
    WriteBigMacRegister(bmac->ioBaseEnet, kHASH2, bmac->hashTableMask[1]); 	// bits 31 - 16
    WriteBigMacRegister(bmac->ioBaseEnet, kHASH1, bmac->hashTableMask[2]); 	// bits 47 - 32
    WriteBigMacRegister(bmac->ioBaseEnet, kHASH0, bmac->hashTableMask[3]); 	// bits 63 - 48
	
    pWord16 = (unsigned short *)bmac->myAddress;
    WriteBigMacRegister(bmac->ioBaseEnet, kMADD0, *pWord16++);
    WriteBigMacRegister(bmac->ioBaseEnet, kMADD1, *pWord16++);
    WriteBigMacRegister(bmac->ioBaseEnet, kMADD2, *pWord16);
    

    WriteBigMacRegister(bmac->ioBaseEnet, kRXCFG, kRxCRCEnable | kRxHashFilterEnable | kRxRejectOwnPackets);
    
    return;
}


static void
bmac_disableAdapterInterrupts(bmac_t *bmac)
{
    WriteBigMacRegister( bmac->ioBaseEnet, kINTDISABLE, kNoEventsMask );
}

static void
bmac_enableAdapterInterrupts(bmac_t *bmac)
{
    WriteBigMacRegister( bmac->ioBaseEnet, kINTDISABLE, kNormalIntEvents );
}


static void
bmac_startChip(bmac_t *bmac)
{
    unsigned short	oldConfig;

    // enable rx dma channel
    dbdma_continue(bmac->ioBaseEnetRxDMA);
  
    // turn on rx plus any other bits already on (promiscuous possibly)
    oldConfig = ReadBigMacRegister(bmac->ioBaseEnet, kRXCFG);		
    WriteBigMacRegister(bmac->ioBaseEnet, kRXCFG, oldConfig | kRxMACEnable ); 
 
    oldConfig = ReadBigMacRegister(bmac->ioBaseEnet, kTXCFG);		
    WriteBigMacRegister(bmac->ioBaseEnet, kTXCFG, oldConfig | kTxMACEnable );  
}

static boolean_t
bmac_initChip(bmac_t *bmac)
{
    bmac_initRegisters(bmac);

    return TRUE;
}


static void
bmac_enablePromiscuousMode(bmac_t *bmac)
{
    unsigned short	rxCFGVal;
    
    if (bmac->isPromiscuous == FALSE) {
    	bmac->isPromiscuous = TRUE;
    
    	rxCFGVal = ReadBigMacRegister(bmac->ioBaseEnet, kRXCFG );
    	rxCFGVal |= kRxPromiscEnable;	
    	WriteBigMacRegister(bmac->ioBaseEnet, kRXCFG, rxCFGVal );		
    }
}

static void
bmac_disablePromiscuousMode(bmac_t *bmac)
{
    unsigned short		rxCFGVal;
    
    if (bmac->isPromiscuous) {
    	bmac->isPromiscuous = FALSE;
    
    	rxCFGVal = ReadBigMacRegister(bmac->ioBaseEnet, kRXCFG);
    	rxCFGVal &= ~kRxPromiscEnable;	
    	WriteBigMacRegister(bmac->ioBaseEnet, kRXCFG, rxCFGVal );
    }

    return;
}

static boolean_t
bmac_enableMulticastMode(bmac_t *bmac)
{
    bmac->multicastEnabled = TRUE;
    return TRUE;
}
	 
static void
bmac_disableMulticastMode(bmac_t *bmac)
{
    bmac->multicastEnabled = FALSE;
}


static dbdma_command_t	dbdmaCmd_Nop;
static dbdma_command_t	dbdmaCmd_NopWInt;	
static dbdma_command_t	dbdmaCmd_Stop;
static dbdma_command_t	dbdmaCmd_Branch;

/*
 * Breaks up an ethernet data buffer into two physical chunks. We know that
 * the buffer can't straddle more than two pages. If the content of paddr2 is
 * zero this means that all of the buffer lies in one physical page. Note
 * that we use the fact that tx and rx descriptors have the same size and
 * same layout of relevent fields (data address and count). 
 */

static void
bmac_construct_xmt(io_req_t nb, dbdma_command_t *desc, char *doubleBuf)
{
    vm_offset_t pageBreak;
    vm_offset_t vaddr;
    unsigned long   	paddr;
    unsigned long 	size;            
    
    size = nb->io_count;
    vaddr = (vm_offset_t) nb->io_data;

    paddr = kvtophys(vaddr);
    pageBreak = round_page(vaddr);

    /*
     * 95% of the case MACH will send the driver a 
     * physically contiguous network buffer.. however,
     * do some double buffering just in case.
     * (BMAC really doesn't like scattered packets ..)
     */

    if (trunc_page(vaddr) != trunc_page(vaddr+size)
    &&	round_page(paddr) != kvtophys(pageBreak))	
    {    
      paddr = kvtophys((vm_offset_t) doubleBuf);
      bcopy_nc((char *) nb->io_data, doubleBuf, size);
    } else
	flush_dcache(vaddr, size, FALSE);
   
    DBDMA_BUILD(desc, DBDMA_CMD_OUT_LAST,
                   (DBDMA_KEY_STREAM0),
                   (size), paddr,
                   (DBDMA_INT_ALWAYS),
                   (DBDMA_WAIT_IF_FALSE),
                   (DBDMA_BRANCH_NEVER) ); 
  
    eieio();	// Make sure everything is flushed out.. 
}

static void
bmac_construct_rxbuf(char *addr, dbdma_command_t *desc)
{
    DBDMA_BUILD(desc, DBDMA_CMD_IN_LAST, (DBDMA_KEY_STREAM0),
                   (ETHERNET_BUF_SIZE),
                   (kvtophys((vm_offset_t) addr)),
                   (DBDMA_INT_ALWAYS), (DBDMA_WAIT_NEVER),
                   (DBDMA_BRANCH_NEVER) ); 
  
    return;
}


static unsigned char reverseBitOrder(unsigned char data )
{
    unsigned char		val;
    int			i;

    for ( i=0; i < 8; i++ )
    {
      val <<= 1;
      if (data & 1) val |= 1;
      data >>= 1;
    }
    return( val );
}      
    

static boolean_t
bmac_allocateMemory(bmac_t *bmac)
{
    int				maxDMACommands;

    /*-------------------------------------------*/
    /*    Init receive channel DBDMA pointers    */
    /*-------------------------------------------*/  
    bmac->dmaCommands = (dbdma_command_t *) io_map(0, PAGE_SIZE);

    bmac->maxDMACommands = PAGE_SIZE / sizeof(enet_command_t);
  
    if ( (RX_RING_LENGTH + TX_RING_LENGTH + 4) > bmac->maxDMACommands )  {
      panic("Could not fit commands into a page!\n");
      return FALSE;
    }

    bmac->rxDMACommands = bmac->dmaCommands;
    bmac->rxMaxCommand  = RX_RING_LENGTH;

    bmac->txDMACommands = (bmac->dmaCommands + RX_RING_LENGTH + 2);	
    bmac->txMaxCommand  = TX_RING_LENGTH;

    DBDMA_BUILD( (&dbdmaCmd_Nop),
                  DBDMA_CMD_NOP,
                  DBDMA_KEY_STREAM0,
                  0,
                  0,
                  DBDMA_INT_NEVER,
                  DBDMA_WAIT_NEVER,
                  DBDMA_BRANCH_NEVER);

    DBDMA_BUILD( (&dbdmaCmd_NopWInt), 
	          DBDMA_CMD_NOP, 
		  DBDMA_KEY_STREAM0, 
     		  0, 
		  0, 
		  DBDMA_INT_ALWAYS, 
	  	  DBDMA_WAIT_NEVER, 
		  DBDMA_BRANCH_NEVER);

    DBDMA_BUILD( (&dbdmaCmd_Stop),
                  DBDMA_CMD_STOP,
                  DBDMA_KEY_STREAM0,
                  0,
                  0,
                  DBDMA_INT_NEVER,
                  DBDMA_WAIT_NEVER,
                  DBDMA_BRANCH_NEVER);

    DBDMA_BUILD( (&dbdmaCmd_Branch),
                  DBDMA_CMD_NOP,
                  DBDMA_KEY_STREAM0,
                  0,
                  0,
                  DBDMA_INT_NEVER,
                  DBDMA_WAIT_NEVER,
                  DBDMA_BRANCH_ALWAYS);

    return TRUE;
}


static void
bzero_nc(void *_ptr, int len)
{
	char	*ptr = _ptr;

	while (len-- > 0)
		*ptr++ = 0;
}

static boolean_t
bmac_initTxRing(bmac_t *bmac)
{
    boolean_t			kr;
    unsigned long			buf_phys;
    int		i;
    char	*addr;

    if (bmac->txDoubleBuffers[0] == NULL) {
	addr = (char *) io_map(0, ETHERMTU * bmac->txMaxCommand);

	for (i = 0; i < bmac->txMaxCommand; i++, addr += ETHERMTU) 
		bmac->txDoubleBuffers[i] = addr;
    }

    bzero_nc( (void *)bmac->txDMACommands,
			sizeof(dbdma_command_t) * bmac->txMaxCommand);

    bmac->txCommandHead = 0;
    bmac->txCommandTail = 0;

    bmac->txDMACommands[bmac->txMaxCommand] = dbdmaCmd_Branch; 

    buf_phys = kvtophys((vm_offset_t) bmac->txDMACommands);

    DBDMA_ST4_ENDIAN(((unsigned long *)&bmac->txDMACommands[bmac->txMaxCommand].d_cmddep), buf_phys);
 
    dbdma_reset(bmac->ioBaseEnetTxDMA);
    dbdma_st4_endian((&bmac->ioBaseEnetTxDMA->d_cmdptrlo), buf_phys);
    dbdma_st4_endian((&bmac->ioBaseEnetTxDMA->d_wait), 0x00200020);
    return TRUE;
}

static boolean_t
bmac_initRxRing(bmac_t *bmac)
{
    int 		i;
    boolean_t		status;
    unsigned long   	buf_phys;
    char		*bufAddr;

    bzero_nc((void *)bmac->rxDMACommands,
			sizeof(dbdma_command_t ) * bmac->rxMaxCommand);

    if (bmac->rxNetbuf[0] == NULL) { 
	bufAddr = (char *)io_map(0, (bmac->rxMaxCommand - 1)*(ETHERNET_BUF_SIZE));
	
	for (i = 0; i < bmac->rxMaxCommand; i ++, bufAddr += (ETHERNET_BUF_SIZE))
		bmac->rxNetbuf[i] = bufAddr;
    }

    for (i = 0; i < bmac->rxMaxCommand; i++) 
      bmac_construct_rxbuf(bmac->rxNetbuf[i], &bmac->rxDMACommands[i]);
    
    bmac->rxCommandHead = 0;
    bmac->rxCommandTail = i;

    bmac->rxDMACommands[i] = dbdmaCmd_Branch; 

    buf_phys = kvtophys((vm_offset_t) bmac->rxDMACommands);

    DBDMA_ST4_ENDIAN(((unsigned long *)&bmac->rxDMACommands[i].d_cmddep), buf_phys);
 
    dbdma_reset(bmac->ioBaseEnetRxDMA);
    DBDMA_ST4_ENDIAN((&bmac->ioBaseEnetRxDMA->d_cmdptrlo), buf_phys);

    return TRUE;
}

static void
bmac_performLoopback(bmac_t *bmac, io_req_t packet)
{
	ipc_kmsg_t kmsg;
	struct ether_header *ehp;
	struct packet_header *pkt;

	kmsg = net_kmsg_get();
	if (kmsg == IKM_NULL) {
		/*
		 * Drop the packet.
		 */
		return;
	}

	ehp = (struct ether_header *) (&net_kmsg(kmsg)->header[0]);
	pkt = (struct packet_header *) (&net_kmsg(kmsg)->packet[0]);
	bcopy(packet->io_data, (char *)ehp, sizeof(struct ether_header));
	bcopy(&packet->io_data[sizeof(struct ether_header)], (char *)(pkt + 1),
	      packet->io_count - sizeof(struct ether_header));
	pkt->type = ehp->ether_type;
	pkt->length = packet->io_count - sizeof(struct ether_header)
			+ sizeof(struct packet_header);

	if (pkt->length < ETHERMIN + sizeof(struct ether_header) + sizeof(struct packet_header))
		pkt->length = ETHERMIN + sizeof(struct ether_header) + sizeof(struct packet_header);
	net_packet(&bmac->networkInterface, kmsg, pkt->length, ethernet_priority(kmsg), packet);

}

static boolean_t
bmac_transmitPacket(bmac_t *bmac, io_req_t packet)
{
    dbdma_command_t		*cmd;
    unsigned long		i, cmdvalue;
    struct ether_header	*hdr;


    i = bmac->txCommandTail + 1;
    if ( i >= bmac->txMaxCommand ) i = 0;
    if ( i == bmac->txCommandHead )
    {
	iodone(packet);
	return FALSE;
    }

    cmd = &bmac->txDMACommands[i];

    *cmd = dbdmaCmd_Stop; eieio();

    bmac_construct_xmt(packet, &bmac->txDMACommands[bmac->txCommandTail],
				bmac->txDoubleBuffers[bmac->txCommandTail]);

    bmac->txNetbuf[bmac->txCommandTail] = packet;

    bmac->txCommandTail = i;

    dbdma_continue(bmac->ioBaseEnetTxDMA);

    hdr = (struct ether_header *) packet->io_data;
    if ((hdr->ether_dhost[0] == 0xFF &&
        hdr->ether_dhost[1] == 0xFF &&
        hdr->ether_dhost[2] == 0xFF &&
        hdr->ether_dhost[3] == 0xFF &&
         hdr->ether_dhost[4] == 0xFF &&
         hdr->ether_dhost[5] == 0xFF) ||
        (packet->io_device != DEVICE_NULL &&
         packet->io_device->open_count > 1 &&
         hdr->ether_dhost[0] == bmac->myAddress[0] &&
         hdr->ether_dhost[1] == bmac->myAddress[1] &&
         hdr->ether_dhost[2] == bmac->myAddress[2] &&
         hdr->ether_dhost[3] == bmac->myAddress[3] &&
         hdr->ether_dhost[4] == bmac->myAddress[4] &&
         hdr->ether_dhost[5] == bmac->myAddress[5]) ||
        (bmac->isPromiscuous) ||
        (bmac->resetAndEnabled && (hdr->ether_dhost[0] & 1)))
    		bmac_performLoopback(bmac, packet);

    return TRUE;
}	


static boolean_t
bmac_receivePackets(bmac_t *bmac, boolean_t timeout)
{
    dbdma_command_t      tmpCommand;
    ipc_kmsg_t		packet;
    struct ether_header	*ehp;
    struct packet_header *header;
    unsigned long           i,j,k, last;
    int			receivedFrameSize;
    unsigned long           dmaResid, dmaStatus;
    unsigned short           rxPktStatus;
    boolean_t		passPacketUp;
    boolean_t		status;
    boolean_t		reusePkt;
    dbdma_regmap_t      *dmap = DBDMA_REGMAP(DBDMA_ETHERNET_RV);
    unsigned long           *p;
    
   
    last      = -1;  
    i         = bmac->rxCommandHead;

    while ( 1 )
    {
      passPacketUp = FALSE;
      reusePkt     = FALSE;

      dmaResid = dbdma_ld4_endian(&bmac->rxDMACommands[i].d_status_resid);
      dmaStatus = dmaResid >> 16;
      dmaResid &= 0x0000ffff;
 
      if ( !(dmaStatus & DBDMA_CNTRL_ACTIVE) )
        break;

#if 0
      rxPktStatus = *(unsigned short *)((unsigned char *)bmac->rxNetbuf[i] + ETHERNET_BUF_SIZE  - dmaResid[0] - dmaResid[1] - 2);
      receivedFrameSize = rxPktStatus & kRxLengthMask;
#else
      rxPktStatus =0;
      receivedFrameSize = ETHERNET_BUF_SIZE - dmaResid - 2;
#endif


      if ( receivedFrameSize < (ETHERMINPACKET - ETHERCRC) || rxPktStatus & kRxAbortBit )
      
	packet = NULL;
      else 
	packet = net_kmsg_get();

      if (packet) {
	receivedFrameSize -= ETHERCRC +sizeof(struct ether_header);

	ehp = (struct ether_header *) (&net_kmsg(packet)->header[0]);
	header  = (struct packet_header *) (&net_kmsg(packet)->packet[0]);

	bcopy_nc((void *) bmac->rxNetbuf[i], (void *)ehp, sizeof(*ehp));
	bcopy_nc((void *) (bmac->rxNetbuf[i] + sizeof(struct ether_header)),
		(void *) (header + 1), receivedFrameSize);
	header->type = ehp->ether_type;
	header->length = receivedFrameSize + sizeof(struct packet_header);
	net_packet(&bmac->networkInterface, packet, header->length,
                                   ethernet_priority(packet), (io_req_t)0);
      }

      bmac->rxDMACommands[i].d_status_resid = 0; eieio();

      last = i;
      i++;
      if (i >= bmac->rxMaxCommand)
      { 
        i = 0;
      }

    }

    if ( last != -1 )
    {
#if 0
      char	*newPacket;
      enet_command_t	tmpCommand;

      newPacket              		= bmac->rxNetbuf[last];
      tmpCommand          		= bmac->rxDMACommands[last];
      bmac->rxDMACommands[last].desc_seg[0] 	= dbdmaCmd_Stop;
      bmac->rxDMACommands[last].desc_seg[1]   = dbdmaCmd_Nop;  
      bmac->rxNetbuf[last]      = 0;

      bcopy_nc((void *) ((unsigned long *)&tmpCommand+1),
             (void *)((unsigned long *)&bmac->rxDMACommands[bmac->rxCommandTail]+1),
             sizeof(dbdma_command_t)-sizeof(unsigned long) );

      bmac->rxNetbuf[bmac->rxCommandTail] = newPacket;

      bmac->rxDMACommands[bmac->rxCommandTail].desc_seg[0].d_cmd_count = tmpCommand.desc_seg[0].d_cmd_count;
#endif
      bmac->rxCommandTail = last;
      bmac->rxCommandHead = i;
    }

    dbdma_continue(bmac->ioBaseEnetRxDMA);

    return TRUE;
}
 
static void
bmac_receiveInterruptOccurred(int unit, void *ssp)
{
	bmac_t	*bmac = bmacs[0];

	simple_lock(&bmac->lock);
	bmac_receivePackets(bmac, FALSE);
	simple_unlock(&bmac->lock);
}

static void
bmac_transmitInterruptOccurred(int unit, void *ssp)
{
    unsigned long		dmaStatus, dmaCmd;
    bmac_t	*bmac		=bmacs[0];

    simple_lock(&bmac->lock);
    ReadBigMacRegister(bmac->ioBaseEnet, kSTAT);
    while ( 1 )
    {
      dmaCmd = dbdma_ld4_endian(&bmac->txDMACommands[bmac->txCommandHead].d_status_resid);
      dmaStatus = dmaCmd >> 16;

      if ( !(dmaStatus & DBDMA_CNTRL_ACTIVE))
      {
        break;
      }

      if (bmac->txNetbuf[bmac->txCommandHead]) 
      	iodone(bmac->txNetbuf[bmac->txCommandHead]);

      bmac->txNetbuf[bmac->txCommandHead] = NULL;

      if ( ++bmac->txCommandHead >= bmac->txMaxCommand ) bmac->txCommandHead = 0;
      if (bmac->txCommandTail == bmac->txCommandHead)
	  break;
    }

    simple_unlock(&bmac->lock);

    bmac_start(0);
    return; 
}
    


#define ENET_CRCPOLY 0x04c11db7

/* Real fast bit-reversal algorithm, 6-bit values */
static int reverse6[] = 
{	0x0,0x20,0x10,0x30,0x8,0x28,0x18,0x38,
	0x4,0x24,0x14,0x34,0xc,0x2c,0x1c,0x3c,
	0x2,0x22,0x12,0x32,0xa,0x2a,0x1a,0x3a,
	0x6,0x26,0x16,0x36,0xe,0x2e,0x1e,0x3e,
	0x1,0x21,0x11,0x31,0x9,0x29,0x19,0x39,
	0x5,0x25,0x15,0x35,0xd,0x2d,0x1d,0x3d,
	0x3,0x23,0x13,0x33,0xb,0x2b,0x1b,0x3b,
	0x7,0x27,0x17,0x37,0xf,0x2f,0x1f,0x3f
};

static
unsigned long crc416(unsigned int current, unsigned short nxtval )
{
    register unsigned int counter;
    register int highCRCBitSet, lowDataBitSet;

    /* Swap bytes */
    nxtval = ((nxtval & 0x00FF) << 8) | (nxtval >> 8);

    /* Compute bit-by-bit */
    for (counter = 0; counter != 16; ++counter)
    {	/* is high CRC bit set? */
      if ((current & 0x80000000) == 0)	
        highCRCBitSet = 0;
      else
        highCRCBitSet = 1;
		
      current = current << 1;
	
      if ((nxtval & 0x0001) == 0)
        lowDataBitSet = 0;
      else
	lowDataBitSet = 1;

      nxtval = nxtval >> 1;
	
      /* do the XOR */
      if (highCRCBitSet ^ lowDataBitSet)
        current = current ^ ENET_CRCPOLY;
    }
    return current;
}

static
unsigned long mace_crc(unsigned short *address)
{	
    register unsigned long newcrc;

    newcrc = crc416(0xffffffff, *address);	/* address bits 47 - 32 */
    newcrc = crc416(newcrc, address[1]);	/* address bits 31 - 16 */
    newcrc = crc416(newcrc, address[2]);	/* address bits 15 - 0  */

    return(newcrc);
}

/*
 * Add requested mcast addr to BMac's hash table filter.  
 *  
 */

static void
bmac_addToHashTableMask(bmac_t *bmac, unsigned char *addr)
{	
    unsigned long	 crc;
    unsigned short	 mask;

    crc = mace_crc((unsigned short *)addr)&0x3f; /* Big-endian alert! */
    crc = reverse6[crc];	/* Hyperfast bit-reversing algorithm */
    if (bmac->hashTableUseCount[crc]++)	
      return;			/* This bit is already set */
    mask = crc % 16;
    mask = (unsigned char)1 << mask;
    bmac->hashTableUseCount[crc/16] |= mask;
}

static void
bmac_removeFromHashTableMask(bmac_t *bmac, unsigned char *addr)
{	
    unsigned int crc;
    unsigned char mask;

    /* Now, delete the address from the filter copy, as indicated */
    crc = mace_crc((unsigned short *)addr)&0x3f; /* Big-endian alert! */
    crc = reverse6[crc];	/* Hyperfast bit-reversing algorithm */
    if (bmac->hashTableUseCount[crc] == 0)
      return;			/* That bit wasn't in use! */

    if (--bmac->hashTableUseCount[crc])
      return;			/* That bit is still in use */

    mask = crc % 16;
    mask = ((unsigned char)1 << mask) ^ 0xffff; /* To turn off bit */
    bmac->hashTableMask[crc/16] &= mask;
}

/*
 * Sync the adapter with the software copy of the multicast mask
 *  (logical address filter).
 */

static void
bmac_updateBMacHashTableMask(bmac_t *bmac)
{
    unsigned short 		rxCFGReg;

    rxCFGReg = ReadBigMacRegister(bmac->ioBaseEnet, kRXCFG);
    rxCFGReg &= ~kRxMACEnable;
    WriteBigMacRegister(bmac->ioBaseEnet, kRXCFG, rxCFGReg );
    do
    {
      rxCFGReg = ReadBigMacRegister(bmac->ioBaseEnet, kRXCFG);
    }
    while ( rxCFGReg & kRxMACEnable );

    WriteBigMacRegister(bmac->ioBaseEnet, kHASH3, bmac->hashTableMask[0]); 	// bits 15 - 0
    WriteBigMacRegister(bmac->ioBaseEnet, kHASH2, bmac->hashTableMask[1]); 	// bits 31 - 16
    WriteBigMacRegister(bmac->ioBaseEnet, kHASH1, bmac->hashTableMask[2]); 	// bits 47 - 32
    WriteBigMacRegister(bmac->ioBaseEnet, kHASH0, bmac->hashTableMask[3]); 	// bits 63 - 48

    rxCFGReg |= (kRxMACEnable | kRxHashFilterEnable);
    WriteBigMacRegister(bmac->ioBaseEnet, kRXCFG, rxCFGReg );
}

static void
bmac_addMulticastAddress(bmac_t *bmac, unsigned char *addr)
{
    bmac->multicastEnabled = FALSE;
    bmac_addToHashTableMask(bmac, addr);
    bmac_updateBMacHashTableMask(bmac);
}

static void
bmac_removeMulticastAddress(bmac_t *bmac, unsigned char *addr)
{
    bmac_removeFromHashTableMask(bmac, addr);
    bmac_updateBMacHashTableMask(bmac);
}

static void
bmac_timeoutOccurred(bmac_t *bmac)
{
      bmac_transmitInterruptOccurred(0, 0);
}

/*
 * Procedure for reading EEPROM 
 */
#define kSROMAddressLength		5
#define kDataInOn			0x0008
#define kDataInOff			0x0000
#define kClk				0x0002
#define kChipSelect			0x0001
#define kSDIShiftCount			3
#define kSD0ShiftCount			2
#define	kDelayValue			1000				// number of microseconds

#define kSROMStartOffset		10				// this is in words
#define kSROMReadCount			3				// number of words to read from SROM 

static unsigned char
bmac_clock_out_bit(vm_offset_t base)
{
    unsigned short         data;
    unsigned short         val;

    WriteBigMacRegister(base, kSROMCSR, kChipSelect | kClk);
    delay(kDelayValue);
    
    data = ReadBigMacRegister(base, kSROMCSR);
    delay(kDelayValue);
    val = (data >> kSD0ShiftCount) & 1;

    WriteBigMacRegister(base, kSROMCSR, kChipSelect);
    delay(kDelayValue);
    
    return val;
}

static void
bmac_clock_in_bit(vm_offset_t base, unsigned int val)
{
    unsigned short		data;    

    if (val != 0 && val != 1)	
    {
	return;
    }
    
    data = (val << kSDIShiftCount);
    WriteBigMacRegister(base, kSROMCSR, data | kChipSelect  );
    delay(kDelayValue);
    
    WriteBigMacRegister(base, kSROMCSR, data | kChipSelect | kClk );
    delay(kDelayValue);

    WriteBigMacRegister(base, kSROMCSR, data | kChipSelect);
    delay(kDelayValue);
}

static void
reset_and_select_srom(vm_offset_t base)
{
    /* first reset */
    WriteBigMacRegister(base, kSROMCSR, 0);
    delay(kDelayValue);
    
    /* send it the read command (110) */
    bmac_clock_in_bit(base, 1);
    bmac_clock_in_bit(base, 1);
    bmac_clock_in_bit(base, 0);
}

static unsigned short
read_srom(vm_offset_t base, unsigned int addr, unsigned int addr_len)
{
    unsigned short data, val;
    int i;
    
    /* send out the address we want to read from */
    for (i = 0; i < addr_len; i++)	{
	val = addr >> (addr_len-i-1);
	bmac_clock_in_bit(base, val & 1);
    }
    
    /* Now read in the 16-bit data */
    data = 0;
    for (i = 0; i < 16; i++)	{
	val = bmac_clock_out_bit(base);
	data <<= 1;
	data |= val;
    }
    WriteBigMacRegister(base, kSROMCSR, 0);
    
    return data;
}

/*
 * It looks like Cogent and SMC use different methods for calculating
 * checksums. What a pain.. 
 */

static boolean_t
bmac_verifyCheckSum(bmac_t *bmac)
{
    unsigned short data, storedCS;
    
    reset_and_select_srom(bmac->ioBaseEnet);
    data = read_srom(bmac->ioBaseEnet, 3, bmac->sromAddressBits);
    storedCS = ((data >> 8) & 0x0ff) | ((data << 8) & 0xff00);
    
    return TRUE;
}


static void
bmac_getStationAddress(bmac_t *bmac, unsigned char *ea)
{
    int i;
    unsigned short data;

    for (i = 0; i < 6; i++)	
    {
      reset_and_select_srom(bmac->ioBaseEnet);
      data = read_srom(bmac->ioBaseEnet, i + bmac->enetAddressOffset/2, bmac->sromAddressBits);
     ea[2*i]   = reverseBitOrder(data & 0x0ff);
     ea[2*i+1] = reverseBitOrder((data >> 8) & 0x0ff);
    }
}

static boolean_t
bmac_sendPacketDone(io_req_t r)
{
	io_req_free(r);
	return	TRUE;
}

static void
bmac_sendPacket(bmac_t *bmac, unsigned char *packet, int len)
{
	io_req_t	p;

	io_req_alloc(p);

	p->io_op |= IO_LOANED;
	p->io_data = (char *)packet;
	p->io_count = len;
	p->io_done = bmac_sendPacketDone;
	p->io_device = NULL;

	bmac_transmitPacket(bmac, p);
}

static boolean_t
bmac_resetAndEnable(bmac_t *bmac, boolean_t enable)
{

    bmac->resetAndEnabled = FALSE;

    //bmac_clearTimeout(bmac);

    bmac_resetChip(bmac);
    
    if (enable) 
    {
      if (!bmac_initRxRing(bmac) || !bmac_initTxRing(bmac))	
      {
        return FALSE;
      }

      if (!bmac_initChip(bmac)) 
      {
	return FALSE;
      }

        bmac_startChip(bmac);
	
    	bmac_enableAdapterInterrupts(bmac);

    	bmac->resetAndEnabled = TRUE;

      {
	 unsigned int	i;
         unsigned char		myPacket[100];

	 for (i = 0; i < sizeof(myPacket); i++)
		myPacket[i] = i;

         bmac_sendPacket(bmac, myPacket, sizeof(myPacket));
      }
      bmac->networkInterface.if_flags |= IFF_RUNNING;
    }

    return TRUE;
}

static int
bmac_probe(int port, struct bus_device *bus)
{
	int	unit = bus->unit;
	device_node_t	*dev;
	bmac_t		*bmac;

	if ((dev = find_devices("bmac")) == NULL)
		return 0;

	if (kmem_alloc(kernel_map, (vm_offset_t *)&bmac, sizeof(bmac_t)) != KERN_SUCCESS)
		return 1;

	bzero((void *)bmac, sizeof(*bmac));
	bmacs[unit] = bmac;


	bmac->ioBaseEnet      = io_map(dev->addrs[0].address, 4096);
	bmac->ioBaseEnetTxDMA = (dbdma_regmap_t *)io_map(dev->addrs[1].address, 4096);
	bmac->ioBaseEnetRxDMA = (dbdma_regmap_t *)io_map(dev->addrs[2].address, 4096);

	// Todo - FIX THIS..
	bmac->ioBaseHeathrow      = powermac_info.io_base_virt;


	bmac->sromAddressBits  = 6;
	bmac->enetAddressOffset = 20;

	simple_lock_init(&bmac->lock, ETAP_IO_CHIP);

	if (!bmac_resetAndEnable(bmac, FALSE))
		return 0;

	bmac_getStationAddress(bmac, bmac->myAddress);

	if (!bmac_verifyCheckSum(bmac))
		return 0;

	bmac_allocateMemory(bmac);
#if 0
	if (!bmac_resetAndEnable(bmac, TRUE))
		return 0;
#endif

	bmac->isPromiscuous = FALSE;
	bmac->multicastEnabled = FALSE;


	pmac_register_ofint(dev->intrs[0], SPLIMP, bmac_transmitInterruptOccurred);
	pmac_register_ofint(dev->intrs[2], SPLIMP, bmac_receiveInterruptOccurred);

	return 1;
}

static void
bmac_attach(struct bus_device *bus)
{
	bmac_t	*bmac = bmacs[bus->unit];
	struct	ifnet	*ifp = &bmac->networkInterface;

	ifp->if_unit = bus->unit;
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST|IFF_MULTICAST|IFF_ALLMULTI;
	ifp->if_header_size = sizeof(struct ether_header);
        ifp->if_header_format = HDR_ETHERNET;
        ifp->if_address_size = 6;
        ifp->if_address = (char *)&bmac->myAddress[0];
	if_init_queues(ifp);
	ifp->if_snd.ifq_maxlen = bmac->txMaxCommand < 16 ? 16 : bmac->txMaxCommand;

	printf(" ethernet address [%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x] ",
		bmac->myAddress[0], bmac->myAddress[1], bmac->myAddress[2],
		bmac->myAddress[3], bmac->myAddress[4], bmac->myAddress[5]);
}


io_return_t
bmac_open(dev_t dev, dev_mode_t flag, io_req_t ior)
{
	bmac_t	*bmac;
	int	unit;
	extern net_pool_t inline_pagepool;
	static	boolean_t	pagepool_initted = FALSE;
	spl_t	s;
	
	unit = minor(dev);

	if (unit < 0 || unit >= NBMAC || (bmac = bmacs[unit]) == NULL)
		return	D_DEVICE_DOWN;


	/*
	 * Populate immediately buffer pool, since we are
	 * are registering a network device.
	 */
	if (!pagepool_initted) {
		inline_pagepool->net_kmsg_max += bmac->rxMaxCommand;
		inline_pagepool->net_queue_free_min += bmac->rxMaxCommand / 2;
		net_kmsg_more(inline_pagepool);
		pagepool_initted = TRUE;
	}

	s = splhigh();
	simple_lock(&bmac->lock);

	bmac->networkInterface.if_flags |= IFF_UP;
	bmac_resetAndEnable(bmac, TRUE);

	simple_unlock(&bmac->lock);
	splx(s);

	return(0);
}

static void
bmac_start(int unit)
{
    bmac_t	*bmac = bmacs[unit];
    io_req_t	packet;
    unsigned int	i;
    spl_t	s;

    s = splimp();
    simple_lock(&bmac->lock);


    while (1)
    {
      i = bmac->txCommandTail + 1;
      if ( i >= bmac->txMaxCommand ) i = 0;

      if ( i == bmac->txCommandHead )
        break;

      IF_DEQUEUE(&bmac->networkInterface.if_snd, packet)

      if (packet == NULL) 
        break;

      bmac_transmitPacket(bmac, packet);
    }

    simple_unlock(&bmac->lock);
    splx(s);
}

io_return_t
bmac_getstat(dev_t dev, dev_flavor_t flavor, dev_status_t status, natural_t *count)
{
	int		unit = minor(dev);
	bmac_t		*bmac;

	if (unit < 0 || unit >= NBMAC || (bmac = bmacs[unit]) == NULL) 
	    return D_NO_SUCH_DEVICE;

	return (net_getstat(&bmac->networkInterface, flavor, status, count));
}

io_return_t
bmac_setstat(dev_t dev, dev_flavor_t flavor, dev_status_t status, natural_t count)
{
	int		unit = minor(dev);
	bmac_t		*bmac;

	spl_t	s;

	if (unit < 0 || unit >= NBMAC || (bmac = bmacs[unit]) == NULL) 
	    return D_NO_SUCH_DEVICE;


	s = splimp();
	simple_lock(&bmac->lock);

	switch (flavor) {
	case NET_STATUS:
	{
		struct net_status *ns = (struct net_status *)status;
		unsigned long	flags;

		if (count < NET_STATUS_COUNT) {
			simple_unlock(&bmac->lock);
			splx(s);
			return D_INVALID_SIZE;
		}

		flags = (bmac->networkInterface.if_flags & IFF_CANTCHANGE)
			| (ns->flags & ~IFF_CANTCHANGE);

		if (((flags & IFF_UP) & bmac->networkInterface.if_flags) == 0)
			bmac_resetAndEnable(bmac, TRUE);
		else if (((bmac->networkInterface.if_flags & IFF_UP) & flags) == 0)
			bmac_resetAndEnable(bmac, FALSE);

		if (flags & IFF_PROMISC)
			bmac_enablePromiscuousMode(bmac);
		else
			bmac_disablePromiscuousMode(bmac);

		bmac->networkInterface.if_flags = flags;
	}
		break;

	case NET_ADDRESS:
	    {
                register union ether_cvt {
                    char        addr[6];
                    int         lwd[2];
                } *ec = (union ether_cvt *)status;

                if (count < sizeof(*ec)/sizeof(int))  {
		    simple_unlock(&bmac->lock);
		    splx(s);
                    return (D_INVALID_SIZE);
		}
 
		bcopy((char *) ec->addr, (char *)bmac->myAddress,
						sizeof(bmac->myAddress));
		/* Reset the chip.. */
		bmac_resetAndEnable(bmac, TRUE);
		break;
	    }


	case NET_ADDMULTI:
	    {
		unsigned temp;
		boolean_t allmulti;

		if (count != ETHER_ADDR_SIZE * 2) {
		    simple_unlock(&bmac->lock);
		    splx(s);
		    return (D_INVALID_SIZE);
		}

		if ((((unsigned char *)status)[0] & 1) == 0 ||
		    ((unsigned char *)status)[6] !=
		    ((unsigned char *)status)[0]) {
		    simple_unlock(&bmac->lock);
		    return (KERN_INVALID_VALUE);
		}

		bmac_addMulticastAddress(bmac, (unsigned char *) status);
		break;
	    }

	    case NET_DELMULTI:
		if (count != ETHER_ADDR_SIZE * 2) {
		    simple_unlock(&bmac->lock);
		    splx(s);
		    return D_INVALID_SIZE;
		}

		bmac_removeMulticastAddress(bmac, (unsigned char *) status);
		break;

	default:
		simple_unlock(&bmac->lock);
		splx(s);
		return (D_INVALID_OPERATION);
	}

	simple_unlock(&bmac->lock);
	splx(s);
	return (D_SUCCESS);
}


io_return_t
bmac_output(dev_t dev, io_req_t ior)
{
	int		unit = minor(dev);
	bmac_t		*bmac;

	if (unit < 0 || unit >= NBMAC ||
	    (bmac = bmacs[unit]) == NULL) 
		return D_DEVICE_DOWN;
    
	
	return net_write(&bmac->networkInterface, bmac_start, ior);
}

io_return_t
bmac_setinput(dev_t dev, ipc_port_t receive_port, int priority, filter_t filter[], unsigned int filter_count, device_t device)
{
	int	unit = minor(dev);
	bmac_t	*bmac;

	if (unit < 0 || unit >= NBMAC || (bmac = bmacs[unit]) == NULL) 
	    return D_NO_SUCH_DEVICE;

	return (net_set_filter(&bmac->networkInterface,
			(ipc_port_t)receive_port, priority,
			filter, filter_count, device));

}


static caddr_t bmac_std[NBMAC] = { 0 };
static struct bus_device *bmac_info[NBMAC];
struct  bus_driver      bmac_driver =
        {(probe_t)bmac_probe, 0,
         (void (*)(struct bus_device*))bmac_attach, 0,
         bmac_std, "bmac", bmac_info };
#endif /* NBMAC > 0 */
