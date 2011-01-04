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

#include <device/net_io.h>
#include <vm/vm_kern.h>


/*
 *	Compilation of a source network filter into hp700 instructions.
 */

#define USE_EXTRA_REGS 0

/* We could profitably import reg.h into the microkernel.  But for now: */
#define REG_ARG0 26
#define REG_ARG1 25
#define REG_ARG2 24
#define REG_ARG3 23
#define REG_MRP	 31
#define REG_RET0 28
#define REG_RET1 29
#define REG_RTN	  2
#define REG_SP	 30
#define REG_T1	 22
#define REG_T2	 21
#define REG_T3	 20
#define REG_T4	 19

/* Originally we dealt in virtual register numbers which were essentially
   indexes into this array, and only converted to machine register numbers
   when emitting instructions.  But that meant a lot of conversions, so
   instead we deal with machine register numbers all along, even though this
   means wasting slots in the regs[] array.  */
const char scratchregs[] = {
    1, REG_T1, REG_T2, REG_T3, REG_T4, REG_ARG3, REG_RET1, REG_MRP,
#if USE_EXTRA_REGS	/* Callee-saves regs available if we save them. */
#define INITIAL_NSCRATCHREGS 8	/* Number of registers above. */
    3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
#endif
};
#define NSCRATCHREGS (sizeof scratchregs / sizeof scratchregs[0])
#define NREGS 32
#define NO_REG 0

#define LDO_BITS 13	/* Offset bits in an ldo instruction (there are
			   actually 14, but the offset is signed).  */
#define MAX_LDO ((1 << LDO_BITS) - 1)
#define COMICLR_BITS 10	/* Immediate bits (again, 11 signed) in a comiclr.  */
#define MAX_COMICLR ((1 << COMICLR_BITS) - 1)

#define LDIL(n, r)	(0x20000000 | ((r) << 21) | IM21L(n))
#define IM21L(n)	(xassert((n) < 0x10000 && !((n) & 0x700)), \
			 (((n) & 0x1800) << 1) | (((n) & 0xe000) << 3))
#if 0
#define IM21L(n)	((((n) >> 31) & 1) | (((n) >> 19) & 0xffe) | \
			 (((n) >> 4) & 0xc000) | (((n) << 3) & 0x1f0000) | \
			 (((n) << 1) & 0x3000))
/* I am convinced that the PA-RISC opcodes were designed by maniacs.  */
#endif

#define LDO(n, r, s)	(0x34000000 | ((r) << 21) | ((s) << 16) | IM14(n))
#define IM14(n)		(xassert(n >= 0), ((n) << 1))
#define LDI(n, r)	LDO(n, 0, r)

#define ARITH_OP(op, cond, r, s, t) \
			(0x08000000 | (op) | (cond) | ((r) << 16) | \
			 ((s) << 21) | (t))
#define OP_ADD		(0x18 << 6)
#define OP_SUB		(0x10 << 6)
#define OP_AND		(0x08 << 6)
#define OP_OR		(0x09 << 6)
#define OP_XOR		(0x0a << 6)
#define OP_COMCLR	(0x22 << 6)
#define COND_NEVER	0
#define COND_EQ		1
#define COND_ULT	4
#define COND_ULE	5
#define ARITH_COND(c, f) \
			((((c) << 1) | (f)) << 12)
#define ARITH_NEVER	ARITH_COND(COND_NEVER, 0)
#define ARITH_EQ	ARITH_COND(COND_EQ, 0)
#define ARITH_NE	ARITH_COND(COND_EQ, 1)
#define ARITH_ULT	ARITH_COND(COND_ULT, 0)
#define ARITH_ULE	ARITH_COND(COND_ULE, 0)
#define ARITH_UGT	ARITH_COND(COND_ULE, 1)
#define ARITH_UGE	ARITH_COND(COND_ULT, 1)
#define COPY(r, s)	ARITH_OP(OP_OR, ARITH_NEVER, r, 0, s)

#define COMICLR(c, n, r, s) \
			(xassert((n) <= MAX_COMICLR), \
			 0x90000000 | (c) | \
			 ((n) << 1) | ((r) << 21) | ((s) << 16))

