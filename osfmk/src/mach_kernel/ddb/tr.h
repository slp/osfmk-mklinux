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
 *	File:		ddb/tr.h
 *	Author:		Alan Langerman, Jeffrey Heller
 *	Date:		1992
 *
 *	Internal trace routines.  Like old-style XPRs but
 *	less formatting.
 */

#include <mach_assert.h>
#include <mach_tr.h>

#include <kern/cpu_number.h>

/*
 *	Originally, we only wanted tracing when
 *	MACH_TR and MACH_ASSERT were turned on
 *	together.  Now, there's no reason why
 *	MACH_TR and MACH_ASSERT can't be completely
 *	orthogonal.
 */
#define	TRACE_BUFFER	(MACH_TR)

/*
 *	Log events in a circular trace buffer for future debugging.
 *	Events are unsigned integers.  Each event has a descriptive
 *	message.
 *
 *	TR_DECL must be used at the beginning of a routine using
 *	one of the tr calls.  The macro should be passed the name
 *	of the function surrounded by quotation marks, e.g.,
 *		TR_DECL("netipc_recv_intr");
 *	and should be terminated with a semi-colon.  The TR_DECL
 *	must be the *last* declaration in the variable declaration
 *	list, or syntax errors will be introduced when TRACE_BUFFER
 *	is turned off.
 */
#ifndef	_DDB_TR_H_
#define	_DDB_TR_H_

#if	TRACE_BUFFER

#include <machine/db_machdep.h>

#define	__ui__			(unsigned int)
#define	TR_INIT()		tr_init()
#define TR_SHOW(a,b,c)		show_tr((a),(b),(c))
#define	TR_DECL(funcname)	char	*__ntr_func_name__ = funcname
#define	tr1(msg)							\
	tr(__ntr_func_name__, __FILE__, __LINE__, (msg),		\
		0,0,0,0)
#define	tr2(msg,tag1)							\
	tr(__ntr_func_name__, __FILE__, __LINE__, (msg),		\
		__ui__(tag1),0,0,0)
#define	tr3(msg,tag1,tag2)						\
	tr(__ntr_func_name__, __FILE__, __LINE__, (msg),		\
		__ui__(tag1),__ui__(tag2),0,0)
#define	tr4(msg,tag1,tag2,tag3)						\
	tr(__ntr_func_name__, __FILE__, __LINE__, (msg),		\
		__ui__(tag1),__ui__(tag2),__ui__(tag3),0)
#define	tr5(msg,tag1,tag2,tag3,tag4)					\
	tr(__ntr_func_name__, __FILE__, __LINE__, (msg),		\
		__ui__(tag1),__ui__(tag2),__ui__(tag3),__ui__(tag4))

/*
 *	Adjust tr log indentation based on function
 *	call graph.
 */
#if	NCPUS == 1
extern int tr_indent;
#define	tr_start()	tr_indent++
#define tr_stop()	tr_indent--
#else	/* NCPUS == 1 */
extern int tr_indent[NCPUS];
#define	tr_start()	tr_indent[cpu_number()]++
#define tr_stop()	(--tr_indent[cpu_number()]<0?tr_indent[cpu_number()]=0:0);
#endif	/* NCPUS == 1 */

extern void	tr_init(void);
extern void	tr(
			char		*funcname,
			char		*file,
			unsigned int	lineno,
			char		*fmt,
			unsigned int	tag1,
		   	unsigned int	tag2,
			unsigned int	tag3,
			unsigned int	tag4);

extern void db_show_tr(
			db_expr_t	addr,
			boolean_t	have_addr,
			db_expr_t	count,
			char *		modif);

#else	/* TRACE_BUFFER */

#define	TR_INIT()
#define TR_SHOW(a,b,c)
#define	TR_DECL(funcname)
#define tr1(msg)
#define tr2(msg, tag1)
#define tr3(msg, tag1, tag2)
#define tr4(msg, tag1, tag2, tag3)
#define tr5(msg, tag1, tag2, tag3, tag4)
#define	tr_start()
#define tr_stop()

#endif	/* TRACE_BUFFER */

#endif	/* _DDB_TR_H_ */
