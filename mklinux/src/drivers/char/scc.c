#define RCS_ID "$Id: scc.c,v 1.67 1997/04/10 15:24:41 jreuter Exp jreuter $"

#define BANNER "Z8530 SCC driver version 2.4c.dl1bke (experimental) by DL1BKE\n"

/*

   ********************************************************************
   *   SCC.C - Linux driver for Z8530 based HDLC cards for AX.25      *
   ********************************************************************


   ********************************************************************

	Copyright (c) 1993, 1997 Joerg Reuter DL1BKE

	portions (c) 1993 Guido ten Dolle PE1NNZ

   ********************************************************************
   
   The driver and the programs in the archive are UNDER CONSTRUCTION.
   The code is likely to fail, and so your kernel could --- even 
   a whole network. 

   This driver is intended for Amateur Radio use. If you are running it
   for commercial purposes, please drop me a note. I am nosy...

   ...BUT:
 
   ! You  m u s t  recognize the appropriate legislations of your country !
   ! before you connect a radio to the SCC board and start to transmit or !
   ! receive. The GPL allows you to use the  d r i v e r,  NOT the RADIO! !

   For non-Amateur-Radio use please note that you might need a special
   allowance/licence from the designer of the SCC Board and/or the
   MODEM. 

   This program is free software; you can redistribute it and/or modify 
   it under the terms of the (modified) GNU General Public License 
   delivered with the Linux kernel source.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should find a copy of the GNU General Public License in 
   /usr/src/linux/COPYING; 
   
   ******************************************************************** 

		
   Incomplete history of z8530drv:
   -------------------------------

   940913	- started to write the driver, rescued most of my own
		  code (and Hans Alblas' memory buffer pool concept) from 
		  an earlier project "sccdrv" which was initiated by 
		  Guido ten Dolle. Not much of the old driver survived, 
		  though. The first version I put my hands on was sccdrv1.3
		  from August 1993. The memory buffer pool concept
		  appeared in an unauthorized sccdrv version (1.5) from
		  August 1994.

   950131	- changed copyright notice to GPL without limitations.
   
     .
     .	<snip!>
     .
      
   951114	- rewrote memory management, took first steps to allow
		  compilation as a module. Well, it looks like a whole
		  new driver now.

   960413	- Heiko Eissfeld fixed missing verify_area() calls

   960507	- the driver may be used as a network driver now.
 		  (for kernel AX.25). 

   960512	- added scc_net_ioctl(), restructured ioctl routines
   		  added /proc filesystem support
   		  
   960517	- fixed fullduplex mode 2 operation and waittime bug
   
   960518	- added fullduplex mode 3 to allow direct hardware
   		  access via protocol layer. Removed kiss.not_slip,
   		  I do not think someone really needed it.
   		  
   960607	- switching off receiver while transmission in halfduplex
   		  mode with external MODEM clock.
   		  
   960715	- rewrote interrupt service routine for "polling" mode,
   		  fixed bug in grouping algorithm.
   		  
   960719	- New transmit timer routines (let's see what it breaks),
   		  some improvements regarding DCD and SYNC interrupts,
   		  clean-up of printk().
   		  
   960725	- Fixed Maxkeyup problems. Will generate HDLC abort and
   		  recover now.

   960808	- Maxkeyup will set dev->tbusy until all remaining frames
   		  are sent.
   		  
   970115	- Fixed return values in scc_net_tx(), added missing
   		  scc_enqueue_buffer()
   		  
   970410	- Fixed problem with new *_timer() code in sched.h for
   		  kernel 2.0.30.

   Thanks to:
   ----------
   
   PE1CHL Rob	  - for a lot of good ideas from his SCC driver for DOS
   PE1NNZ Guido   - for his port of the original driver to Linux
   KA9Q   Phil    - from whom we stole the mbuf-structure
   PA3AYX Hans    - for some useful changes
   DL8MBT Flori   - for support
   DG0FT  Rene    - for the BayCom USCC support
   PA3AOU Harry   - for ESCC testing, information supply and support
   Heiko Eissfeld - verify_area() clean-up
   DL3KHB Klaus   - fullduplex mode 2 bug-hunter
   PA3GCU Richard - for support and finding various bugs.

   PE1KOX Rob, DG1RTF Thomas, ON5QK Roland, G4XYW Andy, Linus,
   GW4PTS Alan, EI9GL Paul, G0DZB Peter, G8IMB Martin

   and all who sent me bug reports and ideas... 
   
   
   NB -- if you find errors, change something, please let me know
      	 first before you distribute it... And please don't touch
   	 the version number. Just replace my callsign in
   	 "v2.4c.dl1bke" with your own. Just to avoid confusion...

   If you want to add your modification to the linux distribution
   please (!) contact me first.
   
   New versions of the driver will be announced on the linux-hams
   mailing list on vger.rutgers.edu. To subscribe send an e-mail
   to majordomo@vger.rutgers.edu with the following line in
   the body of the mail:
   
	   subscribe linux-hams
	   
   The content of the "Subject" field will be ignored.

   vy 73,
   Joerg Reuter	ampr-net: dl1bke@db0pra.ampr.org
		AX-25   : DL1BKE @ DB0ACH.#NRW.DEU.EU
		Internet: jreuter@poboxes.com  
*/

/* ----------------------------------------------------------------------- */

#undef  SCC_DELAY 	/* perhaps your ISA bus is a *bit* too fast? */
#undef  SCC_LDELAY 1	/* slow it even a bit more down */
#undef  DONT_CHECK	/* don't look if the SCCs you specified are available */

#define MAXSCC          4       /* number of max. supported chips */
#define RXBUFFERS       8       /* default number of RX buffers */
#define TXBUFFERS       8       /* default number of TX buffers */
#define BUFSIZE         384     /* must not exceed 4096-sizeof(mbuf) */
#undef  DISABLE_ALL_INTS	/* use cli()/sti() in ISR instead of */
				/* enable_irq()/disable_irq()        */
#undef	SCC_DEBUG

#define DEFAULT_CLOCK	4915200 /* default pclock if nothing is specified */

/* ----------------------------------------------------------------------- */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/tqueue.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/major.h>
#include <linux/termios.h>
#include <linux/serial.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/in.h>
#include <linux/fcntl.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/delay.h>

#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/socket.h>

#ifdef CONFIG_SCC_STANDALONE
#include "scc.h"
#else
#include <linux/scc.h>
#endif

#include <net/ax25.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>
#include <asm/bitops.h>

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>

#ifdef MODULE
int init_module(void);
void cleanup_module(void);
#endif

#ifndef Z8530_MAJOR
#define Z8530_MAJOR 34
#endif

#ifndef PROC_NET_Z8530
#define PROC_NET_Z8530 PROC_NET_LAST+10		/* sorry... */
#endif

#if !defined(CONFIG_SCC_DEV) && !defined(CONFIG_SCC_TTY)
#define CONFIG_SCC_DEV
#define CONFIG_SCC_TTY
#endif

int scc_init(void);

static struct mbuf * scc_enqueue_buffer(struct mbuf **queue, struct mbuf * buffer);
static struct mbuf * scc_dequeue_buffer(struct mbuf **queue);
static void alloc_buffer_pool(struct scc_channel *scc);
static void free_buffer_pool(struct scc_channel *scc);
static struct mbuf * scc_get_buffer(struct scc_channel *scc, char type);

#ifdef CONFIG_SCC_TTY
int scc_open(struct tty_struct *tty, struct file *filp);
static void scc_close(struct tty_struct *tty, struct file *filp);
int scc_write(struct tty_struct *tty, int from_user, const unsigned char *buf, int count);
static void scc_put_char(struct tty_struct *tty, unsigned char ch);
static void scc_flush_chars(struct tty_struct *tty);
static int scc_write_room(struct tty_struct *tty);
static int scc_chars_in_buffer(struct tty_struct *tty);
static void scc_flush_buffer(struct tty_struct *tty);
static int scc_tty_ioctl(struct tty_struct *tty, struct file *file, unsigned int cmd, unsigned long arg);
static void scc_set_termios(struct tty_struct *tty, struct termios *old_termios);
static void scc_set_ldisc(struct tty_struct *tty);
static void scc_throttle(struct tty_struct *tty);
static void scc_unthrottle(struct tty_struct *tty);
static void scc_start(struct tty_struct *tty);
static void scc_stop(struct tty_struct *tty);
static void kiss_encode(struct scc_channel *scc);
static void scc_rx_timer(unsigned long);
static void scc_change_speed(struct scc_channel *scc);
#endif

static void t_dwait(unsigned long);
static void t_txdelay(unsigned long);
static void t_tail(unsigned long);
static void t_busy(unsigned long);
static void t_maxkeyup(unsigned long);
static void t_idle(unsigned long);
static void scc_tx_done(struct scc_channel *);
static void scc_start_tx_timer(struct scc_channel *, void (*)(unsigned long), unsigned long);
static void scc_start_maxkeyup(struct scc_channel *);
static void scc_start_defer(struct scc_channel *);

static int scc_ioctl(struct scc_channel *scc, unsigned int cmd, void * arg);

static void z8530_init(void);

static void init_channel(struct scc_channel *scc);
static void scc_key_trx (struct scc_channel *scc, char tx);
static void scc_isr(int irq, void *dev_id, struct pt_regs *regs);
static void scc_init_timer(struct scc_channel *scc);

#ifdef CONFIG_SCC_DEV
static int scc_net_setup(struct scc_channel *scc, unsigned char *name);
static void scc_net_rx(struct scc_channel *scc, struct mbuf *bp);
#endif

static unsigned char *SCC_DriverName = "scc";

#ifdef CONFIG_SCC_TTY
static int baud_table[] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
	9600, 19200, 38400, 57600, 115200, 0 };

struct semaphore scc_sem = MUTEX;
static struct termios scc_std_termios;
unsigned char scc_wbuf[BUFSIZE];

struct tty_driver scc_driver;
static struct tty_struct *scc_table[2*MAXSCC];
static struct termios scc_termios[2 * MAXSCC];
static struct termios scc_termios_locked[2 * MAXSCC];
static int scc_refcount;
#endif


static struct irqflags { unsigned char used : 1; } Ivec[16];
	
static struct scc_channel SCC_Info[2 * MAXSCC];	/* information per channel */

static struct scc_ctrl {
	io_port chan_A;
	io_port chan_B;
	int irq;
} SCC_ctrl[MAXSCC+1];

static unsigned char Driver_Initialized = 0;
static int Nchips = 0;
static io_port Vector_Latch = 0;

#define SCC_DEVNAME (scc->stat.is_netdev? scc->dev->name:kdevname(scc->tty->device))

/* ******************************************************************** */
/* *			Port Access Functions			      * */
/* ******************************************************************** */

/* These provide interrupt save 2-step access to the Z8530 registers */

static __inline__ unsigned char
InReg(io_port port, unsigned char reg)
{
	unsigned long flags;
	unsigned char r;
	
	save_flags(flags);
	cli();
#ifdef SCC_LDELAY
	Outb(port, reg);
	udelay(SCC_LDELAY);
	r=Inb(port);
	udelay(SCC_LDELAY);
#else
	Outb(port, reg);
	r=Inb(port);
#endif
	restore_flags(flags);
	return r;
}

static __inline__ void
OutReg(io_port port, unsigned char reg, unsigned char val)
{
	unsigned long flags;
	
	save_flags(flags);
	cli();
#ifdef SCC_LDELAY
	Outb(port, reg); udelay(SCC_LDELAY);
	Outb(port, val); udelay(SCC_LDELAY);
#else
	Outb(port, reg);
	Outb(port, val);
#endif
	restore_flags(flags);
}

static __inline__ void
wr(struct scc_channel *scc, unsigned char reg, unsigned char val)
{
	OutReg(scc->ctrl, reg, (scc->wreg[reg] = val));
}

static __inline__ void
or(struct scc_channel *scc, unsigned char reg, unsigned char val)
{
	OutReg(scc->ctrl, reg, (scc->wreg[reg] |= val));
}

static __inline__ void
cl(struct scc_channel *scc, unsigned char reg, unsigned char val)
{
	OutReg(scc->ctrl, reg, (scc->wreg[reg] &= ~val));
}

#ifdef DISABLE_ALL_INTS
static __inline__ void scc_cli(int irq)
{ cli(); }
static __inline__ void scc_sti(int irq)
{ sti(); }
#else
static __inline__ void scc_cli(int irq)
{ disable_irq(irq); }
static __inline__ void scc_sti(int irq)
{ enable_irq(irq); }
#endif


/* ******************************************************************** */
/* * 			Memory Buffer Management			*/
/* ******************************************************************** */

/*
 * The new buffer scheme uses a ring chain of buffers. This has the
 * advantage to access both, first and last element of the list, very
 * fast. It has the disadvantage to mess things up double if something
 * is wrong.
 *
 * We could have used the socket buffer routines from skbuff.h instead,
 * but our own routines keep the driver portable...
 *
 * Every scc channel has its own buffer pool now with an adjustable
 * number of buffers for transmit and receive buffers. The buffer
 * size remains the same for each AX.25 frame, but is (as a semi-
 * undocumented feature) adjustable within a range of 512 and 4096
 * to benefit experiments with higher frame lengths. If you need
 * larger frames... Well, you'd better choose a whole new concept
 * for that, like a DMA capable I/O card and a block device driver...
 *
 */

/* ------ Management of buffer queues ------ */


/* Insert a buffer at the "end" of the chain */

static struct mbuf * 
scc_enqueue_buffer(struct mbuf **queue, struct mbuf * buffer)
{
	unsigned long flags;
	struct mbuf * anchor;
	
	save_flags(flags); cli();		/* do not disturb! */

	anchor = *queue;			/* the anchor is the "start" of the chain */

	if (anchor == NULLBUF)			/* found an empty list */
	{
		*queue = buffer;		/* new anchor */
		buffer->next = buffer->prev = NULLBUF;
	} else	
	if (anchor->prev == NULLBUF)		/* list has one member only */
	{
		anchor->next = anchor->prev = buffer;
		buffer->next = buffer->prev = anchor;
	} else {				/* every other case */
		buffer->prev = anchor->prev;	/* self explaining, isn't it? */
		buffer->next = anchor;
		anchor->prev->next = buffer;
		anchor->prev = buffer;
	}
	
	restore_flags(flags);
	return *queue;				/* return "start" of chain */
}



