 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Support for the Mute sound system
  *
  * Copyright 1997 Bernd Schmidt
  * Copyright 2003 Richard Drummond
  */
#include <pspkerneltypes.h>
#include <pspkernel.h>
#ifdef AUDIO_UPDATE
#include <pspaudio.h>
#endif
#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "audio.h"
#include "gensound.h"
#include "sounddep/sound.h"

//sound stuff
#ifdef AUDIO_UPDATE
uae_u16 sndbuffer[NUMSAMPLESINBUFFER*4]; //channel 0
#else
uae_u16 sndbuffer[DEFAULT_SOUND_FREQ*2]; //channel 0
uae_u16 sndbuffer1[DEFAULT_SOUND_FREQ*2]; // channel 1 only used for quad
#endif

uae_u16 *sndbufpt;
int sndbufsize;

int init_sound (void)
{
#ifdef CYCLE_SWITCH
	if (g_cputurbo == 512)
	{
			sample_evtime = 82226.4;
	}
	else
	if (g_cputurbo == 256)
	{
			sample_evtime = 41113.2;
	}
#else
	sample_evtime = 82226.4/*41113.2*/;
#endif

	if (currprefs.sound_bits == 16) {
		init_sound_table16 ();
		switch (currprefs.sound_stereo)
		{
		default:
		case 0:  // stereo
			sample_handler = sample16s_handler;
			//sceAudioChangeChannelConfig(0, PSP_AUDIO_FORMAT_MONO);
		break;
		}
	}

	sound_available = 1;
#ifdef AUDIO_UPDATE
	sndbufsize= NUMSAMPLESINBUFFER*4;
#else
	sndbufsize = 8192*4;
#endif
	sndbufpt = sndbuffer;
	return 1;
}

int setup_sound (void)
{
  sound_available = 1;
  return 1;
}

void close_sound(void)
{
}

static int lastfreq;

void update_sound (int freq)
{
#if 0
#ifdef CYCLE_SWITCH
	if (g_cputurbo == 512)
	{
		scaled_sample_evtime = (unsigned long)(82226.4);
	}
	else
	if (g_cputurbo == 256)
	{
		scaled_sample_evtime = (unsigned long)(41113.2);
	}
#else
	scaled_sample_evtime = (unsigned long)(82226.4/*41113.2*/);
#endif
#endif
    if (freq < 0)
	freq = lastfreq;
    lastfreq = freq;

    int obtainedfreq = currprefs.sound_freq;

    //if (currprefs.ntscmode)
	//scaled_sample_evtime = (unsigned long)(MAXHPOS_NTSC * MAXVPOS_NTSC * freq * CYCLE_UNIT + obtainedfreq - 1) / obtainedfreq;
    //else
	scaled_sample_evtime = (unsigned long)(MAXHPOS_PAL * MAXVPOS_PAL * freq * CYCLE_UNIT + obtainedfreq - 1) / obtainedfreq;

}
void reset_sound (void)
{
}

void pause_sound (void)
{
}

void resume_sound (void)
{
}

void sound_volume (int dir)
{
}

void finish_sound_buffer (void)
{
}

/*
 * Handle audio specific cfgfile options
 */
void audio_default_options (struct uae_prefs *p)
{
}

void audio_save_options (FILE *f, const struct uae_prefs *p)
{
}

int audio_parse_option (struct uae_prefs *p, const char *option, const char *value)
{
    return 0;
}
#ifdef AUDIO_UPDATE
#ifndef NO_SOUND_THREADS
/* This function gets called by pspaudiolib every time the
   audio buffer needs to be filled. - channel 0 */
void sound_callback(void *buf, unsigned int numsamples, void *pdata)
{
    if (!currprefs.produce_sound) return;

	//stereo & mono - compensation is managed in audio.c
	while(numsamples << 2 > ((int)sndbufpt-(int)(&sndbuffer)))
	{
#ifdef SOUND_DELAY
			sceKernelDelayThread(g_snddelay);
#else
			sceKernelDelayThread(1000);
#endif
	}
	int i;
	short *dst=buf;
	short *src=(short *)sndbuffer;

	// each sample contains a left channel and a right channel word
	for(i=0;i<numsamples << 1;i++)
		dst[i]=src[i];
	sndbufpt = (uae_u16*)src;
}

#endif
#else
/* This function gets called by pspaudiolib every time the
   audio buffer needs to be filled. - channel 0 */
void sound_callback(void *buf, unsigned int numsamples, void *pdata)
{
    if (!currprefs.produce_sound) return;

	int i,j=0;
	short *dst=buf;
	short *src=(short *)sndbuffer;

	if (changed_prefs.sound_stereo == 0)
	{
		//mono
		while(numsamples>((int)sndbufpt-(int)(&sndbuffer)))
		{
#ifdef SOUND_DELAY
			sceKernelDelayThread(g_snddelay);
#else
			sceKernelDelayThread(1000);
#endif
		}
		// each sample contains a left channel and a right channel word
		for(i=numsamples;i--;)
		{
			short a=src[i/2];
			dst[i*2]=a;
			dst[i*2+1]=a;
		}
	}
	else
	{
		//stereo
		while(numsamples*2>((int)sndbufpt-(int)(&sndbuffer)))
		{
#ifdef SOUND_DELAY
			sceKernelDelayThread(g_snddelay);
#else
			sceKernelDelayThread(1000);
#endif
		}

		// each sample contains a left channel and a right channel word
		for(i=0;i<numsamples*2;i+=4)
		{
			dst[i]=src[j];
			dst[i+1]=src[j+1];
			dst[i+2]=src[j];
			dst[i+3]=src[j+1];
			j+=2;
		}
	}
	sndbufpt = (uae_u16*)src;
}

/* This function gets called by pspaudiolib every time the
   audio buffer needs to be filled. - channel 1
   only used for 4 channel */
void sound_callback1(void *buf, unsigned int numsamples, void *pdata)
{
    if (!currprefs.produce_sound) return;

	while(numsamples*2>((int)sndbufpt1-(int)(&sndbuffer1)))
	{
#ifdef SOUND_DELAY
			sceKernelDelayThread(g_snddelay);
#else
			sceKernelDelayThread(1000);
#endif
	}
	int i,j=0;
	short *dst=buf;
	short *src=(short *)sndbuffer1;

	for(i=0;i<numsamples*2;i+=4)
	{
		dst[i]=src[j];
		dst[i+1]=src[j+1];
		dst[i+2]=src[j];
		dst[i+3]=src[j+1];
		j+=2;
	}
	sndbufpt1 = (uae_u16*)src;
}
#endif
