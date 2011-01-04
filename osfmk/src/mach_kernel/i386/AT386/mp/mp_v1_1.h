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

#ifndef	_MP_MP_V1_1_H_
#define	_MP_MP_V1_1_H_

#include <i386/apic.h>
#include <kern/lock.h>
#include <chips/busses.h>

struct MP_Config_EntryP {
	unsigned char	Entry_Type;
	unsigned char	Local_Apic_Id;
	unsigned char	Local_Apic_Version;
	unsigned char	CPU_Flags;
	unsigned int	CPU_Signature;
	unsigned int	Feature_Flags;
	unsigned int	Reserved[2];
};

/* Entry types */

#define MP_CPU_ENTRY		0	/* Processor entry */
#define MP_BUS_ENTRY		1	/* bus entry */
#define MP_IO_APIC_ENTRY	2	/* I/O APIC entry */
#define MP_IO_INT_ENTRY		3	/* I/O Interrupt assignment */
#define MP_LOC_INT_ENTRY	4	/* Local Interrupt assignment */

struct MP_Config_EntryB {
	unsigned char	Entry_Type;
	unsigned char	Bus_Id;
	char		Ident[6];
};

struct MP_Config_EntryA {
	unsigned char	Entry_Type;
	unsigned char	IO_Apic_Id;
	unsigned char	IO_Apic_Version;
	unsigned char	IO_Apic_Flags;
	vm_offset_t	IO_Apic_Address;
};

struct MP_Config_EntryI {
	unsigned char	Entry_Type;
	unsigned char	Int_Type;
	unsigned short	Int_Flag;
	unsigned char	Source_Bus;
	unsigned char	Source_IRQ;
	unsigned char	Dest_IO_Apic;
	unsigned char	Dest_INTIN;
};
struct MP_Config_EntryL {
	unsigned char	Entry_Type;
	unsigned char	Int_Type;
	unsigned short	Int_Flag;
	unsigned char	Source_Bus;
	unsigned char	Source_IRQ;
	unsigned char	Dest_Local_Apic;
	unsigned char	Dest_INTIN;
};

struct MP_FPS_struct {
	unsigned int	Signature;
	vm_offset_t	Config_Ptr;
	unsigned char	Length;
	unsigned char	Spec_Rev;
	unsigned char	CheckSum;
	unsigned char	Feature[5];
};

struct MP_Config_Table {
	unsigned int	Signature;
	unsigned short	Length;
	unsigned char	Spec_Rev;
	unsigned char	CheckSum;
	char		OEM[8];
	char		PROD[12];
	vm_offset_t	OEM_Ptr;
	unsigned short	OEM_Size;
	unsigned short	Entries;
	vm_offset_t	Local_Apic;
	unsigned int	Reserved;
};

#define	IMCR_ADDRESS		0x22
#define IMCR_DATA		0x23
#define	IMCR_SELECT		0x70
#define IMCR_APIC_ENABLE	0x01

extern	boolean_t 	mp_v1_1_take_irq(int 	pic,
					 int 	unit,
					 int 	spl, 
					 intr_t	intr);

extern	boolean_t	mp_v1_1_reset_irq(int 		pic,
					  int 		*unit, 
					  int 		*spl, 
					  intr_t 	*intr);


void mp_v1_1_init(void);
boolean_t mp_v1_1_io_lock(int, struct processor **);
void mp_v1_1_io_unlock(struct processor *);

/* Intel default Configurations */

#define	MP_PROPRIETARY_CONF	0
#define	MP_ISA_CONF		1
#define	MP_EISA_1_CONF		2
#define	MP_EISA_2_CONF		3
#define	MP_MCA_CONF		4
#define	MP_ISA_PCI_CONF		5
#define	MP_EISA_PCI_CONF	6
#define	MP_MCA_PCI_CONF		7

#if	NCPUS > 1
#define at386_io_lock_state() \
	processor_t mp_v1_1_saved_processor = PROCESSOR_NULL;
#define at386_io_lock(x) \
	mp_v1_1_io_lock(x, &mp_v1_1_saved_processor)
#define at386_io_unlock() \
	mp_v1_1_io_unlock(mp_v1_1_saved_processor)
#endif	/* NCPUS > 1 */

#endif	/* _MP_MP_V1_1_H_ */
