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

/*
 * AIL header file containing AIL flipc configuration macros.  User-visible
 * for FLIPC functions that are implemented as macros and for allowing
 * tests to conditionalize on the various values.  Macros are defined
 * as zero to disable features, and one to enable them.  
 */
/* Can we use the real time lock primitives?  */
/* Note that this macro is required to have the same name and usage
   conventions here as in mach_kernel/flipc/flipc.h, as both kernel
   and user include flipc_cb.h, which uses this macro.  */
#define REAL_TIME_PRIMITIVES 1

/* Should we use bus-locking primitives to get around hardware
   bugs?  None of our current platforms need it.  */
#define FLIPC_BUSLOCK 0

/*
 * Should we sanity check stuff we find in the comm buffer?
 * This is similar to the paranoid consistency checking the ME
 * does, but applications may be more willing to be trusting.
 * Note that this macro is currently a no-op, as there aren't
 * any such checks in the AIL.
 */
#define SANITY_CHECKING 0

/*
 * On the critical path, should we check what the user gives us
 * for errors?
 */
#define NO_ERROR_CHECKING 1

/* Is the message engine implemented as part of the kernel?  */
#define KERNEL_MESSAGE_ENGINE 0

/*
 * How many times to spin on a simple lock before yielding the CPU?
 * *This macro has a numeric value*.  On a system known not to be an MP,
 * this should be 1. 
 */
#define LOCK_SPIN_LIMIT 1

