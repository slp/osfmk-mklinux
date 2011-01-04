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
 * File : smc.h
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains all SMC descriptions used for Network bootstrap.
 */

#ifndef __SMC_H__
#define __SMC_H__

/*
 * SMC Ethernet address bytes
 */
#define	SMC_ADDR_0	0x00
#define	SMC_ADDR_1	0x00
#define	SMC_ADDR_2	0xC0

/*
 * 83c583 registers
 */
#define SMC_REG_0		0x00		/* LAN adapter register	0 */
#define SMC_REG_1		0x01		/* LAN adapter register	1 */
#define SMC_REG_2		0x02		/* LAN adapter register	2 */
#define SMC_REG_3		0x03		/* LAN adapter register	3 */
#define SMC_REG_4		0x04		/* LAN adapter register	4 */
#define SMC_CNFG_LAAR_584	0x05		/* LAN adapter register	5 */
#define SMC_REG_5		0x06		/* LAN adapter register	6 */
#define SMC_GP2			0x07		/* general purpose register 2*/
#define SMC_LAR_0		0x08		/* LAN address register	0 */
#define SMC_LAR_1		0x09		/* LAN address register	1 */
#define SMC_LAR_2		0x0A		/* LAN address register	2 */
#define SMC_LAR_3		0x0B		/* LAN address register	3 */
#define SMC_LAR_4		0x0C		/* LAN address register	4 */
#define SMC_LAR_5		0x0D		/* LAN address register	5 */
#define SMC_HID			0x0E		/* Hardware ID byte */
#define SMC_CKSUM		0x0F		/* Checksum byte */

#define	SMC_REG_ICR		SMC_REG_1	/* Interface Configuration */
#define	SMC_REG_EER		SMC_REG_1	/* EEROM */
#define	SMC_REG_HWR		SMC_REG_4	/* Hardware Support Register */
#define	SMC_REG_REV		SMC_GP2		/* Revision */
#define	SMC_REG_RAR		SMC_LAR_3	/* RAM Address Register */

/*
 * Register 0 description.
 */
#define	SMC_ADDR_MASK		0x3F	/* Bits 18...13 of RAM addr */
#define	SMC_ADDR_MENB		0x40	/* Shared memory Enable */

/*
 * Register 1 description (583/584).
 */
#define	SMC_SIXTEEN_BIT_BIT	0x01	/* 16bits BUS */
#define	SMC_OTHER_BIT		0x02
#define	SMC_MSZ_583_BIT		0x08
#define	SMC_ICR_MASK		0x0C
#define	SMC_RLA			0x10
#define	SMC_RECALL_DONE_MASK	0x10

/*
 * Register 1 description (585/790).
 */
#define SMC_EER_JMP        	0x04    /* Initialization Jumpers Field */
#define SMC_EER_EA         	0x04    /* EEROM Address Field */
#define SMC_EER_UNLOCK     	0x10    /* Unlock Store */
#define SMC_EER_RC         	0x40    /* Recall EEROM */
#define SMC_EER_STO        	0x80    /* Store into Non-Volatile EEROM */

#define SMC_OFFSET_585_LAN_ADDR	   6
#define SMC_OFFSET_585_ENGR_DATA  10

/*
 * Register 3 description.
 */
#define	SMC_EAR_MASK		0x0F
#define	SMC_EA6			0x80
#define	SMC_ENGR_PAGE		0xA0

/*
 * Register 3 description (585/790).
 */
#define SMC_HWR_SWH		0x80	/* Switch Register Set */
#define SMC_HWR_LPRM		0x40	/* LAN Address ROM Select */
#define SMC_HWR_ETHER		0x20	/* NIC Type. 1=83C690, 0=83C825. (R) */
#define SMC_HWR_HOST16		0x10	/* Set When Host has 16 bit bus. (R) */
#define SMC_HWR_STAT2		0x08	/* Interrupt Status (R) */
#define SMC_HWR_STAT1		0x04	/* Interrupt Status (R) */
#define SMC_HWR_GIN2		0x02	/* General Purpose Input 2 (R) */
#define SMC_HWR_GIN1		0x01	/* General Purpose Input 1 (R) */

#define SMC_HWR_MASK		0x20	/* Interrupt Mask Bit (W) */
#define SMC_HWR_NUKE		0x08	/* Hardware Reset (W) */
#define SMC_HWR_CLR1		0x04	/* Clear Interrupt (W, 585 only) */
#define SMC_HWR_HWCS		0x02	/* WCS Control (W, 585 only) */
#define SMC_HWR_CA		0x01	/* Control Attention (W, 585 only) */

