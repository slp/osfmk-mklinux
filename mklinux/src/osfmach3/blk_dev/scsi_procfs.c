/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/malloc.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/stat.h>
#include <linux/blk.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/malloc.h>

#include <asm/system.h>
#include <asm/irq.h>
#include <asm/dma.h>

#include "../../drivers/scsi/scsi.h"
#include "../../drivers/scsi/hosts.h"
#include "../../drivers/scsi/constants.h"

#include <osfmach3/server_thread.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/device_utils.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/mach_init.h>
#include <device/scsi_info.h>

char *osfmach3_scsi_info_name = "scsi_info";

extern int (* dispatch_scsi_info_ptr)(int ino, char *buffer, char **start,
                                      off_t offset, int length, int inout);
struct proc_dir_entry proc_scsi_scsi = {
    PROC_SCSI_SCSI, 4, "scsi",
    S_IFREG | S_IRUGO | S_IWUSR, 1, 0, 0, 0,
    NULL,
    NULL, NULL,
    NULL, NULL, NULL
};

#define MAX_SCSI_DEVICE_CODE 10
const char *const scsi_dev_types[MAX_SCSI_DEVICE_CODE] =
{
    "Direct-Access    ",
    "Sequential-Access",
    "Printer          ",
    "Processor        ",
    "WORM             ",
    "CD-ROM           ",
    "Scanner          ",
    "Optical Device   ",
    "Medium Changer   ",
    "Communications   "
};

void proc_print_scsidevice(Scsi_Device *scd, char *buffer, int *size, int len)
{	    
    int x, y = *size;
    
    y = sprintf(buffer + len, 
		    "Host: scsi%d Channel: %02d Id: %02d Lun: %02d\n  Vendor: ",
		    scd->host->host_no, scd->channel, scd->id, scd->lun);
    for (x = 0; x < 8; x++) {
	if (scd->vendor[x] >= 0x20)
	    y += sprintf(buffer + len + y, "%c", scd->vendor[x]);
	else
	    y += sprintf(buffer + len + y," ");
    }
    y += sprintf(buffer + len + y, " Model: ");
    for (x = 0; x < 16; x++) {
	if (scd->model[x] >= 0x20)
	    y +=  sprintf(buffer + len + y, "%c", scd->model[x]);
	else
	    y += sprintf(buffer + len + y, " ");
    }
    y += sprintf(buffer + len + y, " Rev: ");
    for (x = 0; x < 4; x++) {
	if (scd->rev[x] >= 0x20)
	    y += sprintf(buffer + len + y, "%c", scd->rev[x]);
	else
	    y += sprintf(buffer + len + y, " ");
    }
    y += sprintf(buffer + len + y, "\n");
    
    y += sprintf(buffer + len + y, "  Type:   %s ",
		     scd->type < MAX_SCSI_DEVICE_CODE ? 
		     scsi_dev_types[(int)scd->type] : "Unknown          " );
    y += sprintf(buffer + len + y, "               ANSI"
		     " SCSI revision: %02x", (scd->scsi_level < 3)?1:2);
    if (scd->scsi_level == 2)
	y += sprintf(buffer + len + y, " CCS\n");
    else
	y += sprintf(buffer + len + y, "\n");

    *size = y; 
    return;
}

