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
 * Taken from
 *
 *  Copyright (c) 1994	Wolfgang Stanglmeier, Koeln, Germany
 *			<wolf@dentaro.GUN.de>
*/

#include <pci.h>
#if NPCI > 0

/*========================================================
 *
 *	#includes  and  declarations
 *
 *========================================================
 */

#include <mach/mach_types.h>
#include <mach/kern_return.h>
#include <kern/misc_protos.h>
#include <vm/vm_kern.h>
#include <busses/pci/pci.h>
#include <i386/pci/pci.h>
#include <i386/pci/pci_device.h>
#include <i386/pci/pcibios.h>

struct pci_vendors {
	pci_vendor_id_t	vendor_id;
	char *		vendor_name;
} pci_vendors[] = {
	OLD_NCR_ID,      "NCR",
	ATI_ID,          "ATI TECHNOLOGIES INC",
	DEC_ID,          "DIGITAL EQUIPMENT CORPORATION",
	CIRRUS_ID,       "CIRRUS",
	IBM_ID,          "IBM",
	NCR_ID,          "NCR",
	AMD_ID,          "AMD",
	MATROX_ID,	 "MATROX",
	COMPAQ_ID,       "COMPAQ",
	NEC_ID,          "NEC",
	HP_ID,           "HEWLETT PACKARD",
	KPC_ID,          "KUBOTA PACIFIC CORP.",
	OPTI_ID,	 "OPTI",
	TI_ID,           "TEXAS INSTRUMENTS",
	SONY_ID,	 "SONY",
	MOT_ID,		 "MOTOROLA",          
	MYLEX_ID,        "MYLEX",
	APPLE_ID,        "APPLE",
	QLOGIC_ID,       "QLOGIC",
	BIT3_ID,         "BIT3_ID",
	CABLETRON_ID,    "CABLETRON",
	THREE_COM_ID,    "3COM",
	CERN_ID,         "CERN",
	ECP_ID,          "ECP",
	ECU_ID,          "ECU",
	PROTEON_ID,      "PROTEON",
	S3_ID,		 "S3 INC.",
	INTEL_ID,        "INTEL CORP.",
	ADP_ID,          "ADAPTEC",
	0,		 0
};

struct pci_devices {
	pci_vendor_id_t	vendor_id;
	pci_dev_id_t	device_id;
	char *		device_name;
} pci_devices[] = {
	DEC_ID,		DEC_PPB,        "PCI-PCI bridge",
	DEC_ID,       	DEC_4250,       "TULIP ethernet",
	DEC_ID,       	DEC_TGA,        "TGA graphics",
	DEC_ID,        	DEC_NVRAM,      "ZEPHYR presto-nvram",
	DEC_ID,        	DEC_PZA,        "PZA SCSI",
	DEC_ID,        	DEC_21140,  	"DECchip 21140 Fast Ethernet",
	DEC_ID,        	DEC_7407,       "VME interface processor",
	DEC_ID,        	DEC_FTA,        "FTA FDDI interface",
	DEC_ID,        	DEC_21041,  	"DECchip 21041 Ethernet",
	INTEL_ID,      	INTEL_PCEB,      "82375EB pci-eisa bridge",
	INTEL_ID,	INTEL_DRAM,	"82424ZX cache dram controller",
	INTEL_ID,	INTEL_SIO,      "82378IB pci-isa bridge",
	INTEL_ID,	INTEL_PSC,      "82425EX pci-isa bridge",
	INTEL_ID,	INTEL_CACHE,    "82434LX pci cache memory controller",
	OLD_NCR_ID,     NCR_810,        "53C810 SCSI I/O Processor",
	OLD_NCR_ID,     NCR_825,        "53C825 SCSI I/O Processor",
	QLOGIC_ID,	QLOGIC_ISP1020,	"ISP1020 SCSI Adapter",
	QLOGIC_ID,	ADP_7870,       "AIC-7870 on motherboard",
	ATI_ID,         ATI_MACH32,     "MACH32",
	ATI_ID,         ATI_MACH64_CX,	"MACH64_CX",   
	ATI_ID,         ATI_MACH64_GX,  "MACH64_GX",
	MYLEX_ID,       MYLEX_960P,	"PCI RAID",
	S3_ID,		S3_VISION864,	"VISION864 VGA",
	S3_ID,		S3_VISION964,	"VISION964 VGA",
	THREE_COM_ID,	TCM_3C590,	"3C590 Ethernet",
	THREE_COM_ID,	TCM_3C595_0,	"3C595 100BaseTX Ethernet",
	THREE_COM_ID,	TCM_3C595_1,	"3C595 100BaseT4 Ethernet",
	THREE_COM_ID,	TCM_3C595_2,	"3C595 100BaseT4 MII Ethernet",
	0,		0,		0
};

