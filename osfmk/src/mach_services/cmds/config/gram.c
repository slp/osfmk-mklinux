#ifndef lint
static const char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#endif

#include <stdlib.h>
#include <string.h>

#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYPATCH 20070509

#define YYEMPTY (-1)
#define yyclearin    (yychar = YYEMPTY)
#define yyerrok      (yyerrflag = 0)
#define YYRECOVERING (yyerrflag != 0)

extern int yyparse(void);

static int yygrowstack(void);
#define YYPREFIX "yy"
#line 21 "gram.y"
typedef union {
	char	*str;
	int	val;
	struct	file_list *file;
	struct	idlst *lst;
} YYSTYPE;
#line 109 "gram.y"
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
/*
 * Copyright (c) 1988 Regents of the University of California.
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
 *	@(#)config.y	5.18 (Berkeley) 7/31/92
 */

#include "config.h"
#include <ctype.h>
#include <stdio.h>

struct	device cur;
struct	device *curp = 0;

#line 101 "y.tab.c"
#define ADDRMOD 257
#define AND 258
#define ANY 259
#define ARGS 260
#define AT 261
#define BIN 262
#define COMMA 263
#define CONFIG 264
#define CONTROLLER 265
#define CPU 266
#define CSR 267
#define DEVICE 268
#define DISK 269
#define DRIVE 270
#define DST 271
#define DUMPS 272
#define DYNAMIC 273
#define EQUALS 274
#define FLAGS 275
#define HZ 276
#define IDENT 277
#define INCLUDE 278
#define MACHINE 279
#define MAJOR 280
#define MASTER 281
#define MAXUSERS 282
#define MAXDSIZ 283
#define MBA 284
#define MBII 285
#define MINOR 286
#define MINUS 287
#define NEXUS 288
#define NOT 289
#define ON 290
#define OPTIONS 291
#define MAKEOPTIONS 292
#define PRIORITY 293
#define PSEUDO_DEVICE 294
#define ROOT 295
#define SEMICOLON 296
#define SIZE 297
#define SLAVE 298
#define SWAP 299
#define SYMPREF 300
#define UNIT_SHIFT 301
#define TIMEZONE 302
#define TRACE 303
#define UBA 304
#define VECTOR 305
#define VME 306
#define VME16D16 307
#define VME24D16 308
#define VME32D16 309
#define VME16D32 310
#define VME24D32 311
#define VME32D32 312
#define LUN 313
#define SLOT 314
#define TAPE 315
#define ID 316
#define NUMBER 317
#define FPNUMBER 318
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,   14,   14,   15,   15,   15,   15,   15,   15,   15,
    4,   17,   17,   17,   17,   17,   17,   17,   17,   17,
   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,
   17,   17,   17,   17,   20,   20,   21,   22,   22,   23,
   23,   23,   23,   24,   29,   29,   30,   12,   12,   25,
   10,   10,   26,   11,   11,   27,    9,    9,    8,   28,
   28,    6,    6,    7,    7,    7,   18,   18,   31,   31,
   32,   32,   32,    2,    2,    2,    1,   19,   19,   33,
   33,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,   16,   16,   34,   34,   34,   34,   34,   34,
   34,   35,   38,   36,   36,   39,   39,   39,   40,   40,
   41,   41,   41,   41,   41,   41,   41,   41,   13,   13,
   37,   37,   37,   37,   37,   42,    5,    5,    5,    5,
};
short yylen[] = {                                         2,
    1,    2,    0,    2,    2,    2,    3,    2,    1,    2,
    2,    2,    2,    2,    2,    2,    2,    2,    1,    2,
    2,    4,    3,    2,    4,    3,    3,    5,    4,    3,
    5,    4,    2,    2,    2,    1,    2,    2,    1,    1,
    1,    1,    1,    3,    3,    1,    2,    1,    1,    3,
    1,    1,    3,    1,    1,    3,    1,    1,    4,    1,
    0,    2,    0,    1,    2,    3,    3,    1,    1,    3,
    1,    2,    3,    1,    1,    0,    1,    3,    1,    3,
    2,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    3,    4,    4,    4,    4,    4,    3,
    4,    3,    0,    2,    0,    3,    3,    3,    2,    0,
    2,    2,    2,    2,    2,    2,    2,    2,    1,    2,
    1,    2,    3,    3,    0,    2,    1,    2,    2,    3,
};
short yydefred[] = {                                      3,
    0,    0,    0,    0,  103,    0,  103,  103,    0,    0,
    0,    0,  103,    0,    0,    0,    0,    0,  103,    9,
    0,    0,    0,    0,  103,    0,    2,    0,    0,   19,
    0,    0,   10,   77,   37,    0,    0,   15,    0,    0,
   20,   18,   11,   12,    0,   33,   34,    0,    0,    0,
    0,   68,    0,    0,    0,    0,   79,    0,   13,   14,
    0,    0,    0,    8,    0,    6,    4,    5,    0,    0,
    0,    0,    0,   39,   40,   41,   42,   43,    0,    0,
    0,  110,   83,   91,   82,   90,   84,   85,   86,   87,
   88,   89,   92,    0,    0,    0,    0,    7,   72,    0,
    0,    0,   81,    0,    0,    0,    0,    0,    0,    0,
    0,   60,    0,    0,    0,    0,   38,   94,    0,    0,
    0,    0,    0,   99,    0,    0,  102,   95,   97,   96,
   74,   75,   73,   67,   70,   80,   78,  101,    0,    0,
   22,   25,   98,    0,    0,   57,   58,   56,   54,   55,
   53,   51,   52,   50,   48,   49,    0,    0,   46,  108,
  107,  106,    0,    0,  126,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  109,   28,   31,    0,    0,    0,
   47,    0,  123,    0,  128,  124,  114,  117,  111,  112,
  116,  113,  115,    0,  119,  118,    0,   66,   62,   45,
  130,  120,   59,
};
short yydgoto[] = {                                       1,
  145,  133,   94,   26,  165,  181,  155,  156,  148,  154,
  151,  157,  196,    2,   27,   28,   29,   51,   56,   30,
   31,   73,   74,   75,   76,   77,   78,  113,  158,  159,
   52,   53,   57,   32,   36,   81,  124,   37,   82,  126,
  175,  125,
};
short yysindex[] = {                                      0,
    0, -235, -285, -298,    0, -298,    0,    0, -295, -298,
 -298, -298,    0, -291, -232, -196, -280, -279,    0,    0,
 -298, -229, -255, -206,    0, -204,    0, -197, -180,    0,
 -244, -153,    0,    0,    0,  -90, -109,    0,  -90,  -90,
    0,    0,    0,    0,  -90,    0,    0, -144, -298, -101,
  -86,    0,  -95, -298,  -89,  -79,    0, -109,    0,    0,
 -239,  -83,  -78,    0,  -90,    0,    0,    0,  -96,  -96,
  -96,  -96, -244,    0,    0,    0,    0,    0, -298, -142,
 -281,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -121, -281, -281, -281,    0,    0, -167,
 -280, -298,    0, -167, -279, -111,  -56,  -55, -103, -100,
 -281,    0, -277, -277, -277, -277,    0,    0,  -99,  -98,
  -97,  -94, -298,    0,  -72, -181,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  -92,  -91,
    0,    0,    0,  -88,  -87,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  -75,  -34,    0,    0,
    0,    0,  -77, -161,    0,  -85,  -84,  -82,  -81,  -80,
  -76,  -74,  -73, -268,    0,    0,    0,  -59,  -71,  -70,
    0, -277,    0, -298,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  -69,    0,    0,  -68,    0,    0,    0,
    0,    0,    0,
};
short yyrindex[] = {                                      0,
    0,  231,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  -62,  -58,    0,    0,    0, -200,    0,    0, -200, -200,
    0,    0,    0,    0, -200,    0,    0,    0,    0, -258,
  -54,    0, -243,    0,    0,  -50,    0,    0,    0,    0,
    0,  -46,  -45,    0, -200,    0,    0,    0, -276, -276,
 -276, -276,  -44,    0,    0,    0,    0,    0,    0,    0,
 -209,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -209, -209, -209,    0,    0, -238,
    0,    0,    0, -236,    0, -190,  -43,  -42,  -41,  -40,
 -209,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -152, -160,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  -39,  -38,
    0,    0,    0,    0, -188,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -148, -138,    0,    0,
    0,    0, -133, -219,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -158,    0,
    0,    0,    0, -178,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,
};
short yygindex[] = {                                      0,
   -4,  135,  -57,  224,  -93,    0,   68,   95,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  186,    0,    0,    0,    0,  141,    0,   78,
  160,    0,  157,    0,  179,  114,   94,  244,    0,    0,
    0,  101,
};
#define YYTABLESIZE 264
short yytable[] = {                                      35,
  106,   38,  144,   61,   71,   42,   43,   44,   49,   54,
   33,  122,   50,   55,   71,   69,   59,   34,  194,   69,
    3,   41,  121,  123,   76,   46,   76,   70,    4,    5,
    6,   61,    7,    8,   76,   34,   34,   71,   34,   61,
    9,   10,   11,   12,   99,   13,   14,   15,  195,  103,
   71,   16,   69,  127,   72,   17,   18,   76,   19,   76,
   20,   62,   63,  125,   21,   22,   23,   24,  174,   64,
  185,   64,  105,  127,  118,  167,  127,  107,  108,   25,
  168,   11,  100,   64,   47,  169,  125,   60,  170,   64,
  201,   66,  105,  171,  129,  105,   50,  135,   67,   65,
   55,   65,   83,   84,  105,  100,   64,   64,   64,   63,
   64,   63,  104,   65,  129,   68,  172,  129,  164,   79,
  121,   44,   85,   63,   86,   87,   88,   89,   90,   91,
   92,  173,  104,   44,   93,  104,   65,   65,   65,  122,
   65,   83,   84,  121,  104,  119,   63,   63,  131,  132,
   63,   98,   95,   96,   34,  184,   44,   44,   97,  164,
   44,   85,  122,   86,   87,   88,   89,   90,   91,   92,
   80,  120,  100,   93,   83,   84,  101,  102,  111,  164,
  146,  149,  152,  105,  104,   39,   40,  109,  128,  129,
  130,   45,  110,  112,   85,  127,   86,   87,   88,   89,
   90,   91,   92,   65,  143,  138,   93,  147,  150,  153,
  114,  115,  116,  141,  139,  140,  142,  160,  161,  162,
  166,  180,  163,  182,  176,  177,  197,  123,  178,  179,
    1,  186,  187,   36,  188,  189,  190,   93,  136,   48,
  191,   16,  192,  193,  198,   17,  199,  202,  203,   21,
   24,   35,   27,   30,   23,   26,   29,   32,  117,  200,
  134,  137,   58,  183,
};
short yycheck[] = {                                       4,
   58,    6,  280,  280,  263,   10,   11,   12,  289,  289,
  296,  293,   17,   18,  273,  260,   21,  316,  287,  263,
  256,  317,   80,  305,  263,  317,  263,  272,  264,  265,
  266,  287,  268,  269,  273,  316,  316,  296,  316,  316,
  276,  277,  278,  279,   49,  281,  282,  283,  317,   54,
  295,  287,  296,  273,  299,  291,  292,  296,  294,  296,
  296,  317,  318,  273,  300,  301,  302,  303,  126,  258,
  164,  260,  273,  293,   79,  257,  296,  317,  318,  315,
  262,  278,  273,  272,  317,  267,  296,  317,  270,  296,
  184,  296,  293,  275,  273,  296,  101,  102,  296,  258,
  105,  260,  284,  285,  305,  296,  295,  296,  297,  258,
  299,  260,  273,  272,  293,  296,  298,  296,  123,  273,
  273,  260,  304,  272,  306,  307,  308,  309,  310,  311,
  312,  313,  293,  272,  316,  296,  295,  296,  297,  273,
  299,  284,  285,  296,  305,  288,  295,  296,  316,  317,
  299,  296,   39,   40,  316,  317,  295,  296,   45,  164,
  299,  304,  296,  306,  307,  308,  309,  310,  311,  312,
  261,  314,  274,  316,  284,  285,  263,  273,   65,  184,
  113,  114,  115,  263,  274,    7,    8,  271,   95,   96,
   97,   13,  271,  290,  304,  317,  306,  307,  308,  309,
  310,  311,  312,   25,  111,  317,  316,  113,  114,  115,
   70,   71,   72,  317,  271,  271,  317,  317,  317,  317,
  293,  297,  317,  258,  317,  317,  286,  305,  317,  317,
    0,  317,  317,  296,  317,  317,  317,  296,  104,   16,
  317,  296,  317,  317,  316,  296,  317,  317,  317,  296,
  296,  296,  296,  296,  296,  296,  296,  296,   73,  182,
  101,  105,   19,  163,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 318
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"ADDRMOD","AND","ANY","ARGS","AT",
"BIN","COMMA","CONFIG","CONTROLLER","CPU","CSR","DEVICE","DISK","DRIVE","DST",
"DUMPS","DYNAMIC","EQUALS","FLAGS","HZ","IDENT","INCLUDE","MACHINE","MAJOR",
"MASTER","MAXUSERS","MAXDSIZ","MBA","MBII","MINOR","MINUS","NEXUS","NOT","ON",
"OPTIONS","MAKEOPTIONS","PRIORITY","PSEUDO_DEVICE","ROOT","SEMICOLON","SIZE",
"SLAVE","SWAP","SYMPREF","UNIT_SHIFT","TIMEZONE","TRACE","UBA","VECTOR","VME",
"VME16D16","VME24D16","VME32D16","VME16D32","VME24D32","VME32D32","LUN","SLOT",
"TAPE","ID","NUMBER","FPNUMBER",
};
char *yyrule[] = {
"$accept : Configuration",
"Configuration : Many_specs",
"Many_specs : Many_specs Spec",
"Many_specs :",
"Spec : Device_spec SEMICOLON",
"Spec : Config_spec SEMICOLON",
"Spec : Include_spec SEMICOLON",
"Spec : MINUS Include_spec SEMICOLON",
"Spec : TRACE SEMICOLON",
"Spec : SEMICOLON",
"Spec : error SEMICOLON",
"Include_spec : INCLUDE Save_id",
"Config_spec : MACHINE Save_id",
"Config_spec : SYMPREF Save_id",
"Config_spec : UNIT_SHIFT NUMBER",
"Config_spec : CPU Save_id",
"Config_spec : OPTIONS Opt_list",
"Config_spec : MAKEOPTIONS Mkopt_list",
"Config_spec : IDENT Save_id",
"Config_spec : System_spec",
"Config_spec : HZ NUMBER",
"Config_spec : TIMEZONE NUMBER",
"Config_spec : TIMEZONE NUMBER DST NUMBER",
"Config_spec : TIMEZONE NUMBER DST",
"Config_spec : TIMEZONE FPNUMBER",
"Config_spec : TIMEZONE FPNUMBER DST NUMBER",
"Config_spec : TIMEZONE FPNUMBER DST",
"Config_spec : TIMEZONE MINUS NUMBER",
"Config_spec : TIMEZONE MINUS NUMBER DST NUMBER",
"Config_spec : TIMEZONE MINUS NUMBER DST",
"Config_spec : TIMEZONE MINUS FPNUMBER",
"Config_spec : TIMEZONE MINUS FPNUMBER DST NUMBER",
"Config_spec : TIMEZONE MINUS FPNUMBER DST",
"Config_spec : MAXUSERS NUMBER",
"Config_spec : MAXDSIZ NUMBER",
"System_spec : System_id System_parameter_list",
"System_spec : System_id",
"System_id : CONFIG Save_id",
"System_parameter_list : System_parameter_list System_parameter",
"System_parameter_list : System_parameter",
"System_parameter : swap_spec",
"System_parameter : root_spec",
"System_parameter : dump_spec",
"System_parameter : arg_spec",
"swap_spec : SWAP optional_on swap_device_list",
"swap_device_list : swap_device_list AND swap_device",
"swap_device_list : swap_device",
"swap_device : swap_device_spec optional_size",
"swap_device_spec : device_name",
"swap_device_spec : major_minor",
"root_spec : ROOT optional_on root_device_spec",
"root_device_spec : device_name",
"root_device_spec : major_minor",
"dump_spec : DUMPS optional_on dump_device_spec",
"dump_device_spec : device_name",
"dump_device_spec : major_minor",
"arg_spec : ARGS optional_on arg_device_spec",
"arg_device_spec : device_name",
"arg_device_spec : major_minor",
"major_minor : MAJOR NUMBER MINOR NUMBER",
"optional_on : ON",
"optional_on :",
"optional_size : SIZE NUMBER",
"optional_size :",
"device_name : Save_id",
"device_name : Save_id NUMBER",
"device_name : Save_id NUMBER ID",
"Opt_list : Opt_list COMMA Option",
"Opt_list : Option",
"Option : Basic_option",
"Option : Basic_option DYNAMIC Save_id",
"Basic_option : Save_id",
"Basic_option : NOT Save_id",
"Basic_option : Save_id EQUALS Opt_value",
"Opt_value : ID",
"Opt_value : NUMBER",
"Opt_value :",
"Save_id : ID",
"Mkopt_list : Mkopt_list COMMA Mkoption",
"Mkopt_list : Mkoption",
"Mkoption : Save_id EQUALS Opt_value",
"Mkoption : NOT Save_id",
"Dev : UBA",
"Dev : MBA",
"Dev : VME16D16",
"Dev : VME24D16",
"Dev : VME32D16",
"Dev : VME16D32",
"Dev : VME24D32",
"Dev : VME32D32",
"Dev : VME",
"Dev : MBII",
"Dev : ID",
"Device_spec : Basic_device_spec",
"Device_spec : Basic_device_spec DYNAMIC Save_id",
"Basic_device_spec : DEVICE Dev_name Dev_info Int_spec",
"Basic_device_spec : MASTER Dev_name Dev_info Int_spec",
"Basic_device_spec : DISK Dev_name Dev_info Int_spec",
"Basic_device_spec : TAPE Dev_name Dev_info Int_spec",
"Basic_device_spec : CONTROLLER Dev_name Dev_info Int_spec",
"Basic_device_spec : PSEUDO_DEVICE Init_dev Dev",
"Basic_device_spec : PSEUDO_DEVICE Init_dev Dev NUMBER",
"Dev_name : Init_dev Dev NUMBER",
"Init_dev :",
"Dev_info : Con_info Info_list",
"Dev_info :",
"Con_info : AT Dev NUMBER",
"Con_info : AT SLOT NUMBER",
"Con_info : AT NEXUS NUMBER",
"Info_list : Info_list Info",
"Info_list :",
"Info : CSR NUMBER",
"Info : DRIVE NUMBER",
"Info : SLAVE NUMBER",
"Info : ADDRMOD NUMBER",
"Info : LUN NUMBER",
"Info : FLAGS NUMBER",
"Info : BIN NUMBER",
"Info : Dev Value",
"Value : NUMBER",
"Value : MINUS NUMBER",
"Int_spec : Vec_spec",
"Int_spec : PRIORITY NUMBER",
"Int_spec : PRIORITY NUMBER Vec_spec",
"Int_spec : Vec_spec PRIORITY NUMBER",
"Int_spec :",
"Vec_spec : VECTOR Id_list",
"Id_list : Save_id",
"Id_list : Save_id Id_list",
"Id_list : Save_id NUMBER",
"Id_list : Save_id NUMBER Id_list",
};
#endif
#if YYDEBUG
#include <stdio.h>
#endif

