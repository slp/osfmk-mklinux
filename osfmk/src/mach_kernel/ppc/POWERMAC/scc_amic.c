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
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */

/*
 * Serial DMA functions for the AMIC (6100/7100/8100) class
 * of machines.
 */

#include	<scc.h>
#if	NSCC > 0

#include <mach_kgdb.h>

#include <mach_kdb.h>
#include <kgdb/gdb_defs.h>
#include <kgdb/kgdb_defs.h>     /* For kgdb_printf */

#include <kern/spl.h>
#include <mach/std_types.h>
#include <types.h>
#include <sys/syslog.h>
#include <device/io_req.h>
#include <device/tty.h>
#include <chips/busses.h>
#include <platforms.h>

#include <ppc/misc_protos.h>
#include <ppc/exception.h>

#include <ppc/POWERMAC/serial_io.h>
#include <ppc/POWERMAC/interrupts.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/scc_8530.h>

#define	SCC_DMA_IF		0x80
#define	SCC_DMA_RELOAD		0x40
#define	SCC_DMA_FROZEN		0x20
#define	SCC_DMA_PAUSE		0x10
#define	SCC_DMA_ENABLE_INT	0x08
#define	SCC_DMA_CONTINUE	0x04
#define	SCC_DMA_RUN		0x02
#define	SCC_DMA_RESET		0x01

#define	SCC_DMA_BUFF_SIZE	8192	/* Each DMA area is this big */

/* The SCC_RX_DMA_COUNT says hold many bytes to transfer before
 * an interrupt is generated.. However, the DMA engine is polled
 * for bytes at regular intervals.
 */

#define	SCC_RX_DMA_COUNT	1

/*
 * 6100/7100/8100 DMA Map (AMIC)
 */

struct scc_amic_regmap {
	unsigned char		*tx_dma_address;
	volatile unsigned char	*tx_control;
	volatile unsigned char	*tx_count0;
	volatile unsigned char	*tx_count1;
	volatile unsigned char	*tx_address0;
	volatile unsigned char	*tx_address1;
	volatile unsigned char	*tx_address2;
	volatile unsigned char	*tx_address3;
	unsigned int		prev_tx_index;

	unsigned char		*rx_dma_address;
	volatile unsigned char	*rx_control;
	volatile unsigned char	*rx_count0;
	volatile unsigned char	*rx_count1;
	volatile unsigned char	*rx_address0;
	volatile unsigned char	*rx_address1;
	volatile unsigned char	*rx_address2;
	volatile unsigned char	*rx_address3;
	unsigned int		rx_prev_index;
	int			rx_dma_count;
};

typedef struct scc_amic_regmap scc_amic_regmap_t;

scc_amic_regmap_t	scc_amic_regs[NSCC_LINE];

/* Prototypes.. */
static int scc_q_to_b( struct cirbuf *cb, unsigned char	*cp, int count);
void	scc_amic_rx_intr(int device, void *ssp);
void	scc_amic_tx_intr(int device, void *ssp);

static int scc_amic_scan(void);
void    scc_amic_init(int chan);
void    scc_amic_tx_pause(int chan);
void    scc_amic_rx_pause(int chan);
void    scc_amic_start_tx(int chan, struct tty *tp);
void    scc_amic_start_rx(int chan);
void    scc_amic_reset_rx(int chan);
void    scc_amic_setup_8530(int chan);
void	scc_amic_tx_continue(int unit);

struct scc_dma_ops scc_amic_ops = {
	scc_amic_init,
	scc_amic_setup_8530,
	scc_amic_start_rx,
	scc_amic_reset_rx,
	scc_amic_start_tx,
	scc_amic_tx_pause,	/* scc_amic_pause_tx not used */
	scc_amic_tx_continue	/* scc_amic_continue */
};

extern struct tty	scc_tty[NSCC_LINE];

#define	u_min(a,b)	((a) < (b) ? (a) : (b))