/* Remove a buffer from the "start" of the chain an return it */

static struct mbuf * 
scc_dequeue_buffer(struct mbuf **queue)
{
	unsigned long flags;
	struct mbuf *buffer;
	
	save_flags(flags); cli();

	buffer = *queue;			/* head of the chain */
	
	if (buffer != NULLBUF)			/* not an empty list? */
	{
		if (buffer->prev != NULLBUF)	/* not last buffer? */
		{
			if (buffer->prev->next == buffer->prev->prev)
			{			/* only one buffer remaining... */
				buffer->next->prev = NULLBUF;
				buffer->next->next = NULLBUF;
			} else {		/* the one remaining situation... */
				buffer->next->prev = buffer->prev;
				buffer->prev->next = buffer->next;
			}
		} 

		*queue = buffer->next;		/* new head of chain */
		
		buffer->next = NULLBUF;		/* for security only... */
		buffer->prev = NULLBUF;
		buffer->rw_ptr = buffer->data;
	} 
	
	restore_flags(flags);
	return buffer;				/* give it away... */
}

/* ------ buffer pool management ------ */


/* allocate buffer pool	for a channel */

static void 
alloc_buffer_pool(struct scc_channel *scc)
{
	int k;
	struct mbuf * bptr;
	int  buflen;
	
	if (scc->mempool)
	{
		printk(KERN_DEBUG "z8530drv: alloc_buffer_pool() had buffers already allocated\n");
		return;
	}
	
	buflen = sizeof(struct mbuf) + scc->stat.bufsize;
	
	if (scc->stat.bufsize <  336) scc->stat.bufsize = 336;
	if (buflen > 4096)
	{
		scc->stat.bufsize = 4096-sizeof(struct mbuf);
		buflen = 4096;
	}
	
	if (scc->stat.rxbuffers < 4)  scc->stat.rxbuffers = 4;
	if (scc->stat.txbuffers < 4)  scc->stat.txbuffers = 4;

	
	/* allocate receive buffers for this channel */
	
	for (k = 0; k < scc->stat.rxbuffers ; k++)
	{
		/* allocate memory for the struct and the buffer */
		
		bptr = (struct mbuf *) kmalloc(buflen, GFP_ATOMIC);
		
		/* should not happen, but who knows? */
		
		if (bptr == NULLBUF)
		{
			printk(KERN_WARNING "z8530drv: %s: can't allocate memory for rx buffer pool", SCC_DEVNAME);
			break;
		}
		
		/* clear memory */
		
		memset(bptr, 0, buflen);
		
		/* initialize structure */
		
		bptr->rw_ptr = bptr->data;
		
		/* and append the buffer to the pool */
		
		scc_enqueue_buffer(&scc->rx_buffer_pool, bptr);
	}
	
	/* now do the same for the transmit buffers */
	
	for (k = 0; k < scc->stat.txbuffers ; k++)
	{
		bptr = (struct mbuf *) kmalloc(buflen, GFP_ATOMIC);
		
		if (bptr == NULLBUF)
		{
			printk(KERN_WARNING "z8530drv: %s: can't allocate memory for tx buffer pool", SCC_DEVNAME);
			break;
		}
		
		memset(bptr, 0, buflen);
		
		bptr->rw_ptr = bptr->data;
		
		scc_enqueue_buffer(&scc->tx_buffer_pool, bptr);
	}
	
	scc->mempool = 1;
}


/* remove buffer pool */

static void 
free_buffer_pool(struct scc_channel *scc)
{
	unsigned long flags;
	int rx_cnt, tx_cnt;
	
	if (!scc->mempool)
		return;
	
	/* this one is a bit tricky and probably dangerous. */
	rx_cnt = tx_cnt = 0;
		
	save_flags(flags); cli();
	
	/* esp. to free the buffers currently in use by ISR */
	
	if (scc->rx_bp != NULLBUF)
	{
		kfree(scc->rx_bp);
		scc->rx_bp = NULLBUF;
		rx_cnt++;
	}

	if (scc->tx_bp != NULLBUF)
	{
		kfree(scc->tx_bp);
		scc->tx_bp = NULLBUF;
		tx_cnt++;
	}
	
	if (scc->kiss_decode_bp != NULLBUF)
	{
		kfree(scc->kiss_decode_bp);
		scc->kiss_decode_bp = NULLBUF;
		tx_cnt++;
	}

#ifdef CONFIG_SCC_TTY
	if (scc->kiss_encode_bp != NULLBUF)
	{
		kfree(scc->kiss_encode_bp);
		scc->kiss_encode_bp = NULLBUF;
		rx_cnt++;
	}
#endif
	
	restore_flags(flags);
	
	
	while (scc->rx_queue != NULLBUF)
	{
		kfree(scc_dequeue_buffer(&scc->rx_queue));
		rx_cnt++;
	}
	scc->stat.rx_queued = 0;
	
	while (scc->tx_queue != NULLBUF)
	{
		kfree(scc_dequeue_buffer(&scc->tx_queue));
		tx_cnt++;
	}
	scc->stat.tx_queued = 0;
	
	while (scc->rx_buffer_pool != NULLBUF)
	{
		kfree(scc_dequeue_buffer(&scc->rx_buffer_pool));
		rx_cnt++;
	}
	
	while (scc->tx_buffer_pool != NULLBUF)
	{
		kfree(scc_dequeue_buffer(&scc->tx_buffer_pool));
		tx_cnt++;
	}
	
	if (rx_cnt < scc->stat.rxbuffers)	/* hmm... hmm... :-( */
		printk(KERN_ERR "z8530drv: oops, deallocated only %d of %d rx buffers\n", rx_cnt, scc->stat.rxbuffers);
	if (rx_cnt > scc->stat.rxbuffers)	/* WHAT?!! */
		printk(KERN_ERR "z8530drv: oops, deallocated %d instead of %d rx buffers. Very strange.\n", rx_cnt, scc->stat.rxbuffers);
		
	if (tx_cnt < scc->stat.txbuffers)
		printk(KERN_ERR "z8530drv: oops, deallocated only %d of %d tx buffers\n", tx_cnt, scc->stat.txbuffers);
	if (tx_cnt > scc->stat.txbuffers)
		printk(KERN_ERR "z8530drv: oops, deallocated %d instead of %d tx buffers. Very strange.\n", tx_cnt, scc->stat.txbuffers);
		
	scc->mempool = 0;
}
		
		
/* ------ rx/tx buffer management ------ */

/* 
   get a fresh buffer from the pool if possible; if not: get one from
   the queue. We will remove the oldest frame from the queue and hope
   it was a good idea... ;-)
   
 */

static struct mbuf * 
scc_get_buffer(struct scc_channel *scc, char type)
{
	struct mbuf * bptr;
	
	if (type == BT_TRANSMIT)
	{
		bptr = scc_dequeue_buffer(&scc->tx_buffer_pool);
		
		/* no free buffers in the pool anymore? */
		
		if (bptr == NULLBUF)
		{
#if 0
			printk(KERN_DEBUG "z8530drv: tx buffer pool for %s empty\n", SCC_DEVNAME);
#endif
			/* use the oldest from the queue instead */
			
			bptr = scc_dequeue_buffer(&scc->tx_queue);
			scc->stat.tx_queued--;
			scc->stat.nospace++;
			
			/* this should never, ever happen... */
			
			if (bptr == NULLBUF)
				printk(KERN_ERR "z8530drv: panic - all tx buffers for %s gone\n", SCC_DEVNAME);
		}
	} else {
		bptr = scc_dequeue_buffer(&scc->rx_buffer_pool);
		
		if (bptr == NULLBUF)
		{
			printk(KERN_DEBUG "z8530drv: rx buffer pool for %s empty\n", SCC_DEVNAME);
			
			bptr = scc_dequeue_buffer(&scc->rx_queue);
			scc->stat.rx_queued--;
			scc->stat.nospace++;
			
			if (bptr == NULLBUF)
				printk(KERN_ERR "z8530drv: panic - all rx buffers for %s gone\n", SCC_DEVNAME);
		}	
	}
	
	if (bptr != NULLBUF)
	{
		bptr->rw_ptr = bptr->data;
		bptr->cnt = 0;
	}

	return bptr;
}


/* ******************************************************************** */
/* *			Interrupt Service Routines		      * */
/* ******************************************************************** */



/* ----> subroutines for the interrupt handlers <---- */


#ifdef CONFIG_SCC_TTY
/* kick rx_timer (try to send received frame or a status message ASAP) */

/* of course we could define a "bottom half" routine to do the job,
   but since its structures are saved in an array instead of a linked
   list we would get in trouble if it clashes with another driver.
   IMHO we are fast enough with a timer routine called on the next 
   timer-INT... Your opinions?
 */

static __inline__ void
kick_rx_timer(struct scc_channel *scc)
{
	del_timer(&(scc->rx_t));
	scc->rx_t.expires = jiffies + 1;
	scc->rx_t.function = scc_rx_timer;
	scc->rx_t.data = (unsigned long) scc;
	add_timer(&scc->rx_t);
}
#endif

static __inline__ void
scc_notify(struct scc_channel *scc, int event)
{
	struct mbuf *bp;
	
	if (scc->kiss.fulldup != KISS_DUPLEX_OPTIMA)
		return;
	
	bp = scc_get_buffer(scc, BT_RECEIVE);

	if (bp == NULLBUF)
		return;
		
	bp->data[0] = PARAM_HWEVENT;
	bp->data[1] = event;
	bp->cnt += 2;

	if (scc->stat.is_netdev)
	{
#ifdef CONFIG_SCC_DEV
		scc_net_rx(scc, bp);
		scc_enqueue_buffer(&scc->rx_buffer_pool, bp);
#endif
	} else {
#ifdef CONFIG_SCC_TTY
		scc_enqueue_buffer(&scc->rx_queue, bp);
		scc->stat.rx_queued++;
		kick_rx_timer(scc);
#endif
	}
}

static __inline__ void
flush_rx_FIFO(struct scc_channel *scc)
{
	int k;
	
	for (k=0; k<3; k++)
		Inb(scc->data);
		
	if(scc->rx_bp != NULLBUF)	/* did we receive something? */
	{
		scc->stat.rxerrs++;  /* then count it as an error */
		scc_enqueue_buffer(&scc->rx_buffer_pool, scc->rx_bp);
		
		scc->rx_bp = NULLBUF;
	}
}


/* ----> four different interrupt handlers for Tx, Rx, changing of	*/
/*       DCD/CTS and Rx/Tx errors					*/

/* Transmitter interrupt handler */
static __inline__ void
scc_txint(struct scc_channel *scc)
{
	struct mbuf *bp;

	scc->stat.txints++;

	bp = scc->tx_bp;
	
	/* send first octet */
	
	if (bp == NULLBUF)
	{
		do
		{
			if (bp != NULLBUF)
			{
				scc_enqueue_buffer(&scc->tx_buffer_pool, bp);
				scc->stat.tx_queued--;
			}

			bp = scc_dequeue_buffer(&scc->tx_queue);
			
			if (bp == NULLBUF)
			{
				scc_tx_done(scc);
				Outb(scc->ctrl, RES_Tx_P);	/* clear int */
				return;
			}
			
			if (bp->cnt > 0)
			{
				bp->rw_ptr++;
				bp->cnt--;
			}
			
		} while (bp->cnt < 1);
		
		scc->tx_bp = bp;
		scc->stat.tx_state = TXS_ACTIVE;

		OutReg(scc->ctrl, R0, RES_Tx_CRC);
						/* reset CRC generator */
		or(scc,R10,ABUNDER);		/* re-install underrun protection */
		Outb(scc->data,*bp->rw_ptr++);	/* send byte */

		if (!scc->enhanced)		/* reset EOM latch */
			Outb(scc->ctrl,RES_EOM_L);

		bp->cnt--;
		return;
	}
	
	/* End Of Frame... */
	
	if (bp->cnt <= 0)
	{
		Outb(scc->ctrl, RES_Tx_P);	/* reset pending int */
		cl(scc, R10, ABUNDER);		/* send CRC */
		scc_enqueue_buffer(&scc->tx_buffer_pool, bp);
		scc->stat.tx_queued--;
		scc->tx_bp = NULLBUF;
		scc->stat.tx_state = TXS_NEWFRAME; /* next frame... */
		return;
	} 
	
	/* send octet */
	
	Outb(scc->data,*bp->rw_ptr);		
	bp->rw_ptr++;			/* increment pointer */
	bp->cnt--;                      /* decrease byte count */
}