/* define the initial stack-sizes */
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH  YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH  500
#endif
#endif

#define YYINITSTACKSIZE 500

int      yydebug;
int      yynerrs;
int      yyerrflag;
int      yychar;
short   *yyssp;
YYSTYPE *yyvsp;
YYSTYPE  yyval;
YYSTYPE  yylval;

/* variables for the parser stack */
static short   *yyss;
static short   *yysslim;
static YYSTYPE *yyvs;
static int      yystacksize;
#line 746 "gram.y"

/*
 * return the passed string in a new space
 */
char *
ns(str)
	register char *str;
{
	register char *cp;

	cp = malloc((unsigned)(strlen(str)+1));
	(void) strcpy(cp, str);
	return (cp);
}

/*
 * Search and destroy an option
 */
opt_remove(opp, tailp, s)
struct opt **opp;
struct opt **tailp;
char *s;
{
	struct opt *prev_op = NULL;
	struct opt *op;

	while (*opp) {
		if (strcmp(s, (*opp)->op_name) != 0) {
			prev_op = *opp;
			opp = &(*opp)->op_next;
			continue;
		}
		if (*opp == *tailp) {
			*tailp = prev_op;
		}
		op = (*opp);
		(*opp) = (*opp)->op_next;
		free(op->op_name);
		if (op->op_value)
			free(op->op_value);
		free(op);
	}
}

