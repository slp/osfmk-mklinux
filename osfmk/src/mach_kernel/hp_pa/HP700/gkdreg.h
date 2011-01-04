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
 * 	Utah $Hdr: gkdreg.h 1.4 94/12/14$
 */

/*
 *  Structure describing the registers of the PS2 interface on
 *  HP Geckos.
 *
 *         Offset      Register
 *           0           id      (r)
 *           0           reset   (w)
 *           4           data    (r/w)
 *           8           control (r/w)
 *          12           status  (r)
 */

struct gkd_reg {
	volatile u_int           :24,
                         id_reset:8;  /* Read returns ID, write resets*/
	volatile u_int           :24,
	                     data:8;  /* read: receive, write: transmit */ 
	volatile u_int           :24,
	                  control:8;  /* Control register */
	volatile u_int           :24,
                           status:8;  /* Status register */
};

/*
 *  Some constants
 */
#define   GKD_PADDING    256

/*
 *  Given an address, return next port
 */
#define   gkd_next(a)    ((struct gkd_reg *)((caddr_t)a + GKD_PADDING))

/*
 *  Enable/Disable a port given a pointer
 */
#define   gkd_disable(a)  ((struct gkd_reg *)a)->control = 0
#define   gkd_enable(a)   ((struct gkd_reg *)a)->control = 1

/* PS/2 port status register bit masks */

#define PS2_CLKSHD		0x80    /* Clock line shadow */
#define PS2_DATSHD		0x40    /* Data line shadow */
#define PS2_CMPINT		0x10    /* Composite interrupt */
#define PS2_TERR		0x08	/* Timeout error */
#define PS2_PERR		0x04	/* Parity error */
#define PS2_TBNE		0x02	/* Transmit buffer not empty */
#define PS2_RBNE		0x01	/* Receive buffer not empty  */

/*
 * Commands to keyboard
 */
#define   GKD_KBD_LEDS     0xED	        /* Set LEDS */


/* Constants used in operations */

#define PS2_MAX_READTRIES       4       /* max number of read retrys      */
#define PS2_MAX_RETRANS         4       /* Max number of retrans attempts */
#define PS2_RINGBUF_SIZE        32      /* Size of input ring bufer for reads */
#define PS2_MAX_INTERFACES      1       /* Max number of interfaces per host */
#define PS2_MAX_PORTS           16      /* Max number of ports per interface */
#define PS2_READ_TIMEOUT_SECONDS  4     /* Max number of seconds that we wait
					   for a response from the device */
/*
 * Status values for ps2_cmd.status
 */

#define PS2_CTL_FREE            0       /* This value MUST be zero */
#define PS2_SUCCESS     	1       /* Successful command complete */
#define PS2_CTL_ALLOCATED       2       /* Allocated */
#define PS2_CTL_XMIT_ERROR      3       /* Byte transmission error */
#define PS2_CTL_READ_TIMEOUT    4	/* Read timeout */
#define PS2_CTL_INVALID		5       /* Invalid response byte */
#define PS2_CTL_RETRANS_MAX	7	/* Max retrans count exceeded */

#define PS2_500mSec_Try 	1000

/*
 * call type parameter to ps2_state()
 */

#define PS2_WAS_RETRANS 	5
#define PS2_NO_INPUT		6
#define PS2_VALID_BYTE		7

/* Return status values */

#define PS2_TIMEOUT     	8
#define PS2_FAIL        	9
#define PS2_NO_ACK      	10


/* values returned by devices */

#define PS2_TEST_SUCCESS	0xAA
#define PS2_RETRANS_REQ		0xFE    /* also sent to a device */
#define PS2_ACK			0xFA
#define PS2_MOUSE_ID		0x00
#define PS2_KEYBOARD_ID_BYTE1	0xAB
#define PS2_KEYBOARD_ID_BYTE2	0x83

/*
 * Values returned
 */
#define   GKD_DEV_ACK      0xFA
#define   GKD_DEV_RETRY    0XFE 

/* commands sent to devices */

#define PS2_1TO1		0xE6
#define PS2_2TO1		0xE7
#define PS2_RESOL               0xE8
#define PS2_STATS		0xE9
#define PS2_STREAM		0xEA
#define PS2_REPT		0xEB
#define PS2_INDICAT		0xED
#define PS2_SCANCOD		0xF0
#define PS2_PMTMODE		0xF0
#define PS2_ID   		0xF2
#define PS2_SAMPRAT             0xF3
#define PS2_RATDEL              0xF3
#define PS2_ENAB		0xF4
#define PS2_DEFAULT_DISABLE	0xF5
#define	PS2_DISAB		0xF5
#define PS2_SETDEF		0xF6
#define PS2_ALLKEY_T            0xF7
#define PS2_ALLKEY_MB           0xF8
#define PS2_ALLKEY_M            0xF9
#define PS2_ALLKEY_TMB          0xFA
#define PS2_KEY_T		0xFB
#define PS2_KEY_MB		0xFC
#define PS2_KEY_M	 	0xFD
#define PS2_RESET_CMD		0xFF

/* PS/2 port control register bit masks */

#define PS2_ENBL		0x01	/* Enable bit 		 */
#define PS2_LPBXR		0x02	/* Loopback mode control */
#define PS2_DIAG		0x20    /* Diagnostic mode control */
#define PS2_DATDIR		0x40	/* External data line control  */
#define PS2_CLKDIR		0x80	/* External clock line control */

