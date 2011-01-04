/*
 *
 *
 *
 */
#include "awacs_types.h"

/*
 *
 */
int probe_awacs( struct address_info *hw_config )
{
  UNUSED( hw_config );
#if 0
  printk( "probe_awacs() - %s %d\n\r", __FILE__, __LINE__ ); 
#endif 
  return( 1 );
}

/*
 *
 */
void attach_awacs( struct address_info *hw_config )
{
  int             audio_flags = 0;
  int             format_mask = AFMT_S16_LE;
  awacs_devc_t    *devc;

  struct audio_driver *driver = &awacs_audio_driver;
#if 0
  printk( "attach_awacs() - %s %d\n\r", __FILE__, __LINE__ );
#endif
  devc = (awacs_devc_t *) vmalloc( sizeof(awacs_devc_t) );
  memset( devc, 0, sizeof(awacs_devc_t) );
  
  if ((devc->dev = sound_install_audiodrv (AUDIO_DRIVER_VERSION,
					      hw_config->name,
					      driver,
					      sizeof (struct audio_driver),
					      audio_flags,
					      format_mask,
					      devc,
					      -1,
					      -1)) < 0)
    {
      printk( "sound_install_audiodrv - rc = %d - %s %d\n\r", devc->dev, __FILE__, __LINE__ );
      return;
    }

  if ( (devc->mixer_dev = sound_install_mixer( MIXER_DRIVER_VERSION, 
					       hw_config->name,
					       (struct mixer_operations *)&awacs_mixer_driver,
					       sizeof(struct mixer_operations), 
					       devc)) < 0 )
    {
      printk( "sound_install_mixer - rc = %d - %s %d\n\r", devc->dev, __FILE__, __LINE__ );
      return;
    }
	 

  awacs_devs[awacs_num_devs].dev   = devc->dev;
  awacs_devs[awacs_num_devs].mixer = devc->mixer_dev;
  awacs_devs[awacs_num_devs].devc  = devc;
  awacs_num_devs++;

  devc->audio_bits     = 16;
  devc->audio_channels = 2;
  devc->audio_speed    = 22050;
  memset( devc->mach_dev_name, sizeof(devc->mach_dev_name), 0 );
  strcpy( devc->mach_dev_name, hw_config->name );

  return;
}

/*
 *
 */
void unload_awacs( struct address_info *hw_config )
{
  UNUSED( hw_config );
  printk( "unload_awacs() - %s %d\n\r", __FILE__, __LINE__ );  
}

/*
 *
 */
int probe_awacs_mpu( struct address_info *hw_config )
{
  UNUSED( hw_config );
#if 0
  printk( "probe_awacs_mpu() - %s %d\n\r", __FILE__, __LINE__ );  
#endif
  return( 0 );
}
/*
 *
 */
void attach_awacs_mpu( struct address_info *hw_config )
{
  UNUSED( hw_config );
#if 0
  printk( "attach_awacs_mpu() - %s %d\n\r", __FILE__, __LINE__ );  
#endif
}

/*
 *
 */
void unload_awacs_mpu( struct address_info *hw_config )
{
  UNUSED( hw_config );
#if 0
  printk( "unload_awacs_mpu() - %s %d\n\r", __FILE__, __LINE__ ); 
#endif 
}

/*
 *
 *
 */
awacs_devc_t *awacs_dev_to_devc( int dev )
{
  int           i;

  for (i=0; i < awacs_num_devs; i++ )
    {
      if ( awacs_devs[i].dev == dev )
	{
	  return awacs_devs[i].devc;
        }
    }
  return 0;
}


/*
 *
 *
 */
awacs_devc_t *awacs_mixer_to_devc( int dev )
{
  int           i;

  for (i=0; i < awacs_num_devs; i++ )
    {
      if ( awacs_devs[i].mixer == dev )
	{
	  return awacs_devs[i].devc;
        }
    }
  return 0;
}


