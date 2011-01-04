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
 * Eisa Bus handling
 */

#include <mach/boolean.h>
#include <kern/misc_protos.h>
#include <i386/pio.h>
#include <i386/AT386/eisa.h>
#include <i386/AT386/eisa_entries.h>

/* Forward */

extern boolean_t	is_eisa_board(
				int			board,
				eisa_board_id_t		*bid);

boolean_t is_eisa_bus = FALSE;

boolean_t
is_eisa_board(
	int			board,
	eisa_board_id_t		*bid)
{		
	register i;

	outb(EISA_ID_REG(board, EISA_ID_REG_0), 0xff);
	if(inb(EISA_ID_REG(board, EISA_ID_REG_0)) & 0x80)
		return(FALSE);

	for (i=0; i<4; i++)
		bid->byte[3-i] = inb(EISA_ID_REG(board, i));
	return(TRUE);
}
	
void
probe_eisa(void)
{
	eisa_board_id_t bid;

	/* probe system board */

	if (is_eisa_bus = is_eisa_board(EISA_SYSTEM_BOARD, &bid))
	  printf("Eisa system board: %c%c%c (0x%x) eisa bus version 0x%x\n",
		 bid.id.sys_id.name_char_0+0x40,
		 bid.id.sys_id.name_char_1+0x40,
		 bid.id.sys_id.name_char_2+0x40,
		 bid.id.sys_id.reserved,
		 bid.id.sys_id.bus_vers);
}
