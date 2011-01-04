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
/*	$NetBSD: keyboard.h,v 1.4 1994/12/03 23:34:32 briggs Exp $	*/

/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/* ADB Keyboard Status - ADB Register 2 */

#define	ADBKS_LED_NUMLOCK		0x0001
#define	ADBKS_LED_CAPSLOCK		0x0002
#define	ADBKS_LED_SCROLLLOCK		0x0004
#define	ADBKS_SCROLL_LOCK		0x0040
#define	ADBKS_NUMLOCK			0x0080
/* Bits 3 to 5 are reserved */
#define	ADBKS_APPLE_CMD			0x0100
#define	ADBKS_OPTION			0x0200
#define	ADBKS_SHIFT			0x0400
#define	ADBKS_CONTROL			0x0800
#define	ADBKS_CAPSLOCK			0x1000
#define	ADBKS_RESET			0x2000
#define	ADBKS_DELETE			0x4000
/* bit 16 is reserved */

/* 
 * Special key values
 */

#define ADBK_LEFT	0x3B
#define ADBK_RIGHT	0x3C
#define ADBK_UP		0x3E
#define ADBK_DOWN	0x3D
#define ADBK_PGUP	0x74
#define ADBK_PGDN	0x79
#define ADBK_HOME	0x73
#define ADBK_END	0x77
#define ADBK_CONTROL	0x36
#define ADBK_CONTROL_R  0x7D
#define ADBK_FLOWER	0x37
#define ADBK_SHIFT	0x38
#define ADBK_SHIFT_R    0x7B
#define ADBK_CAPSLOCK	0x39
#define ADBK_OPTION	0x3A
#define ADBK_OPTION_R   0x7C
#define	ADBK_NUMLOCK	0x47
#define ADBK_SPACE	0x31
#define ADBK_F		0x03
#define ADBK_O		0x1F
#define ADBK_P		0x23
#define ADBK_Q		0x0C
#define ADBK_V		0x09
#define ADBK_1		0x12
#define ADBK_2		0x13
#define ADBK_3		0x14
#define ADBK_4		0x15
#define ADBK_5		0x17
#define ADBK_6		0x16
#define ADBK_7		0x1A
#define ADBK_8		0x1C
#define ADBK_9		0x19
#define ADBK_0		0x1D
#define	ADBK_POWER	0x7f	/* actual 0x7f 0x7f */

#define ADBK_KEYVAL(key)	((key) & 0x7f)
#define ADBK_PRESS(key)		(((key) & 0x80) == 0)
#define ADBK_KEYDOWN(key)	(key)
#define ADBK_KEYUP(key)		((key) | 0x80)
#define ADBK_MODIFIER(key)	((((key) & 0x7f) == ADBK_SHIFT) || \
				 (((key) & 0x7f) == ADBK_SHIFT_R) || \
				 (((key) & 0x7f) == ADBK_CONTROL) || \
				 (((key) & 0x7f) == ADBK_CONTROL_R) || \
				 (((key) & 0x7f) == ADBK_FLOWER) || \
				 (((key) & 0x7f) == ADBK_OPTION) || \
				 (((key) & 0x7f) == ADBK_OPTION_R) || \
				 (((key) & 0x7f) == ADBK_NUMLOCK) || \
				 (((key) & 0x7f) == ADBK_CAPSLOCK))

/* Michel Pollet: small support for different keyboard layout */
#define ADB_KEYMAP_US		0
#define ADB_KEYMAP_FRENCH	1
#define ADB_KEYMAP_GERMAN       2
#define ADB_KEYMAP_MAX		3

typedef unsigned char	adb_keymap_t[128][2];

