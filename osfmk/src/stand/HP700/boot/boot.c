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
#include <sys/param.h>
#include "stand.h"
#include "reboot.h"

extern	unsigned opendev;
u_int howto = 0;
u_int endaddr = 0;
u_int rstaddr = 0;
u_int bootdev = 0;

extern  char *argv[];
extern char etext;
extern char edata;
extern char end;

main()
{
	extern int btunit();
	int io;
	char line[256];
	char kernel[32];
	int kname;

	getbinfo();

	printf("OSF Mach boot\n");

	if (bootdev == 0 || B_TYPE(bootdev) >= ndevs) {	/* use default */
		extern int sddevix;
		bootdev = MAKEBOOTDEV(sddevix, 0, 0, btunit(), 0);
	}

	files_init();

	strcpy(kernel, MACH);

	while(1) {
		argv[2] = '\0';
		kname = 0;

		if (howto & RB_ASKNAME) {
			register int donehow = 0;
			char *c, *ck;

			if (*kernel)
				printf("[%s]", kernel);
			printf(": ");
			gets(line);
			
			for(c = line, ck = kernel; *c != '\0'; c++, ck++) {
				if(*c == '-') {
					argv[2] = ++c;		
					break;
				}
				if(*c == ' ') {
					if(kname)
						*ck = 0;
				}
				else {
					*ck = *c;
					kname = 1;
				}
			}
			
			if(kname)
				*ck = 0;
		} 
		else 
			printf(": %s\n", kernel);
		
		io = open(kernel, 0);

		if (io >= 0) {
			fcacheall();
			copyunix(howto, opendev, io);
			close(io);
		}
		else
			printf("open %s failed\n", kernel);

		*kernel = '\0';
		howto |= RB_ASKNAME;
	}
}
	
