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
 * msg.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * msg.c,v
 * Revision 1.76.2.3.1.2  1994/09/07  04:18:16  menze
 * OSF modifications
 *
 * Revision 1.76.2.3  1994/09/01  18:44:52  menze
 * Subsystem initialization functions can fail
 * allocAdjustMin* becomes allocReserve*
 * Added allocGetwithSize and modified allocGet
 * Added "overflow" reservation rom option
 *
 * Revision 1.76.2.2  1994/07/22  19:37:05  menze
 * Messages store Paths instead of Allocators
 * Msg-related reservations (and rom options) are implemented here now
 * Lost some non-ANSI constructions
 *
 * Revision 1.76.2.1  1994/07/22  19:33:17  menze
 *   [ 1994/04/25         hkaram ]
 *   Allocator-oriented resource-allocation modifications
 *
 * Revision 1.76  1994/03/12  22:02:38  davidm
 * Pointers now printed with "%lx" format and cast to (u_long) to make
 * it compile cleanly on Alphas.
 *
 * Revision 1.75  1994/02/05  00:08:33  menze
 *   [ 1994/01/30          menze ]
 *   assert.h renamed xk_assert.h
 *
 * Revision 1.74  1993/12/14  18:27:42  menze
 *   [ 1993/12/14          menze ]
 *   Fixed a '#else' comment
 *
 *   [ 1993/12/13          menze ]
 *   Modifications from UMass:
 *
 *     [ 93/06/19          yates ]
 *     fixed FINDAFREEONE macro
 *
 *   [ 1993/12/11          menze ]
 *   fixed #endif comments
 *
 *   [ 1993/12/01             ho ]
 *   Added cast of silly tu_leaf function in MSG_NON_REC mode.
 *
 *   [ 1993/10/02          menze ]
 *   Added more strict prototypes
 *
 *   [ 1993/09/28          menze ]
 *   Added a cast to a msgForEach call (fix from cs.umass.edu)
 *
 */

/*
 *  The msg tool package supports tree structured message management.
 *
 *  The algorithms were developed by Peter Druschel in C++
 *
 */

#include <xkern/include/domain.h>
#include <xkern/include/xk_alloc.h>
#include <xkern/include/msg.h>
#include <xkern/framework/msg_internal.h>
#include <xkern/include/assert.h>
#include <xkern/include/xk_debug.h>
#include <xkern/include/platform.h>
#include <xkern/include/romopt.h>
#include <xkern/include/xk_path.h>

static MNodeStruct   dummyStack;

/* limit for number of nodes allocated to a Msg */
#ifdef CONTROL_RESOURCES
static int numNodesHardLimit = 1000;
#endif /* CONTROL_RESOURCES */
static int numNodesSoftLimit = 100;

/* lower bound of the push over flow mnode size */
#define OVERFLOW_LBOUND 128

#if XK_DEBUG
int	tracemsg;
#endif /* XK_DEBUG */

static unsigned		atomicDec( unsigned long * );
static unsigned		atomicInc( unsigned long * );
static boolean_t	frag2Buf( char *, long, void * );
static void		incRef( MNode );
static xkern_return_t	makeContiguous( Msg );
static void 		msgInternalShow( MNode, long, long, long);
static boolean_t 	msgInternalForEach( MNode, long, long, XCharFun,
					    void *);
static boolean_t 	msgIntForEachAlternate( MNode, long, long, XCharFun, 
					       XCharFun, void *);
static char *		msgTopUnderflow( Msg, long );
static MNode    	newMNode(Allocator, u_int);
static MNode    	newMNodePartial(Allocator, u_int);
static MNode    	newMNodeInPlace(Allocator, char *, u_int, MsgCIPFreeFunc,
					void *);
static PNode    	newPNode( Allocator, long, long, MNode, long, long,
				  MNode );
static char *		msgPushOverflow( Msg, long );
static xkern_return_t	readContigRom( char **, int, int, void * );
static xkern_return_t	readNodeRom( char **, int, int, void * );
static xkern_return_t	readOverflowRom( char **, int, int, void * );
static xkern_return_t	readUltimateRom( char **, int, int, void * );
static boolean_t	s_copy( char *, long, void * );

#ifdef MSG_NR_TEST
boolean_t msgnrUserFun(MNode this, long off, long len, void *arg);
void msgnrtest(void);
#endif /* MSG_NR_TEST */

#ifndef MSG_NON_REC
static void	decRef( MNode );
#endif /* MSG_NON_REC */

#define __CONCAT(a,b) a##b

/*
 * Forward:
 */

#ifdef MSG_NON_REC

#define  msgnrpush(stack, item) \
           *stack->top++ = (item); \
           if (stack->top >= stack->limit) Kabort("message stack overflow");

#define  msgnrpop(stack, item) \
  	   ( (--(stack->top) >= stack->bottom) ? \
		((*((MNode * )item) = *(stack->top)) , 1) : 0);

static struct mstack msgStack;

static xkern_return_t msgnrpush_overflow(struct mstack *);
static xkern_return_t msgnrstackinit(struct mstack *);
/* 
 * types of user functions called in ForEach()
 */
typedef boolean_t (*XNodeFun)( struct mstack *, PNode, void * );
typedef boolean_t (*XLeafFun)( MNode );

#endif /* MSG_NON_REC */

static RomOpt	msgOpts[] = {
    { "ultimate", 0, readUltimateRom },
    { "contig", 0, readContigRom },
    { "node", 0, readNodeRom },
    { "inplace", 0, readNodeRom },
    { "overflow", 0, readOverflowRom },
    { "input", 0, 0 }	/* processed in path.c */
};


/* Msg operations */

/* initializer; must be called before any Msg instances are created 	*/
/* returns 1 after successful intialization				*/

xkern_return_t
msgInit()
{
  xTrace0(msg, TR_FULL_TRACE, "msgInit");

  /* init the dummy stack for empty messages */
  dummyStack.type = t_MNodeDummy;
  dummyStack.refCnt = 1;
  dummyStack.b.leaf.size = 0;
  dummyStack.b.leaf.data = NULL;

#ifdef MSG_NON_REC
  if ( msgnrstackinit(&msgStack) == XK_FAILURE ) return XK_FAILURE;
#endif /* MSG_NON_REC */
  findRomOpts("msg", msgOpts, sizeof(msgOpts)/sizeof(RomOpt), 0);

  return XK_SUCCESS;
}


/*
 * Utility functions
 */

static unsigned atomicInc(where)
    unsigned long *where;
{
  return (*where)++;
}

static unsigned atomicDec(where)
    unsigned long *where;
{
  xTrace1(msg, TR_FULL_TRACE, "atomicDec %x", where);
  xTrace1(msg, TR_FULL_TRACE, "atomicDec %x", *where);
  return (*where)--;
}

static void incRef(this)
    MNode this;
{
  xAssert(this != (MNode)0 && this != (MNode)-1);
  xTrace2(msg, TR_MAJOR_EVENTS, "incRef %x refCnt %d", this, this->refCnt);
  xAssert(this->type == t_MNodeLeaf || this->type == t_MNodePair || this->type == t_MNodeUsrPage || this->type == t_MNodeDummy);
 (void) atomicInc(&this->refCnt);
}



#ifndef MSG_NON_REC

/* 
 * decRef  - decrement reference count and free message if necessary
 * 
 * If the reference count reaches 0, the node is freed by the following 
 *  three cases:
 * 1. Recursively follow the b.pair structure and free the top node
 * 2. Free the page
 * 3. Use a node-specific function to free the b.buf.data structure and
 *    free the node structure
 */
static void decRef(this)
    MNode this;
{
  xTrace1(msg, TR_FULL_TRACE, "decRef %x", this);
  xAssert(this != (MNode)0 && this != (MNode)-1 && this->refCnt >= 1);
  xTrace5(msg, TR_MAJOR_EVENTS, "decRef %x refCnt %d addr %x arg addr %x type %d", this, this->refCnt, decRef, &this, this->type);
  xAssert(this->type == t_MNodeLeaf || this->type == t_MNodePair || this->type == t_MNodeUsrPage || this->type == t_MNodeDummy);

  if (atomicDec(&this->refCnt) == 1) {
    switch(this->type) {
    case t_MNodeLeaf:
      allocPut(this->alloc, this);
      break;
    case t_MNodePair:
      decRef(this->b.pair.l.node);
      decRef(this->b.pair.r.node);
      allocPut(this->alloc, this);
      break;
    case t_MNodeUsrPage:
      xTrace3(msg, TR_FULL_TRACE, "decRef t_MNodeUsrPage %x %x %d",
	      this->b.usrpage.bFree,
	      this->b.usrpage.data, this->b.usrpage.size);
      this->b.usrpage.bFree(this->b.usrpage.data, this->b.usrpage.size,
			    this->b.usrpage.freeArg);
      allocPut(this->alloc, this);
      break;
    case t_MNodeDummy:
      break;
    default:
      printf("xkpanic: decRef wrong type %lx %d\n", (u_long) this, this->type);
      xAssert(0);
    }
  }
  xTrace1(msg, TR_FULL_TRACE, "decRef done %x", this);
}

