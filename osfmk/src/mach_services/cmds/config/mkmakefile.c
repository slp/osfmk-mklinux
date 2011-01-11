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
 * Copyright (c) 1989,1988,1987 Carnegie Mellon University
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
 * Copyright (c) 1980,1990 Regents of the University of California.
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
static char sccsid[] = "@(#)mkmakefile.c	5.39 (Berkeley) 7/31/92";
#endif /* not lint */

/*
 * Build the makefile for the system, from
 * the information in the files files and the
 * additional files for the machine being compiled to.
 */

#include <stdio.h>
#include <ctype.h>
#include "gram.h"
#include "config.h"

#define next_word(fp, wd) \
	{ register char *word = get_word(fp); \
	  if (word == (char *)EOF) \
		return; \
	  else \
		wd = word; \
	}
#define next_quoted_word(fp, wd) \
	{ register char *word = get_quoted_word(fp); \
	  if (word == (char *)EOF) \
		return; \
	  else \
		wd = word; \
	}

static	struct file_list *fcur;
char *tail();

/*
 * Lookup a file, by name.
 */
struct file_list *
fl_lookup(file)
	register char *file;
{
	register struct file_list *fp;

	for (fp = ftab ; fp != 0; fp = fp->f_next) {
		if (eq(fp->f_fn, file))
			return (fp);
	}
	return (0);
}

/*
 * Lookup a file, by final component name.
 */
struct file_list *
fltail_lookup(file)
	register char *file;
{
	register struct file_list *fp;

	for (fp = ftab ; fp != 0; fp = fp->f_next) {
		if (eq(tail(fp->f_fn), tail(file)))
			return (fp);
	}
	return (0);
}

static struct file_list zeroed_fent;

/*
 * Make a new file list entry
 */
struct file_list *
new_fent()
{
	register struct file_list *fp;

	fp = (struct file_list *) malloc(sizeof *fp);
	*fp = zeroed_fent;
	if (fcur == 0)
		fcur = ftab = fp;
	else
		fcur->f_next = fp;
	fcur = fp;
	return (fp);
}

extern char *search_path;

/*
 * Build the makefile from the skeleton
 */
makefile()
{
	FILE *ifp, *ofp;
	char line[BUFSIZ];
	struct opt *op;
	char *mfile, *to;
	extern int build_cputypes();
	extern int build_confdep();

	read_files();
	(void) sprintf(line, "%s/template.mk", config_directory);
	ifp = VPATHopen(line, "r");
	if (ifp == 0) {
		perror(line);
		exit(1);
	}
	if (makedepends) fprintf(dependfile, " \\\n\t%s", line);

	mfile = path("Makefile");
	to = path("Makefile.old");
	rename(mfile, to);
	free(to);

	ofp = fopen(mfile, "w");
	if (ofp == 0) {
		perror(mfile);
		free(mfile);
		exit(1);
	}
	free(mfile);

	if (cputype == 0) {
		printf("cpu type must be specified\n");
		exit(1);
	}
	do_build("cputypes.h", build_cputypes);
	fprintf(ofp, "IDENT=-D%s -D%s", machinename, raise(ident));
	for (op = opt; op; op = op->op_next)
		if (op->op_value)
			fprintf(ofp, " -D%s=\"%s\"", op->op_name, op->op_value);
		else
			fprintf(ofp, " -D%s", op->op_name);
	fprintf(ofp, "\n");
	for (op = mkopt; op; op = op->op_next)
		fprintf(ofp, "%s=%s\n", op->op_name, op->op_value);
	if (hadtz == 0)
		printf("timezone not specified; gmt assumed\n");
	if (maxusers == 0) {
		printf("maxusers not specified; 8 assumed\n");
		maxusers = 8;
	}
	do_build("confdep.h", build_confdep);
	while (fgets(line, BUFSIZ, ifp) != 0) {
		if (*line != '%') {
			fprintf(ofp, "%s", line);
			continue;
		}
		if (eq(line, "%OBJS\n")) {
			do_objs(ofp, "OBJS=", -1, 0);
			continue;
		}
		if (eq(line, "%CFILES\n")) {
			do_files(ofp, 'c', 0);
			do_objs(ofp, "COBJS=", 'c', 0);
			continue;
		}
		if (eq(line, "%SFILES\n")) {
			do_files(ofp, 's', 1);
			do_objs(ofp, "SOBJS=", 's', 1);
			continue;
		}
		if (eq(line, "%BFILES\n")) {
			do_files(ofp, 'b', 0);
			do_objs(ofp, "BOBJS=", 'b', 0);
			continue;
		}
		if (eq(line, "%MACHDEP\n")) {
			do_machdep(ofp);
			continue;
		}
		if (eq(line, "%ORDERED\n")) {
			do_ordered(ofp);
			continue;
		}
		if (eq(line, "%DYNAMIC\n")) {
			do_dynamic(ofp);
			continue;
		}
		if (eq(line, "%CFLAGS\n")) {
			do_cflags(ofp);
			continue;
		}
		if (eq(line, "%LOAD\n")) {
			do_load(ofp);
			continue;
		}
		fprintf(stderr,
			"Unknown %% construct in generic makefile: %s",
			line);
	}
	(void) fclose(ifp);
	(void) fclose(ofp);
}

