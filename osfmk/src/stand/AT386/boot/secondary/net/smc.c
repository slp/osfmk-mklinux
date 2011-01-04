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

/*
 * File : smc.c
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains SMC boards functions used for Network bootstrap.
 */

#include <secondary/net.h>
#include <secondary/net/ns8390.h>
#include <secondary/net/3c503.h>
#include <secondary/net/smc.h>
#include <secondary/net/dlink.h>
#include <secondary/net/endian.h>
#include <secondary/protocols/udpip.h>
#include <secondary/protocols/arp.h>

#ifndef SMC_DEFAULT_RAM_BASE
#define	SMC_DEFAULT_RAM_BASE	0xCC000
#endif

i386_ioport_t smc_addr[] = {
	0x200,
	0x220,
	0x240,
	0x260,
	0x280,
	0x2A0,
	0x2C0,
	0x2E0,
	0x300,
	0x320,
	0x340,
	0x360,
	0x380,
	0x3A0,
	0x3C0,
	0x3E0,
	0,
};

extern  void ns8390_reset(void);

void	smc_reset(void);
void	smc_dis16bits(void);
void	smc_en16bits(void);
int	smc_check_for_585(i386_ioport_t);
int	smc_bid_slot_16bit(i386_ioport_t);
int	bid_check_bic_type(i386_ioport_t, u32bits *);
u32bits smc_bid_get_ram_size(i386_ioport_t, u8bits, u32bits);
void	smc_bid_wait_for_recall(i386_ioport_t);
void	smc_bid_recall_lan_address(i386_ioport_t);
void	smc_bid_recall_engr_eeprom(i386_ioport_t);
u32bits smc_bid_get_eeprom_info(i386_ioport_t, u32bits);
u32bits	smc_bid_get_id_byte_info(i386_ioport_t, u32bits);
u32bits smc_bid_get_media_type(i386_ioport_t, u8bits);
int 	smc_bid_check_for_690(i386_ioport_t);
int	smc_bid_board_16bit(i386_ioport_t);
int	smc_bid_interface_chip(i386_ioport_t);
int	smc_bid_check_aliasing(i386_ioport_t);
u32bits smc_bid_get_base_info(i386_ioport_t, u32bits);
u8bits  smc_bid_get_board_rev_number(i386_ioport_t);
u32bits	smc_gc_get_bid(i386_ioport_t, unsigned);
void	smc_gc_58x_ram_base(i386_ioport_t, u32bits);
void	smc_gc_copy_bid_ram_size(u32bits);
int	smc_gc_is_board_there(i386_ioport_t);
int 	smc_get_at_config(i386_ioport_t);
int	probe_smc(void);

void
smc_reset(void)
{
	u8bits temp;

	ns8390.ns_pstart = 0x06;
	if (ns8390.ns_ramsize == 0x10000)
		ns8390.ns_pstop = 0xFF;
	else
		ns8390.ns_pstop = (ns8390.ns_ramsize >> 8) & 0xFF;
	ns8390.ns_txt = 0;

	if (ns8390.ns_flags & NS8390_FLAGS_BOARD_16BITS)
		if (ns8390.ns_flags & NS8390_FLAGS_INTERFACE) {
			temp = (inb(ns8390.ns_iobase + SMC_CNFG_LAAR_584) &
				SMC_CNFG_LAAR_MASK) | SMC_LA19;
			delayed_outb(ns8390.ns_iobase + SMC_CNFG_LAAR_584, temp);
		} else
			delayed_outb(ns8390.ns_iobase + SMC_CNFG_LAAR_584, SMC_LA19);

	if (ns8390.ns_flags & NS8390_FLAGS_SLOT_16BITS)
		if (ns8390.ns_flags & NS8390_FLAGS_INTERFACE) {
			temp = (inb(ns8390.ns_iobase + SMC_CNFG_LAAR_584) &
				SMC_CNFG_LAAR_MASK) | SMC_LAN16ENB;
			delayed_outb(ns8390.ns_iobase + SMC_CNFG_LAAR_584, temp);
		} else
			delayed_outb(ns8390.ns_iobase + SMC_CNFG_LAAR_584,
			     SMC_LA19|SMC_LAN16ENB);
}

void
smc_en16bits(void)
{
	u8bits temp;

	if (ns8390.ns_flags & NS8390_FLAGS_SLOT_16BITS)
		if (ns8390.ns_flags & NS8390_FLAGS_INTERFACE) {
			temp = (inb(ns8390.ns_iobase + SMC_CNFG_LAAR_584) &
				SMC_CNFG_LAAR_MASK) |SMC_LAN16ENB|SMC_MEM16ENB;
			delayed_outb(ns8390.ns_iobase + SMC_CNFG_LAAR_584, temp);
		} else
			delayed_outb(ns8390.ns_iobase + SMC_CNFG_LAAR_584,
			     SMC_LA19|SMC_LAN16ENB|SMC_MEM16ENB);
}

