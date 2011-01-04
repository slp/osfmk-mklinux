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
 * Automatically generated C config: don't edit
 */

/*
 * General setup
 */
#undef  CONFIG_MATH_EMULATION
#define CONFIG_BLK_DEV_FD 1
#define CONFIG_ST506 1

/*
 * Please see drivers/block/README.ide for help/info on IDE drives
 */
#undef  CONFIG_BLK_DEV_HD
#define CONFIG_BLK_DEV_IDE 1
#undef  CONFIG_BLK_DEV_IDECD
#undef  CONFIG_BLK_DEV_XD
#define CONFIG_NET 1
#undef  CONFIG_MAX_16M
#undef  CONFIG_PCI
#define CONFIG_SYSVIPC 1
#define CONFIG_BINFMT_ELF 1
#define CONFIG_M486 1

/*
 * Loadable module support
 */
#undef  CONFIG_MODVERSIONS

/*
 * Networking options
 */
#define CONFIG_INET 1
#undef  CONFIG_IP_FORWARD
#undef  CONFIG_IP_MULTICAST
#undef  CONFIG_IP_FIREWALL
#undef  CONFIG_IP_ACCT

/*
 * (it is safe to leave these untouched)
 */
#undef  CONFIG_INET_PCTCP
#undef  CONFIG_INET_RARP
#define CONFIG_INET_SNARL 1
#undef  CONFIG_TCP_NAGLE_OFF
#undef  CONFIG_IPX

/*
 * SCSI support
 */
#define CONFIG_SCSI 1

/*
 * SCSI support type (disk, tape, CDrom)
 */
#define CONFIG_BLK_DEV_SD 1
#define CONFIG_CHR_DEV_ST 1
#define CONFIG_BLK_DEV_SR 1
#define CONFIG_CHR_DEV_SG 1

/*
 * SCSI low-level drivers
 */
#define CONFIG_SCSI_AHA152X 1
#define CONFIG_SCSI_AHA1542 1
#define CONFIG_SCSI_AHA1740 1
#define CONFIG_SCSI_AHA274X 1
#undef  CONFIG_SCSI_BUSLOGIC
#undef  CONFIG_SCSI_EATA_DMA
#undef  CONFIG_SCSI_U14_34F
#undef  CONFIG_SCSI_FUTURE_DOMAIN
#undef  CONFIG_SCSI_GENERIC_NCR5380
#undef  CONFIG_SCSI_IN2000
#undef  CONFIG_SCSI_PAS16
#undef  CONFIG_SCSI_QLOGIC
#undef  CONFIG_SCSI_SEAGATE
#undef  CONFIG_SCSI_T128
#undef  CONFIG_SCSI_ULTRASTOR
#undef  CONFIG_SCSI_7000FASST

/*
 * Network device support
 */
#define CONFIG_NETDEVICES 1
#define CONFIG_DUMMY 1
#define CONFIG_SLIP 1
#define CONFIG_SLIP_COMPRESSED 1
#define SL_SLIP_LOTS 1
#define CONFIG_PPP 1
#define CONFIG_PLIP 1
#undef  CONFIG_NET_ALPHA
#define CONFIG_NET_VENDOR_SMC 1
#define CONFIG_WD80x3 1
#define CONFIG_ULTRA 1
#undef  CONFIG_LANCE
#undef  CONFIG_NET_VENDOR_3COM
#undef  CONFIG_NET_ISA
#undef  CONFIG_NET_EISA
#undef  CONFIG_NET_POCKET

/*
 * CD-ROM drivers (not for SCSI or IDE/ATAPI drives)
 */
#undef  CONFIG_CDU31A
#undef  CONFIG_MCD
#undef  CONFIG_SBPCD
#undef  CONFIG_AZTCD
#undef  CONFIG_CDU535

/*
 * Filesystems
 */
#define CONFIG_MINIX_FS 1
#undef  CONFIG_EXT_FS
#define CONFIG_EXT2_FS 1
#undef  CONFIG_XIA_FS
#define CONFIG_MSDOS_FS 1
#undef  CONFIG_UMSDOS_FS
#define CONFIG_PROC_FS 1
#define CONFIG_NFS_FS 1
#define CONFIG_ISO9660_FS 1
#undef  CONFIG_HPFS_FS
#undef  CONFIG_SYSV_FS

/*
 * character devices
 */
#undef  CONFIG_CYCLADES
#undef  CONFIG_PRINTER
#define CONFIG_BUSMOUSE 1
#define CONFIG_PSMOUSE 1
#define CONFIG_82C710_MOUSE 1
#define CONFIG_MS_BUSMOUSE 1
#define CONFIG_ATIXL_BUSMOUSE 1
#undef  CONFIG_QIC02_TAPE
#undef  CONFIG_FTAPE

/*
 * Sound
 */
#undef  CONFIG_SOUND

/*
 * Kernel hacking
 */
#undef  CONFIG_PROFILE
#define CONFIG_SCSI_CONSTANTS 1
