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
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)main.c	5.20 (Berkeley) 7/31/92";
#endif /* not lint */

#include <stdio.h>
#include <ctype.h>
#include "gram.h"
#include "config.h"

char *search_path;

/*
 * Config builds a set of files for building a UNIX
 * system given a description of the desired system.
 */
main(argc, argv)
	int argc;
	char **argv;
{
	char *dependfiletmp;
	char *dependfilename;

	object_directory = "..";
	config_directory = (char *) 0;
	search_path = (char *) 0;
	DEV_MASK = 0x7;
	DEV_SHIFT = 3;
	while ((argc > 1) && (argv[1][0] == '-')) {
		char		*c;

		argv++; argc--;
		c = &argv[0][1];
		if (*c == 'I') {
			int i;

			c++;
			if (search_path == 0) {
				i = strlen(c) + 1;
				search_path = malloc(i);
				strcpy(search_path, c);
			} else {
				i = strlen(c) + strlen(search_path) + 2;
				search_path = realloc(search_path, i);
				strcat(search_path, ":");
				strcat(search_path, c);
			}
			continue;
		}
		if (strcmp(c, "MD") == 0) {
			makedepends = 1;
			continue;
		}
		for (; *c; c++) {
			switch (*c) {
				case 'o':
					object_directory = argv[1];
					goto check_arg;

				case 'c':
					config_directory = argv[1];

				 check_arg:
				 	if (argv[1] == (char *) 0)
						goto usage_error;
					argv++; argc--;
					break;

				case 'p':
					fprintf(stderr,
			"config -p obsolete, make controls profiling\n");
					break;
				default:
					goto usage_error;
			}
		}
	}
	if (config_directory == (char *) 0)
		config_directory = "conf";
	if (search_path == (char *) 0)
		search_path = ".";
	if (argc != 2) {
		usage_error: ;
		fprintf(stderr,
"usage: config [-Iinc-dir ...] [-c config-dir] [-o object-dir] sysname\n");
		exit(1);
	}
	PREFIX = argv[1];
	if (makedepends) {
		dependfiletmp = (char *)malloc(strlen(PREFIX)+3);
		dependfilename = (char *)malloc(strlen(PREFIX)+3);
		sprintf(dependfiletmp, "%s.X", PREFIX);
		sprintf(dependfilename, "%s.d", PREFIX);
		(void)unlink(dependfiletmp);
		dependfile = fopen(dependfiletmp, "a");
		if (dependfile == (FILE *)NULL) {
			perror(dependfiletmp);
			exit(1);
		}
		fprintf(dependfile,"%s:", path("Makefile"));
	}
	if (include_file(argv[1])) {
		perror(argv[1]);
		exit(2);
	}
	dtab = NULL;
	confp = &conf_list;
	opt = 0;
	if (yyparse())
		exit(3);
	if (machinename == NULL) {
		fprintf(stderr, "config: \"machine\" must be specified\n");
		exit(1);
	}
	machinedep();
	makefile();			/* build Makefile */
	headers();			/* make a lot of .h files */
	swapconf();			/* swap config files */
	if (makedepends) {
		fprintf(dependfile, "\n");
		fclose(dependfile);
		rename(dependfiletmp, dependfilename);
	}
	exit(global_status);
}

/*
 * get_word
 *	returns EOF on end of file
 *	NULL on end of line
 *	pointer to the word otherwise
 */
char *
get_word(fp)
	register FILE *fp;
{
	static char line[256];
	register int ch;
	register char *cp;

	while ((ch = getc(fp)) != EOF)
		if (ch != ' ' && ch != '\t')
			break;
	if (ch == EOF)
		return ((char *)EOF);
	if (ch == '\n')
		return (NULL);
	cp = line;
	*cp++ = ch;
	switch (ch) {
	case '|':
	case '(': case ')':
	case '>': case '<':
		*cp = 0;
		return (line);
		/* NOTREACHED */
	default:
		break;
	}
	while ((ch = getc(fp)) != EOF) {
		if (isspace(ch))
			break;	/* whitespace ends word */
		switch (ch) {
			/* certain special characters end word */
		case '|':
		case '(': case ')':
		case '>': case '<':
			/* stop without taking char */
			break;
		default:
			*cp++ = ch;
			continue;
		}
		break;
	}
	*cp = 0;
	if (ch == EOF)
		return ((char *)EOF);
	(void) ungetc(ch, fp);
	return (line);
}

/*
 * get_quoted_word
 *	like get_word but will accept something in double or single quotes
 *	(to allow embedded spaces).
 */
char *
get_quoted_word(fp)
	register FILE *fp;
{
	static char line[256];
	register int ch;
	register char *cp;

	while ((ch = getc(fp)) != EOF)
		if (ch != ' ' && ch != '\t')
			break;
	if (ch == EOF)
		return ((char *)EOF);
	if (ch == '\n')
		return (NULL);
	cp = line;
	if (ch == '"' || ch == '\'') {
		register int quote = ch;

		while ((ch = getc(fp)) != EOF) {
			if (ch == quote)
				break;
			if (ch == '\n') {
				*cp = 0;
				printf("config: missing quote reading `%s'\n",
					line);
				exit(2);
			}
			*cp++ = ch;
		}
	} else {
		*cp++ = ch;
		switch (ch) {
		case '|':
		case '(': case ')':
		case '>': case '<':
			*cp = 0;
			return (line);
			/* NOTREACHED */
		default:
			break;
		}
		while ((ch = getc(fp)) != EOF) {
			if (isspace(ch))
				break;	/* whitespace ends word */
			switch (ch) {
				/* certain special characters end word */
			case '|':
			case '(': case ')':
			case '>': case '<':
				/* stop without taking char */
				break;
			default:
				*cp++ = ch;
				continue;
			}
			break;
		}
		if (ch != EOF)
			(void) ungetc(ch, fp);
	}
	*cp = 0;
	if (ch == EOF)
		return ((char *)EOF);
	return (line);
}

/*
 * get_rest
 *	returns EOF on end of file
 *	NULL on end of line
 *	pointer to the word otherwise
 */
char *
get_rest(fp)
	register FILE *fp;
{
	static char line[80];
	register int ch;
	register char *cp;

	cp = line;
	while ((ch = getc(fp)) != EOF) {
		if (ch == '\n')
			break;
		*cp++ = ch;
	}
	*cp = 0;
	if (ch == EOF)
		return ((char *)EOF);
	return (line);
}

/*
 * prepend the path to a filename
 */
char *
path(file)
	char *file;
{
	register char *cp;

	cp = malloc((unsigned)(strlen(object_directory)+1+
			       strlen(PREFIX)+1+
			       strlen(file)+1));
	(void) sprintf(cp, "%s/%s/%s", object_directory, PREFIX, file);
	return (cp);
}
