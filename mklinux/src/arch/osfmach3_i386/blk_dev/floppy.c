/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#include <machine/disk.h>

#include <osfmach3/device_utils.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/assert.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/block_dev.h>

#include <linux/fd.h>
#include <linux/kernel.h>
#include <linux/errno.h>

#include "mach_ioctl.h"

#if	CONFIG_OSFMACH3_DEBUG
#define FLOPPY_DEBUG	1
#endif	/* CONFIG_OSFMACH3_DEBUG */

#ifdef	FLOPPY_DEBUG
extern int floppy_debug;
#endif	/* FLOPPY_DEBUG */

extern struct floppy_struct floppy_type[];
#define TYPE(x) ( ((x)>>2) & 0x1f )

int floppy_interleave = 1;	/* XXX what's the correct value ? */

/*
 * Mach floppy device naming convention:
 * 	fd0a: 	3.50"  720 KB
 * 	fd0b:	3.50" 1.44 MB
 * 	fd0c:	5.25"  360 KB
 * 	fd0d:	5.25" 1.20 MB
 *
 * This array should reflect what's in the 'floppy_type' array above.
 */
char floppy_mach_minor[32] = {
	'?',	/* 0 no testing		*/
	'c',	/* 1 360KB PC		*/
	'd',	/* 2 1.2MB AT		*/
	'?',	/* 3 360KB SS 3.5"	*/
	'a',	/* 4 720KB 3.5"		*/
	'c',	/*  5 360KB AT      */
	'?',	/*  6 720KB AT      */
	'b',	/*  7 1.44MB 3.5"   */
	'?',	/*  8 2.88MB 3.5"   */
	'?',	/*  9 2.88MB 3.5"   */

	'?',	/* 10 1.44MB 5.25"  */
	'?',	/* 11 1.68MB 3.5"   */
	'?',	/* 12 410KB 5.25"   */
	'?',	/* 13 820KB 3.5"    */
	'?',	/* 14 1.48MB 5.25"  */
	'?',	/* 15 1.72MB 3.5"   */
	'?',	/* 16 420KB 5.25"   */
	'?',	/* 17 830KB 3.5"    */
	'?',	/* 18 1.49MB 5.25"  */
	'?',	/* 19 1.74 MB 3.5"  */

	'?',	/* 20 880KB 5.25"   */
	'?',	/* 21 1.04MB 3.5"   */
	'?',	/* 22 1.12MB 3.5"   */
	'?',	/* 23 1.6MB 5.25"   */
	'?',	/* 24 1.76MB 3.5"   */
	'?',	/* 25 1.92MB 3.5"   */
	'?',	/* 26 3.20MB 3.5"   */
	'?',	/* 27 3.52MB 3.5"   */
	'?',	/* 28 3.84MB 3.5"   */

	'?',	/* 29 1.84MB 3.5"   */
	'?',	/* 30 800KB 3.5"    */
	'?'	/* 31 1.6MB 3.5"    */
};
	
int machine_floppy_format(kdev_t device, struct format_descr *tmp_format_req)
{
	union io_arg	ia;
	int 		type;
	mach_port_t	device_port;
	mach_msg_type_number_t count;
	kern_return_t	kr;

	type = TYPE(device);
	ASSERT(type > 0 && type < 32);

	ia.ia_fmt.start_trk = tmp_format_req->head * floppy_type[type].track
		+ tmp_format_req->track;
	ia.ia_fmt.num_trks = 1;
	ia.ia_fmt.intlv = floppy_interleave;

	device_port = bdev_port_lookup(device);
	if (device_port == MACH_PORT_NULL) {
		panic("machine_floppy_format: no mach device for dev %s\n",
		      kdevname(device));
	}

#ifdef	FLOPPY_DEBUG
	if (floppy_debug) {
		printk("machine_floppy_format(dev=%s) "
		       "req=(device=0x%x,head=0x%x,track=0x%x) "
		       "io_arg=(start_trk=0x%x,num_trks=0x%x,intlv=0x%x)\n",
		       kdevname(device),
		       tmp_format_req->device, tmp_format_req->head,
		       tmp_format_req->track,
		       ia.ia_fmt.start_trk, ia.ia_fmt.num_trks,
		       ia.ia_fmt.intlv);
	}
#endif	/* FLOPPY_DEBUG */

	count = sizeof (ia) / sizeof (int);
	server_thread_blocking(FALSE);
	kr = device_set_status(device_port,
			       OSFMACH3_V_FORMAT,
			       (dev_status_t) &ia,
			       count);
	server_thread_unblocking(FALSE);
	if (kr != D_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("machine_floppy_format(dev=%s): "
			     "device_set_status(0x%x, V_FORMAT"	
			     "(start_trk=0x%x,num_trks=0x%x,intlv=0x%x)",
			     kdevname(device), device_port,
			     ia.ia_fmt.start_trk,
			     ia.ia_fmt.num_trks,
			     ia.ia_fmt.intlv));
		return -EINVAL;
	}

	return 0;
}
