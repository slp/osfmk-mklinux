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
 * Structure of LIF information on HP-UX disks.
 * Since we have no sources, all this is pure guess
 * based on lifls command ouput and hex dump of boot block.
 */

#define LIF_HEADER_OFF	0

#define LIF_MAGIC	0x8000

struct lif_header {
	unsigned short	magic;			
	unsigned char	name[6];	
};

#define IS_OSF1_DISK(lif) (lif->name[0] == 0)


#define LIF_LABEL_OFF	0x800
#define LIF_MAX_PART	8

/* partition types */

#define LIF_FS		0xcd38	
#define LIF_SWAP	0x5243
#define LIF_ISL		0xce00
#define LIF_AUTO	0xcfff
#define LIF_HPUX	0xcd80
#define LIF_IOMAP	0xcd60
#define LIF_EST		0xd001
#define LIF_PAD		0xcffe

struct lif_partition {
	unsigned char	name[10];
	unsigned short 	type;
	unsigned int	start;		/* 256 bytes unit */
	unsigned int	size;		/* 256 bytes unit */
	unsigned char	date[6];	/* strange format, each deciomal digit 
					 * is encoded as a 4 bit hex digit
					 * 92/12/08 15:48:13 -> 92 12 08 15 48 13 */
	unsigned short 	magic;		/* seems to always be 0x8001 */
	unsigned int	filler;		/* One partition descriptor is 32 bytes long */ 
};
					
	
struct lif_label {
	struct lif_partition part[8];
};