#define COMB_SKIP_1(cond, not, nullify, r, s) \
			(0x80000008 | ((cond) << 13) | ((not) << 27) | \
			 ((r) << 16) | ((s) << 21) | ((nullify) << 1))

#define LDHX_S(i, r, s)	(0x0c002040 | ((i) << 16) | ((r) << 21) | (s))

#define LDH(n, r, s)	(xassert((n) >= 0), \
			 0x44000000 | ((n) << 1) | ((r) << 21) | ((s) << 16))

#define EXTRU(r, p, l, s) \
			(0xd0001800 | ((r) << 21) | ((p) << 5) | (32 - (l)) | \
			 (s) << 16)

#define SUBI(n, r, s)	(xassert((n) >= 0), \
			 0x94000000 | ((n) << 1) | ((r) << 21) | ((s) << 16))

#define ZVDEP(r, l, s)	(0xd4000000 | ((r) << 16) | (32 - (l)) | ((s) << 21))

#define VEXTRU(r, l, s)	(0xd0001000 | ((r) << 21) | (32 - (l)) | ((s) << 16))

#define BV_X(n, i, r)	(0xe800c000 | ((n) << 1) | ((i) << 16) | ((r) << 21))
#define BV(i, r)	BV_X(0, i, r)
#define BV_N(i, r)	BV_X(1, i, r)

#define MTSAR(r)	(0x01601840 | ((r) << 16))

#define LDW_STW(store, modify, r, neg, n, s) \
			(0x48000000 | ((store) << 29) | ((modify) << 26) | \
			 ((r) << 16 | ((s) << 21) | \
			 (((neg) ? (1 << 13) - (n) : (n)) << 1) | (neg)))
#define STWM(r, n, s)	(xassert(n > 0), LDW_STW(1, 1, r, 0, n, s))
#define STW_NEG(r, n, s) \
			LDW_STW(1, 0, r, 1, n, s)

#define LDW_NEG(n, s, r) \
			LDW_STW(0, 0, r, 1, n, s)
#define LDWM_NEG(n, s, r) \
			LDW_STW(0, 1, r, 1, n, s)

#define BL(d, r)	(xassert((unsigned)(d) < (unsigned)(1 << 11)), \
			 0xe8000000 | ((r) << 21) | ((d) << 3))
/* This only codes shortish branches, to avoid the bitfiddling needed to
   put the more significant bits in the right place.  */

/* Every NETF_arg generates at most three instructions (3 for PUSHWORD).
   Every NETF_op generates at most 4 instructions (4 for LSH).  */
#define MAX_INSTR_PER_ARG 3
#define MAX_INSTR_PER_OP  4
#define MAX_INSTR_PER_ITEM (MAX_INSTR_PER_ARG + MAX_INSTR_PER_OP)
int junk_filter[MAX_INSTR_PER_ITEM];

enum {NF_LITERAL, NF_HEADER, NF_DATA};
struct common {	/* Keeps track of values we might want to avoid reloading. */
    char type;	/* NF_LITERAL: immediate; NF_HEADER: header word;
		   NF_DATA: data word. */
    char nuses;	/* Number of remaining uses for this value. */
    char reg;	/* Register this value is currently in, or 0 if none. */
    unsigned short value;
		/* Immediate value or header or data offset. */
};
struct reg {	/* Keeps track of the current contents of registers. */
    char commoni;	/* Index in common[] of the contained value. */
#define NOT_COMMON_VALUE NET_MAX_FILTER	/* When not a common[] value. */
    char stacktimes;	/* Number of times register appears in stack. */
};
struct local {	/* Gather local arrays so we could kalloc() if needed.  */
    struct common common[NET_MAX_FILTER];	/* Potentially common values. */
    struct reg regs[NREGS];			/* Register statuses. */
    char commonpos[NET_MAX_FILTER];		/* Index in common[] for the
						   value loaded in each filter
						   command. */
    char stackregs[NET_FILTER_STACK_DEPTH];	/* Registers making up the
						   stack. */
#if USE_EXTRA_REGS
    char maxreg;
#endif
};

int allocate_register(struct local *s, int commoni);
void xassert(int cond);
void *kmem_alloc_exec(vm_size_t size);
#if USE_EXTRA_REGS
int compile_preamble(int *instructions, struct local *s);
#endif