/* External/Status interrupt handler */
static __inline__ void
scc_exint(struct scc_channel *scc)
{
	unsigned char status,changes,chg_and_stat;

	scc->stat.exints++;

	status = InReg(scc->ctrl,R0);
	changes = status ^ scc->status;
	chg_and_stat = changes & status;
	
	/* ABORT: generated whenever DCD drops while receiving */

	if (chg_and_stat & BRK_ABRT)		/* Received an ABORT */
		flush_rx_FIFO(scc);


	/* DCD: on = start to receive packet, off = ABORT condition */
	/* (a successfully received packet generates a special condition int) */
	
	if(changes & DCD)                       /* DCD input changed state */
	{
		if(status & DCD)                /* DCD is now ON */
		{
			if (scc->modem.clocksrc != CLK_EXTERNAL)
				OutReg(scc->ctrl,R14,SEARCH|scc->wreg[R14]); /* DPLL: enter search mode */
				
			or(scc,R3,ENT_HM|RxENABLE); /* enable the receiver, hunt mode */
		} else {                        /* DCD is now OFF */
			cl(scc,R3,ENT_HM|RxENABLE); /* disable the receiver */
			flush_rx_FIFO(scc);
		}
		
		if (!scc->kiss.softdcd)
			scc_notify(scc, (status & DCD)? HWEV_DCD_ON:HWEV_DCD_OFF);
	}
	
	/* HUNT: software DCD; on = waiting for SYNC, off = receiving frame */
	
	if (changes & SYNC_HUNT)
	{
		if (scc->kiss.softdcd)
			scc_notify(scc, (status & SYNC_HUNT)? HWEV_DCD_OFF:HWEV_DCD_ON);
		else
			cl(scc,R15,SYNCIE);	/* oops, we were too lazy to disable this? */
	}

#ifdef notdef
	/* CTS: use external TxDelay (what's that good for?!)
	 * Anyway: If we _could_ use it (BayCom USCC uses CTS for
	 * own purposes) we _should_ use the "autoenable" feature
	 * of the Z8530 and not this interrupt...
	 */
	 
	if (chg_and_stat & CTS)			/* CTS is now ON */
	{
		if (scc->kiss.txdelay == 0)	/* zero TXDELAY = wait for CTS */
			scc_start_tx_timer(scc, t_txdelay, 0);
	}
#endif
	
	if (scc->stat.tx_state == TXS_ACTIVE && (status & TxEOM))
	{
		scc->stat.tx_under++;	  /* oops, an underrun! count 'em */
		Outb(scc->ctrl, RES_EXT_INT);	/* reset ext/status interrupts */

		if (scc->tx_bp != NULLBUF)
		{
			scc_enqueue_buffer(&scc->tx_buffer_pool, scc->tx_bp);
			scc->stat.tx_queued--;
			scc->tx_bp = NULLBUF;
		}
		
		or(scc,R10,ABUNDER);
		scc_start_tx_timer(scc, t_txdelay, 1);	/* restart transmission */
	}
		
	scc->status = status;
	Outb(scc->ctrl,RES_EXT_INT);
}


/* Receiver interrupt handler */
static __inline__ void
scc_rxint(struct scc_channel *scc)
{
	struct mbuf *bp;

	scc->stat.rxints++;

	if((scc->wreg[5] & RTS) && scc->kiss.fulldup == KISS_DUPLEX_HALF)
	{
		Inb(scc->data);		/* discard char */
		or(scc,R3,ENT_HM);	/* enter hunt mode for next flag */
		return;
	}

	bp = scc->rx_bp;
	
	if (bp == NULLBUF)
	{
		bp = scc_get_buffer(scc, BT_RECEIVE);
		if (bp == NULLBUF)
		{
			Inb(scc->data);
			or(scc, R3, ENT_HM);
			return;
		}
		
		scc->rx_bp = bp;
		
		*bp->rw_ptr=0;		/* KISS data */
		bp->rw_ptr++;
		bp->cnt++;
	}
	
	if (bp->cnt > scc->stat.bufsize)
	{
#ifdef notdef
		printk(KERN_DEBUG "z8530drv: oops, scc_rxint() received huge frame...\n");
#endif
		scc_enqueue_buffer(&scc->rx_buffer_pool, bp);
		scc->rx_bp = NULLBUF;
		Inb(scc->data);
		or(scc, R3, ENT_HM);
		return;
	}
		
	/* now, we have a buffer. read character and store it */
	*bp->rw_ptr = Inb(scc->data);
	bp->rw_ptr++;
	bp->cnt++;
}


/* Receive Special Condition interrupt handler */
static __inline__ void
scc_spint(struct scc_channel *scc)
{
	unsigned char status;
	struct mbuf *bp;

	scc->stat.spints++;

	status = InReg(scc->ctrl,R1);		/* read receiver status */
	
	Inb(scc->data);				/* throw away Rx byte */
	bp = scc->rx_bp;

	if(status & Rx_OVR)			/* receiver overrun */
	{
		scc->stat.rx_over++;             /* count them */
		or(scc,R3,ENT_HM);               /* enter hunt mode for next flag */
		
		if (bp) scc_enqueue_buffer(&scc->rx_buffer_pool, bp);
		scc->rx_bp = NULLBUF;
	}

	if(status & END_FR && bp != NULLBUF)	/* end of frame */
	{
		/* CRC okay, frame ends on 8 bit boundary and received something ? */
		
		if (!(status & CRC_ERR) && (status & 0xe) == RES8 && bp->cnt)
		{
			/* ignore last received byte (first of the CRC bytes) */
			bp->cnt--;

			if (scc->stat.is_netdev)
			{
#ifdef CONFIG_SCC_DEV
				scc_net_rx(scc, bp);
				scc_enqueue_buffer(&scc->rx_buffer_pool, bp);
#endif
			} else {
#ifdef CONFIG_SCC_TTY
				scc_enqueue_buffer(&scc->rx_queue, bp);
				scc->stat.rx_queued++;
				kick_rx_timer(scc);
#endif
			}

			scc->rx_bp = NULLBUF;
			scc->stat.rxframes++;
		} else {				/* a bad frame */
			scc_enqueue_buffer(&scc->rx_buffer_pool, bp);
			scc->rx_bp = NULLBUF;
			scc->stat.rxerrs++;
		}
	} 

	Outb(scc->ctrl,ERR_RES);
}


/* ----> interrupt service routine for the Z8530 <---- */

static void
scc_isr_dispatch(struct scc_channel *scc, int vector)
{
	switch (vector & VECTOR_MASK)
	{
		case TXINT: scc_txint(scc); break;
		case EXINT: scc_exint(scc); break;
		case RXINT: scc_rxint(scc); break;
		case SPINT: scc_spint(scc); break;
	}
}

/* If the card has a latch for the interrupt vector (like the PA0HZP card)
   use it to get the number of the chip that generated the int.
   If not: poll all defined chips.
 */

#define SCC_IRQTIMEOUT 30000

static void
scc_isr(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned char vector;	
	struct scc_channel *scc;
	struct scc_ctrl *ctrl;
	int k;
	
	scc_cli(irq);

	if (Vector_Latch)
	{
	    	for(k=0; k < SCC_IRQTIMEOUT; k++)
    		{
			Outb(Vector_Latch, 0);      /* Generate INTACK */
        
			/* Read the vector */
			if((vector=Inb(Vector_Latch)) >= 16 * Nchips) break; 
			if (vector & 0x01) break;
        	 
		        scc=&SCC_Info[vector >> 3 ^ 0x01];
			if (!scc->tty && !scc->dev) break;

			scc_isr_dispatch(scc, vector);

			OutReg(scc->ctrl,R0,RES_H_IUS);              /* Reset Highest IUS */
		}  
		scc_sti(irq);

		if (k == SCC_IRQTIMEOUT)
			printk(KERN_WARNING "z8530drv: endless loop in scc_isr()?\n");

		return;
	}

	/* Find the SCC generating the interrupt by polling all attached SCCs
	 * reading RR3A (the interrupt pending register)
	 */

	ctrl = SCC_ctrl;
	while (ctrl->chan_A)
	{
		if (ctrl->irq != irq)
		{
			ctrl++;
			continue;
		}

		scc = NULL;
		for (k = 0; InReg(ctrl->chan_A,R3) && k < SCC_IRQTIMEOUT; k++)
		{
			vector=InReg(ctrl->chan_B,R2);	/* Read the vector */
			if (vector & 0x01) break; 

			scc = &SCC_Info[vector >> 3 ^ 0x01];
		        if (!scc->tty && !scc->dev) break;

			scc_isr_dispatch(scc, vector);
		}

		if (k == SCC_IRQTIMEOUT)
		{
			printk(KERN_WARNING "z8530drv: endless loop in scc_isr()?!\n");
			break;
		}

		/* This looks wierd and it is. At least the BayCom USCC doesn't
		 * use the Interrupt Daisy Chain, thus we'll have to start
		 * all over again to be sure not to miss an interrupt from 
		 * (any of) the other chip(s)...
		 * Honestly, the situation *is* braindamaged...
		 */

		if (scc != NULL)
		{
			OutReg(scc->ctrl,R0,RES_H_IUS);
			ctrl = SCC_ctrl; 
		} else
			ctrl++;
	}
	
	scc_sti(irq);
}



/* ******************************************************************** */
/* *			Init Channel					*/
/* ******************************************************************** */


/* ----> set SCC channel speed <---- */

static __inline__ void 
set_brg(struct scc_channel *scc, unsigned int tc)
{
	cl(scc,R14,BRENABL);		/* disable baudrate generator */
	wr(scc,R12,tc & 255);		/* brg rate LOW */
	wr(scc,R13,tc >> 8);   		/* brg rate HIGH */
	or(scc,R14,BRENABL);		/* enable baudrate generator */
}

static __inline__ void 
set_speed(struct scc_channel *scc)
{
	disable_irq(scc->irq);

	if (scc->modem.speed > 0)	/* paranoia... */
		set_brg(scc, (unsigned) (scc->clock / (scc->modem.speed * 64)) - 2);

	enable_irq(scc->irq);
}


/* ----> initialize a SCC channel <---- */

static __inline__ void 
init_brg(struct scc_channel *scc)
{
	wr(scc, R14, BRSRC);				/* BRG source = PCLK */
	OutReg(scc->ctrl, R14, SSBR|scc->wreg[R14]);	/* DPLL source = BRG */
	OutReg(scc->ctrl, R14, SNRZI|scc->wreg[R14]);	/* DPLL NRZI mode */
}

/*
 * Initialization according to the Z8530 manual (SGS-Thomson's version):
 *
 * 1. Modes and constants
 *
 * WR9	11000000	chip reset
 * WR4	XXXXXXXX	Tx/Rx control, async or sync mode
 * WR1	0XX00X00	select W/REQ (optional)
 * WR2	XXXXXXXX	program interrupt vector
 * WR3	XXXXXXX0	select Rx control
 * WR5	XXXX0XXX	select Tx control
 * WR6	XXXXXXXX	sync character
 * WR7	XXXXXXXX	sync character
 * WR9	000X0XXX	select interrupt control
 * WR10	XXXXXXXX	miscellaneous control (optional)
 * WR11	XXXXXXXX	clock control
 * WR12	XXXXXXXX	time constant lower byte (optional)
 * WR13	XXXXXXXX	time constant upper byte (optional)
 * WR14	XXXXXXX0	miscellaneous control
 * WR14	XXXSSSSS	commands (optional)
 *
 * 2. Enables
 *
 * WR14	000SSSS1	baud rate enable
 * WR3	SSSSSSS1	Rx enable
 * WR5	SSSS1SSS	Tx enable
 * WR0	10000000	reset Tx CRG (optional)
 * WR1	XSS00S00	DMA enable (optional)
 *
 * 3. Interrupt status
 *
 * WR15	XXXXXXXX	enable external/status
 * WR0	00010000	reset external status
 * WR0	00010000	reset external status twice
 * WR1	SSSXXSXX	enable Rx, Tx and Ext/status
 * WR9	000SXSSS	enable master interrupt enable
 *
 * 1 = set to one, 0 = reset to zero
 * X = user defined, S = same as previous init
 *
 *
 * Note that the implementation differs in some points from above scheme.
 *
 */
 
static void
init_channel(struct scc_channel *scc)
{
	disable_irq(scc->irq);
	
#ifdef CONFIG_SCC_TTY
	del_timer(&scc->rx_t);
#endif
	del_timer(&scc->tx_t);
	del_timer(&scc->tx_wdog);



	wr(scc,R4,X1CLK|SDLC);		/* *1 clock, SDLC mode */
	wr(scc,R1,0);			/* no W/REQ operation */
	wr(scc,R3,Rx8|RxCRC_ENAB);	/* RX 8 bits/char, CRC, disabled */	
	wr(scc,R5,Tx8|DTR|TxCRC_ENAB);	/* TX 8 bits/char, disabled, DTR */
	wr(scc,R6,0);			/* SDLC address zero (not used) */
	wr(scc,R7,FLAG);		/* SDLC flag value */
	wr(scc,R9,VIS);			/* vector includes status */
	wr(scc,R10,(scc->modem.nrz? NRZ : NRZI)|CRCPS|ABUNDER); /* abort on underrun, preset CRC generator, NRZ(I) */
	wr(scc,R14, 0);


/* set clock sources:

   CLK_DPLL: normal halfduplex operation
   
		RxClk: use DPLL
		TxClk: use DPLL
		TRxC mode DPLL output
		
   CLK_EXTERNAL: external clocking (G3RUH or DF9IC modem)
   
  	        BayCom: 		others:
  	        
  	        TxClk = pin RTxC	TxClk = pin TRxC
  	        RxClk = pin TRxC 	RxClk = pin RTxC
  	     

   CLK_DIVIDER:
   		RxClk = use DPLL
   		TxClk = pin RTxC
   		
   		BayCom:			others:
   		pin TRxC = DPLL		pin TRxC = BRG
   		(RxClk * 1)		(RxClk * 32)
*/  

   		
	switch(scc->modem.clocksrc)
	{
		case CLK_DPLL:
			wr(scc, R11, RCDPLL|TCDPLL|TRxCOI|TRxCDP);
			init_brg(scc);
			break;

		case CLK_DIVIDER:
			wr(scc, R11, ((scc->brand & BAYCOM)? TRxCDP : TRxCBR) | RCDPLL|TCRTxCP|TRxCOI);
			init_brg(scc);
			break;

		case CLK_EXTERNAL:
			wr(scc, R11, (scc->brand & BAYCOM)? RCTRxCP|TCRTxCP : RCRTxCP|TCTRxCP);
			OutReg(scc->ctrl, R14, DISDPLL);
			break;

	}
	
	set_speed(scc);			/* set baudrate */
	
	if(scc->enhanced)
	{
		or(scc,R15,SHDLCE|FIFOE);	/* enable FIFO, SDLC/HDLC Enhancements (From now R7 is R7') */
		wr(scc,R7,AUTOEOM);
	}

	if((InReg(scc->ctrl,R0)) & DCD)		/* DCD is now ON */
	{
		if (scc->modem.clocksrc != CLK_EXTERNAL)
			or(scc,R14, SEARCH);
			
		or(scc,R3,ENT_HM|RxENABLE);	/* enable the receiver, hunt mode */
	}
	
	/* enable ABORT, DCD & SYNC/HUNT interrupts */

	wr(scc,R15, BRKIE|TxUIE|DCDIE);
	if (scc->kiss.softdcd)
		or(scc,R15, SYNCIE);

	Outb(scc->ctrl,RES_EXT_INT);	/* reset ext/status interrupts */
	Outb(scc->ctrl,RES_EXT_INT);	/* must be done twice */

	or(scc,R1,INT_ALL_Rx|TxINT_ENAB|EXT_INT_ENAB); /* enable interrupts */
	
	scc->status = InReg(scc->ctrl,R0);	/* read initial status */
	
	or(scc,R9,MIE);			/* master interrupt enable */
	
	scc_init_timer(scc);
			
	enable_irq(scc->irq);
}




