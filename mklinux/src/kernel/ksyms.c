/* 
 * Herein lies all the functions/variables that are "exported" for linkage
 * with dynamically loaded kernel modules.
 *			Jon.
 *
 * - Stacked module support and unified symbol table added (June 1994)
 * - External symbol table support added (December 1994)
 * - Versions on symbols added (December 1994)
 * by Bjorn Ekwall <bj0rn@blox.se>
 */

#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/smp.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/sched.h>
#include <linux/kernel_stat.h>
#include <linux/mm.h>
#include <linux/malloc.h>
#include <linux/ptrace.h>
#include <linux/sys.h>
#include <linux/utsname.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/timer.h>
#include <linux/binfmts.h>
#include <linux/personality.h>
#include <linux/termios.h>
#include <linux/tqueue.h>
#include <linux/tty.h>
#include <linux/serial.h>
#include <linux/locks.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/sem.h>
#include <linux/minix_fs.h>
#include <linux/ext2_fs.h>
#include <linux/random.h>
#include <linux/mount.h>
#include <linux/pagemap.h>
#include <linux/sysctl.h>
#include <linux/hdreg.h>
#include <linux/skbuff.h>
#include <linux/genhd.h>
#include <linux/swap.h>
#include <linux/ctype.h>
#include <linux/file.h>

#ifndef	CONFIG_PMAC_KEYBOARD
extern unsigned char aux_device_present, kbd_read_mask;
#endif	/* CONFIG_PMAC_KEYBOARD */

#ifdef CONFIG_PCI
#include <linux/bios32.h>
#include <linux/pci.h>
#endif
#if defined(CONFIG_PROC_FS)
#include <linux/proc_fs.h>
#endif
#ifdef CONFIG_KERNELD
#include <linux/kerneld.h>
#endif
#include <asm/irq.h>
#ifdef __SMP__
#include <linux/smp.h>
#endif

extern char *get_options(char *str, int *ints);
extern void set_device_ro(kdev_t dev,int flag);
extern struct file_operations * get_blkfops(unsigned int);
extern void blkdev_release(struct inode * inode);

extern void *sys_call_table;

extern struct timezone sys_tz;
extern int request_dma(unsigned int dmanr, char * deviceID);
extern void free_dma(unsigned int dmanr);

extern void hard_reset_now(void);

struct symbol_table symbol_table = {
#include <linux/symtab_begin.h>
#ifdef MODVERSIONS
	{ (void *)1 /* Version version :-) */,
		SYMBOL_NAME_STR (Using_Versions) },
#endif

	/* stackable module support */
	X(register_symtab_from),
#ifdef CONFIG_KERNELD
	X(kerneld_send),
#endif
	X(get_options),

	/* system info variables */
	/* These check that they aren't defines (0/1) */
#ifndef EISA_bus__is_a_macro
	X(EISA_bus),
#endif
#ifndef MCA_bus__is_a_macro
	X(MCA_bus),
#endif
#ifndef wp_works_ok__is_a_macro
	X(wp_works_ok),
#endif

#ifdef CONFIG_PCI
	/* PCI BIOS support */
	X(pcibios_present),
	X(pcibios_find_class),
	X(pcibios_find_device),
	X(pcibios_read_config_byte),
	X(pcibios_read_config_word),
	X(pcibios_read_config_dword),
    	X(pcibios_strerror),
	X(pcibios_write_config_byte),
	X(pcibios_write_config_word),
	X(pcibios_write_config_dword),
#endif

	/* process memory management */
	X(verify_area),
	X(do_mmap),
	X(do_munmap),
	X(exit_mm),

	/* internal kernel memory management */
	X(__get_free_pages),
	X(free_pages),
	X(kmalloc),
	X(kfree),
	X(vmalloc),
#ifndef	CONFIG_OSFMACH3
	X(vremap),
#endif	/* CONFIG_OSFMACH3 */
	X(vfree),
 	X(mem_map),
#ifndef	CONFIG_OSFMACH3
 	X(remap_page_range),
#endif	/* CONFIG_OSFMACH3 */
	X(high_memory),
	X(update_vm_cache),

	/* filesystem internal functions */
	X(getname),
	X(putname),
	X(__iget),
	X(iput),
	X(namei),
	X(lnamei),
	X(open_namei),
	X(sys_close),
	X(close_fp),
	X(check_disk_change),
	X(invalidate_buffers),
	X(invalidate_inodes),
	X(invalidate_inode_pages),
	X(fsync_dev),
	X(permission),
	X(inode_setattr),
	X(inode_change_ok),
	X(set_blocksize),
	X(getblk),
	X(bread),
	X(breada),
	X(__brelse),
	X(__bforget),
	X(ll_rw_block),
	X(__wait_on_buffer),
	X(mark_buffer_uptodate),
	X(unlock_buffer),
	X(dcache_lookup),
	X(dcache_add),
	X(add_blkdev_randomness),
	X(generic_file_read),
	X(generic_file_mmap),
	X(generic_readpage),
	X(__fput),
	X(make_bad_inode),

	/* device registration */
	X(register_chrdev),
	X(unregister_chrdev),
	X(register_blkdev),
	X(unregister_blkdev),
	X(tty_register_driver),
	X(tty_unregister_driver),
	X(tty_std_termios),