void
smc_dis16bits(void)
{
	u8bits temp;

	if (ns8390.ns_flags & NS8390_FLAGS_SLOT_16BITS)
		if (ns8390.ns_flags & NS8390_FLAGS_INTERFACE) {
			temp = (inb(ns8390.ns_iobase + SMC_CNFG_LAAR_584) &
				SMC_CNFG_LAAR_MASK) | SMC_LAN16ENB;
			delayed_outb(ns8390.ns_iobase + SMC_CNFG_LAAR_584, temp);
		} else
			delayed_outb(ns8390.ns_iobase + SMC_CNFG_LAAR_584,
			     SMC_LA19|SMC_LAN16ENB);
}

/*
 * Check for presence of 585/790 interface.
 */
int
smc_check_for_585(i386_ioport_t addr)
{
	u8bits reg_temp;
	u8bits reg_save;
	unsigned i;

	reg_temp = inb(addr + SMC_REG_HWR);
        for (i = 0; i < 6; i++) {
		delayed_outb(addr + SMC_REG_HWR, (reg_temp &
					  (SMC_HWR_LPRM | SMC_HWR_GIN2 |
					   SMC_HWR_GIN1)) | SMC_HWR_SWH);
                reg_save = inb(addr + SMC_LAR_0 + i);
		delayed_outb(addr + SMC_REG_HWR,
		     reg_temp & (SMC_HWR_LPRM | SMC_HWR_GIN2 | SMC_HWR_GIN1));
                if (reg_save != inb(addr + SMC_LAR_0 + i)) {
			delayed_outb(addr + SMC_REG_HWR,
			     reg_temp & (SMC_HWR_SWH | SMC_HWR_LPRM |
					 SMC_HWR_GIN2 | SMC_HWR_GIN1));
			return (1);
		}
	}
	delayed_outb(addr + SMC_REG_HWR, reg_temp);
        return (0);
}

/*
 * Identify the NIC as a 690 or an 8390.
 */
int
smc_bid_check_for_690(i386_ioport_t addr)
{
	u8bits save_cr;
	u8bits save_tcr;
	int ret;

	save_cr = inb(addr + NS8390_CR) & (NS8390_TXP ^ 0xFF);
	save_cr = inb(addr + NS8390_CR) & (NS8390_TXP ^ 0xFF);
	delayed_outb(addr + NS8390_CR, (save_cr & NS8390_PS_MASK) | NS8390_PS2);

	save_tcr = inb(addr + NS8390_TCR);
	delayed_outb(addr + NS8390_CR, save_cr);
	delayed_outb(addr + NS8390_TCR, NS8390_ATD | NS8390_OFST);
	delayed_outb(addr + NS8390_CR, (save_cr & NS8390_PS_MASK) | NS8390_PS2);
	ret = (inb(addr + NS8390_TCR) &
	       (NS8390_ATD | NS8390_OFST)) == (NS8390_ATD | NS8390_OFST);
	delayed_outb(addr + NS8390_CR, save_cr);
	delayed_outb(addr + NS8390_TCR, save_tcr);
	return (!ret);
}

/*
 * Check for presence of 585/790 chip.
 */
int
bid_check_bic_type(i386_ioport_t addr,
		   u32bits *ret)
{
	if (smc_check_for_585(addr))
		return (0);

	switch (inb(addr + SMC_REG_REV) >> 4) {
	case 4:
		*ret = (SMC_BID_NIC_SUPERSET |
			SMC_BID_NIC_790_BIT | SMC_BID_INTERFACE_585_CHIP);
		break;
	case 2:
		*ret = SMC_BID_NIC_790_BIT | SMC_BID_INTERFACE_585_CHIP;
		break;
	case 3:
		*ret = SMC_BID_INTERFACE_585_CHIP;
		break;
	default:
		return (0);
	}
	return (1);
}

/*
 * Sense if this 16 bit board is in a 16 bit slot.
 */
int
smc_bid_slot_16bit(i386_ioport_t addr)
{
	if (smc_check_for_585(addr))
		return (inb(addr + SMC_REG_HWR) & SMC_HWR_HOST16);
	return (inb(addr + SMC_REG_1) & SMC_SIXTEEN_BIT_BIT);
}

/*
 * Figure out, if possible, the size of the RAM available on this board.
 */
