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

#include "boot.h"
#include <sys/elf.h>
#include <ddb/nlist.h>

#define trunc_page	i386_trunc_page
#define round_page	i386_round_page

extern struct thread_syscall_state thread_state;
#ifdef DEBUG
extern int debug;
#endif
extern int entry;
extern int region_count;
extern int sym_start;

#ifdef DEBUG
#define DPRINTF(args)	if (debug) printf args ;
#define DWAIT		if (debug) getchar();
#else
#define DPRINTF(args)
#define DWAIT
#endif

static Elf32_Ehdr ehdr;
static Elf32_Phdr phdr;
static Elf32_Shdr shdr;
static Elf32_Shdr strhdr;
static Elf32_Shdr symhdr;

static char symbuf[MAXBSIZE];

int
load_elf(const char *name, int *base, struct region_desc *rd)
{
	int i;
	int top = *base;
	int nsyms;
	struct nlist nl;
	int adjust = -1;
	vm_prot_t prot;
	int skew;
	Elf32_Sym *sym;
	int bufcnt;
	struct region_desc *rp;
	int text_start = 0;
	int text_end = 0;
	int data_start = 0;
	int data_end = 0;
	int bss_start = 0;
	int bss_end = 0;

DPRINTF(("load_elf(name %s, *base 0x%x)\n", name, *base))
DWAIT

	poff = 0;
	if (read(&ehdr, sizeof(ehdr)))
		return 1;
	if (ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
	    ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
	    ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
	    ehdr.e_ident[EI_MAG3] != ELFMAG3) {
		printf("Invalid format!\n");
		return 1;
	}

	rp = rd;
	for (i = 0; i < ehdr.e_phnum; i++) {
		/*
		 * Read the program header
		 */
		poff = ehdr.e_phoff + (i * ehdr.e_phentsize);
		if (read(&phdr, sizeof(phdr)))
			return 1;
		/*
		 * Only interested in load segments
		 */
		if (phdr.p_type != PT_LOAD)
			continue;
		/*
		 * Calculate base offset within memory object adjustment
		 */
		if (adjust == -1) {
			adjust = trunc_page(phdr.p_vaddr);
DPRINTF(("load_elf: adjust 0x%x\n", adjust))
			entry = *base + ehdr.e_entry - adjust;
DPRINTF(("load_elf: entry 0x%x\n", entry))
DWAIT
		}
		/*
		 * Fill in region description, adjusting to page alignment
		 */
		rp->addr = trunc_page(phdr.p_vaddr);
		rp->size = round_page(phdr.p_vaddr + phdr.p_filesz) -
			rp->addr;
		rp->offset = rp->addr - adjust;
		prot = 0;
		if (phdr.p_flags & PF_R)
			prot |= VM_PROT_READ;
		if (phdr.p_flags & PF_W)
			prot |= VM_PROT_WRITE;
		if (phdr.p_flags & PF_X)
			prot |= VM_PROT_EXECUTE;
		rp->prot = prot;
		rp->mapped = TRUE;
		if (top < *base + rp->offset + rp->size)
			top = *base + rp->offset + rp->size;
		/*
		 * Calculate file offset, adjusting for page alignment
		 */
		poff = trunc_page(phdr.p_offset);
DPRINTF(("load_elf: file offset 0x%x\n", poff))
DPRINTF(("load_elf: addr 0x%x size 0x%x offset 0x%x\n", rp->addr, rp->size, rp->offset))
DPRINTF(("load_elf: prot 0x%x mapped %d\n", rp->prot, rp->mapped))
DWAIT
		/*
		 * Read the segment into memory
		 */
		printf(rp == rd ? "%d" : "+%d", phdr.p_filesz);
		skew = phdr.p_vaddr - rp->addr;
		if (xread((void *) (*base + rp->offset), skew + phdr.p_filesz))
			return 1;
		/*
		 * Clear any portions of memory added for page alignment
		 */
		if (skew)
			pclr((void *) (*base + rp->offset), skew);
		skew = (rp->addr + rp->size) - (phdr.p_vaddr + phdr.p_filesz);
		if (skew)
			pclr((void *) (*base + rp->offset + rp->size - skew),
			      skew);
		/*
		 * Check if an additional region is needed
		 */
		rp++;
		if (phdr.p_memsz - phdr.p_filesz)
			printf("+%d", (int) (phdr.p_memsz - phdr.p_filesz));
		if (phdr.p_memsz - phdr.p_filesz <= skew)
			continue;
		rp->addr = round_page(phdr.p_vaddr + phdr.p_filesz);
		rp->size = round_page(phdr.p_vaddr + phdr.p_memsz) -
			rp->addr;
		rp->offset = rp->addr - adjust;
		rp->prot = prot;
		rp->mapped = FALSE;
DPRINTF(("load_elf: addr 0x%x size 0x%x offset 0x%x\n", rp->addr, rp->size, rp->offset))
DPRINTF(("load_elf: prot 0x%x mapped %d\n", rp->prot, rp->mapped))
DWAIT
		if (*base == KERNEL_BOOT_ADDR) {
			pclr((void *) (*base + rp->offset), rp->size);
			if (top < *base + rp->offset + rp->size)
				top = *base + rp->offset + rp->size;
		}
		rp++;
	}

	/*
	 * Create a stack region
	 */
	rp->addr = STACK_BASE;
	rp->size = STACK_SIZE;
	rp->offset = 0;
	rp->prot = VM_PROT_READ|VM_PROT_WRITE;
	rp->mapped = FALSE;
	rp++;

	/*
	 * Set initial thread state
	 */
	thread_state.eax = 0;
	thread_state.edx = 0;
	thread_state.efl = 0;
	thread_state.eip = ehdr.e_entry;
	thread_state.esp = STACK_PTR;

	region_count = rp - rd;
DPRINTF(("load_elf: region_count %d\n", region_count))
DPRINTF(("load_elf: top 0x%x\n", top))
DWAIT

	symhdr.sh_type = SHT_SYMTAB;
	symhdr.sh_link = 0;
	symhdr.sh_size = 0;
	symhdr.sh_entsize = sizeof(Elf32_Sym);
	poff = ehdr.e_shoff;
	for (i = 0; i < ehdr.e_shnum; i++) {
		if (read(&shdr, sizeof(shdr)))
			return 1;
		if (shdr.sh_type == SHT_SYMTAB)
			symhdr = shdr;
		else if (shdr.sh_type == SHT_PROGBITS) {
			/* (text_start | text_end) == 0 produces smaller code
			   than (text_start == 0 && text_end == 0).  */
			if ((text_start | text_end) == 0) {
				text_start = shdr.sh_addr;
				text_end = text_start + shdr.sh_size;
			} else if ((data_start | data_end) == 0) {
				data_start = shdr.sh_addr;
				data_end = data_start + shdr.sh_size;
			}
		}
		else if (shdr.sh_type == SHT_NOBITS) {
			if ((bss_start | bss_end) == 0) {
				bss_start = shdr.sh_addr;
				bss_end = bss_start + shdr.sh_size;
			}
		}
	}
	poff = ehdr.e_shoff + symhdr.sh_link * sizeof(shdr);
	if (read(&strhdr, sizeof(strhdr)))
		return 1;

DPRINTF(("load_elf: text_start 0x%x text_end 0x%x\n", text_start, text_end))
DPRINTF(("load_elf: data_start 0x%x data_end 0x%x\n", data_start, data_end))
DPRINTF(("load_elf: bss_start 0x%x bss_end 0x%x\n", bss_start, bss_end))
DWAIT

	printf("[+%d", (int) symhdr.sh_size);
	nsyms = symhdr.sh_size / symhdr.sh_entsize;
DPRINTF(("load_elf: nsyms %d\n", nsyms))
	sym_start = (vm_offset_t) top;
DPRINTF(("load_elf: sym_offset 0x%x sym_size 0x%x\n", sym_start, (int) ((nsyms * sizeof(struct nlist)) + sizeof(int))))
	i = nsyms * sizeof(struct nlist);
	pcpy(&i, (void *)top, sizeof(i));
	top += sizeof(int);
	poff = symhdr.sh_offset;
DPRINTF(("load_elf: symtab offset 0x%x\n", poff))
	bufcnt = sizeof(symbuf) / symhdr.sh_entsize;
	while (nsyms > 0) {
		i = (nsyms > bufcnt) ? bufcnt : nsyms;
		if (read((void *) symbuf, i * symhdr.sh_entsize))
			return 1;
		sym = (Elf32_Sym *) symbuf;
		nsyms -= i;
		while (i--) {
			nl.n_un.n_strx = (long)sym->st_name + sizeof(int);
			switch (ELF32_ST_TYPE(sym->st_info)) {
			case STT_FILE:
				nl.n_type = N_FN;
				break;
			case STT_OBJECT:
			case STT_FUNC:
				if (sym->st_value >= text_start &&
				    sym->st_value < text_end)
					nl.n_type = N_TEXT;
				else if (sym->st_value >= data_start &&
					 sym->st_value < data_end)
					nl.n_type = N_DATA;
				else if (sym->st_value >= bss_start &&
					 sym->st_value < bss_end)
					nl.n_type = N_BSS;
				else
					nl.n_type = N_ABS;
				break;
			case STT_NOTYPE:
				if (sym->st_shndx == SHN_UNDEF) {
					nl.n_type = N_UNDF;
					break;
				}
#ifdef DEBUG
			default:
				printf("ELF32_ST_TYPE %d\n",
				       ELF32_ST_TYPE(sym->st_info));
				break;
#endif
			}
			switch (ELF32_ST_BIND(sym->st_info)) {
			case STB_GLOBAL:
				nl.n_type |= N_EXT;
				break;
#ifdef DEBUG
			case STB_LOCAL:
				break;
			default:
				printf("ELF32_ST_BIND %d\n",
				       ELF32_ST_BIND(sym->st_info));
				break;
#endif
			}
			nl.n_other = (char)0;
			nl.n_desc = (short)0;
			nl.n_value = (unsigned long)sym->st_value;
			pcpy(&nl, (void *) top, sizeof(struct nlist));
			top += sizeof(struct nlist);
			sym++;
		}
DPRINTF(("load_elf: %d symbols to go...\n", nsyms))
	}
DPRINTF(("load_elf: str_offset 0x%x str_size 0x%x\n", top, (int) (strhdr.sh_size + sizeof(int))))
DWAIT
	i = strhdr.sh_size + sizeof(int);
	pcpy(&i, (void *)top, sizeof(i));
	top += sizeof(int);
	poff = strhdr.sh_offset;
DPRINTF(("load_elf: strtab offset 0x%x\n", poff))
DWAIT
	printf("+%d]\n", (int) strhdr.sh_size);
	if (xread((void *) top, strhdr.sh_size))
		return 1;
	top += strhdr.sh_size;
	if (top & (sizeof(int)-1))
		top += sizeof(int) - (top & (sizeof(int)-1));
	*base = top;
DPRINTF(("load_elf: new base 0x%x\n", top))
DWAIT
	return 0;
}