/* ******************************************************************** */
/* *			SCC timer functions			      * */
/* ******************************************************************** */


/* ----> scc_key_trx sets the time constant for the baudrate 
         generator and keys the transmitter		     <---- */

static void
scc_key_trx(struct scc_channel *scc, char tx)
{
	unsigned int time_const;
		
	if (scc->brand & PRIMUS)
		Outb(scc->ctrl + 4, scc->option | (tx? 0x80 : 0));

	if (scc->modem.speed < 300) 
		scc->modem.speed = 1200;

	time_const = (unsigned) (scc->clock / (scc->modem.speed * (tx? 2:64))) - 2;

	disable_irq(scc->irq);

	if (tx)
	{
		or(scc, R1, TxINT_ENAB);	/* t_maxkeyup may have reset these */
		or(scc, R15, TxUIE);
	}

	if (scc->modem.clocksrc == CLK_DPLL)
	{				/* force simplex operation */
		if (tx)
		{
			cl(scc, R3, RxENABLE|ENT_HM);	/* switch off receiver */
			cl(scc, R15, DCDIE);		/* No DCD changes, please */
			set_brg(scc, time_const);	/* reprogram baudrate generator */

			/* DPLL -> Rx clk, BRG -> Tx CLK, TRxC mode output, TRxC = BRG */
			wr(scc, R11, RCDPLL|TCBR|TRxCOI|TRxCBR);
			
			or(scc,R5,RTS|TxENAB);		/* set the RTS line and enable TX */
		} else {
			cl(scc,R5,RTS|TxENAB);
			
			set_brg(scc, time_const);	/* reprogram baudrate generator */
			
			/* DPLL -> Rx clk, DPLL -> Tx CLK, TRxC mode output, TRxC = DPLL */
			wr(scc, R11, RCDPLL|TCDPLL|TRxCOI|TRxCDP);
			
			or(scc,R3,RxENABLE|ENT_HM);
			or(scc,R15, DCDIE);
		}
	} else {
		if (tx)
		{
			if (scc->kiss.fulldup == KISS_DUPLEX_HALF)
			{
				cl(scc, R3, RxENABLE);
				cl(scc, R15, DCDIE);
			}
				
				
			or(scc,R5,RTS|TxENAB);		/* enable tx */
		} else {
			cl(scc,R5,RTS|TxENAB);		/* disable tx */
			
			if (scc->kiss.fulldup == KISS_DUPLEX_HALF)
			{
				or(scc, R3, RxENABLE|ENT_HM);
				or(scc, R15, DCDIE);
			}
		}
	}

	enable_irq(scc->irq);
}


/* ----> SCC timer interrupt handler and friends. <---- */

static void 
scc_start_tx_timer(struct scc_channel *scc, void (*handler)(unsigned long), unsigned long when)
{
	unsigned long flags;
	
	save_flags(flags);
	cli();

	del_timer(&scc->tx_t);

	if (when == 0)
	{
		handler((unsigned long) scc);
	} else 
	if (when != TIMER_OFF)
	{
		scc->tx_t.data = (unsigned long) scc;
		scc->tx_t.function = handler;
		scc->tx_t.expires = jiffies + (when*HZ)/100;
		add_timer(&scc->tx_t);
	}
	
	restore_flags(flags);	
}

static void
scc_start_defer(struct scc_channel *scc)
{
	unsigned long flags;
	
	save_flags(flags);
	cli();

	del_timer(&scc->tx_wdog);
	
	if (scc->kiss.maxdefer != 0 && scc->kiss.maxdefer != TIMER_OFF)
	{
		scc->tx_wdog.data = (unsigned long) scc;
		scc->tx_wdog.function = t_busy;
		scc->tx_wdog.expires = jiffies + HZ*scc->kiss.maxdefer;
		add_timer(&scc->tx_wdog);
	}
	restore_flags(flags);
}

static void
scc_start_maxkeyup(struct scc_channel *scc)
{
	unsigned long flags;
	
	save_flags(flags);
	cli();

	del_timer(&scc->tx_wdog);
	
	if (scc->kiss.maxkeyup != 0 && scc->kiss.maxkeyup != TIMER_OFF)
	{
		scc->tx_wdog.data = (unsigned long) scc;
		scc->tx_wdog.function = t_maxkeyup;
		scc->tx_wdog.expires = jiffies + HZ*scc->kiss.maxkeyup;
		add_timer(&scc->tx_wdog);
	}
	
	restore_flags(flags);
}

/* 
 * This is called from scc_txint() when there are no more frames to send.
 * Not exactly a timer function, but it is a close friend of the family...
 */

static void
scc_tx_done(struct scc_channel *scc)
{
	/* 
	 * trx remains keyed in fulldup mode 2 until t_idle expires.
	 */
				 
	switch (scc->kiss.fulldup)
	{
		case KISS_DUPLEX_LINK:
			scc->stat.tx_state = TXS_IDLE2;
			if (scc->kiss.idletime != TIMER_OFF)
			scc_start_tx_timer(scc, t_idle, scc->kiss.idletime*100);
			break;
		case KISS_DUPLEX_OPTIMA:
			scc_notify(scc, HWEV_ALL_SENT);
			break;
		default:
			scc->stat.tx_state = TXS_BUSY;
			scc_start_tx_timer(scc, t_tail, scc->kiss.tailtime);
	}

#ifdef CONFIG_SCC_DEV
	if (scc->stat.is_netdev)
 		scc->dev->tbusy = 0;
#endif
}


static unsigned char Rand = 17;

static __inline__ int 
is_grouped(struct scc_channel *scc)
{
	int k;
	struct scc_channel *scc2;
	unsigned char grp1, grp2;

	grp1 = scc->kiss.group;
	
	for (k = 0; k < (Nchips * 2); k++)
	{
		scc2 = &SCC_Info[k];
		grp2 = scc2->kiss.group;
		
		if (scc2 == scc || !(scc2->tty && scc2->dev && grp2))
			continue;
		
		if ((grp1 & 0x3f) == (grp2 & 0x3f))
		{
			if ( (grp1 & TXGROUP) && (scc2->wreg[R5] & RTS) )
				return 1;
			
			if ( (grp1 & RXGROUP) && (scc2->status & DCD) )
				return 1;
		}
	}
	return 0;
}

/* DWAIT and SLOTTIME expired
 *
 * fulldup == 0:  DCD is active or Rand > P-persistence: start t_busy timer
 *                else key trx and start txdelay
 * fulldup == 1:  key trx and start txdelay
 * fulldup == 2:  mintime expired, reset status or key trx and start txdelay
 */

static void 
t_dwait(unsigned long channel)
{
	struct scc_channel *scc = (struct scc_channel *) channel;
	
	if (scc->stat.tx_state == TXS_WAIT)
	{
		if (scc->tx_queue == NULL)
		{
			scc->stat.tx_state = TXS_IDLE;
			return;
		}

		scc->stat.tx_state = TXS_BUSY;
	}

	if (scc->kiss.fulldup == KISS_DUPLEX_HALF)
	{
		Rand = Rand * 17 + 31;
		
		if ( (scc->kiss.softdcd? !(scc->status & SYNC_HUNT):(scc->status & DCD))  || (scc->kiss.persist) < Rand || (scc->kiss.group && is_grouped(scc)) )
		{
			scc_start_defer(scc);
			scc_start_tx_timer(scc, t_dwait, scc->kiss.slottime);
			return ;
		}
	}

	if ( !(scc->wreg[R5] & RTS) )
	{
		scc_key_trx(scc, TX_ON);
		scc_start_tx_timer(scc, t_txdelay, scc->kiss.txdelay);
	} else {
		scc_start_tx_timer(scc, t_txdelay, 0);
	}
}


/* TXDELAY expired
 *
 * kick transmission by a fake scc_txint(scc), start 'maxkeyup' watchdog.
 */

static void 
t_txdelay(unsigned long channel)
{
	struct scc_channel *scc = (struct scc_channel *) channel;

	scc_start_maxkeyup(scc);

	if (scc->tx_bp == NULLBUF)
	{
		disable_irq(scc->irq);
		scc_txint(scc);	
		enable_irq(scc->irq);
	}
}
	

/* TAILTIME expired
 *
 * switch off transmitter. If we were stopped by Maxkeyup restart
 * transmission after 'mintime' seconds
 */

static void 
t_tail(unsigned long channel)
{
	struct scc_channel *scc = (struct scc_channel *) channel;
	
 	del_timer(&scc->tx_wdog);
 	scc_key_trx(scc, TX_OFF);

 	if (scc->stat.tx_state == TXS_TIMEOUT)		/* we had a timeout? */
 	{
 		scc->stat.tx_state = TXS_WAIT;
 
  		if (scc->tx_bp != NULLBUF)
	 		scc_enqueue_buffer(&scc->tx_queue, scc->tx_bp);

 		scc->tx_bp = NULLBUF;

 		if (scc->kiss.mintime != TIMER_OFF)	/* try it again */
 			scc_start_tx_timer(scc, t_dwait, scc->kiss.mintime*100);
 		else
 			scc_start_tx_timer(scc, t_dwait, 0);
 		return;
 	}
 	
 	scc->stat.tx_state = TXS_IDLE;
 
#ifdef CONFIG_SCC_DEV
 	if (scc->stat.is_netdev)
		scc->dev->tbusy = 0;
#endif
}


/* BUSY timeout
 *
 * throw away send buffers if DCD remains active too long.
 */

static void 
t_busy(unsigned long channel)
{
	struct scc_channel *scc = (struct scc_channel *) channel;
	struct mbuf *bp;

	del_timer(&scc->tx_t);

	while ((bp = scc_dequeue_buffer(&scc->tx_queue)) != NULLBUF)
		scc_enqueue_buffer(&scc->tx_buffer_pool, bp);

	scc->stat.tx_queued = 0;
	scc->stat.txerrs++;
		
	scc->tx_queue = NULLBUF;
	scc->stat.tx_state = TXS_IDLE;

#ifdef CONFIG_SCC_DEV
 	if (scc->stat.is_netdev)
 		scc->dev->tbusy = 0;
#endif
}

/* MAXKEYUP timeout
 *
 * this is our watchdog.
 */

static void 
t_maxkeyup(unsigned long channel)
{
	struct scc_channel *scc = (struct scc_channel *) channel;
	unsigned long flags;

	save_flags(flags);
	cli();

	if (scc->stat.is_netdev)
	{
#ifdef CONFIG_SCC_DEV
		/* 
		 * let things settle down before we start to
		 * accept new data.
		 */
		scc->dev->tbusy = 1;
		scc->dev->trans_start = jiffies;
#endif
	}

	del_timer(&scc->tx_t);

	cl(scc, R1, TxINT_ENAB);	/* force an ABORT, but don't */
	cl(scc, R15, TxUIE);		/* count it. */
	OutReg(scc->ctrl, R0, RES_Tx_P);

	restore_flags(flags);

	scc->stat.txerrs++;
	scc->stat.tx_state = TXS_TIMEOUT;
	scc_start_tx_timer(scc, t_tail, scc->kiss.tailtime);
}

/* IDLE timeout
 *
 * in fulldup mode 2 it keys down the transmitter after 'idle' seconds
 * of inactivity. We will not restart transmission before 'mintime'
 * expires.
 */

static void 
t_idle(unsigned long channel)
{
	struct scc_channel *scc = (struct scc_channel *) channel;
	
	del_timer(&scc->tx_wdog);
	scc_key_trx(scc, TX_OFF);

	if (scc->kiss.mintime != TIMER_OFF)
		scc_start_tx_timer(scc, t_dwait, scc->kiss.mintime*100);
	scc->stat.tx_state = TXS_WAIT;
}


#ifdef CONFIG_SCC_TTY
static void
scc_rx_timer(unsigned long channel)
{
	struct scc_channel *scc;
	
	scc = (struct scc_channel *) channel;
	
	if (scc->rx_queue && scc->throttled)
	{
		scc->rx_t.expires = jiffies + HZ/25;	/* 40 msec */
		add_timer(&scc->rx_t);
		return;
	}
	
	kiss_encode(scc);

	if (scc->rx_queue && !scc->throttled)
	{

		printk(KERN_DEBUG "z8530drv: warning, %s should be throttled\n", 
		       SCC_DEVNAME);
		       
		scc->rx_t.expires = jiffies + HZ/25;	/* 40 msec */
		add_timer(&scc->rx_t);
	}
}
#endif

static void
scc_init_timer(struct scc_channel *scc)
{
	unsigned long flags;
	
	save_flags(flags); 
	cli();
	
	scc->stat.tx_state = TXS_IDLE;
	
#ifdef CONFIG_SCC_TTY	
	scc->rx_t.data = (unsigned long) scc;
	scc->rx_t.function = scc_rx_timer;
#endif
	
	restore_flags(flags);
}


