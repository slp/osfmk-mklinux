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
/*
 * msg_internal.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.22
 * Date: 1993/07/03 00:49:41
 */

/* this should be a power of two; see PAGEROUND in msg.c */


/* enum NodeType */

enum NodeType {
  t_MNodeJunk,
  t_MNodeLeaf,
  t_MNodePair,
  t_MNodeUsrPage,
  t_MNodeDummy
};


#define mprtype t_MNodePair
#define mlftype t_MNodeLeaf
#define muptype t_MNodeUsrPage

/*
 *     the left-right node pair constituent
 */

struct nlink {
  long offset;
  long length;
  struct mnode *node;
};


/*
 * struct mnode
 * all tree nodes
 */
struct mnode {
  enum NodeType type;
  unsigned long refCnt;
  Allocator alloc;
  union {
    struct {
      struct nlink l, r;
    } pair;
    struct {
      long  size;     /* size of buffer */
      char *data;     /* the buffer */
    } leaf;
    struct {
      long  size;     /* size of buffer */
      char *data;     /* the buffer */
      MsgCIPFreeFunc	bFree; /* the buffer's deallocator function */
      void *freeArg;
    } usrpage;
  } b;
};


/* 
 * Make sure the data starts on a well-aligned boundary
 */
struct dummy {
    struct mnode m;
    long   data;
};
#define MNODE_SIZE	offsetof(struct dummy, data)


/* non-recursive routines for traversing a message */

#ifdef MSG_NON_REC

#define MSG_NR_STK_SZ (8*1024 - 4)

struct mstack {
  int size;
  MNode *top, *bottom, *limit;
  XCharFun func;
  void *arg;
  int indent;
};

#endif /* MSG_NON_REC */