void
scc_amic_init(int channel)
{
	scc_amic_regmap_t	*dmap = &scc_amic_regs[channel];
	volatile unsigned char	*base_addr;


	if (powermac_info.dma_buffer_virt == 0) {
		return;
		}

	if (channel == 1) {
		base_addr = (unsigned char *)
			POWERMAC_IO(PDM_SCC_AMIC_BASE_PHYS +
				    PDM_SCC_AMIC_CHANNEL_B_OFFS);
	} else {
		base_addr = (unsigned char *)
			POWERMAC_IO(PDM_SCC_AMIC_BASE_PHYS +
				    PDM_SCC_AMIC_CHANNEL_A_OFFS);
	}

	dmap->tx_address3 = base_addr;
	dmap->tx_address2 = base_addr+1;
	dmap->tx_address1 = base_addr+2;
	dmap->tx_address0 = base_addr+3;
	dmap->tx_control  = base_addr+8;
	dmap->tx_count1   = base_addr+4; 
	dmap->tx_count0   = base_addr+5;

	dmap->rx_address3 = base_addr+0x10;
	dmap->rx_address2 = base_addr+0x11;
	dmap->rx_address1 = base_addr+0x12;
	dmap->rx_address0 = base_addr+0x13;
	dmap->rx_control  = base_addr+0x18;
	dmap->rx_count1   = base_addr+0x14; 
	dmap->rx_count0   = base_addr+0x15;

	dmap->rx_prev_index = 0;
	dmap->prev_tx_index = 0;

	*dmap->tx_control = SCC_DMA_RESET; eieio();
	*dmap->tx_control = 0; eieio();
	*dmap->rx_control = SCC_DMA_RESET; eieio();
	*dmap->rx_control = 0; eieio();

	while (*dmap->rx_control & SCC_DMA_RUN)
		eieio();

	if (powermac_info.class != POWERMAC_CLASS_PDM)
		panic("CLASS UNSUPPORTED FOR SCC_AMIC DMA\n");

	if (channel == 1) {
		dmap->tx_dma_address =
			(unsigned char *) powermac_info.dma_buffer_virt +
				PDM_DMA_BUFFER_SCC_B_TX_OFFSET;
		dmap->rx_dma_address =
			(unsigned char *) powermac_info.dma_buffer_virt +
				PDM_DMA_BUFFER_SCC_B_RX_OFFSET;

		/* Its Flip! TODO - Change it to the right way */
		pmac_register_int(PMAC_DMA_SCC_A_TX, SPLTTY, scc_amic_tx_intr);
		pmac_register_int(PMAC_DMA_SCC_A_RX, SPLTTY, scc_amic_rx_intr);
	} else {
		dmap->tx_dma_address =
			(unsigned char *) powermac_info.dma_buffer_virt +
				PDM_DMA_BUFFER_SCC_A_TX_OFFSET;
		dmap->rx_dma_address =
			(unsigned char *) powermac_info.dma_buffer_virt +
				PDM_DMA_BUFFER_SCC_A_RX_OFFSET;

		pmac_register_int(PMAC_DMA_SCC_B_TX, SPLTTY, scc_amic_tx_intr);
		pmac_register_int(PMAC_DMA_SCC_B_RX, SPLTTY, scc_amic_rx_intr);
	}

}

void
scc_amic_setup_8530(int chan)
{
	scc_softc_t	scc = &scc_softc[0];
	scc_regmap_t	regs = scc->regs;
	struct scc_softreg	*sr = &scc->softr[chan];


	scc_amic_start_rx(chan);
	sr->wr1 = SCC_WR1_DMA_MODE | SCC_WR1_DMA_RECV_DATA;
	scc_write_reg(regs,  chan,  1, sr->wr1);
	sr->wr1 |= SCC_WR1_DMA_ENABLE;
	scc_write_reg(regs,  chan,  1, sr->wr1);
	sr->wr1 |= SCC_WR1_RXI_SPECIAL_O;
	scc_write_reg(regs,  chan,  1, sr->wr1);

}

