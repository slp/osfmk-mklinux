/* fdomain.c -- Future Domain TMC-16x0 SCSI driver
 * Created: Sun May  3 18:53:19 1992 by faith@cs.unc.edu
 * Revised: Sat Nov  2 09:27:47 1996 by root@cs.unc.edu
 * Author: Rickard E. Faith, faith@cs.unc.edu
 * Copyright 1992, 1993, 1994, 1995, 1996 Rickard E. Faith
 *
 * $Id: fdomain.c,v 5.44 1996/08/08 18:58:53 root Exp $

 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.

 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.

 **************************************************************************

 SUMMARY:

 Future Domain BIOS versions supported for autodetect:
    2.0, 3.0, 3.2, 3.4 (1.0), 3.5 (2.0), 3.6, 3.61
 Chips are supported:
    TMC-1800, TMC-18C50, TMC-18C30, TMC-36C70
 Boards supported:
    Future Domain TMC-1650, TMC-1660, TMC-1670, TMC-1680, TMC-1610M/MER/MEX
    Future Domain TMC-3260 (PCI)
    Quantum ISA-200S, ISA-250MG
    Adaptec AHA-2920 (PCI)
    IBM ?
 LILO command-line options:
    fdomain=<PORT_BASE>,<IRQ>[,<ADAPTER_ID>]



 DESCRIPTION:
 
 This is the Linux low-level SCSI driver for Future Domain TMC-1660/1680
 TMC-1650/1670, and TMC-3260 SCSI host adapters.  The 1650 and 1670 have a
 25-pin external connector, whereas the 1660 and 1680 have a SCSI-2 50-pin
 high-density external connector.  The 1670 and 1680 have floppy disk
 controllers built in.  The TMC-3260 is a PCI bus card.

 Future Domain's older boards are based on the TMC-1800 chip, and this
 driver was originally written for a TMC-1680 board with the TMC-1800 chip.
 More recently, boards are being produced with the TMC-18C50 and TMC-18C30
 chips.  The latest and greatest board may not work with this driver.  If
 you have to patch this driver so that it will recognize your board's BIOS
 signature, then the driver may fail to function after the board is
 detected.

 Please note that the drive ordering that Future Domain implemented in BIOS
 versions 3.4 and 3.5 is the opposite of the order (currently) used by the
 rest of the SCSI industry.  If you have BIOS version 3.4 or 3.5, and have
 more then one drive, then the drive ordering will be the reverse of that
 which you see under DOS.  For example, under DOS SCSI ID 0 will be D: and
 SCSI ID 1 will be C: (the boot device).  Under Linux, SCSI ID 0 will be
 /dev/sda and SCSI ID 1 will be /dev/sdb.  The Linux ordering is consistent
 with that provided by all the other SCSI drivers for Linux.  If you want
 this changed, you will probably have to patch the higher level SCSI code.
 If you do so, please send me patches t