/* Compile a packet filter into PA-RISC machine code.  We do everything in
   the 8 callee-saves registers listed in scratchregs[], except when
   USE_EXTRA_REGS is defined, in which case we may also allocate caller-
   saves registers if needed.

   Rather than maintaining an explicit stack in memory, we allocate registers
   dynamically to correspond to stack elements -- we can do this because we
   know the state of the stack at every point in the filter program.  We also
   attempt to keep around in registers values (immediates, or header or data
   words) that are used later on, to avoid having to load them again.
   Since there are only 8 registers being used, we might be forced to reload
   a value that we could have kept if we had more.  We might even be unable
   to contain the stack in the registers, in which case we return failure and
   cause the filter to be interpreted by net_do_filter().  But for all current
   filters I looked at, 8 registers is enough even to avoid reloads.  When
   USE_EXTRA_REGS is defined there are 24 available registers, which is
   plenty.

   We depend heavily on NET_MAX_FILTER and NET_FILTER_STACK_DEPTH being
   small.  We keep indexes to arrays sized by them in char-sized fields,
   originally because we tried allocating these arrays on the stack.
   Even then we overflowed the small (4K) kernel stack, so we were forced
   to allocate the arrays dynamically, which is the reason for the existence
   of `struct local'.

   We also depend on the filter being logically correct, for instance not
   being longer than NET_MAX_FILTER or underflowing its stack.  This is
   supposed to have been checked by parse_net_filter() before the filter
   is compiled.

   We are supposed to return 1 (TRUE) if the filter accepts the packet
   and 0 (FALSE) otherwise.  In fact, we may return any non-zero value
   for true, which is sufficient for our caller and convenient for us.

   There are lots and lots of optimisations that we could do but don't.
   This is supposedly a *micro*-kernel, after all.  Here are some things
   that could be added without too much headache:
   - More registers.  We trash ret0 in the various premature-return cases,
     whereas with slightly longer code we could preserve it and use it with
     the other registers.  We could note the last uses of arg0, arg1, and
     arg2, and convert them to general registers after those uses.  But if
     register shortage turns out to be a problem it is probably best just
     to define USE_EXTRA_REGS and have done with it.
   - Minimising range checks.  Every time we refer to a word in the data
     part, we generate code to ensure that it is within bounds.  But often
     the truth of these tests is implied by earlier tests.  Instead, at the
     start of the filter and after every COR or CNAND we could insert
     a single check when that is necessary.  (After CAND and CNOR we don't
     need to check since if they terminate it will be to return FALSE
     anyway so all we'd do would be to return it prematurely.)
   - Remembering immediate values.  Instead of generating code as soon as we
     see a PUSHLIT, we could remember that value and only generate code when
     it is used.  This would enable us to generate certain shorter
     instructions (like addib) that incorporate the immediate value instead
     of ever putting it in a register.  Also, when emitting code to load an
     immediate that is too big for a single ldi instruction, we could
     check other registers to see if any already holds a value that we
     could ldo from to get the value desired.
 */