struct dynlist {
	char *target;
	char *objects;
	struct dynlist *next;
};

struct dynlist *dynamic_targets;

save_dynamic(this, target)
char *this, *target;
{
	struct dynlist *d;
	int len;
	char *ptr, *sp, *cp;
	char och;

	for (d = dynamic_targets; d != NULL; d = d->next) {
		if (!eq(d->target, target))
			continue;
		sp = tail(this);
		cp = sp + (len = strlen(sp)) - 1;
		och = *cp;
		*cp = 'o';
		len = strlen(d->objects) + 1 + len + 1;
		ptr = realloc(d->objects, len);
		if (ptr == NULL) {
			fprintf(stderr, "objects realloc failed\n");
			exit(1);
		}
		strcat(ptr, " ");
		strcat(ptr, sp);
		d->objects = ptr;
		*cp = och;
		return;
	}
	d = (struct dynlist *)malloc(sizeof(struct dynlist));
	if (d == NULL) {
		fprintf(stderr, "dynlist malloc failed\n");
		exit(1);
	}
	d->next = dynamic_targets;
	dynamic_targets = d;
	d->target = ns(target);
	if (d->target == NULL) {
		fprintf(stderr, "target ns failed\n");
		exit(1);
	}
	sp = tail(this);
	cp = sp + strlen(sp) - 1;
	och = *cp;
	*cp = 'o';
	d->objects = ns(sp);
	if (d->objects == NULL) {
		fprintf(stderr, "objects ns failed\n");
		exit(1);
	}
	*cp = och;
}

struct file_state {
	char *fs_fname;		/* filename we are reading */
	FILE *fs_fp;		/* open file pointer */
	char *fs_wd;		/* next word in the input stream */
	char *fs_this;		/* entry that we are now processing */
	char *fs_dynamic;	/* dynamic module to be linked into */
	char *fs_special;	/* special rule to use for compilation */
	struct device *fs_dev;	/* device when this is an option */
	int fs_type;		/* type of entry to create */
	int fs_flags;		/* flags for entry */
	int fs_define_dynamic;	/* want _DYNAMIC option for device */
} file_state;

/*
 * Read in the information about files used in making the system.
 * Store it in the ftab linked list.
 */
read_files()
{
	char fnamebuf[1024];
	struct file_state fs;

	dynamic_targets = NULL;
	ftab = 0;
	fs.fs_fname = fnamebuf;
	(void) sprintf(fs.fs_fname, "%s/files", config_directory);
	fs.fs_fp = VPATHopen(fs.fs_fname, "r");
	if (fs.fs_fp == 0) {
		perror(fs.fs_fname);
		exit(1);
	}
	if (makedepends) fprintf(dependfile, " \\\n\t%s", fs.fs_fname);

	for (;;) {
		if (read_file(&fs, 1))
			break;
	}
	(void) fclose(fs.fs_fp);
	(void) sprintf(fs.fs_fname, "%s/%s/files",
		       config_directory, machinename);
	fs.fs_fp = VPATHopen(fs.fs_fname, "r");
	if (fs.fs_fp == 0) {
		perror(fs.fs_fname);
		exit(1);
	}
	if (makedepends) fprintf(dependfile, " \\\n\t%s", fs.fs_fname);

	for (;;) {
		if (read_file(&fs, 2))
			break;
	}
	(void) sprintf(fs.fs_fname, "files.%s", raise(ident));
	fs.fs_fp = VPATHopen(fs.fs_fname, "r");
	if (fs.fs_fp == 0)
		return;
	if (makedepends) fprintf(dependfile, " \\\n\t%s", fs.fs_fname);

	for (;;) {
		if (read_file(&fs, 3))
			break;
	}
}