#else

static void msgnrstackreinit(struct mstack *);
static boolean_t msgnrNodeDestroy( struct mstack *, PNode, void  *);
static void msgNodeForEach( MNode, XNodeFun, XLeafFun, void *,  struct mstack *);

static boolean_t decRefnr(this)
    MNode this;
{
  xTrace1(msg, TR_FULL_TRACE, "decRefnr %x", this);
  xAssert(this != (MNode)0 && this != (MNode)-1 && this->refCnt >= 1);
  xTrace5(msg, TR_MAJOR_EVENTS, "decRefnr %x refCnt %d addr %x arg addr %x type %d", this, this->refCnt, decRefnr, &this, this->type);

  if (atomicDec(&this->refCnt) == 1) {
    switch(this->type) {
    case t_MNodeLeaf:
      allocPut(this->alloc, this);
      break;
    case t_MNodeUsrPage:
      xTrace3(msg, TR_FULL_TRACE, "decRefnr t_MNodeUsrPage %x %x %d",
	      this->b.usrpage.bFree,
	      this->b.usrpage.data, this->b.usrpage.size);
      this->b.usrpage.bFree(this->b.usrpage.data, this->b.usrpage.size,
			    this->b.usrpage.freeArg);
      allocPut(this->alloc, this);
      break;
    case t_MNodePair:
      xTrace1(msg, TR_ERRORS, "decRefnr called with pair! %x", this);
      return FALSE;
    case t_MNodeDummy:
      break;
    default:
      printf("xkpanic: decRef wrong type %x %d\n", this, this->type);
      xAssert(0);
    }
  }
  xTrace1(msg, TR_FULL_TRACE, "decRefnonrec done %x", this);
  return TRUE;
}

static void decRef(this)
     MNode this;
{
  xTrace1(msg, TR_FULL_TRACE, "msg: decRef (nonrec) %x", this);
  msgnrstackreinit(&msgStack);
  msgNodeForEach(this, msgnrNodeDestroy, decRefnr, 0, &msgStack);
}
#endif


/* creator/constructors */
static MNode
newMNode(alloc, length)
     Allocator alloc;
     u_int length;
{
  MNode mnode;

  length += MNODE_SIZE;
  mnode = allocGetWithSize(alloc, &length);
  if (mnode != NULL) {
    mnode->type = t_MNodeLeaf;
    mnode->refCnt = 1;
    mnode->alloc = alloc;
    mnode->b.leaf.size = length - MNODE_SIZE;
    mnode->b.leaf.data = ((char *)mnode) + MNODE_SIZE;
  }
  return mnode;
}

static MNode
newMNodePartial(alloc, length)
     Allocator alloc;
     u_int length;
{
  MNode mnode;

  length += MNODE_SIZE;
  mnode = allocGetPartial(alloc, &length, MNODE_SIZE + 1);
  if ( mnode ) {
    mnode->type = t_MNodeLeaf;
    mnode->refCnt = 1;
    mnode->alloc = alloc;
    mnode->b.leaf.size = length - MNODE_SIZE;
    mnode->b.leaf.data = ((char *)mnode) + MNODE_SIZE;
  }
  return mnode;
}

static MNode
newMNodeInPlace(alloc, buffer, length, freefunc, arg)
     Allocator alloc;
     char *buffer;
     u_int length;
     MsgCIPFreeFunc freefunc;
     void *arg;
{
  MNode mnode;

  mnode = allocGet(alloc, sizeof(struct mnode));
  if ( mnode ) {
    mnode->type = t_MNodeUsrPage;
    mnode->refCnt = 1;
    mnode->alloc = alloc;
    mnode->b.usrpage.size = length;
    mnode->b.usrpage.data = buffer;
    mnode->b.usrpage.bFree = freefunc;
    mnode->b.usrpage.freeArg = arg;
  }
  return mnode;
}



static PNode
newPNode(alloc, loff, llen, ln, roff, rlen, rn)
    Allocator alloc;
    long loff, llen, roff, rlen;
    MNode ln, rn;
{
  PNode pnode;

  xTrace0(msg, TR_FULL_TRACE, "newPNode");
  pnode = allocGet(alloc, sizeof(struct mnode));
  if ( pnode ) {
      pnode->refCnt = 1;
      pnode->type = t_MNodePair;
      pnode->alloc = alloc;
      pnode->b.pair.l.offset = loff;
      pnode->b.pair.l.length = llen;
      pnode->b.pair.l.node = ln;
      pnode->b.pair.r.offset = roff;
      pnode->b.pair.r.length = rlen;
      pnode->b.pair.r.node = rn;
  }
  return pnode;
}


static void
msgConstructEmpty( Msg this, Path path )
{
  this->tree = this->stack = &dummyStack;
  incRef(this->tree);
  this->stackHeadPtr = this->stackTailPtr =
    this->headPtr = this->tailPtr = NULL;
  this->state.myStack = 0; /* so that push won't write */
  this->tailstate.myLastStack = 0;
  this->state.numNodes = 1;
  this->lastStack = 0;
  this->path = path;
  this->attr = NULL;
  this->attrLen = 0;
}




xkern_return_t
msgConstructUltimate(this, path, dataLen, stackLen)
    Msg this;
    Path path;
    u_int dataLen;
    u_int stackLen;
{
  PNode root, pair;
  MNode data = NULL;
  MNode stack = NULL;
  int remainLen, stackNodeLen;
  int num_nodes = 0;
  u_int	length;
  Allocator alloc = pathGetAlloc(path, MSG_ALLOC);

  length = stackLen + dataLen;
  /* sanity checks on length arguments */
  if (length == 0) {
      /* create an empty msg */
      msgConstructEmpty(this, path);
      return XK_SUCCESS;
  }

  remainLen = length;

  /* Allocate a contig stack Mnode if a non-zero stackLen
   * was specified. If not create a partial `stack' mnode which
   * really acts as the first data node. This is a stack node 
   * because after all it is the leftmost node.
   */
  if (stackLen != 0)
    stack = newMNode(alloc, stackLen);
  else
    stack = newMNodePartial(alloc, remainLen);
  if ( stack == NULL ) return XK_FAILURE;
  num_nodes++;

  dataLen = (remainLen < stack->b.leaf.size) ? remainLen : stack->b.leaf.size;
  remainLen -= dataLen;
  stackNodeLen = dataLen;

  root = (PNode)stack;

  /* create a linked list of all the pair nodes connecting
     data buffers : the stack node is the leftmost node obviously */
  while (remainLen > 0) {

    /* allocate a data mnode */
    data = newMNodePartial(alloc, remainLen);
    if (data == NULL) {
      decRef(root);
      return XK_FAILURE;
    }
    num_nodes++;

    dataLen = (remainLen < data->b.leaf.size) ? remainLen : data->b.leaf.size;
    remainLen -= dataLen;

    /* allocate a pair node to join the present root and data nodes */
    pair = newPNode(alloc, 0L, length - remainLen - dataLen, root,
		     0L, dataLen, data);
    if (pair == NULL) {
      decRef(data);
      decRef(root);
      return XK_FAILURE;
    }
    num_nodes++;

    root = pair;
  }

  /* initialize the message structure */
  this->stack = stack;
  this->tree = root;
  this->stackHeadPtr = this->stack->b.leaf.data + stackLen;
  this->headPtr = this->stack->b.leaf.data;
  this->stackTailPtr = this->headPtr + stackNodeLen;
  this->tailPtr = this->headPtr + length;
  this->state.myStack = 1;
  this->state.numNodes = num_nodes;
  this->lastStack = data ? data : stack;
  this->lastStackTailPtr = 
    data ? data->b.leaf.data + dataLen : this->stackTailPtr;
  this->tailstate.myLastStack = 1;
  this->path = path;
  this->attr = 0;
  this->attrLen = 0;
  return XK_SUCCESS;
}


