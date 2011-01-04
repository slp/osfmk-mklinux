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
#include <sb.h>

#ifndef _OS_H_
#define _OS_H_
/*
 * OS specific settings for Mach
 */

/*
 * Insert here the includes required by your kernel.
 */
#include <eisa.h>
#include <himem.h>

#include <i386/pio.h>
#include <i386/AT386/misc_protos.h>
#include <i386/AT386/himem.h>

#include <mach/machine/vm_types.h>

#include <vm/vm_kern.h>

#include <device/errno.h>
#include <device/misc_protos.h>
#include <device/device_typedefs.h>
#include <device/io_req.h>
#include <device/ds_routines.h>

#include <i386/AT386/sound/sb_entries.h>

#include <string.h>

#define O_ACCMODE (0xffffffff)

/* These few lines are used by 386BSD (only??). */

#if NSB > 0
#define KERNEL_SOUNDCARD
#else
#undef KERNEL_SOUNDCARD
#endif


/*
 * Rest of the file is compiled only if the driver is really required.
 */
#ifdef CONFIGURE_SOUNDCARD

/* 
 * select() is currently implemented in Linux specific way. Don't enable.
 * I don't remember what the SHORT_BANNERS means so forget it.
 */

#undef ALLOW_SELECT
#undef SHORT_BANNERS

/* The soundcard.h could be in a nonstandard place so inclyde it here. */
#include <i386/soundcard.h>

/*
 * Here is the first portability problem. Every OS has it's own way to
 * pass a pointer to the buffer in read() and write() calls. In Linux it's
 * just a char*. In BSD it's struct uio. This parameter is passed to
 * all functions called from read() or write(). Since nothing can be 
 * assumed about this structure, the driver uses set of macros for
 * accessing the user buffer. 
 *
 * The driver reads/writes bytes in the user buffer sequentially which
 * means that calls like uiomove() can be used.
 *
 * snd_rw_buf is the type which is passed to the device file specific
 * read() and write() calls.
 * 
 * The following macros are used to move date to and from the
 * user buffer. These macros should be used only when the 
 * target or source parameter has snd_rw_buf type.
 * The offs parameter is a offset relative to the beginning of
 * the user buffer. In Linux the offset is required but for example
 * BSD passes the offset info in the uio structure. It could be usefull
 * if these macros verify that the offs parameter and the value in
 * the snd_rw_buf structure are equal.
 */
typedef struct io_req snd_rw_buf;

/*
 * Move bytes from the buffer which the application given in a
 * write() call.
 * offs is position relative to the beginning of the buffer in
 * user space. The count is number of bytes to be moved.
 */
/*
 * The mapped or allocated area for writing data will be
 * removed or deallocated in the ds_write_done routine.
 */
/*
 * For now, large data case is omitted.
 */
#define WRITE_PREPARE(buf) \
{ \
    kern_return_t kr; \
    boolean_t wait = FALSE; \
 \
    kr = device_write_get((buf), &wait); \
    if (wait == TRUE) { \
	printf("Data is too big\n"); \
	return (RET_ERROR(D_INVALID_SIZE)); \
    } \
}

#define READ_PREPARE(buf) \
{ \
    kern_return_t kr; \
 \
    kr = device_read_alloc((buf), (vm_size_t) (buf)->io_count); \
    if (kr != KERN_SUCCESS) { \
	printf("device_read_alloc failed\n"); \
	return (RET_ERROR(D_INVALID_SIZE)); \
    } \
}

#define COPY_FROM_USER(target, source, offs, count) \
{ \
    bcopy(((io_req_t) (source))->io_data + (offs), (char *) (target), \
	  (count)); \
}

/* Like COPY_FOM_USER but for writes. */
#define COPY_TO_USER(target, offs, source, count) \
{ \
    kern_return_t kr; \
 \
    bcopy((char *) (source), (target)->io_data + (offs), (count)); \
    (target)->io_residual += (count); \
}

#define DMA_BASE (0x00)
#define DMA_MASK (0x0a)
#define DMA_CLEAR (0x0c)
#define DMA_MODE (0x0b)
#define DMA_PAGE (0x83)
#define DMA_OFFSET (0x02)
#define DMA_COUNT (0x03)