/*
 * filename [ standard | optional ]
 *	[ config-dependent | ordered | sedit ]
 *	[ if_dynamic <module> | only_dynamic <module> ]
 *	[ dev* | profiling-routine ] [ device-driver]
 *	[ compile-with "compile rule" ]
 */
int
read_file(fs, pass)
struct file_state *fs;
int pass;
{
	struct file_list *tp, *pf;
	char *rest;
	char *needs;
	int isdup;

	fs->fs_wd = get_word(fs->fs_fp);
	if (fs->fs_wd == (char *)EOF)
		return(1);
	if (fs->fs_wd == 0)
		return(0);
	if (*fs->fs_wd == '#') {
		while (fs->fs_wd = get_word(fs->fs_fp))
			if (fs->fs_wd == (char *)EOF)
				break;
		return(0);
	}
	fs->fs_this = ns(fs->fs_wd);
	next_word(fs->fs_fp, fs->fs_wd);
	if (fs->fs_wd == 0) {
		printf("%s: No type for %s.\n", fs->fs_fname, fs->fs_this);
		exit(1);
	}
	if ((pf = fl_lookup(fs->fs_this)) &&
	    (pf->f_type != INVISIBLE || pf->f_flags))
		isdup = 1;
	else
		isdup = 0;
	tp = 0;
	if (pass == 3 && (tp = fltail_lookup(fs->fs_this)) != 0)
		printf("%s: Local file %s overrides %s.\n",
		    fs->fs_fname, fs->fs_this, tp->f_fn);
	fs->fs_dynamic = (char *)0;
	fs->fs_dev = (struct device *)0;
	fs->fs_type = NORMAL;
	fs->fs_flags = SEDIT;
	fs->fs_define_dynamic = 0;
	rest = (char *)0;
	needs = (char *)0;
	if (eq(fs->fs_wd, "standard")) {
		parse_standard(fs, &rest);
		save(fs, tp, pf, rest, needs);
		return(0);
	}
	if (!eq(fs->fs_wd, "optional")) {
		printf("%s: %s must be optional or standard\n",
		       fs->fs_fname, fs->fs_this);
		exit(1);
	}
	if (parse_optional(fs, &rest, &needs, isdup)) {
		/* save this file entry */
		save(fs, tp, pf, rest, needs);
		return(0);
	}
	/* mark the file invisible */
	while ((fs->fs_wd = get_word(fs->fs_fp)) != 0)
		;
	if (tp == 0)
		tp = new_fent();
	tp->f_fn = fs->fs_this;
	tp->f_type = INVISIBLE;
	tp->f_needs = needs;
	tp->f_flags = isdup;
	return(0);
}

save(fs, tp, pf, rest, needs)
struct file_state *fs;
struct file_list *tp, *pf;
char *rest, *needs;
{
	if (fs->fs_define_dynamic && fs->fs_dev)
		fs->fs_dev->d_flags++;
	if (fs->fs_dynamic && !want_dynamic(fs))
		return;
	if (tp == 0)
		tp = new_fent();
	tp->f_fn = fs->fs_this;
	tp->f_extra = rest;
	tp->f_type = fs->fs_type;
	tp->f_flags = fs->fs_flags;
	tp->f_needs = needs;
	if (pf && pf->f_type == INVISIBLE)
		pf->f_flags = 1;		/* mark as duplicate */
	return;
}

