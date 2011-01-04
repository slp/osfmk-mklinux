/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#include <osfmach3/device_utils.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/assert.h>
#include <osfmach3/server_thread.h>
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
	printk("machine_floppy_format: not implemented");
	return -EOPNOTSUPP;
}
