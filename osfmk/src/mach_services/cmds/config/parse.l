%{
/*
 * Copyright (c) 1995, 1994, 1993, 1992, 1991, 1990  
 * Open Software Foundation, Inc. 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of ("OSF") or Open Software 
 * Foundation not be used in advertising or publicity pertaining to 
 * distribution of the software without specific, written prior permission. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL OSF BE LIABLE FOR ANY 
 * SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN 
 * ACTION OF CONTRACT, NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE 
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * OSF Research Institute MK6.1 (unencumbered) 1/31/1995
 */
/*-
 * Copyright (c) 1980 The Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)lang.l	5.9 (Berkeley) 6/19/92
 */

#include <ctype.h>
#include "gram.h"
#include "config.h"

#ifdef yywrap
#undef yywrap
#endif
#ifndef FLEX_SCANNER
/*
 * For lex scanners, redefine the input() and unput() macros
 * in order to allow pushing context to an include file. The maximum
 * number of include files is limited only by available memory and
 * the size of the input buffer.
 */
#undef input
#undef unput
extern unsigned char *bufbase;
#define input() \
	((yytchar=yysptr>bufbase?U(*--yysptr):getc(yyin))==EOF?0:yytchar)
#define unput(c) {*yysptr++=(c);}
#endif

#define tprintf if (do_trace) printf

/*
 * Key word table
 */

struct kt {
	char *kt_name;
	int kt_val;
} key_words[] = {
	{ "and",	AND },
	{ "args",	ARGS },
	{ "at",		AT },
	{ "config",	CONFIG },
	{ "controller",	CONTROLLER },
	{ "cpu",	CPU },
	{ "csr",	CSR },
	{ "device",	DEVICE },
	{ "disk",	DISK },
	{ "drive",	DRIVE },
	{ "dst",	DST },
	{ "dumps",	DUMPS },
	{ "dynamic",	DYNAMIC },
	{ "flags",	FLAGS },
	{ "hz",		HZ },
	{ "ident",	IDENT },
	{ "include",	INCLUDE },
	{ "machine",	MACHINE },
	{ "major",	MAJOR },
	{ "makeoptions", MAKEOPTIONS },
	{ "makevariables", MAKEOPTIONS },
	{ "master",	MASTER },
	{ "maxdsiz",	MAXDSIZ },
	{ "maxusers",	MAXUSERS },
	{ "mba",	MBA },
	{ "minor",	MINOR },
	{ "nexus",	NEXUS },
	{ "not",	NOT },
	{ "on",		ON },
	{ "options",	OPTIONS },
	{ "priority",	PRIORITY },
	{ "pseudo-device",PSEUDO_DEVICE },
	{ "root",	ROOT },
	{ "size",	SIZE },
	{ "slave",	SLAVE },
	{ "swap",	SWAP },
	{ "sympref",	SYMPREF },
	{ "unit-shift",	UNIT_SHIFT },
	{ "tape",	DEVICE },
	{ "timezone",	TIMEZONE },
	{ "trace",	TRACE },
	{ "uba",	UBA },
	{ "vector",	VECTOR },
	{ "lun",	LUN },			/* MMAX only */
	{ "slot",	SLOT },			/* MMAX only */
	{ "tape",	TAPE },			/* MMAX only */
	{ "bin",	BIN },			/* SQT ONLY */
	{ "am",		ADDRMOD },		/* MIPS */
	{ "mbii",	MBII },			/* MIPS */
 	{ "vme",	VME },			/* MIPS */
	{ 0, 0 },
};
%}
WORD	([A-Za-z_][-A-Za-z_]*|[A-Z][-A-Za-z_0-9]*)
%%
{WORD}		{
			int i;

			if ((i = kw_lookup(yytext)) == -1)
			{
				yylval.str = (char *)yytext;
				tprintf("id(%s) ", yytext);
				return ID;
			}
			tprintf("(%s) ", yytext);
			return i;
		}
\"[^"]+\"	{
			yytext[strlen((char *)yytext)-1] = '\0';
			yylval.str = (char *)yytext + 1;
			tprintf("string(\"%s\") ", yylval.str);
 			return ID;
		}
0[0-7]*		{
			yylval.val = octal(yytext);
			tprintf("#O:%o ", yylval.val);
			return NUMBER;
		}
0x[0-9a-fA-F]+	{
			yylval.val = hex(yytext);
			tprintf("#X:%x ", yylval.val);
			return NUMBER;
		}
[1-9][0-9]*	{
			yylval.val = atoi((char *)yytext);
			tprintf("#D:%d ", yylval.val);
			return NUMBER;
		}