filter_fct_t
net_filter_alloc(filter_t *filter, unsigned int size, unsigned int *lenp)
{
    struct local *s;
    int len, oldi, i, j, ncommon, sp;
    int type, value, arg, op, reg, reg1, dst, commoni;
    int *instructions, *instp;
#if USE_EXTRA_REGS
    int oldmaxreg;
#endif
    boolean_t compiling;

#define SCHAR_MAX 127	/* machine/machlimits->h, anyone? */
    assert(NET_MAX_FILTER <= SCHAR_MAX);
    assert(NET_FILTER_STACK_DEPTH <= SCHAR_MAX);
    assert(NREGS <= SCHAR_MAX);

    assert(size < NET_MAX_FILTER);

    s = (struct local *) kalloc(sizeof *s);

#if USE_EXTRA_REGS
    s->maxreg = INITIAL_NSCRATCHREGS;
#endif
    len = 0;
    compiling = FALSE;

    /* This loop runs at least twice, once with compiling==FALSE to determine
       the length of the instructions we will compile, and once with
       compiling==TRUE to compile them.  The code generated on the two passes
       must be the same.  In the USE_EXTRA_REGS case, the loop can be re-run
       an extra time while !compiling, if we decide to use the callee-saves
       registers.  This is because we may be able to generate better code with
       the help of these registers than before.  */
    while (1) {

	/* Identify values that we can potentially preserve in a register to
	   avoid having to reload them.  All immediate values and references to
	   known offsets in the header or data are candidates.  The results of
	   this loop are the same on every run, so with a bit of work we
	   could run it just once; but this is not a time-critical
	   application.  */
	ncommon = 0;
	for (i = 0; i < size; i++) {
	    oldi = i;
	    arg = NETF_ARG(filter[i]);
	    if (arg == NETF_PUSHLIT) {
		type = NF_LITERAL;
		value = filter[++i];
		if (value == 0)
		    continue;
	    } else if (arg >= NETF_PUSHSTK) {
		continue;
	    } else if (arg >= NETF_PUSHHDR) {
		type = NF_HEADER;
		value = arg - NETF_PUSHHDR;
	    } else if (arg >= NETF_PUSHWORD) {
		type = NF_DATA;
		value = arg - NETF_PUSHWORD;
	    } else {
		continue;
	    }
	    for (j = 0; j < ncommon; j++) {
		if (s->common[j].type == type && s->common[j].value == value) {
		    s->common[j].nuses++;
		    break;
		}
	    }
	    if (j == ncommon) {
		s->common[j].type = type;
		s->common[j].value = value;
		s->common[j].nuses = 1;
		ncommon++;
	    }
	    s->commonpos[oldi] = j;
	}

#if USE_EXTRA_REGS
	oldmaxreg = s->maxreg;
#endif

	/* Initially, no registers hold common values or are on the stack.  */
	for (i = 0; i < ncommon; i++)
	    s->common[i].reg = NO_REG;
	for (i = 0; i < NSCRATCHREGS; i++) {
	    s->regs[scratchregs[i]].commoni = NOT_COMMON_VALUE;
	    s->regs[scratchregs[i]].stacktimes = 0;
	}

	/* Now read through the filter and generate code. */
	sp = -1;	/* sp points to top element */
	for (i = 0; i < size; i++) {
	    if (!compiling)
		instp = junk_filter;

	    assert(sp >= -1);
	    assert(sp < NET_FILTER_STACK_DEPTH - 1);
	    commoni = s->commonpos[i];
	    arg = NETF_ARG(filter[i]);
	    op = NETF_OP(filter[i]);

	    /* Generate code to get the required value into a register and
	       set `reg' to the number of this register. */
	    switch (arg) {
	    case NETF_PUSHLIT:
		value = filter[++i];
		reg = s->common[commoni].reg;
		if (reg == 0) {
		    if ((reg = allocate_register(s, commoni)) == 0)
			goto fail;
		    assert(value >= 0);	/* Comes from unsigned short. */
		    if (value > MAX_LDO) {
			*instp++ = LDIL(value & ~MAX_LDO, reg);
			value &= MAX_LDO;
			if (value != 0)
			    *instp++ = LDO(value, reg, reg);
		    } else
			*instp++ = LDO(value, 0, reg);
		}
		s->common[commoni].nuses--;
		break;
	    case NETF_NOPUSH:
		reg = s->stackregs[sp--];
		s->regs[reg].stacktimes--;
		break;
	    case NETF_PUSHZERO:
		reg = 0;
		break;
	    case NETF_PUSHIND:
	    case NETF_PUSHHDRIND:
		reg1 = s->stackregs[sp--];
		s->regs[reg1].stacktimes--;
		if (arg == NETF_PUSHIND)
		    *instp++ = ARITH_OP(OP_COMCLR, ARITH_ULT, reg1, REG_ARG1,
					REG_RET0);
						/* comclr,< <reg1>,arg1,ret0 */
		else
		    *instp++ = COMICLR(ARITH_UGT,
				       NET_HDW_HDR_MAX/sizeof (unsigned short),
				       reg1, REG_RET0);
						/* comiclr,> N,<reg1>,ret0 */
		assert((NET_HDW_HDR_MAX / sizeof(unsigned short)) <=
		       MAX_COMICLR);
		*instp++ = BV_N(0, REG_RTN);	/* bv,n (rp) */
		if ((reg = allocate_register(s, -1)) == 0)
		    goto fail;
		*instp++ = LDHX_S(reg1,
				  (arg == NETF_PUSHIND) ? REG_ARG0 : REG_ARG2,
				  reg); 	/* ldhx,s reg1(arg0/2),reg */
		break;
	    default:
		if (arg >= NETF_PUSHSTK)
		    reg = s->stackregs[sp - (arg - NETF_PUSHSTK)];
		else if (arg >= NETF_PUSHWORD) {
		    assert(2 * (NETF_PUSHHDR - NETF_PUSHWORD) <= MAX_LDO);
		    assert(NETF_PUSHHDR - NETF_PUSHWORD <= MAX_COMICLR);
		    assert(NETF_PUSHSTK - NETF_PUSHHDR <= MAX_LDO);
		    reg = s->common[commoni].reg;
		    if (reg == 0) {
			if ((reg = allocate_register(s, commoni)) == 0)
			    goto fail;
			if (arg < NETF_PUSHHDR) {
			    value = arg - NETF_PUSHWORD;
			    *instp++ = COMICLR(ARITH_ULT, value, REG_ARG1,
					       REG_RET0);
						/* comiclr,< value,arg1,ret0 */
			    *instp++ = BV_N(0, REG_RTN);
						/* bv,n (rp) */
			    reg1 = REG_ARG0;
			} else {
			    value = arg - NETF_PUSHHDR;
			    reg1 = REG_ARG2;
			}
			*instp++ = LDH(2 * value, reg1, reg);
		    }
		    s->common[commoni].nuses--;
		}
	    }

	    /* Now generate code to do `op' on `reg1' (lhs) and `reg' (rhs). */
	    if (op != NETF_NOP) {
		reg1 = s->stackregs[sp--];
		s->regs[reg1].stacktimes--;
	    }
	    switch (op) {
	    case NETF_OP(NETF_CAND):
	    case NETF_OP(NETF_COR):
	    case NETF_OP(NETF_CNAND):
	    case NETF_OP(NETF_CNOR):
		dst = -1;
	    case NETF_OP(NETF_NOP):
		break;
	    default:
		/* Allocate a register to put the result in. */
		if ((dst = allocate_register(s, -1)) == 0)
		    goto fail;
	    }
	    switch (op) {
	    case NETF_OP(NETF_NOP):
		dst = reg;
		break;
	    case NETF_OP(NETF_EQ):
	    case NETF_OP(NETF_LT):
	    case NETF_OP(NETF_LE):
	    case NETF_OP(NETF_GT):
	    case NETF_OP(NETF_GE):
	    case NETF_OP(NETF_NEQ):
		switch (op) {
		case NETF_OP(NETF_EQ): j = ARITH_NE; break;
		case NETF_OP(NETF_LT): j = ARITH_UGE; break;
		case NETF_OP(NETF_LE): j = ARITH_UGT; break;
		case NETF_OP(NETF_GT): j = ARITH_ULE; break;
		case NETF_OP(NETF_GE): j = ARITH_ULT; break;
		case NETF_OP(NETF_NEQ): j = ARITH_EQ; break;
		}
		*instp++ = ARITH_OP(OP_COMCLR, j, reg1, reg, dst);
		*instp++ = LDI(1, dst);
		break;
	    case NETF_OP(NETF_AND):
	    case NETF_OP(NETF_OR):
	    case NETF_OP(NETF_XOR):
	    case NETF_OP(NETF_ADD):
	    case NETF_OP(NETF_SUB):
		switch (op) {
		case NETF_OP(NETF_AND): j = OP_AND; break;
		case NETF_OP(NETF_OR): j = OP_OR; break;
		case NETF_OP(NETF_XOR): j = OP_XOR; break;
		case NETF_OP(NETF_ADD): j = OP_ADD; break;
		case NETF_OP(NETF_SUB): j = OP_SUB; break;
		}
		*instp++ = ARITH_OP(j, ARITH_NEVER, reg1, reg, dst);
		if (op == NETF_OP(NETF_ADD) || op == NETF_OP(NETF_SUB))
		    *instp++ = EXTRU(dst, 31, 16, dst);
		/* Adds and subtracts can produce results that don't fit in
		   16 bits so they have to be masked.  The logical operations
		   can't so they don't.  */
		break;
	    case NETF_OP(NETF_LSH):
	    case NETF_OP(NETF_RSH):
		*instp++ = SUBI(31, reg, REG_RET0);
		*instp++ = MTSAR(REG_RET0);
		if (op == NETF_OP(NETF_LSH)) {
		    *instp++ = ZVDEP(reg1, 32, dst);
		    *instp++ = EXTRU(dst, 31, 16, dst);
		} else
		    *instp++ = VEXTRU(reg, 32, dst);
		/* For some reason, all arithmetic is done in 16 bits,
		   so the result of LSH has to be masked with 0xFFFF.  The
		   result of RSH doesn't since it can't be any bigger than
		   the 16-bit value that was shifted.
		   We use ret0 to compute the shift amount because we can't use
		   reg or reg1 (which might have values we subsequently use),
		   nor dst (which might be the same as reg1).  Alternatively, we
		   could allocate another register, but we would need to
		   temporarily do s->regs[dst].stacktimes++ to avoid just
		   getting dst again.  */
		break;
	    case NETF_OP(NETF_COR):
		/* comb,<>,n reg1,reg,$x | bv (rp) | ldi 1,ret0 | $x:
		   I have found no way to do this in less than three
		   instructions (as for the other NETF_C* operations), unless
		   it be to branch to a "bv (rp) | ldi 1,ret0" postamble,
		   and what would be the point in that?  */
		*instp++ = COMB_SKIP_1(COND_EQ, 1, 1, reg1, reg);
		*instp++ = BV(0, REG_RTN);
		*instp++ = LDI(1, REG_RET0);
		break;
	    case NETF_OP(NETF_CNAND):
		/* xor,= reg1,reg,ret0 | bv,n (rp)
		   This leaves a non-zero (true) value in ret0 if the values
		   are different.  */
		*instp++ = ARITH_OP(OP_XOR, ARITH_EQ, reg1, reg, REG_RET0);
		*instp++ = BV_N(0, REG_RTN);
		break;
	    case NETF_OP(NETF_CAND):
	    case NETF_OP(NETF_CNOR):
		/* comclr,{=|<>} reg1,reg,ret0 | bv,n (rp) */
		j = (op == NETF_OP(NETF_CAND)) ? ARITH_EQ : ARITH_NE;
		*instp++ = ARITH_OP(OP_COMCLR, j, reg1, reg, REG_RET0);
		*instp++ = BV_N(0, REG_RTN);
		break;
	    default:
		printf("op == 0x%x\n", op);
		panic("net_filter_alloc: bad op");
		/* Should have been caught by parse_net_filter(). */
	    }
	    /* If the op generated a result, push it on the stack. */
	    if (dst >= 0) {
		s->stackregs[++sp] = dst;
		s->regs[dst].stacktimes++;
	    }
	    if (!compiling) {
		assert(instp - junk_filter <= MAX_INSTR_PER_ITEM);
		len += instp - junk_filter;
	    }
	}
	if (compiling) {
	    /* If the stack contains any values, we are supposed to return 0 or
	       1 according as the top-of-stack is zero or not.  Since the only
	       place we are called requires just zero-false/nonzero-true, we
	       simply copy the value into ret0.  If the stack is empty, we
	       return TRUE.  */
	    *instp++ = BV(0, REG_RTN);			/* bv (rp) */
	    if (sp >= 0)
		*instp++ = COPY(s->stackregs[sp], REG_RET0);
	    else
		*instp++ = LDI(1, REG_RET0);
	    break;
	} else {
	    len += 2;
#if USE_EXTRA_REGS
	    if (s->maxreg > oldmaxreg) {
		len = 0;
		continue;
	    }
	    len += compile_preamble(NULL, s);
#endif
	}
	if ((instructions = kmem_alloc_exec(len * sizeof (int))) == NULL)
	    return NULL;
	instp = instructions;
#if USE_EXTRA_REGS
	instp += compile_preamble(instp, s);
#endif
	compiling = TRUE;
    }

    assert(instp - instructions == len);
    *lenp = len * sizeof (int);
    fdcache(HP700_SID_KERNEL, (vm_offset_t)instructions, len * sizeof (int));
    kfree((vm_offset_t) s, sizeof *s);
    return (filter_fct_t) instructions;
fail:
    assert(!compiling);
    kfree((vm_offset_t) s, sizeof *s);
    printf("net_filter_alloc: failed to compile (filter too complex)\n");
    printf("-- will work, but more slowly; consider enabling USE_EXTRA_REGS\n");
    return NULL;
}