/* ******************************************************************** */
/* *			KISS interpreter			      * */
/* ******************************************************************** */


/*
 * this will set the "kiss" parameters through KISS commands or ioctl()
 */

#define CAST(x) (unsigned long)(x)

static unsigned int
kiss_set_param(struct scc_channel *scc, unsigned int cmd, unsigned int arg)
{
	int dcd;

	switch (cmd)
	{
		case PARAM_TXDELAY:	scc->kiss.txdelay=arg;		break;
		case PARAM_PERSIST:	scc->kiss.persist=arg;		break;
		case PARAM_SLOTTIME:	scc->kiss.slottime=arg;		break;
		case PARAM_TXTAIL:	scc->kiss.tailtime=arg;		break;
		case PARAM_FULLDUP:	scc->kiss.fulldup=arg;		break;
		case PARAM_DTR:		break; /* does someone need this? */
		case PARAM_GROUP:	scc->kiss.group=arg;		break;
		case PARAM_IDLE:	scc->kiss.idletime=arg;		break;
		case PARAM_MIN:		scc->kiss.mintime=arg;		break;
		case PARAM_MAXKEY:	scc->kiss.maxkeyup=arg;		break;
		case PARAM_WAIT:	scc->kiss.waittime=arg;		break;
		case PARAM_MAXDEFER:	scc->kiss.maxdefer=arg;		break;
		case PARAM_TX:		scc->kiss.tx_inhibit=arg;	break;

		case PARAM_SOFTDCD:	
			scc->kiss.softdcd=arg;
			if (arg)
				or(scc, R15, SYNCIE);
			else
				cl(scc, R15, SYNCIE);
			break;
				
		case PARAM_SPEED:
			if (arg < 256)
				scc->modem.speed=arg*100;
			else
				scc->modem.speed=arg;

			if (scc->stat.tx_state == 0)	/* only switch baudrate on rx... ;-) */
				set_speed(scc);
			break;
			
		case PARAM_RTS:	
			if ( !(scc->wreg[R5] & RTS) )
			{
				if (arg != TX_OFF)
					scc_key_trx(scc, TX_ON);
					scc_start_tx_timer(scc, t_txdelay, scc->kiss.txdelay);
			} else {
				if (arg == TX_OFF)
				{
					scc->stat.tx_state = TXS_BUSY;
					scc_start_tx_timer(scc, t_tail, scc->kiss.tailtime);
				}
			}
			break;
			
		case PARAM_HWEVENT:
			dcd = (scc->kiss.softdcd? !(scc->status & SYNC_HUNT):(scc->status & DCD));
			scc_notify(scc, dcd? HWEV_DCD_ON:HWEV_DCD_OFF);
			break;

		default:		return -EINVAL;
	}
	
	return 0;
}


 
static unsigned long
kiss_get_param(struct scc_channel *scc, unsigned int cmd)
{
	switch (cmd)
	{
		case PARAM_TXDELAY:	return CAST(scc->kiss.txdelay);
		case PARAM_PERSIST:	return CAST(scc->kiss.persist);
		case PARAM_SLOTTIME:	return CAST(scc->kiss.slottime);
		case PARAM_TXTAIL:	return CAST(scc->kiss.tailtime);
		case PARAM_FULLDUP:	return CAST(scc->kiss.fulldup);
		case PARAM_SOFTDCD:	return CAST(scc->kiss.softdcd);
		case PARAM_DTR:		return CAST((scc->wreg[R5] & DTR)? 1:0);
		case PARAM_RTS:		return CAST((scc->wreg[R5] & RTS)? 1:0);
		case PARAM_SPEED:	return CAST(scc->modem.speed);
		case PARAM_GROUP:	return CAST(scc->kiss.group);
		case PARAM_IDLE:	return CAST(scc->kiss.idletime);
		case PARAM_MIN:		return CAST(scc->kiss.mintime);
		case PARAM_MAXKEY:	return CAST(scc->kiss.maxkeyup);
		case PARAM_WAIT:	return CAST(scc->kiss.waittime);
		case PARAM_MAXDEFER:	return CAST(scc->kiss.maxdefer);
		case PARAM_TX:		return CAST(scc->kiss.tx_inhibit);
		default:		return NO_SUCH_PARAM;
	}

}

#undef CAST
#undef SVAL

/* interpret frame: strip CRC and decode KISS */

static void 
kiss_interpret_frame(struct scc_channel * scc)
{
	unsigned char kisscmd;
	unsigned long flags;
	struct mbuf *bp;

	bp = scc->kiss_decode_bp;
	bp->rw_ptr = bp->data;
	
	if (bp->cnt < 2)
	{
		scc_enqueue_buffer(&scc->tx_buffer_pool, bp);
		scc->kiss_decode_bp = NULLBUF;
		return;
	}
	
	kisscmd = *bp->rw_ptr;
	bp->rw_ptr++;

	if (kisscmd & 0xa0)
	{
		if (bp->cnt > 3) 
			bp->cnt -= 2;
		else
		{
			scc_enqueue_buffer(&scc->tx_buffer_pool, bp);
			scc->kiss_decode_bp = NULLBUF;
			return;
		}
	}
	
	kisscmd &= 0x1f;
			
	if (kisscmd)
	{
		kiss_set_param(scc, kisscmd, *bp->rw_ptr);
		scc_enqueue_buffer(&scc->tx_buffer_pool, bp);
		scc->kiss_decode_bp = NULLBUF;
		return;
	}

	scc_enqueue_buffer(&scc->tx_queue, bp);	/* enqueue frame */
	scc->kiss_decode_bp = NULLBUF;
	
	scc->stat.txframes++;
	scc->stat.tx_queued++;

	save_flags(flags); 
	cli();

	/*
	 * start transmission if the trx state is idle or
	 * t_idle hasn't expired yet. Use dwait/persistance/slottime
	 * algorithm for normal halfduplex operation.
	 */

	if(scc->stat.tx_state == TXS_IDLE || scc->stat.tx_state == TXS_IDLE2)
	{
		scc->stat.tx_state = TXS_BUSY;
		if (scc->kiss.fulldup == KISS_DUPLEX_HALF)
			scc_start_tx_timer(scc, t_dwait, scc->kiss.waittime);
		else
			scc_start_tx_timer(scc, t_dwait, 0);
	}

	restore_flags(flags);
}

#ifdef CONFIG_SCC_TTY
static inline void 
kiss_store_byte(struct scc_channel *scc, unsigned char ch)
{
	struct mbuf *bp = scc->kiss_decode_bp;
	static int k = 0;
	
	if (bp != NULLBUF)
	{
		if (bp->cnt > scc->stat.bufsize)
		{
			if (!k++)
				printk(KERN_NOTICE "z8530drv: KISS frame too long\n");
		} else {
			k = 0;
			*bp->rw_ptr = ch;
			bp->rw_ptr++;
			bp->cnt++;
		}
	}
}

static inline int 
kiss_decode(struct scc_channel *scc, unsigned char ch)
{
	switch (scc->stat.tx_kiss_state) 
	{
		case KISS_IDLE:
			if (ch == FEND)
			{
				scc->kiss_decode_bp = scc_get_buffer(scc, BT_TRANSMIT);
				if (scc->kiss_decode_bp == NULLBUF)
					return 1;

				scc->stat.tx_kiss_state = KISS_DATA;
			} else scc->stat.txerrs++;
			break;
					
		case KISS_DATA:
			if (ch == FESC)
				scc->stat.tx_kiss_state = KISS_ESCAPE;
			else if (ch == FEND)
			{
				kiss_interpret_frame(scc);
				scc->stat.tx_kiss_state = KISS_IDLE;
			}
			else kiss_store_byte(scc, ch);
			break;
					
		case KISS_ESCAPE:
			if (ch == TFEND)
			{
				kiss_store_byte(scc, FEND);
				scc->stat.tx_kiss_state = KISS_DATA;
			}
			else if (ch == TFESC)
			{
				kiss_store_byte(scc, FESC);
				scc->stat.tx_kiss_state = KISS_DATA;
			}
			else
			{
				scc_enqueue_buffer(&scc->tx_buffer_pool, scc->kiss_decode_bp);
				scc->kiss_decode_bp = NULLBUF;
				scc->stat.txerrs++;
				scc->stat.tx_kiss_state = KISS_IDLE;
			}
			break;
	} /* switch */
	
	return 0;
	
}

/* ----> Encode received data and write it to the flip-buffer  <---- */

static void
kiss_encode(struct scc_channel *scc)
{
	struct mbuf *bp;
	struct tty_struct * tty = scc->tty;
	unsigned char ch;

	bp = scc->kiss_encode_bp;
	
	/* worst case: FEND 0 FESC TFEND -> 4 bytes */
	
	while(tty->flip.count < TTY_FLIPBUF_SIZE-4)
	{
		if (bp == NULLBUF)
		{
			bp = scc_dequeue_buffer(&scc->rx_queue);
			scc->kiss_encode_bp = bp;
			
			if (bp == NULLBUF)
			{
				scc->stat.rx_kiss_state = KISS_IDLE;
				break;
			}
		}
		

		if (bp->cnt <= 0)
		{
			if (--scc->stat.rx_queued < 0)
				scc->stat.rx_queued = 0;
					
			if (scc->stat.rx_kiss_state == KISS_RXFRAME)	/* new packet? */
			{
				tty_insert_flip_char(tty, FEND, 0);  /* send FEND for old frame */
				scc->stat.rx_kiss_state = KISS_IDLE; /* generate FEND for new frame */
			}
			
			scc_enqueue_buffer(&scc->rx_buffer_pool, bp);
			
			bp = scc->kiss_encode_bp = NULLBUF;
			continue;
		}
		

		if (scc->stat.rx_kiss_state == KISS_IDLE)
		{
			tty_insert_flip_char(tty, FEND, 0);
			scc->stat.rx_kiss_state = KISS_RXFRAME;
		}
				
		switch(ch = *bp->rw_ptr)
		{
			case FEND:
				tty_insert_flip_char(tty, FESC, 0);
				tty_insert_flip_char(tty, TFEND, 0);
				break;
			case FESC:
				tty_insert_flip_char(tty, FESC, 0);
				tty_insert_flip_char(tty, TFESC, 0);
				break;
			default:
				tty_insert_flip_char(tty, ch, 0);
		}
			
		bp->rw_ptr++;
		bp->cnt--;
	}
		
 	queue_task(&tty->flip.tqueue, &tq_timer); /* kick it... */
}
#endif /* CONFIG_SCC_TTY */


/* ******************************************************************* */
/* *		Init channel structures, special HW, etc...	     * */
/* ******************************************************************* */


static void
z8530_init(void)
{
	struct scc_channel *scc;
	int chip, k;
	unsigned long flags;
	char *flag;


	printk(KERN_INFO "Init Z8530 driver: %u channels, IRQ", Nchips*2);
	
	flag=" ";
	for (k = 0; k < 16; k++)
		if (Ivec[k].used) 
		{
			printk("%s%d", flag, k);
			flag=",";
		}
	printk("\n");
	

	/* reset and pre-init all chips in the system */
	for (chip = 0; chip < Nchips; chip++)
	{
		scc=&SCC_Info[2*chip];
		if (!scc->ctrl) continue;

		/* Special SCC cards */

		if(scc->brand & EAGLE)			/* this is an EAGLE card */
			Outb(scc->special,0x08);	/* enable interrupt on the board */
			
		if(scc->brand & (PC100 | PRIMUS))	/* this is a PC100/EAGLE card */
			Outb(scc->special,scc->option);	/* set the MODEM mode (0x22) */

			
		/* Init SCC */

		/* some general init we can do now */
		
		save_flags(flags);
		cli();
		
		Outb(scc->ctrl, 0);
		OutReg(scc->ctrl,R9,FHWRES);		/* force hardware reset */
		udelay(100);				/* give it 'a bit' more time than required */
		wr(scc, R2, chip*16);			/* interrupt vector */
		wr(scc, R9, VIS);			/* vector includes status */
		
        	restore_flags(flags);
        }

 
	Driver_Initialized = 1;
}


/* ******************************************************************** */
/* * 	Filesystem Routines: open, close, ioctl, settermios, etc      * */
/* ******************************************************************** */


/* scc_paranoia_check(): warn user if something went wrong		*/

static __inline__ int 
scc_paranoia_check(struct scc_channel *scc, kdev_t device, const char *routine)
{
#ifdef SCC_PARANOIA_CHECK

static const char *badmagic = 
	KERN_ALERT "Warning: bad magic number for Z8530 SCC struct (%s) in %s\n"; 
static const char *badinfo =  
	KERN_CRIT "Warning: Z8530 not found for (%s) in %s (forgot to run sccinit?)\n";
       
	if (!scc->init) 
	{
       		printk(badinfo, kdevname(device), routine);
		return 1;
	}
	
	if (scc->magic != SCC_MAGIC)
	{
		printk(badmagic, kdevname(device), routine);
		return 1;
	}
#endif

	return 0;
}

#ifdef CONFIG_SCC_TTY
/* ----> this one is called whenever you open the device <---- */

int 
scc_open(struct tty_struct *tty, struct file * filp)
{
	struct scc_channel *scc;
	int chan;
	
	chan = MINOR(tty->device) - tty->driver.minor_start;
	
	if (Driver_Initialized)
	{
		if ( (chan < 0) || (chan >= (Nchips * 2)) )
                	return -ENODEV;
        } else {
        	tty->driver_data = &SCC_Info[0];
        	MOD_INC_USE_COUNT;
        	return 0;
        }
 
 	scc = &SCC_Info[chan];
 	
	tty->driver_data = scc;
 	tty->termios->c_cflag &= ~CBAUD;
 	
	if (scc->magic != SCC_MAGIC)
	{
		printk(KERN_ALERT "z8530drv: scc_open() found bad magic number for device (%s)",
		       kdevname(tty->device));
		return -ENODEV;
	}

	MOD_INC_USE_COUNT;

	if(scc->tty != NULL || scc->stat.is_netdev)
	{
		scc->tty_opened++;
		return 0;
	}
	
	scc->tty = tty;

 	if(!scc->init) 
 		return 0;
 
	alloc_buffer_pool(scc);
 	
	scc->throttled = 0;

	scc->stat.tx_kiss_state = KISS_IDLE;
	scc->stat.rx_kiss_state = KISS_IDLE;
 
	init_channel(scc);
	return 0;
}


