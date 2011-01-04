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

#ifndef _FILE_SYSTEMS_EXTERNS_H_
#define	_FILE_SYSTEMS_EXTERNS_H_

/* convert DEV_BSIZE unit blocks to device records */

#define dbtorec(dev, db)	(((db)*DEV_BSIZE)/(dev)->rec_size)

extern fs_ops_t fs_switch[];

extern boolean_t	strprefix(const char *, const char *);
extern void		ovbcopy(char *, char *, size_t);
extern unsigned int	dev_rec_size(long);

#if	DEBUG
extern	int	debug;
#endif /* DEBUG */

#endif /* _FILE_SYSTEMS_EXTERNS_H_ */