/* Allocate a register.  Registers that are already being used to make up
   the virtual stack are ineligible.  Among the others, we choose the one
   whose value has the least number of subsequent uses (ideally, and
   usually, 0) of the common value it already holds.  If commoni is >=
   0, it is the index in common[] of the value we are going to put in
   the allocated register, so we can update the various data structures
   appropriately.  */
int
allocate_register(struct local *s, int commoni)
{
    int i, reg, bestreg, nuses, bestregnuses, maxreg;

    bestreg = NO_REG;
#if USE_EXTRA_REGS
    maxreg = s->maxreg;
#else
    maxreg = NSCRATCHREGS;
#endif
    while (1) {
	bestregnuses = NOT_COMMON_VALUE;
	for (i = 0; i < maxreg; i++) {
	    reg = scratchregs[i];
	    if (s->regs[reg].stacktimes == 0) {
		nuses = (s->regs[reg].commoni == NOT_COMMON_VALUE) ?
			0 : s->common[s->regs[reg].commoni].nuses;
		if (nuses < bestregnuses) {
		    bestreg = reg;
		    bestregnuses = nuses;
		}
	    }
	}
	if (bestreg != NO_REG)
	    break;
#if USE_EXTRA_REGS
	if (maxreg == NSCRATCHREGS)
	    return NO_REG;
	s->maxreg = ++maxreg;
#else
	return NO_REG;
#endif
    }
    if (bestregnuses > 0)
	printf("net_filter_alloc: forced to reallocate r%d\n", bestreg);
	/* With USE_EXTRA_REGS, we could push up the number of registers
	   here to have one extra available for common values, but it's usually
	   not worth the overhead of the extra save-and-restore in the preamble.
	   Anyway, this never happens with typical filters.  */
    if (s->regs[bestreg].commoni != NOT_COMMON_VALUE)
	s->common[s->regs[bestreg].commoni].reg = NO_REG;
    if (commoni >= 0) {
	s->regs[bestreg].commoni = commoni;
	s->common[commoni].reg = bestreg;
    } else
	s->regs[bestreg].commoni = NOT_COMMON_VALUE;
    return bestreg;
}