/* ----> and this whenever you close the device <---- */

static void
scc_close(struct tty_struct *tty, struct file * filp)
{
	struct scc_channel *scc = tty->driver_data;
	unsigned long flags;

        if (!scc || (scc->magic != SCC_MAGIC))
                return;

                
        MOD_DEC_USE_COUNT;
	
	if(scc->tty_opened || scc->stat.is_netdev)
	{
		scc->tty_opened--;
		return;
	}
	
	tty->driver_data = NULLBUF;
	
	if (!Driver_Initialized)
		return;
	
	save_flags(flags);
	cli();
	
	Outb(scc->ctrl,0);		/* Make sure pointer is written */
	wr(scc,R1,0);			/* disable interrupts */
	wr(scc,R3,0);
	
	scc->tty = NULL;

	del_timer(&scc->tx_t);
	del_timer(&scc->tx_wdog);
	del_timer(&scc->rx_t);

	restore_flags(flags);
	
	if (!scc->init)
		return;

	free_buffer_pool(scc);
	
	scc->throttled = 0;
	tty->stopped = 0;
}


/*
 * change scc_speed
 */
 
static void
scc_change_speed(struct scc_channel * scc)
{
	long speed;
	
	if (scc->tty == NULL)
		return;
		

	speed = baud_table[scc->tty->termios->c_cflag & CBAUD];
	
	if (speed > 0) scc->modem.speed = speed;
	
	if (scc->stat.tx_state == 0)	/* only switch baudrate on rx... ;-) */
		set_speed(scc);
}
#endif /* CONFIG_SCC_TTY */

/* ----> ioctl-routine of the driver <---- */


/* sub routines to read/set KISS parameters */
 
/* generic ioctl() functions for both TTY or network device mode:
 *
 * TIOCSCCCFG		- configure driver	arg: (struct scc_hw_config *) arg
 * TIOCSCCINI		- initialize driver	arg: ---
 * TIOCSCCCHANINI	- initialize channel	arg: (struct scc_modem *) arg
 * TIOCSCCGKISS		- get level 1 parameter	arg: (struct scc_kiss_cmd *) arg
 * TIOCSCCSKISS		- set level 1 parameter arg: (struct scc_kiss_cmd *) arg
 * TIOCSCCSTAT		- get driver status	arg: (struct scc_stat *) arg
 */
 
static int
scc_ioctl(struct scc_channel *scc, unsigned int cmd, void *arg)
{
	unsigned long flags;
	struct scc_kiss_cmd kiss_cmd;
	struct scc_mem_config memcfg;
	struct scc_hw_config hwcfg;
	int error, chan;
	unsigned char device_name[10];
	
	if (!Driver_Initialized)
	{
		if (cmd == TIOCSCCCFG)
		{
			int found = 1;

			if (!suser()) return -EPERM;
			if (!arg) return -EFAULT;

			error = verify_area(VERIFY_READ, arg, sizeof(struct scc_hw_config));
			if (error) return error;

			if (Nchips >= MAXSCC) 
				return -EINVAL;

			memcpy_fromfs(&hwcfg, arg, sizeof(hwcfg));

			if (hwcfg.irq == 2) hwcfg.irq = 9;

			if (!Ivec[hwcfg.irq].used && hwcfg.irq)
			{
				if (request_irq(hwcfg.irq, scc_isr, SA_INTERRUPT, "AX.25 SCC", NULL))
					printk(KERN_WARNING "z8530drv: warning, cannot get IRQ %d\n", hwcfg.irq);
				else
					Ivec[hwcfg.irq].used = 1;
			}

			if (hwcfg.vector_latch) 
				Vector_Latch = hwcfg.vector_latch;

			if (hwcfg.clock == 0)
				hwcfg.clock = DEFAULT_CLOCK;

#ifndef DONT_CHECK
			disable_irq(hwcfg.irq);

			check_region(scc->ctrl, 1);
			Outb(hwcfg.ctrl_a, 0);
			udelay(5);
			OutReg(hwcfg.ctrl_a,R13,0x55);		/* is this chip really there? */
			udelay(5);

			if (InReg(hwcfg.ctrl_a,R13) != 0x55)
				found = 0;

			enable_irq(hwcfg.irq);
#endif

			if (found)
			{
				SCC_Info[2*Nchips  ].ctrl = hwcfg.ctrl_a;
				SCC_Info[2*Nchips  ].data = hwcfg.data_a;
				SCC_Info[2*Nchips  ].irq  = hwcfg.irq;
				SCC_Info[2*Nchips+1].ctrl = hwcfg.ctrl_b;
				SCC_Info[2*Nchips+1].data = hwcfg.data_b;
				SCC_Info[2*Nchips+1].irq  = hwcfg.irq;
			
				SCC_ctrl[Nchips].chan_A = hwcfg.ctrl_a;
				SCC_ctrl[Nchips].chan_B = hwcfg.ctrl_b;
				SCC_ctrl[Nchips].irq    = hwcfg.irq;
			}


			for (chan = 0; chan < 2; chan++)
			{
				sprintf(device_name, "%s%i", SCC_DriverName, 2*Nchips+chan);

				SCC_Info[2*Nchips+chan].special = hwcfg.special;
				SCC_Info[2*Nchips+chan].clock = hwcfg.clock;
				SCC_Info[2*Nchips+chan].brand = hwcfg.brand;
				SCC_Info[2*Nchips+chan].option = hwcfg.option;
				SCC_Info[2*Nchips+chan].enhanced = hwcfg.escc;

#ifdef DONT_CHECK
				printk(KERN_INFO "%s: data port = 0x%3.3x  control port = 0x%3.3x\n",
					device_name, 
					SCC_Info[2*Nchips+chan].data, 
					SCC_Info[2*Nchips+chan].ctrl);

#else
				printk(KERN_INFO "%s: data port = 0x%3.3x  control port = 0x%3.3x -- %s\n",
					device_name,
					chan? hwcfg.data_b : hwcfg.data_a, 
					chan? hwcfg.ctrl_b : hwcfg.ctrl_a,
					found? "found" : "missing");
#endif

				if (found)
				{
					request_region(SCC_Info[2*Nchips+chan].ctrl, 1, "scc ctrl");
					request_region(SCC_Info[2*Nchips+chan].data, 1, "scc data");
#ifdef CONFIG_SCC_DEV
					if (Nchips+chan != 0)
						scc_net_setup(&SCC_Info[2*Nchips+chan], device_name);
#endif
				}
			}
			
			if (found) Nchips++;
			
#ifdef SCC_DEBUG
			printk(KERN_INFO "Available modes:"
#ifdef CONFIG_SCC_DEV
			" dev"
#endif
#ifdef CONFIG_SCC_TTY
			" tty"
#endif
			"\n");
#endif
			
			return 0;
		}
		
		if (cmd == TIOCSCCINI)
		{
			if (!suser())
				return -EPERM;
				
			if (Nchips == 0)
				return -EINVAL;
			
			z8530_init();
			return 0;
		}
		
		return -EINVAL;	/* confuse the user */
	}
	
	if (!scc->init)
	{
		if (cmd == TIOCSCCCHANINI)
		{
			if (!suser()) return -EPERM;
			if (!arg) return -EINVAL;
			error = verify_area(VERIFY_READ, arg, sizeof(struct scc_modem));
			if (error) return error;

			scc->stat.rxbuffers = RXBUFFERS;
			scc->stat.txbuffers = TXBUFFERS;
			scc->stat.bufsize   = BUFSIZE;

			memcpy_fromfs(&scc->modem, arg, sizeof(struct scc_modem));
			
			/* default KISS Params */
		
			if (scc->modem.speed < 4800)
			{
				scc->kiss.txdelay = 36;		/* 360 ms */
				scc->kiss.persist = 42;		/* 25% persistence */			/* was 25 */
				scc->kiss.slottime = 16;	/* 160 ms */
				scc->kiss.tailtime = 4;		/* minimal reasonable value */
				scc->kiss.fulldup = 0;		/* CSMA */
				scc->kiss.waittime = 50;	/* 500 ms */
				scc->kiss.maxkeyup = 10;	/* 10 s */
				scc->kiss.mintime = 3;		/* 3 s */
				scc->kiss.idletime = 30;	/* 30 s */
				scc->kiss.maxdefer = 120;	/* 2 min */
				scc->kiss.softdcd = 0;		/* hardware dcd */
			} else {
				scc->kiss.txdelay = 10;		/* 100 ms */
				scc->kiss.persist = 64;		/* 25% persistence */			/* was 25 */
				scc->kiss.slottime = 8;		/* 160 ms */
				scc->kiss.tailtime = 1;		/* minimal reasonable value */
				scc->kiss.fulldup = 0;		/* CSMA */
				scc->kiss.waittime = 50;	/* 500 ms */
				scc->kiss.maxkeyup = 7;		/* 7 s */
				scc->kiss.mintime = 3;		/* 3 s */
				scc->kiss.idletime = 30;	/* 30 s */
				scc->kiss.maxdefer = 120;	/* 2 min */
				scc->kiss.softdcd = 0;		/* hardware dcd */
			}
			
			scc->init = 1;
			
			return 0;
		}
		
		return -EINVAL;
	}
	
	switch(cmd)
	{
		case TIOCSCCSMEM:
		case TIOCCHANMEM_OLD:
			if (!suser()) return -EPERM;
			if (!arg) return -EINVAL;
			error = verify_area(VERIFY_READ, arg, sizeof(struct scc_mem_config));
			if (error) return error;
	
			memcpy_fromfs(&memcfg, arg, sizeof(struct scc_mem_config));

			save_flags(flags); 
			cli();

			free_buffer_pool(scc);
			scc->stat.rxbuffers = memcfg.rxbuffers;
			scc->stat.txbuffers = memcfg.txbuffers;
			scc->stat.bufsize   = memcfg.bufsize;
			if (scc->tty || (scc->dev && (scc->dev->flags & IFF_UP)))
				alloc_buffer_pool(scc);
		
			restore_flags(flags);
			return 0;
		
		case TIOCSCCGSTAT:
		case TIOCSCCSTAT_OLD:
			if (!arg) return -EINVAL;
			error = verify_area(VERIFY_WRITE, arg, sizeof(struct scc_mem_config));
			if (error) return error;
			
			memcpy_tofs(arg, &scc->stat, sizeof(struct scc_stat));
			return 0;
		
		case TIOCSCCGKISS:
		case TIOCGKISS_OLD:
			if (!arg) return -EINVAL;
			error = verify_area(VERIFY_WRITE, arg, sizeof(struct scc_mem_config));
			if (error) return error;
		
			memcpy_fromfs(&kiss_cmd, arg, sizeof(struct scc_kiss_cmd));
			kiss_cmd.param = kiss_get_param(scc, kiss_cmd.command);
			memcpy_tofs(arg, &kiss_cmd, sizeof(struct scc_kiss_cmd));
			return 0;
			break;
		
		case TIOCSCCSKISS:
		case TIOCSKISS_OLD:
			if (!suser()) return -EPERM;
			if (!arg) return -EINVAL;
			error = verify_area(VERIFY_READ, arg, sizeof(struct scc_mem_config));
			if (error) return error;

			memcpy_fromfs(&kiss_cmd, arg, sizeof(struct scc_kiss_cmd));
			return kiss_set_param(scc, kiss_cmd.command, kiss_cmd.param);
			break;

		default:
			return -ENOIOCTLCMD;
		
	}
	
	return 0;
}

#ifdef CONFIG_SCC_TTY
/* addtional ioctl() for TTY driver mode:
 *
 * TIOCMGET 	- get modem status	arg: (unsigned long *) arg
 * TIOCMBIS 	- set PTT		arg: ---
 * TIOCMBIC 	- reset PTT		arg: ---
 * TIOCMBIC 	- set PTT		arg: ---
 */