want_dynamic(fs)
struct file_state *fs;
{
	register struct device *dp;

	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if (dp->d_dynamic == 0)
			continue;
		if (dp->d_type == PSEUDO_DEVICE && dp->d_slave == 0)
			continue;
		if (eq(dp->d_dynamic, fs->fs_dynamic)) {
			save_dynamic(fs->fs_this, fs->fs_dynamic);
			fs->fs_flags |= (DYNAMICF | ORDERED);
			return(1);
		}
	}
	if (fs->fs_flags & DYNAMICF)	/* only_dynamic */
		return(0);
	return(1);
}

parse_standard(fs, restp)
struct file_state *fs;
char **restp;
{
	for (;;) {
		next_word(fs->fs_fp, fs->fs_wd);
		if (fs->fs_wd == 0)
			return;
		if (!parse_opt(fs))
			break;
	}
	if (eq(fs->fs_wd, "device-driver"))
		fs->fs_type = DRIVER;
	else if (eq(fs->fs_wd, "profiling-routine"))
		fs->fs_type = PROFILING;
	next_word(fs->fs_fp, fs->fs_wd);
	if (fs->fs_wd) {
		if (*fs->fs_wd != '|') {
			printf("%s: syntax error describing %s\n",
			       fs->fs_fname, fs->fs_this);
			exit(1);
		}
		*restp = ns(get_rest(fs->fs_fp));
	}
	return;
}

parse_optional(fs, restp, needsp, isdup)
struct file_state *fs;
char **restp;
char **needsp;
int isdup;
{
	int options;
	int driver;
	int not_option = 0;
	int nreqs = 0;

	options = (strncmp(fs->fs_this, "OPTIONS/", 8) == 0);
	if (options)
		fs->fs_type = INVISIBLE;
	for (;;) {
		next_word(fs->fs_fp, fs->fs_wd);
		if (fs->fs_wd == 0) {
			if (nreqs == 0) {
				printf("%s: what is %s optional on?\n",
				       fs->fs_fname, fs->fs_this);
				exit(1);
			}
			return(1);
		}
		if (parse_opt(fs))
			continue;
		if (eq(fs->fs_wd, "not")) {
			not_option = !not_option;
			continue;
		}
		if ((driver = eq(fs->fs_wd, "device-driver")) ||
		    eq(fs->fs_wd, "profiling-routine")) {
			if (!options) {
				if (driver)
					fs->fs_type = DRIVER;
				else
					fs->fs_type = PROFILING;
			}
			next_word(fs->fs_fp, fs->fs_wd);
			if (fs->fs_wd && *fs->fs_wd != '|') {
				printf("%s: syntax error describing %s\n",
				       fs->fs_fname, fs->fs_this);
				exit(1);
			}
			if (fs->fs_wd)
				*restp = ns(get_rest(fs->fs_fp));
			return(1);
		}
		nreqs++;
		if (*needsp == 0 && nreqs == 1)
			*needsp = ns(fs->fs_wd);
		if (isdup)
			return(0);
		if (options)
			add_option(fs);
		if (!want_request(fs, not_option, needsp, nreqs))
			return(0);
	}
}

#define LPAREN	"("
#define	RPAREN	")"

