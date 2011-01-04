/*
 *	dev_table.h
 *
 *	Global definitions for device call tables
 */
/*
 * Copyright (C) by Hannu Savolainen 1993-1996
 *
 * USS/Lite for Linux is distributed under the GNU GENERAL PUBLIC LICENSE (GPL)
 * Version 2 (June 1991). See the "COPYING" file distributed with this software
 * for more info.
 */


#ifndef _DEV_TABLE_H_
#define _DEV_TABLE_H_

#include <linux/config.h>

/*
 * Sound card numbers 27 to 999. (1 to 26 are defined in soundcard.h)
 * Numbers 1000 to N are reserved for driver's internal use.
 */
#define SNDCARD_DESKPROXL		27	/* Compaq Deskpro XL */

/*
 *	NOTE! 	NOTE!	NOTE!	NOTE!
 *
 *	If you modify this file, please check the dev_table.c also.
 *
 *	NOTE! 	NOTE!	NOTE!	NOTE!
 */

extern int sound_started;

struct driver_info {
	char *driver_id;
	int card_subtype;	/* Driver specific. Usually 0 */
	int card_type;		/*	From soundcard.h	*/
	char *name;
	void (*attach) (struct address_info *hw_config);
	int (*probe) (struct address_info *hw_config);
	void (*unload) (struct address_info *hw_config);
};

struct card_info {
	int card_type;	/* Link (search key) to the driver list */
	struct address_info config;
	int enabled;
	void *for_driver_use;
};

typedef struct pnp_sounddev
{
	int id;
	void (*setup)(void *dev);
	char *driver_name;
}pnp_sounddev;

/*
 * Device specific parameters (used only by dmabuf.c)
 */
#define MAX_SUB_BUFFERS		(32*MAX_REALTIME_FACTOR)

#define DMODE_NONE		0
#define DMODE_OUTPUT		PCM_ENABLE_OUTPUT
#define DMODE_INPUT		PCM_ENABLE_INPUT

struct dma_buffparms {
	int      dma_mode;	/* DMODE_INPUT, DMODE_OUTPUT or DMODE_NONE */
	int	 closing;

	/*
 	 * Pointers to raw buffers
 	 */

  	char     *raw_buf;
    	unsigned long   raw_buf_phys;

     	/*
         * Device state tables
         */

	unsigned long flags;
#define DMA_BUSY	0x00000001
#define DMA_RESTART	0x00000002
#define DMA_ACTIVE	0x00000004
#define DMA_STARTED	0x00000008
#define DMA_EMPTY	0x00000010	
#define DMA_ALLOC_DONE	0x00000020
#define DMA_SYNCING	0x00000040
#define DMA_CLEAN	0x00000080

	int      open_mode;

	/*
	 * Queue parameters.
	 */
       	int      qlen;
       	int      qhead;
       	int      qtail;
	int	 cfrag;	/* Current incomplete fragment (write) */

	int      nbufs;
	int      counts[MAX_SUB_BUFFERS];
	int      subdivision;

	int      fragment_size;
	int	 max_fragments;

	int	 bytes_in_use;

	int	 underrun_count;
	int	 byte_counter;

	int	 mapping_flags;
#define			DMA_MAP_MAPPED		0x00000001
	char	neutral_byte;
#ifdef OS_DMA_PARMS
	OS_DMA_PARMS
#endif
};

/*
 * Structure for use with various microcontrollers and DSP processors 
 * in the recent soundcards.
 */
typedef struct coproc_operations {
		char name[32];
		int (*open) (void *devc, int sub_device);
		void (*close) (void *devc, int sub_device);
		int (*ioctl) (void *devc, unsigned int cmd, caddr_t arg, int local);
		void (*reset) (void *devc);

		void *devc;		/* Driver specific info */
	} coproc_operations;

struct audio_driver {
	int (*open) (int dev, int mode);
	void (*close) (int dev);
	void (*output_block) (int dev, unsigned long buf, 
			      int count, int intrflag, int dma_restart);
	void (*start_input) (int dev, unsigned long buf, 
			     int count, int intrflag, int dma_restart);
	int (*ioctl) (int dev, unsigned int cmd, caddr_t arg, int local);
	int (*prepare_for_input) (int dev, int bufsize, int nbufs);
	int (*prepare_for_output) (int dev, int bufsize, int nbufs);
	void (*reset) (int dev);
	void (*halt_xfer) (int dev);
	int (*local_qlen)(int dev);
        void (*copy_from_user)(int dev, char *localbuf, int localoffs,
                               const char *userbuf, int useroffs, int len);
	void (*halt_input) (int dev);
	void (*halt_output) (int dev);
	void (*trigger) (int dev, int bits);
	int (*set_speed)(int dev, int speed);
	unsigned int (*set_bits)(int dev, unsigned int bits);
	short (*set_channels)(int dev, short channels);
};