static void
initContigMsg( Msg this, Path path, char *buf, u_int stackLen, u_int dataLen )
{
  this->stackHeadPtr = buf + stackLen;
  this->headPtr = buf;
  this->stackTailPtr = this->tailPtr = this->headPtr + stackLen + dataLen;
  this->state.myStack = 1;
  this->state.numNodes = 1;
  this->lastStack = this->stack;
  this->lastStackTailPtr = this->stackTailPtr;
  this->tailstate.myLastStack = 1;
  this->path = path;
  this->attr = NULL;
  this->attrLen = 0;
}


xkern_return_t
msgConstructContig(this, path, dataLen, stackLen)
    Msg this;
    Path path;
    u_int dataLen;
    u_int stackLen;
{
  u_int		length = dataLen + stackLen;
  Allocator	alloc = pathGetAlloc(path, MSG_ALLOC);

  if ( length == 0 ) {
      /* create an empty msg */
      msgConstructEmpty(this, path);
      return XK_SUCCESS;
  }

  this->tree = this->stack = newMNode(alloc, length);
  if ( this->tree == NULL ) {
      return XK_FAILURE;
  }
  initContigMsg(this, path, this->stack->b.leaf.data, stackLen, dataLen);
  return XK_SUCCESS;
}



/* make a data area into a message; the caller's routine is responsible
 *  for eventually freeing the data area. */
xkern_return_t
msgConstructInPlace(this, path, dataLen, stackLen, buffer, freefunc, arg)
     Msg this;
     Path path;
     u_int dataLen;
     u_int stackLen;
     char *buffer;
     MsgCIPFreeFunc freefunc;
     void *arg;
{
  this->tree = this->stack = newMNodeInPlace(pathGetAlloc(path, MSG_ALLOC),
					     buffer, stackLen + dataLen,
					     freefunc, arg);
  if (this->stack == NULL)
    return XK_FAILURE;

  initContigMsg(this, path, buffer, stackLen, dataLen);
  return XK_SUCCESS;
}


/* construct a Msg that is a copy of another Msg			*/
void msgConstructCopy(this, another)
    Msg this, another;
{
  xTrace0(msg, TR_FULL_TRACE, "msgConstructCopy");
  incRef(another->tree);

  *this = *another;
  this->state.myStack = 0;
  this->tailstate.myLastStack = 0;
}


/*
 * msgConstructAppend()
 *
 *  Allocate a buffer and reserve it for appending.
 *  The stack head is at the low address instead of the high.
 *  This will cause an overflow on a msgPush operation, but not
 *  on a msgAppend operation.
 *  Return to the user a pointer to the beginning of the buffer.
 *
 */
static xkern_return_t
msgConstructAppend(
     Msg  this,
     Path path,
     long totalsize,
     char **bufferptr)
{
  xTrace0(msg, TR_FULL_TRACE, "msgConstructAppend");

  xAssert(totalsize > 0 );
  this->tree = this->stack = newMNode(pathGetAlloc(path, MSG_ALLOC), totalsize);
  if ( this->tree == NULL )
    return XK_FAILURE;

  this->stackHeadPtr = this->stack->b.leaf.data;
  this->headPtr = this->stack->b.leaf.data;
  this->stackTailPtr = this->tailPtr = this->stackHeadPtr;
  this->state.myStack = 1;
  this->state.numNodes = 1;
  this->path = path;
  this->attr = NULL;
  this->attrLen = 0;
  this->lastStack = this->stack;
  this->lastStackTailPtr = this->stackTailPtr;
  this->tailstate.myLastStack = 1;
  *bufferptr = this->stackTailPtr;
  return XK_SUCCESS;
}


/*
 *  msgAppend
 *
 *   copy data to the end of the last stack, if there is room;
 *     otherwise, allocate a new buffer and use msgJoin.
 *   NB the user supplies the size of the new message buffer
 *
 */
xkern_return_t
msgAppend(this, appendfunc, tail, tailLen, arg, newlength)
     Msg	this;
     MStoreFun  appendfunc;
     void	*tail;
     void       *arg;
     long	tailLen, newlength;
{
  char		*where;

  xTrace0(msg, TR_FULL_TRACE, "msgAppend");
  xAssert(tailLen >= 0 );

  if (this->tailstate.myLastStack &&
      this->lastStack &&
      ((this->lastStack == this->stack &&
	this->lastStackTailPtr >= this->stackTailPtr &&
	this->lastStackTailPtr + tailLen < this->lastStack->b.leaf.data + this->lastStack->b.leaf.size)
       ||
       (this->lastStackTailPtr + tailLen < this->lastStack->b.leaf.data + this->lastStack->b.leaf.size))) {
    /* cleanest case - there is room in the last stack */
    where = this->lastStackTailPtr;
    appendfunc(tail, where, tailLen, arg);    
    this->lastStackTailPtr += tailLen;
    this->stackTailPtr = this->lastStackTailPtr;
    this->tailPtr += tailLen;
  }
  else {
    /* this is more work; must get a new stack for the tail */
    Msg_s	newMsg;

    if ( msgConstructAppend(&newMsg, this->path, newlength, &where)
		== XK_FAILURE ) {
	return XK_FAILURE;
    }
    appendfunc(tail, where, tailLen, arg);    
    newMsg.stackTailPtr = (newMsg.tailPtr += tailLen);
    newMsg.lastStackTailPtr = newMsg.stackTailPtr;
    msgJoin(this, this, &newMsg);
    msgDestroy(&newMsg);
    /*
     * I lost last stack ownership after the join
     * I should regain it back because the newMsg is gone
     */
    this->tailstate.myLastStack = 1;
  }
  return XK_SUCCESS;
}


/* assignment								*/
void msgAssign(this, another)
    Msg this, another;
{
  xTrace0(msg, TR_FULL_TRACE, "msgAssign");

  if (this != another) {
    incRef(another->tree);
    decRef(this->tree);
    *this = *another;
    this->state.myStack = 0;
    this->tailstate.myLastStack = 0;
  }
}


/* truncate this Msg to length newLength				*/
void msgTruncate(this, newLength)
     Msg this;
     long newLength;
{
  long delta;

  xTrace0(msg, TR_FULL_TRACE, "msgTruncate");
  delta = this->tailPtr - this->stackHeadPtr - newLength;
  if (delta > 0)
    this->tailPtr -= delta; 
  if (this->lastStack == this->stack) {
    this->stackTailPtr = this->tailPtr;
    this->lastStackTailPtr = this->tailPtr;
  }
  else
    this->lastStack = 0;
}


/* remove a chunk of length len from the head of this Msg		*/
/* and assign it to head						*/
void msgChopOff(this, head, len)
     Msg this, head;
     long len;
{
  long mlen = this->tailPtr - this->stackHeadPtr;

  xTrace0(msg, TR_FULL_TRACE, "msgChopOff");

  if (len < 0 || len > mlen) len = mlen;

  if (this != head) {
    incRef(this->tree);
    decRef(head->tree);
    head->stack = this->stack;
    head->tree = this->tree;

    head->headPtr = this->headPtr;
    head->stackHeadPtr = this->stackHeadPtr;
    head->stackTailPtr = this->stackTailPtr;
    head->state = this->state;
    this->state.myStack = 0;  /*  giving stack away */
    this->tailstate.myLastStack = 0;  /*  giving stack away */

    /* pop off the header */
    this->stackHeadPtr += len;
  }
  /* truncate the header */
  head->tailPtr = head->stackHeadPtr + len;
}