static int
scc_tty_ioctl(struct tty_struct *tty, struct file * file, unsigned int cmd, unsigned long arg)
{
	struct scc_channel * scc = tty->driver_data;
	unsigned long flags;
	unsigned int result;
	unsigned int value;
	int error;

        if (scc->magic != SCC_MAGIC) 
        {
		printk(KERN_ALERT "z8530drv: scc_ioctl() found bad magic number for device %s",
			SCC_DEVNAME);
			
                return -ENODEV;
        }
	                                                 
	if (!Driver_Initialized && cmd == TIOCSCCINI)
		scc->tty=tty;

	switch(cmd)
	{
		case TIOCSCCCFG:
		case TIOCSCCINI:
		case TIOCSCCCHANINI:
		case TIOCSCCSMEM:
		case TIOCSCCSKISS:
		case TIOCSCCGKISS:

		case TIOCSCCCFG_OLD:
		case TIOCSCCINI_OLD:
		case TIOCCHANINI_OLD:
		case TIOCCHANMEM_OLD:
		case TIOCSKISS_OLD:
		case TIOCGKISS_OLD:
		case TIOCSCCSTAT_OLD:
			return scc_ioctl(scc, cmd, (void *) arg);

		case TCSBRK:
			return 0;
			
		case TIOCMGET:
			error = verify_area(VERIFY_WRITE, (void *) arg,sizeof(unsigned int *));
			if (error)
				return error;

			save_flags(flags);
			cli();
		
			result =  ((scc->wreg[R5] & RTS) ? TIOCM_RTS : 0)
				| ((scc->wreg[R5] & DTR) ? TIOCM_DTR : 0)
				| ((InReg(scc->ctrl,R0) & DCD)  ? TIOCM_CAR : 0)
				| ((InReg(scc->ctrl,R0) & CTS)  ? TIOCM_CTS : 0);
		
			restore_flags(flags);
			
			put_user(result,(unsigned int *) arg);
			return 0;
			
		case TIOCMBIS:
		case TIOCMBIC:
		case TIOCMSET:
			switch (cmd) 
			{
				case TIOCMBIS:
					scc->wreg[R5] |= DTR;
					scc->wreg[R5] |= RTS;
					break;
				case TIOCMBIC:
					scc->wreg[R5] &= ~DTR;
					scc->wreg[R5] &= ~RTS;
					break;
				case TIOCMSET:
					error = verify_area(VERIFY_READ, (void *) arg,sizeof(unsigned int *));
					if (error)
						return error;
					value = get_user((unsigned int *) arg);
			
					if(value & TIOCM_DTR)
						scc->wreg[R5] |= DTR;
					else
						scc->wreg[R5] &= ~DTR;
					if(value & TIOCM_RTS)
						scc->wreg[R5] |= RTS;
					else
						scc->wreg[R5] &= ~RTS;
					break;
			}
			
			OutReg(scc->ctrl, R5, scc->wreg[R5]);
			return 0;
		
		case TCGETS:
			error = verify_area(VERIFY_WRITE, (void *) arg, sizeof(struct termios));
			if (error)
				return error;
			if (!arg) 
				return -EFAULT;
			
			memcpy_tofs((void *) arg, scc->tty->termios, sizeof(struct termios));
			return 0;
		
		case TCSETS:
		case TCSETSF:		/* should flush first, but... */
		case TCSETSW:		/* should wait 'till flush, but... */
			if (!arg)
				return -EFAULT;
		
			error = verify_area(VERIFY_READ, (void *) arg, sizeof(struct termios));
			if (error)
				return error;
			memcpy_fromfs(scc->tty->termios, (void *) arg, sizeof(struct termios));
			scc_change_speed(scc);
			return 0;
		
	}
	
	return -ENOIOCTLCMD;
}


/* ----> tx routine: decode KISS data and scc_enqueue it <---- */

/* send raw frame to SCC. used for AX.25 */
int 
scc_write(struct tty_struct *tty, int from_user, const unsigned char *buf, int count)
{
	struct scc_channel * scc = tty->driver_data;
	unsigned char *p;
	int cnt, cnt2;
	
	if (!tty) 
		return -EINVAL;
	
	if (scc_paranoia_check(scc, tty->device, "scc_write"))
		return -EINVAL;

	if (scc->kiss.tx_inhibit) return count;

	cnt2 = count;
	
	while (cnt2)
	{
		cnt   = cnt2 > BUFSIZE? BUFSIZE:cnt2;
		cnt2 -= cnt;
		
		if (from_user)
		{
			down(&scc_sem);
			memcpy_fromfs(scc_wbuf, buf, cnt);
			up(&scc_sem);
		}
		else
			memcpy(scc_wbuf, buf, cnt);
		
		/* The timeout of the slip driver is very small, */
		/* thus we'll wake him up now. 	    		 */

		if (cnt2 == 0)
		{
			wake_up_interruptible(&tty->write_wait);
		
			if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
			    tty->ldisc.write_wakeup)
				(tty->ldisc.write_wakeup)(tty);
		} else 
			buf += cnt;
			
		p=scc_wbuf;
		
		while(cnt--) 
		  if (kiss_decode(scc, *p++))
		  	return -ENOMEM;
	} /* while cnt2 */

	return count;
}
				

/* put a single char into the buffer */

static void 
scc_put_char(struct tty_struct * tty, unsigned char ch)
{
	struct scc_channel *scc = tty->driver_data;
	
	if (scc_paranoia_check(scc, tty->device, "scc_put_char"))
		return;

	kiss_decode(scc, ch);
}

static void 
scc_flush_chars(struct tty_struct * tty)
{
	struct scc_channel *scc = tty->driver_data;
	
	scc_paranoia_check(scc, tty->device, "scc_flush_chars"); /* just to annoy the user... */
	
	return;	/* no flush needed */
}


static int 
scc_write_room(struct tty_struct *tty)
{
	struct scc_channel *scc = tty->driver_data;
	
	if (scc_paranoia_check(scc, tty->device, "scc_write_room"))
		return 0;
	
	return BUFSIZE;
}

static int 
scc_chars_in_buffer(struct tty_struct *tty)
{
	struct scc_channel *scc = tty->driver_data;
	
	if (scc && scc->kiss_decode_bp)
		return scc->kiss_decode_bp->cnt;
	else
		return 0;
}

static void 
scc_flush_buffer(struct tty_struct *tty)
{
	struct scc_channel *scc = tty->driver_data;

	if (scc_paranoia_check(scc, tty->device, "scc_flush_buffer"))
		return;
	wake_up_interruptible(&tty->write_wait);
	if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
	    tty->ldisc.write_wakeup)
		(tty->ldisc.write_wakeup)(tty);
}

static void 
scc_throttle(struct tty_struct *tty)
{
	struct scc_channel *scc = tty->driver_data;
	
	if (scc_paranoia_check(scc, tty->device, "scc_throttle"))
		return;

#ifdef SCC_DEBUG		
	printk(KERN_DEBUG "z8530drv: scc_throttle() called for device %d\n", MINOR(tty->device));
#endif
	scc->throttled = 1;
	
	del_timer(&(scc->rx_t));
	scc->rx_t.expires = jiffies + HZ/25;
	add_timer(&scc->rx_t);
}

static void 
scc_unthrottle(struct tty_struct *tty)
{
	struct scc_channel *scc = tty->driver_data;
	
	if (scc_paranoia_check(scc, tty->device, "scc_unthrottle"))
		return;
		
#ifdef SCC_DEBUG
	printk(KERN_DEBUG "z8350drv: scc_unthrottle() called for device %d\n", MINOR(tty->device));
#endif		
	scc->throttled = 0;
	del_timer(&(scc->rx_t));
}

/* experimental, the easiest way to stop output is a fake scc_throttle */

static void 
scc_start(struct tty_struct *tty)
{
	struct scc_channel *scc = tty->driver_data;
	
	if (scc_paranoia_check(scc, tty->device, "scc_start"))
		return;
		
	scc_unthrottle(tty);
}

static void 
scc_stop(struct tty_struct *tty)
{
	struct scc_channel *scc = tty->driver_data;
	
	if (scc_paranoia_check(scc, tty->device, "scc_stop"))
		return;
		
	scc_throttle(tty);
}

static void
scc_set_termios(struct tty_struct * tty, struct termios * old_termios)
{
	struct scc_channel *scc = tty->driver_data;
	
	if (scc_paranoia_check(scc, tty->device, "scc_set_termios"))
		return;
		
	if (old_termios && (tty->termios->c_cflag == old_termios->c_cflag)) 
		return;
		
	scc_change_speed(scc);
}

static void
scc_set_ldisc(struct tty_struct * tty)
{
	struct scc_channel *scc = tty->driver_data;

	if (scc_paranoia_check(scc, tty->device, "scc_set_ldisc"))
		return;
		
	scc_change_speed(scc);
}
#endif /* CONFIG_SCC_TTY */

#ifdef CONFIG_SCC_DEV
/* ******************************************************************** */
/* *			    Network driver routines		      * */
/* ******************************************************************** */

static unsigned char ax25_bcast[AX25_ADDR_LEN] =
{'Q' << 1, 'S' << 1, 'T' << 1, ' ' << 1, ' ' << 1, ' ' << 1, '0' << 1};
static unsigned char ax25_nocall[AX25_ADDR_LEN] =
{'L' << 1, 'I' << 1, 'N' << 1, 'U' << 1, 'X' << 1, ' ' << 1, '1' << 1};

static int 
scc_net_init(struct device *dev)
{
	return 0;	/* dummy */
}

/* ----> open network device <---- */

static int 
scc_net_open(struct device *dev)
{
	struct scc_channel *scc = (struct scc_channel *) dev->priv;

	if (scc == NULL || scc->magic != SCC_MAGIC)
		return -ENODEV;

	if (scc->tty != NULL)
		return -EBUSY;

 	if (!scc->init)
		return -EINVAL;

	scc->stat.is_netdev = 1;

	MOD_INC_USE_COUNT;

	alloc_buffer_pool(scc);

	scc->stat.tx_kiss_state = KISS_IDLE;	/* unused */
	scc->stat.rx_kiss_state = KISS_IDLE;	/* unused */
 
	init_channel(scc);

	dev->tbusy = 0;
	dev->start = 1;

	return 0;
}

/* ----> close network device <---- */

static int 
scc_net_close(struct device *dev)
{
	struct scc_channel *scc = (struct scc_channel *) dev->priv;
	unsigned long flags;

	if (scc == NULL || scc->magic != SCC_MAGIC)
		return -ENODEV;

	if (!scc->stat.is_netdev)
		return -ENXIO;

	MOD_DEC_USE_COUNT;

	scc->stat.is_netdev = 0;

	save_flags(flags); 
	cli();
	
	Outb(scc->ctrl,0);		/* Make sure pointer is written */
	wr(scc,R1,0);			/* disable interrupts */
	wr(scc,R3,0);

	del_timer(&scc->tx_t);
	del_timer(&scc->tx_wdog);
#ifdef CONFIG_SCC_TTY
	del_timer(&scc->rx_t);
#endif
	restore_flags(flags);

	free_buffer_pool(scc);

	dev->tbusy = 1;
	dev->start = 0;

	return 0;
}

/* ----> receive frame, called from scc_rxint() <---- */

static void 
scc_net_rx(struct scc_channel *scc, struct mbuf *bp)
{
	struct sk_buff *skb;

	if ( bp == NULL || bp->cnt == 0 || 
	    scc == NULL || scc->magic != SCC_MAGIC)
		return;
		
	skb = dev_alloc_skb(bp->cnt+1);

	if (skb == NULL)
	{
		scc->dev_stat.rx_dropped++;
		return;
	}

	scc->dev_stat.rx_packets++;

	skb->dev = scc->dev;
	skb->protocol = htons(ETH_P_AX25);
	
	memcpy(skb_put(skb, bp->cnt), bp->data, bp->cnt);

	skb->mac.raw = skb->data;
	netif_rx(skb);

	return;
}

/* ----> transmit frame <---- */

static int 
scc_net_tx(struct sk_buff *skb, struct device *dev)
{
	struct scc_channel *scc = (struct scc_channel *) dev->priv;
	struct mbuf *bp;
	long ticks;
	
	/* 
	 * We have our own queues, dev->tbusy is set only by t_maxkeyup
	 * to avoid running the driver into maxkeup again and again.
	 *
	 * Hope I havn't outsmart me _again_ ;-)
	 */

	if (dev->tbusy)
	{
		ticks = (signed) (jiffies - dev->trans_start);

		if (ticks < scc->kiss.maxdefer*HZ || scc == NULL)
			return 1;

		/* 
		 * Arrgh... Seems to be a _very_ busy channel.
		 * throw away transmission queue. 
		 */

		del_timer(&scc->tx_wdog);
		t_busy((unsigned long) scc);

                dev->tbusy=0;
                dev->trans_start = jiffies;
        }

	if (skb == NULL)
	{
		dev_tint(dev);
		return 0;
	}

	if (scc == NULL || scc->magic != SCC_MAGIC)
	{
		dev_kfree_skb(skb, FREE_WRITE);
		return 0;
	}

	scc->dev_stat.tx_packets++;
	bp = scc_get_buffer(scc, BT_TRANSMIT);

	if (bp == NULLBUF || skb->len > scc->stat.bufsize || skb->len == 0)
	{
		scc->dev_stat.tx_dropped++;
		if (bp) scc_enqueue_buffer(&scc->tx_buffer_pool, bp);
		dev_kfree_skb(skb, FREE_WRITE);
		return 0;
	}

	memcpy(bp->data, skb->data, skb->len);
	bp->cnt = skb->len;

	dev_kfree_skb(skb, FREE_WRITE);

	scc->kiss_decode_bp = bp;
	kiss_interpret_frame(scc);
	dev->trans_start = jiffies;
	return 0;
}

/* ----> set interface callsign <---- */

static int 
scc_net_set_mac_address(struct device *dev, void *addr)
{
	struct sockaddr *sa = (struct sockaddr *) addr;
	memcpy(dev->dev_addr, sa->sa_data, dev->addr_len);
	return 0;
}

/* ----> rebuild header <---- */

static int 
scc_net_rebuild_header(void *buff, struct device *dev, unsigned long raddr, struct sk_buff *skb)
{
    return ax25_rebuild_header(buff, dev, raddr, skb);
}

/* ----> "hard" header <---- */

static int 
scc_net_header(struct sk_buff *skb, struct device *dev, unsigned short type, 
	       void *daddr, void *saddr, unsigned len)
{
    return ax25_encapsulate(skb, dev, type, daddr, saddr, len);
}

/* ----> get statistics <---- */

static struct enet_statistics *
scc_net_get_stats(struct device *dev)
{
	struct scc_channel *scc = (struct scc_channel *) dev->priv;
	
	if (scc == NULL || scc->magic != SCC_MAGIC)
		return NULL;
		
	scc->dev_stat.rx_errors = scc->stat.rxerrs + scc->stat.rx_over;
	scc->dev_stat.tx_errors = scc->stat.txerrs + scc->stat.tx_under;
	scc->dev_stat.rx_fifo_errors = scc->stat.rx_over;
	scc->dev_stat.tx_fifo_errors = scc->stat.tx_under;

	return &scc->dev_stat;
}

/* ----> ioctl for network devices <---- */

