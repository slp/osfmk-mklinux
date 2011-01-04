/* Interface with ptrace and core-dumping routines */


#include "fpu_system.h"
#include "exception.h"
#include "reg_constant.h"
#include "fpu_emu.h"
#include "control_w.h"
#include "status_w.h"


#define EXTENDED_Ebias 0x3fff
#define EXTENDED_Emin (-0x3ffe)  /* smallest valid exponent */

#define DOUBLE_Emax 1023         /* largest valid exponent */
#define DOUBLE_Ebias 1023
#define DOUBLE_Emin (-1022)      /* smallest valid exponent */

#define SINGLE_Emax 127          /* largest valid exponent */
#define SINGLE_Ebias 127
#define SINGLE_Emin (-126)       /* smallest valid exponent */


/* Copy and paste from round_to_int. Original comments maintained */
/*===========================================================================*/

/* r gets mangled such that sig is int, sign: 
   it is NOT normalized */
/* The return value (in eax) is zero if the result is exact,
   if bits are changed due to rounding, truncation, etc, then
   a non-zero value is returned */
/* Overflow is signalled by a non-zero return value (in eax).
   In the case of overflow, the returned significand always has the
   largest possible value */

static int round_to_int_cwd(FPU_REG *r, long int user_control_word)
{
  char     very_big;
  unsigned eax;

  if (r->tag == TW_Zero)
    {
      /* Make sure that zero is returned */
      significand(r) = 0;
      return 0;        /* o.k. */
    }
  
  if (r->exp > EXP_BIAS + 63)
    {
      r->sigl = r->sigh = ~0;      /* The largest representable number */
      return 1;        /* overflow */
    }

  eax = shrxs(&r->sigl, EXP_BIAS + 63 - r->exp);
  very_big = !(~(r->sigh) | ~(r->sigl));  /* test for 0xfff...fff */
#define	half_or_more	(eax & 0x80000000)
#define	frac_part	(eax)
#define more_than_half  ((eax & 0x80000001) == 0x80000001)
  switch (user_control_word & CW_RC)
    {
    case RC_RND:
      if ( more_than_half               	/* nearest */
	  || (half_or_more && (r->sigl & 1)) )	/* odd -> even */
	{
	  if ( very_big ) return 1;        /* overflow */
	  significand(r) ++;
	  return PRECISION_LOST_UP;
	}
      break;
    case RC_DOWN:
      if (frac_part && r->sign)
	{
	  if ( very_big ) return 1;        /* overflow */
	  significand(r) ++;
	  return PRECISION_LOST_UP;
	}
      break;
    case RC_UP:
      if (frac_part && !r->sign)
	{
	  if ( very_big ) return 1;        /* overflow */
	  significand(r) ++;
	  return PRECISION_LOST_UP;
	}
      break;
    case RC_CHOP:
      break;
    }

  return eax ? PRECISION_LOST_DOWN : 0;

}



/* Conver a number in the emulator format to the
 * hardware format.
 * Taken from the emulator sources, function reg_load_extended
 */

/* Get a long double from the debugger */
void hardreg_to_softreg(const char hardreg[10],
			      FPU_REG *soft_reg)

{
	unsigned long sigl, sigh, exp;
		
	sigl = *((unsigned long *) hardreg);
	sigh = *(1 + (unsigned long *) hardreg);
	exp = *(4 + (unsigned short *) hardreg);
	
	soft_reg->tag = TW_Valid;   /* Default */
	soft_reg->sigl = sigl;
	soft_reg->sigh = sigh;
	if (exp & 0x8000)
		soft_reg->sign = SIGN_NEG;
	else
		soft_reg->sign = SIGN_POS;
	exp &= 0x7fff;
	soft_reg->exp = exp - EXTENDED_Ebias + EXP_BIAS;
	
	if ( exp == 0 )
	{
		if ( !(sigh | sigl) )
		{
			soft_reg->tag = TW_Zero;
			return;
		}
		/* The number is a de-normal or pseudodenormal. */
		if (sigh & 0x80000000)
		{
			/* Is a pseudodenormal. */
			/* Convert it for internal use. */
			/* This is non-80486 behaviour because the number
			   loses its 'denormal' identity. */
			soft_reg->exp++;
			return;
		}
		else
		{
			/* Is a denormal. */
			/* Convert it for internal use. */
			soft_reg->exp++;
			normalize_nuo(soft_reg);
			return;
		}
	}
	else if ( exp == 0x7fff )
	{
		if ( !((sigh ^ 0x80000000) | sigl) )
		{
			/* Matches the bit pattern for Infinity. */
			soft_reg->exp = EXP_Infinity;
			soft_reg->tag = TW_Infinity;
			return;
		}
		
		soft_reg->exp = EXP_NaN;
		soft_reg->tag = TW_NaN;
		if ( !(sigh & 0x80000000) )
		{
			/* NaNs have the ms bit set to 1. */
			/* This is therefore an Unsupported NaN data type. */
			/* This is non 80486 behaviour */
			/* This should generate an Invalid Operand exception
			   later, so we convert it to a SNaN */
			soft_reg->sigh = 0x80000000;
			soft_reg->sigl = 0x00000001;
			soft_reg->sign = SIGN_NEG;
			return;
		}
		return;
	}
	
	if ( !(sigh & 0x80000000) )
	{
		/* Unsupported data type. */
		/* Valid numbers have the ms bit set to 1. */
		/* Unnormal. */
		/* Convert it for internal use. */
		/* This is non-80486 behaviour */
		/* This should generate an Invalid Operand exception
		   later, so we convert it to a SNaN */
		soft_reg->sigh = 0x80000000;
		soft_reg->sigl = 0x00000001;
		soft_reg->sign = SIGN_NEG;
		soft_reg->exp = EXP_NaN;
		soft_reg->tag = TW_NaN;
		return;
	}
	return;
}

/* Conver a number in the emulator format to the
 * hardware format.
 * Adapted from function write_to_extended
 */


void softreg_to_hardreg(const FPU_REG *rp, char d[10], long int user_control_word)
{
	long e;
	FPU_REG tmp;
	e = rp->exp - EXP_BIAS + EXTENDED_Ebias;

	/*
	  All numbers except denormals are stored internally in a
	  format which is compatible with the extended real number
	  format.
	  */
	if (e > 0) {
		*(unsigned long *) d = rp->sigl;
		*(unsigned long *) (d + 4) = rp->sigh;
	} else {
		/*
		  The number is a de-normal stored as a normal using our
		  extra exponent range, or is Zero.
		  Convert it back to a de-normal, or leave it as Zero.
		  */
		reg_move(rp, &tmp);
		tmp.exp += -EXTENDED_Emin + 63;  /* largest exp to be 63 */
		round_to_int_cwd(&tmp, user_control_word);
		e = 0;
		*(unsigned long *) d= tmp.sigl;
		*(unsigned long *) (d + 4) = tmp.sigh;
	}
	e |= rp->sign == SIGN_POS ? 0 : 0x8000;
	*(unsigned short *) (d + 8) = e;
}