struct audio_operations {
        char name[32];
	int flags;
#define NOTHING_SPECIAL 	0x00
#define NEEDS_RESTART		0x01
#define DMA_AUTOMODE		0x02
#define DMA_DUPLEX		0x04
#define DMA_PSEUDO_AUTOMODE	0x08
#define DMA_HARDSTOP		0x10
	int  format_mask;	/* Bitmask for supported audio formats */
	void *devc;		/* Driver specific info */
	struct audio_driver *d;
	long buffsize;
	int dmachan1, dmachan2;
	struct dma_buffparms *dmap_in, *dmap_out;
	struct coproc_operations *coproc;
	int mixer_dev;
	int enable_bits;
 	int open_mode;
	int go;
	int min_fragment;	/* 0 == unlimited */
};

struct mixer_operations {
	char id[16];
	char name[32];
	int (*ioctl) (int dev, unsigned int cmd, caddr_t arg);
	
	void *devc;
};

struct synth_operations {
	struct synth_info *info;
	int midi_dev;
	int synth_type;
	int synth_subtype;

	int (*open) (int dev, int mode);
	void (*close) (int dev);
	int (*ioctl) (int dev, unsigned int cmd, caddr_t arg);
	int (*kill_note) (int dev, int voice, int note, int velocity);
	int (*start_note) (int dev, int voice, int note, int velocity);
	int (*set_instr) (int dev, int voice, int instr);
	void (*reset) (int dev);
	void (*hw_control) (int dev, unsigned char *event);
	int (*load_patch) (int dev, int format, const char *addr,
	     int offs, int count, int pmgr_flag);
	void (*aftertouch) (int dev, int voice, int pressure);
	void (*controller) (int dev, int voice, int ctrl_num, int value);
	void (*panning) (int dev, int voice, int value);
	void (*volume_method) (int dev, int mode);
	int (*pmgr_interface) (int dev, struct patmgr_info *info);
	void (*bender) (int dev, int chn, int value);
	int (*alloc_voice) (int dev, int chn, int note, struct voice_alloc_info *alloc);
	void (*setup_voice) (int dev, int voice, int chn);
	int (*send_sysex)(int dev, unsigned char *bytes, int len);

 	struct voice_alloc_info alloc;
 	struct channel_info chn_info[16];
};

struct midi_input_info { /* MIDI input scanner variables */
#define MI_MAX	10
    		int             m_busy;
    		unsigned char   m_buf[MI_MAX];
		unsigned char	m_prev_status;	/* For running status */
    		int             m_ptr;
#define MST_INIT			0
#define MST_DATA			1
#define MST_SYSEX			2
    		int             m_state;
    		int             m_left;
	};

struct midi_operations {
	struct midi_info info;
	struct synth_operations *converter;
	struct midi_input_info in_info;
	int (*open) (int dev, int mode,
		void (*inputintr)(int dev, unsigned char data),
		void (*outputintr)(int dev)
		);
	void (*close) (int dev);
	int (*ioctl) (int dev, unsigned int cmd, caddr_t arg);
	int (*putc) (int dev, unsigned char data);
	int (*start_read) (int dev);
	int (*end_read) (int dev);
	void (*kick)(int dev);
	int (*command) (int dev, unsigned char *data);
	int (*buffer_status) (int dev);
	int (*prefix_cmd) (int dev, unsigned char status);
	struct coproc_operations *coproc;
	void *devc;
};

struct sound_lowlev_timer {
		int dev;
		unsigned int (*tmr_start)(int dev, unsigned int usecs);
		void (*tmr_disable)(int dev);
		void (*tmr_restart)(int dev);
	};

struct sound_timer_operations {
	struct sound_timer_info info;
	int priority;
	int devlink;
	int (*open)(int dev, int mode);
	void (*close)(int dev);
	int (*event)(int dev, unsigned char *ev);
	unsigned long (*get_time)(int dev);
	int (*ioctl) (int dev, unsigned int cmd, caddr_t arg);
	void (*arm_timer)(int dev, long time);
};

#ifdef _DEV_TABLE_C_   
	struct audio_operations *audio_devs[MAX_AUDIO_DEV] = {NULL}; int num_audiodevs = 0;
	struct mixer_operations *mixer_devs[MAX_MIXER_DEV] = {NULL}; int num_mixers = 0;
	struct synth_operations *synth_devs[MAX_SYNTH_DEV+MAX_MIDI_DEV] = {NULL}; int num_synths = 0;
	struct midi_operations *midi_devs[MAX_MIDI_DEV] = {NULL}; int num_midis = 0;

