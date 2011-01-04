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

#include <machine/asm.h>
#include <machine/psw.h>
#include <machine/spl.h>

	.space	$TEXT$
	.subspa $CODE$


/*
 *	void splx(level)
 *	void splon(level)
 */
	.export	splx,entry
	.export	splx_pc,entry
	.export	splon,entry
	.proc
	.callinfo
splon
splx
	rsm	PSW_I,r0
	mtctl	arg0,eiem
	comiclr,= SPLHIGH,arg0,0
	ssm	PSW_I,r0
splx_pc
	bv	(rp)
	nop
	.procend

/*
 *	void spllo(void)
 */
	.export	spllo,entry
	.proc
	.callinfo
spllo
	ldil	L%SPL0,t1
	ldo	R%SPL0(t1),t1
	mtctl	t1,eiem
	bv	(rp)
	ssm	PSW_I,r0
	.procend

/*
 *	spl_t splhigh(void)
 *	spl_t splsched(void)
 *	spl_t sploff(void)
 */
	.export	splhigh,entry
	.export	splsched,entry
	.export	sploff,entry

	.proc
	.callinfo
sploff
splhigh
splsched
	rsm	PSW_I,r0
	mfctl	eiem,ret0
	ldi	SPLHIGH,t1
	bv	(rp)
	mtctl	t1,eiem
	.procend

/*
 *	spl_t splclock(void)
 */
	.export	splclock,entry
	.proc
	.callinfo
splclock
	mfctl	eiem,ret0
	mtctl	r0,eiem		/* set spl */
	bv	(rp)
	ssm	PSW_I,r0
	.procend

/*
 *	spl_t splvm(void)
 */
	.export	splvm,entry
	.proc
	.callinfo
splvm
	mfctl	eiem,ret0
	ldil	L%SPLVM,t1
	ldo	R%SPLVM(t1),t1
	mtctl	t1,eiem
	bv	(rp)
	ssm	PSW_I,r0
	.procend

/*
 *	spl_t splcio(void)
 */
	.export	splcio,entry
	.proc
	.callinfo
splcio
	mfctl	eiem,ret0
	ldil	L%SPLCIO,t1
	ldo	R%SPLCIO(t1),t1
	mtctl	t1,eiem
	bv	(rp)
	ssm	PSW_I,r0
	.procend

/*
 *	spl_t splhpib(void)
 */
	.export	splhpib,entry
	.proc
	.callinfo
splhpib
	mfctl	eiem,ret0
	ldil	L%SPLCIOHPIB,t1
	ldo	R%SPLCIOHPIB(t1),t1
	mtctl	t1,eiem
	bv	(rp)
	ssm	PSW_I,r0
	.procend

/*
 *	spl_t splbio(void)
 */
	.export	splbio,entry
	.proc
	.callinfo
splbio
	mfctl	eiem,ret0
	ldil	L%SPLBIO,t1
	ldo	R%SPLBIO(t1),t1
	mtctl	t1,eiem
	bv	(rp)
	ssm	PSW_I,r0
	.procend

/*
 *	spl_t splimp(void)
 */
	.export	splimp,entry
	.proc
	.callinfo
splimp
	mfctl	eiem,ret0
	ldil	L%SPLIMP,t1
	ldo	R%SPLIMP(t1),t1
	mtctl	t1,eiem
	bv	(rp)
	ssm	PSW_I,r0
	.procend

/*
 *	spl_t spllan(void)
 */
	.export	spllan,entry
	.proc
	.callinfo
spllan
	mfctl	eiem,ret0
	ldil	L%SPLLAN,t1
	ldo	R%SPLLAN(t1),t1
	mtctl	t1,eiem
	bv	(rp)
	ssm	PSW_I,r0
	.procend

/*
 *	spl_t spltty(void)
 */
	.export	spltty,entry
	.proc
	.callinfo
spltty
	mfctl	eiem,ret0
	ldil	L%SPLTTY,t1
	ldo	R%SPLTTY(t1),t1
	mtctl	t1,eiem
	bv	(rp)
	ssm	PSW_I,r0
	.procend

/*
 *	spl_t splnet(void)
 */
	.export	spltty,entry
	.proc
	.callinfo
splnet
	mfctl	eiem,ret0
	ldil	L%SPLNET,t1
	ldo	R%SPLNET(t1),t1
	mtctl	t1,eiem
	bv	(rp)
	ssm	PSW_I,r0
	.procend

/*
 *	spl_t splsoftclock(void)
 */
	.export	splsoftclock,entry
	.proc
	.callinfo
splsoftclock
	mfctl	eiem,ret0
	ldil	L%SPLSCLK,t1
	ldo	R%SPLSCLK(t1),t1
	mtctl	t1,eiem
	bv	(rp)
	ssm	PSW_I,r0
	.procend

/*
 *	spl_t splhil(void)
 */
	.export	splhil,entry
	.proc
	.callinfo
splhil
	mfctl	eiem,ret0
	ldil	L%SPLHIL,t1
	ldo	R%SPLHIL(t1),t1
	mtctl	t1,eiem
	bv	(rp)
	ssm	PSW_I,r0
	.procend

/*
 *	spl_t splgkd(void)
 */
	.export	splgkd,entry
	.proc
	.callinfo
splgkd
	mfctl	eiem,ret0
	ldil	L%SPLGKD,t1
	ldo	R%SPLGKD(t1),t1
	mtctl	t1,eiem
	bv	(rp)
	ssm	PSW_I,r0
	.procend

/*
 *	spl_t splasp(void)
 */
	.export	splasp,entry
	.proc
	.callinfo
splasp
	mfctl	eiem,ret0
	ldil	L%SPLASP,t1
	ldo	R%SPLASP(t1),t1
	mtctl	t1,eiem
	bv	(rp)
	ssm	PSW_I,r0
	.procend

/*
 *	spl_t splwax(void)
 */
	.export	splwax,entry
	.proc
	.callinfo
splwax
	mfctl	eiem,ret0
	ldil	L%SPLWAX,t1
	ldo	R%SPLWAX(t1),t1
	mtctl	t1,eiem
	bv	(rp)
	ssm	PSW_I,r0
	.procend

	.end

/*
 *	spl_t get_spl(void)
 */
	.export	get_spl,entry
	.proc
	.callinfo
get_spl
	mfctl	eiem,ret0
	bv	(rp)
	nop
	.procend