/*
 * add a device to the list of devices
 */
newdev(dp)
	register struct device *dp;
{
	register struct device *np;

	np = (struct device *) malloc(sizeof *np);
	*np = *dp;
	if (curp == 0)
		dtab = np;
	else
		curp->d_next = np;
	curp = np;
	curp->d_next = 0;
}

/*
 * note that a configuration should be made
 */
mkconf(sysname)
	char *sysname;
{
	register struct file_list *fl, **flp;

	fl = (struct file_list *) malloc(sizeof *fl);
	fl->f_type = SYSTEMSPEC;
	fl->f_needs = sysname;
	fl->f_rootdev = NODEV;
	fl->f_dumpdev = NODEV;
	fl->f_fn = 0;
	fl->f_next = 0;
	for (flp = confp; *flp; flp = &(*flp)->f_next)
		;
	*flp = fl;
	confp = flp;
}

struct file_list *
newswap()
{
	struct file_list *fl = (struct file_list *)malloc(sizeof (*fl));

	fl->f_type = SWAPSPEC;
	fl->f_next = 0;
	fl->f_swapdev = NODEV;
	fl->f_swapsize = 0;
	fl->f_needs = 0;
	fl->f_fn = 0;
	return (fl);
}

/*
 * Add a swap device to the system's configuration
 */
