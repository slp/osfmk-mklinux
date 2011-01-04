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

#include <device/cdli.h>
#include <sys/errno.h>
#include <kern/lock.h>
#include <vm/vm_kern.h>
#include <i386/AT386/if_ns8390_entries.h>
#define	ns8390wdname		"ci0"

struct dev_ops	cdli_ns8390_dops =
{
	/*name,		open,		close,		read,
	  write,	getstat,	setstat,	mmap,
	  async_in,	reset,		port_death,	subdev,
	  dev_info */
	  ns8390wdname,	wd8003open,	NULL_CLOSE,	NULL_READ,
	  ns8390output, ns8390getstat,	ns8390setstat,	NULL_MMAP,
	  ns8390setinput, NULL_RESET,	NULL_DEATH, 	0,
	  NO_DINFO
};

int cdli_ns8390_open(struct ndd *);
int cdli_ns8390_close(struct ndd *);
int cdli_ns8390_output(struct ndd *, caddr_t);
int cdli_ns8390_ctl(struct ndd *, int, caddr_t, int);

int
cdli_ns8390_open(struct ndd *nddp)
{
	return(KERN_SUCCESS);
}

int
cdli_ns8390_close(struct ndd *nddp)
{
	return(KERN_SUCCESS);
}

int
cdli_ns8390_output(struct ndd *nddp, caddr_t data)
{
	return(KERN_SUCCESS);
}

int
cdli_ns8390_ctl(struct ndd *nddp, int x, caddr_t addr, int y)
{
	return(KERN_SUCCESS);
}

int
cdli_ns8390init()
{
	struct ndd *nddp;
	struct ns_8022 *filter;
	struct ns_user *ns_user;
	int	ret;

	printf("%s initializing pseudo-CDLI ns8390 driver\n", ns8390wdname);

	if (!(nddp = (struct ndd *)kalloc(sizeof(struct ndd))))
	{
		return(ENOMEM);
	}

	bzero((char *)nddp, sizeof(*nddp));
	nddp->ndd_name = ns8390wdname;
	nddp->ndd_unit = 0;
	nddp->ndd_type = NDD_ISO88023;
	nddp->d_ops = cdli_ns8390_dops;
	nddp->ndd_open = cdli_ns8390_open;
	nddp->ndd_close = cdli_ns8390_close;
	nddp->ndd_output = cdli_ns8390_output;
	nddp->ndd_ctl = cdli_ns8390_ctl;

	(void) dmx_init(nddp);

	if (ret = ns_attach(nddp)) /* Add network device driver */
	{
		goto bad;
	}

	if (!(filter = (struct ns_8022 *)kalloc(sizeof(*filter))))
	{
		return(ENOMEM);
	}

	bzero((char *)filter, sizeof(*filter));
	filter->filtertype = NS_STATUS_MASK;
	filter->dsap = 0x0;
	filter->orgcode[0] = '\0';
	filter->orgcode[1] = '\0';
	filter->orgcode[2] = '\0';
	filter->ethertype = NS_ETHERTYPE;

	eth_add_demuxer(nddp->ndd_type);

	if (!(ns_user = (struct ns_user *)kalloc(sizeof(struct ns_user))))
	{
		return(ENOMEM);
	}

	bzero((char *)ns_user, sizeof(struct ns_user));

	ns_user->isr = 0;

	if (ret = dmx_8022_add_filter(nddp, filter, ns_user)) /* Add demuxing filter */
	{
		goto bad;
	}

	ret = ns_alloc(nddp->ndd_name, &nddp);
 bad:
	return(ret);
}
