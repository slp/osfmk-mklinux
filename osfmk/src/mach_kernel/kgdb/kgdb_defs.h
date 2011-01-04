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

#ifndef	_KGDB_DEFS_H_
#define	_KGDB_DEFS_H_

#include <kgdb.h>
#include <mach_kgdb.h>

#include <stdarg.h>
#include <mach/boolean.h>

#if	MACH_KGDB

#include <kgdb/gdb_defs.h>
#include <machine/kgdb_defs.h>
#include <machine/kgdb_setjmp.h>

extern boolean_t kgdb_debug;
extern boolean_t kgdb_initialized;

#define KGDB_DEBUG(args) if (kgdb_debug) kgdb_printf args

extern kgdb_jmp_buf_t *kgdb_recover;

extern const char *kgdb_panic_msg;

extern char kgdb_data[];		/* In/Out data area */

/*
 * request/response format
 */
typedef struct kgdb_cmd_pkt {
	char		k_cmd;
	vm_offset_t	k_addr;
	int		k_size;
	vm_offset_t	k_data;
} kgdb_cmd_pkt_t;

#define KGDB_CMD_NO_ADDR 0xffffffff

/*
 * Kernel kgdb state 
 */
extern int kgdb_state;

#define	STEP_NONE	0
#define	STEP_ONCE	1

/*
 * Exported interfaces
 */
extern void kgdb_printf( const char *format, ...);

extern void kgdb_panic(
	const char 		*message);

extern void kgdb_enter(
	int		generic_type,
 	int		type,
	kgdb_regs_t	*regs);

extern void kgdb_init(void);

extern void kgdb_machine_init(void);

extern int kgdb_fromhex(
	int		val);

extern int kgdb_tohex(
	int		val);

extern void kgdb_putpkt(
	kgdb_cmd_pkt_t	*snd);

extern void kgdb_getpkt(
	kgdb_cmd_pkt_t	*rcv);

extern void kgdb_set_single_step(
	kgdb_regs_t	*regs);

extern void kgdb_clear_single_step(
	kgdb_regs_t	*regs);

boolean_t kgdb_in_single_step(void);

#else	/* MACH_KGDB */

#include <mach_kdb.h>

#if	MACH_KDB
#include <ddb/db_output.h>

#define kgdb_printf	db_printf
#else	/* MACH_KDB */
#define kgdb_printf	printf
#endif	/* MACH_KDB */

#include <machine/kgdb_defs.h>

#endif	/* MACH_KGDB */

#endif	/* _KGDB_DEFS_H_ */
