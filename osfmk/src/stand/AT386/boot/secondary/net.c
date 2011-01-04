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

#include "boot.h"
#include "secondary/net.h"
#include "secondary/net/smc.h"
#include "secondary/net/3c503.h"
#include "secondary/net/dlink.h"
#include "secondary/protocols/udpip.h"
#include "secondary/protocols/bootp.h"
#include "secondary/protocols/tftp.h"

int (*boot_probe[])(void) = {
#ifdef	DEVICE_3C503
	probe_3c503,
#endif
#ifdef	DEVICE_NS8390
	probe_smc,
#endif
	(int (*)(void))0
};

int net_rootoff = 0;
int net_rootunit = 0;
int net_rootpart = 0;
char net_rootdev;

int network_used;

static int net_board_present = 0;

int
net_probe(void)
{
	/* to be initialized only after the bss got cleared
	 * since the dlink data structure is in the bss (?!)
	 */
	static int (**probe)(void);
	static int firsttime = 1;

	network_used = 0;
	if (firsttime) {
		firsttime = 0;
		net_board_present = 0;
		
		for (probe = boot_probe; *probe != (int (*)(void))0; probe++)
			if ((*probe)())
				break;

		if (*probe == (int (*)(void))0) {
			printf("\nNo network device found!\n");
			getchar();
			return 1;
		}
		timerinit();
		udpip_init();
		arp_init();
		tftp_init();
		bootp_init();
       	}
	net_board_present = 1;
	return 0;
}

int
net_open(void)
{
/*
 * dev and unit have a sense here also...
 * Should care which board to use
 */
	if (net_board_present == 0 && net_probe() == 1) {
		printf("\nNo network device present\n");
		return 1;
	}
	PPRINT(("network device opened\n"));
	network_used = 1;
	return 0;
}

int
net_read(void *addr, int count, int phys)
{
	return tftp_engine(TFTP_ENGINE_READ, (char *)0,
				   addr, count);
}

int
net_openfile(const char *filename)
{
	return tftp_engine(TFTP_ENGINE_START, (char *)filename, (char *)0, 0);
}

net_get_root_device()
{
	/* XXX hd(0,a) becomes the standard de facto
	 * when network has been used, unless otherwise
	 * specified (hd(0,f)/mach_servers/... or even
	 * wd(0)/mach_servers/...).
	 * To be cleaned up when bootstrap is able to
	 * cope with the network (/dev/boot_device)
	 */
  	if (default_net_rootpath != (char *)0) {
		const char *cp = default_net_rootpath;
		while (*cp && *cp!='(')
		  	cp++;
		if (!*cp)
			cp = default_net_rootpath;
		else {
			if (cp++ != default_net_rootpath) {
				if (default_net_rootpath[1] != 'd' ||
				    (default_net_rootpath[0] != 'h'
				     && default_net_rootpath[0] != 'f'
				     && default_net_rootpath[0] != 's'
				     && default_net_rootpath[0] != DEV_WD)) {
					goto bad_rootpath;
				}
				dev = default_net_rootpath[0];
			}
			if (*cp >= '0' && *cp <= '9')
				if ((unit = *cp++ - '0') > 1) {
					goto bad_rootpath;
				}
			if (!*cp || (*cp == ',' && !*++cp))
				goto bad_rootpath;
					
			if (*cp >= 'a' && *cp <= 'p')
			  	part = *cp++ - 'a';
			while (*cp && *cp++!=')') ;
		}
	} else {
bad_rootpath:
	  	/* default: boot on hd(0,a) */
	  	dev = 'h';
		unit = 0;
		part = 0;
	}
	printf("root device %cd(%d,%c)\n", dev, unit, 'a'+part);
}





