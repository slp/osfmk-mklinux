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

#ifndef _STDARG_H
#define _STDARG_H

#if 0
#include <standards.h>
#else
#ifndef	_ANSI_C_SOURCE
#define _ANSI_C_SOURCE
#endif
#endif

#ifdef _ANSI_C_SOURCE

#include <va_list.h>		/* defines va_list */

/* Register save area located below the frame pointer */
typedef struct {
  long   __gp_save[8];		/* save area for GP registers */
  double __fp_save[8];		/* save area for FP registers */
} __va_regsave_t;

/* Macros to access the register save area */
/* We cast to void * and then to TYPE * because this avoids
   a warning about increasing the alignment requirement.  */
#define __VA_FP_REGSAVE(AP,TYPE)					\
  ((TYPE *) (void *) (&(((__va_regsave_t *)				\
			 (AP)->reg_save_area)->__fp_save[(int)(AP)->fpr])))

#define __VA_GP_REGSAVE(AP,TYPE)					\
  ((TYPE *) (void *) (&(((__va_regsave_t *)				\
			 (AP)->reg_save_area)->__gp_save[(int)(AP)->gpr])))

/* Common code for va_start for both varargs and stdarg.  This depends
   on the format of rs6000_args in rs6000.h.  The fields used are:

   #0	WORDS			# words used for GP regs/stack values
   #1	FREGNO			next available FP register
   #2	NARGS_PROTOTYPE		# args left in the current prototype
   #3	ORIG_NARGS		original value of NARGS_PROTOTYPE
   #4	VARARGS_OFFSET		offset from frame pointer of varargs area */

#define __va_words		__builtin_args_info (0)
#define __va_fregno		__builtin_args_info (1)
#define	__va_nargs		__builtin_args_info (2)
#define __va_orig_nargs		__builtin_args_info (3)
#define __va_varargs_offset	__builtin_args_info (4)

#define __va_start_common(AP, FAKE)					\
__extension__ ({							\
   register int __words = __va_words - FAKE;				\
									\
   (AP)->gpr = (__words < 8) ? __words : 8;				\
   (AP)->fpr = __va_fregno - 33;					\
   (AP)->reg_save_area = (((char *) __builtin_frame_address (0))	\
			  + __va_varargs_offset);			\
   (AP)->overflow_arg_area = ((char *)__builtin_saveregs ()		\
			      + (((__words >= 8) ? __words - 8 : 0)	\
				 * sizeof (long)));			\
   (void)0;								\
})

#define va_start(AP,LASTARG) __va_start_common (AP, 0)

#ifdef _SOFT_FLOAT
#define __va_float_p(TYPE)	0
#else
#define __va_float_p(TYPE)	(__builtin_classify_type(*(TYPE *)0) == 8)
#endif

#define __va_aggregate_p(TYPE)	(__builtin_classify_type(*(TYPE *)0) >= 12)
#define __va_size(TYPE)		((sizeof(TYPE) + sizeof (long) - 1) / sizeof (long))

#define va_arg(AP,TYPE)							\
__extension__ ({							\
  register TYPE *__ptr;							\
									\
  if (__va_float_p (TYPE) && (AP)->fpr < 8)				\
    {									\
      __ptr = __VA_FP_REGSAVE (AP, TYPE);				\
      (AP)->fpr++;							\
    }									\
									\
  else if (__va_aggregate_p (TYPE) && (AP)->gpr < 8)			\
    {									\
      __ptr = * __VA_GP_REGSAVE (AP, TYPE *);				\
      (AP)->gpr++;							\
    }									\
									\
  else if (!__va_float_p (TYPE) && !__va_aggregate_p (TYPE)		\
	   && (AP)->gpr + __va_size(TYPE) <= 8)				\
    {									\
      __ptr = __VA_GP_REGSAVE (AP, TYPE);				\
      (AP)->gpr += __va_size (TYPE);					\
    }									\
									\
  else if (__va_aggregate_p (TYPE))					\
    {									\
      __ptr = * (TYPE **) (void *) ((AP)->overflow_arg_area);		\
      (AP)->overflow_arg_area += sizeof (TYPE *);			\
    }									\
  else									\
    {									\
      __ptr = (TYPE *) (void *) ((AP)->overflow_arg_area);		\
      (AP)->overflow_arg_area += __va_size (TYPE) * sizeof (long);	\
    }									\
									\
  *__ptr;								\
})

#define va_end(AP)	((void)0)

#endif /* _ANSI_C_SOURCE */
#endif /* _STDARG_H */


