/*
 *  linux/arch/ppc/kernel/root_sd.c
 *
 *  Copyright (C) 1996, by Gary Thomas
 *
 *  Set root device to be "/dev/sda1"
 */

/*
 * bootup setup stuff..
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
#include <linux/tty.h>

#define DEFAULT_ROOT_DEVICE 0x0801	/* sda1 */

void
set_root_dev(void)
{
  ROOT_DEV = to_kdev_t(DEFAULT_ROOT_DEVICE);
}
