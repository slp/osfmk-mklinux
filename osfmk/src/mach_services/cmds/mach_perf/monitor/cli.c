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
/*
 * OLD HISTORY
 * Revision 1.1.4.2  1995/11/03  17:09:24  bruel
 * 	Print usage on '?' argument.
 * 	Also prevents from redefining ?.
 * 	[95/10/26            bernadat]
 * 	[95/11/03            bruel]
 * 
 * Revision 1.1.4.1  1995/04/12  12:27:26  madhu
 * 	copyright marker not _FREE
 * 	safe_gets interface now has a prompt as first arg.
 * 	[1995/04/11  14:53:15  madhu]
 * 
 * Revision 1.1.2.1  1994/12/09  14:03:09  madhu
 * 	changes by Nick for synthetic tests.
 * 	[1994/12/09  13:46:50  madhu]
 * 
 * 	added simple cli interface for procs used for synthetic benchmarking, and a configuration file allowing defaults to be inbuild
 * 
 */

#include <mach_perf.h>

/* These data structures are from the machine-generated file cli_init.c */

extern char *cli_init[];
extern int cli_init_count;

/*
 * Contrary to all the rest of the code, we wish to use the true malloc()
 * and free() routines, since the allocations do not need to be
 * accountable
 */

extern struct test_dir *test_dirs[];

static cmd_vec 	command_line;
static proc_vec *prochead = NULL; /* a linked list of procs */
/*
 * A few routines to deal with memory allocation because we need to use
 * static memory and not malloc(). Everything is still coded to use
 * pointers, so this is simple to interchange.
 */
static int proc_count = 0; /* number of procs allocated */
static proc_vec procs[MAX_PROCS];
static cmd_vec 	cmds[MAX_CMDS];

proc_vec*  proc_alloc(void)
{
    if (proc_count < MAX_PROCS) {
	return &procs[proc_count++];
    }
    printf("proc : no more memory to allocate procs\n");
    leave(1);
}

cmd_vec* cmd_alloc(void)
{
    int i;
    for (i=0; i < MAX_CMDS; i++) {
	if (cmds[i].argc == 0) {
	    return &cmds[i];
	}
    }
    printf("proc : no more memory to allocate proc bodies\n");
    leave(1);
}

void cmd_free(cmd_vec* cmdp)
{
    cmdp->argc = 0;
}

/*
 * Read a simple command line from console and build argc/argv
 * this function skips over any lines that start with a '#' (comments)
 */

int read_cmd(char **argv[], char *prompt)
{
    char 	*arg;
    int 	no_arg;
    int     argc;
    int 	len;
    char    dispstr[2] ; /* needed for safe_gets */

    *dispstr=0;
    /* the first commands come directly from the initialisation file */
    if (cli_init[cli_init_count] != ((char*)0)) {
	if (debug)
	    printf("line from init file : %s\n",cli_init[cli_init_count]);
	len = strlen(cli_init[cli_init_count]);
	if (len>=LINE_LENGTH-1) {
	    printf("read_cmd : line too long in init file, file aborted\n");
	    /* skip over the rest of the initialisation data */
	    while (cli_init[cli_init_count] != ((char*)0))
		cli_init_count++;
	    printf("%s", prompt);
	    if(standalone) 
	        safe_gets(dispstr, command_line.data, LINE_LENGTH);
	    else {
		int readlen;
                readlen = read(0, command_line.data, LINE_LENGTH) ;
                *(command_line.data+readlen-1) = 0 ;
	    }
	} else {
	    bcopy(cli_init[cli_init_count],
		  &(command_line.data[0]),
		  len+1); /* don't forget the terminator */
	    cli_init_count++;
	}
    } else {
	printf("%s", prompt);
	if(standalone) 
	    safe_gets(dispstr, command_line.data, LINE_LENGTH);
	else {
    	    int readlen;
            readlen = read(0, command_line.data, LINE_LENGTH) ;
            *(command_line.data+readlen-1) = 0 ;
        }
    }
    
    for (arg = &(command_line.data[0]), no_arg = 1, command_line.argc = 0;
	 *arg && command_line.argc < MAX_ARGS;
	 arg++) {
	if (*arg == ' ' || *arg == '\t') {
	    if (!no_arg) {
		*arg = 0;
		no_arg = 1;
	    }
	} else {
	    if (*arg == '#')
		break;		/* # starts comment */
	    if (no_arg) {
		command_line.argv[command_line.argc++] = arg;
		no_arg = 0;
	    }
	}
    }
    *argv = command_line.argv;
    return(command_line.argc);
}

/*
 * find the definition of the proc given it's name, or return NULL
 */

proc_vec *find_proc(char *name)
{
    proc_vec *procp;
    for (procp = prochead; procp != NULL; procp=procp->next) {
	if (!strcmp(procp->name,name))
	    return procp;
    }
    return NULL;
}

/*
 * proc command. Used for defining a synthetic benchmark
 */

