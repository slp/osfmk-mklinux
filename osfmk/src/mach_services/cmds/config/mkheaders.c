/*
 * Copyright (c) 1995, 1994, 1993, 1992, 1991, 1990  
 * Open Software Foundation, Inc. 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of ("OSF") or Open Software 
 * Foundation not be used in advertising or publicity pertaining to 
 * distribution of the software without specific, written prior permission. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL OSF BE LIABLE FOR ANY 
 * SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN 
 * ACTION OF CONTRACT, NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE 
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * OSF Research Institute MK6.1 (unencumbered) 1/31/1995
 */
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)mkheaders.c	5.9 (Berkeley) 6/25/92";
#endif /* not lint */

/*
 * Make all the .h files for the optional entries
 */

#include <stdio.h>
#include <ctype.h>
#include "config.h"
#include "gram.h"

char	*toheader();	/* forward */

headers()
{
	register struct file_list *fl;

	for (fl = ftab; fl != 0; fl = fl->f_next)
		if (fl->f_needs != 0)
			do_count(fl->f_needs, fl->f_needs, 1);
}

/*
 * count all the devices of a certain type and recurse to count
 * whatever the device is connected to
 */
do_count(dev, hname, search)
	register char *dev, *hname;
	int search;
{
	register struct device *dp, *mp;
	register int count;

	for (count = 0,dp = dtab; dp != 0; dp = dp->d_next) {
		if (dp->d_unit != -1 && eq(dp->d_name, dev)) {
			if (dp->d_type == PSEUDO_DEVICE) {
				count =
				    dp->d_slave != UNKNOWN ? dp->d_slave : 1;
				if (dp->d_flags) /* was this an OPTIONS line */
					dev = NULL;
				break;
			}
			count++;
			/*
			 * Allow holes in unit numbering,
			 * assumption is unit numbering starts
			 * at zero.
			 */
			if (dp->d_unit + 1 > count)
				count = dp->d_unit + 1;
			if (search) {
				mp = dp->d_conn;
				if (mp != 0 && mp != TO_NEXUS &&
				    mp->d_conn != 0 && mp->d_conn != TO_NEXUS) {
                                        /*
					 * Check for the case of the
					 * controller that the device
					 * is attached to is in a separate
					 * file (e.g. "sd" and "sc").
					 * In this case, do NOT define
					 * the number of controllers
					 * in the hname .h file.
					 */
					if (!file_needed(mp->d_name))
						do_count(mp->d_name, hname, 0);
					search = 0;
				}
			}
		}
	}

	/* OPTION/ line specifies "hname"_DYNAMIC define */
	if (dp && dp->d_type == PSEUDO_DEVICE && dp->d_flags == 2)
		do_header(dev, hname, count, 1, dp->d_dynamic == NULL ? 0 : 1);
	else
		do_header(dev, hname, count, 0, 0);
}

/*
 * Scan the file list to see if name is needed to bring in a file.
 */
file_needed(name)
	char *name;
{
	register struct file_list *fl;

	for (fl = ftab; fl != 0; fl = fl->f_next) {
		if (fl->f_needs && strcmp(fl->f_needs, name) == 0)
			return (1);
	}
	return (0);
}

char	headerdata[256];
int	hlen;

do_header(dev, hname, count, isdynamic, definedynamic)
	char *dev;
	char *hname;
	int count;
	int isdynamic;
	int definedynamic;
{
	char *name, *tomacro();
	extern void build_header();

	if (dev) {
		name = tomacro(dev, 1);		/* PSUEDO_DEVICE define */
	} else {
		name = tomacro(hname, 0);	/* OPTION/ define */
	}
	hlen = sprintf(&headerdata[0],"#define %s %d\n", name, count);

	if (isdynamic) {
		hlen += sprintf(&headerdata[hlen], "#define %s_DYNAMIC %d\n",
			tomacro(hname,0), definedynamic);
	}
	do_paramfile(toheader(hname), headerdata, hlen);
}

/*
 *  Build a define parameter file.  First determine if the new contents
 *  differ from the old before actually replacing the original (so as not
 *  to introduce avoidable extraneous compilations).
 */

do_paramfile(name, data)
	char *name;
	char *data;
{
	char *fname;
	char *cp = data;
	FILE *ofp;
	int c;

	fname = path(name);
	ofp = fopen(fname, "r");
	if (ofp != 0)
	{
		while ((c = *cp++) != '\0')
			if (fgetc(ofp) != c)
				goto copy;
		if (fgetc(ofp) == EOF) {
			if (do_trace) printf("Identical header %s\n", fname);
			goto same;
		}
	}
copy:
	if (do_trace) printf("Writing header %s\n", fname);

	if (ofp != 0)
		fclose(ofp);

	ofp = fopen(fname, "w");
	if (ofp == 0) {
		perror(fname);
		free(fname);
		exit(1);
	}
	fputs(data, ofp);
same:
	fclose(ofp);
	free(fname);
}

/*
 * convert a dev name to a .h file name. Returns pointer to static
 * data that is overwritten each time it it called.
 */
char *
toheader(dev)
	char *dev;
{
	static char hbuf[80];

	(void) strcpy(hbuf, dev);
	(void) strcat(hbuf, ".h");
	return (hbuf);
}

/*
 * convert a dev name to a macro name
 */
char *tomacro(dev, isdev)
	register char *dev;
	register int isdev;
{
	static char mbuf[64];
	register char *cp;

	cp = mbuf;
	if (isdev)
		*cp++ = 'N';
	while (*dev)
		if (!islower(*dev))
			*cp++ = *dev++;
		else
			*cp++ = toupper(*dev++);
	*cp++ = 0;
	return (mbuf);
}