u32bits
smc_bid_get_ram_size(i386_ioport_t addr,
		     u8bits rev,
		     u32bits local_bid)
{
	if (rev < 2) {
		if (local_bid & SMC_BID_MICROCHANNEL)
			return (SMC_BID_RAM_SIZE_16K);
		if ((local_bid & SMC_BID_BOARD_16BIT) == 0) {
			if ((local_bid & SMC_BID_INTERFACE_CHIP) == 0)
				return (SMC_BID_RAM_SIZE_UNKNOWN);
			if ((inb(addr + SMC_REG_1) & SMC_MSZ_583_BIT) == 0)
				return (SMC_BID_RAM_SIZE_8K);
			return (SMC_BID_RAM_SIZE_32K);
		}
		if ((local_bid & SMC_BID_SLOT_16BIT) == 0)
			return (SMC_BID_RAM_SIZE_8K);
		return (SMC_BID_RAM_SIZE_16K);
	}

	switch (local_bid & SMC_BID_BOARD_TYPE_MASK) {
	case SMC_WD8013EBT:
		if (local_bid & SMC_BID_SLOT_16BIT)
			break;
		/*FALL THRU*/
	case SMC_WD8003E:
	case SMC_WD8003S:
	case SMC_WD8003WT:
	case SMC_WD8003W:
	case SMC_WD8003EB:
		if (inb(addr + SMC_HID) & SMC_BID_RAM_SIZE_BIT)
			return (SMC_BID_RAM_SIZE_32K);
		else
			return (SMC_BID_RAM_SIZE_8K);
	}

	if ((local_bid & SMC_BID_BOARD_TYPE_MASK) == SMC_WD8013EBT ||
	    (local_bid & SMC_BID_MICROCHANNEL)) {
		if (inb(addr + SMC_HID) & SMC_BID_RAM_SIZE_BIT)
			return (SMC_BID_RAM_SIZE_32K);
		else
			return (SMC_BID_RAM_SIZE_8K);
	}
	return (SMC_BID_RAM_SIZE_UNKNOWN);
}

/*
 * Wait for the recall operation to complete.
 */
void
smc_bid_wait_for_recall(i386_ioport_t addr)
{
	while (inb(addr + SMC_REG_1) & SMC_RECALL_DONE_MASK)
		continue;
}

/*
 * Recall the LAN Address bytes from the EEPROM.
 */
void
smc_bid_recall_lan_address(i386_ioport_t addr)
{
	if (smc_check_for_585(addr)) {
		delayed_outb(addr + SMC_REG_HWR, (inb(addr + SMC_REG_HWR) &
					  (SMC_HWR_LPRM |
					   SMC_HWR_HWCS | SMC_HWR_CA)));
		delayed_outb(addr + SMC_REG_EER,
		     SMC_OFFSET_585_LAN_ADDR | SMC_EER_RC);
		while (inb(addr + SMC_REG_EER) & SMC_EER_RC)
			continue;
		return;
	}

	delayed_outb(addr + SMC_REG_1, (inb(addr + SMC_REG_1) & SMC_ICR_MASK) |
	     SMC_OTHER_BIT);
	delayed_outb(addr + SMC_REG_3, (inb(addr + SMC_REG_3) & SMC_EAR_MASK) |
	     SMC_EA6);
	delayed_outb(addr + SMC_REG_1, (inb(addr + SMC_REG_1) & SMC_ICR_MASK) |
	     SMC_RLA);
	smc_bid_wait_for_recall(addr);
}

/*
 * Recall the reserved Engineering bytes from the EEPROM.
 */
void
smc_bid_recall_engr_eeprom(i386_ioport_t addr)
{
	if (smc_check_for_585(addr)) {
		delayed_outb(addr + SMC_REG_EER,
		     SMC_OFFSET_585_ENGR_DATA | SMC_EER_RC);
		while (inb(addr + SMC_REG_EER) & SMC_EER_RC)
			continue;
		return;
	}

	delayed_outb(addr + SMC_REG_1, (inb(addr + SMC_REG_1) & SMC_ICR_MASK) |
	     SMC_OTHER_BIT);
	delayed_outb(addr + SMC_REG_3, (inb(addr + SMC_REG_3) & SMC_EAR_MASK) |
	     SMC_ENGR_PAGE);
	delayed_outb(addr + SMC_REG_1, (inb(addr + SMC_REG_1) & SMC_ICR_MASK) |
	     SMC_RLA | SMC_OTHER_BIT);
	smc_bid_wait_for_recall(addr);
}

/*
 * Extract information about the board using the ID bytes in the EEPROM.
 */