#define DMA2_BASE (0xc0)
#define DMA2_MASK (0xd4)
#define DMA2_CLEAR (0xd8)
#define DMA2_MODE (0xd6)
#define DMA2_PAGE (0x)
#define DMA2_OFFSET (0x02)
#define DMA2_COUNT (0x03)

#define DMA_MASK_SET (0x4)
#define DMA_READ (0x8)
#define DMA_WRITE (0x4)
#define DMA_AUTOINITIALIZE (0x10)
#define DMA_SINGLE_MODE (0x40)

#define DMA_PAGE_0              0x87    /* DMA page registers */
#define DMA_PAGE_1              0x83
#define DMA_PAGE_2              0x81
#define DMA_PAGE_3              0x82
#define DMA_PAGE_5              0x8B
#define DMA_PAGE_6              0x89
#define DMA_PAGE_7              0x8A

/* DMA control */
#define clear_dma_ff(chan) \
{ \
    if (chan <= 3) { \
	outb(DMA_CLEAR, 0); \
    } else { \
	outb(DMA2_CLEAR, 0); \
    } \
}

#define disable_dma(chan) \
{ \
    if (chan <= 3) { \
	outb(DMA_MASK, (chan) | DMA_MASK_SET); \
    } else { \
	outb(DMA2_MASK, ((chan) & 3) | DMA_MASK_SET); \
    } \
}

static int get_dma_residue(unsigned int chan);

static __inline__ int get_dma_residue(unsigned int chan)
{
    unsigned int io_port = (chan <= 3)? ((chan & 3) << 1) + 1 + DMA_BASE
	: ((chan & 3) << 2) + 2 + DMA2_BASE;

    /* using short to get 16-bit wrap around */
    unsigned short count;

    count = 1 + inb(io_port);
    count += inb(io_port) << 8;
	
    return ((chan <=3) ? count : (count << 1));
}

#define enable_dma(chan) \
{ \
    if (chan <= 3) { \
	outb(DMA_MASK, (chan)); \
    } else { \
	outb(DMA2_MASK, ((chan) & 3)); \
    } \
}

#define set_dma_page(chan, page) \
{ \
    switch (chan) { \
    case 0: \
	outb(DMA_PAGE_0, (page)); \
	DEB(printf("outb(%x, %x);", DMA_PAGE_0, (page))); \
	break; \
    case 1: \
	outb(DMA_PAGE_1, (page)); \
	DEB(printf("outb(%x, %x);", DMA_PAGE_1, (page))); \
	break; \
    case 2: \
	outb(DMA_PAGE_2, (page)); \
	DEB(printf("outb(%x, %x);", DMA_PAGE_2, (page))); \
	break; \
    case 3: \
	outb(DMA_PAGE_3, (page)); \
	DEB(printf("outb(%x, %x);", DMA_PAGE_3, (page))); \
	break; \
    case 5: \
	outb(DMA_PAGE_5, (page)); \
	DEB(printf("outb(%x, %x);", DMA_PAGE_5, (page))); \
	break; \
    case 6: \
	outb(DMA_PAGE_6, (page)); \
	DEB(printf("outb(%x, %x);", DMA_PAGE_6, (page))); \
	break; \
    case 7: \
	outb(DMA_PAGE_7, (page)); \
	DEB(printf("outb(%x, %x);", DMA_PAGE_7, (page))); \
	break; \
    } \
}

#define dma_start(mode, physaddr, count, channel) \
{ \
    unsigned long flags; \
 \
    DISABLE_INTR(flags); \
    outb(DMA_MASK, (channel) | DMA_MASK_SET); \
    DEB(printf("outb(%x, %x);", DMA_MASK, (channel) | DMA_MASK_SET)); \
    outb(DMA_CLEAR, 0); \
    DEB(printf("outb(%x, %x);", DMA_CLEAR, 0)); \
    outb(DMA_MODE, DMA_SINGLE_MODE | (mode) | (channel));  \
    DEB(printf("outb(%x, %x);", DMA_MODE, DMA_SINGLE_MODE | (mode) | (channel))); \
    outb(DMA_BASE + (((channel) & 3) << 1), (physaddr) & 0xff); \
    DEB(printf("outb(%x, %x);", DMA_BASE + (((channel) & 3) << 1), \
	       (physaddr) & 0xff)); \
    outb(DMA_BASE + (((channel) & 3) << 1), ((physaddr) & 0xff00) >> 8); \
    DEB(printf("outb(%x, %x);", DMA_BASE + (((channel) & 3) << 1), \
	       ((physaddr) & 0xff00) >> 8)); \
    set_dma_page(channel, ((physaddr) >> 16) & 0xff); \
    outb(DMA_BASE + (((channel) & 3) << 1) + 1, (count) & 0xff); \
    DEB(printf("outb(%x, %x);", DMA_BASE + (((channel) & 3) << 1) + 1, \
	       (count) & 0xff)); \
    outb(DMA_BASE + (((channel) & 3) << 1) + 1, ((count) & 0xff00) >> 8); \
    DEB(printf("outb(%x, %x);", DMA_BASE + (((channel) & 3) << 1) + 1, \
	       ((count) & 0xff00) >> 8)); \
    outb(DMA_MASK, (channel)); \
    DEB(printf("outb(%x, %x)\n", DMA_MASK, (channel))); \
    RESTORE_INTR(flags); \
}