want_request(fs, not_option, needsp, nreqs)
struct file_state *fs;
int not_option;		/* want file if option missing */
char **needsp;
int nreqs;
{
	register struct device *dp;
	register struct opt *op;
	struct cputype *cp;
	int expression = 0;
	char *word;
	int value;
	int greater;

	if (eq(fs->fs_wd, LPAREN)) {
		expression = 1;
		next_word(fs->fs_fp, fs->fs_wd);
		if (!fs->fs_wd || (fs->fs_wd == (char *)EOF)) {
			printf("%s: syntax error in expression describing %s\n",
				fs->fs_fname, fs->fs_this);
			exit (1);
		}
		if (nreqs == 1) {
			free(*needsp);
			*needsp = ns(fs->fs_wd);
		}
	}
	word = ns(fs->fs_wd);

	if (expression) {
		char *next;
		next_word(fs->fs_fp, next);
		if (!next || (next == (char *)EOF)) {
			printf("%s: syntax error in expression describing %s\n",
				fs->fs_fname, fs->fs_this);
			printf("expecting '<' or '>', found end-of-line\n");
			exit (1);
		}
		switch (*next) {
		case '<': greater = 0; break;
		case '>': greater = 1; break;
		default:
			printf("%s: syntax error in expression describing %s -",
				fs->fs_fname, fs->fs_this);
			printf("expecting '<' or '>', found %c\n", *next);
			exit (1);
		}
		next_word(fs->fs_fp, next);
		if (!next || (next == (char *)EOF)) {
			printf("%s: syntax error in expression describing %s\n",
				fs->fs_fname, fs->fs_this);
			exit (1);
		}
		value = atoi(next);
		next_word(fs->fs_fp, next);
		if (!next || (next == (char *)EOF) || !eq(next,RPAREN)) {
			printf("%s: syntax error in expression describing %s\n",
				fs->fs_fname, fs->fs_this);
			printf("expecting '%s', found %s\n", RPAREN, next);
			exit (1);
		}
	}

 	for (dp = dtab; dp != 0; dp = dp->d_next)
		if (eq(dp->d_name, word)) {
		    if ((dp->d_type != PSEUDO_DEVICE)
			|| (!expression && dp->d_slave)
			|| (expression && greater && (dp->d_slave > value))
			|| (expression && !greater && (dp->d_slave < value))) {
			free(word);
			return(!not_option);
		    }
		}
	for (op = opt; op != 0; op = op->op_next)
		if (op->op_value == 0 && opteq(op->op_name, word)) {
			if (nreqs == 1) {
				free(*needsp);
				*needsp = 0;
			}
			free(word);
			return(!not_option);
		}
	for (cp = cputype; cp; cp = cp->cpu_next)
		if (opteq(cp->cpu_name, word)) {
			if (nreqs == 1) {
				free(*needsp);
				*needsp = 0;
			}
			free(word);
			return(!not_option);
		}
	free(word);
	return(not_option);
}

add_option(fs)
struct file_state *fs;
{
	register struct opt *op;
	struct opt *lop = 0;
	struct device tdev;
	char *od;
	extern struct  device * curp;


	/*
	 *  Allocate a pseudo-device entry which we will insert into
	 *  the device list below.  The flags field is set non-zero to
	 *  indicate an internal entry rather than one generated from
	 *  the configuration file.  The slave field is set to define
	 *  the corresponding symbol as 0 should we fail to find the
	 *  option in the option list.
	 */
	init_dev(&tdev);
	tdev.d_name = ns(fs->fs_wd);
	tdev.d_type = PSEUDO_DEVICE;
	tdev.d_flags++;
	tdev.d_slave = 0;
	for (op=opt; op; lop=op, op=op->op_next) {
		/*
		 *  Found an option which matches the current device
		 *  dependency identifier.  Set the slave field to
		 *  define the option in the header file.
		 */
		od = raise(ns(fs->fs_wd));
		if (strcmp(op->op_name, od) == 0) {
			if (op->op_value)
				tdev.d_slave = atoi(op->op_value);
			else
				tdev.d_slave = 1;
			tdev.d_dynamic = op->op_dynamic;
			if (lop == 0)
				opt = op->op_next;
			else
				lop->op_next = op->op_next;
			free(op);
			op = 0;
		}
		free(od);
		if (op == 0)
			break;
	}
	newdev(&tdev);
	fs->fs_dev = curp;
}

parse_opt(fs)
struct file_state *fs;
{
	if (eq(fs->fs_wd, "config-dependent")) {
		fs->fs_flags |= CONFIGDEP;
		return(1);
	}
	if (eq(fs->fs_wd, "compile-with")) {
		next_quoted_word(fs->fs_fp, fs->fs_wd);
		if (fs->fs_wd == 0) {
			printf("%s: %s missing compile command string.\n",
			       fs->fs_fname);
			exit(1);
		}
		fs->fs_special = ns(fs->fs_wd);
		return(1);
	}
	if (eq(fs->fs_wd, "ordered")) {
		fs->fs_flags |= ORDERED;
		return(1);
	}
	if (eq(fs->fs_wd, "sedit")) {
		fs->fs_flags |= SEDIT;
		return(1);
	}
	if (eq(fs->fs_wd, "define_dynamic")) {
		fs->fs_define_dynamic++;
		return(1);
	}
	if (eq(fs->fs_wd, "if_dynamic")) {
		next_word(fs->fs_fp, fs->fs_wd);
		if (fs->fs_wd == 0) {
			printf("%s: missing if_dynamic module for %s\n",
			       fs->fs_fname, fs->fs_this);
			exit(1);
		}
		fs->fs_dynamic = ns(fs->fs_wd);
		return(1);
	}
	if (eq(fs->fs_wd, "only_dynamic")) {
		fs->fs_flags |= DYNAMICF;
		next_word(fs->fs_fp, fs->fs_wd);
		if (fs->fs_wd == 0) {
			printf("%s: missing only_dynamic module for %s\n",
			       fs->fs_fname, fs->fs_this);
			exit(1);
		}
		fs->fs_dynamic = ns(fs->fs_wd);
		return(1);
	}
	return(0);
}