#if USE_EXTRA_REGS
int
compile_preamble(int *instructions, struct local *s)
{
    int *instp;
    int len;
    int extra_regs, i, j, t, disp;

    extra_regs = s->maxreg - INITIAL_NSCRATCHREGS;
    if (extra_regs > 0) {
	len = extra_regs * 2 + 4;
	/* stw rp | (n-1) * stw | bl | stw | ldw rp | (n-1) * ldw | bv | ldw */
    } else
	return 0;
    if (instructions == NULL)
	return len;
    instp = instructions;
    /* Generate a wrapper function to save the callee-saves registers
       before invoking the filter code we have generated.  It would be
       marginally better to have the filter branch directly to the
       postamble code on return, but the difference is trivial and it
       is easier to have it always branch to (rp).  */
#define FRAME_SIZE 128	/* This is plenty without being excessive. */
    *instp++ = STW_NEG(REG_RTN, 20, REG_SP);		/* stw rp,-20(sp) */
    i = INITIAL_NSCRATCHREGS;
    t = STWM(scratchregs[i], FRAME_SIZE, REG_SP);	/* stwm r3,128(sp) */
    j = FRAME_SIZE;
    while (++i < s->maxreg) {
	*instp++ = t;
	j -= sizeof (int);
	t = STW_NEG(scratchregs[i], j, REG_SP);		/* stw r4,-124(sp) &c */
    }
    disp = extra_regs + 2;	/* n * ldw | bv | ldw rp */
    *instp++ = BL(disp, REG_RTN);			/* bl filter,rp */
    *instp++ = t;					/* stw in delay slot */
    *instp++ = LDW_NEG(FRAME_SIZE + 20, REG_SP, REG_RTN);
							/* ldw -148(sp),rp */
    while (--i > INITIAL_NSCRATCHREGS) {
	*instp++ = LDW_NEG(j, REG_SP, scratchregs[i]);	/* ldw -124(sp),r4 &c */
	j += sizeof (int);
    }
    *instp++ = BV(0, REG_RTN);				/* bv (rp) */
    *instp++ = LDWM_NEG(FRAME_SIZE, REG_SP, scratchregs[i]);
							/* ldwm -128(sp),r3
							   in delay slot */
    assert(instp - instructions == len);
    return len;
}
#endif	/* USE_EXTRA_REGS */

