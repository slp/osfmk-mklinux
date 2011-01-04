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

#ifndef	_MISC_PROTOS_H_
#define	_MISC_PROTOS_H_

#include <dipc.h>

#include <stdarg.h>
#include <string.h>
#include <machine/setjmp.h>
#include <mach/boolean.h>
#include <mach/message.h>
#include <mach/machine/vm_types.h>
#include <ipc/ipc_types.h>

/* Set a bit in a bit array */
extern void setbit(
	int		which,
	int		*bitmap);

/* Clear a bit in a bit array */
extern void clrbit(
	int		which,
	int		*bitmap);

/* Find the first set bit in a bit array */
extern int ffsbit(
	int		*bitmap);
extern int ffs(
	unsigned int	mask);

/* Move overlapping, arbitrarily aligned data from one array to another */
/* Not present on all ports */
extern void ovbcopy(
	const char	*from,
	char		*to,
	vm_size_t	nbytes);

/* Move arbitrarily-aligned data from one array to another */
extern void bcopy(
	const char	*from,
	char		*to,
	vm_size_t	nbytes);

extern int bcmp(
		const char *a,
		const char *b,
		vm_size_t len);

/* Zero an arbitrarily aligned array */
extern void bzero(
	char	*from,
	vm_size_t	nbytes);

/* Move arbitrarily-aligned data from a user space to kernel space */
extern boolean_t copyin(
	const char	*user_addr,
	char		*kernel_addr,
	vm_size_t	nbytes);

/* Move a NUL-terminated string from a user space to kernel space */
extern boolean_t copyinstr(
	const char	*user_addr,
	char		*kernel_addr,
	vm_size_t	max,
	vm_size_t	*actual);

/* Move arbitrarily-aligned data from a user space to kernel space */
extern boolean_t copyinmsg(
	const char	*user_addr,
	char		*kernel_addr,
	mach_msg_size_t nbytes);

/* Move arbitrarily-aligned data from a kernel space to user space */
extern boolean_t copyout(
	const char	*kernel_addr,
	char		*user_addr,
	vm_size_t	 nbytes);

/* Move arbitrarily-aligned data from a kernel space to user space */
extern boolean_t copyoutmsg(
	const char	*kernel_addr,
	char		*user_addr,
	mach_msg_size_t nbytes);

extern int sscanf(const char *input, const char *fmt, ...);

extern integer_t sprintf(char *buf, const char *fmt, ...);

extern void printf(const char *format, ...);

extern void printf_init(void);

extern void panic(const char *string, ...);

extern void panic_init(void);

extern void log(int level, char *fmt, ...);

void 
_doprnt(
	register const char	*fmt,
	va_list			*argp,
	void			(*putc)(char),
	int			radix);

extern void safe_gets(
	char	*str,
	int	maxlen);

extern void cnputc(char);

extern int cngetc(void);

extern int cnmaygetc(void);

extern int _setjmp(
	jmp_buf_t	*jmp_buf);

extern int _longjmp(
	jmp_buf_t	*jmp_buf,
	int		value);

extern void bootstrap_create(void);

extern void halt_cpu(void);

extern void halt_all_cpus(
		boolean_t	reboot);

extern void Debugger(
		const char	* message);

extern void delay(
		int		n);

extern char *machine_boot_info(
		char		*buf,
		vm_size_t	buf_len);

/*
 * Machine-dependent routine to fill in an array with up to callstack_max
 * levels of return pc information.
 */
extern void machine_callstack(
		natural_t	*buf,
		vm_size_t	callstack_max);

extern void consider_machine_collect(void);

extern void norma_bootstrap(void);

#if	DIPC
extern boolean_t	no_bootstrap_task(void);
extern ipc_port_t	get_root_master_device_port(void);
#endif	/* DIPC */

#endif	/* _MISC_PROTOS_H_ */
