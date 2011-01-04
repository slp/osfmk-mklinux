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

#include <platforms.h>

#include <ppc/proc_reg.h> /* For isync */
#include <mach_debug.h>
#include <kern/assert.h>
#include <kern/cpu_number.h>
#include <kern/spl.h>
#include <mach/std_types.h>
#include <types.h>
#include <chips/busses.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/powermac_pci.h>
#include <ppc/io_map_entries.h>
#include <ppc/POWERMAC/dbdma.h>


static int	dbdma_alloc_index = 0;
dbdma_command_t	*dbdma_alloc_commands = NULL;

void
dbdma_start(dbdma_regmap_t *dmap, dbdma_command_t *commands)
{
	unsigned long addr = kvtophys((vm_offset_t) commands);

	if (addr & 0xf)
		panic("dbdma_start command structure not 16-byte aligned");

	dmap->d_intselect = 0xff;	/* Endian magic - clear out interrupts */
	DBDMA_ST4_ENDIAN(&dmap->d_control, 
			 DBDMA_CLEAR_CNTRL( (DBDMA_CNTRL_ACTIVE	|
					     DBDMA_CNTRL_DEAD	|
					     DBDMA_CNTRL_WAKE	|
					     DBDMA_CNTRL_FLUSH	|
					     DBDMA_CNTRL_PAUSE	|
					     DBDMA_CNTRL_RUN      )));      
	eieio();
     
	while (DBDMA_LD4_ENDIAN(&dmap->d_status) & DBDMA_CNTRL_ACTIVE)
		eieio();

	dmap->d_cmdptrhi = 0;	eieio();/* 64-bit not yet */
	DBDMA_ST4_ENDIAN(&dmap->d_cmdptrlo, addr); eieio();

	DBDMA_ST4_ENDIAN(&dmap->d_control, DBDMA_SET_CNTRL(DBDMA_CNTRL_RUN));
	eieio();

}

void
dbdma_stop(dbdma_regmap_t *dmap)
{
	DBDMA_ST4_ENDIAN(&dmap->d_control, DBDMA_CLEAR_CNTRL(DBDMA_CNTRL_RUN) |
			  DBDMA_SET_CNTRL(DBDMA_CNTRL_FLUSH)); eieio();

	while (DBDMA_LD4_ENDIAN(&dmap->d_status) & (DBDMA_CNTRL_ACTIVE|DBDMA_CNTRL_FLUSH))
		eieio();
}

void
dbdma_flush(dbdma_regmap_t *dmap)
{
	DBDMA_ST4_ENDIAN(&dmap->d_control,DBDMA_SET_CNTRL(DBDMA_CNTRL_FLUSH));
	eieio();

	while (DBDMA_LD4_ENDIAN(&dmap->d_status) & (DBDMA_CNTRL_FLUSH))
		eieio();
}

void
dbdma_reset(dbdma_regmap_t *dmap)
{
	DBDMA_ST4_ENDIAN(&dmap->d_control, 
			 DBDMA_CLEAR_CNTRL( (DBDMA_CNTRL_ACTIVE	|
					     DBDMA_CNTRL_DEAD	|
					     DBDMA_CNTRL_WAKE	|
					     DBDMA_CNTRL_FLUSH	|
					     DBDMA_CNTRL_PAUSE	|
					     DBDMA_CNTRL_RUN      )));      
	eieio();

	while (DBDMA_LD4_ENDIAN(&dmap->d_status) & DBDMA_CNTRL_RUN)
		eieio();
}

void
dbdma_continue(dbdma_regmap_t *dmap)
{
	DBDMA_ST4_ENDIAN(&dmap->d_control, DBDMA_SET_CNTRL(DBDMA_CNTRL_RUN|DBDMA_CNTRL_WAKE) | DBDMA_CLEAR_CNTRL(DBDMA_CNTRL_PAUSE|DBDMA_CNTRL_DEAD));
	eieio();
}

void
dbdma_pause(dbdma_regmap_t *dmap)
{
	DBDMA_ST4_ENDIAN(&dmap->d_control,DBDMA_SET_CNTRL(DBDMA_CNTRL_PAUSE));
	eieio();

	while (DBDMA_LD4_ENDIAN(&dmap->d_status) & DBDMA_CNTRL_ACTIVE)
		eieio();
}

dbdma_command_t	*
dbdma_alloc(int count)
{
	dbdma_command_t	*dbdmap;

	/*
	 * For now, we assume that dbdma_alloc() is called only when
	 * the system is bootstrapping, i.e. before the other CPUs
	 * are activated...
	 * If that's not the case, we need to protect the global
	 * variables here.
	 */
	assert(cpu_number() == master_cpu);

	if (dbdma_alloc_index == 0) 
		dbdma_alloc_commands = (dbdma_command_t *) io_map(0, PAGE_SIZE);

	if ((dbdma_alloc_index+count) >= PAGE_SIZE / sizeof(dbdma_command_t)) 
		panic("Too many dbdma command structures!");

	dbdmap = &dbdma_alloc_commands[dbdma_alloc_index];
	dbdma_alloc_index += count;
	return	dbdmap;
}
