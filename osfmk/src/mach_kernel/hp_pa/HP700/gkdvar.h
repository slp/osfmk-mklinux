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
 * Copyright (c) 1994, The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: gkdvar.h 1.4 94/12/14$
 */

struct gkd_dev {
	struct	gkd_reg	*regs;	/* port registers */
	int	     dev_type;	/* device type */
	short		flags;	/* misc flags */
	short            leds;	/* keyboard led states */
	short        released;  /* something released ? */
	short            spec;  /* something special going on .. */
	short          in_cmd;	/* processing a keyboard command */
	short       cmd_state;	/* current state command is in */
	short	     cmd_serr;  /* error count in this state */	
	short	     cmd_terr;  /* error count total for command */
	void     (*cmd_func)(struct gkd_dev *, unsigned char);  /* current command function */
	unsigned char *cmd_ptr;	/* pointer to command */
	short       cmd_nread;  /* number of bytes read for this state */
	short       cmd_nindx;  /* index into cmd_buf of returned chars */

#define GKD_CMD_LEN 4
	unsigned char      
		cmd_buf[GKD_CMD_LEN];	/* Command buffer */
	struct tty tty;		/* input queue */
};

#define	GKD_MAX_PORTS	2	/* we only support 2 ports */

/* 
 * Flags
 */
#define	F_GKD_INITED	0x0001	/* we've inited the interface */
#define F_GKD_SHIFT     0x0010  /* kbd: shift is down */
#define F_GKD_CTRL      0x0020  /* kbd: control is down */
#define F_GKD_CAPS      0x0040  /* kbd: CAPS is on */
#define F_GKD_ALT       0x0080  /* kbd: ALT is on */
#define F_GKD_NUMLK     0x0100  /* kbd: NUMLOCK is on */
#define F_GKD_SCRLK     0x0200  /* kbd: SCROLL LOCK is on */
#define F_KBD_FLAGS     0x03F0  /* kbd: isolate just the keyboard flags */
#define F_KBD_SCC       0x0070  /* kbd: isolate just shift control & caps */
#define F_GKD_OPEN      0x0400  /* device has been opened */
#define F_GKD_NOBLOCK   0x0800  /* device opened with FNDELAY */
#define F_GKD_ASLEEP    0x1000  /* process asleep on this device */
#define F_GKD_CSLEEP	0x2000	/* process issued a command and is sleep */

/*
 * Keyboard LED states
 */
#define KBD_NONE        0x0	/* kbd: shut off all leds */
#define KBD_SCRL_LOCK   0x1	/* kbd: scroll lock on */
#define KBD_NUM_LOCK    0x2	/* kbd: kbd numlock on */
#define KBD_CAP_LOCK    0x4	/* kbd: caps lock on */

/*
 * Keyboard codes of interest
 */
#define GKD_KEY_UP	0xf0    /* a key was depressed */
#define GKD_LEFT_SHIFT  0x12    /* left shift pressed */
#define GKD_RIGHT_SHIFT 0x59    /* right shift pressed */
#define GKD_CTRL        0x14    /* control pressed */
#define GKD_ALT         0x11    /* alt key pressed */
#define GKD_CAPS        0x58    /* CAPS was pressed */
#define GKD_NUM_LCK     0x77    /* Numlock was pressed */
#define GKD_SCROLL_LCK  0x7e    /* Scroll lock was pressed */

#define GKD_SPEC_KEY1   0xe0    /* starting special key sequence 1 */
#define GKD_SPEC_KEY2   0xe1    /* starting special key sequence 2 */
#define GKD_SPEC_KEY3   0xe2    /* state value for sysreq key */

/*
 * Compatibility between HIL & GKD for ITE
 */
#define GKBD_CTRLSHIFT  0x80    /* key + CTRL + SHIFT */
#define GKBD_CTRL       0x90    /* key + CTRL */
#define GKBD_SHIFT      0xA0    /* key + SHIFT */
#define GKBD_KEY        0xB0    /* key only */

/*
 * Special return codes
 */
#define GKD_DONE        0
#define GKD_NEED_MORE   1

/*
 * Calculate pointers into command string
 */
#define GKD_GSB(dev)   (dev->cmd_ptr[dev->cmd_state  + 2])
#define GKD_GSN(dev)   (dev->cmd_ptr[1])
#define GKD_SNUM(dev)  (dev->cmd_ptr[0])

/*
 * Size of PS2 read buffer
 */
#define GKDBUFSIZE      128

extern int gkdprobe(struct hp_device *);
extern void gkdattach(struct hp_device *);
extern void gkdkbdenable(int unit);
extern void gkd_chk_numlock(char *, char);
extern void gkdkbddisable(int);
extern void gkdkbdbell(int);
extern int gkdkbdgetc(int, int*, int);
extern void gkditefilter(struct gkd_dev *, char, char);