/*
 * Register 5 description.
 */
#define SMC_LA19		0x01	/* Address lines for enabling */
#define SMC_LA20		0x02	/*   shared RAM above 1Mbyte  */
#define SMC_LA21		0x04
#define SMC_LA22		0x08
#define SMC_LA23		0x10
#define SMC_LAN16ENB		0x40	/* Enables 16bit shrd RAM for LAN */
#define SMC_MEM16ENB		0x80	/* Enables 16bit shrd RAM for host */
#define	SMC_CNFG_LAAR_MASK	0x1F

/*
 * Register 11 (RAR) description (585/790)
 */
#define	SMC_RAR_HRAM		0x80	/* High RAM Address */
#define	SMC_RAR_RA17		0x40	/* RAM Base Address field 17 */
#define	SMC_RAR_RAMSZ1		0x20	/* Buffer window size 1 */
#define	SMC_RAR_RAMSZ0		0x10	/* Buffer window size 0 */
#define	SMC_RAR_RA16		0x08	/* RAM Base Address field 16 */
#define	SMC_RAR_RA15		0x04	/* RAM Base Address field 15 */
#define	SMC_RAR_RA14		0x02	/* RAM Base Address field 14 */
#define	SMC_RAR_RA13		0x01	/* RAM Base Address field 13 */

/*
 * Hardware ID byte description.
 */
#define	SMC_BID_MEDIA_TYPE_BIT	0x01	/* Media Type */
#define	SMC_BID_BOARD_REV_MASK	0x1E	/* Mask of board revision number */
#define	SMC_BID_SOFT_CONFIG_BIT	0x20	/* Sotware configuration */
#define	SMC_BID_RAM_SIZE_BIT	0x40	/* Extended size */
#define	SMC_BID_BUS_TYPE_BIT	0x80	/* Bus Type */

/*
 * 584 EEPROM registers.
 */
#define	SMC_EEPROM_0		0x08	/* EEPROM 0 */
#define	SMC_EEPROM_1		0x09	/* EEPROM 1 */
#define	SMC_EEPROM_2		0x0A	/* EEPROM 2 */
#define	SMC_EEPROM_3		0x0B	/* EEPROM 3 */
#define	SMC_EEPROM_4		0x0C	/* EEPROM 4 */
#define	SMC_EEPROM_5		0x0D	/* EEPROM 5 */
#define	SMC_EEPROM_6		0x0E	/* EEPROM 6 */
#define	SMC_EEPROM_7		0x0F	/* EEPROM 7 */

/*
 * EEPROM 0 description.
 */
#define	SMC_EEPROM_MEDIA_MASK		0x07
#define	SMC_STARLAN_TYPE		0x00
#define	SMC_ETHERNET_TYPE		0x01
#define	SMC_TP_TYPE			0x02
#define	SMC_EW_TYPE			0x03
#define	SMC_EEPROM_IRQ_MASK		0x18
#define	SMC_PRIMARY_IRQ			0x00
#define	SMC_ALTERNATE_IRQ_1		0x08
#define	SMC_ALTERNATE_IRQ_2		0x10
#define	SMC_EEPROM_RAM_SIZE_MASK	0xE0
#define	SMC_EEPROM_RAM_SIZE_RES1	0x00
#define	SMC_EEPROM_RAM_SIZE_RES2	0x20
#define	SMC_EEPROM_RAM_SIZE_8K		0x40
#define	SMC_EEPROM_RAM_SIZE_16K		0x60
#define	SMC_EEPROM_RAM_SIZE_32K		0x80
#define	SMC_EEPROM_RAM_SIZE_64K		0xA0

/*
 * EEPROM 1 description.
 */
#define	SMC_EEPROM_BUS_TYPE_MASK	0x07
#define	SMC_EEPROM_BUS_TYPE_AT		0x00
#define	SMC_EEPROM_BUS_TYPE_MCA		0x01
#define	SMC_EEPROM_BUS_TYPE_EISA	0x02
#define	SMC_EEPROM_BUS_TYPE_NEC		0x03
#define	SMC_EEPROM_BUS_SIZE_MASK	0x18
#define	SMC_EEPROM_BUS_SIZE_8BIT	0x00
#define	SMC_EEPROM_BUS_SIZE_16BIT	0x08
#define	SMC_EEPROM_BUS_SIZE_32BIT	0x10
#define	SMC_EEPROM_BUS_SIZE_64BIT	0x18
#define	SMC_EEPROM_RAM_PAGING		0x40
#define	SMC_EEPROM_ROM_PAGING		0x80
#define	SMC_EEPROM_PAGING_MASK		0xC0

