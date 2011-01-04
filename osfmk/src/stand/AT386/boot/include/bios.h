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

/* BIOS interface */

/* COM PORT Parameter byte */

#define COM_CS7		0x02
#define COM_CS8		0x03
#define COM_1STOP	0x00
#define COM_2STOP	0x04
#define COM_NOPAR	0x00
#define COM_PARODD	0x08
#define COM_PAREVEN	0x18
#define COM_1200	0x80
#define COM_2400	0xa0
#define COM_4800	0xc0
#define COM_9600	0xe0

/* COM line status byte */

#define COM_TIME_OUT	0x80
#define COM_TSRE	0x40
#define	COM_THRE	0x20
#define COM_BREAK	0x10
#define COM_FRAME_ERR	0x08
#define COM_PARITY_ERR	0x04	
#define COM_OVERRUN_ERR	0x02
#define COM_DATA_READY	0x01

/* Drive ids */

#define	BIOS_DEV_FLOPPY	0x0
#define	BIOS_DEV_WIN	0x80
#define BIOS_FAKE_WD    0xff    /* BIOS "network" device */
