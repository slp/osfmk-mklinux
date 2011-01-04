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
 * rose_syminfo.h
 *
 * This header file contains data structures used by the secondary
 * bootstrap to load ROSE symbol table information into an image.
 *
 * It is also used by debuggers wishing to interpret that information.
 *
 * Other header files required:
 *	mach_o_header.h
 *	mach_o_format.h
 */

/*
 * struct ro_sym_info.
 *
 * There is one of these structure for each LDC_SYMBOLS or LDC_STRINGS
 * section loaded.  They are kept in a linked list fashion.
 *
 * The types (kind) of LDC_SYMBOLS can be any of SYMC_IMPORTS, 
 * SYMC_DEFINED_SYMBOLS, and SYMC_STABS.  SYMC_IMPORTS
 * is an invalid kind for the kernel.
 *
 * This structure, if referring to a defined symbol section, must also
 * refer to that section's associated strings section.  The field,
 * ro_symi_strings_section, provides this association.  In the case
 * of a strings section, that field will be undefined.
 *
 * Note: the length field is the length of the data only; it does not
 *	 include the remainder of the structure.
 */
struct ro_sym_info {
	struct ro_sym_info	*ro_symi_next;	 /* next section */
	mo_long_t		ro_symi_len;	 /* length, in bytes */
	struct ro_sym_info	*ro_symi_strings_section;
	union {					 /* start of storage */
		struct symbol_info_t	ro_un_symbols[1];
		char			ro_un_strings[1];
	} ro_symi_un;
};

#define ro_symi_symbols ro_symi_un.ro_un_symbols
#define ro_symi_strings ro_symi_un.ro_un_strings

/*
 * struct ro_sym_info_hdr
 *
 * There is only one of these structures in memory.  It points to 
 * the lists of various symbol-related sections.  The imports pointer
 * is included for completeness; it should be null.
 */
struct ro_sym_info_hdr {
	struct ro_sym_info	*ro_symh_defined;
	struct ro_sym_info	*ro_symh_imports;
	struct ro_sym_info	*ro_symh_strings;
	struct ro_sym_info	*ro_symh_stabs;
	unsigned		ro_symh_end;
};