opteq(cp, dp)
	char *cp, *dp;
{
	char c, d;

	for (; ; cp++, dp++) {
		if (*cp != *dp) {
			c = isupper(*cp) ? tolower(*cp) : *cp;
			d = isupper(*dp) ? tolower(*dp) : *dp;
			if (c != d)
				return (0);
		}
		if (*cp == 0)
			return (1);
	}
}

do_objs(fp, msg, ext, anycase)
	FILE	*fp;
	char	*msg;
	int	ext;
	int	anycase;
{
	register struct file_list *tp, *fl;
	register int lpos, len;
	register char *cp, och, *sp;
	char swapname[32];

	fprintf(fp, msg);
	lpos = strlen(msg);
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if (tp->f_type == INVISIBLE)
			continue;

		/*
		 *	Check for '.o' file in list
		 */
		cp = tp->f_fn + (len = strlen(tp->f_fn)) - 1;
		if ((ext == -1 && (tp->f_flags & ORDERED)) || /* not in objs */
		    (ext != -1 && *cp != ext &&
		     (!anycase || *cp != toupper(ext))))
			continue;
		else if (*cp == 'o') {
			if (len + lpos > 72) {
				lpos = 8;
				fprintf(fp, "\\\n\t");
			}
			fprintf(fp, "%s ", tp->f_fn);
			fprintf(fp, " ");
			lpos += len + 1;
			continue;
		}
		sp = tail(tp->f_fn);
		for (fl = conf_list; fl; fl = fl->f_next) {
			if (fl->f_type != SWAPSPEC)
				continue;
			(void) sprintf(swapname, "swap%s.c", fl->f_fn);
			if (eq(sp, swapname))
				goto cont;
		}
		cp = sp + (len = strlen(sp)) - 1;
		och = *cp;
		*cp = 'o';
		if (len + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "%s ", sp);
		lpos += len + 1;
		*cp = och;
cont:
		;
	}
	if (lpos != 8)
		putc('\n', fp);
}

/* not presently used and probably broken,  use ORDERED instead */
do_ordered(fp)
	FILE *fp;
{
	register struct file_list *tp, *fl;
	register int lpos, len;
	register char *cp, och, *sp;

	fprintf(fp, "ORDERED=");
	lpos = 10;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if ((tp->f_flags & ORDERED) != ORDERED)
			continue;
		if ((tp->f_flags & DYNAMICF) == DYNAMICF)
			continue;
		sp = tail(tp->f_fn);
		cp = sp + (len = strlen(sp)) - 1;
		och = *cp;
		*cp = 'o';
		if (len + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "%s ", sp);
		lpos += len + 1;
		*cp = och;
cont:
		;
	}
	if (lpos != 8)
		putc('\n', fp);
}

do_dynamic(fp)
	FILE *fp;
{
	register struct dynlist *dp;
	register int lpos, len;
	register char *sp;
	char *	ns;

	fprintf(fp, "DYNAMIC=");
	lpos = 10;
	for (dp = dynamic_targets; dp != 0; dp = dp->next) {
		sp = dp->target;
		len = strlen(sp);
		if (len + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "%s ", sp);
		lpos += len + 1;
	}
	if (lpos != 8)
		putc('\n', fp);
	fprintf(fp, "\n");
	ns = sympref ? sympref : "";
	for (dp = dynamic_targets; dp != 0; dp = dp->next) {
		fprintf(fp, "%s${DYNAMIC_MODULE}_OBJECTS=%s\n",
			dp->target, dp->objects);
		fprintf(fp, "%s${DYNAMIC_MODULE}_ENTRY=%s%s_configure\n",
			dp->target, ns, dp->target);
		fprintf(fp, "%s${DYNAMIC_MODULE}_IDIR=${DYNAMIC_IDIR}\n",
			dp->target);
	}
}