struct pci_classes {
	pci_class_t	class;
	char *		class_name;
} pci_classes[] = {
	BASE_BC,	"",
	BASE_MASS,	"Mass Storage controller",
	BASE_NETWORK,	"Network controller",
	BASE_DISPLAY,	"Display controller",
	BASE_MULTMEDIA,	"Multimedia device",
	BASE_MEM,	"Memory controller",
	BASE_BRIDGE,	"Bridge device",
	0,		0	
};

struct pci_subclasses {
	pci_class_t	class;
	pci_subclass_t	subclass;
	char *		subclass_name;
} pci_subclasses[] = {
	BASE_BC,	SUB_PREDEF,	"",
	BASE_BC,	SUB_PRE_VGA,	"VGA compat. device",

	BASE_MASS,	SUB_SCSI,	"SCSI bus controller",
	BASE_MASS,	SUB_IDE,	"IDE controller",
	BASE_MASS,	SUB_FDI,	"Floppy disk controller",
	BASE_MASS,	SUB_IPI,	"IPI bus controller",
	BASE_MASS,	SUB_MASS_OTHER,	"Unkown mass storage controller",

	BASE_NETWORK,	SUB_ETHERNET,	"Ethernet controller",
	BASE_NETWORK,	SUB_TOKEN_RING,	"Token Ring controller",
	BASE_NETWORK,	SUB_FDDI,	"FDDI controller",
	BASE_NETWORK,	SUB_NETWORK_OTHER, "Unkown network controller",

	BASE_DISPLAY,	SUB_VGA,	"VGA compatible controller",
	BASE_DISPLAY,	SUB_XGA,	"XGA controller",
	BASE_DISPLAY,	SUB_DISPLAY_OTHER, "Unknown display controller",

	BASE_MULTMEDIA,	SUB_VIDEO,	"Video",
	BASE_MULTMEDIA,	SUB_AUDIO,	"Audio",
	BASE_MULTMEDIA,	SUB_MULTMEDIA_OTHER, "Unknown multimedia device",

	BASE_MEM,	SUB_RAM,	"RAM",
	BASE_MEM,	SUB_FLASH,	"FLASH",
	BASE_MEM,	SUB_MEM_OTHER,	"Other memory controller",

	BASE_BRIDGE,	SUB_HOST,	"Host bridge",
	BASE_BRIDGE,	SUB_ISA,	"ISA bridge",
	BASE_BRIDGE,	SUB_EISA,	"EISA bridge",
	BASE_BRIDGE,	SUB_MC,		"MC bridge",
	BASE_BRIDGE,	SUB_PCI,	"PCI-to-PCI bridge",
	BASE_BRIDGE,	SUB_PCMCIA,	"PCMCIA bridge",
	BASE_BRIDGE,	SUB_BRIDGE_OTHER, "Other bridge device",
	0,		0	
};

void
pci_print_id(
	pci_vendor_id_t vendor_id,
	pci_dev_id_t device_id,
	pci_class_t class,
	pci_subclass_t subclass)
{
	struct pci_vendors *vp;
	struct pci_devices *dp;
	struct pci_classes *cp;
	struct pci_subclasses *sp;

	/* vendor id */

	for (vp = pci_vendors; vp->vendor_name; vp++)
		if (vp->vendor_id == vendor_id) {
			printf("%s ", vp->vendor_name);
			break;
		}
	if (!vp->vendor_name)
		printf("vendor 0x%x ", vendor_id);

	/* device id */

	for (dp = pci_devices; dp->device_name; dp++)
		if (dp->vendor_id == vendor_id &&
		    dp->device_id == device_id) {
			printf("%s ", dp->device_name);
			return;	 /* no need to print class/subclass */
		}
	if (!dp->device_name)
		printf("device 0x%x ", device_id);
	
	/* class */

	for (cp = pci_classes; cp->class_name; cp++)
		if (cp->class == class) {
			printf("%s ", cp->class_name );
			break;
		}
	if (!cp->class_name)
		printf("class 0x%x ", class);

	/* subclass */

	for (sp = pci_subclasses; sp->subclass_name; sp++)
		if (sp->class == class &&
		    sp->subclass == subclass) {
			printf("%s ", sp->subclass_name );
			break;
		}

	if (!sp->subclass_name)
		printf("subclass 0x%x ", subclass);
}

#endif /* NPCI > 0 */




