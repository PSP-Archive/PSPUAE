 /* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Support for the Mute sound system.
  * 
  * Copyright 1997 Bernd Schmidt
  */

#ifdef AUDIO_UPDATE
#include <pspaudio.h>
#endif

#ifdef SOUND_DELAY
extern int g_snddelay;
#endif

extern uae_u16 sndbuffer[];
extern uae_u16 *sndbufpt;

extern int sndbufsize; 
extern void finish_sound_buffer (void); 

static __inline__ void check_sound_buffers (void)
{
    if ((char *)sndbufpt - (char *)sndbuffer >= sndbufsize) {
		finish_sound_buffer ();
		sndbufpt = sndbuffer;
	} 
}
#define AUDIO_NAME "pspaudio"

#define PUT_SOUND_BYTE(b) do { *(uae_u8 *)sndbufpt = b; sndbufpt = (uae_u16 *)(((uae_u8 *)sndbufpt) + 1); } while (0)
#ifdef AUDIO_UPDATE

#else
#define PUT_SOUND_WORD(b) do { *(uae_u16 *)sndbufpt = b; sndbufpt = (uae_u16 *)(((uae_u8 *)sndbufpt) + 2); } while (0)
#endif
#define PUT_SOUND_BYTE_LEFT(b) PUT_SOUND_BYTE(b)
#define PUT_SOUND_WORD_LEFT(b) PUT_SOUND_WORD(b)
#define PUT_SOUND_BYTE_RIGHT(b) PUT_SOUND_BYTE(b)
#define PUT_SOUND_WORD_RIGHT(b) PUT_SOUND_WORD(b)
#define PUT_SOUND_WORD_MONO(b) PUT_SOUND_WORD(b)

#ifdef AUDIO_UPDATE
#define PUT_SOUND_WORD(b) *sndbufpt++ = b;
#endif


#define SOUND16_BASE_VAL 0
#define SOUND8_BASE_VAL 128 

#define DEFAULT_SOUND_BITS 16

#ifdef AUDIO_UPDATE
#define DEFAULT_SOUND_FREQ 44100
#define NUMSAMPLESINBUFFER 4096
#else
#define DEFAULT_SOUND_FREQ 22050
#endif

//FOL
#ifdef AUDIO_LATENCY
#define DEFAULT_SOUND_LATENCY 200
#else
#define DEFAULT_SOUND_LATENCY 0
#endif
//FOL
#define HAVE_STEREO_SUPPORT