do_files(fp, ext, anycase)
	FILE	*fp;
	char	ext;
	int	anycase;
{
	register struct file_list *tp;
	register int len;
	register char *cp, och, *sp, *np;

	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if (tp->f_type == INVISIBLE)
			continue;
		cp = (np = tp->f_fn) + (len = strlen(tp->f_fn)) - 1;
		if (*cp != ext && (!anycase || *cp != toupper(ext)))
			continue;
		if (np[0] == '.' && np[1] == '/')
			np += 2;
		sp = tail(np);
		och = *cp;
		*cp = 'o';
		fprintf(fp, "%s_SOURCE=", sp);
		*cp = och;
		fprintf(fp, "%s\n", np);
	}
	putc('\n', fp);
}

/*
 *  Include machine dependent makefile in output
 */

do_machdep(ofp)
	FILE *ofp;
{
	int c;
	FILE *ifp;
	char line[BUFSIZ];

	(void) sprintf(line, "%s/%s/template.mk",
		       config_directory, machinename);
	ifp = VPATHopen(line, "r");
	if (ifp == 0) {
		perror(line);
		exit(1);
	}
	if (makedepends) fprintf(dependfile, " \\\n\t%s", line);

	while (fgets(line, BUFSIZ, ifp) != 0)
		fputs(line, ofp);
	fclose(ifp);
}



/*
 *  Format configuration dependent parameter file.
 */

build_confdep(fp)
	FILE *fp;
{
	fprintf(fp,
	    "#define TIMEZONE %d\n#define MAXUSERS %d\n#define DST %d\n",
	    zone, maxusers, dst);
	fprintf(fp,
	    "#define MAXDSIZ 0x%x\n", maxdsiz);
}



/*
 *  Format cpu types file.
 */

build_cputypes(fp)
	FILE *fp;
{
	struct cputype *cp;

	for (cp = cputype; cp; cp = cp->cpu_next)
		fprintf(fp, "#define\t%s\t1\n", cp->cpu_name);
}



/*
 *  Build a define parameter file.  Create it first in a temporary location and
 *  determine if this new contents differs from the old before actually
 *  replacing the original (so as not to introduce avoidable extraneous
 *  compilations).
 */

do_build(name, format)
	char *name;
	int (*format)();
{
	static char temp[]="#config.tmp";
	char *tname, *fname;
	FILE *tfp, *ofp;
	int c;

	tname = path(temp);
	tfp = fopen(tname, "w+");
	if (tfp == 0) {
		perror(tname);
		free(tname);
		exit(1);
	}
	if (unlink(tname) < 0) {
		perror("unlink");
		free(tname);
		exit(1);
	}
	free(tname);
	(*format)(tfp);
	fname = path(name);
	ofp = fopen(fname, "r");
	if (ofp != 0)
	{
		fseek(tfp, 0, 0);
		while ((c = fgetc(tfp)) != EOF)
			if (fgetc(ofp) != c)
				goto copy;
		if (fgetc(ofp) == EOF)
			goto same;
		
	}
copy:
	if (ofp)
		fclose(ofp);
	ofp = fopen(fname, "w");
	if (ofp == 0) {
		perror(fname);
		free(fname);
		exit(1);
	}
	fseek(tfp, 0, 0);
	while ((c = fgetc(tfp)) != EOF)
		fputc(c, ofp);
same:
	fclose(ofp);
	fclose(tfp);
	free(fname);
}

char *
tail(fn)
	char *fn;
{
	register char *cp;

	cp = strrchr(fn, '/');
	if (cp == 0)
		return (fn);
	return (cp+1);
}

/*
 * Create any special cflags for each file which is part of the system.
 */
