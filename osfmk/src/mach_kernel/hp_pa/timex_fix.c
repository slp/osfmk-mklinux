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
 *  (c) Copyright 1990 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */

#ifdef TIMEX_BUG
/*
 * timex_fix is a function to be called by the UNIX exception trap 
 * handler as a temporary workaround to TIMEX defect ?????.  It works  
 * by shuffling the contents of exception registers 2-5 in such a way
 * as to undo the effects of the bug.
 * 
 * The 4 arguments are pointers to unsigned ints which contain the
 * contents of exception registers 2, 3, 4 and 5.  (Registers 1, 6 and
 * 7 are not affected by the bug.)  
 * 
 * We recommend that this function be called immediately after the 
 * exception registers are unloaded into memory.  It should be called
 * even if there is a user-supplied signal handler.
 *
 * After the function has determined the correct exception register
 * ordering, it places the new values back into the locations pointed
 * to by the arguments.
 *
 * The return value of this function is 1 if it succeeds.  Some 
 * flop sequences cannot be fixed up, and when this is detected, 0
 * is returned instead.  We believe that unfixable sequences will be
 * exceedingly rare (never emitted by existing compilers).  It would
 * be ideal if UNIX would terminate a process with a diagnostic if 
 * this ever occurs, but this is not worth spending time on unless 
 * it's really easy.
 *
 * Written by Steve Mangelsdorf   6/29/90
 *   9/10:  tests version number (returns without doing anything if
 *          not version 1.0
 */

typedef int register_number_type;
typedef int precision_type;
typedef unsigned int instruction_type;
typedef enum { false, true } boolean;
typedef int exception_register_number_type;

struct register_type {
  instruction_type inst;
  register_number_type ra, rb, rr, ara, arb;
  precision_type src_prec, dst_prec;
}; 

boolean hit( a, ap, b, bp )

register_number_type a, b;
precision_type ap, bp;

{
#define max(x,y) ((x > y) ? x : y)
  precision_type p; 
  if (a == -1 || b == -1)
    return( 0 );
  else {
    p = max( ap, bp ) - 1; 
    return( (a >> p) == (b >> p) ); 
  }
}

boolean wwdep( x1, x2 )

struct register_type x1, x2;

{
  return(
    hit( x1.rr, x1.dst_prec, x2.rr, x2.dst_prec ) ||
    hit( x1.rr, x1.dst_prec, x2.ara, x2.dst_prec ) ||
    hit( x1.ara, x1.dst_prec, x2.rr, x2.dst_prec ) ||
    hit( x1.ara, x1.dst_prec, x2.ara, x2.dst_prec )
  );
}

boolean rwdep( x1, x2 )

struct register_type x1, x2;

{
  return(
    hit( x1.ra, x1.src_prec, x2.rr, x2.dst_prec ) ||
    hit( x1.rb, x1.src_prec, x2.rr, x2.dst_prec ) ||
    hit( x1.ara, x1.src_prec, x2.rr, x2.dst_prec ) ||
    hit( x1.arb, x1.src_prec, x2.rr, x2.dst_prec ) ||
    hit( x1.ra, x1.src_prec, x2.ara, x2.dst_prec ) ||
    hit( x1.rb, x1.src_prec, x2.ara, x2.dst_prec ) ||
    hit( x1.ara, x1.src_prec, x2.ara, x2.dst_prec ) ||
    hit( x1.arb, x1.src_prec, x2.ara, x2.dst_prec )
  );
}

void decode( ip )

struct register_type *ip;

{
#define val(x) ( x < 8 ? -1 : x )
#define bits(n,i,j) ((n >> (31-j)) & ~(0xFFFFFFFF << (j-i+1)))