void
xassert(int cond)
{
    assert(cond);
}

struct exec_page_header {
    struct exec_page_header *next;
    char usedmap;
} *exec_pages;
#define WORD_ALIGN(x)	((x) & ~3)
#define BLOCKS_PER_PAGE 8	/* number of bits in usedmap. */
#define BLOCK_SIZE WORD_ALIGN((PAGE_SIZE - sizeof (struct exec_page_header)) / \
			      BLOCKS_PER_PAGE)

/* Simple allocator for executable memory.  At the two extremes of complexity,
   we have (simple) allocate one page for every executable block and (complex)
   duplicate or modify kalloc to deal separately with executable pages.  As a
   compromise, this allocator allocates in big blocks (about 500 bytes) with a
   `used' bitmap tracking the use of these blocks.  Currently, all allocations
   are smaller than this, and there are never enough of them at once to require
   more than one executable page.  */
void *
kmem_alloc_exec(vm_size_t size)
{
    struct exec_page_header *p;
    int nblocks, i, blockmask;
    vm_address_t v;
    kern_return_t kr;

    nblocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    assert(nblocks <= BLOCKS_PER_PAGE);
    blockmask = (1 << nblocks) - 1;
    while (1) {		/* Loop back at most once to add an extra page. */
	for (p = exec_pages; p != NULL; p = p->next) {
	    for (i = 0; i <= BLOCKS_PER_PAGE - nblocks; i++) {
		if (!(p->usedmap & (blockmask << i))) {
		    p->usedmap |= blockmask << i;
		    v = (vm_address_t) (p + 1) + i * BLOCK_SIZE;
		    ficache(HP700_SID_KERNEL, (vm_offset_t) v, size);
		    return (void *) v;
		}
	    }
	}
	kr = kmem_alloc_wired(kernel_map, &v, PAGE_SIZE);
	if (kr != KERN_SUCCESS)
	    return NULL;
	kr = vm_map_protect(kernel_map, v, v + PAGE_SIZE, VM_PROT_ALL, FALSE);
	if (kr != KERN_SUCCESS) {
	    kmem_free(kernel_map, v, PAGE_SIZE);
	    return NULL;
	}
	p = (struct exec_page_header *) v;
	p->next = exec_pages;
	exec_pages = p;
	p->usedmap = 0;
    }
}

void
net_filter_free(filter_fct_t fp, unsigned int len)
{
    struct exec_page_header *p, **linkp;
    int offset, blockno, nblocks, blockmask;

    nblocks = (len + BLOCK_SIZE - 1) / BLOCK_SIZE;
    p = (struct exec_page_header *) trunc_page(fp);
    offset = (vm_address_t) fp - (vm_address_t) (p + 1);
    assert(offset % BLOCK_SIZE == 0);
    blockno = offset / BLOCK_SIZE;
    blockmask = ((1 << nblocks) - 1) << blockno;
    assert((p->usedmap & blockmask) == blockmask);
    p->usedmap &= ~blockmask;
    if (p->usedmap == 0) {
	for (linkp = &exec_pages; (*linkp) != p; linkp = &(*linkp)->next)
	    assert(*linkp != NULL);
	*linkp = p->next;
	kmem_free(kernel_map, (vm_offset_t) p, PAGE_SIZE);
    }
}