/*
 * EEPROM 3 description.
 */
#define	SMC_EEPROM_HMI			0x01
#define	SMC_EEPROM_LOW_COST		0x08

/*
 * 584 chip definitions.
 */
#define	SMC_EEPROM_OVERRIDE		0xFFD0FFB0	/* Mask to EEPROM values */

/*
 * Board ID.
 */
#define	SMC_BID_STARLAN_MEDIA		0x00000001	/* StarLAN */
#define	SMC_BID_ETHERNET_MEDIA		0x00000002      /* Ethernet */
#define	SMC_BID_TWISTED_PAIR_MEDIA	0x00000003      /* Twisted Pair */
#define	SMC_BID_EW_MEDIA		0x00000004	/* Ethernet and Twisted Pair */
#define	SMC_BID_MICROCHANNEL		0x00000008	/* MicroChannel Adapter */
#define	SMC_BID_INTERFACE_CHIP		0x00000010	/* Soft Config Adapter */
#define	SMC_BID_ADVANCED_FEATURES	0x00000020	/* used to be INTELLIGENT */
#define	SMC_BID_BOARD_16BIT		0x00000040	/* 16 bit capability */
#define	SMC_BID_PAGED_RAM		0x00000080	/* Adapter has paged RAM */
#define	SMC_BID_PAGED_ROM		0x00000100	/* Adapter has paged ROM */
#define	SMC_BID_LITE_VERSION		0x00000400	/* Reduced Feature Adapter, I.E Tiger */
#define	SMC_BID_NIC_SUPERSET		0x00000800	/* NIC is a superset of 790 */
#define	SMC_BID_NEC_BUS			0x00001000	/* Adapter is a PC-98 Bus type */
#define	SMC_BID_HMI_ADAPTER		0x00002000	/* Adapter has integral hub */
#define	SMC_BID_RAM_SIZE_MASK		0x00070000	/* Isolates RAM Size */
#define	SMC_BID_RAM_SIZE_UNKNOWN	0x00000000	/* 000 => Unknown RAM Size */
#define	SMC_BID_RAM_SIZE_8K		0x00020000	/* 010 => 8k RAM */
#define	SMC_BID_RAM_SIZE_16K		0x00030000	/* 011 => 16k RAM */
#define	SMC_BID_RAM_SIZE_32K		0x00040000	/* 100 => 32k RAM */
#define	SMC_BID_RAM_SIZE_64K		0x00050000	/* 101 => 64k RAM */
#define	SMC_BID_BOARD_TYPE_MASK		0x0000FFFF
#define	SMC_BID_SLOT_16BIT		0x00080000      /* 16 bit board - 16 bit slot */
#define	SMC_BID_NIC_690_BIT		0x00100000	/* NIC is 690 */
#define	SMC_BID_ALTERNATE_IRQ_BIT	0x00200000	/* Alternate IRQ is used */
#define	SMC_BID_INTERFACE_CHIP_MASK	0x03C00000	/* Isolates Intfc Chip Type */
#define	SMC_BID_INTERFACE_5X3_CHIP	0x00000000	/* 0000 = 583 or 593 chips */
#define	SMC_BID_INTERFACE_584_CHIP	0x00400000	/* 0001 = 584 chip */
#define	SMC_BID_INTERFACE_594_CHIP	0x00800000	/* 0010 = 594 chip */
#define	SMC_BID_INTERFACE_585_CHIP	0x01000000	/* 0100 = 585 chip */
#define	SMC_BID_NIC_790_BIT		0x08000000	/* NIC is 790 BIC/NIC Chip */
#define	SMC_BID_EEPROM_OVERRIDE		0xFFD0FFB0	/* EEPROM overwrite mask */

/*
 * First word definitions for board types
 */
