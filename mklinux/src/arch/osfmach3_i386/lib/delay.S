#include <linux/linkage.h>

/*
 * BogoMips loop. Non-inlined because various x86's have so wildly
 * varying results depending on the exact alignment.
 */

ENTRY(__do_delay)
1:	decl %eax
	jns 1b
	ret