u32bits
smc_bid_get_eeprom_info(i386_ioport_t addr,
			u32bits local_bid)
{
	u32bits ret;
	u8bits eeprom;

	smc_bid_recall_engr_eeprom(addr);
	eeprom = inb(addr + SMC_EEPROM_1);

	switch (eeprom & SMC_EEPROM_BUS_TYPE_MASK) {
	case SMC_EEPROM_BUS_TYPE_MCA:
		ret = SMC_BID_MICROCHANNEL;
		break;
	case SMC_EEPROM_BUS_TYPE_NEC:
		ret = SMC_BID_NEC_BUS;
		break;
	default:
		ret = 0;
		break;
	}

	if (eeprom & SMC_EEPROM_RAM_PAGING)
		ret |= SMC_BID_PAGED_RAM;
	if (eeprom & SMC_EEPROM_ROM_PAGING)
		ret |= SMC_BID_PAGED_ROM;

	switch (eeprom & SMC_EEPROM_BUS_SIZE_MASK) {
	case SMC_EEPROM_BUS_SIZE_16BIT:
		ret |= SMC_BID_BOARD_16BIT;
		ns8390.ns_flags |= NS8390_FLAGS_BOARD_16BITS;
		if (smc_bid_slot_16bit(addr)) {
			ret |= SMC_BID_SLOT_16BIT;
			ns8390.ns_flags |= NS8390_FLAGS_SLOT_16BITS;
		}
		break;
	}

	eeprom = inb(addr + SMC_EEPROM_0);
	switch (eeprom & SMC_EEPROM_MEDIA_MASK) {
	case SMC_STARLAN_TYPE:
		ret |= SMC_BID_STARLAN_MEDIA;
		break;
	case SMC_TP_TYPE:
		ret |= SMC_BID_TWISTED_PAIR_MEDIA;
		break;
	case SMC_EW_TYPE:
		ret |= SMC_BID_EW_MEDIA;
		break;
	default:
		ret |= SMC_BID_ETHERNET_MEDIA;
		break;
	}
	switch (eeprom & SMC_EEPROM_IRQ_MASK) {
	case SMC_ALTERNATE_IRQ_1:
		ret |= SMC_BID_ALTERNATE_IRQ_BIT;
		break;
	}
	switch (eeprom & SMC_EEPROM_RAM_SIZE_MASK) {
	case SMC_EEPROM_RAM_SIZE_8K:
		ret |= SMC_BID_RAM_SIZE_8K;
		break;
	case SMC_EEPROM_RAM_SIZE_16K:
		if (((local_bid | ret) & 
		     (SMC_BID_BOARD_16BIT|SMC_BID_SLOT_16BIT)) ==
		    SMC_BID_BOARD_16BIT)
			ret |= SMC_BID_RAM_SIZE_8K;
		else
			ret |= SMC_BID_RAM_SIZE_16K;
		break;
	case SMC_EEPROM_RAM_SIZE_32K:
		ret |= SMC_BID_RAM_SIZE_32K;
		break;
	case SMC_EEPROM_RAM_SIZE_64K:
		if (((local_bid | ret) & 
		     (SMC_BID_BOARD_16BIT|SMC_BID_SLOT_16BIT)) ==
		    SMC_BID_BOARD_16BIT)
			ret |= SMC_BID_RAM_SIZE_32K;
		else
			ret |= SMC_BID_RAM_SIZE_64K;
		break;
	default:
		ret |= SMC_BID_RAM_SIZE_UNKNOWN;
		break;
	}

	eeprom = inb(addr + SMC_EEPROM_3);
	if (eeprom & SMC_EEPROM_LOW_COST)
		ret |= SMC_BID_LITE_VERSION;
	if (eeprom & SMC_EEPROM_HMI)
		ret |= SMC_BID_HMI_ADAPTER;

	smc_bid_recall_lan_address(addr);
	return (ret);
}

/*
 * Extract information about the board using the hardware ID byte
 *	in the boards LAN address ROM.
 */
u32bits
smc_bid_get_id_byte_info(i386_ioport_t addr,
			 u32bits local_bid)
{
	u8bits hib;
	u32bits ret;

	hib = inb(addr + SMC_HID);
	if (hib & SMC_BID_BUS_TYPE_BIT) {
		ret = SMC_BID_MICROCHANNEL;
	} else
		ret = 0;
	if (hib & SMC_BID_SOFT_CONFIG_BIT) {
		switch ((local_bid | ret) & SMC_BID_BOARD_TYPE_MASK) {
		case SMC_WD8003EB:
		case SMC_WD8003W:
			ret |= SMC_BID_ALTERNATE_IRQ_BIT;
			break;
		}
	}
	return (ret);
}

/*
 * Find the media type of the board.
 */
u32bits
smc_bid_get_media_type(i386_ioport_t addr,
		       u8bits rev)
{
	if (inb(addr + SMC_HID) & SMC_BID_MEDIA_TYPE_BIT)
		return (SMC_BID_ETHERNET_MEDIA);
	if (rev == 1)
		return (SMC_BID_STARLAN_MEDIA);
	return (SMC_BID_TWISTED_PAIR_MEDIA);
}

