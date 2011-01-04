/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/kernel.h>
#include <linux/tty.h>
#include <linux/delay.h>

struct screen_info screen_info = {
	0, 24,			/* orig-x, orig-y */
	{ 0, 0 },		/* unused */
	0,			/* orig-video-page */
	0,			/* orig-video-mode */
	80,			/* orig-video-cols */
	0,0,0,			/* ega_ax, ega_bx, ega_cx */
	25,			/* orig-video-lines */
	1,			/* orig-video-isVGA */
	16			/* orig-video-points */
};

char x86 = 0;

extern int host_cpus;

int get_cpuinfo(char * buffer)
{
        int i, len = 0;
	int n;
#if 0
        static const char *x86_cap_flags[] = {
                "fpu", "vme", "de", "pse", "tsc", "msr", "pae", "mce",
                "cx8", "apic", "10", "11", "mtrr", "pge", "mca", "cmov",
                "16", "17", "18", "19", "20", "21", "22", "mmx",
                "24", "25", "26", "27", "28", "29", "30", "31"
        };
#endif
        
        for ( n = 0 ; n < host_cpus ; n++ ) {
		if (len) buffer[len++] = '\n'; 
		len += sprintf(buffer+len,"processor\t: %d\n"
			       "cpu\t\t: %c86\n"
			       "model\t\t: %s\n"
			       "vendor_id\t: %s\n",
			       n,
			       x86 + '0',
			       "unknown",
			       "unknown");
        
		len += sprintf(buffer+len, 
			       "stepping\t: unknown\n");
        
		len += sprintf(buffer+len,
			       "fdiv_bug\t: %s\n"
			       "hlt_bug\t\t: %s\n"
			       "f00f_bug\t: %s\n"
			       "fpu\t\t: %s\n"
			       "fpu_exception\t: %s\n"
			       "cpuid\t\t: %s\n"
			       "wp\t\t: %s\n"
			       "flags\t\t:",
			       "unknown",
			       "unknwon",
			       "unknown",
			       "unknown",
			       "unknown",
			       "unknown",
			       "unknown");
        
		for ( i = 0 ; i < 32 ; i++ ) {
#if 0
			if ( CD(x86_capability) & (1 << i) ) {
				len += sprintf(buffer+len, " %s",
					       x86_cap_flags[i]);
			}
#endif
		}
		len += sprintf(buffer+len,
			       "\nbogomips\t: %lu.%02lu\n",
			       (loops_per_sec+2500)/500000,
			       ((loops_per_sec+2500)/5000) % 100);
        }
        return len;
}
