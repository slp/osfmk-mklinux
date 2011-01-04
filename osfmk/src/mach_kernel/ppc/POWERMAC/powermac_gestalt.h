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

#ifndef _POWERMAC_GESTALT_H_
#define _POWERMAC_GESTALT_H_

/* Machine gestalt's */

enum {
	gestaltClassic				= 1,
	gestaltMacXL				= 2,
	gestaltMac512KE				= 3,
	gestaltMacPlus				= 4,
	gestaltMacSE				= 5,
	gestaltMacII				= 6,
	gestaltMacIIx				= 7,
	gestaltMacIIcx				= 8,
	gestaltMacSE030				= 9,
	gestaltPortable				= 10,
	gestaltMacIIci				= 11,
	gestaltPowerMac8100_120			= 12,
	gestaltMacIIfx				= 13,
	gestaltMacClassic			= 17,
	gestaltMacIIsi				= 18,
	gestaltMacLC				= 19,
	gestaltQuadra900			= 20,
	gestaltPowerBook170			= 21,
	gestaltQuadra700			= 22,
	gestaltClassicII			= 23,
	gestaltPowerBook100			= 24,
	gestaltPowerBook140			= 25,
	gestaltQuadra950			= 26,
	gestaltMacLCIII				= 27,
	gestaltPerforma450			= gestaltMacLCIII,
	gestaltPowerBookDuo210		= 29,
	gestaltMacCentris650		= 30,
	gestaltPowerBookDuo230		= 32,
	gestaltPowerBook180			= 33,
	gestaltPowerBook160			= 34,
	gestaltMacQuadra800			= 35,
	gestaltMacQuadra650			= 36,
	gestaltMacLCII				= 37,
	gestaltPowerBookDuo250		= 38,
	gestaltAWS9150_80			= 39,
	gestaltPowerMac8100_110		= 40,
	gestaltAWS8150_110			= gestaltPowerMac8100_110,
	gestaltPowerMac5200			= 41,
	gestaltPowerMac6200			= 42,
	gestaltMacIIvi				= 44,
	gestaltMacIIvm				= 45,
	gestaltPerforma600			= gestaltMacIIvm,
	gestaltPowerMac7100_80		= 47,
	gestaltMacIIvx				= 48,
	gestaltMacColorClassic		= 49,
	gestaltPerforma250			= gestaltMacColorClassic,
	gestaltPowerBook165c		= 50,
	gestaltMacCentris610		= 52,
	gestaltMacQuadra610			= 53,
	gestaltPowerBook145			= 54,
	gestaltPowerMac8100_100		= 55,
	gestaltMacLC520				= 56,
	gestaltAWS9150_120			= 57,
	gestaltMacCentris660AV		= 60,
	gestaltPerforma46x			= 62,
	gestaltPowerMac8100_80		= 65,
	gestaltAWS8150_80			= gestaltPowerMac8100_80,
	gestaltPowerMac9500			= 67,
	gestaltPowerMac7500			= 68,
	gestaltPowerMac8500			= 69,
	gestaltPowerBook180c		= 71,
	gestaltPowerBook520			= 72,
	gestaltPowerBook520c		= gestaltPowerBook520,
	gestaltPowerBook540			= gestaltPowerBook520,
	gestaltPowerBook540c		= gestaltPowerBook520,
	gestaltPowerMac6100_60		= 75,
	gestaltAWS6150_60			= gestaltPowerMac6100_60,
	gestaltPowerBookDuo270c		= 77,
	gestaltMacQuadra840AV		= 78,
	gestaltPerforma550			= 80,
	gestaltPowerBook165			= 84,
	gestaltPowerBook190			= 85,
	gestaltMacTV				= 88,
	gestaltMacLC475				= 89,
	gestaltPerforma47x			= gestaltMacLC475,
	gestaltMacLC575				= 92,
	gestaltMacQuadra605			= 94,
	gestaltQuadra630			= 98,
	gestaltPowerMac6100_66		= 100,
	gestaltAWS6150_66			= gestaltPowerMac6100_66,
	gestaltPowerMac6100_80		= 101,	/* UNDOCUMENTED FEATURE! */
	gestaltPowerBookDuo280		= 102,
	gestaltPowerBookDuo280c		= 103,
	gestaltPowerMac7200			= 108,
	gestaltPowerMac7100_66		= 112,	/* Power Macintosh 7100/66 */
	gestaltPowerMac7100_80_chipped  = 113,  /* Clock chipped 7100/66 */
	gestaltPowerBook150			= 115,
	gestaltPowerBookDuo2300		= 124,
	gestaltPowerBook500PPCUpgrade = 126,
	gestaltPowerBook5300		= 128,
	gestaltPowerBook3400            = 306,
	gestaltPowerBook2400            = 307,
	gestaltPowerMac4400		= 514
};

#endif /* !defined _POWERMAC_GESTALT_H_ */
