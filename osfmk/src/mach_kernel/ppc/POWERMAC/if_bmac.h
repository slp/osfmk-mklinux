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

// ---------------------------------------------------------------------------------------------
// Heathrow (F)eature (C)ontrol (R)egister Addresses
// ---------------------------------------------------------------------------------------------
#define kHeathrowFCR			0x0038			// FCR offset from Heathrow Base Address
#define kEnetEnabledBits		0x60000000		// mask to enable Enet Xcvr/Controller
#define kResetEnetCell			0x80000000		// mask used to reset Enet cell
#define kClearResetEnetCell		0x7fffffff		// mask used to clear reset Enet cell
#define kDisableEnet			0x1fffffff		// mask to disable Enet Xcvr/Controller


// ---------------------------------------------------------------------------------------------
//	BMAC & Heathrow I/O Addresses
// ---------------------------------------------------------------------------------------------
#define kTxDMABaseOffset		0x08200			// offset from Heathrow Base address
#define kRxDMABaseOffset		0x08300
#define kControllerBaseOffset		0x11000	


// ---------------------------------------------------------------------------------------------
//	BigMac Register Numbers & Bit Assignments
// ---------------------------------------------------------------------------------------------
#define kXIFC				0x0000 
#define	kTxOutputEnable			0x0001		
#define	kMIILoopbackBits		0x0006		
#define	kMIIBufferEnable		0x0008		
#define	kSQETestEnable			0x0010		
#define kSTAT				0x0200
#define kINTDISABLE			0x0210
#define	kIntFrameReceived		0x0001		
#define	kIntRxFrameCntExp		0x0002		
#define	kIntRxAlignCntExp		0x0004		
#define	kIntRxCRCCntExp			0x0008		
#define	kIntRxLenCntExp			0x0010		
#define	kIntRxOverFlow			0x0020		
#define	kIntRxCodeViolation		0x0040		
#define	kIntSQETestError		0x0080		
#define	kIntFrameSent			0x0100		
#define	kIntTxUnderrun			0x0200		
#define	kIntTxMaxSizeError		0x0400		
#define	kIntTxNormalCollExp		0x0800		
#define	kIntTxExcessCollExp		0x1000		
#define	kIntTxLateCollExp		0x2000		
#define	kIntTxNetworkCollExp		0x4000		
#define	kIntTxDeferTimerExp		0x8000
#define	kNormalIntEvents		~(0xFFFF & (kIntFrameReceived | kIntTxUnderrun | kIntFrameSent ) )
#if EXTRA_INTERRUPTS
  	#define kXtraInterrupts		~(0xFFFF & (kIntFrameReceived | kIntRxFrameCntExp \
									| kIntFrameSent | kIntTxUnderrun | kIntFrameSent) )
#endif
#define	kNoEventsMask			0xFFFF		
#define kTXRST				0x0420				
#define	kTxResetBit			0x0001		
#define kTXCFG				0x0430				
#define	kTxMACEnable			0x0001
#define kTxThreshold			0x0004		
#define	kTxNeverGiveUp			0x0400		
#define kIPG1				0x0440			
#define kIPG2				0x0450				
#define kALIMIT				0x0460				
#define kSLOT				0x0470				
#define kPALEN				0x0480			
#define kPAPAT				0x0490				
#define kTXSFD				0x04A0		
#define kJAM				0x04B0			
#define kTXMAX				0x04C0		
#define kTXMIN				0x04D0			
#define kPAREG				0x04E0				
#define kDCNT				0x04F0				
#define kNCCNT				0x0500				
#define kNTCNT				0x0510			
#define kEXCNT				0x0520			
#define kLTCNT				0x0530				
#define kRSEED				0x0540		
#define kTXSM				0x0550				
#define kRXRST				0x0620				
#define	kRxResetValue			0x0000		
#define kRXCFG				0x0630					
#define	kRxMACEnable			0x0001		
#define	kReservedValue			0x0004		
#define	kRxPromiscEnable		0x0040		
#define	kRxCRCEnable			0x0100		
#define	kRxRejectOwnPackets		0x0200
#define	kRxHashFilterEnable		0x0800		
#define	kRxAddrFilterEnable		0x1000		
#define kRXMAX				0x0640				
#define kRXMIN				0x0650			
#define kMADD2				0x0660			
#define kMADD1				0x0670			
#define kMADD0				0x0680				
#define kFRCNT				0x0690				
#define kLECNT				0x06A0			
#define kAECNT				0x06B0				
#define kFECNT				0x06C0				
#define kRXSM				0x06D0				
#define kRXCV				0x06E0			
#define kHASH3				0x0700				
#define kHASH2				0x0710					
#define kHASH1				0x0720					
#define kHASH0				0x0730
#define kAFR2				0x0740
#define kAFR1				0x0750	
#define kAFR0				0x0760	
#define kAFCR				0x0770	
#define	kEnableAllCompares		0x0fff
#define kTXFIFOCSR			0x0100	
#define	kTxFIFOEnable			0x0001		
#define	kTxFIFO128			0x0000		
#define kTXTH				0x0110			
#define kRXFIFOCSR			0x0120				
#define	kRxFIFOEnable			kTxFIFOEnable		
#define	kRxFIFO128			kTxFIFO128			
#define kMEMADD				0x0130				
#define kMEMDATAHI			0x0140			
#define kMEMDATALO			0x0150				
#define kXCVRIF				0x0160				
#define	kCOLActiveLow			0x0002
#define	kSerialMode			0x0004
#define	kClkBit				0x0008
#define	kLinkStatus			0x0100
#define kCHIPID				0x0170				
#define kMIFCSR				0x0180				
#define kSROMCSR			0x0190				
#define kTXPNTR				0x01A0				
#define kRXPNTR				0x01B0				

// ---------------------------------------------------------------------------------------------
//	Misc. Bit definitions for BMac Status word
// ---------------------------------------------------------------------------------------------
#define kRxAbortBit			0x8000			// status bit in BMac status for rx packets
#define kRxLengthMask			0x3FFF			// bits that determine length of rx packets


#define TX_RING_LENGTH		2
#define RX_RING_LENGTH		64

#define NETWORK_BUFSIZE		(ETHERMAXPACKET + ETHERCRC + 2)
#define TRANSMIT_QUEUE_SIZE		128