/* assign to this Msg the concatenation of Msg1 and Msg2		*/
xkern_return_t
msgJoin(this, msg1, msg2)
     Msg this, msg1, msg2;
{
  long msg1Length = msg1->tailPtr - msg1->stackHeadPtr;
  long msg2Length = msg2->tailPtr - msg2->stackHeadPtr;
  long msg2Offset;

  xTrace0(msg, TR_FULL_TRACE, "msgJoin");
  if (msg1Length == 0) {
    /* result is just msg2 */
    msgAssign(this, msg2);
  }
  else if (msg2Length == 0) {
    /* result is just msg1 */
    msgAssign(this, msg1);
  }
  else {
    PNode pnode;

    /* create a new pair node */
    msg2Offset = msg2->stackHeadPtr - msg2->headPtr;
    pnode  = newPNode(pathGetAlloc(msg1->path, MSG_ALLOC), 0L,
			      msg1->tailPtr - msg1->headPtr, msg1->tree,
			      msg2Offset, msg2Length, msg2->tree);
    if (pnode == NULL) return XK_FAILURE;

    /* increment refCnts      */
    incRef(msg1->tree);
    incRef(msg2->tree);
    /* cleanup my old binding */
    decRef(this->tree);

    this->tree = pnode;
    this->headPtr = msg1->headPtr;
    this->tailPtr = msg1->tailPtr + msg2Length;
    this->stack = msg1->stack;
    this->stackHeadPtr = msg1->stackHeadPtr;
    this->stackTailPtr = msg1->stackTailPtr;
    this->state.myStack = (this == msg1) && this->state.myStack;
    this->state.numNodes = msg1->state.numNodes + msg2->state.numNodes + 1;
    this->tailstate.myLastStack = (this == msg2) && msg2->tailstate.myLastStack;
    this->lastStack = msg2->lastStack;
    this->lastStackTailPtr = msg2->lastStackTailPtr;
    this->path = msg1->path;
#ifdef CONTROL_RESOURCES
    if (this->state.numNodes > numNodesHardLimit) makeContiguous();
#endif /* CONTROL_RESOURCES */
  }
  return XK_SUCCESS;
}

static char *msgPushOverflow(this, hdrLen)
    Msg this;
    long hdrLen;
{
  MNode	newMN, newPN;
  long offset = this->stackHeadPtr - this->headPtr;
  long length = this->tailPtr - this->stackHeadPtr;
  long mnodeSize;
  Allocator alloc = pathGetAlloc(this->path, MSG_ALLOC);

  xTrace0(msg, TR_FULL_TRACE, "msgPushOverflow");


  mnodeSize = (hdrLen >= OVERFLOW_LBOUND) ? hdrLen : OVERFLOW_LBOUND;
  /* create a new root node and a new stack */
  if ( ! (newMN = newMNode(alloc, mnodeSize)) ) {
      return NULL;
  }

  xAssert(hdrLen <= newMN->b.leaf.size);

  newPN =  newPNode(alloc, 0L, newMN->b.leaf.size,
		    newMN, offset, length, this->tree);
  if ( newPN == NULL ) {
      allocPut(alloc, newMN);
      return NULL;
  }
  this->stack = newMN;
  this->tree = newPN;
  this->stackTailPtr = this->stack->b.leaf.data + this->stack->b.leaf.size;
  this->stackHeadPtr = this->stackTailPtr - hdrLen;
  this->headPtr = this->stack->b.leaf.data;
  this->tailPtr = this->stackTailPtr + length;
  this->state.myStack = 1;
  this->state.numNodes += 2;
  return this->stackHeadPtr;
}

/* push a header onto this Msg						*/
xkern_return_t
msgPush(this, store, hdr, hdrLen, arg)
    Msg this;
    MStoreFun store;
    void *hdr;
    long hdrLen;
    void *arg;
{
  char *where;

  xTrace0(msg, TR_FULL_TRACE, "msgPush");
  if ((this->state.myStack ||
       (this->tree == this->stack &&  /* see if we can cheaply own the stack */
        this->tree->refCnt == 1 &&
	(this->state.myStack = 1))) &&	/* = rather than == intended */
      this->stackHeadPtr <= this->stackTailPtr &&
      this->stackHeadPtr - this->stack->b.leaf.data >= hdrLen) {
    this->stackHeadPtr -= hdrLen;
    where = this->stackHeadPtr;
  }
  else {
    /* need a new stack */
    if ( (where = msgPushOverflow(this, hdrLen)) == NULL )
      return XK_FAILURE;
  }
  store(hdr, where, hdrLen, arg);
  return XK_SUCCESS;
}


/*
 * helper function called during foreach()
 * to copy the stack in topUnderflow()
 */
struct arg {
  char *buf;
  long size;
};
static boolean_t
s_copy(buf, len, arg)
    char *buf;
    long len;
    void *arg;
{
  struct arg *a = (struct arg *)arg;
  long chunk_size;
  int chunki;

  xTrace0(msg, TR_FULL_TRACE, "s_copy");
  chunk_size = (len < a->size) ? len : a->size;
  /* bcopy takes an int, so be careful */
  chunki = chunk_size;
  xAssert( (long) chunki == chunk_size);
  bcopy(buf, a->buf, chunki);
  a->buf += chunk_size;
  a->size -= chunk_size;
  return (a->size != 0);
}



/* 
 * msgTopUnderflow - restructure a message with short initial node
 *
 * returns a pointer to the beginning of the data
 *
 * When the first node of a message has too little data to satisfy a
 * msgPop request, the message is restructured to make the requested
 * data reside in one stack.
 *
 * This is an inefficient operation that should be rarely called.
 */

static char *msgTopUnderflow(this, hdrLen)
    Msg this;
    long hdrLen;
{
  struct arg c_args;
  MNode newStack;
  Allocator alloc = pathGetAlloc(this->path, MSG_ALLOC);

  xTrace0(msg, TR_FULL_TRACE, "msgTopUnderflow");
  xTrace0(msg, TR_SOFT_ERROR, "msgTopUnderflow");

  if (this->stackHeadPtr == this->stackTailPtr) {
    /* we are right at the boundary of the current stack */
    /* find the leftmost leaf node just to the right of the stack */
    PNode newLeaf;
    PNode newNode = (PNode)(this->tree);
    long newOff = this->stackHeadPtr - this->headPtr;
    long newLen = 0;

    while (newNode->type == t_MNodePair) {
      if (newOff < newNode->b.pair.l.length) {
	/* explore the left subtree */
	newOff += newNode->b.pair.l.offset;
	newLen = newNode->b.pair.l.length;
	newNode = (PNode)newNode->b.pair.l.node;
      }
      else {
	/* explore the right subtree */
	newOff = newOff - newNode->b.pair.l.length + newNode->b.pair.r.offset;
	newLen = newNode->b.pair.r.length;
	newNode = (PNode)newNode->b.pair.r.node;
      } 
    }
    /* found a leaf node */
    newLeaf = (PNode)newNode;
    newLen -= newOff;

    /* printf("found a leaf node, newLen=%d\n", newLen); */
    /* check if the leaf node contains the entire requested head */
    if (newLen >= hdrLen) {
      long offset, length;

      /* just make the leaf node the new stack */
      this->stack = newLeaf;
      
      offset = this->stackHeadPtr - this->headPtr;
      length = this->tailPtr - this->stackHeadPtr;
      this->stackHeadPtr = this->stack->b.leaf.data + newOff;
      this->stackTailPtr = this->stackHeadPtr + newLen;
      this->headPtr = this->stackHeadPtr - offset;
      this->tailPtr = this->stackHeadPtr + length;
      this->state.myStack = 0; 
      return this->stackHeadPtr;
    }

    /* else we have to copy after all... */
  }
  xTrace0(msg, TR_SOFT_ERROR, "msgtopunderflow will copy");

  /* create a new root node and a new stack */
  newStack = newMNode(alloc, hdrLen);
  if ( newStack == NULL ) return NULL;

  xAssert(hdrLen <= newStack->b.leaf.size);

  /* fill the new stack with the requested head */
  c_args.buf = newStack->b.leaf.data + newStack->b.leaf.size - hdrLen;
  c_args.size = hdrLen;
  /* overflow checks handled by msgForEach */
  msgForEach(this, s_copy, &c_args);

  /* assign new message */
  {
    long offset = this->stackHeadPtr - this->headPtr;
    long length = this->tailPtr - this->stackHeadPtr;

    this->tree = newPNode(alloc, 0L, newStack->b.leaf.size, newStack,
			  offset + hdrLen, length - hdrLen, this->tree);
    if ( this->tree == NULL ) {
      allocPut(newStack->alloc, newStack);
      return NULL;
    }

    this->stack = newStack;
    this->stackTailPtr = this->stack->b.leaf.data + this->stack->b.leaf.size;
    this->stackHeadPtr = this->stackTailPtr - hdrLen;
    this->headPtr = this->stack->b.leaf.data;
    this->tailPtr = this->stackHeadPtr + length;
    this->state.myStack = 1;
    this->state.numNodes += 2;
    return this->stackHeadPtr;
  }
}


/* msgPop
 *
 * pop a header from this Msg
 * returns 1 after successful pop
 *   
 * calls msgTopUnderflow if necessary
 */
