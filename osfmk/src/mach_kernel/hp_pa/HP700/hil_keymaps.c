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
 * Copyright (c) 1990-1994, The University of Utah and
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
 *	Utah $Hdr: hil_keymaps.c 1.6 94/12/14$
 */

/*
 * Keymaps for various HP-HIL keyboard layouts.
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
#define	NULL	0
#include <hp_pa/HP700/kbdmap.h>

char	us_keymap[] = {
	NULL,	'`',	'\\',	ESC,	NULL,	DEL,	NULL,	NULL,  
	'\n',	'\t',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,  
	NULL,	'\n',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,  
	NULL,	'\t',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	'\b',	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	ESC,	'\r',	NULL,	'\n',	'0',	'.',	',',	'+',
	'1',	'2',	'3',	'-',	'4',	'5',	'6',	'*',
	'7',	'8',	'9',	'/',	'E',	'(',	')',	'^',
	'1',	'2',	'3',	'4',	'5',	'6',	'7',	'8',
	'9',	'0',	'-',	'=',	'[',	']',	';',	'\'',
	',',	'.',	'/',	'\040',	'o',	'p',	'k',	'l',
	'q',	'w',	'e',	'r',	't',	'y',	'u',	'i',
	'a',	's',	'd',	'f',	'g',	'h',	'j',	'm',
	'z',	'x',	'c',	'v',	'b',	'n',	NULL,	NULL
};

char	us_shiftmap[] = {
	NULL,	'~',	'|',	DEL,	NULL,	DEL,	NULL,	NULL,
	'\n',	'\t',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	'\n',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	'\t',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	DEL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	ESC,	'\r',	NULL,	'\n',	'0',	'.',	',',	'+',
	'1',	'2',	'3',	'-',	'4',	'5',	'6',	'*',
	'7',	'8',	'9',	'/',	'`',	'|',	'\\',	'~',
	'!',	'@',	'#',	'$',	'%',	'^',	'&',	'*',
	'(',	')',	'_',	'+',	'{',	'}',	':',	'\"',
	'<',	'>',	'?',	'\040',	'O',	'P',	'K',	'L',
	'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
	'A',	'S',	'D',	'F',	'G',	'H',	'J',	'M',
	'Z',	'X',	'C',	'V',	'B',	'N',	NULL,	NULL
};

char	us_ctrlmap[] = {
	NULL,	'`',	'\034',	ESC,	NULL,	DEL,	NULL,	NULL,
	'\n',	'\t',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	'\n',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	'\t',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	'\b',	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	ESC,	'\r',	NULL,	'\n',	'0',	'.',	',',	'+',
	'1',	'2',	'3',	'-',	'4',	'5',	'6',	'*',
	'7',	'8',	'9',	'/',	'E',	'(',	')',	'\036',
	'1',	'2',	'3',	'4',	'5',	'6',	'7',	'8',
	'9',	'0',	'-',	'=',	'\033',	'\035',	';',	'\'',
	',',	'.',	'/',	'\040',	'\017',	'\020',	'\013',	'\014',
	'\021',	'\027',	'\005',	'\022',	'\024',	'\031',	'\025',	'\011',
	'\001',	'\023',	'\004',	'\006',	'\007',	'\010',	'\012',	'\015',
	'\032',	'\030',	'\003',	'\026',	'\002',	'\016',	NULL,	NULL
};

char	us_ctrlshiftmap[] = {
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
	'\001',	'\023',	'\004',	'\006',	'\007',	'\010',	'\012',	'\015',
	'\032',	'\030',	'\003',	'\026',	'\002',	'\016',	NULL,	NULL
};

char	*us_stringmap[] = {
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	"\033V","\033h",
	"\033U",NULL,	NULL,	NULL,	NULL,	NULL,	"\033K","\033J",
	NULL,	NULL,	NULL,	"\033p","\033q","\033t","\033u","\033v",
	"\033r","\033s","\033B","\033A","\033w",NULL,	"\033D","\033C",
	"\033L","\033M",NULL,	NULL,	"\033P",NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL
};

#ifdef UK_KEYBOARD
char	uk_keymap[] = {
	NULL,	'`',	'<',	ESC,	NULL,	DEL,	NULL,	NULL,  
	'\n',	'\t',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,  
	NULL,	'\n',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,  
	NULL,	'\t',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	'\b',	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	ESC,	'\r',	NULL,	'\n',	'0',	'.',	',',	'+',
	'1',	'2',	'3',	'-',	'4',	'5',	'6',	'*',
	'7',	'8',	'9',	'/',	'E',	'(',	')',	'^',
	'1',	'2',	'3',	'4',	'5',	'6',	'7',	'8',
	'9',	'0',	'+',	'\'',	'[',	']',	'*',	'\\',
	',',	'.',	'-',	'\040',	'o',	'p',	'k',	'l',
	'q',	'w',	'e',	'r',	't',	'y',	'u',	'i',
	'a',	's',	'd',	'f',	'g',	'h',	'j',	'm',
	'z',	'x',	'c',	'v',	'b',	'n',	NULL,	NULL
};

char	uk_shiftmap[] = {
	NULL,	'~',	'>',	DEL,	NULL,	DEL,	NULL,	NULL,
	'\n',	'\t',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	'\n',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	'\t',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	DEL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	ESC,	'\r',	NULL,	'\n',	'0',	'.',	',',	'+',
	'1',	'2',	'3',	'-',	'4',	'5',	'6',	'*',
	'7',	'8',	'9',	'/',	'`',	'|',	'\\',	'~',
	'!',	'\"',	'#',	'$',	'%',	'&',	'^',	'(',
	')',	'=',	'?',	'/',	'{',	'}',	'@',	'|',
	';',	':',	'_',	'\040',	'O',	'P',	'K',	'L',
	'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
	'A',	'S',	'D',	'F',	'G',	'H',	'J',	'M',
	'Z',	'X',	'C',	'V',	'B',	'N',	NULL,	NULL
};

char	uk_ctrlmap[] = {
	NULL,	'`',	'<',	ESC,	NULL,	DEL,	NULL,	NULL,
	'\n',	'\t',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	'\n',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	'\t',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	'\b',	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	ESC,	'\r',	NULL,	'\n',	'0',	'.',	',',	'+',
	'1',	'2',	'3',	'-',	'4',	'5',	'6',	'*',
	'7',	'8',	'9',	'/',	'E',	'(',	')',	'\036',
	'1',	'2',	'3',	'4',	'5',	'6',	'7',	'8',
	'9',	'0',	'+',	'\'',	'\033',	'\035',	'*',	'\034',
	',',	'.',	'/',	'\040',	'\017',	'\020',	'\013',	'\014',
	'\021',	'\027',	'\005',	'\022',	'\024',	'\031',	'\025',	'\011',
	'\001',	'\023',	'\004',	'\006',	'\007',	'\010',	'\012',	'\015',
	'\032',	'\030',	'\003',	'\026',	'\002',	'\016',	NULL,	NULL
};

char	uk_ctrlshiftmap[] = {
	NULL,	'~',	'>',	DEL,	NULL,	DEL,	NULL,	NULL,
	'\n',	'\t',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	'\n',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	'\t',	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	DEL,	NULL,
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	ESC,	'\r',	NULL,	'\n',	'0',	'.',	',',	'+',
	'1',	'2',	'3',	'-',	'4',	'5',	'6',	'*',
	'7',	'8',	'9',	'/',	'`',	'|',	'\034',	'~',
	'!',	'\"',	'#',	'$',	'%',	'&',	'\036',	'(',
	')',	'=',	'?',	'/',	'{',	'}',	'\000',	'|',
	';',	':',	'\037',	'\040',	'\017',	'\020',	'\013',	'\014',
	'\021',	'\027',	'\005',	'\022',	'\024',	'\031',	'\025',	'\011',
	'\001',	'\023',	'\004',	'\006',	'\007',	'\010',	'\012',	'\015',
	'\032',	'\030',	'\003',	'\026',	'\002',	'\016',	NULL,	NULL
};
#endif

/*
 * The keyboard map table.
 * Lookup is by hardware returned language code.
 */
struct kbdmap hkbd_map[] = {
	KBD_US,		"US ASCII",
	us_keymap,	us_shiftmap,	us_ctrlmap,	us_ctrlshiftmap,
	us_stringmap,

#ifdef UK_KEYBOARD
	KBD_UK,		"United Kingdom",
	uk_keymap,	uk_shiftmap,	uk_ctrlmap,	uk_ctrlshiftmap,
	us_stringmap,
#endif

	0,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,
};