static int 
scc_net_ioctl(struct device *dev, struct ifreq *ifr, int cmd)
{
	struct scc_channel *scc = (struct scc_channel *) dev->priv;
	unsigned int ncmd = 0;
	int res;

	if (scc == NULL || scc->magic != SCC_MAGIC)
		return -EINVAL;

	if (!Driver_Initialized && cmd == SIOCSCCINI)
	{
		scc->dev = dev;
		scc->stat.is_netdev = 1;
	}

	switch (cmd)
	{
		case SIOCSCCRESERVED:	return -EINVAL;      /* unused */
		case SIOCSCCCFG:	ncmd = TIOCSCCCFG;	break;
		case SIOCSCCINI:	ncmd = TIOCSCCINI;	break;
		case SIOCSCCCHANINI:	ncmd = TIOCSCCCHANINI;	break;
		case SIOCSCCSMEM:	ncmd = TIOCSCCSMEM;	break;
		case SIOCSCCGKISS:	ncmd = TIOCSCCGKISS;	break;
		case SIOCSCCSKISS:	ncmd = TIOCSCCSKISS;	break;
		case SIOCSCCGSTAT:	ncmd = TIOCSCCGSTAT;	break;
		default:		return -EINVAL;
	}

	res = scc_ioctl(scc, ncmd, (void *) ifr->ifr_data);

#ifdef CONFIG_SCC_TTY
	if (!(dev->flags & IFF_UP))
		scc->stat.is_netdev = 0;
#endif

	return res;
	
}


/* ----> initialize interface <---- */

static int 
scc_net_setup(struct scc_channel *scc, unsigned char *name)
{
	int k;
	unsigned char *buf;
	struct device *dev;

	if (dev_get(name) != NULL)
	{
		printk(KERN_INFO "Z8530drv: device %s already exists.\n", name);
		return -EEXIST;
	}

	if ((scc->dev = (struct device *) kmalloc(sizeof(struct device), GFP_KERNEL)) == NULL)
		return -ENOMEM;

#ifndef CONFIG_SCC_TTY
	scc->stat.is_netdev = 1;
#endif
	dev = scc->dev;
	memset(dev, 0, sizeof(struct device));

	buf = (unsigned char *) kmalloc(10, GFP_KERNEL);
	strcpy(buf, name);

	dev->priv = (void *) scc;
	dev->name = buf;
	dev->init = scc_net_init;

	if (register_netdev(dev) != 0)
	{
		kfree(dev);
		return -EIO;
	}

	for (k=0; k < DEV_NUMBUFFS; k++)
		skb_queue_head_init(&dev->buffs[k]);

	dev->open            = scc_net_open;
	dev->stop	     = scc_net_close;

	dev->hard_start_xmit = scc_net_tx;
	dev->hard_header     = scc_net_header;
	dev->rebuild_header  = scc_net_rebuild_header;
	dev->set_mac_address = scc_net_set_mac_address;
	dev->get_stats       = scc_net_get_stats;
	dev->do_ioctl        = scc_net_ioctl;

	memcpy(dev->broadcast, ax25_bcast,  AX25_ADDR_LEN);
	memcpy(dev->dev_addr,  ax25_nocall, AX25_ADDR_LEN);
 
	dev->flags      = 0;
	dev->family     = AF_INET;
	dev->pa_addr    = 0;
	dev->pa_brdaddr = 0;
	dev->pa_mask    = 0;
	dev->pa_alen    = 4;

	dev->type = ARPHRD_AX25;
	dev->hard_header_len = AX25_MAX_HEADER_LEN + AX25_BPQ_HEADER_LEN;
	dev->mtu = AX25_DEF_PACLEN;
	dev->addr_len = AX25_ADDR_LEN;

	return 0;
}
#endif /* CONFIG_SCC_DEV */

/* ******************************************************************** */
/* *		dump statistics to /proc/net/z8530drv		      * */
/* ******************************************************************** */


static int 
scc_net_get_info(char *buffer, char **start, off_t offset, int length, int dummy)
{
	struct scc_channel *scc;
	char *txstate, *clksrc, *brand;
	struct scc_kiss *kiss;
	struct scc_stat *stat;
	int len = 0;
	off_t pos = 0;
	off_t begin = 0;
	int k;

	if (!Driver_Initialized)
	{
		len += sprintf(buffer, "Driver not initialized.\n");
		goto done;
	}

	if (!Nchips)
	{
		len += sprintf(buffer, "Z8530 chips not found\n");
		goto done;
	}

	for (k = 0; k < Nchips*2; k++)
	{
		scc = &SCC_Info[k];
		stat = &scc->stat;
		kiss = &scc->kiss;

		if (!scc->init)
			continue;

		switch(stat->tx_state)
		{
			case TXS_IDLE:		txstate = "idle";	break;
			case TXS_BUSY:		txstate = "busy";	break;
			case TXS_ACTIVE:	txstate = "active";	break;
			case TXS_NEWFRAME:	txstate = "new";	break;
			case TXS_IDLE2:		txstate = "keyed";	break;
			case TXS_WAIT:		txstate = "wait";	break;
			case TXS_TIMEOUT:	txstate = "timeout";	break;
			default:		txstate = "???";
		}
		switch(scc->modem.clocksrc)
		{
			case CLK_DPLL:		clksrc = "dpll";	break;
			case CLK_EXTERNAL:	clksrc = "ext";		break;
			case CLK_DIVIDER:	clksrc = "div";		break;
			default:		clksrc = "???";
		}
		switch(scc->brand)
		{
			case PA0HZP:	brand="pa0hzp";	break;
			case EAGLE:	brand="eagle";	break;
			case PC100:	brand="pc1100";	break;
			case PRIMUS:	brand="primus";	break;
			case DRSI:	brand="drsi";	break;
			case BAYCOM:	brand="baycom";	break;
			default:	brand="???";
		}
	
		len += sprintf(buffer+len, "Device     : %s%d\n", SCC_DriverName, k);
		len += sprintf(buffer+len, "Hardware   : %3.3x %3.3x %d %lu %s %s %3.3x %3.3x %d\n", 
				scc->data, scc->ctrl, scc->irq, scc->clock, brand,
				scc->enhanced? "enh":"norm", Vector_Latch, scc->special, 
				scc->option);
		len += sprintf(buffer+len, "Received   : %lu %lu %d\n", 
				stat->rxframes, stat->rxerrs, stat->rx_over);
		len += sprintf(buffer+len, "Transmitted: %lu %lu %d %s\n", 
				stat->txframes, stat->txerrs, stat->tx_under, txstate);
		len += sprintf(buffer+len, "Interrupts : %lu %lu %lu %lu\n",
				stat->rxints, stat->txints, stat->exints, stat->spints);
		len += sprintf(buffer+len, "Buffers    : %d %d/%d %d/%d %d\n", 
				stat->bufsize, stat->rx_queued, stat->rxbuffers,
				stat->tx_queued, stat->txbuffers, stat->nospace);
		len += sprintf(buffer+len, "MODEM      : %lu %s %s %s\n", 
				scc->modem.speed, scc->modem.nrz? "nrz":"nrzi",
				clksrc, kiss->softdcd? "soft":"hard");
		len += sprintf(buffer+len, "Mode       : %s\n",
				stat->is_netdev? "dev":"tty");
#define K(x) kiss->x
		len += sprintf(buffer+len, "KISS params: %d %d %d %d %d %d %d %d %d %d %d %d\n",
				K(txdelay), K(persist), K(slottime), K(tailtime),
				K(fulldup), K(waittime), K(mintime), K(maxkeyup),
				K(idletime), K(maxdefer), K(tx_inhibit), K(group));
#undef K
#ifdef SCC_DEBUG
		{
			int reg;

			len += sprintf(buffer+len, "Z8530 wregs: ");
			for (reg = 0; reg < 16; reg++)
				len += sprintf(buffer+len, "%2.2x ", scc->wreg[reg]);
			len += sprintf(buffer+len, "\n");
			
			len += sprintf(buffer+len, "Z8530 rregs: %2.2x %2.2x XX ", InReg(scc->ctrl,R0), InReg(scc->ctrl,R1));
			for (reg = 3; reg < 8; reg++)
				len += sprintf(buffer+len, "%2.2x ", InReg(scc->ctrl, reg));
			len += sprintf(buffer+len, "XX ");
			for (reg = 9; reg < 16; reg++)
				len += sprintf(buffer+len, "%2.2x ", InReg(scc->ctrl, reg));
			len += sprintf(buffer+len, "\n");
		}
#endif
		len += sprintf(buffer+len, "\n");

                pos = begin + len;

                if (pos < offset) {
                        len   = 0;
                        begin = pos;
                }

                if (pos > offset + length)
                        break;
	}

done:

        *start = buffer + (offset - begin);
        len   -= (offset - begin);

        if (len > length) len = length;

        return len;
}

#ifdef CONFIG_INET
struct proc_dir_entry scc_proc_dir_entry = 
{ 
  PROC_NET_Z8530, 8, "z8530drv", S_IFREG | S_IRUGO, 1, 0, 0, 0, 
  &proc_net_inode_operations, scc_net_get_info 
};

#define scc_net_procfs_init()   proc_net_register(&scc_proc_dir_entry);
#define scc_net_procfs_remove() proc_net_unregister(PROC_NET_Z8530);
#else
#define scc_net_procfs_init()
#define scc_net_procfs_remove()
#endif

  
/* ******************************************************************** */
/* * 			Init SCC driver 			      * */
/* ******************************************************************** */

int scc_init (void)
{
	int chip, chan, k;
#ifdef CONFIG_SCC_DEV
	char devname[10];
#endif
	
	printk(KERN_INFO BANNER);

#ifdef CONFIG_SCC_TTY
	memset(&scc_std_termios, 0, sizeof(struct termios));
        memset(&scc_driver, 0, sizeof(struct tty_driver));
        scc_driver.magic = TTY_DRIVER_MAGIC;
        scc_driver.name = SCC_DriverName;
        scc_driver.major = Z8530_MAJOR;
        scc_driver.minor_start = 0;
        scc_driver.num = MAXSCC*2;
        scc_driver.type = TTY_DRIVER_TYPE_SERIAL;
        scc_driver.subtype = 1;			/* not needed */
        scc_driver.init_termios = scc_std_termios;
        scc_driver.init_termios.c_cflag = B9600  | CREAD | CS8 | HUPCL | CLOCAL;
        scc_driver.init_termios.c_iflag = IGNBRK | IGNPAR;
        scc_driver.flags = TTY_DRIVER_REAL_RAW;
        scc_driver.refcount = &scc_refcount;
        scc_driver.table = scc_table;
        scc_driver.termios = (struct termios **) scc_termios;
        scc_driver.termios_locked = (struct termios **) scc_termios_locked;
        scc_driver.open = scc_open;
        scc_driver.close = scc_close;
        scc_driver.write = scc_write;
        scc_driver.start = scc_start;
        scc_driver.stop = scc_stop;
        
        scc_driver.put_char = scc_put_char;
        scc_driver.flush_chars = scc_flush_chars;        
	scc_driver.write_room = scc_write_room;
	scc_driver.chars_in_buffer = scc_chars_in_buffer;
	scc_driver.flush_buffer = scc_flush_buffer;
	
	scc_driver.throttle = scc_throttle;
	scc_driver.unthrottle = scc_unthrottle;
        
        scc_driver.ioctl = scc_tty_ioctl;
        scc_driver.set_termios = scc_set_termios;
        scc_driver.set_ldisc = scc_set_ldisc;

        
        if (tty_register_driver(&scc_driver))
        {
		printk(KERN_ERR "Failed to register Z8530 SCC driver\n");
		return -EIO;
	}
#endif
	
	memset(&SCC_ctrl, 0, sizeof(SCC_ctrl));
	
	/* pre-init channel information */
	
	for (chip = 0; chip < MAXSCC; chip++)
	{
		memset((char *) &SCC_Info[2*chip  ], 0, sizeof(struct scc_channel));
		memset((char *) &SCC_Info[2*chip+1], 0, sizeof(struct scc_channel));
		
		for (chan = 0; chan < 2; chan++)
			SCC_Info[2*chip+chan].magic    = SCC_MAGIC;
	}

	for (k = 0; k < 16; k++) Ivec[k].used = 0;

#ifdef CONFIG_SCC_DEV
	sprintf(devname,"%s0", SCC_DriverName);
	
	if (scc_net_setup(SCC_Info, devname))
		printk(KERN_WARNING "z8530drv: cannot setup network device\n");
#endif
	scc_net_procfs_init();

	return 0;
}

/* ******************************************************************** */
/* *			    Module support 			      * */
/* ******************************************************************** */


#ifdef MODULE
int init_module(void)
{
	int result = 0;
	
	result = scc_init();

	if (result == 0)
		printk(KERN_INFO "Copyright 1993,1996 Joerg Reuter DL1BKE (jreuter@lykos.tng.oche.de)\n");
		
	return result;
}

void cleanup_module(void)
{
	long flags;
	io_port ctrl;
	int k;
	struct scc_channel *scc;
	
	save_flags(flags); 
	cli();

#ifdef CONFIG_SCC_TTY
	if ( (k = tty_unregister_driver(&scc_driver)) )
	{
		printk(KERN_ERR "z8530drv: failed to unregister tty driver: (%d)\n", -k);
		restore_flags(flags);
		return;
	}
#endif

	if (Nchips == 0)
		unregister_netdev(SCC_Info[0].dev);

	for (k = 0; k < Nchips; k++)
		if ( (ctrl = SCC_ctrl[k].chan_A) )
		{
			Outb(ctrl, 0);
			OutReg(ctrl,R9,FHWRES);	/* force hardware reset */
			udelay(50);
		}
		
	for (k = 0; k < Nchips*2; k++)
	{
		scc = &SCC_Info[k];
		if (scc)
		{
			release_region(scc->ctrl, 1);
			release_region(scc->data, 1);
			if (scc->dev)
			{
				unregister_netdev(scc->dev);
				kfree(scc->dev);
			}
		}
	}
	
	for (k=0; k < 16 ; k++)
		if (Ivec[k].used) free_irq(k, NULL);
		
	restore_flags(flags);

	scc_net_procfs_remove();
}
#endif