void
scc_amic_tx_pause(int chan)
{
	scc_softc_t	scc = &scc_softc[0];
	struct scc_softreg	*sr = &scc->softr[chan];
	scc_amic_regmap_t *dmap = &scc_amic_regs[chan];


	*dmap->tx_control = (*dmap->tx_control & ~SCC_DMA_IF) | SCC_DMA_PAUSE; eieio();

	while ((*dmap->tx_control & SCC_DMA_FROZEN) == 0)
		eieio();

	sr->dma_flags |= SCC_FLAGS_DMA_PAUSED;

}

void
scc_amic_tx_continue(int chan)
{
	scc_softc_t	scc = &scc_softc[0];
	struct scc_softreg	*sr = &scc->softr[chan];
	scc_amic_regmap_t *dmap = &scc_amic_regs[chan];

	*dmap->tx_control = (*dmap->tx_control & ~(SCC_DMA_IF|SCC_DMA_PAUSE)); eieio();
	sr->dma_flags &= ~SCC_FLAGS_DMA_PAUSED;
}


void
scc_amic_rx_pause(int chan)
{
	scc_amic_regmap_t *dmap = &scc_amic_regs[chan];


	*dmap->rx_control = SCC_DMA_PAUSE | SCC_DMA_ENABLE_INT |
		SCC_DMA_CONTINUE | SCC_DMA_RUN; eieio();

	while ((*dmap->rx_control & SCC_DMA_FROZEN) == 0)
		eieio();

}

void
scc_amic_reset_rx(int chan)
{
	scc_amic_regmap_t *dmap = &scc_amic_regs[chan];


	*dmap->rx_control = SCC_DMA_RESET; eieio();

	while (*dmap->rx_control & SCC_DMA_RUN)
		eieio();

	dmap->rx_prev_index = 0;

}

void
scc_amic_start_tx(int chan, struct tty *tp)
{
	scc_softc_t	scc = &scc_softc[0];
	struct scc_softreg	*sr = &scc->softr[chan];
	scc_amic_regmap_t	*dmap = &scc_amic_regs[chan];
	int			bytes;


	*dmap->tx_control = SCC_DMA_RESET; eieio();

	while (*dmap->tx_control & SCC_DMA_RUN)
		eieio();

	if (tp->t_outq.c_cc == 0) {
		return;
	}

	sr->dma_flags |= SCC_FLAGS_DMA_TX_BUSY;

	bytes = u_min(SCC_DMA_BUFF_SIZE, tp->t_outq.c_cc);
	bytes = scc_q_to_b(&tp->t_outq, dmap->tx_dma_address, bytes);

	*dmap->tx_count1 = (bytes >> 8) & 0x1f;
	*dmap->tx_count0 = bytes & 0xff; eieio();
	*dmap->tx_control =  SCC_DMA_ENABLE_INT | SCC_DMA_RUN; eieio();

}

/*
 * SCC private version of q_to_b which does not
 * use bcopy. (i.e. can be used on the uncached
 * DMA buffer)
 */

static int
scc_q_to_b(
	struct cirbuf		*cb,
	unsigned char		*cp,
	int			count)
{
	unsigned char *		ocp = cp;
	register int	i;


	while (count != 0) {
	    if (cb->c_cl == cb->c_cf)
		break;		/* empty */
	    if (cb->c_cl < cb->c_cf)
		i = cb->c_end - cb->c_cf;
	    else
		i = cb->c_cl - cb->c_cf;
	    if (i > count)
		i = count;
	    bcopy_nc((char *)cb->c_cf, (char *)cp, i);
	    cp += i;
	    count -= i;
	    cb->c_cf += i;
	    cb->c_cc -= i;
	    if (cb->c_cf == cb->c_end)
		cb->c_cf = cb->c_start;
	}


	return (cp - ocp);
}

		
void 
scc_amic_start_rx(int chan)
{
	scc_amic_regmap_t 	*dmap = &scc_amic_regs[chan];


	*dmap->rx_control = SCC_DMA_RESET; eieio();
	*dmap->rx_control = 0; eieio();

	*dmap->rx_count1 = (SCC_RX_DMA_COUNT>>8) & 0x1f;
	*dmap->rx_count0 = SCC_RX_DMA_COUNT & 0xff; eieio();
 	*dmap->rx_control = SCC_DMA_RUN | SCC_DMA_ENABLE_INT | SCC_DMA_CONTINUE; eieio();

	dmap->rx_dma_count = SCC_RX_DMA_COUNT;
	dmap->rx_prev_index = 0;

}

