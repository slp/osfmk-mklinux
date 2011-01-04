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
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)device.h	7.3 (Berkeley) 5/7/91
 */

#ifndef _HP_PA_HP700_DEVICE_H_
#define	_HP_PA_HP700_DEVICE_H_

#include <device/tty.h>

struct hp_device {
	struct driver	*hp_driver;
	struct driver	*hp_cdriver;
	int		hp_unit;
	int		hp_ctlr;
	int		hp_slave;
	char		*hp_addr;
	int		hp_dk;
	int		hp_flags;
	int		hp_alive;
	int		hp_ipl;
};

struct driver {
	int	(*d_init)(struct hp_device *hd);
	char	*d_name;
	int	(*d_start)(struct tty*);
	int	(*d_go)(void);
	int	(*d_intr)(void);
	int	(*d_done)(void);
};

struct hp_ctlr {
	struct driver	*hp_driver;
	int		hp_unit;
	int		hp_alive;
	char		*hp_addr;
	int		hp_flags;
	int		hp_ipl;
};

struct	devqueue {
	struct	devqueue *dq_forw;
	struct	devqueue *dq_back;
	int	dq_ctlr;
	int	dq_unit;
	int	dq_slave;
	struct	driver *dq_driver;
};

#define	MAXCTLRS	16	/* Size of HW table (arbitrary) */
#define	MAXSLAVES	8	/* Slaves per controller (HPIB/SCSI limit) */

struct hp_hw {
	caddr_t	hw_pa;		/* physical address of control space */
	int	hw_size;	/* size of control space */
	caddr_t	hw_kva;		/* kernel virtual address of control space */
	short	hw_id;		/* HW returned id */
	short	hw_secid;	/* secondary HW id (displays) */
	short	hw_type;	/* type (defined below) */
	short	hw_sc;		/* select code (if applicable) */
};

/* bus types */
#define	B_MASK		0xE000
#define	B_DIO		0x2000
#define B_DIOII		0x4000
#define B_VME		0x6000
/* controller types */
#define	C_MASK		0x8F
#define C_FLAG		0x80
#define	C_HPIB		0x81
#define C_SCSI		0x82
#define C_VME		0x83
/* device types (controllers with no slaves) */
#define D_MASK		0x8F
#define	D_BITMAP	0x01
#define	D_LAN		0x02
#define	D_FPA		0x03
#define	D_KEYBOARD	0x04
#define	D_COMMDCA	0x05
#define	D_COMMDCM	0x06
#define	D_COMMDCL	0x07
#define	D_PPORT		0x08
#define D_POWER		0x09
#define	D_MISC		0x7F

#define HW_ISCTLR(hw)	((hw)->hw_type & C_FLAG)
#define HW_ISDIOII(hw)	((hw)->hw_type & B_DIOII)
#define HW_ISHPIB(hw)	(((hw)->hw_type & C_MASK) == C_HPIB)
#define HW_ISSCSI(hw)	(((hw)->hw_type & C_MASK) == C_SCSI)
#define HW_ISDEV(hw,d)	(((hw)->hw_type & D_MASK) == (d))

/* 
 * informations at bus scan for attach routines.
 * should be cleaned up in a more generic way.
 */
struct hp_businfo {
	int	bustype;
};

#define	MAXCSRS	1
typedef struct hp_ctlrinfo {
	int		   ctlrnum;
	struct hp_ctlrinfo *parent_ctlrinfop;
	struct hp_businfo  *ctlr_businfop;
	int                irq;
	int		   numcsrs;
	vm_offset_t        csrs[MAXCSRS];
	vm_offset_t	   vcsrs[MAXCSRS];
} hp_ctlrinfo_t;

#define BUSTYPE_NOSUCHBUS	0
#define BUSTYPE_SYS		1
#define BUSTYPE_EISA		2
#define BUSTYPE_ZALON		3

#endif /* _HP_PA_HP700_DEVICE_H_ */