xkern_return_t
msgPop(this, load, hdr, hdrLen, arg)
    Msg this;
    MLoadFun load;
    void *hdr;
    long hdrLen;
    void *arg;
{
  char *where;
  long actualLen;
  long length = this->tailPtr - this->stackHeadPtr;

  xTrace0(msg, TR_FULL_TRACE, "msgPop");
  xAssert(hdrLen>=0);
  if (!hdrLen) xTrace0(msg, TR_SOFT_ERROR, "msgPop zero length - useless");

  if (hdrLen <= length) {
    /* Msg is long enough */
    if (this->stackHeadPtr + hdrLen <= this->stackTailPtr) {
      where = this->stackHeadPtr;
    }
    else if ( (where = msgTopUnderflow(this, hdrLen)) == NULL ) {
      return XK_FAILURE;
    }
    actualLen = load(hdr, where, hdrLen, arg);
    xAssert(actualLen >= 0 && actualLen <= hdrLen);

    if (actualLen > 0) {  /* if a "peek", preserve ownerships */
      this->stackHeadPtr += actualLen;
      this->state.myStack = 0;

      /* quick check to see if we can still keep last stack ownerships */
      if (this->lastStack == this->stack)
	this->tailstate.myLastStack = 0;
    }
    return XK_SUCCESS;
  }
  return XK_FAILURE;
}


/* msgPopDiscard
 *
 * pop and discard an object of length len
 * returns TRUE after successful pop
 */
boolean_t msgPopDiscard(this, len)
    Msg this;
    long len;
{
  long length;

  xTrace0(msg, TR_FULL_TRACE, "msgPopDiscard");
  xAssert(len>=0);
  xIfTrace(msg, TR_SOFT_ERROR) {
    if (len==0)
      printf("msgPopDiscard of length 0; useless call");
  }
  length = this->tailPtr - this->stackHeadPtr;
  if (len > length) len = length;
  this->stackHeadPtr += len;
  this->state.myStack = 0;
  this->tailstate.myLastStack = 0;
  return TRUE;
}



xkern_return_t
msgSetAttr(
    Msg		this,
    int		name,
    void	*attr,
    int		len )
{
    /* 
     * Only the default attribute (name == 0) is supported at this time. 
     */
    if ( name != 0 ) {
	xTrace1(msg, TR_SOFT_ERRORS,
		"msgSetAttr called with unsupported name %d", name);
	return XK_FAILURE;
    }
#if XK_DEBUG
    if ( this->attr ) {
	xTrace0(msg, TR_SOFT_ERRORS,
		"msgSetAttr overriding previous non-null attribute");
    }
#endif
    this->attr = attr;
    this->attrLen = len;
    return XK_SUCCESS;
}


void *
msgGetAttr( this, name )
    Msg		this;
    int		name;
{
    /* 
     * Only the default attribute (name == 0) is supported at this time. 
     */
    if ( name != 0 ) {
	xTrace1(msg, TR_SOFT_ERRORS,
		"msgGetAttr called with unsupported name %d", name);
	return 0;
    }
    return this->attr;
}

/*
 * Forward:
 */
static void	msgPairShow( MNode, long, long, long );
static void	msgLeafShow( MNode, long, long, long );

static char blanks[] = "                                ";

#ifdef MSG_NON_REC
/*
 *  Message Iteration Functions
 *
 *  Routines for processing the message data; hides the message tree
 *   structure and lengths of node buffers
 */

/*
 *  We use a single stack for processing message nodes: msgStack
 *
 *  MP-safe versions will have to put a lock on getting the
 *  msgStack.  Probably will have a pool of stacks.
 *
 */
static xkern_return_t
msgnrstackinit(stack)
     struct mstack *stack;
{
  xTrace0(msg, TR_FULL_TRACE, "msgnrstackinit");

  stack->top = stack->bottom = (MNode *)xSysAlloc(MSG_NR_STK_SZ);
  if ( ! stack->top ) {
      return XK_FAILURE;
  }
  stack->size = MSG_NR_STK_SZ;
  stack->limit = (MNode *)(((char *)(stack->bottom)) + MSG_NR_STK_SZ);
  return XK_SUCCESS;
}

static void
msgnrpush_overflow(stack)
     struct mstack *stack;
{
  char *newstack;
  int newsize = 2 * (stack->size +4) -4;

  xTrace0(msg, TR_FULL_TRACE, "msgnrpush_overflow");

  xAssert (stack->top < stack->limit);

  if ( (newstack = xSysAlloc(newsize)) == 0 ) {
      Kabort("msgnrpush_overflow");
  }
  bcopy(stack->bottom, newstack, stack->size);
  allocFree(stack->bottom);
  stack->top = (MNode *)(newstack + ((char *)stack->top -
				     (char *)stack->bottom));
  stack->bottom = (MNode *)newstack;
  stack->size = newsize;
  stack->limit = (MNode *)(((char *)(stack->bottom)) + stack->size);
  xTrace1(msg, TR_SOFT_ERROR, "msgnrpush_overflow overflow to %d", stack->size);
}

static void msgNodeForEach(this, fnode, fleaf, arg, stack)
    MNode this;
    XNodeFun fnode;
    XLeafFun fleaf;
    void *arg;
    struct mstack *stack;
{
  boolean_t stackcontinue = TRUE;
  boolean_t nodecontinue = TRUE;

  xTrace0(msg, TR_FULL_TRACE, "msgNodeForEach");

  while (stackcontinue==TRUE && nodecontinue==TRUE) {
    switch(this->type) {
    case t_MNodeLeaf:
      (void)( (fleaf)(this));
      break;
    case t_MNodePair:
      nodecontinue = (fnode)(stack, this, arg);
      break;
    case t_MNodeUsrPage:
      (void)( (fleaf)(this));
      break;
    case t_MNodeDummy:
      break;
    default:
      xAssert(0);
    }
    stackcontinue = msgnrpop(stack, &this);
    xAssert(this);
  }
}

/* 
 * msgnrNodeDestroy  - decrement reference count and free message if necessary
 * 
 * If the reference count reaches 0, the node is freed
 */
static boolean_t msgnrNodeDestroy(stack, this, arg)
    struct mstack *stack;
    PNode this;
    void  *arg;
{
  xTrace1(msg, TR_FULL_TRACE, "msgnrNodeDestroy %x", this);
  xAssert(this != (MNode)0 && this != (MNode)-1 && this->refCnt >= 1);
  xTrace5(msg, TR_MAJOR_EVENTS, "msgnrNodeDestroy %x refCnt %d addr %x arg addr %x type %d", this, this->refCnt, decRef, &this, this->type);

  if (atomicDec(&this->refCnt) == 1)
    if (this->type == t_MNodePair) {
      xAssert(this->b.pair.l.node);
      msgnrpush(stack, this->b.pair.l.node);
      xAssert(this->b.pair.r.node);
      msgnrpush(stack, this->b.pair.r.node);
      allocPut(this->alloc, this);
    }
  xTrace1(msg, TR_EVENTS, "msgnrNodeDestroy done %x", this);
  return TRUE;
}


void msgDestroy(this)
     Msg this;
{
  xTrace0(msg, TR_FULL_TRACE, "msgDestroy non rec");
  decRef(this->tree);
}

#ifdef MSG_NR_TEST

boolean_t msgnrUserFun(this, off, len, arg)
     MNode this;
     long off, len;
     void *arg;
{
  printf("Msg User func: %x %x %ld %ld\n", this, arg, off, len);
  return TRUE;
}

struct Uargs {
  long a, b; 
} UserArgs;

void msgnrtest()
{
  Msg m1, m2;
  char *buf;
  int i=0;

  msgConstructAllocate(&m1, 1024, &buf);
  while (i++ < 1000) {
    msgConstructAllocate(&m2, 1024, &buf);
    if (i < 10)  msgShow(&m1);
    /* if ( !(i % 100) ) msgShow(&m1); */
    msgJoin(&m1, &m2, &m1);
  }
  msgShow(&m1);
  msgForEach(&m1, msgnrUserFun, &UserArgs);
  msgDestroy(&m1);
}

#endif /* MSG_NR_TEST */

/*
 * Forward:
 */
/* 
 * types of user functions called in ForEach()
 */
typedef boolean_t (*XNodeFunArgs)( struct mstack *, MNode, long, long );
typedef boolean_t (*XLeafFunArgs)( struct mstack *, MNode, long, long );
static boolean_t	msgnrPairArgs( struct mstack *, PNode, long, long );
static boolean_t	msgnrLeafForEach( struct mstack *, MNode, long, long );
static boolean_t	msgnrLeafShow( struct mstack *, MNode, long, long );
static void	msgNonRecForEach(struct mstack *, MNode, XNodeFunArgs, XLeafFunArgs, long, long );


