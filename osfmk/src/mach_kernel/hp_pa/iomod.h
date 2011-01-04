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
 * Copyright (c) 1990,1991 The University of Utah and
 * the Center for Software Science (CSS).
 * All rights reserved.

 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * Copyright (c) 1990 mt Xinu, Inc.
 * This file may be freely distributed in any form as long as
 * this copyright notice is included.
 * MTXINU, THE UNIVERSITY OF UTAH, AND CSS PROVIDE THIS SOFTWARE ``AS
 * IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: iomod.h 1.4 91/09/25$
 *	Author: Jeff Forys (CSS), Dave Slattengren (mtXinu)
 */

#ifndef	_HP_PA_IOMOD_H_
#define	_HP_PA_IOMOD_H_

#ifndef ASSEMBLER
#include <types.h>
#endif	/* ASSEMBLER */
#include <machine/iodc.h>

/*
 * Structures and definitions for I/O Modules on HP-PA (9000/800).
 *
 * Memory layout:
 *
 *	0x00000000	+---------------------------------+
 *			|           Page Zero             |
 *	0x00000800	+ - - - - - - - - - - - - - - - - +
 *			|                                 |
 *			|                                 |
 *			|      Memory Address Space       |
 *			|                                 |
 *			|                                 |
 *	0xEF000000	+---------------------------------+
 *			|                                 |
 *			|        PDC Address Space        |
 *			|                                 |
 *	0xF1000000	+---------------------------------+
 *			|                                 |
 *			|                                 |
 *			|        I/O Address Space        |
 *			|                                 |
 *			|                                 |
 *	0xFFF80000	+ - - - - - - - - - - - - - - - - +
 *			|  Fixed Physical Address Space   |
 *	0xFFFC0000	+ - - - - - - - - - - - - - - - - +
 *			|  Local Broadcast Address Space  |
 *	0xFFFE0000	+ - - - - - - - - - - - - - - - - +
 *			| Global Broadcast Address Space  |
 *	0xFFFFFFFF	+---------------------------------+
 *
 * "Memory Address Space" is used by memory modules,
 *   "Page Zero" is described in "pdc.h".
 * "PDC Address Space" is used by Processor-Dependent Code.
 * "I/O Address Space" is used by I/O modules (and is not cached),
 *   "Fixed Physical" is used by modules on the central bus,
 *   "Local Broadcast" is used to reach all modules on the same bus, and
 *   "Global Broadcast" is used to reach all modules (thru bus converters).
 *   
 * SPA space (see below) ranges from 0xF1000000 thru 0xFFFC0000.
 */

#define	PDC_ADDR	0xEF000000	/* explained above */
#define	IO_ADDR		0xF1000000
#define	FP_ADDR		0xFFF80000
#define	LBCAST_ADDR	0xFFFC0000
#define	GBCAST_ADDR	0xFFFE0000

#define	PDC_LOW		PDC_ADDR	/* define some ranges */
#define	PDC_HIGH	IO_ADDR
#define	FPA_LOW		FP_ADDR
#define	FPA_HIGH	LBCAST_ADDR
#define	SPA_LOW		IO_ADDR
#define	SPA_HIGH	LBCAST_ADDR

#define	FPA_IOMOD	((FPA_HIGH-FPA_LOW)/sizeof(struct iomod))
#define	MAXMODBUS	FPA_IOMOD	/* maximum modules/bus */

#define BCONV_ALIGN	0xffff		/* bus convertor range alignment */


#ifndef ASSEMBLER
/*
 * Every module has 4K-bytes of address space associated with it.
 * A Hard Physical Address (HPA) can be broken down as follows.
 *
 * Since this is an I/O space, the high 4 bits are always 1's.
 *
 * The "flex" address specifies which bus a module is on; there are
 * 256K-bytes of HPA space for each bus, however only values from
 * 64 - 1022 are valid for the "flex" field (1022 designates the
 * central bus).  The "flex" addr is set at bus configuration time.
 *
 * The "fixed" address specifies a particular module on the same
 * bus (i.e. among modules with the same "flex" address).  This
 * value can also be found in "device_path.dp_mod" in "pdc.h".
 *
 * A modules HPA space consists of 2 pages; the "up" bit specifies
 * which of these pages is being addressed.  In general, the lower
 * page is privileged and the upper page it module-type dependent.
 *
 */
