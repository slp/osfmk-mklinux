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
 * msg_s.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * msg_s.h,v
 * Revision 1.12.2.4  1994/07/21  23:21:46  menze
 * Messages now store Paths rather than Allocators
 *
 * Revision 1.12.2.3  1994/03/31  16:52:55  menze
 * Allocator field declaration changed
 *
 * Revision 1.12.2.2  1994/03/30  19:05:30  hkaram
 * Added the allocator field to the Msg structure
 *
 * Revision 1.12.2.1  1994/03/25  23:07:30  hkaram
 * Resource allocation modifications
 *
 * Revision 1.12  1993/12/14  18:23:35  menze
 *   [ 1993/12/11          menze ]
 *   fixed #endif comments
 *
 */

/*
 * Independent definition of the message structure.  Protocols should
 * include msg.h which includes this file.
 */

#ifndef msg_s_h
#define msg_s_h

typedef struct mnode MNodeStruct, *MNode, *MNodeLeaf, *PNode;

/*
 * the Msg structure
 */

#ifndef xtype_h
#include <xkern/include/xtype.h>
#endif

typedef struct msg {
  char	     *headPtr;
  char	     *tailPtr;
  char	     *stackHeadPtr;
  char	     *stackTailPtr;
  struct {
    unsigned short numNodes;
    unsigned int   myStack;
  } state;
  MNodeLeaf  stack;
  MNode      tree;
  void       *attr;
  MNodeLeaf  lastStack;
  char	     *lastStackTailPtr;
  struct {
    unsigned int   myLastStack;
  } tailstate;
  struct path *path;
  int	attrLen;
} Msg_s;

#endif /* msg_s_h */