static void msgnrstackreinit(stack)
     struct mstack *stack;
{
  xTrace0(msg, TR_FULL_TRACE, "msgnrstackreinit");

  stack->top = stack->bottom;
  stack->arg = 0;
  stack->indent = 0;
  stack->func = 0;
}

static boolean_t msgnrPairArgs(stack, this, off, len)
    PNode this;
    struct mstack *stack;
    long off, len;
{
  long chunk_size, lchunk=0, loff=0;
  struct nlink *k, *l;
  int indent = stack->indent;

  xTrace0(msg, TR_FULL_TRACE, "msgnrPairArgs");
  if (indent > sizeof(blanks)) indent = sizeof(blanks);
  xTrace5(msg, TR_FULL_TRACE, "%.*sPair: refCnt = %d off = %d len = %d\n",
	  indent, blanks, this->refCnt, off, len);
  /* left child */
  l = &this->b.pair.l;
  chunk_size = (off < l->length) ? l->length - off : 0;
  if (chunk_size > len) chunk_size  = len;
  if (chunk_size) {
    loff = l->offset + off;
    lchunk = chunk_size;
    len -= chunk_size;
  }
  
  off = off - l->length + chunk_size;

  /* right child */
  k = &this->b.pair.r;
  chunk_size = (off < k->length) ? k->length - off : 0;
  if (chunk_size > len) chunk_size  = len;
  if (chunk_size) {
    msgnrpush(stack, k->node);
    msgnrpush(stack, (MNode)(k->offset+off));
    msgnrpush(stack, (MNode)chunk_size);
  }
  if (lchunk) {
    msgnrpush(stack, l->node);
    msgnrpush(stack, (MNode)loff);
    msgnrpush(stack, (MNode)lchunk);
  }
  stack->indent += 2;
  return TRUE;
}

static boolean_t msgnrLeafShow(stack, this, off, len)
    struct mstack *stack;
    MNode this;
    long off, len;
{
  int lim, i;
  char *c;
  int indent = stack->indent;
  
  xTrace0(msg, TR_FULL_TRACE, "msgnrLeafShow");
  xAssert(this->b.leaf.size - off >= len);
  if (indent > sizeof(blanks)) indent = sizeof(blanks);
  printf("%.*sLeaf: %#x refCnt %d off %d len %d", indent, blanks, 
	 this->b.leaf.data, this->refCnt, off, len);
  /*
   * Display first 8 characters of data
   */
  printf("\tdata: ");
  lim = len < 8 ? len : 8;
  for (c=this->b.leaf.data+off, i=0; i < lim; c++, i++) {
    printf("%.2x ", *c & 0xff);
  }
  putchar('\n');
  stack->indent -= 2;
  return TRUE;
}

static void msgNonRecForEach(stack, this, fnode, fleaf, arg1, arg2)
    struct mstack *stack;
    MNode this;
    XNodeFunArgs fnode;
    XLeafFunArgs fleaf;
    long arg1, arg2;
{
  boolean_t stackcontinue = TRUE;
  boolean_t nodecontinue = TRUE;

  xTrace0(msg, TR_FULL_TRACE, "msgNonRecForEach");

  while (stackcontinue==TRUE && nodecontinue==TRUE) {
    switch(this->type) {
    case t_MNodeLeaf:
      (void)( (fleaf)(stack, this, arg1, arg2));
      break;
    case t_MNodePair:
      nodecontinue = (fnode)(stack, this, arg1, arg2);
      break;
    case t_MNodeUsrPage:
      (void)( (fleaf)(stack, this, arg1, arg2));
      break;
    case t_MNodeDummy:
      break;
    default:
      xAssert(0);
    }
    stackcontinue = msgnrpop(stack, &arg2);
    stackcontinue = msgnrpop(stack, &arg1);
    stackcontinue = msgnrpop(stack, &this);
    xAssert(this);
  }
}

void msgShow(this)
    Msg this;
{
  long offset = this->stackHeadPtr - this->headPtr;
  long length = this->tailPtr - this->stackHeadPtr;

  xTrace0(msg, TR_FULL_TRACE, "msgShow nonrec");
  printf("Msg: Stack=%#x, stackHeadPtr=%#x, stackTailPtr=%#x, myStack=%d, myLastStack %d, numNodes=%d offset %ld length %ld\n",
	 this->stack, this->stackHeadPtr, this->stackTailPtr,
	 this->state.myStack, this->tailstate.myLastStack,
	 this->state.numNodes,
	 offset, length);
  msgnrstackreinit(&msgStack);
  if (length > 0)
    (void) msgNonRecForEach(&msgStack, this->tree, msgnrPairArgs, msgnrLeafShow, offset, length);
}

static boolean_t msgnrLeafForEach(stack, this, off, len)
     struct mstack *stack;
     MNode this;
     long off, len;
{
  xTrace0(msg, TR_FULL_TRACE, "msgnrLeafForEach");
  xAssert(this->b.leaf.size - off >= len);
  return (stack->func)(this->b.leaf.data + off, len, stack->arg);
}

/* for every contig in this Msg, invoke the function f with 		*/
/* arguments const char *buf (=address of contig), long len 		*/
/* (=length of contig), and void * arg (=user-supplied argument),  	*/
/* while f returns TRUE							*/
void msgForEach(this, f, arg)
    Msg this;
    XCharFun f;
    void *arg;
{
  long offset = this->stackHeadPtr - this->headPtr;
  long length = this->tailPtr - this->stackHeadPtr;

  xTrace0(msg, TR_FULL_TRACE, "msgForEach nonrec");

  /* try the common case of one leaf */
  if (this->tree->type == t_MNodeLeaf || this->tree->type == t_MNodeUsrPage) {
    (f)(this->tree->b.leaf.data + offset, length, arg);
    return;
  }

  msgnrstackreinit(&msgStack);
  msgStack.func = f;
  msgStack.arg = arg;
  (void) msgNonRecForEach(&msgStack, this->tree, msgnrPairArgs, msgnrLeafForEach, offset, length);
}


#else

/* destructor								*/
void msgDestroy(this)
    Msg this;
{
  xTrace1(msg, TR_FULL_TRACE, "msgDestroy arg this: %x", this);
  decRef(this->tree);
  xTrace0(msg, TR_FULL_TRACE, "msgDestroy done");
}

void msgShow(
    Msg this )
{
  long offset = this->stackHeadPtr - this->headPtr;
  long length = this->tailPtr - this->stackHeadPtr;

  xTrace0(msg, TR_FULL_TRACE, "msgShow");
  printf("Msg: Stack=%#lx, stackHeadPtr=%#lx, stackTailPtr=%#lx, myStack=%d, myLastStack %d, numNodes=%d offset %ld length %ld\n",
	 (u_long) this->stack, (u_long) this->stackHeadPtr,
	 (u_long) this->stackTailPtr,
	 this->state.myStack, this->tailstate.myLastStack,
	 this->state.numNodes,
	 offset, length);
  if (length > 0)
    (void) msgInternalShow(this->tree, offset, length, 0L);
}

/* for every contig in this Msg, invoke the function f with 		*/
/* arguments const char *buf (=address of contig), long len 		*/
/* (=length of contig), and void * arg (=user-supplied argument),  	*/
/* while f returns TRUE							*/

void msgForEach(this, f, arg)
    Msg this;
    XCharFun f;
    void *arg;
{
  long offset = this->stackHeadPtr - this->headPtr;
  long length = this->tailPtr - this->stackHeadPtr;

  xTrace0(msg, TR_FULL_TRACE, "msgForEach");
  (void) msgInternalForEach(this->tree, offset, length, f, arg);
}


#endif /* MSG_NON_REC */

static boolean_t msgPairForEach( PNode, long, long, XCharFun, void * );
static boolean_t msgLeafForEach( PNode, long, long, XCharFun, void * );

static boolean_t msgInternalForEach(this, off, len, f, arg)
    MNode this;
    long off, len;
    XCharFun f;
    void *arg;
{
  xTrace0(msg, TR_FULL_TRACE, "msgInternalForEach");
  switch(this->type) {
    case t_MNodeLeaf:
      return msgLeafForEach(this, off, len, f, arg);
    case t_MNodePair:
      return msgPairForEach(this, off, len, f, arg);
    case t_MNodeUsrPage:
      return msgLeafForEach(this, off, len, f, arg);
    case t_MNodeDummy: return TRUE;
    default:
      xAssert(0);
  }
  /*
   * To make the compiler happy
   */
  return FALSE;
}