/*
 * Sense if this board has 16 bit capability.
 */
int
smc_bid_board_16bit(i386_ioport_t addr)
{
	u8bits save;

	save = inb(addr + SMC_REG_1);
	delayed_outb(addr + SMC_REG_1, save ^ SMC_SIXTEEN_BIT_BIT);
	(void)inb(addr + SMC_REG_0);
	if ((inb(addr + SMC_REG_1) & SMC_SIXTEEN_BIT_BIT) !=
	    (save & SMC_SIXTEEN_BIT_BIT)) {
		delayed_outb(addr + SMC_REG_1, save & ~SMC_SIXTEEN_BIT_BIT);
		return (1);
	}
	delayed_outb(addr + SMC_REG_1, save);
	return (0);
}

/*
 * Check for the presence of an interface chip.
 */
int
smc_bid_interface_chip(i386_ioport_t addr)
{
	u8bits save;

	save = inb(addr + SMC_GP2);
	delayed_outb(addr + SMC_GP2, 0x35);
	if (inb(addr + SMC_GP2) == 0x35) {
		delayed_outb(addr + SMC_GP2, 0x3A);
		(void)inb(addr + SMC_REG_0);
		if (inb(addr + SMC_GP2) == 0x3A) {
			delayed_outb(addr + SMC_GP2, save);
			return (1);
		}
	}
	return (smc_check_for_585(addr));
}

/*
 * Check for register aliasing.
 */
int
smc_bid_check_aliasing(i386_ioport_t addr)
{
	return (inb(addr + SMC_REG_1) == inb(addr + SMC_LAR_1) &&
		inb(addr + SMC_REG_2) == inb(addr + SMC_LAR_2) &&
		inb(addr + SMC_REG_3) == inb(addr + SMC_LAR_3) &&
		inb(addr + SMC_REG_4) == inb(addr + SMC_LAR_4) &&
		inb(addr + SMC_REG_5) == inb(addr + SMC_LAR_5) &&
		inb(addr + SMC_GP2)   == inb(addr + SMC_CKSUM));
}
/*
 * Identify which WD80XX board is being used.
 */
u32bits
smc_bid_get_base_info(i386_ioport_t addr,
		      u32bits current_bid)
{
	if (current_bid & SMC_BID_MICROCHANNEL) {
		if (!smc_bid_interface_chip(addr))
			return (0);
		ns8390.ns_flags |= NS8390_FLAGS_INTERFACE;
		return (SMC_BID_INTERFACE_CHIP);
	}
	if (smc_bid_check_aliasing(addr))
		return (0);
	if (smc_bid_interface_chip(addr)) {
		ns8390.ns_flags |= NS8390_FLAGS_INTERFACE;
		return (SMC_BID_INTERFACE_CHIP);
	}
	if (!smc_bid_board_16bit(addr))
		return (0);
	ns8390.ns_flags |= NS8390_FLAGS_BOARD_16BITS;
	if (!smc_bid_slot_16bit(addr))
		return (SMC_BID_BOARD_16BIT);
	ns8390.ns_flags |= NS8390_FLAGS_SLOT_16BITS;
	return (SMC_BID_BOARD_16BIT|SMC_BID_SLOT_16BIT);
}

/*
 * Find the board revision number from the hardware ID byte.
 */
u8bits
smc_bid_get_board_rev_number(i386_ioport_t addr)
{
	return ((inb(addr + SMC_HID) & SMC_BID_BOARD_REV_MASK) >> 1);
}

/*
 * Identify which WD80XX board is being used.
 */
