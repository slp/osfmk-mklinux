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

#include "stand.h"
#include "saio.h"
#include <sys/param.h>
#include <machine/exec.h>

char *argv[3];

/*ARGSUSED*/
copyunix(howto, devtype, io)
	register howto;
	register devtype;
	register int io;
{
	register int i;
	register char *addr;
	struct header hdr;
	struct som_exec_auxhdr som;

	/*
	 * Read HP-UX file header to find location of AUX header
	 * (which contains entry points and sizes a la exec).
	 */
	i = read(io, &hdr, sizeof(struct header));
	if (i != sizeof(struct header)) {
		printf("Short read on header\n");
		return;
	}
		
	(void) lseek(io, hdr.aux_header_location, SEEK_SET);

	/*
	 * The rest of this should be quasi-standard...
	 */
	i = read(io, (char *)&som, sizeof(struct som_exec_auxhdr));
	if (i != sizeof(struct som_exec_auxhdr)) {
		printf("Short read on SOM header \n");
		return;
	}

	addr = (char *) som.exec_tmem;
	printf("text (0x%x) at 0x%x\n", som.exec_tsize, addr);

	if (lseek(io, som.exec_tfile, SEEK_SET) == -1) {
		printf("Short read on text (0x%x 0x%x 0x%x)\n", 
		       som.exec_tsize, som.exec_tmem, som.exec_tfile);
		return;
	}

	if (read(io, addr, som.exec_tsize) != som.exec_tsize) {
		printf("Short read on text (0x%x 0x%x 0x%x)\n", 
		       som.exec_tsize, som.exec_tmem, som.exec_tfile);
		return;
	}

	addr = (char *) som.exec_dmem;
	printf("data (0x%x) at 0x%x\n", som.exec_dsize, addr);

	if (lseek(io, som.exec_dfile, SEEK_SET) == -1) {
		printf("Short read on data (0x%x 0x%x 0x%x)\n", 
		       som.exec_dsize, som.exec_dmem, som.exec_dfile);
		return;
	}

	if (read(io, addr, som.exec_dsize) != som.exec_dsize) {
		printf("Short read on data (0x%x 0x%x 0x%x)\n", 
		       som.exec_dsize, som.exec_dmem, som.exec_dfile);
		return;
	}

	fcacheall();
	execute(som.exec_entry, argv, 2);
}