	/* block device driver support */
	X(block_read),
	X(block_write),
	X(block_fsync),
#ifndef	CONFIG_OSFMACH3
	X(wait_for_request),
#endif	/* CONFIG_OSFMACH3 */
	X(blksize_size),
	X(hardsect_size),
	X(blk_size),
#ifndef	CONFIG_OSFMACH3
	X(blk_dev),
#endif	/* CONFIG_OSFMACH3 */
	X(is_read_only),
	X(set_device_ro),
	X(bmap),
	X(sync_dev),
	X(get_blkfops),
	X(blkdev_open),
	X(blkdev_release),
#ifndef	CONFIG_OSFMACH3
	X(gendisk_head),
	X(resetup_one_dev),
#endif	/* CONFIG_OSFMACH3 */

#ifdef CONFIG_SERIAL	
	/* Module creation of serial units */
	X(register_serial),
	X(unregister_serial),
#endif
	/* tty routines */
	X(tty_hangup),
	X(tty_wait_until_sent),
	X(tty_check_change),
	X(tty_hung_up_p),
	X(do_SAK),
	X(console_print),

	/* filesystem registration */
	X(register_filesystem),
	X(unregister_filesystem),

	/* executable format registration */
	X(register_binfmt),
	X(unregister_binfmt),
	X(search_binary_handler),
	X(prepare_binprm),
	X(remove_arg_zero),

	/* execution environment registration */
	X(lookup_exec_domain),
	X(register_exec_domain),
	X(unregister_exec_domain),

	/* sysctl table registration */
	X(register_sysctl_table),
	X(unregister_sysctl_table),

#ifndef	CONFIG_OSFMACH3
	/* interrupt handling */
	X(request_irq),
	X(free_irq),
	X(enable_irq),
	X(disable_irq),
	X(probe_irq_on),
	X(probe_irq_off),
#endif	/* CONFIG_OSFMACH3 */
	X(bh_active),
	X(bh_mask),
	X(bh_mask_count),
	X(bh_base),
	X(add_timer),
	X(del_timer),
	X(tq_timer),
	X(tq_immediate),
	X(tq_scheduler),
	X(timer_active),
	X(timer_table),
 	X(intr_count),

#ifndef	CONFIG_OSFMACH3
	/* autoirq from  drivers/net/auto_irq.c */
	X(autoirq_setup),
	X(autoirq_report),
#endif	/* CONFIG_OSFMACH3 */

#ifndef	CONFIG_OSFMACH3
	/* dma handling */
	X(request_dma),
	X(free_dma),
#ifdef HAVE_DISABLE_HLT
	X(disable_hlt),
	X(enable_hlt),
#endif

	/* IO port handling */
	X(check_region),
	X(request_region),
	X(release_region),
#endif	/* CONFIG_OSFMACH3 */

	/* process management */
	X(wake_up),
	X(wake_up_interruptible),
	X(sleep_on),
	X(interruptible_sleep_on),
	X(schedule),
	X(current_set),
	X(jiffies),
	X(xtime),
	X(do_gettimeofday),
	X(loops_per_sec),
	X(need_resched),
	X(kstat),
	X(kill_proc),
	X(kill_pg),
	X(kill_sl),

	/* misc */
	X(panic),
	X(printk),
	X(sprintf),
	X(vsprintf),
	X(kdevname),
	X(simple_strtoul),
	X(system_utsname),
	X(sys_call_table),
	X(hard_reset_now),
	X(_ctype),
	X(get_random_bytes),

	/* Signal interfaces */
	X(send_sig),

	/* Program loader interfaces */
	X(setup_arg_pages),
	X(copy_strings),
	X(do_execve),
	X(flush_old_exec),
	X(open_inode),
	X(read_exec),

	/* Miscellaneous access points */
	X(si_meminfo),

	/* Added to make file system as module */
	X(set_writetime),
	X(sys_tz),
	X(__wait_on_super),
	X(file_fsync),
	X(clear_inode),
	X(refile_buffer),
	X(nr_async_pages),
	X(___strtok),
	X(init_fifo),
	X(super_blocks),
	X(fifo_inode_operations),
	X(chrdev_inode_operations),
	X(blkdev_inode_operations),
	X(read_ahead),
	X(get_hash_table),
	X(get_empty_inode),
	X(insert_inode_hash),
	X(event),
	X(__down),
	X(__up),
	X(securelevel),
/* all busmice */
	X(add_mouse_randomness),
	X(fasync_helper),
/* psaux mouse */
#ifndef	CONFIG_PMAC_KEYBOARD
#ifndef	CONFIG_OSFMACH3
	X(aux_device_present),
#endif	/* CONFIG_OSFMACH3 */
	X(kbd_read_mask),
#endif	/* CONFIG_PMAC_KEYBOARD */

#ifdef CONFIG_BLK_DEV_IDE_PCMCIA
	X(ide_register),
	X(ide_unregister),
#endif

#ifdef CONFIG_BLK_DEV_MD
	X(disk_name),	/* for md.c */
#endif
 	
	/* binfmt_aout */
	X(get_write_access),
	X(put_write_access),

#ifdef	CONFIG_OSFMACH3
	X(get_fs),
	X(copyout),
	X(copyin),
	X(memcpy_fromfs),
	X(memcpy_tofs),
#endif	/* CONFIG_OSFMACH3 */

	/********************************************************
	 * Do not add anything below this line,
	 * as the stacked modules depend on this!
	 */
#include <linux/symtab_end.h>
};

/*
int symbol_table_size = sizeof (symbol_table) / sizeof (symbol_table[0]);
*/
