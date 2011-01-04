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

#ifndef	__ADB_IO_H_
#define	__ADB_IO_H_

/*
 * Default ADB ID's
 */

#define	ADB_DEV_PROTECT		1
#define	ADB_DEV_KEYBOARD	2
#define	ADB_DEV_MOUSE		3
#define	ADB_DEV_TABLET		4
#define	ADB_DEV_MODEM		5
#define	ADB_DEV_RESERVED6	6
#define	ADB_DEV_APPL		7

/*
 * Number of devices
 */

#define	ADB_DEVICE_COUNT	16	/* Note ID 0 is special */

#define	ADB_IOC		('A'<<8)
#define	ADB_GET_INFO	(ADB_IOC | 1)
#define	ADB_READ_REG	(ADB_IOC | 2)
#define	ADB_WRITE_REG	(ADB_IOC | 3)
#define	ADB_GET_COUNT	(ADB_IOC | 4)
#define	ADB_SET_HANDLER (ADB_IOC | 5)

#define	ADB_READ_DATA	(ADB_IOC | 10)	/* For MACH and the Server */

struct adb_info {
	unsigned short a_type;		/* Type / Original address */
	unsigned short a_addr;		/* Current address */
	unsigned short a_handler;	/* Current Handler */
	unsigned short a_orighandler;	/* Handler ID at boot */
	unsigned long	_pad[5];	/* extra info */
};

struct adb_regdata {
	unsigned char	a_addr;		/* Device to talk to*/
	unsigned char	a_reg;		/* Register */
	unsigned short	a_count;	/* Size of buffer */
	unsigned char	a_buffer[32];	/* Buffer */
};

/*
 * Layout of extended mouse register 1
 */
struct adb_extmouse {
	unsigned char	m_id[4];		/* Unique identifier*/
	unsigned char	m_resolution[2];	/* Resolution in units/inch*/
	unsigned char	m_class;		/* Class of mouse*/
	unsigned char	m_buttons;		/* Number of buttons*/
};

#endif /* __ADB_IO_H_ */
