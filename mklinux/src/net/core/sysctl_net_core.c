/* -*- linux-c -*-
 * sysctl_net_core.c: sysctl interface to net core subsystem.
 *
 * Begun April 1, 1996, Mike Shaver.
 * Added /proc/sys/net/core directory entry (empty =) ). [MS]
 */

#include <linux/mm.h>
#include <linux/sysctl.h>
#include <linux/config.h>

#ifdef CONFIG_NET_ALIAS
extern int sysctl_net_alias_max;
extern int proc_do_net_alias_max(ctl_table *, int, struct file *, void *, size_t *);
#endif

ctl_table core_table[] = {
#ifdef CONFIG_NET_ALIAS
	{NET_CORE_NET_ALIAS_MAX, "net_alias_max", &sysctl_net_alias_max, sizeof(int),
	 0644, NULL, &proc_do_net_alias_max },
#endif  
	{0}
};