void
scc_amic_tx_intr(int device, void *ssp)
{
	int	chan;
	scc_softc_t		scc;
	struct scc_softreg	*sr;
	scc_amic_regmap_t	*dmap;
	struct tty		*tp;
	extern void		scc_start(struct tty *tp);


	chan = (device == PMAC_DMA_SCC_B_TX ? 0 : 1);
	scc = &scc_softc[0];
	sr = &scc->softr[chan];
	dmap = &scc_amic_regs[chan];
	tp = &scc_tty[chan];

	sr->dma_flags &= ~(SCC_FLAGS_DMA_PAUSED|SCC_FLAGS_DMA_TX_BUSY);

	if ((*dmap->tx_control & SCC_DMA_IF) == 0) {
		return;
	}

	*dmap->tx_control = 0;	/* Clear out everything */

	if ((tp->t_addr == 0) || /* not probed --> stray */
	    (tp->t_state & TS_TTSTOP)) {
		return;
	}

	if (tp->t_outq.c_cc && tp->t_state&TS_BUSY) {
		scc_amic_start_tx(chan, tp);
	} else {
		tp->t_state &= ~TS_BUSY;
		if (tp->t_state & TS_FLUSH)
			tp->t_state &= ~TS_FLUSH;

		scc_start(tp);
	}

	return;
}

void
scc_amic_rx_intr(int device, void *ssp)
{
	int	chan;
	scc_softc_t		scc;
	scc_amic_regmap_t	*dmap;
	int			index, bytes, copy_bytes, i, state;
	struct tty		*tp;
	unsigned char		*dma_address, *wrap_address;


	chan = (device == PMAC_DMA_SCC_B_RX ? 0 : 1);
	scc = &scc_softc[0];
	dmap = &scc_amic_regs[chan];

	tp = &scc_tty[chan];

	wrap_address = dmap->rx_dma_address + SCC_DMA_BUFF_SIZE;

	for (;;) {
		scc_amic_rx_pause(chan);

		index = ((*dmap->rx_address1 << 8) & 0x1f00) |  *dmap->rx_address0;

		if (index >= dmap->rx_prev_index)
			bytes = index - dmap->rx_prev_index;
		else
			bytes = (SCC_DMA_BUFF_SIZE - dmap->rx_prev_index) + index;

		dmap->rx_dma_count -= bytes;

		if (dmap->rx_dma_count <= 0) {
			scc_amic_rx_pause(chan);
			dmap->rx_dma_count = SCC_RX_DMA_COUNT;
			*dmap->rx_count1 = SCC_RX_DMA_COUNT >> 8;
			*dmap->rx_count0 = SCC_RX_DMA_COUNT & 0xff; eieio();
			*dmap->rx_control = SCC_DMA_IF | SCC_DMA_RUN | SCC_DMA_CONTINUE | SCC_DMA_ENABLE_INT; eieio();
		}

		*dmap->rx_control = SCC_DMA_IF | SCC_DMA_ENABLE_INT | SCC_DMA_CONTINUE
				| SCC_DMA_RUN; eieio();

		if (bytes == 0)
			break;

		if (tp->t_state & TS_ISOPEN) {
			dma_address = dmap->rx_dma_address + dmap->rx_prev_index;

			for (i = 0; i < bytes; i++, dma_address++)  {
				if (wrap_address == dma_address) 
					dma_address = dmap->rx_dma_address;

				/* Performance hack.. don't bother 
				 * to send the characters to the upper
				 * layers if it will be lost.
				 */
				if (tp->t_inq.c_cc >= tp->t_inq.c_hog) 
					break;

				ttyinput(*dma_address, tp);
			}
		} else {
			if (tp->t_state & TS_INIT)
				tt_open_wakeup(tp);
		}
	
		dmap->rx_prev_index = index;
	}

}

#endif	/* NSCC > 0 */