#define dma_start16(mode, physaddr, count, channel) \
{ \
    unsigned long flags; \
 \
    DISABLE_INTR(flags); \
    outb(DMA2_MASK, ((channel) & 3) | DMA_MASK_SET); \
    DEB(printf("outb(%x, %x);", DMA2_MASK, ((channel) & 3) | DMA_MASK_SET)); \
    outb(DMA2_CLEAR, 0); \
    DEB(printf("outb(%x, %x);", DMA2_CLEAR, 0)); \
    outb(DMA2_MODE, DMA_SINGLE_MODE | (mode) | ((channel) & 3));  \
    DEB(printf("outb(%x, %x);", DMA2_MODE, \
	       DMA_SINGLE_MODE | (mode) | ((channel) & 3))); \
    outb(DMA2_BASE + (((channel) & 3) << 2), ((physaddr) >> 1) & 0xff); \
    DEB(printf("outb(%x, %x);", DMA2_BASE + (((channel) & 3) << 2), \
	       ((physaddr) >> 1) & 0xff)); \
    outb(DMA2_BASE + (((channel) & 3) << 2), ((physaddr) >> 9) & 0xff); \
    DEB(printf("outb(%x, %x);", DMA2_BASE + (((channel) & 3) << 2), \
	       ((physaddr) >> 9) & 0xff)); \
    set_dma_page(channel, ((physaddr) >> 16) & 0xff); \
    outb(DMA2_BASE + (((channel) & 3) << 2) + 2, ((count) >> 1) & 0xff); \
    DEB(printf("outb(%x, %x);", DMA2_BASE + (((channel) & 3) << 2) + 2, \
	       ((count) >> 1) & 0xff)); \
    outb(DMA2_BASE + (((channel) & 3) << 2) + 2, ((count) >> 9) & 0xff); \
    DEB(printf("outb(%x, %x);", DMA2_BASE + (((channel) & 3) << 2) + 2, \
	       ((count) >> 9) & 0xff)); \
    outb(DMA2_MASK, (channel)); \
    DEB(printf("outb(%x, %x)\n", DMA2_MASK, (channel))); \
    RESTORE_INTR(flags); \
}


/* 
 * The following macros are like COPY_*_USER but work just with one byte (8bit),
 * short (16 bit) or long (32 bit) at a time.
 * The same restrictions apply than for COPY_*_USER
 */
#define GET_BYTE_FROM_USER(target, addr, offs) \
{ \
    unsigned char *c; \
 \
    c = (unsigned char *) ((int) (target)); \
    *c = *((unsigned char *) ((addr)->io_data + (offs))); \
}

#define GET_SHORT_FROM_USER(target, addr, offs)	{uiomove((char*)&(target), 2, addr);}
#define GET_WORD_FROM_USER(target, addr, offs)	{uiomove((char*)&(target), 4, addr);}
#define PUT_WORD_TO_USER(addr, offs, data) \
{ \
    bcopy((char *) (addr), ((char *) &(data)) + (offs), 4); \
}

/*
 * The way how the ioctl arguments are passed is another nonportable thing.
 * In Linux the argument is just a pointer directly to the user segment. On
 * 386bsd the data is already moved to the kernel space. The following
 * macros should handle the difference.
 */