#define	FLEX_MASK	0xFFFC0000

struct hpa {
	u_int	hpa_ones: 4,	/* must be 1's; this is an I/O space addr */
		hpa_flex:10,	/* bus address for this module */
		hpa_fixed:6,	/* location of module on bus */
		hpa_up	: 1,	/* 1 == upper page, 0 == lower page */
		hpa_set	: 5,	/* register set */
		hpa_reg	: 4,	/* register number within a register set */
		hpa_zeros:2;	/* must be 0's; addrs are word aligned */
};


/*
 * Certain modules require additional memory (i.e. more than that
 * provided by the HPA space).  A Soft Physical Address (SPA) can be
 * broken down as follows, on a module-type specific basis (either
 * Memory SPA or I/O SPA).
 *
 * SPA space must be a power of 2, and aligned accordingly.  The IODC
 * provides all information needed by software to configure SPA space
 * for a particular module.
 */

struct memspa {
	u_int	spa_page:21,	/* page of memory */
		spa_off	:11;	/* offset into memory page */
};

struct iospa {
	u_int	spa_ones: 4,	/* must be 1's; this is an I/O space addr */
		spa_iopg:17,	/* page in I/O address space */
		spa_set	: 5,	/* register set */
		spa_reg	: 4,	/* register number within a register set */
		spa_mode: 2;	/* aligned according to bus transaction mode */
};

/*
 * It is possible to send a command to all modules on a particular bus
 * (local broadcast), or all modules (global broadcast).  A Broadcast
 * Physical Address (BPA) can be broken down as follows.
 *
 * Read and Clear transactions are not allowed in BPA space.  All pages
 * in BPA space are privileged.
 */

struct bpa {
	u_int	bpa_ones:14,	/* must be 1's; this is in BPA space */
		bpa_gbl	: 1,	/* 0 == local, 1 == global broadcast */
		bpa_page: 6,	/* page in local/global BPA space */
		bpa_set	: 5,	/* register set */
		bpa_reg	: 4,	/* register number within a register set */
		bpa_zeros:2;	/* must be 0's; addrs are word aligned */
};


/*
 * All I/O and Memory modules have 4K-bytes of HPA space associated with
 * it (described above), however not all modules implement every register.
 * The first 2K-bytes of registers are "priviliged".
 *
 * (WO) == Write Only, (RO) == Read Only
 */

struct iomod {
/* SRS (Supervisor Register Set) */
	u_int	io_eir;		/* (WO) interrupt CPU; set bits in EIR CR */
	u_int	io_eim;		/* (WO) External Interrupt Message address */
	u_int	io_dc_rw;	/* write address of IODC to read IODC data */
	int	io_ii_rw;	/* read/clear external intrpt msg (bit-26) */
	caddr_t	io_dma_link;	/* pointer to "next quad" in DMA chain */
	u_int	io_dma_command;	/* (RO) chain command to exec on "next quad" */
	caddr_t	io_dma_address;	/* (RO) start of DMA */
	int	io_dma_count;	/* (RO) number of bytes remaining to xfer */
	caddr_t	io_flex;	/* (WO) HPA flex addr, LSB: bus master flag */
	caddr_t	io_spa;		/* (WO) SPA space; 0-20:addr, 24-31:iodc_spa */
	int	resv1[2];	/* (reserved) */
	u_int	io_command;	/* (WO) module commands (see below) */
	u_int	io_status;	/* (RO) error returns (see below) */
	u_int	io_control;	/* memory err logging (bit-9), bc forwarding */
	u_int	io_test;	/* (RO) self-test information */
/* ARS (Auxiliary Register Set) */
	u_int	io_err_sadd;	/* (RO) slave bus error or memory error addr */
	caddr_t	chain_addr;	/* start address of chain RAM */
	u_int	sub_mask_clr;	/* ignore intrpts on sub-channel (bitmask) */
	u_int	sub_mask_set;	/* service intrpts on sub-channel (bitmask) */
	u_int	diagnostic;	/* diagnostic use (reserved) */
	int	resv2[2];	/* (reserved) */
	caddr_t	nmi_address;	/* address to send data to when NMI detected */
	caddr_t	nmi_data;	/* NMI data to be sent */
	int	resv3[3];	/* (reserved) */
	u_int	io_mem_low;	/* bottom of memory address range */
	u_int	io_mem_high;	/* top of memory address range */
	u_int	io_io_low;	/* bottom of I/O HPA address Range */
	u_int	io_io_high;	/* top of I/O HPA address Range */