static boolean_t msgPairForEach(this, off, len, f, arg)
    PNode this;
    long off, len;
    XCharFun f;
    void *arg;
{
  long chunk_size;
  struct nlink *k;

  xTrace0(msg, TR_FULL_TRACE, "msgPairForEach");
  /* left child */
  k = &this->b.pair.l;
  chunk_size = (off < k->length) ? k->length - off : 0;
  if (chunk_size > len) chunk_size  = len;
  if (chunk_size) {
    if (!msgInternalForEach(k->node, k->offset + off, chunk_size, f, arg))
      return 0;
    len -= chunk_size;
  }
  off = off - k->length + chunk_size;

  /* right child */
  k = &this->b.pair.r;
  chunk_size = (off < k->length) ? k->length - off : 0;
  if (chunk_size > len) chunk_size  = len;
  if (chunk_size) {
    if (!msgInternalForEach(k->node, k->offset + off, chunk_size, f, arg))
      return 0;
  }
  return 1;
}

static boolean_t msgLeafForEach(this, off, len, f, arg)
    MNode this;
    long off, len;
    XCharFun f;
    void *arg;
{
  xTrace0(msg, TR_FULL_TRACE, "msgLeafForEach");
  xAssert(this->b.leaf.size - off >= len);
  return f(this->b.leaf.data + off, len, arg);
}

static boolean_t msgPairForEachAlternate( PNode, long, long, XCharFun, XCharFun, void * );
static boolean_t msgUsrLeafForEachAlternate( PNode, long, long, XCharFun, XCharFun, void * );

void msgForEachAlternate(this, f, fUsr, arg)
    Msg this;
    XCharFun f;
    XCharFun fUsr;
    void *arg;
{
  long offset = this->stackHeadPtr - this->headPtr;
  long length = this->tailPtr - this->stackHeadPtr;

  xTrace0(msg, TR_FULL_TRACE, "msgForEachAlternate");
  (void) msgIntForEachAlternate(this->tree, offset, length, f, fUsr, arg);
}

static boolean_t msgIntForEachAlternate(this, off, len, f, fUsr, arg)
    MNode this;
    long off, len;
    XCharFun f;
    XCharFun fUsr;
    void *arg;
{
  xTrace0(msg, TR_FULL_TRACE, "msgIntForEachAlternate");
  switch(this->type) {
    case t_MNodeLeaf:
      return msgLeafForEach(this, off, len, f, arg);
    case t_MNodePair:
      return msgPairForEachAlternate(this, off, len, f, fUsr, arg);
    case t_MNodeUsrPage:
      return msgUsrLeafForEachAlternate(this, off, len, f, fUsr, arg);
    case t_MNodeDummy: return TRUE;
    default:
      xAssert(0);
  }
  /*
   * To make the compiler happy
   */
  return FALSE;
}

static boolean_t msgPairForEachAlternate(this, off, len, f, fUsr, arg)
    PNode this;
    long off, len;
    XCharFun f;
    XCharFun fUsr;
    void *arg;
{
  long chunk_size;
  struct nlink *k;

  xTrace0(msg, TR_FULL_TRACE, "msgPairForEachAlternate");
  /* left child */
  k = &this->b.pair.l;
  chunk_size = (off < k->length) ? k->length - off : 0;
  if (chunk_size > len) chunk_size  = len;
  if (chunk_size) {
    if (!msgIntForEachAlternate(k->node, k->offset + off, chunk_size, f, (XCharFun)fUsr, arg))
      return 0;
    len -= chunk_size;
  }
  off = off - k->length + chunk_size;

  /* right child */
  k = &this->b.pair.r;
  chunk_size = (off < k->length) ? k->length - off : 0;
  if (chunk_size > len) chunk_size  = len;
  if (chunk_size) {
    if (!msgIntForEachAlternate(k->node, k->offset + off, chunk_size, f, (XCharFun)fUsr, arg))
      return 0;
  }
  return 1;
}

boolean_t msgUsrLeafForEachAlternate(this, off, len, f, fUsr, arg)
    MNode this;
    long off, len;
    XCharFun f;
    XCharFun fUsr;
    void *arg;
{
  xTrace0(msg, TR_FULL_TRACE, "msgUsrLeafForEachAlternate");
  xAssert(this->b.leaf.size - off >= len);
  if (fUsr)
      return fUsr(this->b.leaf.data + off, len, arg);
  return f(this->b.leaf.data + off, len, arg);
}

static void msgInternalShow(this, off, len, indent)
    MNode this;
    long off, len, indent;
{
    xTrace0(msg, TR_FULL_TRACE, "msgInternalShow");
    switch(this->type) {
      case t_MNodeLeaf:
	msgLeafShow(this, off, len, indent);
	break;
      case t_MNodePair:
	msgPairShow(this, off, len, indent);
	break;
      case t_MNodeUsrPage:
	msgLeafShow(this, off, len, indent);
	break;
      default:
	break;
    }
}

static void msgPairShow(this, off, len, indent)
    PNode this;
    long off, len, indent;
{
  long chunk_size;
  struct nlink *k;

  xTrace0(msg, TR_FULL_TRACE, "msgPairShow");
  printf("%.*sPair: refCnt = %ld off = %ld len = %ld\n", (int) indent, blanks, 
	 this->refCnt, off, len);
  /* left child */
  k = &this->b.pair.l;
  chunk_size = (off < k->length) ? k->length - off : 0;
  if (chunk_size > len) chunk_size  = len;
  if (chunk_size) {
    msgInternalShow(k->node, k->offset + off, chunk_size, indent+2);
    len -= chunk_size;
  }
  off = off - k->length + chunk_size;

  /* right child */
  k = &this->b.pair.r;
  chunk_size = (off < k->length) ? k->length - off : 0;
  if (chunk_size > len) chunk_size  = len;
  if (chunk_size) {
    msgInternalShow(k->node, k->offset + off, chunk_size, indent+2);
  }
}

static void msgLeafShow(this, off, len, indent)
    MNode this;
    long off, len, indent;
{
  int lim, i;
  char *c;
  
  xTrace0(msg, TR_FULL_TRACE, "msgLeafShow");
  xAssert(this->b.leaf.size - off >= len);
  printf("%.*sLeaf: %#lx refCnt %ld off %ld len %ld", (int) indent, blanks, 
	 (u_long) this->b.leaf.data, this->refCnt, off, len);
  /*
   * Display first 8 characters of data
   */
  printf("\tdata: ");
  lim = len < 80 ? len : 80;    /* travos: was 8 XXX */
  for (c=this->b.leaf.data+off, i=0; i < lim; c++, i++) {
    printf("%.2x ", *c & 0xff);
  }
  putchar('\n');
}

/* copy this entire Msg into contiguous storage */
static xkern_return_t
makeContiguous(this)
     Msg this;
{
  long length;
  struct arg c_args;
  Msg_s tmp;

  xTrace0(msg, TR_FULL_TRACE, "makeContiguous");

  length = this->tailPtr - this->stackHeadPtr;
  xAssert(length >= 0);

  if (this->stack == this->tree) 
    /* this Msg is contiguous */
    return XK_SUCCESS;

  /* create a new Msg with a contiguous buffer */
  if ( msgConstructContig(&tmp, this->path, length, 0) == XK_FAILURE ) {
      return XK_FAILURE;
  }
  c_args.buf = this->stackHeadPtr;

  /* copy into the new Msg */
  c_args.size = length;
  msgForEach(this, s_copy, &c_args);

  /* assign the new Msg */
  msgAssign(this, &tmp);
  msgDestroy(&tmp);
  return XK_SUCCESS;
}

/* perform housecleaning to free unnecessary resources allocated to this msg */
void msgCleanUp(this)
     Msg this;
{
#ifdef undef
  long offset = this->stackHeadPtr - this->headPtr;
#endif
  long length = this->tailPtr - this->stackHeadPtr;

  if (length > 0) {
    if (this->state.numNodes > numNodesSoftLimit) makeContiguous(this);

#ifdef undef
    /* prune the tree */
    MNode oldTree = tree;

    treePrune(offset, length, &offset, tree);
    
    if (tree != oldTree) {
      /* the root node was pruned */
      /* must change headPtr to reflect the new offset */
      this->headPtr = this->stackHeadPtr - offset;
    }

    /* if the stack was empty, it may have been deallocated */
    /* set it not-owned, so push won't try to write into it */
    this->state.myStack = 0;
    this->tailstate.myLastStack = 0;
#endif /* undef */

  } else {
    /* this Msg is empty */
    /* free its resources */
    Msg_s empty;
    if ( msgConstructContig(&empty, this->path, 0, 0) == XK_SUCCESS ) {
	msgAssign(this, &empty);
	msgDestroy(&empty);
    }
  }
}