int proc (int argc, char *argv[])
{
    struct test_dir *td, **tds;
    int 	i;
    int		internal_argc;
    char 	**internal_argv;
    proc_vec	*procp;
    cmd_vec	*cmdp;
    boolean_t 	found;
    char	*charp;

    if (argc != 2 || *argv[1] == '?') {
	printf("Usage : proc <proc_name>\n");
	printf("          test <args>\n");
	printf("          test <args>\n");
	printf("          ...\n");
	printf("        endproc\n");
	return 0;
    }

    /* a proc name must not be the same as that of a predefined test */

    for (tds = test_dirs; *tds; tds++) {
	for (td = *tds; td->name; td++)
	    if (!strcmp(argv[1],(char*)td->name)) {
		printf("syntax : proc name cannot be that of a test\n");
		return 0;
	}
    }

    /*
     * Create the new entry in the list of procedures,
     * making sure first that such an entry doesn't already exist
     */
    found = FALSE;

    if ((procp = find_proc(argv[1])) != NULL) {
	/* if the proc already existed, empty it. */
	while ((cmdp = procp->cmdhead) != NULL) {
	    procp->cmdhead = cmdp->next;
	    cmd_free(cmdp);
	}
	found = TRUE;
    }

    if (!found) {
	procp = proc_alloc();
	strcpy(procp->name,argv[1]);
	procp->cmdhead = NULL;
	procp->next = prochead;
	prochead = procp;
    }

    /* read command lines up until an `endproc' */

    cmdp = NULL; 
    do {
	do {
	    internal_argc = read_cmd(&internal_argv,"proc:   ");
	} while (argc == 0);

	/* 'endproc' defines the end of a proc definition */
	if (!strcmp(internal_argv[0],"endproc"))
	    break;

	/*
	 * otherwise make a new slot for this command at the end of the
	 * current proc's command list
         */

	if (cmdp == NULL) {
	    procp->cmdhead = cmd_alloc();
	    cmdp = procp->cmdhead;
	} else {
	    cmdp->next = cmd_alloc();
	    cmdp = cmdp->next;
	}
	cmdp->next = NULL;

	/*
         * and copy the command line into the proc's definiton
         */

	charp = &(cmdp->data[0]);

	for (i = 0; i < internal_argc; i++) {
	    bcopy(argv[i],charp,strlen(argv[i])+1);
	    cmdp->argv[i] = charp;
	    charp += strlen(charp)+1;
	}
	cmdp->argc = internal_argc;

    } while (TRUE);  /* `break' caused by `endproc' string */

    return 0;
}

void show_usage(void)
{
    enable_more();
    printf("Usage: show"
"\t             show a list of the defined procedures\n"
"\t\t<proc_names> show the definition of the given procedures\n"
"\t\t*            show the definition of all defined procedures\n");
    leave(1);
}

/*
 * command : show [procname]...
 * this command displays the definition of one or more procs
 * with the argument '*', it shows all proc definitions,
 * and with no arguments, it shows the list of all defined procs.
 */

show(int argc, char *argv[])
{
    boolean_t 	all,names_only,found;

    proc_vec	*procp;
    cmd_vec	*cmdp;

    int pos = 0;
    int i,j;

    all = names_only = FALSE;

    if (argc <= 1) {

	/*
	 * for the case of no arguments, we set names_only.
	 * this causes a single time arond the outer loop below
         * listing all the names but not the procedure contents
	 */
	   
	names_only = TRUE;
	printf("Proc macros for synthetic benchmarking\n");

    } else if ((argc == 2) && (!strcmp(argv[1],"*")))
	all = TRUE;

    for (i = 1; names_only || (i < argc); i++) {
	found = FALSE;
	for (procp = prochead; procp != NULL; procp = procp->next) {
	    if (names_only) {
		/* display a list of the defined procs, four per line */
		if (!(pos % 4)) {
		    if (pos)
			printf("\n");
		    printf("\t");
		}

		printf("%s",procp->name);

		spaces(16-strlen(procp->name));
		pos++;

		/*
		 * go immediately around the loop without referencing
		 * argv[1] which does not contain a known value (fudge)
		 */
		found = TRUE;
		continue;
	    }

	    /* display the proc definition if we've got a match */

	    if (all || (!strcmp(argv[i],procp->name))) {
		printf("proc %s\n",procp->name);
		for (cmdp = procp->cmdhead; cmdp != NULL; cmdp=cmdp->next) {
		    printf(" ");
		    for (j=0;j<cmdp->argc;j++)
			printf(" %s",cmdp->argv[j]);
		    printf("\n");
		}
		printf("endproc\n\n");
		found = TRUE;
	    }
	}
	if (names_only)
	    break;
	if (!found) {
	    printf("\nUndefined proc name %s\n",argv[i]);
	    show_usage();
	}
    }
    /* if only printing names we may have finished half way along a line */
    if (names_only) {
	if (pos == 0)
	    printf("\tNone\n\n");
	if ((pos % 4))
	    printf("\n\n");
    }
}