	int	priv_trs[160];	/* TRSes (Type-dependent Register Sets) */
	int	priv_hvrs[320];	/* HVRSes (HVERSION-dependent Register Sets) */

	int	hvrs[512];	/* HVRSes (HVERSION-dependent Register Sets) */
};

#endif /* ASSEMBLER */

/* io_flex */
#define	DMA_ENABLE	0x1	/* flex register enable DMA bit */

/* io_spa */
#define	IOSPA(spa,iodc_data)	\
	((caddr_t) ((u_int) spa | (u_int) iodc_data.iodc_spa_shift | \
		    (u_int) iodc_data.iodc_spa_enb << 5 | \
		    (u_int) iodc_data.iodc_spa_pack << 6 | \
		    (u_int) iodc_data.iodc_spa_io << 7))

/* io_command */
#define	CMD_STOP	0	/* halt any I/O, enable diagnostic access */
#define	CMD_FLUSH	1	/* abort DMA */
#define	CMD_CHAIN	2	/* initiate DMA */
#define	CMD_CLEAR	3	/* clear errors */
#define	CMD_RESET	5	/* reset any module */
#define CMD_PORT	32	/* set port bit on bus convertor */

/* io_status */
#define	IO_ERR_MEM_SL	0x10000	/* SPA space lost or corrupted */
#define	IO_ERR_MEM_SE	0x00200	/* severity: minor */
#define	IO_ERR_MEM_HE	0x00100	/* severity: affects invalid parts */
#define	IO_ERR_MEM_FE	0x00080	/* severity: bad */
#define	IO_ERR_MEM_RY	0x00040	/* IO_COMMAND register ready for command */
#define	IO_ERR_DMA_DG	0x00010	/* module in diagnostic mode */
#define IO_ERR_BC_LP    0x00008 /* lower port on a bus convertor */
#define	IO_ERR_DMA_PW	0x00004	/* Power Failing */
#define	IO_ERR_DMA_PL	0x00002	/* Power Lost */
#define	IO_ERR_BC_PF	0x00001	/* Power failed on remote bus */
#define	IO_ERR_VAL(x)	 (((x) >> 10) & 0x3f)
#define	IO_ERR_DEPEND	 0	/* unspecified error */
#define	IO_ERR_SPA	 1	/* (module-type specific) */
#define	IO_ERR_INTERNAL	 2	/* (module-type specific) */
#define	IO_ERR_MODE	 3	/* invlaid mode or address space mapping */
#define	IO_ERR_ERROR_M	 4	/* bus error (master detect) */
#define	IO_ERR_DPARITY_S 5	/* data parity (slave detect) */
#define	IO_ERR_PROTO_M	 6	/* protocol error (master detect) */
#define	IO_ERR_ADDRESS	 7	/* no slave acknowledgement in transaction */
#define	IO_ERR_MORE	 8	/* device transfered more data than expected */
#define	IO_ERR_LESS	 9	/* device transfered less data than expected */
#define	IO_ERR_SAPARITY	10	/* slave addrss phase parity */
#define	IO_ERR_MAPARITY	11	/* master address phase parity */
#define	IO_ERR_MDPARITY	12	/* mode phase parity */
#define	IO_ERR_STPARITY	13	/* status phase parity */
#define	IO_ERR_CMD	14	/* unimplemented I/O Command */
#define	IO_ERR_BUS	15	/* generic bus error */
#define	IO_ERR_CORR	24	/* correctable memory error */
#define	IO_ERR_UNCORR	25	/* uncorrectable memory error */
#define	IO_ERR_MAP	26	/* equivalent to IO_ERR_CORR */
#define	IO_ERR_LINK	28	/* Bus Converter "link" (connection) error */
#define	IO_ERR_CCMD	32	/* Illegal DMA command */
#define	IO_ERR_ERROR_S	52	/* bus error (slave detect) */
#define	IO_ERR_DPARITY_M 53	/* data parity (master detect) */
#define	IO_ERR_PROTOCOL	54	/* protocol error (slave detect) */
#define	IO_ERR_SELFTEST	58	/* (module-type specific) */
#define	IO_ERR_BUSY	59	/* slave was busy too often or too long */
#define	IO_ERR_RETRY	60	/* "busied" transaction not retried soon enuf */
#define	IO_ERR_ACCESS	61	/* illegal register access */
#define	IO_ERR_IMPROP	62	/* "improper" data written */
#define	IO_ERR_UNKNOWN	63