adb_keymap_t keyboard_french = {
		/* Scan code      Normal     Shifted */
	{	/*   0x00, */       'q',       'Q' },//
	{	/*   0x01, */       's',       'S' },
	{	/*   0x02, */       'd',       'D' },
	{	/*   0x03, */       'f',       'F' },
	{	/*   0x04, */       'h',       'H' },
	{	/*   0x05, */       'g',       'G' },
	{	/*   0x06, */       'w',       'W' },//
	{	/*   0x07, */       'x',       'X' },
	{	/*   0x08, */       'c',       'C' },
	{	/*   0x09, */       'v',       'V' },
	{	/*   0x0A, */      0xff,      0xff },
	{	/*   0x0B, */       'b',       'B' },
	{	/*   0x0C, */       'a',       'A' },//
	{	/*   0x0D, */       'z',       'Z' },//
	{	/*   0x0E, */       'e',       'E' },
	{	/*   0x0F, */       'r',       'R' },
	{	/*   0x10, */       'y',       'Y' },
	{	/*   0x11, */       't',       'T' },
	{	/*   0x12, */       '1',       '&' },//
	{	/*   0x13, */       '2',       '{' },//
	{	/*   0x14, */       '3',       '"' },//
	{	/*   0x15, */       '4',       '\'' },//
	{	/*   0x16, */       '6',       '6' },//
	{	/*   0x17, */       '5',       '(' },//
	{	/*   0x18, */       '-',       '_' },//
	{	/*   0x19, */       '9',       'c' },//
	{	/*   0x1A, */       '7',       '}' },//
	{	/*   0x1B, */       ')',       ')' },//
	{	/*   0x1C, */       '8',       '!' },//
	{	/*   0x1D, */       '0',       '@' },//
	{	/*   0x1E, */       '$',       '*' },//
	{	/*   0x1F, */       'o',       'O' },
	{	/*   0x20, */       'u',       'U' },
	{	/*   0x21, */       '^',       '^' },
	{	/*   0x22, */       'i',       'I' },
	{	/*   0x23, */       'p',       'P' },
	{	/*   0x24, */      0x0D,      0x0D },
	{	/*   0x25, */       'l',       'L' },
	{	/*   0x26, */       'j',       'J' },
	{	/*   0x27, */	    'u',       '%' },//
	{	/*   0x28, */       'k',       'K' },
	{	/*   0x29, */       'm',       'M' },//
	{	/*   0x2A, */      	'`',       '#' },//
	{	/*   0x2B, */       ';',       '.' },//
	{	/*   0x2C, */       '=',       '+' },//
	{	/*   0x2D, */       'n',       'N' },
	{	/*   0x2E, */       ',',       '?' },//
	{	/*   0x2F, */       ':',       '/' },//
	{	/*   0x30, */      0x09,      0x09 },
	{	/*   0x31, */       ' ',       ' ' },
	{	/*   0x32, */       '<',       '>' },
	{	/*   0x33, */      0x7F,      0x7F }, /* Delete */
	{	/*   0x34, */      0xff,      0xff },
	{	/*   0x35, */      0x1B,      0x1B },
	{	/*   0x36, */      0xff,      0xff },
	{	/*   0x37, */      0xff,      0xff },
	{	/*   0x38, */      0xff,      0xff },
	{	/*   0x39, */      0xff,      0xff },
	{	/*   0x3A, */      0xff,      0xff },
	{	/*   0x3B, */       'h',      0xff },
	{	/*   0x3C, */       'l',      0xff },
	{	/*   0x3D, */       'j',      0xff },
	{	/*   0x3E, */       'k',      0xff },
	{	/*   0x3F, */      0xff,      0xff },
	{	/*   0x40, */      0xff,      0xff },
	{	/*   0x41, */       '.',       '.' },
	{	/*   0x42, */      0xff,      0xff },
	{	/*   0x43, */       '*',       '*' },
	{	/*   0x44, */      0xff,      0xff },
	{	/*   0x45, */       '+',       '+' },
	{	/*   0x46, */      0xff,      0xff },
	{	/*   0x47, */      0xff,      0xff },
	{	/*   0x48, */      0xff,      0xff },
	{	/*   0x49, */      0xff,      0xff },
	{	/*   0x4A, */      0xff,      0xff },
	{	/*   0x4B, */       '/',       '/' },
	{	/*   0x4C, */      0x0D,      0x0D },
	{	/*   0x4D, */      0xff,      0xff },
	{	/*   0x4E, */       '-',       '-' },
	{	/*   0x4F, */      0xff,      0xff },
	{	/*   0x50, */      0xff,      0xff },
	{	/*   0x51, */       '=',       '=' },
	{	/*   0x52, */       '0',       '0' },
	{	/*   0x53, */       '1',       '1' },
	{	/*   0x54, */       '2',       '2' },
	{	/*   0x55, */       '3',       '3' },
	{	/*   0x56, */       '4',       '4' },
	{	/*   0x57, */       '5',       '5' },
	{	/*   0x58, */       '6',       '6' },
	{	/*   0x59, */       '7',       '7' },
	{	/*   0x5A, */      0xff,      0xff },
	{	/*   0x5B, */       '8',       '8' },
	{	/*   0x5C, */       '9',       '9' },
	{	/*   0x5D, */      0xff,      0xff },
	{	/*   0x5E, */      0xff,      0xff },
	{	/*   0x5F, */      0xff,      0xff },
	{	/*   0x60, */      0xff,      0xff },
	{	/*   0x61, */      0xff,      0xff },
	{	/*   0x62, */      0xff,      0xff },
	{	/*   0x63, */      0xff,      0xff },
	{	/*   0x64, */      0xff,      0xff },
	{	/*   0x65, */      0xff,      0xff },
	{	/*   0x66, */      0xff,      0xff },
	{	/*   0x67, */      0xff,      0xff },
	{	/*   0x68, */      0xff,      0xff },
	{	/*   0x69, */      0xff,      0xff },
	{	/*   0x6A, */      0xff,      0xff },
	{	/*   0x6B, */      0xff,      0xff },
	{	/*   0x6C, */      0xff,      0xff },
	{	/*   0x6D, */      0xff,      0xff },
	{	/*   0x6E, */      0xff,      0xff },
	{	/*   0x6F, */      0xff,      0xff },
	{	/*   0x70, */      0xff,      0xff },
	{	/*   0x71, */      0xff,      0xff },
	{	/*   0x72, */      0xff,      0xff },
	{	/*   0x73, */      0xff,      0xff },
	{	/*   0x74, */      0xff,      0xff },
	{	/*   0x75, */      0xff,      0xff },
	{	/*   0x76, */      0xff,      0xff },
	{	/*   0x77, */      0xff,      0xff },
	{	/*   0x78, */      0xff,      0xff },
	{	/*   0x79, */      0xff,      0xff },
	{	/*   0x7A, */      0xff,      0xff },
	{	/*   0x7B, */      0xff,      0xff },
	{	/*   0x7C, */      0xff,      0xff },
	{	/*   0x7D, */      0xff,      0xff },
	{	/*   0x7E, */      0xff,      0xff },
	{	/*   0x7F, */      0xff,      0xff }
};