u32bits
smc_gc_get_bid(i386_ioport_t addr,
	       unsigned mca)
{
	u8bits rev;
	u32bits local_bid;
	u32bits ret;

	if ((rev = smc_bid_get_board_rev_number(addr)) == 0)
		return 0;
	local_bid = mca;
	local_bid |= smc_bid_get_base_info(addr, local_bid);
	local_bid |= smc_bid_get_media_type(addr, rev);
	if (rev >= 2) {
		local_bid |= smc_bid_get_id_byte_info(addr, local_bid);
		if (rev >= 3) {
			if (local_bid & SMC_BID_MICROCHANNEL) {
				local_bid |= SMC_BID_INTERFACE_594_CHIP;
				local_bid |= smc_bid_get_ram_size(addr, rev,
								  local_bid);
			} else {
				local_bid &= SMC_BID_EEPROM_OVERRIDE;
				local_bid |= (smc_check_for_585(addr) ?
					      SMC_BID_INTERFACE_585_CHIP :
					      SMC_BID_INTERFACE_584_CHIP);
				local_bid |= smc_bid_get_eeprom_info(addr,
								     local_bid);
				if (rev >= 4) {
					local_bid |= SMC_BID_ADVANCED_FEATURES;
					if (bid_check_bic_type(addr, &ret)) {
						local_bid &=
						  ~SMC_BID_INTERFACE_CHIP_MASK;
						local_bid |= ret;
					}
				}
			}
		} else
			local_bid |= smc_bid_get_ram_size(addr, rev, local_bid);
	} else
		local_bid |= smc_bid_get_ram_size(addr, rev, local_bid);

	if (local_bid & SMC_BID_NIC_790_BIT)
		ns8390.ns_flags |= NS8390_FLAGS_WD83C690;
	else {
		/*
		 * Enable NIC access.
		 */
		if ((local_bid & SMC_BID_INTERFACE_CHIP) == 0)
			delayed_outb(addr + SMC_REG_0,
			     inb(addr + SMC_REG_0) & SMC_ADDR_MASK);

		if (smc_bid_check_for_690(addr + 0x10)) {
			local_bid |= SMC_BID_NIC_690_BIT;
			ns8390.ns_flags |= NS8390_FLAGS_WD83C690;
		}
	}
	return (local_bid);
}

/*
 * Set up RAM base address and enable shared memory.
 */
void
smc_gc_58x_ram_base(i386_ioport_t addr,
		    u32bits board_id)
{
	u8bits value;
	u8bits rar;

	if ((board_id & SMC_BID_INTERFACE_CHIP) == 0) {
		ns8390.ns_rambase = (void *)SMC_DEFAULT_RAM_BASE;
		delayed_outb(addr + SMC_REG_0, ((SMC_DEFAULT_RAM_BASE >> 13) &
					SMC_ADDR_MASK) | SMC_ADDR_MENB);
		return;
	}

	switch (board_id & SMC_BID_INTERFACE_CHIP_MASK) {
	case SMC_BID_INTERFACE_5X3_CHIP:
		value = inb(addr + SMC_REG_0) & SMC_ADDR_MASK;
		ns8390.ns_rambase = (void *)
			((value | (SMC_ADDR_MASK + 1)) << 13);
		delayed_outb(addr + SMC_REG_0, value | SMC_ADDR_MENB);
		break;

	case SMC_BID_INTERFACE_585_CHIP:
		value = inb(addr + SMC_REG_HWR) &
			(SMC_HWR_SWH|SMC_HWR_LPRM|SMC_HWR_GIN2|SMC_HWR_GIN1);
		delayed_outb(addr + SMC_REG_HWR, value | SMC_HWR_SWH);
		rar = inb(addr + SMC_REG_RAR);
		ns8390.ns_rambase = (void *)
			((rar & (SMC_RAR_RA16|SMC_RAR_RA15|
				 SMC_RAR_RA14|SMC_RAR_RA13)) << 13);
		if (rar & SMC_RAR_RA17)
			ns8390.ns_rambase = (void *)
				(((unsigned)ns8390.ns_rambase) | 0xD0000);
		else
			ns8390.ns_rambase = (void *)
				(((unsigned)ns8390.ns_rambase) | 0xC0000);
		if (rar & SMC_RAR_HRAM)
			ns8390.ns_rambase = (void *)
				(((unsigned)ns8390.ns_rambase) | 0xF00000);
		delayed_outb(addr + SMC_REG_HWR, value);
		delayed_outb(addr + SMC_REG_0,
		     (inb(addr + SMC_REG_0) & SMC_ADDR_MASK) | SMC_ADDR_MENB);
		break;

	case SMC_BID_INTERFACE_584_CHIP:
		value = inb(addr + SMC_REG_0) & SMC_ADDR_MASK;
		ns8390.ns_rambase = (void *)
			(((inb(addr + SMC_CNFG_LAAR_584) &
			   SMC_CNFG_LAAR_MASK) << 19) | (value << 13));
		delayed_outb(addr + SMC_REG_0, value | SMC_ADDR_MENB);
		break;
	}
}

/*
 * Set up RAM size.
 */
void
smc_gc_copy_bid_ram_size(u32bits board_id)
{
	if ((board_id & SMC_BID_INTERFACE_CHIP) == 0)
		ns8390.ns_ramsize = 8 * 1024;
	else
		switch (board_id & SMC_BID_RAM_SIZE_MASK) {
		case SMC_BID_RAM_SIZE_8K:
			ns8390.ns_ramsize = 8 * 1024;
			break;

		case SMC_BID_RAM_SIZE_16K:
			ns8390.ns_ramsize = 16 * 1024;
			break;

		case SMC_BID_RAM_SIZE_32K:
			ns8390.ns_ramsize = 32 * 1024;
			break;

		case SMC_BID_RAM_SIZE_64K:
			ns8390.ns_ramsize = 64 * 1024;
			break;
		}
}

