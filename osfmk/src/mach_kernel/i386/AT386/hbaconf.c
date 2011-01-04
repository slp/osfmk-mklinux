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
#include <types.h>
#include <sys/sdi.h>
#include <device/hbaconf.h>

#define BLC_CNTLS		0	/* number of Buslogic cntlr's */



#if BLC_CNTLS

#define BLC_0			0		/* cntlr */
#define BLC_0_SIOA		0x330	/* ioaddr1 */
#define BLC_0_CHAN		5		/* dmachan1 */
#define BLC_0_VECT		11		/* iov */

#include "blc_space.c"

extern void blcintr(int vect);

extern struct modwrapper	blc_wrapper;

struct intr_info	blc_intr_info [] = {
	/*	ivect_no,		int_pri,	itype	*/
	{	BLC_0_VECT,		5,			0	},
	{	0,				0,			0	}
};

struct o_mod_drvintr	blc_attach_info = {
	blc_intr_info,								/* drv_intrinfo */
	blcintr										/* ihndler */
};

/* unload delay time */
int blc_conf_data = { 0 };

#endif

#include <adsl.h>
#if NADSL>0
#define ADSL_CNTLS		NADSL	/* number of Adaptec 78xx cntlr's */

#define ADSL_0			0		/* cntlr */
#define ADSL_0_SIOA		0x330	/* ioaddr1 */
#define ADSL_0_CHAN		5		/* dmachan1 */
#define ADSL_0_VECT		1		/* iov, must be nonzero */

#include "adsl_space.c"

extern void adslintr(int vect);

extern struct modwrapper	adsl_wrapper;

struct intr_info	adsl_intr_info [] = {
	/*	ivect_no,		int_pri,	itype	*/
	{	ADSL_0_VECT,	5,			0	},
	{	0,				0,			0	}
};

struct o_mod_drvintr	adsl_attach_info = {
	adsl_intr_info,								/* drv_intrinfo */
	adslintr									/* ihndler */
};

/* unload delay time */
int adsl_conf_data = { 0 };

#endif




/* table of all HBA drivers */
struct hba_driver_s	hba_driver_tab [] = {
/*	{	mwp,				probed }, */
#if BLC_CNTLS
	{	&blc_wrapper,		0 },
#endif
#if ADSL_CNTLS
	{	&adsl_wrapper,		0 },
#endif
};

int		num_hba_driver = {
	sizeof (hba_driver_tab) / sizeof (struct hba_driver_s)
};