/*
 * IOCTL_FROM_USER is used to copy a record pointed by the argument to
 * a buffer in the kernel space. On 386bsd it can be done just by calling
 * memcpy. With Linux a memcpy_from_fs should be called instead.
 * Parameters of the following macros are like in the COPY_*_USER macros.
 */

/*
 * When the ioctl argument points to a record or array (longer than 32 bits),
 * the macros IOCTL_*_USER are used. It's assumed that the source and target
 * parameters are direct memory addresses.
 */
#define IOCTL_FROM_USER(target, source, offs, count) \
{ \
    memcpy((char *) (target), &((source)[offs]), (count)); \
}

#define IOCTL_TO_USER(target, offs, source, count) \
{ \
    memcpy((char *) &((target)[offs]), (source), (count)); \
}

/* The following macros are used if the ioctl argument points to 32 bit int */
#define IOCTL_IN(arg)			(*(int*)arg)
#define IOCTL_OUT(arg, ret)		*(int*)arg = ret, 0

/*
 * When the driver displays something to the console, printk() will be called.
 * The name can be changed here.
 */
#define printk 		printf

/*
 * The following macros define an interface to the process management.
 */

struct snd_wait {
	  int mode; int aborting;
	};

/*
 * DEFINE_WAIT_QUEUE is used where a wait queue is required. It must define
 * a structure which can be passed as a parameter to a sleep(). The second
 * parameter is name of a flag variable (must be defined as int).
 */
#define DEFINE_WAIT_QUEUE(qname, flag) int *qname = NULL; \
	volatile struct snd_wait flag = {0}
/* Like the above but defines an array of wait queues and flags */
#define DEFINE_WAIT_QUEUES(qname, flag) int *qname = {NULL}; \
	volatile struct snd_wait flag = {{0}}

#define RESET_WAIT_QUEUE(q, f) {f.aborting = 0;f.mode = WK_NONE;}
#define SET_ABORT_FLAG(q, f) f.aborting = 1
#define TIMED_OUT(q, f) (f.mode & WK_TIMEOUT)
#define SOMEONE_WAITING(q, f) (f.mode & WK_SLEEP)
/*
 * This driver handles interrupts little bit nonstandard way. The following
 * macro is used to test if the current process has received a signal which
 * is aborts the process. This macro is called from close() to see if the
 * buffers should be discarded. If this kind info is not available, a constant
 * 1 or 0 could be returned (1 should be better than 0).
 * I'm not sure if the following is correct for 386BSD.
 */
#define PROCESS_ABORTING(q, f) (f.aborting)

/*
 * The following macro calls sleep. It should be implemented such that
 * the process is resumed if it receives a signal. The following is propably
 * not the way how it should be done on 386bsd.
 * The on_what parameter is a wait_queue defined with DEFINE_WAIT_QUEUE(),
 * and the second is a workarea parameter. The third is a timeout 
 * in ticks. Zero means no timeout.
 *
 * Condition DISABLE INTR
 */
#define DO_SLEEP(q, f, time_limit) \
{ \
    int flag; \
    thread_t th; \
    spl_t s; \
 \
    f.mode = WK_SLEEP; \
    assert_wait((event_t) &(q), FALSE); \
    thread_set_timeout(time_limit); \
    thread_block((void (*)(void)) 0); \
    f.aborting = 0; \
    th = current_thread(); \
    s = splsched(); \
    thread_lock(th); \
    if (th->wait_result == THREAD_TIMED_OUT) { \
	f.mode |= WK_TIMEOUT; \
    } else if (th->wait_result == THREAD_INTERRUPTED) { \
	f.aborting = 1; \
    } \
    thread_unlock(th); \
    splx(s); \
    f.mode &= ~WK_SLEEP; \
}

/* An the following wakes up a process */
#define WAKE_UP(q, f)	{f.mode = WK_WAKEUP;wakeup(&(q));}

/*
 * Timing macros. This driver assumes that there is a timer running in the
 * kernel. The timer should return a value which is increased once at every
 * timer tick. The macro HZ should return the number of such ticks/sec.
 */

#ifndef HZ
extern int hz;
#define HZ	hz
#endif

/* 
 * GET_TIME() returns current value of the counter incremented at timer
 * ticks.  This can overflow, so the timeout might be real big...
 * 
 */
natural_t timeout_ticks;

#define GET_TIME() timeout_ticks
#if 0
#define GET_TIME()	(lbolt)	/* Returns current time (1/HZ secs since boot) */
#endif /* 0 */