/*
 * Given the I/O Base IsBoardThere check for an adapter by computing
 *	a checksum on the LAN address bytes.
 */
int
smc_gc_is_board_there(i386_ioport_t addr)
{
	u8bits cksum;

	cksum =  inb(addr + SMC_LAR_0);
	cksum += inb(addr + SMC_LAR_1);
	cksum += inb(addr + SMC_LAR_2);
	cksum += inb(addr + SMC_LAR_3);
	cksum += inb(addr + SMC_LAR_4);
	cksum += inb(addr + SMC_LAR_5);
	cksum += inb(addr + SMC_HID);
	cksum += inb(addr + SMC_CKSUM);

	return (cksum != 0xFF);
}

/*
 * Get AT bus configuration.
 */
int
smc_get_at_config(i386_ioport_t addr)
{
	u32bits board_id;

	if (smc_gc_is_board_there(addr)) {
		NPRINT(("Wrong SMC board Checksum @ 0x%x\n", addr));
		return (1);
	}
	ns8390.ns_iobase = addr;
	ns8390.ns_nicbase = addr + 0x10;

	board_id = smc_gc_get_bid(addr, 0);
	smc_gc_copy_bid_ram_size(board_id);
	smc_gc_58x_ram_base(addr, board_id);

	NPRINT(("Found @x%x SMC %x ",addr,ns8390.ns_rambase));
	switch (board_id & SMC_BID_BOARD_TYPE_MASK) {
	case SMC_WD8003E:
		NPRINT(("WD8003E or WD8003EBT"));
		break;
	case SMC_WD8003S:
		NPRINT(("WD8003S or WD8003SH"));
		break;
	case SMC_WD8003WT:
		NPRINT(("WD8003WT"));
		break;
	case SMC_WD8003W:
		NPRINT(("WD8003W"));
		break;
	case SMC_WD8003EB:
		if (board_id & SMC_BID_INTERFACE_584_CHIP) {
			NPRINT(("WD8003EP"));
		} else {
			NPRINT(("WD8003EB"));
		}
		break;
	case SMC_WD8003EW:
		NPRINT(("WD8003EW"));
		break;
	case SMC_WD8003ETA:
		NPRINT(("WD8003ETA"));
		break;
	case SMC_WD8003STA:
		NPRINT(("WD8003STA"));
		break;
	case SMC_WD8003EA:
		if (board_id & SMC_BID_INTERFACE_594_CHIP) {
			NPRINT(("WD8003EPA"));
		} else {
			NPRINT(("WD8013EA"));
		}
		break;
	case SMC_WD8003SHA:
		NPRINT(("WD8003SHA"));
		break;
	case SMC_WD8003WA:
		if (board_id & SMC_BID_INTERFACE_594_CHIP) {
			NPRINT(("WD8003WPA"));
		} else {
			NPRINT(("WD8013WA"));
		}
		break;
	case SMC_WD8013EBT:
		NPRINT(("WD8013EBT"));
		break;
	case SMC_WD8013EB:
		if (board_id & SMC_BID_INTERFACE_584_CHIP) {
			NPRINT(("WD8013EP"));
		} else {
			NPRINT(("WD8013EB"));
		}
		break;
	case SMC_WD8013W:
		NPRINT(("WD8013W"));
		break;
	case SMC_WD8013EW:
		NPRINT(("WD8013EW"));
		break;
	case SMC_WD8013EWC:
		NPRINT(("WD8013EWC"));
		break;
	case SMC_WD8013WC:
		NPRINT(("WD8013WC"));
		break;
	case SMC_WD8013EPC:
		NPRINT(("WD8013EPC"));
		break;
	case SMC_WD8003WC:
		NPRINT(("WD8003WC"));
		break;
	case SMC_WD8003EPC:
		NPRINT(("WD8003EPC"));
		break;
	case SMC_WD8208T:
		NPRINT(("WD8208T"));
		break;
	case SMC_WD8208:
		NPRINT(("WD8208"));
		break;
	case SMC_WD8208C:
		NPRINT(("WD8208C"));
		break;
	case SMC_WD8216T:
		NPRINT(("WD8216T"));
		break;
	case SMC_WD8216:
		NPRINT(("WD8216"));
		break;
	case SMC_WD8216C:
		NPRINT(("WD8216C"));
		break;
	case SMC_WD8216LT:
		NPRINT(("WD8216LT"));
		break;
	case SMC_WD8216L:
		NPRINT(("WD8216L"));
		break;
	case SMC_WD8216LC:
		NPRINT(("WD8216LC"));
		break;
	case SMC_WD8216N:
		NPRINT(("WD8216N"));
		break;
	case SMC_WD8216TN:
		NPRINT(("WD8216TN"));
		break;
	case SMC_WD8216CN:
		NPRINT(("WD8216CN"));
		break;
	case SMC_WD8216TH:
		NPRINT(("WD8216TH"));
		break;
	case SMC_WD8216LTH:
		NPRINT(("WD8216LTH"));
		break;
	case SMC_WD8416:
		NPRINT(("WD8416"));
		break;
	case SMC_WD8416T:
		NPRINT(("WD8416T"));
		break;
	case SMC_WD8416C:
		NPRINT(("WD8416C"));
		break;
	case SMC_WD8416L:
		NPRINT(("WD8416L"));
		break;
	case SMC_WD8416LT:
		NPRINT(("WD8416LT"));
		break;
	case SMC_WD8416LC:
		NPRINT(("WD8416LC"));
		break;
	default:
		NPRINT(("unknown"));
		break;
	}

	NPRINT((" rev 0x%x,", smc_bid_get_board_rev_number(addr)));

	switch(board_id & SMC_BID_RAM_SIZE_MASK) {
	case SMC_BID_RAM_SIZE_8K:
		NPRINT((" 8 kB"));
		break;
	case SMC_BID_RAM_SIZE_16K:
		NPRINT((" 16 kB"));
		break;
	case SMC_BID_RAM_SIZE_32K:
		NPRINT((" 32 kB"));
		break;
	case SMC_BID_RAM_SIZE_64K:
		NPRINT((" 64 kB"));
		break;
	default:
		NPRINT((" Unknown size"));
		break;
	}
	NPRINT((" RAM\n\t"));
	if (board_id & SMC_BID_BOARD_16BIT) {
		if (board_id & SMC_BID_SLOT_16BIT) {
			NPRINT(("in 16 bit slot"));
		} else {
			NPRINT(("16 bit board in 8 bit slot"));
		}
	}
	if (board_id & SMC_BID_INTERFACE_CHIP) {
		switch (board_id & SMC_BID_INTERFACE_CHIP_MASK) {
		case SMC_BID_INTERFACE_584_CHIP:
			NPRINT((" 584 chip"));
			break;
		case SMC_BID_INTERFACE_594_CHIP:
			NPRINT((" 594 chip"));
			break;
		case SMC_BID_INTERFACE_585_CHIP:
			NPRINT((" 585 chip"));
			break;
		case SMC_BID_INTERFACE_5X3_CHIP:
			NPRINT((" 583 chip"));
			break;
		default:
			NPRINT((" Unknown BIC chip"));
			break;
		}
	}
	if (board_id & SMC_BID_NIC_790_BIT) {
		NPRINT((", WD83C790 NIC"));
	} else if (board_id & SMC_BID_NIC_690_BIT) {
		NPRINT((", WD83C690 NIC"));
	} else {
		NPRINT((", NS8390 NIC"));
	}
	NPRINT(("\n"));

	return (0);
}