static int osfmach3_construct_device(
    Scsi_Device *SDpnt,
    char *scsi_result)
{
  int  type=-1;

  /* Some low level driver could use device->type (DB) */
  SDpnt->type = -1;

  SDpnt->borken = 1;
  SDpnt->was_reset = 0;
  SDpnt->expecting_cc_ua = 0;

  /*
   * It would seem some TOSHIBA CDROM gets things wrong
   */
  if (!strncmp (scsi_result + 8, "TOSHIBA", 7) &&
      !strncmp (scsi_result + 16, "CD-ROM", 6) &&
      scsi_result[0] == TYPE_DISK) {
    scsi_result[0] = TYPE_ROM;
    scsi_result[1] |= 0x80;     /* removable */
  }

  if (!strncmp (scsi_result + 8, "NEC", 3)) {
    if (!strncmp (scsi_result + 16, "CD-ROM DRIVE:84 ", 16) ||
        !strncmp (scsi_result + 16, "CD-ROM DRIVE:25", 15))
      SDpnt->manufacturer = SCSI_MAN_NEC_OLDCDR;
    else
      SDpnt->manufacturer = SCSI_MAN_NEC;
  }
  else if (!strncmp (scsi_result + 8, "TOSHIBA", 7))
    SDpnt->manufacturer = SCSI_MAN_TOSHIBA;
  else if (!strncmp (scsi_result + 8, "SONY", 4))
    SDpnt->manufacturer = SCSI_MAN_SONY;
  else if (!strncmp (scsi_result + 8, "PIONEER", 7))
    SDpnt->manufacturer = SCSI_MAN_PIONEER;
  else
    SDpnt->manufacturer = SCSI_MAN_UNKNOWN;

  memcpy (SDpnt->vendor, scsi_result + 8, 8);
  memcpy (SDpnt->model, scsi_result + 16, 16);
  memcpy (SDpnt->rev, scsi_result + 32, 4);

  SDpnt->removable = (0x80 & scsi_result[1]) >> 7;
  SDpnt->lockable = SDpnt->removable;
  SDpnt->changed = 0;
  SDpnt->access_count = 0;
  SDpnt->busy = 0;
  SDpnt->has_cmdblocks = 0;
  /*
   * Currently, all sequential devices are assumed to be tapes, all random
   * devices disk, with the appropriate read only flags set for ROM / WORM
   * treated as RO.
   */
  switch (type = (scsi_result[0] & 0x1f)) {
  case TYPE_TAPE:
  case TYPE_DISK:
  case TYPE_MOD:
  case TYPE_PROCESSOR:
  case TYPE_SCANNER:
    SDpnt->writeable = 1;
    break;
  case TYPE_WORM:
  case TYPE_ROM:
    SDpnt->writeable = 0;
    break;
  }

  SDpnt->single_lun = 0;
  SDpnt->soft_reset =
    (scsi_result[7] & 1) && ((scsi_result[3] & 7) == 2);
  SDpnt->random = (type == TYPE_TAPE) ? 0 : 1;
  SDpnt->type = (type & 0x1f);
  SDpnt->scsi_level = scsi_result[2] & 0x07;
  if (SDpnt->scsi_level >= 2 ||
      (SDpnt->scsi_level == 1 &&
       (scsi_result[3] & 0x0f) == 1))
    SDpnt->scsi_level++;

  SDpnt->disconnect = 0;

  /*
   * Set the tagged_queue flag for SCSI-II devices that purport to support
   * tagged queuing in the INQUIRY data.
   */
  SDpnt->tagged_queue = 0;
  if ((SDpnt->scsi_level >= SCSI_2) &&
      (scsi_result[7] & 2)) {
    SDpnt->tagged_supported = 1;
    SDpnt->current_tag = 0;
  }
	return 0;
}

int scsi_proc_osfmach3(char *buffer, char **start, off_t offset, int length,
                    int hostno, int inout)
{
	int		count, devcount, size, len = 0, i;
	kern_return_t	kr;
	mach_port_t	info_port;
	scsi_info_t	*devices, *devmem;
	Scsi_Device	scd;
	struct Scsi_Host	host;
	off_t		begin = 0, pos = 0;

	kr = device_open(device_server_port,
			MACH_PORT_NULL,
			D_READ,
			server_security_token,
			osfmach3_scsi_info_name,
			&info_port);

	if (kr != KERN_SUCCESS)  {
		printk("OPEN ON <%s> FAILED %x\n",
			osfmach3_scsi_info_name,
			kr);
		return 0;
	}

	count = sizeof(int);
	devcount = 0;

	kr = device_get_status(info_port, SCSI_INFO_GET_DEVICE_COUNT,
			&devcount, &count);

	if (kr != KERN_SUCCESS || devcount == 0)  {

#if	CONFIG_OSFMACH3_DEBUG
		printk("Get Status Failed %x/%d\n",
			kr, devcount);
#endif	/* CONFIG_OSFMACH3_DEBUG */
		goto out;
	}

	devmem = devices = (scsi_info_t *) kmalloc((sizeof(*devices)*(devcount+1)),
			GFP_KERNEL);

	count = (devcount * sizeof(*devices)) / sizeof(int);

	kr = device_get_status(info_port, SCSI_INFO_GET_DEVICE_INFO,
			(int*) devices, &count);

	if (kr != KERN_SUCCESS) {
		printk("Failed to get device info %x\n", kr);
		kfree(devmem);
		goto out;
	}

	memset(&host,0, sizeof(host));
	memset(&scd, 0, sizeof(scd));
	scd.host = &host;

	len = sprintf(buffer, "Attached devices: %s\n", (devcount) ? "" : "none"); 
	for (i = 0; i < devcount; i++, devices++) {
		host.host_no = devices->controller;
		scd.id = devices->target_id;
		scd.lun = devices->lun;
		osfmach3_construct_device(&scd, devices->inquiry_data);
		proc_print_scsidevice(&scd, buffer, &size, len);
		len += size;
		pos = begin + len;
		if (pos < offset) {
			len = 0;
			begin = pos;
		}
		if (pos > offset + length)
			break;
	}

	kfree(devmem);
	*start = buffer + (offset - begin);
	len -= (offset - begin);
	if (len > length)
		len = length;

out:
	(void) device_close(info_port);
	return len;
}

int osfmach3_scsi_procfs_dispatch(int ino, char *buffer, char **start,
                                      off_t offset, int length, int func)
{
    if(ino == PROC_SCSI_SCSI) {
        return(scsi_proc_osfmach3(buffer, start, offset, length, 0, func));
    } else
	return (-EBADF);

}

void
osfmach3_scsi_procfs_init(void)
{
	dispatch_scsi_info_ptr = osfmach3_scsi_procfs_dispatch;
	proc_scsi_register(0, &proc_scsi_scsi);
}
