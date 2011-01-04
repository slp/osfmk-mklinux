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

#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/elf.h>

int
main(int argc, char **argv)
{
	int fd;
	FILE *out;
	struct stat st;
	char *base;
	Elf32_Ehdr *e;
	Elf32_Shdr *s;
	int i;

	if (argc != 3) {
		fprintf(stderr, "usage: rmelfhdr input-file output-file\n");
		exit(1);
	}
	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		perror(argv[1]);
		exit(1);
	}
	if (fstat(fd, &st) < 0) {
		perror("fstat");
		exit(1);
	}
	base = mmap(0, st.st_size, PROT_READ,
			MAP_PRIVATE|MAP_FILE|MAP_VARIABLE, fd, 0);
	if (base == (char *) -1) {
		perror("mmap");
		exit(1);
	}
	if ((out = fopen(argv[2], "w")) == NULL) {
		perror(argv[2]);
		exit(1);
	}
	e = (Elf32_Ehdr *) base;
	assert(e->e_ehsize == sizeof(*e));
	assert(e->e_shnum != 0);
	s = (Elf32_Shdr *) (base + e->e_shoff);
	for (i = 0; i < e->e_shnum; s++, i++) {
		if (s->sh_type != SHT_PROGBITS)
			continue;
		if (s->sh_addr != 0)
			continue;
		if (fwrite(base + s->sh_offset, 1, s->sh_size, out) != s->sh_size) {
			perror("fwrite");
			exit(1);
		}
		fclose(out);
		return(0);
	}
	return(1);
}
