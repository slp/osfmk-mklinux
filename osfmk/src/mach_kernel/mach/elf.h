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
 * OSF/1 HISTORY
 * Revision 1.1.2.7  1993/07/20  17:10:05  boot
 * 	Move HISTORY to top of file from bottom
 * 	[1993/07/20  14:40:01  boot]
 *
 * 	Machine-dependent Elf stuff.
 * 	[1993/07/20  12:43:46  boot]
 *
 * Revision 1.1.2.6  1993/06/11  13:23:20  smk
 * 	Moved from ./kernel/sys to ./osf1_server/sys
 * 	[1993/06/10  21:10:20  smk]
 * 
 * Revision 1.1.2.8  1993/05/13  17:26:24  duthie
 * 	Add DT_RELAENT
 * 	[1993/05/13  17:26:07  duthie]
 * 
 * Revision 1.1.2.7  1993/05/12  19:14:33  meissner
 * 	CR 9052 -- add dynamic link support.
 * 	[1993/05/12  19:14:12  meissner]
 * 
 * Revision 1.1.2.6  1993/05/07  19:59:13  meissner
 * 	CR 9034 -- add relocation entries.
 * 	[1993/05/07  19:02:45  meissner]
 * 
 * Revision 1.1.2.5  1993/05/07  14:00:22  meissner
 * 	CR 9033 -- add section attribute flags.
 * 	[1993/05/07  14:00:05  meissner]
 * 
 * Revision 1.1.2.4  1993/04/21  16:15:43  smk
 * 	Added special section numbers
 * 	(SHN_UNDEF etc. )
 * 	[1993/04/21  16:15:24  smk]
 * 
 * Revision 1.1.2.3  1993/04/06  20:07:03  smk
 * 	Added Elf32_Sym structure and related macros
 * 	[1993/04/06  20:06:41  smk]
 * 
 * Revision 1.1.2.2  1993/03/31  14:53:29  boot
 * 	New files for ELF
 * 	[1993/03/31  14:49:33  boot]
 * 
 */

#ifndef _SYS_ELF_H_
#define _SYS_ELF_H_

typedef unsigned long 	Elf32_Addr;
typedef unsigned long 	Elf32_Off;
typedef unsigned long 	Elf32_Word;

typedef unsigned short 	Elf32_Half;
typedef long	 	Elf32_Sword;

/* ELF Header - figure 4-3, page 4-4 */

#define EI_NIDENT 16

typedef struct {
  unsigned char		e_ident[EI_NIDENT];
  Elf32_Half		e_type;
  Elf32_Half		e_machine;
  Elf32_Word		e_version;
  Elf32_Addr		e_entry;
  Elf32_Off		e_phoff;
  Elf32_Off		e_shoff;
  Elf32_Word		e_flags;
  Elf32_Half		e_ehsize;
  Elf32_Half		e_phentsize;
  Elf32_Half		e_phnum;
  Elf32_Half		e_shentsize;
  Elf32_Half		e_shnum;
  Elf32_Half		e_shstrndx;
} Elf32_Ehdr;


/* e_version - object file version - page 4-6 */

#define EV_NONE         0
#define EV_CURRENT      1

/* e_ident[] identification indexes - figure 4-4, page 4-7 */
  
#define EI_MAG0		0
#define EI_MAG1		1
#define EI_MAG2		2
#define EI_MAG3		3
#define EI_CLASS	4
#define EI_DATA		5
#define EI_VERSION	6
#define EI_PAD		7

/* magic number - pg 4-8 */

#define ELFMAG0		0x7f
#define ELFMAG1		'E'
#define ELFMAG2		'L'
#define ELFMAG3		'F'

/* file class or capacity - page 4-8 */

#define ELFCLASSNONE	0
#define ELFCLASS32	1
#define ELFCLASS64	2

/* date encoding - page 4-9 */

#define ELFDATANONE	0
#define ELFDATA2LSB	1
#define ELFDATA2MSB	2

/* object file types - page 4-5 */

#define ET_NONE		0
#define ET_REL		1
#define ET_EXEC		2
#define ET_DYN		3
#define ET_CORE		4

#define ET_LOPROC	0xff00
#define ET_HIPROC	0xffff

/* architecture - page 4-5 */

#define EM_NONE		0
#define EM_M32		1
#define EM_SPARC	2
#define EM_386		3
#define EM_68K		4
#define EM_88K		5
#define EM_860		7
#define EM_MIPS		8
#define EM_PARISC      15	

/* version - page 4-6 */

#define EV_NONE		0
#define EV_CURRENT	1

/* special section indexes - page 4-11, figure 4-7 */

#define SHN_UNDEF       0
#define SHN_LORESERVE   0xff00
#define SHN_LOPROC      0xff00
#define SHN_HIPROC      0xff1f
#define SHN_ABS         0xfff1
#define SHN_COMMON      0xfff2
#define SHN_HIRESERVE   0xffff

/* section header - page 4-13, figure 4-8 */

