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
 *  (c) Copyright 1991 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */

/*
 * Copyright (c) 1990, 1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * THE UNIVERSITY OF UTAH AND CSL PROVIDE THIS SOFTWARE IN ITS "AS IS"
 * CONDITION, AND DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM ITS USE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: exec.h 1.8 94/12/14$
 */

/*
 * HP-UX object format
 */

#ifndef _HP700_EXEC_H_
#define _HP700_EXEC_H_

union name_pt {
                char        *n_name;
                unsigned int n_strx;
              };

struct aux_id { 
	unsigned int mandatory : 1;
	unsigned int copy : 1;
	unsigned int append : 1;
	unsigned int ignore : 1;
	unsigned int reserved : 12;
	unsigned int type : 16;
	unsigned int length;
};

struct som_exec_auxhdr {
	struct aux_id som_auxhdr;	/* som auxiliary header header  */
	long exec_tsize;		/* text size in bytes           */
	long exec_tmem;			/* offset of text in memory     */
	long exec_tfile;		/* location of text in file     */
	long exec_dsize;		/* initialized data             */
	long exec_dmem;			/* offset of data in memory     */
	long exec_dfile;		/* location of data in file     */
	long exec_bsize;		/* uninitialized data (bss)     */
	long exec_entry;		/* offset of entrypoint         */
	long exec_flags;		/* loader flags                 */
	long exec_bfill;		/* bss initialization value     */
};

struct sys_clock { 
	unsigned int secs; 
	unsigned int nanosecs; 
};


/*
 * system_id values
 */
#define CPU_PA_RISC1_0  0x20B
#define CPU_PA_RISC1_1  0x210
#define CPU_PA_RISC1_2  0x211
#define CPU_PA_RISC2_0  0x214

/*
 * a_magic values
 */

#define EXEC_MAGIC	0x0107		/* normal executable */
#define SHARE_MAGIC	0x0108		/* shared executable */
#define DEMAND_MAGIC	0x010B		/* demand-load executable */


struct header {
        short int system_id;                 /* magic number - system        */
        short int a_magic;                   /* magic number - file type     */
        unsigned int version_id;             /* version id; format= YYMMDDHH */
        struct sys_clock file_time;          /* system clock- zero if unused */
        unsigned int entry_space;            /* index of space containing 
                                                entry point                  */
        unsigned int entry_subspace;         /* index of subspace for 
                                                entry point                  */
        unsigned int entry_offset;           /* offset of entry point        */
        unsigned int aux_header_location;    /* auxiliary header location    */
        unsigned int aux_header_size;        /* auxiliary header size        */
        unsigned int som_length;             /* length in bytes of entire som*/
        unsigned int presumed_dp;            /* DP value assumed during 
						compilation 		     */
        unsigned int space_location;         /* location in file of space 
                                                dictionary                   */
        unsigned int space_total;            /* number of space entries      */
        unsigned int subspace_location;      /* location of subspace entries */
        unsigned int subspace_total;         /* number of subspace entries   */
        unsigned int loader_fixup_location;  /* space reference array        */
        unsigned int loader_fixup_total;     /* total number of space 
                                                reference records            */
        unsigned int space_strings_location; /* file location of string area
                                                for space and subspace names */
        unsigned int space_strings_size;     /* size of string area for space 
                                                 and subspace names          */
        unsigned int init_array_location;    /* location in file of 
                                                initialization pointers      */
        unsigned int init_array_total;       /* number of init. pointers     */
        unsigned int compiler_location;      /* location in file of module 
                                                dictionary                   */
        unsigned int compiler_total;         /* number of modules            */
        unsigned int symbol_location;        /* location in file of symbol 
                                                dictionary                   */
        unsigned int symbol_total;           /* number of symbol records     */
        unsigned int fixup_request_location; /* location in file of fix_up 
                                                requests                     */
        unsigned int fixup_request_total;    /* number of fixup requests     */
        unsigned int symbol_strings_location;/* file location of string area 
                                                for module and symbol names  */
        unsigned int symbol_strings_size;    /* size of string area for 
                                                module and symbol names      */
        unsigned int unloadable_sp_location; /* byte offset of first byte of 
                                                data for unloadable spaces   */
        unsigned int unloadable_sp_size;     /* byte length of data for 
                                                unloadable spaces            */
        unsigned int checksum;
};

/******************************************************************************
*                                                                             *
*       The symbol dictionary consists of a sequence of symbol blocks         *
*  An entry into the symbol dictionary requires at least the         	      *
*  symbol_dictionary_record; The symbol_extension and argument_descriptor     *
*  records are optional                                  		      *
*                                                                             *
*******************************************************************************/

struct symbol_dictionary_record{

        unsigned int  symbol_type      : 8;
        unsigned int  symbol_scope     : 4;
        unsigned int  check_level      : 3;
        unsigned int  must_qualify     : 1;
        unsigned int  initially_frozen : 1;
        unsigned int  memory_resident  : 1;
        unsigned int  is_common        : 1;
        unsigned int  dup_common       : 1;
        unsigned int  xleast           : 2;
        unsigned int  arg_reloc        :10;
        union name_pt name;
        union name_pt qualifier_name; 
        unsigned int  symbol_info;
        unsigned int  symbol_value;
         
};

/*    The following is a list of the valid symbol types     */

#define    ST_NULL         0
#define    ST_ABSOLUTE     1
#define    ST_DATA         2
#define    ST_CODE         3
#define    ST_PRI_PROG     4
#define    ST_SEC_PROG     5
#define    ST_ENTRY        6
#define    ST_STORAGE      7
#define    ST_STUB         8
#define    ST_MODULE       9
#define    ST_SYM_EXT     10
#define    ST_ARG_EXT     11
#define    ST_MILLICODE   12
#define    ST_PLABEL      13

/*    The following is a list of the valid symbol scopes    */

#define    SS_UNSAT        0
#define    SS_EXTERNAL     1
#define    SS_GLOBAL       2
#define    SS_UNIVERSAL    3


#endif /* _HP700_EXEC_H_ */