[0-9]"."[0-9]*	{
			double atof();
			yylval.val = (int) (60 * atof((char *)yytext) + 0.5);
			return FPNUMBER;
		}
"-"		{
			return MINUS;
		}
"?"		{
			yylval.val = -1;
			tprintf("? ");
			return NUMBER;
		}
\n/[ \t]	{
			yyline++;
			tprintf("\n... ");
		}
\n/#		{
			yyline++;
			return(SEMICOLON);
		}
\n		{
			yyline++;
			tprintf("\n");
			return SEMICOLON;
		}
#.*		{	/* Ignored (comment) */;	}
[ \t]*		{	/* Ignored (white space) */;	}
";"		{	return SEMICOLON;		}
","		{	return COMMA;			}
"!"		{	return NOT;			}
"="		{	return EQUALS;			}
"@"		{	return AT;			}
.		{	return yytext[0];		}

vme16           {       return VME16D16;                }
vme24           {       return VME24D16;                }
vme32           {       return VME32D32;                }

vme16d16        {       return VME16D16;                }
vme24d16        {       return VME24D16;                }
vme32d16        {       return VME32D16;                }
vme16d32        {       return VME16D32;                }
vme24d32        {       return VME24D32;                }
vme32d32        {       return VME32D32;                }

%%
/*
 * kw_lookup
 *	Look up a string in the keyword table.  Returns a -1 if the
 *	string is not a keyword otherwise it returns the keyword number
 */

kw_lookup(word)
register char *word;
{
	register struct kt *kp;

	for (kp = key_words; kp->kt_name != 0; kp++)
		if (eq(word, kp->kt_name))
			return kp->kt_val;
	return -1;
}

/*
 * Number conversion routines
 */

octal(str)
char *str;
{
	int num;

	(void) sscanf(str, "%o", &num);
	return num;
}

hex(str)
char *str;
{
	int num;

	(void) sscanf(str+2, "%x", &num);
	return num;
}

#ifdef FLEX_SCANNER
struct file_desc {
	struct file_desc *prev_desc;
	YY_BUFFER_STATE buffer;
	FILE *stream;
	char *filename;
	int line_number;
};
#else /* !FLEX_SCANNER */
struct file_desc {
	struct file_desc *prev_desc;
	unsigned char *buffer;
	FILE *stream;
	char *filename;
	int line_number;
};
unsigned char *bufbase = yysbuf;
#endif

struct file_desc *cur_desc = NULL;
char *cur_filename = NULL;

int
include_file(filename)
char *filename;
{
	FILE *new_file;
#ifdef FLEX_SCANNER
	YY_BUFFER_STATE new_buffer;
#endif
	struct file_desc *new_desc;

	if ((new_file = VPATHopen(filename, "r")) == NULL) {
		return(1);
	}
	if (makedepends) {
		fprintf(dependfile, " \\\n\t%s", filename);
	}
	new_desc = (struct file_desc *)malloc(sizeof(struct file_desc));
			/* XXX - report malloc failure */
	new_desc->prev_desc = cur_desc;
#ifdef FLEX_SCANNER
	new_buffer = yy_create_buffer(new_file, YY_BUF_SIZE);
	new_desc->buffer = new_buffer;
#else
	new_desc->buffer = yysptr;
#endif
	new_desc->stream = new_file;
	new_desc->filename = ns(filename);

	if (cur_desc) cur_desc->line_number = yyline;

	yyline = 1;
	cur_desc = new_desc;
	cur_filename = new_desc->filename;

#ifdef FLEX_SCANNER
	yy_switch_to_buffer(new_buffer);
#else
	bufbase = yysptr;
	yyin = new_file;
#endif

	return(0);
}

int
yywrap()
{
	struct file_desc *prev_desc = cur_desc->prev_desc;

#ifdef FLEX_SCANNER
	yy_delete_buffer(cur_desc->buffer);
#endif
	fclose(cur_desc->stream);
	if (prev_desc) {
		free(cur_desc->filename);
	}
	free(cur_desc);

	cur_desc = prev_desc;
	if (cur_desc) {
#ifdef FLEX_SCANNER
		yy_switch_to_buffer(cur_desc->buffer);
#else
		yysptr = bufbase;
		bufbase = cur_desc->buffer;
		yyin = cur_desc->stream;
#endif
		yyline = cur_desc->line_number;
		cur_filename = cur_desc->filename;
		return(0);
	}
	return(1);
}

yyerror(s)
	char *s;
{
	fprintf(stderr, "config: file %s, line %d: %s\n",
		cur_filename, yyline, s);
	global_status = 1;
}