typedef struct {
  Elf32_Word		sh_name;
  Elf32_Word		sh_type;
  Elf32_Word		sh_flags;
  Elf32_Addr		sh_addr;
  Elf32_Off		sh_offset;
  Elf32_Word		sh_size;
  Elf32_Word		sh_link;
  Elf32_Word		sh_info;
  Elf32_Word		sh_addralign;
  Elf32_Word		sh_entsize;
} Elf32_Shdr;

/* section types - page 4-15, figure 4-9 */

#define SHT_NULL	0
#define SHT_PROGBITS	1
#define SHT_SYMTAB	2
#define SHT_STRTAB	3
#define SHT_RELA	4
#define SHT_HASH	5
#define SHT_DYNAMIC	6
#define SHT_NOTE	7
#define SHT_NOBITS	8
#define SHT_REL		9
#define SHT_SHLIB      10
#define SHT_DYNSYM     11

#define SHT_LOPROC	0x70000000
#define SHT_HIPROC	0x7fffffff
#define SHT_LOUSER	0x80000000
#define SHT_HIUSER	0xffffffff

/* section attribute flags - page 4-18, figure 4-11 */

#define	SHF_WRITE	0x1
#define SHF_ALLOC	0x2
#define	SHF_EXECINSTR	0x4
#define SHF_MASKPROC	0xf0000000

/* symbol table - page 4-25, figure 4-15 */
typedef struct
{
    Elf32_Word    st_name;
    Elf32_Addr    st_value;
    Elf32_Word    st_size;
    unsigned char st_info;
    unsigned char st_other;
    Elf32_Half    st_shndx;
} Elf32_Sym;

/* symbol type and binding attributes - page 4-26 */

#define ELF32_ST_BIND(i)    ((i) >> 4)
#define ELF32_ST_TYPE(i)    ((i) & 0xf)
#define ELF32_ST_INFO(b,t)  (((b)<<4)+((t)&0xf))

/* symbol binding - page 4-26, figure 4-16 */

#define STB_LOCAL    0
#define STB_GLOBAL   1
#define STB_WEAK     2
#define STB_LOPROC  13
#define STB_HIPROC  15

/* symbol types - page 4-28, figure 4-17 */

#define STT_NOTYPE   0
#define STT_OBJECT   1
#define STT_FUNC     2
#define STT_SECTION  3
#define STT_FILE     4
#define STT_LOPROC  13
#define STT_HIPROC  15


/* relocation entries - page 4-31, figure 4-19 */

typedef struct
{
    Elf32_Addr	r_offset;
    Elf32_Word	r_info;
} Elf32_Rel;

typedef struct
{
    Elf32_Addr	r_offset;
    Elf32_Word	r_info;
    Elf32_Sword r_addend;
} Elf32_Rela;

/* Macros to split/combine relocation type and symbol page 4-32 */

#define ELF32_R_SYM(__i)	((__i)>>8)
#define ELF32_R_TYPE(__i)	((unsigned char) (__i))
#define ELF32_R_INFO(__s, __t)	(((__s)<<8) + (unsigned char) (__i))


/* program header - page 5-2, figure 5-1 */

typedef struct {
  Elf32_Word		p_type;
  Elf32_Off		p_offset;
  Elf32_Addr		p_vaddr;
  Elf32_Addr		p_paddr;
  Elf32_Word		p_filesz;
  Elf32_Word		p_memsz;
  Elf32_Word		p_flags;
  Elf32_Word		p_align;
} Elf32_Phdr;

/* segment types - page 5-3, figure 5-2 */

#define PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDR		6

#define PT_LOPROC	0x70000000
#define PT_HIPROC	0x7fffffff

/* segment permissions - page 5-6 */

#define PF_X		0x1
#define PF_W		0x2
#define PF_R		0x4
#define PF_MASKPROC	0xf0000000


/* dynamic structure - page 5-15, figure 5-9 */

typedef struct {
	Elf32_Sword	d_tag;
	union {
	    Elf32_Word	d_val;
	    Elf32_Addr	d_ptr;
	} d_un;
} Elf32_Dyn;

/* Dynamic array tags - page 5-16, figure 5-10.  */

#define DT_NULL		0
#define DT_NEEDED	1
#define DT_PLTRELSZ	2
#define DT_PLTGOT	3
#define DT_HASH		4
#define DT_STRTAB	5
#define DT_SYMTAB	6
#define DT_RELA		7
#define DT_RELASZ	8
#define DT_RELAENT      9
#define DT_STRSZ	10
#define DT_SYMENT	11
#define DT_INIT		12
#define DT_FINI		13
#define DT_SONAME	14
#define DT_RPATH	15
#define DT_SYMBOLIC	16
#define DT_REL		17
#define DT_RELSZ	18
#define DT_RELENT	19
#define DT_PLTREL	20
#define DT_DEBUG	21
#define DT_TEXTREL	22
#define DT_JMPREL	23

/*
 *	Bootstrap doesn't need machine dependent extensions.
 */

#endif /* _SYS_ELF_H_ */