  ip->ra = -1;
  ip->rb = -1;
  ip->rr = -1;
  ip->ara = -1;
  ip->arb = -1;
  ip->src_prec = 0;
  ip->dst_prec = 0;
  switch (bits( ip->inst, 0, 5 )) {
    case 0x03 : case 0x23 :
      if (!bits( ip->inst, 26, 26 )) {
        ip->ra  = val( 2 * bits( ip->inst,  6, 10 ) ); 
        ip->rb  = val( 2 * bits( ip->inst, 11, 15 ) );
        ip->rr  = val( 2 * bits( ip->inst, 27, 31 ) ); 
        ip->ara = val( 2 * bits( ip->inst, 16, 20 ) ); 
        ip->arb = val( 2 * bits( ip->inst, 21, 25 ) ); 
        ip->src_prec = 2;
        ip->dst_prec = 2;
      } 
      else {
        ip->ra  = 32 + 2 * bits( ip->inst, 7,  10 ) + bits( ip->inst, 6,  6 ); 
        ip->rb  = 32 + 2 * bits( ip->inst, 12, 15 ) + bits( ip->inst, 11, 11 ); 
        ip->rr  = 32 + 2 * bits( ip->inst, 28, 31 ) + bits( ip->inst, 27, 27 ); 
        ip->ara = 32 + 2 * bits( ip->inst, 17, 20 ) + bits( ip->inst, 16, 16 ); 
        ip->arb = 32 + 2 * bits( ip->inst, 22, 25 ) + bits( ip->inst, 21, 21 ); 
        if (ip->arb == 32) 
          ip->arb = -1;
        ip->src_prec = 1;
        ip->dst_prec = 1;
      }
      break;
    case 0x09 : case 0x0B :
      switch (bits( ip->inst, 21, 22 )) {
        case 0 :
          ip->ra = val( 2 * bits( ip->inst, 6, 10 ) + 
                     bits( ip->inst, 24, 24 ) );
          ip->rr = val( 2 * bits( ip->inst, 27, 31 ) + 
                     bits( ip->inst, 25, 25 ) );
          if (bits( ip->inst, 20, 20 )) {
            ip->src_prec = 2;
            ip->dst_prec = 2;
          }
          else {
            ip->src_prec = 1;
            ip->dst_prec = 1;
          }
          break;
        case 1 :
          ip->ra = val( 2 * bits( ip->inst, 6, 10 ) + 
                     bits( ip->inst, 24, 24 ) );
          ip->rr = val( 2 * bits( ip->inst, 27, 31 ) + 
                     bits( ip->inst, 25, 25 ) );
          if (bits( ip->inst, 20, 20 )) 
            ip->src_prec = 2;
          else
            ip->src_prec = 1;
          if (bits( ip->inst, 18, 18 )) 
            ip->dst_prec = 2;
          else
            ip->dst_prec = 1;
          break;
        case 2 :
          ip->ra = val( 2 * bits( ip->inst, 6,  10 ) + 
                     bits( ip->inst, 24, 24 ) );
          ip->rb = val( 2 * bits( ip->inst, 11, 15 ) + 
                     bits( ip->inst, 19, 19 ) );
          if (bits( ip->inst, 20, 20 )) 
            ip->src_prec = 2;
          else
            ip->src_prec = 1;
          break;
        case 3 :
          ip->ra = val( 2 * bits( ip->inst, 6,  10 ) + 
                     bits( ip->inst, 24, 24 ) );
          ip->rb = val( 2 * bits( ip->inst, 11, 15 ) + 
                     bits( ip->inst, 19, 19 ) );
          ip->rr = val( 2 * bits( ip->inst, 27, 31 ) + 
                     bits( ip->inst, 25, 25 ) ); 
          if (bits( ip->inst, 20, 20 )) 
            ip->src_prec = 2;
          else
            ip->src_prec = 1;
          if (bits( ip->inst, 20, 20 ) || bits( ip->inst, 23, 23 )) 
            ip->dst_prec = 2;
          else
            ip->dst_prec = 1;
          break;
      }
      break;
    default :
      break;
  }
}

boolean timex_fix( xr2_p, xr3_p, xr4_p, xr5_p )

instruction_type *xr2_p, *xr3_p, *xr4_p, *xr5_p;

{
  struct register_type xr[4];
  extern int getfpversion();

  if (getfpversion() != 0x041)
    return( 1 );

  xr[2].inst = *xr2_p;
  xr[3].inst = *xr3_p;

  decode( &xr[2] );
  decode( &xr[3] );

  if (rwdep( xr[3], xr[2] )) {
    xr[0] = xr[2];
    xr[2] = xr[3];
    xr[3] = xr[0];
  }

  if (wwdep( xr[2], xr[3] ) && !rwdep( xr[2], xr[3] ))
    return( 0 );
  else {
    *xr2_p = xr[2].inst;
    *xr3_p = xr[3].inst;
    return( 1 );
  }
}
#endif /* TIMEX_BUG */
