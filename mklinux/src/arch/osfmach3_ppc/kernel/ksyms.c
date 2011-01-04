/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#include <linux/module.h>
#include <linux/smp.h>

#ifdef	CONFIG_OSFMACH3
#include <linux/string.h>
#include <asm/segment.h>
#endif	/* CONFIG_OSFMACH3 */

extern void put_user_byte(char, char *);
extern unsigned long __mem_base;

/* For Linux/PPC module compatability */
extern void _enable_interrupts(void);
extern void _disable_interrupts(void);
extern void __save_flags(void);
extern void __restore_flags(void);
extern void cli(void);
extern void sti(void);
extern int bad_user_access_length(void);
//conflicting types w/ asm/bitops.h
//extern unsigned long set_bit(void);
//extern unsigned long clear_bit(void);
//extern unsigned long change_bit(void);
extern void *__get_instruction_pointer(void); 
extern void dump_thread(void);
extern void __ashrdi3(void);

/* These need to go below that or it breaks */
#include <linux/locks.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ppp.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <asm/unistd.h>
#include <asm/checksum.h>

static struct symbol_table arch_symbol_table = {
#include <linux/symtab_begin.h>
	/* platform dependent support */
	X(put_user_byte),
#ifdef	CONFIG_OSFMACH3
/* start my tinkering */
	X(bmap),
	X(getblk),
	X(ll_rw_block),
	X(__wait_on_buffer),
	X(__brelse),
	X(mark_buffer_uptodate),
	X(set_bit),
	X(set_writetime),
	X(refile_buffer),
	X(invalidate_inode_pages),
	X(blkdev_open),
	X(blkdev_release),
	X(iput),
	X(block_fsync),
	X(block_read),
	X(block_write),
	X(register_blkdev),
	X(inb),
	X(outb),
	X(udelay),
	X(ether_setup),
	X(set_bit),
	X(kfree_skb),
	X(dev_kfree_skb),
	X(dev_alloc_skb),
	X(__get_instruction_pointer),
	X(eth_type_trans),
	X(netif_rx),
	X(dev_tint),
	X(register_netdev),
	X(unregister_netdev),
	X(strncpy),
	X(strncmp),
	X(strnlen),
	X(strncat),
	X(strcpy),
	X(strstr),
	X(strtok),
	X(strchr),
	X(strcmp),
	X(strlen),
	X(strcat),
	X(strrchr),
	X(atomic_add),
	X(atomic_sub),
	X(atomic_inc_return),
	X(atomic_dec_return),
	X(find_first_zero_bit),
	X(find_next_zero_bit),
	X(__ashrdi3),
	X(clear_bit),
	X(ffz),
	X(dump_thread),
	X(inode_pager_uncache),
	X(expand_stack),
	X(force_sig),
	X(start_thread),
	X(bad_user_access_length),
	X(kernel_thread),
	X(ip_fast_csum),
	X(xchg_u32),
	X(current_set),
	X(invalidate_inode_pages),
	X(register_blkdev),
/* end of my tinkering */
	X(memcmp),
	X(memmove),
	X(memcpy),
	X(memset),
	X(memscan),
	X(iput),
#endif	/* CONFIG OSFMACH3 */
	X(__mem_base),
	X(_enable_interrupts),
	X(_disable_interrupts),
	X(__save_flags),
	X(__restore_flags),
	X(__put_user),
	X(__get_user),
	X(cli),
	X(sti),
	X(get_ds),
	X(set_fs),
	X(set_bit),
	X(clear_bit),
	X(change_bit),
#include <linux/symtab_end.h>
};

void arch_syms_export(void)
{
	register_symtab(&arch_symbol_table);
}