mkswap(system, fl, size)
	struct file_list *system, *fl;
	int size;
{
	register struct file_list **flp;
	char name[80];

	if (system == 0 || system->f_type != SYSTEMSPEC) {
		yyerror("\"swap\" spec precedes \"config\" specification");
		return;
	}
	if (size < 0) {
		yyerror("illegal swap partition size");
		return;
	}
	/*
	 * Append swap description to the end of the list.
	 */
	flp = &system->f_next;
	for (; *flp && (*flp)->f_type == SWAPSPEC; flp = &(*flp)->f_next)
		;
	fl->f_next = *flp;
	*flp = fl;
	fl->f_swapsize = size;
	/*
	 * If first swap device for this system,
	 * set up f_fn field to insure swap
	 * files are created with unique names.
	 */
	if (system->f_fn)
		return;
	if (eq(fl->f_fn, "generic"))
		system->f_fn = ns(fl->f_fn);
	else
		system->f_fn = ns(system->f_needs);
}

/*
 * find the pointer to connect to the given device and number.
 * returns 0 if no such device and prints an error message
 */
struct device *
connect(dev, num)
	register char *dev;
	register int num;
{
	register struct device *dp;
	struct device *huhcon();

	if (num == QUES)
		return (huhcon(dev));
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if ((num != dp->d_unit) || !eq(dev, dp->d_name))
			continue;
		if (dp->d_type != CONTROLLER && dp->d_type != MASTER) {
			(void) sprintf(errbuf,
			    "%s connected to non-controller", dev);
			yyerror(errbuf);
			return (0);
		}
		return (dp);
	}
	(void) sprintf(errbuf, "%s %d not defined", dev, num);
	yyerror(errbuf);
	return (0);
}

/*
 * connect to an unspecific thing
 */
