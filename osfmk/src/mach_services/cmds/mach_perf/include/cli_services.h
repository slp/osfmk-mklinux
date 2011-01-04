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
 * The cli is responsible for reading in command lines, and dealing with
 * proc definitions and functions related to dealing with procs
 */

/*
 * MkLinux
 */

/* the storage structure for a command line */

typedef struct cmd_vec {
    struct cmd_vec *next;
    int argc;
    char* argv[MAX_ARGS];
    char data[LINE_LENGTH];
} cmd_vec;

/*
 * the data storage of procs is done in a simple list. Commands
 * within each proc are held in sublists.
 */

typedef struct proc_vec {
    struct proc_vec 	*next;
    char 		name[LINE_LENGTH];
    cmd_vec		*cmdhead;
} proc_vec;

extern int read_cmd(char **argv[], char *prompt);
extern proc_vec* find_proc(char *name);
extern int show(int argc, char *argv[]);
extern int proc(int argc, char *argv[]);