static boolean_t   
frag2Buf( frag, len, bufPtr )
    char	*frag;
    long	len;
    void 	*bufPtr;
{
    xTrace3(msg, TR_FUNCTIONAL_TRACE, "frag2Buf copying %d bytes from %x to %x",
	    len, (int)frag, (int)(*(char **)bufPtr));
    bcopy(frag, *(char **)bufPtr, len);
    *(char **)bufPtr += len;
    return TRUE;
}


void
msg2buf( msg, buf )
    Msg		msg;
    char	*buf;
{
    char	*ptr = buf;

    xTrace2(msg, TR_FUNCTIONAL_TRACE, "msg2Buf (%x, %x)", (int)msg, (int)buf);
    msgForEach(msg, frag2Buf, &ptr);
    xTrace0(msg, TR_FULL_TRACE, "msg2Buf returns");
}

boolean_t
msgDataPtr( char *ptr, long len, void *arg )
{
    *(char **)arg = ptr;
    return 0;
}

void
msgStats()
{
}


/* 
 * In minContig, minUltimate and minNode, we allocate extra space
 * for one PNode per MNode.  This should reserve enough PNode space
 * for all but the most pathological of messages. 
 */
static xkern_return_t
minContig( Allocator alloc, int nMsgs, u_int *size )
{
    u_int	mnodeSize = sizeof(struct mnode);

    xTrace2(msg, TR_FULL_TRACE, "msgReserveContig, %d bytes, %d msgs",
	    *size, nMsgs);
    if ( allocReserve(alloc, nMsgs, &mnodeSize) == XK_FAILURE ) {
	return XK_FAILURE;
    }
    *size += MNODE_SIZE;
    if ( allocReserve(alloc, nMsgs, size) == XK_FAILURE ) {
	allocReserve(alloc, -nMsgs, &mnodeSize);
	return XK_FAILURE;
    }
    *size -= MNODE_SIZE;
    return XK_SUCCESS;
}


xkern_return_t
msgReserveContig( Path path, int nMsgs, u_int dataSize, u_int stackSize )
{
    u_int	size = dataSize + stackSize;

    return minContig(pathGetAlloc(path, MSG_ALLOC), nMsgs, &size);
}


xkern_return_t
msgReserveUltimate( Path path, int nMsgs, u_int dataSize, u_int stackSize )
{
    u_int		mnodeSize = MNODE_SIZE;
    u_int		blocksPerMsg;
    u_int		reqSize;
    xkern_return_t	xkr;
    Allocator		alloc = pathGetAlloc(path, MSG_ALLOC);

    xTrace4(msg, TR_FULL_TRACE,
	    "msgReserveUltimate [%s], data %d, stack %d, %d msgs",
	    alloc->name, dataSize, stackSize, nMsgs);
    reqSize = stackSize + dataSize;
    if ( stackSize ) {
	if ( minContig(alloc, nMsgs, &stackSize) == XK_FAILURE ) {
	    return XK_FAILURE;
	}
    }
    if ( stackSize > reqSize ) {
	return XK_SUCCESS;
    }
    reqSize -= stackSize;
    xkr = allocReservePartial(alloc, nMsgs, &reqSize, MNODE_SIZE,
			      &blocksPerMsg);
    if ( xkr == XK_SUCCESS ) {
	/* 
	 * Extra space for pair-nodes
	 */
	xkr = allocReserve(alloc, blocksPerMsg * nMsgs, &mnodeSize);
	if ( xkr == XK_SUCCESS ) {
	    return XK_SUCCESS;
	}
	allocReservePartial(alloc, -nMsgs, &reqSize, MNODE_SIZE, &blocksPerMsg);
    }
    if ( stackSize ) {
	minContig(alloc, -nMsgs, &stackSize);
    }
    return XK_FAILURE;
}


xkern_return_t
msgReserveNode( Path path, int nNodes )
{
    u_int	mnodeSize = MNODE_SIZE;

    xTrace1(msg, TR_FULL_TRACE, "msgReserveNode, %d nodes", nNodes);
    return allocReserve(pathGetAlloc(path, MSG_ALLOC), nNodes, &mnodeSize);
}


xkern_return_t
msgReserveInPlace( Path path, int nMsgs )
{
    return msgReserveNode(path, nMsgs);
}


xkern_return_t
msgReserveInput( Path path, int nMsgs, u_int msgSize, char *dev )
{
    return pathReserveInput(path, nMsgs, msgSize, dev);
}


/* 
 * msg ultimate PATH_NAME N DATA_SIZE STACK_SIZE
 */
static xkern_return_t
readUltimateRom( char **str, int nFields, int line, void *arg )
{
    int		N, dataSize, stackSize;
    Path	p;
    
    if ( (p = getPathByName(str[2])) == 0 ) {
	sprintf(errBuf, "Couldn't find path %s", str[2]);
	xError(errBuf);
	return XK_FAILURE;
    }
    N = atoi(str[3]); if ( N < 0 ) return XK_FAILURE;
    dataSize = atoi(str[4]); if ( dataSize < 0 ) return XK_FAILURE;
    stackSize = atoi(str[5]); if ( stackSize < 0 ) return XK_FAILURE;
    xTrace4(msg, TR_EVENTS, "msg ROM option: ultimate %s %d %d %d",
	    str[2], N, dataSize, stackSize);
    if ( msgReserveUltimate(p, N, (u_int)dataSize, (u_int)stackSize)
		== XK_FAILURE ) {
	xError("msgReserveUltimate from ROM entry fails");
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


/* 
 * msg contig PATH_NAME N DATA_SIZE STACK_SIZE
 */
static xkern_return_t
readContigRom( char **str, int nFields, int line, void *arg )
{
    int		N, dataSize, stackSize;
    Path	p;
    
    if ( (p = getPathByName(str[2])) == 0 ) {
	sprintf(errBuf, "Couldn't find path %s", str[2]);
	xError(errBuf);
	return XK_FAILURE;
    }
    N = atoi(str[3]); if ( N < 0 ) return XK_FAILURE;
    dataSize = atoi(str[4]); if ( dataSize < 0 ) return XK_FAILURE;
    stackSize = atoi(str[5]); if ( stackSize < 0 ) return XK_FAILURE;
    xTrace4(alloc, TR_EVENTS, "msg ROM option: contig %s %d %d %d",
	    str[2], N, dataSize, stackSize);
    if ( msgReserveContig(p, N, (u_int)dataSize, (u_int)stackSize)
		== XK_FAILURE ) {
	xError("msgReserveContig from ROM entry fails");
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


/* 
 * msg node PATH_NAME N 
 */
static xkern_return_t
readNodeRom( char **str, int nFields, int line, void *arg )
{
    int		numNodes;
    Path	p;

    if ( (p = getPathByName(str[2])) == 0 ) {
	sprintf(errBuf, "Couldn't find path %s", str[2]);
	xError(errBuf);
	return XK_FAILURE;
    }
    numNodes = atoi(str[3]); if (numNodes < 0) return XK_FAILURE;
    xTrace2(alloc, TR_EVENTS, "alloc ROM option: node %s %d",
	    str[2], numNodes);
    if ( msgReserveNode(p, numNodes) == XK_FAILURE ) {
	xError("msgReserveNode from ROM entry fails");
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}

/* 
 * msg overflow PATH_NAME N 
 */
static xkern_return_t
readOverflowRom( char **str, int nFields, int line, void *arg )
{
    int		n;
    Path	p;
    u_int	size = OVERFLOW_LBOUND;

    if ( (p = getPathByName(str[2])) == 0 ) {
	sprintf(errBuf, "Couldn't find path %s", str[2]);
	xError(errBuf);
	return XK_FAILURE;
    }
    n = atoi(str[3]); if (n < 0) return XK_FAILURE;
    xTrace2(alloc, TR_EVENTS, "alloc ROM option: node %s %d",
	    str[2], n);
    if ( minContig(pathGetAlloc(p, MSG_ALLOC), n, &size) == XK_FAILURE ) {
	xError("msgReserveOverflow from ROM entry fails");
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}