struct device *
huhcon(dev)
	register char *dev;
{
	register struct device *dp, *dcp;
	struct device rdev;
	int oldtype;

	/*
	 * First make certain that there are some of these to wildcard on
	 */
	for (dp = dtab; dp != 0; dp = dp->d_next)
		if (eq(dp->d_name, dev))
			break;
	if (dp == 0) {
		(void) sprintf(errbuf, "no %s's to wildcard", dev);
		yyerror(errbuf);
		return (0);
	}
	oldtype = dp->d_type;
	dcp = dp->d_conn;
	/*
	 * Now see if there is already a wildcard entry for this device
	 * (e.g. Search for a "uba ?")
	 */
	for (; dp != 0; dp = dp->d_next)
		if (eq(dev, dp->d_name) && dp->d_unit == -1)
			break;
	/*
	 * If there isn't, make one because everything needs to be connected
	 * to something.
	 */
	if (dp == 0) {
		dp = &rdev;
		init_dev(dp);
		dp->d_unit = QUES;
		dp->d_name = ns(dev);
		dp->d_type = oldtype;
		newdev(dp);
		dp = curp;
		/*
		 * Connect it to the same thing that other similar things are
		 * connected to, but make sure it is a wildcard unit
		 * (e.g. up connected to sc ?, here we make connect sc? to a
		 * uba?).  If other things like this are on the NEXUS or
		 * if they aren't connected to anything, then make the same
		 * connection, else call ourself to connect to another
		 * unspecific device.
		 */
		if (dcp == TO_NEXUS || dcp == 0)
			dp->d_conn = dcp;
		else
			dp->d_conn = connect(dcp->d_name, QUES);
	}
	return (dp);
}

init_dev(dp)
	register struct device *dp;
{

	dp->d_name = "OHNO!!!";
	dp->d_type = DEVICE;
	dp->d_conn = 0;
	dp->d_vec = 0;
	dp->d_addr = dp->d_pri = dp->d_flags = dp->d_dk = 0;
	dp->d_slave = dp->d_drive = dp->d_unit = UNKNOWN;
	dp->d_mach = dp->d_bus = dp->d_addrmod = 0;
	dp->d_dynamic = 0;
}

/*
 * Check the timezone to make certain it is sensible
 */

check_tz()
{
	if (abs(zone) > 12 * 60)
		yyerror("timezone is unreasonable");
	else
		hadtz = 1;
}

/*
 * Check system specification and apply defaulting
 * rules on root, dump, and swap devices.
 */
checksystemspec(fl)
	register struct file_list *fl;
{
	char buf[BUFSIZ];
	register struct file_list *swap;
	int generic;

	if (fl == 0 || fl->f_type != SYSTEMSPEC) {
		yyerror("internal error, bad system specification");
		exit(1);
	}
	swap = fl->f_next;
	if (swap == 0)
		return;
	generic = swap && swap->f_type == SWAPSPEC && eq(swap->f_fn, "generic");
	if (fl->f_rootdev == NODEV && !generic) {
		yyerror("no root device specified");
		exit(1);
	}
	/*
	 * Default swap area to be in 'b' partition of root's
	 * device.  If root specified to be other than on 'a'
	 * partition, give warning, something probably amiss.
	 */
	if (swap == 0 || swap->f_type != SWAPSPEC) {
		dev_t dev;

		swap = newswap();
		dev = fl->f_rootdev;
		if (minor(dev) & DEV_MASK) {
			(void) sprintf(buf,
"Warning, swap defaulted to 'b' partition with root on '%c' partition",
				(minor(dev) & DEV_MASK) + 'a');
			yyerror(buf);
		}
		swap->f_swapdev =
		   makedev(major(dev), (minor(dev) &~ DEV_MASK) | ('b' - 'a'));
		swap->f_fn = devtoname(swap->f_swapdev);
		mkswap(fl, swap, 0);
	}
	/*
	 * Make sure a generic swap isn't specified, along with
	 * other stuff (user must really be confused).
	 */
	if (generic) {
		if (fl->f_rootdev != NODEV)
			yyerror("root device specified with generic swap");
		if (fl->f_dumpdev != NODEV)
			yyerror("dump device specified with generic swap");
		return;
	}
	/*
	 * Default dump device and warn if place is not a
	 * swap area.
	 */
	if (fl->f_dumpdev == NODEV)
		fl->f_dumpdev = swap->f_swapdev;
	if (fl->f_dumpdev != swap->f_swapdev) {
		struct file_list *p = swap->f_next;

		for (; p && p->f_type == SWAPSPEC; p = p->f_next)
			if (fl->f_dumpdev == p->f_swapdev)
				return;
		(void) sprintf(buf,
		    "Warning: dump device is not a swap partition");
		yyerror(buf);
	}
}

/*
 * Verify all devices specified in the system specification
 * are present in the device specifications.
 */
verifysystemspecs()
{
	register struct file_list *fl;
	dev_t checked[50], *verifyswap();
	register dev_t *pchecked = checked;

	for (fl = conf_list; fl; fl = fl->f_next) {
		if (fl->f_type != SYSTEMSPEC)
			continue;
		if (!finddev(fl->f_rootdev))
			deverror(fl->f_needs, "root");
		*pchecked++ = fl->f_rootdev;
		pchecked = verifyswap(fl->f_next, checked, pchecked);
#define	samedev(dev1, dev2) \
	((minor(dev1) &~ DEV_MASK) != (minor(dev2) &~ DEV_MASK))
		if (!alreadychecked(fl->f_dumpdev, checked, pchecked)) {
			if (!finddev(fl->f_dumpdev))
				deverror(fl->f_needs, "dump");
			*pchecked++ = fl->f_dumpdev;
		}
	}
}

/*
 * Do as above, but for swap devices.
 */
