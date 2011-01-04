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
 * arp_table.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * arp_table.h,v
 * Revision 1.6.4.2.1.2  1994/09/07  04:18:25  menze
 * OSF modifications
 *
 * Revision 1.6.4.2  1994/09/01  18:55:46  menze
 * Meta-data allocations now use Allocators and Paths
 *
 */

typedef struct arpent	**ArpTbl;

/*
 * arpLookup -- Find the ETH host equivalent of the given IP host.
 * If the value is not in the table, network ARP requests will be sent.
 * Returns 0 if the lookup was successful and -1 if it was not.
 */
int	arpLookup(XObj, IPhost *, ETHhost *);

/*
 * arpRevLookup -- Find the IP host equivalent of the given ETH host.
 * If the value is not in the table, network RARP requests will be sent.
 * Returns 0 if the lookup was successful and -1 if it was not.
 */
int	arpRevLookup(XObj, IPhost *, ETHhost *);

/*
 * arpRevLookupTable -- Find the IP host equivalent of the given ETH host.
 * Only looks in the local table, does not send network requests.
 * Returns 0 if the lookup was successful and -1 if it was not.
 */
int	arpRevLookupTable(XObj, IPhost *, ETHhost *);

/*
 * Initialize the arp table.
 */
ArpTbl	arpTableInit( Path );

/*
 * Save the IPhost<->ETHhost binding, releasing any previous bindings
 * that either of these addresses might have had.  Unblocks processes
 * waiting for this binding.  One of ip or eth can be
 * null, in which case the blocked processes will be freed and told that
 * the address could not be resolved
 */
void 	arpSaveBinding(ArpTbl, IPhost *ip, ETHhost *eth);


/* 
 * Remove all entries from the table which are not on the same subnet
 * as the given host.  Entries will be removed regardless of lock
 * status. 
 */
void
arpTblPurge(ArpTbl, IPhost *);


/*
 * arpLock -- lock the entry with the given IP host so the entry always
 * remains in the cache.
 */
void	arpLock(ArpTbl, IPhost *h);


/*
 * arpForEach -- call the function in ArpForEach for each of the
 * entries in the arp table
 */
void
arpForEach(ArpTbl, ArpForEach *);
