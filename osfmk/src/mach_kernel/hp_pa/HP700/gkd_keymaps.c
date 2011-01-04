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
 *	Utah $Hdr: gkd_keymaps.c 1.4 94/12/14$
 */

/*
 * Keymaps for various HP-PS2 (Gecko) keyboard layouts.
 * These tables apply only to keyboards in "cooked" mode.
 * Currently only one is supported as an ITE keyboard.
 *
 * Maps are indexed by cooked keycode and contain the ASCII
 * character for that keycode.  The map-set used depends on the
 * keyboard "language".  The map used within that set depends on
 * the shift/control status that is returned by the hardware along
 * with the keycode.  If an entry is NULL for a key in the appropriate
 * unshifted, shifted, control, or control-shifted table, then a
 * single "string" table is consulted.  In this fashion, a multi-
 * character sequence can be returned for a key press.  Note that
 * control/shift status have no effect on multi-character lookup
 * (i.e. there is only one string table per set, not four).
 *
 * Someday we could allow user-definable keymaps, but we would have
 * to come up with a better format (at least externally).  This
 * format takes up lots of space.  Having keymaps for all 18 or so
 * HP supported layouts would be bad news.
 */
#include <hp_pa/HP700/kbdmap.h>

#define NULL 0

char	gkd_us_keymap[] = {
	NULL,	NULL,	NULL,	ESC,	NULL,	DEL,	NULL,	NULL,  
	NULL,	NULL,	NULL,	NULL,	NULL,	'\t',	'`',	NULL,  
	NULL,	NULL,	NULL,	NULL,	NULL,	'q',	'1',	NULL,  
	NULL,	NULL,	'z',	's',	'a',	'w',	'2',	NULL,
	NULL,	'c',	'x',	'd',	'e',	'4',	'3',	NULL,
	NULL,	' ',	'v',	'f',	't',	'r',	'5',	NULL,
	NULL,	'n',	'b',	'h',	'g',	'y',	'6',	NULL,
	ESC,	NULL,	'm',	'j',	'u',	'7',	'8',	NULL,
	NULL,	',',	'k',	'i',	'o',	'0',	'9',	NULL,
	NULL,	'.',	'/',	'l',	';',	'p',	'-',	NULL,
	NULL,	NULL,	'\'',	NULL,	'[',	'=',	NULL,	NULL,
	NULL,	NULL,	'\n',	']',	NULL,	'\\',	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	'\b',	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	DEL,	NULL,	'5',	NULL,	NULL,	ESC,	NULL,
	NULL,	'+',	NULL,	'-',	'*',	NULL,	NULL,	NULL,
	NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL
};

char	gkd_us_shiftmap[] = {
	NULL,	NULL,	NULL,	ESC,	NULL,	DEL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	'\t',	'~',	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	'Q',	'!',	NULL,  
	NULL,	NULL,	'Z',	'S',	'A',	'W',	'@',	NULL,
	NULL,	'C',	'X',	'D',	'E',	'$',	'#',	NULL,
	NULL,	' ',	'V',	'F',	'T',	'R',	'%',	NULL,
	NULL,	'N',	'B',	'H',	'G',	'Y',	'^',	NULL,
	ESC,	NULL,	'M',	'J',	'U',	'&',	'*',	NULL,
	NULL,	'<',	'K',	'I',	'O',	')',	'(',	NULL,
	NULL,	'>',	'?',	'L',	':',	'P',	'_',	NULL,
	NULL,	NULL,	'"',	NULL,	'{',	'+',	NULL,	NULL,
	NULL,	NULL,	'\n',	'}',	NULL,	'|',	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	'\b',	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	DEL,	NULL,	'5',	NULL,	NULL,	ESC,	NULL,
	NULL,	'+',	NULL,	'-',	'*',	NULL,	NULL,	NULL,
	NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL
};

char	gkd_us_ctrlmap[] = {
	NULL,	NULL,	NULL,	ESC,	NULL,	DEL,	NULL,	NULL,  
	NULL,	NULL,	NULL,	NULL,	NULL,	'\t',	'`',	NULL,  
	NULL,	NULL,	NULL,	NULL,	NULL,	'\021',	'1',	NULL,  
	NULL,	NULL,	'\032',	'\023',	'\001',	'\027',	'2',	NULL,
	NULL,	'\003',	'\030',	'\004',	'\005',	'4',	'3',	NULL,
	NULL,	' ',	'\026',	'\006',	'\024',	'\022',	'5',	NULL,
	NULL,	'\016',	'\002',	'\010',	'\007',	'\031',	'6',	NULL,
	ESC,	NULL,	'\015',	'\012',	'\025',	'7',	'8',	NULL,
	NULL,	',',	'\013',	'\011',	'\017',	'0',	'9',	NULL,
	NULL,	'.',	'/',	'\014',	';',	'\020',	'-',	NULL,
	NULL,	NULL,	'\'',	NULL,	'\033',	'=',	NULL, 	NULL,
	NULL,	NULL,	'\n',	'\035',	NULL,	'\034',	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	'\b',	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	DEL,	NULL,	'5',	NULL,	NULL,	ESC,	NULL,
	NULL,	'+',	NULL,	'-',	'*',	NULL,	NULL,	NULL,
	NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL
};

char	gkd_us_ctrlshiftmap[] = {
	NULL,	'~',	'|',	DEL,	NULL,	DEL,	NULL,	NULL,
	'\n',	'\t',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	'\n',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	'\t',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	DEL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	ESC,	'\r',	NULL,	'\n',	'0',	'.',	',',	'+',
	'1',	'2',	'3',	'-',	'4',	'5',	'6',	'*',
	'7',	'8',	'9',	'/',	'`',	'|',	'\034',	'~',
	'!',	'\000',	'#',	'$',	'%',	'\036',	'&',	'*',
	'(',	')',	'\037',	'+',	'{',	'}',	':',	'\"',
	'<',	'>',	'?',	'\040',	'\017',	'\020',	'\013',	'\014',
	'\021',	'\027',	'\005',	'\022',	'\024',	'\031',	'\025',	'\011',
	'\001',	'\023',	'\004',	'5',	'\007',	'\010',	'\012',	'\015',
	'\032',	'\030',	'\003',	'\026',	'\002',	'\016',	NULL,	NULL,
	NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL
};

char	*gkd_us_stringmap[] = {
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	"\033[D", NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	"\033[B", NULL,	"\033[C", "\033[A", NULL, NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL
};

/*
 * The keyboard map table.
 * Lookup is by hardware returned language code.
 *
 * XXX someday we need to come up with a better integration between
 *     hil & gecko drivers 
 */
struct kbdmap gkbd_map[] = {
	KBD_US,		"US ASCII",
	gkd_us_keymap,	gkd_us_shiftmap, gkd_us_ctrlmap, gkd_us_ctrlshiftmap,
	gkd_us_stringmap,

	0,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,
};