#define SMC_WD8003E	SMC_BID_ETHERNET_MEDIA
#define SMC_WD8003EBT	SMC_WD8003E	/* functionally identical to WD8003E */
#define SMC_WD8003S	SMC_BID_STARLAN_MEDIA
#define SMC_WD8003SH	SMC_WD8003S	/* functionally identical to WD8003S */
#define SMC_WD8003WT	SMC_BID_TWISTED_PAIR_MEDIA
#define SMC_WD8003W	(SMC_BID_TWISTED_PAIR_MEDIA | SMC_BID_INTERFACE_CHIP)
#define SMC_WD8003EB	(SMC_BID_ETHERNET_MEDIA | SMC_BID_INTERFACE_CHIP)
#define SMC_WD8003EP	SMC_WD8003EB	/* with INTERFACE_584_CHIP */
#define SMC_WD8003EW	(SMC_BID_EW_MEDIA | SMC_BID_INTERFACE_CHIP)
#define SMC_WD8003ETA	(SMC_BID_ETHERNET_MEDIA | SMC_BID_MICROCHANNEL)
#define SMC_WD8003STA	(SMC_BID_STARLAN_MEDIA | SMC_BID_MICROCHANNEL)
#define SMC_WD8003EA	(SMC_WD8003ETA | SMC_BID_INTERFACE_CHIP)
#define SMC_WD8003EPA	SMC_WD8003EA	/* with INTERFACE_594_CHIP */
#define SMC_WD8003SHA	(SMC_WD8003STA | SMC_BID_INTERFACE_CHIP)
#define SMC_WD8003WA	(SMC_WD8003W | SMC_BID_MICROCHANNEL)
#define SMC_WD8003WPA	SMC_WD8003WA	/* with INTERFACE_594_CHIP */
#define SMC_WD8013EBT	(SMC_BID_ETHERNET_MEDIA | SMC_BID_BOARD_16BIT)
#define SMC_WD8013EB	(SMC_WD8003EB | SMC_BID_BOARD_16BIT)
#define SMC_WD8013EP	SMC_WD8013EB	/* with INTERFACE_584_CHIP */
#define SMC_WD8013W	(SMC_WD8003W | SMC_BID_BOARD_16BIT)
#define SMC_WD8013EW	(SMC_WD8003EW | SMC_BID_BOARD_16BIT)
#define SMC_WD8013EWC	(SMC_WD8013EW | SMC_BID_ADVANCED_FEATURES)
#define SMC_WD8013WC	(SMC_WD8013W | SMC_BID_ADVANCED_FEATURES)
#define SMC_WD8013EPC	(SMC_WD8013EB | SMC_BID_ADVANCED_FEATURES)
#define SMC_WD8003WC	(SMC_WD8003W | SMC_BID_ADVANCED_FEATURES)
#define SMC_WD8003EPC	(SMC_WD8003EP | SMC_BID_ADVANCED_FEATURES)
#define SMC_WD8208T	(SMC_WD8003WC | SMC_BID_PAGED_ROM | SMC_BID_PAGED_RAM)
#define SMC_WD8208	(SMC_WD8003EPC | SMC_BID_PAGED_ROM | SMC_BID_PAGED_RAM)
#define SMC_WD8208C	(SMC_WD8013EW | SMC_BID_PAGED_ROM | SMC_BID_PAGED_RAM)
#define SMC_WD8216T	(SMC_WD8013WC | SMC_BID_PAGED_ROM | SMC_BID_PAGED_RAM)
#define SMC_WD8216	(SMC_WD8013EPC | SMC_BID_PAGED_ROM | SMC_BID_PAGED_RAM)
#define SMC_WD8216C	(SMC_WD8013EWC | SMC_BID_PAGED_ROM | SMC_BID_PAGED_RAM)
#define SMC_WD8216L	(SMC_WD8216 | SMC_BID_LITE_VERSION)
#define SMC_WD8216LT	(SMC_WD8216T | SMC_BID_LITE_VERSION)
#define SMC_WD8216LC	(SMC_WD8216C | SMC_BID_LITE_VERSION)
#define SMC_WD8216N	(SMC_WD8216 | SMC_BID_NEC_BUS)
#define SMC_WD8216TN	(SMC_WD8216T | SMC_BID_NEC_BUS)
#define SMC_WD8216CN	(SMC_WD8216C | SMC_BID_NEC_BUS)
#define SMC_WD8216TH	(SMC_WD8216T | SMC_BID_HMI_ADAPTER)
#define SMC_WD8216LTH	(SMC_WD8216LT | SMC_BID_HMI_ADAPTER)
#define SMC_WD8416	(SMC_WD8216 | SMC_BID_NIC_SUPERSET)
#define SMC_WD8416T	(SMC_WD8216T | SMC_BID_NIC_SUPERSET)
#define SMC_WD8416C	(SMC_WD8216C | SMC_BID_NIC_SUPERSET)
#define SMC_WD8416L	(SMC_WD8416 | SMC_BID_LITE_VERSION)
#define SMC_WD8416LT	(SMC_WD8416T | SMC_BID_LITE_VERSION)
#define SMC_WD8416LC	(SMC_WD8416C | SMC_BID_LITE_VERSION)

extern int probe_smc(void);

#endif	/* __SMC_H__ */
