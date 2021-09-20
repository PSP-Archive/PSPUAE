 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Sound emulation stuff
  *
  * Copyright 1995, 1996, 1997 Bernd Schmidt
  */

#define PERIOD_MAX ULONG_MAX

extern void aud0_handler (void);
extern void aud1_handler (void);
extern void aud2_handler (void);
extern void aud3_handler (void);

extern void AUDxDAT (unsigned int nr, uae_u16 value);
extern void AUDxVOL (unsigned int nr, uae_u16 value);
extern void AUDxPER (unsigned int nr, uae_u16 value);
extern void AUDxLCH (unsigned int nr, uae_u16 value);
extern void AUDxLCL (unsigned int nr, uae_u16 value);
extern void AUDxLEN (unsigned int nr, uae_u16 value);

extern int  init_audio (void);
extern void ahi_install (void);
extern void audio_reset (void);
//ric perf extern void update_audio (void);
extern void update_audio_extern (void);
STATIC_INLINE void update_audio(void);
STATIC_INLINE void schedule_audio (void);
//end ric perf
extern void audio_evhandler (void);

#ifdef AUDIO_UPDATE
extern void audio_hsync (unsigned int);
#else
extern void audio_hsync (int);
#endif

extern void audio_update_adkmasks (void);
extern void audio_update_irq (uae_u16);
extern void update_sound (int freq);
