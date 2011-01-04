/*
 *  linux/arch/ppc/kernel/alignment.c
 *
 *  Copyright (C) 1995  Gary Thomas
 *  Adapted for PowerPC by Gary Thomas
 */

/*
 * This file handles the 'alignment' exception.  This exception is
 * generated [mostly] when a floating point operation crosses a 4K
 * boundary and can be recovered/emulated in software.
 *
 * Other alignment exceptions are considered to be errors.
 */

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/ldt.h>
#include <linux/user.h>
#include <linux/a.out.h>

#include <asm/pgtable.h>
#include <asm/segment.h>
#include <asm/system.h>
#include <asm/io.h>

#include "ppc_machine.h"

/*
 * The DSISR register tells us everything about the instruction (without
 * having to actually examine it!)
 *
 * Note: This handler leaves most cases undone.  The important ones
 * [which *are* handled] come about because GCC likes to use lfd/stfd
 * to do high-speed copies.
 */

struct _dsisr
{
	unsigned long :12;    /* Unused */
	unsigned long :2;     /* Used for 64 bit implementations */
	unsigned long :1;     /* Unused */
	unsigned long op:7;   /* Encoded version of instruction opcode */
	unsigned long rs:5;   /* Source/destination register */
	unsigned long ra:5;   /* General register operand (rA) */
};

unsigned long
_get_long(unsigned long *mem)
{
  unsigned char *cp;
  if ((int)mem & 0x03) {
    /* Address is truly unaligned */
    cp = (unsigned char *)mem;
    return ((cp[0]<<24)|(cp[1]<<16)|(cp[2]<<8)|cp[3]);
  } else {
    return (*mem);
  }
}

void
_put_long(unsigned long *mem, unsigned long val)
{
  unsigned char *sp, *dp;
  if ((int)mem & 0x03) {
    /* Address is truly unaligned */
    sp = (unsigned char *)&val;
    dp = (unsigned char *)mem;
    *dp++ = *sp++;  /* Copy value */
    *dp++ = *sp++;
    *dp++ = *sp++;
    *dp++ = *sp++;
  } else {
    *mem = val;
  }
}

unsigned long
_get_short(unsigned short *mem)
{
  unsigned char *cp;
  if ((int)mem & 0x03) {
    /* Address is truly unaligned */
    cp = (unsigned char *)mem;
    return ((cp[0]<<8)|cp[1]);
  } else {
    return (*mem);
  }
}

void
_put_short(unsigned short *mem, unsigned short val)
{
  unsigned char *sp, *dp;
  if ((int)mem & 0x03) {
    /* Address is truly unaligned */
    sp = (unsigned char *)&val;
    dp = (unsigned char *)mem;
printk("STH - Val: %x/%x.%x\n", val, sp[0], sp[1]);
    *dp++ = *sp++;  /* Copy value */
    *dp++ = *sp++;
  } else {
    *mem = val;
  }
}

AlignmentException(struct pt_regs *regs)
{
	struct _dsisr *ds = (struct _dsisr *)&regs->dsisr;
#ifdef DO_FLOAT_CONVERSIONS
	float fp_val;
#endif
	unsigned long *lp, *mem;
	mem = regs->dar;
	if ((ds->op >= 0x08) && (ds->op <= 0x0F)) {
	  if (ds->rs < 4) {
	    lp = (unsigned long *)&regs->fpr[ds->rs];
	  } else {
	    lp = (unsigned long *)&current->tss.fpr[ds->rs];
	  }
	} else {
	  lp = &regs->gpr[ds->rs];
	}
	switch (ds->op)
	  {
	  case 0x00: /* lwz rs,<ea> */
	  case 0x60: /* lwzx rs,<ea> */
	    *lp = _get_long(mem);
	    break;
	  case 0x02: /* stw rs,<ea> */
	  case 0x62: /* stwx rs,<ea> */
	    _put_long(mem, *lp);
	    break;
	  case 0x04: /* lhz rs,<ea> */
	    *lp = _get_short((unsigned short *)mem);
	    break;
	  case 0x05: /* lha rs,<ea> */
	    *lp = _get_short((unsigned short *)mem);
	    if (*lp & 0x8000) *lp |= 0xFFFF0000;  /* Sign extend */
	    break;
	  case 0x06: /* sth rs,<ea> */
	    _put_short((unsigned short *)mem, *lp);
	    break;
#ifdef DO_FLOAT_CONVERSIONS
	  case 0x08: /* lfs rs,<ea> */
	    *(unsigned long *)&fp_val = _get_long(mem++);
	    *lp++ = fp_val;  /* I hope it doesn't fail! */
	    break;
#endif
	  case 0x09: /* lfd rs,<ea> */
	    *lp++ = _get_long(mem++);
	    *lp++ = _get_long(mem++);
	    break;
#ifdef DO_FLOAT_CONVERSIONS
	  case 0x0A: /* stfs rs,<ea> */
	    fp_val = *(double *)lp;
	    _put_long(mem++, (unsigned long *)&fp_val);
	    break;
#endif
	  case 0x0B: /* stfd rs,<ea> */
	    _put_long(mem++, *lp++);
	    _put_long(mem++, *lp++);
	    break;
	  case 0x10: /* lwzu rs,<ea> */
	    *lp = _get_long(mem);
	    regs->gpr[ds->ra] = (unsigned long)mem;
	    break;
	  case 0x19: /* lfdu rs,<ea> */
	    *lp++ = _get_long(mem++);
	    *lp++ = _get_long(mem++);
	    regs->gpr[ds->ra] = (unsigned long)mem;
	    break;
	  case 0x1B: /* stfdu rs,<ea> */
	    _put_long(mem++, *lp++);
	    _put_long(mem++, *lp++);
	    regs->gpr[ds->ra] = (unsigned long)mem;
	    break;
	  default:
	    printk("Alignment error at PC: %x, SR: %x, DSISR: %x, DAR: %x\n", regs->nip, regs->msr, regs->dsisr, regs->dar);
	    printk("Op: %x, Rd: %d, Ra: %d\n", ds->op, ds->rs, ds->ra);
	    _exception(SIGBUS, regs);	
	    return;
	  };
	regs->nip += 4;  /* Advance over failed instruction */
}

