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

#ifndef _MACHINE_RPC_H_
#define _MACHINE_RPC_H_

#if     ETAP_EVENT_MONITOR
#define ETAP_EXCEPTION_PROBE(_f, _th, _ex, _sysnum)             \
        if (_ex == EXC_SYSCALL) {                               \
                ETAP_PROBE_DATA(ETAP_P_SYSCALL_UNIX,            \
                                _f,                             \
                                _th,                            \
                                _sysnum,                        \
                                sizeof(int));                   \
        }
#else   /* ETAP_EVENT_MONITOR */
#define ETAP_EXCEPTION_PROBE(_f, _th, _ex, _sysnum)
#endif  /* ETAP_EVENT_MONITOR */

#ifdef MACHINE_FAST_EXCEPTION

extern void exception_return_wrapper(kern_return_t);

extern kern_return_t call_exc_serv(mach_port_t,
				   exception_type_t,
				   exception_data_t,
				   mach_msg_type_number_t,
				   int *,
				   mig_impl_routine_t,
				   int *,
				   thread_state_t,
				   mach_msg_type_number_t *);


#endif /* MACHINE_FAST_EXCEPTION */
#endif /* _MACHINE_RPC_H_ */


