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

#ifndef _HP_PA_KKT_H_
#define _HP_PA_KKT_H_

#include <norma_scsi.h>
#include <dipc_xkern.h>

#if     NORMA_SCSI
#define	splkkt	splbio
#elif   DIPC_XKERN
/*
 * With DIPC/KKT/x-kernel, the whole protocol stack 
 * is executed in thread context. Thus, splkkt results 
 * in a no-op.
 */
#include <cpus.h>
#if     NCPUS > 1

#if     defined(__GNUC__)
#include <kern/spl.h>
#include <hp_pa/cpu_number.h>

#warning This code has been compiled but not tested

extern int curr_ipl[];
extern spl_t __inline__ splkkt(void);

extern spl_t __inline__ splkkt(void) {
	spl_t   ss;
	disable_preemption();
	ss = (spl_t)curr_ipl[cpu_number()];
	enable_preemption();
	return  ss;
}
#else   /* __GNUC__ */
#error  Not implemented
#endif  /* __GNUC__ */

#else   /* NCPUS == 1 */

extern spl_t get_spl(void);
#define	splkkt()	get_spl()

#endif  /* NCPUS == 1 */
#endif  /* DIPC_XKERN */

typedef struct transport {
	struct request_block 	*next;
#if	NORMA_SCSI
	void *kkte;
#else	/* NORMA_SCSI */
        vm_offset_t save_address;
        vm_size_t save_size;
        boolean_t save_sg;
#endif	/* NORMA_SCSI */
} transport_t;

#endif  /* _HP_PA_KKT_H_ */
