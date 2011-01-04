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

#define VERBOSE 0

#include <ppc/setjmp.h>

int recursed(jmp_buf_t *bufp, int retval, int depth)
{
  int mumbojumbo[16];
  int i;
 
#if VERBOSE
  for (i=0;i<depth;i++)
    printf(" ");
  printf("in recursed(0x%x,%d,%d)\n",bufp,retval,depth);
#endif
  if (depth == 0) {
#if VERBOSE
    printf("LONGJUMPING from depth %d to buffer at 0x%x!\n",retval, bufp);
#endif
    _longjmp(bufp, retval);
    printf("SHOULDN'T GET HERE\n");
  } else {
    recursed(bufp,retval,depth-1);
  }
  return mumbojumbo[15]=-1; /* make sure we generate our own frame */
}

int testjump()
{
  jmp_buf_t  buf;
  int val;
  int i;

  printf("calling setjmp\n");

  val = _setjmp(&buf);
#if VERBOSE
  for (i=0; i<64; i++) {
    if ((i % 8) == 0) printf("\n%2d :",i);
    printf(" %8x",buf.jmp_buf[i]);
  }
#endif
  printf("\nsetjmp returned %d, structure at 0x%x\n",val,&buf);

  if (val < 5)
    recursed(&buf,val+1,val+1);

  printf("returning from setjmp/longjmp test\n");
}