adb_keymap_t keyboard_us = {
		/* Scan code      Normal     Shifted */
	{	/*   0x00, */       'a',       'A' },
	{	/*   0x01, */       's',       'S' },
	{	/*   0x02, */       'd',       'D' },
	{	/*   0x03, */       'f',       'F' },
	{	/*   0x04, */       'h',       'H' },
	{	/*   0x05, */       'g',       'G' },
	{	/*   0x06, */       'z',       'Z' },
	{	/*   0x07, */       'x',       'X' },
	{	/*   0x08, */       'c',       'C' },
	{	/*   0x09, */       'v',       'V' },
	{	/*   0x0A, */      0xff,      0xff },
	{	/*   0x0B, */       'b',       'B' },
	{	/*   0x0C, */       'q',       'Q' },
	{	/*   0x0D, */       'w',       'W' },
	{	/*   0x0E, */       'e',       'E' },
	{	/*   0x0F, */       'r',       'R' },
	{	/*   0x10, */       'y',       'Y' },
	{	/*   0x11, */       't',       'T' },
	{	/*   0x12, */       '1',       '!' },
	{	/*   0x13, */       '2',       '@' },
	{	/*   0x14, */       '3',       '#' },
	{	/*   0x15, */       '4',       '$' },
	{	/*   0x16, */       '6',       '^' },
	{	/*   0x17, */       '5',       '%' },
	{	/*   0x18, */       '=',       '+' },
	{	/*   0x19, */       '9',       '(' },
	{	/*   0x1A, */       '7',       '&' },
	{	/*   0x1B, */       '-',       '_' },
	{	/*   0x1C, */       '8',       '*' },
	{	/*   0x1D, */       '0',       ')' },
	{	/*   0x1E, */       ']',       '}' },
	{	/*   0x1F, */       'o',       'O' },
	{	/*   0x20, */       'u',       'U' },
	{	/*   0x21, */       '[',       '{' },
	{	/*   0x22, */       'i',       'I' },
	{	/*   0x23, */       'p',       'P' },
	{	/*   0x24, */      0x0D,      0x0D },
	{	/*   0x25, */       'l',       'L' },
	{	/*   0x26, */       'j',       'J' },
	{	/*   0x27, */      '\'',       '"' },
	{	/*   0x28, */       'k',       'K' },
	{	/*   0x29, */       ';',       ':' },
	{	/*   0x2A, */      '\\',       '|' },
	{	/*   0x2B, */       ',',       '<' },
	{	/*   0x2C, */       '/',       '?' },
	{	/*   0x2D, */       'n',       'N' },
	{	/*   0x2E, */       'm',       'M' },
	{	/*   0x2F, */       '.',       '>' },
	{	/*   0x30, */      0x09,      0x09 },
	{	/*   0x31, */       ' ',       ' ' },
	{	/*   0x32, */       '`',       '~' },
	{	/*   0x33, */      0x7F,      0x7F }, /* Delete */
	{	/*   0x34, */      0xff,      0xff },
	{	/*   0x35, */      0x1B,      0x1B },
	{	/*   0x36, */      0xff,      0xff },
	{	/*   0x37, */      0xff,      0xff },
	{	/*   0x38, */      0xff,      0xff },
	{	/*   0x39, */      0xff,      0xff },
	{	/*   0x3A, */      0xff,      0xff },
	{	/*   0x3B, */       'h',      0xff },
	{	/*   0x3C, */       'l',      0xff },
	{	/*   0x3D, */       'j',      0xff },
	{	/*   0x3E, */       'k',      0xff },
	{	/*   0x3F, */      0xff,      0xff },
	{	/*   0x40, */      0xff,      0xff },
	{	/*   0x41, */       '.',       '.' },
	{	/*   0x42, */      0xff,      0xff },
	{	/*   0x43, */       '*',       '*' },
	{	/*   0x44, */      0xff,      0xff },
	{	/*   0x45, */       '+',       '+' },
	{	/*   0x46, */      0xff,      0xff },
	{	/*   0x47, */      0xff,      0xff },
	{	/*   0x48, */      0xff,      0xff },
	{	/*   0x49, */      0xff,      0xff },
	{	/*   0x4A, */      0xff,      0xff },
	{	/*   0x4B, */       '/',       '/' },
	{	/*   0x4C, */      0x0D,      0x0D },
	{	/*   0x4D, */      0xff,      0xff },
	{	/*   0x4E, */       '-',       '-' },
	{	/*   0x4F, */      0xff,      0xff },
	{	/*   0x50, */      0xff,      0xff },
	{	/*   0x51, */       '=',       '=' },
	{	/*   0x52, */       '0',       '0' },
	{	/*   0x53, */       '1',       '1' },
	{	/*   0x54, */       '2',       '2' },
	{	/*   0x55, */       '3',       '3' },
	{	/*   0x56, */       '4',       '4' },
	{	/*   0x57, */       '5',       '5' },
	{	/*   0x58, */       '6',       '6' },
	{	/*   0x59, */       '7',       '7' },
	{	/*   0x5A, */      0xff,      0xff },
	{	/*   0x5B, */       '8',       '8' },
	{	/*   0x5C, */       '9',       '9' },
	{	/*   0x5D, */      0xff,      0xff },
	{	/*   0x5E, */      0xff,      0xff },
	{	/*   0x5F, */      0xff,      0xff },
	{	/*   0x60, */      0xff,      0xff },
	{	/*   0x61, */      0xff,      0xff },
	{	/*   0x62, */      0xff,      0xff },
	{	/*   0x63, */      0xff,      0xff },
	{	/*   0x64, */      0xff,      0xff },
	{	/*   0x65, */      0xff,      0xff },
	{	/*   0x66, */      0xff,      0xff },
	{	/*   0x67, */      0xff,      0xff },
	{	/*   0x68, */      0xff,      0xff },
	{	/*   0x69, */      0xff,      0xff },
	{	/*   0x6A, */      0xff,      0xff },
	{	/*   0x6B, */      0xff,      0xff },
	{	/*   0x6C, */      0xff,      0xff },
	{	/*   0x6D, */      0xff,      0xff },
	{	/*   0x6E, */      0xff,      0xff },
	{	/*   0x6F, */      0xff,      0xff },
	{	/*   0x70, */      0xff,      0xff },
	{	/*   0x71, */      0xff,      0xff },
	{	/*   0x72, */      0xff,      0xff },
	{	/*   0x73, */      0xff,      0xff },
	{	/*   0x74, */      0xff,      0xff },
	{	/*   0x75, */      0xff,      0xff },
	{	/*   0x76, */      0xff,      0xff },
	{	/*   0x77, */      0xff,      0xff },
	{	/*   0x78, */      0xff,      0xff },
	{	/*   0x79, */      0xff,      0xff },
	{	/*   0x7A, */      0xff,      0xff },
	{	/*   0x7B, */      0xff,      0xff },
	{	/*   0x7C, */      0xff,      0xff },
	{	/*   0x7D, */      0xff,      0xff },
	{	/*   0x7E, */      0xff,      0xff },
	{	/*   0x7F, */      0xff,      0xff }
};