#ifdef CONFIG_SEQUENCER
	extern struct sound_timer_operations default_sound_timer;
	struct sound_timer_operations *sound_timer_devs[MAX_TIMER_DEV] = 
		{&default_sound_timer, NULL}; 
	int num_sound_timers = 1;
#else
	struct sound_timer_operations *sound_timer_devs[MAX_TIMER_DEV] = 
		{NULL}; 
	int num_sound_timers = 0;
#endif

/*
 * List of low level drivers compiled into the kernel.
 */

	struct driver_info sound_drivers[] = {
	    {"AWACS",    0, SNDCARD_AWACS,      "PPC - AWACS Driver",     attach_awacs, probe_awacs, unload_awacs },
	  };

	int num_sound_drivers =
	    sizeof(sound_drivers) / sizeof (struct driver_info);
	int max_sound_drivers =
	    sizeof(sound_drivers) / sizeof (struct driver_info);


#ifndef FULL_SOUND
/*
 *	List of devices actually configured in the system.
 *
 *	Note! The detection order is significant. Don't change it.
 */

	struct card_info snd_installed_cards[] = {
	  {SNDCARD_AWACS,     {-1, -1, -1, -1, 0, "awacs"    }, SND_DEFAULT_ENABLE},
     
/* Define some expansion space */
		{0, {0}, 0},
		{0, {0}, 0},
		{0, {0}, 0},
		{0, {0}, 0},
		{0, {0}, 0}
	};

	int num_sound_cards =
	    sizeof(snd_installed_cards) / sizeof (struct card_info);
	int max_sound_cards =
	    sizeof(snd_installed_cards) / sizeof (struct card_info);

#else
	int num_sound_cards = 0;
	struct card_info snd_installed_cards[20] = {{0}};
	int max_sound_cards = 20;
#endif

#   ifdef MODULE
	int trace_init = 0;
#   else
	int trace_init = 1;
#   endif
#else
	extern struct audio_operations * audio_devs[MAX_AUDIO_DEV]; extern int num_audiodevs;
	extern struct mixer_operations * mixer_devs[MAX_MIXER_DEV]; extern int num_mixers;
	extern struct synth_operations * synth_devs[MAX_SYNTH_DEV+MAX_MIDI_DEV]; extern int num_synths;
	extern struct midi_operations * midi_devs[MAX_MIDI_DEV]; extern int num_midis;
	extern struct sound_timer_operations * sound_timer_devs[MAX_TIMER_DEV]; extern int num_sound_timers;

	extern struct driver_info sound_drivers[];
	extern int num_sound_drivers;
	extern int max_sound_drivers;
	extern struct card_info snd_installed_cards[];
	extern int num_sound_cards;
	extern int max_sound_cards;

	extern int trace_init;
#endif	/* _DEV_TABLE_C_ */

void sndtable_init(void);
int sndtable_get_cardcount (void);
struct address_info *sound_getconf(int card_type);
void sound_chconf(int card_type, int ioaddr, int irq, int dma);
int snd_find_driver(int type);
void sound_unload_drivers(void);
void sound_unload_driver(int type);
int sndtable_identify_card(char *name);
void sound_setup (char *str, int *ints);

int sound_alloc_dmap (int dev, struct dma_buffparms *dmap, int chan);
void sound_free_dmap (int dev, struct dma_buffparms *dmap);
extern int sound_map_buffer (int dev, struct dma_buffparms *dmap, buffmem_desc *info);
void install_pnp_sounddrv(struct pnp_sounddev *drv);
int sndtable_probe (int unit, struct address_info *hw_config);
int sndtable_init_card (int unit, struct address_info *hw_config);
int sndtable_start_card (int unit, struct address_info *hw_config);
void sound_timer_init (struct sound_lowlev_timer *t, char *name);
int sound_start_dma (	int dev, struct dma_buffparms *dmap, int chan,
			unsigned long physaddr,
			int count, int dma_mode, int autoinit);
void sound_dma_intr (int dev, struct dma_buffparms *dmap, int chan);

#define AUDIO_DRIVER_VERSION	1
#define MIXER_DRIVER_VERSION	1
int sound_install_audiodrv(int vers,
			   char *name,
			   struct audio_driver *driver,
			   int driver_size,
			   int flags,
      			   unsigned int format_mask,
			   void *devc,
			   int dma1, 
			   int dma2);
int sound_install_mixer(int vers, 
			char *name,
			struct mixer_operations *driver,
			int driver_size,
			void *devc);

#endif	/* _DEV_TABLE_H_ */



