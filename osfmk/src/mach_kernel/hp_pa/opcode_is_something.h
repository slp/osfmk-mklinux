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
#define OPCODE_IS_PROBE(z)  (((z) & 0xfc001fa0) == 0x04001180)
#define OPCODE_IS_PROBER(z) (((z) & 0xfc001fe0) == 0x04001180)
#define OPCODE_IS_PROBEW(z) (((z) & 0xfc001fe0) == 0x040011c0)

#define OPCODE_IS_LPA(z)    (((z) & 0xfc003fc0) == 0x04001340)
#define OPCODE_IS_PDC(z)    (((z) & 0xfc003fc0) == 0x04001380)
#define OPCODE_IS_FDC(z)    (((z) & 0xfc003fc0) == 0x04001280)
#define OPCODE_IS_FIC(z)    (((z) & 0xfc001fdf) == 0x04000280)

#define OPCODE_IS_STORE(opcode) \
  ((((opcode) & 0xf0000000) == 0x60000000) || \
   (((opcode) & 0xf4000200) == 0x24000200) || \
   (((opcode) & 0xfc000200) == 0x0c000200))

#define OPCODE_IS_LOAD(opcode) \
  (((((opcode) & 0xf0000000) == 0x40000000) || \
    (((opcode) & 0xf4000200) == 0x24000000) || \
    (((opcode) & 0xfc000200) == 0x0c000000)) && \
   (((opcode) & 0xfc001fc0) != 0x0c0011c0))

#define OPCODE_IS_BRANCH(opcode) \
/*  (((opcode) & 0x80000000) && !((opcode) & 0x10000000)) */ \
  ((((opcode) & 0xf0000000) == 0xe0000000) || \
   (((opcode) & 0xf0000000) == 0xc0000000) || \
   (((opcode) & 0xf0000000) == 0xa0000000) || \
   (((opcode) & 0xf0000000) == 0x80000000))
  
#define OPCODE_IS_CALL(opcode) \
  ((((opcode) & 0xfc00e000) == 0xe8000000) || \
   (((opcode) & 0xfc00e000) == 0xe8004000) || \
   (((opcode) & 0xfc000000) == 0xe4000000))

#define OPCODE_IS_RETURN(opcode) \
  ((((opcode) & 0xfc00e000) == 0xe800c000) || \
   (((opcode) & 0xfc000000) == 0xe0000000))

#define OPCODE_IS_INTERRUPT_RETURN(opcode) \
  ((((opcode) & 0xfc001fc0) == 0x00000ca0))
 
#define OPCODE_IS_UNCONDITIONAL_BRANCH(opcode)  \
  ((((opcode) & 0xf0000000) == 0xe0000000) ||   \
    ((((opcode) & 0xf0000000) == 0xc0000000) && \
     (((opcode) & 0x08000000) == 0x00000000)))
    

