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
#include <adsl.h> 


#include <busses/pci/pci.h>
#include <i386/pci/pci.h>
#include <i386/pci/pci_device.h>
#include <i386/pci/pcibios.h>
#include <device/errno.h>
#include <device/misc_protos.h>
#include <device/ds_routines.h>
#include <sys/sdi.h>

#if NADSL > 0 || defined(BINARY)

extern struct hba_idata adslidata[];
extern struct intr_info adsl_intr_info[];

int adsl_probe(pcici_t config_id);
int adsl_attach(pcici_t config_id);

struct	adsl_softc {
        pcici_t tag;                    /* PCI config info */
};

#ifdef BINARY
extern	struct	adsl_softc adsl_softc[NADSL];
#else
struct	adsl_softc  adsl_softc[NADSL];
#endif

int adsl_units = 0;

struct pci_driver adaptec7850_device = {
        adsl_probe,
        adsl_attach,
        ADP_ID,
        ADP_7850,
        "adsl",
        "Adaptec 78XX SCSI Controller",
        NULL };

struct pci_driver adaptec7870_device = {
        adsl_probe,
        adsl_attach,
        ADP_ID,
        ADP_7870,
        "adsl",
        "Adaptec 78XX SCSI Controller",
        NULL };


/*
 *      adsl_probe is the probe for the PCI board.
 *
 *	Save IRQ info and indicate that a device wasn't found, 
 *	actual initialization takes place from scsi_hba_hdw.c
 */

int
adsl_probe(pcici_t config_id)
{
        int unit;
	unsigned int irq;
        struct adsl_softc *sc;

        if (adsl_units >= NADSL)
                return -1;
        for (unit = 0; unit < NADSL; unit++)
                if ((sc = &adsl_softc[unit])->tag.cfg1 == 0) {
                        sc->tag = config_id;
                        break;
                }

        if (unit >= NADSL)
                return -1;

        adsl_units = unit;

	/* save info in SVR4 driver config table */
	irq = pci_conf_read(config_id, 0x3c) & 0xF;
	adslidata[unit].iov = irq;
	adsl_intr_info[unit].ivect_no = irq;
        return unit;
}


/*
 * adsl_attach()
 * 	Interface exists: make available by filling in network interface
 * 	record.
 *
 * Arguments:
 *	config_id		Device identifier.
 *
 * Return Value:
 *	None.
 */
int adsl_attach(pcici_t config_id)
{
	return(KERN_SUCCESS);
}

#endif