/* io_control (memory) */
#define	IO_CTL_MEMINIT	0x0	/* prevent some bus errors during memory init*/
#define	IO_CTL_MEMOKAY	0x100	/* enable all bus error logging */

/* io_control (bus convertors) */
#define IO_CTL_BC_OFF	   0x0	/* only allow global resets through */
#define IO_CTL_BC_INCLUDE  0x1	/* transparent for matching addresses (upper)*/
#define IO_CTL_BC_EXCLUDE  0x2	/* opaque for matching addresses (lower)*/
#define IO_CTL_BC_PEEK     0x3	/* Map matching addresses (upper port) */

/* io_spa */
#define	SPA_ENABLE	0x20	/* io_spa register enable spa bit */


/*
 * Before configuring the bus, we build a table which consists of
 * "useful information" on each module.  We then use this table to
 * allocate SPA's and reconfigure the bus.
 */

#define	MODTABSIZ MAXMODBUS	/* size of static I/O module table */

#ifndef ASSEMBLER
struct modtab {
	struct modtab	*mt_next;	/* pointer to next `modtab' entry */
	u_int		m_fixed;	/* fixed bus address */
	struct iodc_data mt_type;	/* type specific info from IODC */
	struct iomod	*m_hpa;		/* ptr to HPA */
	u_int		m_spa;		/* location of SPA */
	u_int		m_spasiz;	/* size of SPA */
	int		m_alive;	/* autoconfigured? */
        struct bus      *m_device;      /* device assigned by autoconfig */
	union {
		struct {		/* Memory module */
			u_int	M_memsiz;	/* size (m_size) */
		} m_Mem;
		struct {		/* I/O module */
			u_int	M_cioeim;	/* EIM address (m_eim) */
			caddr_t	M_ciochain;	/* CIO chain RAM (m_chain) */
		} m_Io;
		struct {		/* Viper memory controller module */
			u_int	M_vi_eim;	/* EIM address (m_vi_eim) */
			u_int	M_vi_iwd;	/* EIM intr word (m_vi_iwd) */
		} m_Vi;
		union {			/* Foriegn I/O module */
			u_int	M_scsiclk;	/* SCSI: clock frequency */
			u_int	M_stirom;	/* GRF: location of STI ROM */
		} m_Fio;
	} m_Dep;
};
#endif /* ASSEMBLER */

#define	m_memsiz	m_Dep.m_Mem.M_memsiz
#define	m_cioeim	m_Dep.m_Io.M_cioeim
#define	m_ciochain	m_Dep.m_Io.M_ciochain
#define	m_vi_eim	m_Dep.m_Vi.M_vi_eim
#define	m_vi_iwd	m_Dep.m_Vi.M_vi_iwd
#define m_scsiclk	m_Dep.m_Fio.M_scsiclk
#define m_stirom	m_Dep.m_Fio.M_stirom

#define	HIGH_HW_EIM	2	/* hw intrpt with highest priority */

#define	EIM_GRPMASK	0x1F	/* EIM register group mask */
#define	EIEM_MASK(eim)	(0x80000000 >> (eim & EIM_GRPMASK))
#define	EIEM_BITCNT	32	/* number of bits in EIEM register */

#ifndef ASSEMBLER
extern struct iomod *prochpa;
#endif /* ASSEMBLER */

#endif	/* _HP_PA_IOMOD_H_ */