do_cflags(f)
	FILE *f;
{
	register char *cp, *np, och, *tp;
	register struct file_list *ftp;

	for (ftp = ftab; ftp != 0; ftp = ftp->f_next) {
		if (ftp->f_type == INVISIBLE)
			continue;
		cp = (np = ftp->f_fn) + strlen(ftp->f_fn) - 1;
		och = *cp;
		if (och == 'o')
			continue;
		if (np[0] == '.' && np[1] == '/')
			np += 2;
		*cp = 'o';
		tp = tail(np);
		if (ftp->f_extra)
			fprintf(f, "%s_CFLAGS+=%s\n", tp, ftp->f_extra);
		if (ftp->f_special) {
			fprintf(f, "%s: ", tp);
			*cp = och;
			fprintf(f, "%s\n\t%s\n\n", tail(np), ftp->f_special);
			continue;
		}
		if (och == 's' || och == 'b') {
			*cp = och;
			continue;
		}
		if (ftp->f_flags & DYNAMICF) {
			fprintf(f, "%s_CFLAGS+=${DYNAMIC_CFLAGS}\n", tp);
			fprintf(f, "%s_DYNAMIC=\n", tp);
		}
		if (ftp->f_type == DRIVER) {
			fprintf(f, "%s_CFLAGS+=${DRIVER_CFLAGS}\n", tp);
			fprintf(f, "%s_DRIVER=\n", tp);
		} else if (ftp->f_type == PROFILING) {
			fprintf(f, "%s_CFLAGS+=${PROFILING_CFLAGS}\n", tp);
			fprintf(f, "%s_PROFILING=\n", tp);
		} else if (ftp->f_type != NORMAL)
			printf("Don't know rules for %s\n", np);
		*cp = och;
	}
}

/*
 * Create the load strings
 */
do_load(f)
	register FILE *f;
{
	register struct file_list *fl;
	register struct dynlist *dp;

	fl = conf_list;
	while (fl) {
		if (fl->f_type != SYSTEMSPEC) {
			fl = fl->f_next;
			continue;
		}
		if (fl->f_fn)
			do_swapspec(f, fl->f_fn, fl->f_needs);
		for (fl = fl->f_next;
		     fl != NULL && fl->f_type == SWAPSPEC;
		     fl = fl->f_next)
		continue;
	}
	fprintf(f, "LOAD=");
	for (fl = conf_list; fl != 0; fl = fl->f_next)
		if (fl->f_type == SYSTEMSPEC)
			fprintf(f, " %s", fl->f_needs);
	fprintf(f,"\n");
	for (fl = conf_list; fl != 0; fl = fl->f_next)
		if (fl->f_type == SYSTEMSPEC)
			fprintf(f, "%s_IDIR=${LOAD_IDIR}\n", fl->f_needs);
}

do_swapspec(f, name, system)
	char *system;
	FILE *f;
	register char *name;
{

	char *gdir = eq(name, "generic")?"${MACHINE}/":"";

	fprintf(f, "%s.swap: swap%s.o\n", system, name);
	fprintf(f, "\t@${CP} swap%s.o %s.swap\n", name, system);
	fprintf(f, "swap%s.o: ${SWAPDEPS}\n", name);
        fprintf(f, "swap%s.o_SOURCE=%sswap%s.c\n", name, gdir, name);
	fprintf(f, "COBJS+=swap%s.o\n", name);
}

char *
raise(str)
	register char *str;
{
	register char *cp = str;

	while (*str) {
		if (islower(*str))
			*str = toupper(*str);
		str++;
	}
	return (cp);
}

FILE *
VPATHopen(file, mode)
register char *file, *mode;
{
	register char *nextpath,*nextchar,*fname,*lastchar;
	char fullname[BUFSIZ];
	FILE *fp;

	nextpath = ((*file == '/') ? "" : search_path);
	do {
		fname = fullname;
		nextchar = nextpath;
		while (*nextchar && (*nextchar != ':'))
			*fname++ = *nextchar++;
		if (nextchar != nextpath && *file) *fname++ = '/';
		lastchar = nextchar;
		nextpath = ((*nextchar) ? nextchar + 1 : nextchar);
		nextchar = file;	/* append file */
		while (*nextchar)  *fname++ = *nextchar++;
		*fname = '\0';
		fp = fopen (fullname, mode);
	} while (fp == NULL && (*lastchar));
	return (fp);
}