int
probe_smc(void)
{
	unsigned i;

	ns8390.ns_flags = 0;
	for (i = 0; smc_addr[i] != 0; i++) {
#ifdef	SMCDEBUG
	NPRINT(("Started looking for an SMC board @ 0x%x\n",
			       smc_addr[i]));
#endif
	if (inb(smc_addr[i] + SMC_LAR_0) == (u8bits) SMC_ADDR_0 &&
		    inb(smc_addr[i] + SMC_LAR_1) == (u8bits) SMC_ADDR_1 &&
		    inb(smc_addr[i] + SMC_LAR_2) == (u8bits) SMC_ADDR_2 &&
		    smc_get_at_config(smc_addr[i]) == 0)
			break;
	}
	if (smc_addr[i] == 0)
		return (0);

	dlink.dl_laddr[0] = inb(smc_addr[i] + SMC_LAR_0);
	dlink.dl_laddr[1] = inb(smc_addr[i] + SMC_LAR_1);
	dlink.dl_laddr[2] = inb(smc_addr[i] + SMC_LAR_2);
	dlink.dl_laddr[3] = inb(smc_addr[i] + SMC_LAR_3);
	dlink.dl_laddr[4] = inb(smc_addr[i] + SMC_LAR_4);
	dlink.dl_laddr[5] = inb(smc_addr[i] + SMC_LAR_5);
	dlink.dl_hlen = 14;
	dlink.dl_type = ARPHRD_ETHERNET;
	dlink.dl_len = 6;
	dlink.dl_output = ns8390_output;
	dlink.dl_input = ns8390_input;

	ns8390.ns_reset = smc_reset;
	ns8390.ns_en16bits = smc_en16bits;
	ns8390.ns_dis16bits = smc_dis16bits;
	ns8390_reset();

	return (1);
}

