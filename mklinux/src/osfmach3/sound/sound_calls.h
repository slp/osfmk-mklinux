/*
 *	DMA buffer calls
 */

int DMAbuf_open(int dev, int mode);
int DMAbuf_release(int dev, int mode);
int DMAbuf_getwrbuffer(int dev, char **buf, int *size, int dontblock);
int DMAbuf_get_curr_buffer(int dev, int *buff_no, char **dma_buf, int *buff_ptr, int *buff_size);
int DMAbuf_getrdbuffer(int dev, char **buf, int *len, int dontblock);
void DMAbuf_primeinput(int dev);
int DMAbuf_rmchars(int dev, int buff_no, int c);
int DMAbuf_start_output(int dev, int buff_no, int l);
int DMAbuf_set_count(int dev, int buff_no, int l);
int DMAbuf_ioctl(int dev, unsigned int cmd, caddr_t arg, int local);
void DMAbuf_init(void);
int DMAbuf_start_dma (int dev, unsigned long physaddr, int count, int dma_mode);
int DMAbuf_open_dma (int dev);
void DMAbuf_close_dma (int dev);
void DMAbuf_reset_dma (int dev);
void DMAbuf_inputintr(int dev);
void DMAbuf_outputintr(int dev, int underflow_flag);
int DMAbuf_select(int dev, struct fileinfo *file, int sel_type, select_table_handle * wait);
void DMAbuf_start_devices(unsigned int devmask);

/*
 *	System calls for /dev/dsp and /dev/audio
 */

int audio_read (int dev, struct fileinfo *file, char *buf, int count);
int audio_write (int dev, struct fileinfo *file, const char *buf, int count);
int audio_open (int dev, struct fileinfo *file);
void audio_release (int dev, struct fileinfo *file);
int audio_ioctl (int dev, struct fileinfo *file,
	   unsigned int cmd, caddr_t arg);
int audio_lseek (int dev, struct fileinfo *file, off_t offset, int orig);
void audio_init (void);

int audio_select(int dev, struct fileinfo *file, int sel_type, select_table_handle * wait);

/*
 *	System calls for the /dev/sequencer
 */

int sequencer_read (int dev, struct fileinfo *file, char *buf, int count);
int sequencer_write (int dev, struct fileinfo *file, const char *buf, int count);
int sequencer_open (int dev, struct fileinfo *file);
void sequencer_release (int dev, struct fileinfo *file);
int sequencer_ioctl (int dev, struct fileinfo *file,
	   unsigned int cmd, caddr_t arg);
int sequencer_lseek (int dev, struct fileinfo *file, off_t offset, int orig);
void sequencer_init (void);
void sequencer_timer(unsigned long dummy);
int note_to_freq(int note_num);
unsigned long compute_finetune(unsigned long base_freq, int bend, int range);
void seq_input_event(unsigned char *event, int len);
void seq_copy_to_input (unsigned char *event, int len);

int sequencer_select(int dev, struct fileinfo *file, int sel_type, select_table_handle * wait);

/*
 *	System calls for the /dev/midi
 */

int MIDIbuf_read (int dev, struct fileinfo *file, char *buf, int count);
int MIDIbuf_write (int dev, struct fileinfo *file, const char *buf, int count);
int MIDIbuf_open (int dev, struct fileinfo *file);
void MIDIbuf_release (int dev, struct fileinfo *file);
int MIDIbuf_ioctl (int dev, struct fileinfo *file,
	   unsigned int cmd, caddr_t arg);
int MIDIbuf_lseek (int dev, struct fileinfo *file, off_t offset, int orig);
void MIDIbuf_bytes_received(int dev, unsigned char *buf, int count);
void MIDIbuf_init(void);

int MIDIbuf_select(int dev, struct fileinfo *file, int sel_type, select_table_handle * wait);

/*
 *
 *	Misc calls from various sources
 */

/*	From soundcard.c	*/
void soundcard_init(void);
void tenmicrosec(int *osp);
void request_sound_timer (int count);
void sound_stop_timer(void);
int snd_ioctl_return(int *addr, int value);
int snd_set_irq_handler (int interrupt_level, void(*iproc)(int, void*, struct pt_regs *), char *name, int *osp);
void snd_release_irq(int vect);
void sound_dma_malloc(int dev);
void sound_dma_free(int dev);
void conf_printf(char *name, struct address_info *hw_config);
void conf_printf2(char *name, int base, int irq, int dma, int dma2);

/*	From sound_switch.c	*/
int sound_read_sw (int dev, struct fileinfo *file, char *buf, int count);
int sound_write_sw (int dev, struct fileinfo *file, const char *buf, int count);
int sound_open_sw (int dev, struct fileinfo *file);
void sound_release_sw (int dev, struct fileinfo *file);
int sound_ioctl_sw (int dev, struct fileinfo *file,
	     unsigned int cmd, caddr_t arg);

/*	From patmgr.c */
int pmgr_open(int dev);
void pmgr_release(int dev);
int pmgr_read (int dev, struct fileinfo *file, char * buf, int count);
int pmgr_write (int dev, struct fileinfo *file, const char * buf, int count);
int pmgr_access(int dev, struct patmgr_info *rec);
int pmgr_inform(int dev, int event, unsigned long parm1, unsigned long parm2,
				    unsigned long parm3, unsigned long parm4);

/*	From sound_timer.c */
void sound_timer_interrupt(void);
void sound_timer_syncinterval(unsigned int new_usecs);

/* From drvr_awacs.c */
int  probe_awacs(struct address_info *hw_config );
void attach_awacs(struct address_info *hw_config );
void unload_awacs(struct address_info *hw_config );

int  probe_awacs_mpu(struct address_info *hw_config );
void attach_awacs_mpu(struct address_info *hw_config );
void unload_awacs_mpu(struct address_info *hw_config );