adb_keymap_t keyboard_german = {
		/* Scan code      Normal     Shifted */
	{	/*   0x00, */       'a',       'A' },
	{	/*   0x01, */       's',       'S' },
	{	/*   0x02, */       'd',       'D' },
	{	/*   0x03, */       'f',       'F' },
	{	/*   0x04, */       'h',       'H' },
	{	/*   0x05, */       'g',       'G' },
	{	/*   0x06, */       'y',       'Y' },
	{	/*   0x07, */       'x',       'X' },
	{	/*   0x08, */       'c',       'C' },
	{	/*   0x09, */       'v',       'V' },
	{	/*   0x0A, */       '^',       '~' },
	{	/*   0x0B, */       'b',       'B' },
	{	/*   0x0C, */       'q',       'Q' },
	{	/*   0x0D, */       'w',       'W' },
	{	/*   0x0E, */       'e',       'E' },
	{	/*   0x0F, */       'r',       'R' },
	{	/*   0x10, */       'z',       'Z' },
	{	/*   0x11, */       't',       'T' },
	{	/*   0x12, */       '1',       '!' },
	{	/*   0x13, */       '2',       '"' },
	{	/*   0x14, */       '3',       '@' },
	{	/*   0x15, */       '4',       '$' },
	{	/*   0x16, */       '6',       '&' },
	{	/*   0x17, */       '5',       '%' },
	{	/*   0x18, */       '~',       '`' },
	{	/*   0x19, */       '9',       ')' },
	{	/*   0x1A, */       '7',       '/' },
	{	/*   0x1B, */       '^',       '?' },
	{	/*   0x1C, */       '8',       '(' },
	{	/*   0x1D, */       '0',       '=' },
	{	/*   0x1E, */       '+',       '*' },
	{	/*   0x1F, */       'o',       'O' },
	{	/*   0x20, */       'u',       'U' },
	{	/*   0x21, */      '\\',       '|' },
	{	/*   0x22, */       'i',       'I' },
	{	/*   0x23, */       'p',       'P' },
	{	/*   0x24, */      0x0D,      0x0D },
	{	/*   0x25, */       'l',       'L' },
	{	/*   0x26, */       'j',       'J' },
	{	/*   0x27, */       ']',       '}' },
	{	/*   0x28, */       'k',       'K' },
	{	/*   0x29, */       '[',       '{' },
	{	/*   0x2A, */       '#',      '\'' },
	{	/*   0x2B, */       ',',       ';' },
	{	/*   0x2C, */       '-',       '_' },
	{	/*   0x2D, */       'n',       'N' },
	{	/*   0x2E, */       'm',       'M' },
	{	/*   0x2F, */       '.',       ':' },
	{	/*   0x30, */      0x09,      0x09 },
	{	/*   0x31, */       ' ',       ' ' },
	{	/*   0x32, */       '<',       '>' },
	{	/*   0x33, */      0x7F,      0x7F }, /* Delete */
	{	/*   0x34, */      0xff,      0xff },
	{	/*   0x35, */      0x1B,      0x1B },
	{	/*   0x36, */      0xff,      0xff },
	{	/*   0x37, */      0xff,      0xff },
	{	/*   0x38, */      0xff,      0xff },
	{	/*   0x39, */      0xff,      0xff },
	{	/*   0x3A, */      0xff,      0xff },
	{	/*   0x3B, */       'h',      0xff },
	{	/*   0x3C, */       'l',      0xff },
	{	/*   0x3D, */       'j',      0xff },
	{	/*   0x3E, */       'k',      0xff },
	{	/*   0x3F, */      0xff,      0xff },
	{	/*   0x40, */      0xff,      0xff },
	{	/*   0x41, */       '.',       '.' },
	{	/*   0x42, */      0xff,      0xff },
	{	/*   0x43, */       '*',       '*' },
	{	/*   0x44, */      0xff,      0xff },
	{	/*   0x45, */       '+',       '+' },
	{	/*   0x46, */      0xff,      0xff },
	{	/*   0x47, */      0xff,      0xff },
	{	/*   0x48, */      0xff,      0xff },
	{	/*   0x49, */      0xff,      0xff },
	{	/*   0x4A, */      0xff,      0xff },
	{	/*   0x4B, */       '/',       '/' },
	{	/*   0x4C, */      0x0D,      0x0D },
	{	/*   0x4D, */      0xff,      0xff },
	{	/*   0x4E, */       '-',       '-' },
	{	/*   0x4F, */      0xff,      0xff },
	{	/*   0x50, */      0xff,      0xff },
	{	/*   0x51, */       '=',       '=' },
	{	/*   0x52, */       '0',       '0' },
	{	/*   0x53, */       '1',       '1' },
	{	/*   0x54, */       '2',       '2' },
	{	/*   0x55, */       '3',       '3' },
	{	/*   0x56, */       '4',       '4' },
	{	/*   0x57, */       '5',       '5' },
	{	/*   0x58, */       '6',       '6' },
	{	/*   0x59, */       '7',       '7' },
	{	/*   0x5A, */      0xff,      0xff },
	{	/*   0x5B, */       '8',       '8' },
	{	/*   0x5C, */       '9',       '9' },
	{	/*   0x5D, */      0xff,      0xff },
	{	/*   0x5E, */      0xff,      0xff },
	{	/*   0x5F, */      0xff,      0xff },
	{	/*   0x60, */      0xff,      0xff },
	{	/*   0x61, */      0xff,      0xff },
	{	/*   0x62, */      0xff,      0xff },
	{	/*   0x63, */      0xff,      0xff },
	{	/*   0x64, */      0xff,      0xff },
	{	/*   0x65, */      0xff,      0xff },
	{	/*   0x66, */      0xff,      0xff },
	{	/*   0x67, */      0xff,      0xff },
	{	/*   0x68, */      0xff,      0xff },
	{	/*   0x69, */      0xff,      0xff },
	{	/*   0x6A, */      0xff,      0xff },
	{	/*   0x6B, */      0xff,      0xff },
	{	/*   0x6C, */      0xff,      0xff },
	{	/*   0x6D, */      0xff,      0xff },
	{	/*   0x6E, */      0xff,      0xff },
	{	/*   0x6F, */      0xff,      0xff },
	{	/*   0x70, */      0xff,      0xff },
	{	/*   0x71, */      0xff,      0xff },
	{	/*   0x72, */      0xff,      0xff },
	{	/*   0x73, */      0xff,      0xff },
	{	/*   0x74, */      0xff,      0xff },
	{	/*   0x75, */      0xff,      0xff },
	{	/*   0x76, */      0xff,      0xff },
	{	/*   0x77, */      0xff,      0xff },
	{	/*   0x78, */      0xff,      0xff },
	{	/*   0x79, */      0xff,      0xff },
	{	/*   0x7A, */      0xff,      0xff },
	{	/*   0x7B, */      0xff,      0xff },
	{	/*   0x7C, */      0xff,      0xff },
	{	/*   0x7D, */      0xff,      0xff },
	{	/*   0x7E, */      0xff,      0xff },
	{	/*   0x7F, */      0xff,      0xff }
};