/*
 * The following three macros are called before and after atomic
 * code sequences. The flags parameter has always type of unsigned long.
 * The macro DISABLE_INTR() should ensure that all interrupts which
 * may invoke any part of the driver (timer, soundcard interrupts) are
 * disabled.
 * RESTORE_INTR() should return the interrupt status back to the
 * state when DISABLE_INTR() was called. The flags parameter is
 * a variable which can carry 32 bits of state information between
 * DISABLE_INTR() and RESTORE_INTR() calls.
 */
#define DISABLE_INTR(flags)	flags = splbio()
#define RESTORE_INTR(flags) \
{ \
    if (flags < 0 || flags > 8) { \
	panic("bad splx %d\n", flags); \
    } \
    splx(flags); \
}

/*
 * SB_INB() and SB_OUTB() should be obvious. NOTE! The order of
 * paratemeters of SB_OUTB() is different than on some other
 * operating systems.
 */

#define SB_INB			inb
/*  
 * The outb(0, 0x80) is just for slowdown. It's bit unsafe since
 * this address could be used for something usefull.
 */
#define SB_OUTB(addr, data)	outb(data, addr);

#if 0
/* memcpy() was not defined og 386bsd. Lets define it here */
#define memcpy(d, s, c)		bcopy(s, d, c)
#endif /* 0 */

/*
 * When a error (such as EINVAL) is returned by a function,
 * the following macro is used. The driver assumes that a
 * error is signalled by returning a negative value.
 */

#define RET_ERROR(err)		-(err)

/* 
   KERNEL_MALLOC() allocates requested number of memory  and 
   KERNEL_FREE is used to free it. 
   These macros are never called from interrupt, in addition the
   nbytes will never be more than 4096 bytes. Generally the driver
   will allocate memory in blocks of 4k. If the kernel has just a
   page level memory allocation, 4K can be safely used as the size
   (the nbytes parameter can be ignored).
*/
extern char *kalloc_interface(int nbytes);
extern void kfree_interface(char *ptr);

#define	KERNEL_MALLOC(nbytes)	kalloc_interface((nbytes))
#define	KERNEL_FREE(addr)	kfree_interface((char *)(addr))

/*
 * The macro PERMANENT_MALLOC(typecast, mem_ptr, size, linux_ptr)
 * returns size bytes of
 * (kernel virtual) memory which will never get freed by the driver.
 * This macro is called only during boot. The linux_ptr is a linux specific
 * parameter which should be ignored in other operating systems.
 * The mem_ptr is a pointer variable where the macro assigns pointer to the
 * memory area. The type is the type of the mem_ptr.
 */
#define PERMANENT_MALLOC(typecast, mem_ptr, size, linux_ptr) \
{ \
    mem_ptr = (typecast) kalloc_interface(size); \
    if (!mem_ptr) { \
	panic("SOUND: Cannot allocate memory\n"); \
    } \
}

/*
 * The macro DEFINE_TIMER defines variables for the ACTIVATE_TIMER if
 * required. The name is the variable/name to be used and the proc is
 * the procedure to be called when the timer expires.
 */

#define DEFINE_TIMER(name, proc) struct dummy {int a;}

/*
 * The ACTIVATE_TIMER requests system to call 'proc' after 'time' ticks.
 */

#define ACTIVATE_TIMER(name, proc, time) \
	timeout((timeout_fcn_t) proc, 0, time);
/*
 * The rest of this file is not complete yet. The functions using these
 * macros will not work
 */
#define ALLOC_DMA_CHN(chn) (0)
#define RELEASE_DMA_CHN(chn) (0)
#define DMA_MODE_READ		0
#define DMA_MODE_WRITE		1
#define RELEASE_IRQ(irq_no)

#define FIX_RETURN(func) \
{ \
    int ret; \
 \
    ret = func; \
    if (ret < 0) { \
	return (-ret); \
    } else { \
	return (0); \
    } \
}

#endif

#if CBUS || HIMEM 
#if     HIMEM
#define AU_LOW_PAGES 24		/* Number of low memory pages reserved */
#endif /* HIMEM */

#if CBUS
NOT SUPPORTED /* DFE */
#endif /* CBUS */
#endif /* CBUS || HIMEM */

#endif
