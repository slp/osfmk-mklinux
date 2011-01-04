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

#include <i386/thread.h>

extern void		get_root_device(void);
extern void		picinit(void);
extern void		slave_clock(void);
extern void		interrupt_processor(
				int		cpu);
extern void		mp_probe_cpus(void);
extern void		remote_kdb(void);
extern void		clear_kdb_intr(void);
extern void		set_cpu_model(void);
extern void		cpu_shutdown(void);
extern void		fix_desc(
				void		* desc,
				int		num_desc);
extern void		cnpollc(
				boolean_t	on);
extern void		form_pic_mask(void);
extern void		intnull(
				int		unit);
extern char *		i386_boot_info(
				char		*buf,
				vm_size_t	buf_len);
extern void 		hardclock(
				int 	vect,	
			        int 	ipl,
			        char 	*ret_addr,
			        struct 	i386_interrupt_state *regs);

extern void		blkclr(
			       const char	*from,
			       int		nbytes);

extern void		kdb_kintr(void);
extern void		kdb_console(void);

extern unsigned	long  	ntohl(unsigned long);
extern char *		machine_boot_info(
				char		*buf,
				vm_size_t	buf_len);

extern unsigned int	div_scale(
				unsigned int	dividend,
				unsigned int	divisor,
				unsigned int	*scale);

extern unsigned int	mul_scale(
				unsigned int	multiplicand,
				unsigned int	multiplier,
				unsigned int	*scale);