dev_t *
verifyswap(fl, checked, pchecked)
	register struct file_list *fl;
	dev_t checked[];
	register dev_t *pchecked;
{

	for (;fl && fl->f_type == SWAPSPEC; fl = fl->f_next) {
		if (eq(fl->f_fn, "generic"))
			continue;
		if (alreadychecked(fl->f_swapdev, checked, pchecked))
			continue;
		if (!finddev(fl->f_swapdev))
			fprintf(stderr,
			   "config: swap device %s not configured", fl->f_fn);
		*pchecked++ = fl->f_swapdev;
	}
	return (pchecked);
}

/*
 * Has a device already been checked
 * for it's existence in the configuration?
 */
alreadychecked(dev, list, last)
	dev_t dev, list[];
	register dev_t *last;
{
	register dev_t *p;

	for (p = list; p < last; p++)
		if (samedev(*p, dev))
			return (1);
	return (0);
}

deverror(systemname, devtype)
	char *systemname, *devtype;
{

	fprintf(stderr, "config: %s: %s device not configured\n",
		systemname, devtype);
}

/*
 * Look for the device in the list of
 * configured hardware devices.  Must
 * take into account stuff wildcarded.
 */
/*ARGSUSED*/
finddev(dev)
	dev_t dev;
{

	/* punt on this right now */
	return (1);
}
#line 940 "y.tab.c"
/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack(void)
{
    int newsize, i;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = yystacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;

    i = yyssp - yyss;
    newss = (yyss != 0)
          ? (short *)realloc(yyss, newsize * sizeof(*newss))
          : (short *)malloc(newsize * sizeof(*newss));
    if (newss == 0)
        return -1;

    yyss  = newss;
    yyssp = newss + i;
    newvs = (yyvs != 0)
          ? (YYSTYPE *)realloc(yyvs, newsize * sizeof(*newvs))
          : (YYSTYPE *)malloc(newsize * sizeof(*newvs));
    if (newvs == 0)
        return -1;

    yyvs = newvs;
    yyvsp = newvs + i;
    yystacksize = newsize;
    yysslim = yyss + newsize - 1;
    return 0;
}

#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse(void)
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register const char *yys;

    if ((yys = getenv("YYDEBUG")) != 0)
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = YYEMPTY;

    if (yyss == NULL && yygrowstack()) goto yyoverflow;
    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yysslim && yygrowstack())
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = YYEMPTY;
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;

    yyerror("syntax error");

#ifdef lint
    goto yyerrlab;
#endif

yyerrlab:
    ++yynerrs;

yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yysslim && yygrowstack())
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = YYEMPTY;
        goto yyloop;
    }

yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    if (yym)
        yyval = yyvsp[1-yym];
    else
        memset(&yyval, 0, sizeof yyval);
    switch (yyn)
    {
case 1:
#line 183 "gram.y"
 { verifysystemspecs(); }
break;
case 4:
#line 194 "gram.y"
 { newdev(&cur); }
break;
case 6:
#line 198 "gram.y"
 {
		  if (include_file(yyvsp[-1].str)) {
			yyerror("Cannot open include file");
		  }
		  free(yyvsp[-1].str);
		}
break;
case 7:
#line 206 "gram.y"
 {
		  include_file(yyvsp[-1].str);
		  free(yyvsp[-1].str);
		}
break;
case 8:
#line 212 "gram.y"
 { do_trace = !do_trace; }
break;
case 11:
#line 220 "gram.y"
 { yyval.str = yyvsp[0].str; }
break;
case 12:
#line 225 "gram.y"
 {
		machinename = yyvsp[0].str;
	      }
break;
case 13:
#line 229 "gram.y"
 {
		sympref = yyvsp[0].str;
	      }
break;
case 14:
#line 233 "gram.y"
 { DEV_SHIFT = yyvsp[0].val; DEV_MASK = ((1<<DEV_SHIFT)-1); }
break;
case 15:
#line 235 "gram.y"
 {
		struct cputype *cp =
		    (struct cputype *)malloc(sizeof (struct cputype));
		cp->cpu_name = yyvsp[0].str;
		cp->cpu_next = cputype;
		cputype = cp;
	      }
break;
case 18:
#line 247 "gram.y"
 { ident = yyvsp[0].str; }
break;
case 20:
#line 251 "gram.y"
 { yyerror("HZ specification obsolete; delete"); }
break;
case 21:
#line 253 "gram.y"
 { zone = 60 * yyvsp[0].val; check_tz(); }
break;
case 22:
#line 255 "gram.y"
 { zone = 60 * yyvsp[-2].val; dst = yyvsp[0].val; check_tz(); }
break;
case 23:
#line 257 "gram.y"
 { zone = 60 * yyvsp[-1].val; dst = 1; check_tz(); }
break;
case 24:
#line 259 "gram.y"
 { zone = yyvsp[0].val; check_tz(); }
break;
case 25:
#line 261 "gram.y"
 { zone = yyvsp[-2].val; dst = yyvsp[0].val; check_tz(); }
break;
case 26:
#line 263 "gram.y"
 { zone = yyvsp[-1].val; dst = 1; check_tz(); }
break;
case 27:
#line 265 "gram.y"
 { zone = -60 * yyvsp[0].val; check_tz(); }
break;
case 28:
#line 267 "gram.y"
 { zone = -60 * yyvsp[-2].val; dst = yyvsp[0].val; check_tz(); }
break;
case 29:
#line 269 "gram.y"
 { zone = -60 * yyvsp[-1].val; dst = 1; check_tz(); }
break;
case 30:
#line 271 "gram.y"
 { zone = -yyvsp[0].val; check_tz(); }
break;
case 31:
#line 273 "gram.y"
 { zone = -yyvsp[-2].val; dst = yyvsp[0].val; check_tz(); }
break;
case 32:
#line 275 "gram.y"
 { zone = -yyvsp[-1].val; dst = 1; check_tz(); }
break;
case 33:
#line 277 "gram.y"
 { maxusers = yyvsp[0].val; }
break;
case 34:
#line 279 "gram.y"
 { maxdsiz = yyvsp[0].val; }
break;
case 35:
#line 283 "gram.y"
 { checksystemspec(*confp); }
break;
case 36:
#line 285 "gram.y"
 { checksystemspec(*confp); }
break;
case 37:
#line 290 "gram.y"
 { mkconf(yyvsp[0].str); }
break;
case 47:
#line 316 "gram.y"
 { mkswap(*confp, yyvsp[-1].file, yyvsp[0].val); }
break;
case 48:
#line 321 "gram.y"
 {
			struct file_list *fl = newswap();

			if (eq(yyvsp[0].str, "generic"))
				fl->f_fn = yyvsp[0].str;
			else {
				fl->f_swapdev = nametodev(yyvsp[0].str, 0, 'b');
				fl->f_fn = devtoname(fl->f_swapdev);
			}
			yyval.file = fl;
		}
break;
case 49:
#line 333 "gram.y"
 {
			struct file_list *fl = newswap();

			fl->f_swapdev = yyvsp[0].val;
			fl->f_fn = devtoname(yyvsp[0].val);
			yyval.file = fl;
		}
break;
case 50:
#line 344 "gram.y"
 {
			struct file_list *fl = *confp;

			if (fl && fl->f_rootdev != NODEV)
				yyerror("extraneous root device specification");
			else
				fl->f_rootdev = yyvsp[0].val;
		}
break;
case 51:
#line 356 "gram.y"
 { yyval.val = nametodev(yyvsp[0].str, 0, 'a'); }
break;
case 53:
#line 362 "gram.y"
 {
			struct file_list *fl = *confp;

			if (fl && fl->f_dumpdev != NODEV)
				yyerror("extraneous dump device specification");
			else
				fl->f_dumpdev = yyvsp[0].val;
		}
break;
case 54:
#line 375 "gram.y"
 { yyval.val = nametodev(yyvsp[0].str, 0, 'b'); }
break;
case 56:
#line 381 "gram.y"
 { yyerror("arg device specification obsolete, ignored"); }
break;
case 57:
#line 386 "gram.y"
 { yyval.val = nametodev(yyvsp[0].str, 0, 'b'); }
break;
case 59:
#line 392 "gram.y"
 { yyval.val = makedev(yyvsp[-2].val, yyvsp[0].val); }
break;
case 62:
#line 402 "gram.y"
 { yyval.val = yyvsp[0].val; }
break;
case 63:
#line 404 "gram.y"
 { yyval.val = 0; }
break;
case 64:
#line 409 "gram.y"
 { yyval.str = yyvsp[0].str; }
break;
case 65:
#line 411 "gram.y"
 {
			char buf[80];

			(void) sprintf(buf, "%s%d", yyvsp[-1].str, yyvsp[0].val);
			yyval.str = ns(buf); free(yyvsp[-1].str);
		}
break;
case 66:
#line 418 "gram.y"
 {
			char buf[80];

			(void) sprintf(buf, "%s%d%s", yyvsp[-2].str, yyvsp[-1].val, yyvsp[0].str);
			yyval.str = ns(buf); free(yyvsp[-2].str);
		}
break;
case 70:
#line 436 "gram.y"
 { opt_tail->op_dynamic = yyvsp[0].str; }
break;
case 71:
#line 441 "gram.y"
 {
		struct opt *op;

		opt_remove(&opt, &opt_tail, yyvsp[0].str);

		op = (struct opt *)malloc(sizeof (struct opt));
		op->op_name = yyvsp[0].str;
		op->op_next = (struct opt *) 0;
		op->op_value = 0;
		op->op_dynamic = 0;
		if (opt == (struct opt *) 0)
			opt = op;
		else
			opt_tail->op_next = op;
		opt_tail = op;
	      }
break;
case 72:
#line 458 "gram.y"
 { opt_remove(&opt, &opt_tail, yyvsp[0].str); }
break;
case 73:
#line 460 "gram.y"
 {
		struct opt *op;

		opt_remove(&opt, &opt_tail, yyvsp[-2].str);

		op = (struct opt *)malloc(sizeof (struct opt));
		op->op_name = yyvsp[-2].str;
		op->op_next = (struct opt *) 0;
		op->op_value = yyvsp[0].str;
		op->op_dynamic = 0;
		if (opt == (struct opt *) 0)
			opt = op;
		else
			opt_tail->op_next = op;
		opt_tail = op;
	      }
break;
case 74:
#line 479 "gram.y"
 { yyval.str = ns(yyvsp[0].str); }
break;
case 75:
#line 481 "gram.y"
 {
		char nb[16];
		(void) sprintf(nb, "%u", yyvsp[0].val);
		yyval.str = ns(nb);
	      }
break;
case 76:
#line 487 "gram.y"
 { yyval.str = ns(""); }
break;
case 77:
#line 492 "gram.y"
 { yyval.str = ns(yyvsp[0].str); }
break;
case 80:
#line 503 "gram.y"
 {
		struct opt *op = (struct opt *)malloc(sizeof (struct opt));
		op->op_name = yyvsp[-2].str;
		op->op_next = (struct opt *) 0;
		op->op_value = yyvsp[0].str;
		if (mkopt == (struct opt *) 0)
			mkopt = op;
		else
			mkopt_tail->op_next = op;
		mkopt_tail = op;
	      }
break;
case 81:
#line 515 "gram.y"
	{
		opt_remove(&mkopt, &mkopt_tail, yyvsp[0].str);
	      }
break;
case 82:
#line 521 "gram.y"
 { yyval.str = ns("uba"); }
break;
case 83:
#line 523 "gram.y"
 { yyval.str = ns("mba"); }
break;
case 84:
#line 525 "gram.y"
 {
		yyval.str = ns("vme16d16");
		}
break;
case 85:
#line 529 "gram.y"
 {
			yyval.str = ns("vme24d16");
		}
break;
case 86:
#line 533 "gram.y"
 {
                yyval.str = ns("vme32d16");
                }
break;
case 87:
#line 537 "gram.y"
 {
                yyval.str = ns("vme16d32");
                }
break;
case 88:
#line 541 "gram.y"
 {
		yyval.str = ns("vme24d32");
		}
break;
case 89:
#line 545 "gram.y"
 {
		yyval.str = ns("vme32d32");
		}
break;
case 90:
#line 549 "gram.y"
 {
			yyval.str = ns("vme");
		}
break;
case 91:
#line 553 "gram.y"
 {
			yyval.str = ns("mbii");
		}
break;
case 92:
#line 557 "gram.y"
 { yyval.str = ns(yyvsp[0].str); }
break;
case 94:
#line 564 "gram.y"
 { cur.d_dynamic = yyvsp[0].str; }
break;
case 95:
#line 569 "gram.y"
 { cur.d_type = DEVICE; }
break;
case 96:
#line 571 "gram.y"
 { cur.d_type = MASTER; }
break;
case 97:
#line 573 "gram.y"
 { cur.d_dk = 1; cur.d_type = DEVICE; }
break;
case 98:
#line 576 "gram.y"
 { cur.d_type = DEVICE; }
break;
case 99:
#line 578 "gram.y"
 { cur.d_type = CONTROLLER; }
break;
case 100:
#line 580 "gram.y"
 {
		cur.d_name = yyvsp[0].str;
		cur.d_type = PSEUDO_DEVICE;
		}
break;
case 101:
#line 585 "gram.y"
 {
		cur.d_name = yyvsp[-1].str;
		cur.d_type = PSEUDO_DEVICE;
		cur.d_slave = yyvsp[0].val;
		}
break;
case 102:
#line 593 "gram.y"
 {
		cur.d_name = yyvsp[-1].str;
		if (eq(yyvsp[-1].str, "mba"))
			seen_mba = 1;
		else if (eq(yyvsp[-1].str, "uba"))
			seen_uba = 1;
		else if (eq(yyvsp[-1].str, "mbii"))
			seen_mbii = 1;
		else if (eq(yyvsp[-1].str, "vme"))
			seen_vme = 1;
		cur.d_unit = yyvsp[0].val;
		}
break;
case 103:
#line 608 "gram.y"
 { init_dev(&cur); }
break;
case 106:
#line 618 "gram.y"
 {
		if (eq(cur.d_name, "mba") || eq(cur.d_name, "uba")
		    || eq(cur.d_name, "mbii") || eq(cur.d_name, "vme")) {
			(void) sprintf(errbuf,
				"%s must be connected to a nexus", cur.d_name);
			yyerror(errbuf);
		}
		cur.d_conn = connect(yyvsp[-1].str, yyvsp[0].val);
		dev_param(&cur, "index", cur.d_unit);
		}
break;
case 107:
#line 630 "gram.y"
 { 
		check_slot(&cur, yyvsp[0].val);
		cur.d_addr = yyvsp[0].val;
		cur.d_conn = TO_SLOT; 
		 }
break;
case 108:
#line 636 "gram.y"
 { check_nexus(&cur, yyvsp[0].val); cur.d_conn = TO_NEXUS; }
break;
case 111:
#line 646 "gram.y"
 {
			cur.d_addr = yyvsp[0].val;
			dev_param(&cur, "csr", yyvsp[0].val);
		}
break;
case 112:
#line 651 "gram.y"
 {
			cur.d_drive = yyvsp[0].val;
			dev_param(&cur, "drive", yyvsp[0].val);
		}
break;
case 113:
#line 656 "gram.y"
 {
		if (cur.d_conn != 0 && cur.d_conn != TO_NEXUS &&
		    cur.d_conn->d_type == MASTER)
			cur.d_slave = yyvsp[0].val;
		else
			yyerror("can't specify slave--not to master");
		}
break;
case 114:
#line 665 "gram.y"
 { cur.d_addrmod = yyvsp[0].val; }
break;
case 115:
#line 668 "gram.y"
 {
		if ((cur.d_conn != 0) && (cur.d_conn != TO_SLOT) &&
			(cur.d_conn->d_type == CONTROLLER)) {
			cur.d_addr = yyvsp[0].val; 
		}
		else {
			yyerror("device requires controller card");
		    }
		}
break;
case 116:
#line 678 "gram.y"
 {
		cur.d_flags = yyvsp[0].val;
		dev_param(&cur, "flags", yyvsp[0].val);
	      }
break;
case 117:
#line 683 "gram.y"
 { 
		 if (yyvsp[0].val < 1 || yyvsp[0].val > 7)  
			yyerror("bogus bin number");
		 else {
			cur.d_bin = yyvsp[0].val;
			dev_param(&cur, "bin", yyvsp[0].val);
		}
	       }
break;
case 118:
#line 692 "gram.y"
 {
		dev_param(&cur, yyvsp[-1].str, yyvsp[0].val);
		}
break;
case 120:
#line 700 "gram.y"
 { yyval.val = -(yyvsp[0].val); }
break;
case 121:
#line 705 "gram.y"
 { cur.d_pri = 0; }
break;
case 122:
#line 707 "gram.y"
 { cur.d_pri = yyvsp[0].val; }
break;
case 123:
#line 709 "gram.y"
 { cur.d_pri = yyvsp[-1].val; }
break;
case 124:
#line 711 "gram.y"
 { cur.d_pri = yyvsp[0].val; }
break;
case 126:
#line 717 "gram.y"
 { cur.d_vec = yyvsp[0].lst; }
break;
case 127:
#line 721 "gram.y"
 {
		struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
		a->id = yyvsp[0].str; a->id_next = 0; yyval.lst = a;
		a->id_vec = 0;
		}
break;
case 128:
#line 726 "gram.y"

		{
		struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
	        a->id = yyvsp[-1].str; a->id_next = yyvsp[0].lst; yyval.lst = a;
		a->id_vec = 0;
		}
break;
case 129:
#line 733 "gram.y"
 {
		struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
		a->id_next = 0; a->id = yyvsp[-1].str; yyval.lst = a;
		a->id_vec = yyvsp[0].val;
		}
break;
case 130:
#line 739 "gram.y"
 {
		struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
		a->id_next = yyvsp[0].lst; a->id = yyvsp[-2].str; yyval.lst = a;
		a->id_vec = yyvsp[-1].val;
		}
break;
#line 1705 "y.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yysslim && yygrowstack())
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;

yyoverflow:
    yyerror("yacc stack overflow");

yyabort:
    return (1);

yyaccept:
    return (0);
}
