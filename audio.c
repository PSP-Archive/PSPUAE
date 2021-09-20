 /*
  * UAE - The Un*x Amiga Emulator
  *
  * OS specific functions
  *
  * Copyright 1995, 1996, 1997 Bernd Schmidt
  * Copyright 1996 Marcus Sundberg
  * Copyright 1996 Manfred Thole
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "custom_private.h"
#include "newcpu.h"
#include "gensound.h"
#include "sounddep/sound.h"
#include "events.h"
#include "audio.h"
#include "savestate.h"
#include "driveclick.h"
#ifdef AVIOUTPUT
#include "avioutput.h"
#endif

//extern int g_audio_opt;

#define MAX_EV ~0ul

int audio_channel_mask = 15;

struct audio_channel_data {
    unsigned long adk_mask;
    unsigned long evtime;
    uae_u8 dmaen, intreq2;
    uaecptr lc, pt;
    int current_sample, last_sample;
#ifndef MULTIPLICATION_PROFITABLE
    int *voltbl;
#endif
    int state;
    unsigned long per;
    int vol;
    int len, wlen;
    uae_u16 dat, dat2;
    int request_word, request_word_skip;
};

STATIC_INLINE unsigned int current_hpos (void)
{
    return (get_cycles () - eventtab[ev_hsync].oldcycles) / CYCLE_UNIT;
}

static struct audio_channel_data audio_channel[4];
int sound_available = 0;
#ifndef MULTIPLICATION_PROFITABLE
static int sound_table[64][256];
#endif
void (*sample_handler) (void);

unsigned long sample_evtime, scaled_sample_evtime;

static unsigned long last_cycles, next_sample_evtime;

#ifndef MULTIPLICATION_PROFITABLE
void init_sound_table16 (void)
{
    int i,j;

    for (i = 0; i < 256; i++)
	for (j = 0; j < 64; j++)
	    sound_table[j][i] = j * (uae_s8)i * (currprefs.sound_stereo ? 2 : 1);
}
#endif

#ifdef MULTIPLICATION_PROFITABLE
typedef uae_s8 sample8_t;
#define DO_CHANNEL_1(v, c) do { (v) *= audio_channel[c].vol; } while (0)
#define SBASEVAL8(logn) ((logn) == 1 ? SOUND8_BASE_VAL << 7 : SOUND8_BASE_VAL << 8)
#define SBASEVAL16(logn) ((logn) == 1 ? SOUND16_BASE_VAL >> 1 : SOUND16_BASE_VAL)
#define FINISH_DATA(data,b,logn) do { if (14 - (b) + (logn) > 0) (data) >>= 14 - (b) + (logn); else (data) <<= (b) - 14 - (logn); } while (0);
#else
typedef uae_u8 sample8_t;
#define DO_CHANNEL_1(v, c) do { (v) = audio_channel[c].voltbl[(v)]; } while (0)
#define SBASEVAL8(logn) SOUND8_BASE_VAL
#define SBASEVAL16(logn) SOUND16_BASE_VAL
#define FINISH_DATA(data,b,logn)
#endif

/* Always put the right word before the left word.  */
#define MAX_DELAY_BUFFER 1024 //
static uae_u32 right_word_saved[MAX_DELAY_BUFFER];
static uae_u32 left_word_saved[MAX_DELAY_BUFFER];
static int saved_ptr;

#define MIXED_STEREO_SCALE 32 //FOL
#define MIXED_STEREO_MAX 16
static int mixed_on, mixed_stereo_size, mixed_mul1, mixed_mul2;

#define DO_CHANNEL(v, c) do { (v) &= audio_channel[c].adk_mask; data += v; } while (0);

void sample16_handler (void)
{
}

void sample16i_rh_handler (void)
{
}

void sample16i_crux_handler (void)
{
}

#ifdef HAVE_STEREO_SUPPORT

void sample16s_handler (void)
{
    uae_u32 data0 = audio_channel[0].current_sample;
    uae_u32 data1 = audio_channel[1].current_sample;
    uae_u32 data2 = audio_channel[2].current_sample;
    uae_u32 data3 = audio_channel[3].current_sample;
    DO_CHANNEL_1 (data0, 0);
    DO_CHANNEL_1 (data1, 1);
    DO_CHANNEL_1 (data2, 2);
    DO_CHANNEL_1 (data3, 3);

    data0 &= audio_channel[0].adk_mask;
    data1 &= audio_channel[1].adk_mask;
    data2 &= audio_channel[2].adk_mask;
    data3 &= audio_channel[3].adk_mask;

    data0 += data3;
    data1 += data2;

	data0 = SBASEVAL16(1) + data0;
	FINISH_DATA (data0, 16, 1);
	data1 = SBASEVAL16(1) + data1;
	FINISH_DATA (data1, 16, 1);
	PUT_SOUND_WORD_LEFT (data0);
	PUT_SOUND_WORD_RIGHT (data1);
}
#endif

STATIC_INLINE schedule_audio (void)
{
    unsigned long best = MAX_EV;
    int i;

    eventtab[ev_audio].active = 0;
    eventtab[ev_audio].oldcycles = get_cycles ();
    for (i = 0; i < 4; i++) {
	struct audio_channel_data *cdp = audio_channel + i;
	if (cdp->evtime != MAX_EV) {
	    if (best > cdp->evtime) {
		best = cdp->evtime;
		eventtab[ev_audio].active = 1;
	    }
	}
    }
    eventtab[ev_audio].evtime = get_cycles () + best;
}

static int isirq (unsigned int nr)
{
    return INTREQR () & (0x80 << nr);
}

#ifdef AUDIO_UPDATE
static void setirq (unsigned int nr, unsigned int debug)
#else
static void setirq (unsigned int nr)
#endif
{
#ifdef DEBUG_AUDIO
    if (debugchannel (nr))
	write_log ("SETIRQ %d %08.8X (%d)\n", nr, m68k_getpc (&regs), debug);
#endif
    INTREQ (0x8000 | (0x80 << nr));
}

static void newsample (unsigned int nr, sample8_t sample)
{
    struct audio_channel_data *cdp = audio_channel + nr;
#ifdef DEBUG_AUDIO
    if (!debugchannel (nr)) sample = 0;
#endif
    if (!(audio_channel_mask & (1 << nr)))
	sample = 0;
    cdp->last_sample = cdp->current_sample;
    cdp->current_sample = sample;
}

static void state23 (struct audio_channel_data *cdp)
{
    if (!cdp->dmaen)
	{
	return;
	}
    if (cdp->request_word >= 0)
	{
	return;
	}
    cdp->request_word = 0;
    if (cdp->wlen == 1) {
	cdp->wlen = cdp->len;
	cdp->pt = cdp->lc;
	cdp->intreq2 = 1;

#ifdef DEBUG_AUDIO
	if (debugchannel (cdp - audio_channel))
	    write_log ("Channel %d looped, LC=%08.8X LEN=%d\n", cdp - audio_channel, cdp->pt, cdp->wlen);
#endif
    } else {
	cdp->wlen = (cdp->wlen - 1) & 0xFFFF;
    }
}

static void audio_handler (unsigned int nr, int timed)
{
    struct audio_channel_data *cdp = audio_channel + nr;
#ifdef AUDIO_UPDATE

#else
   unsigned int audav = adkcon & (0x01 << nr);
   unsigned int audap = adkcon & (0x10 << nr);
   unsigned int napnav = (!audav && !audap) || audav;
#endif
	unsigned long evtime = cdp->evtime;

    cdp->evtime = MAX_EV;
    switch (cdp->state)
    {
	case 0:
	    cdp->request_word = 0;
	    cdp->request_word_skip = 0;
	    cdp->intreq2 = 0;
	    if (cdp->dmaen) {
		cdp->state = 1;
		cdp->wlen = cdp->len;
		/* there are too many stupid sound routines that fail on "too" fast cpus.. */
		if (currprefs.cpu_level > 1)
		    cdp->pt = cdp->lc;
#ifdef DEBUG_AUDIO
		if (debugchannel (nr))
		    write_log ("%d:0>1: LEN=%d\n", nr, cdp->wlen);
#endif
		audio_handler (nr, timed);
		return;
	    } else if (!cdp->dmaen && cdp->request_word < 0 && !isirq (nr)) {
		cdp->evtime = 0;
		cdp->state = 2;
#ifdef AUDIO_UPDATE
		setirq (nr, 0);
#else
		setirq (nr);
#endif
		audio_handler (nr, timed);
		return;
	    }
	return;

	case 1:
	    if (!cdp->dmaen) {
		cdp->state = 0;
		return;
	    }
	    cdp->state = 5;
	    if (cdp->wlen != 1)
		cdp->wlen = (cdp->wlen - 1) & 0xFFFF;
	    cdp->request_word = 2;
	    if (current_hpos () > maxhpos - 20)
		cdp->request_word_skip = 1;
	return;

	case 5:
	    if (!cdp->request_word) {
			cdp->request_word = 2;
			return;
	    }
#ifdef AUDIO_UPDATE
		setirq (nr, 1);
#else
		setirq (nr);
#endif
		if (!cdp->dmaen) {
			cdp->state = 0;
			cdp->request_word = 0;
			return;
	    }
	    cdp->state = 2;
	    cdp->request_word = 3;
#ifdef AUDIO_UPDATE
		{
			unsigned int audav = adkcon & (0x01 << nr);
			unsigned int audap = adkcon & (0x10 << nr);
			unsigned int napnav = (!audav && !audap) || audav;
#endif
			if (napnav)
				cdp->request_word = 2;
			cdp->dat = cdp->dat2;
			return;
#ifdef AUDIO_UPDATE
		}
#endif
	case 2:
	    if (currprefs.produce_sound == 0)
		cdp->per = PERIOD_MAX;

	    if (!cdp->dmaen && isirq (nr) && ((cdp->per <= 30 ) || (evtime == 0 || evtime == MAX_EV || evtime == cdp->per))) {
		cdp->state = 0;
		cdp->evtime = MAX_EV;
		cdp->request_word = 0;
		return;
	    }

	    state23 (cdp);
	    cdp->state = 3;
	    cdp->evtime = cdp->per;
	    newsample (nr, (cdp->dat >> 8) & 0xff);
	    cdp->dat <<= 8;
	    /* Period attachment? */
#ifdef AUDIO_UPDATE
		{
			unsigned int audap = adkcon & (0x10 << nr);
			if (audap) {
				if (cdp->intreq2 && cdp->dmaen)
					setirq (nr, 2);
				cdp->intreq2 = 0;
				cdp->request_word = 1;
				cdp->dat = cdp->dat2;
				if (nr < 3) {
					if (cdp->dat == 0)
						(cdp+1)->per = PERIOD_MAX;
					else if (cdp->dat < maxhpos * CYCLE_UNIT / 2 && currprefs.produce_sound < 3)
						(cdp+1)->per = maxhpos * CYCLE_UNIT / 2;
					else
						(cdp+1)->per = cdp->dat * CYCLE_UNIT;
				}
			}
		}
#else
		if (audap) {
		if (cdp->intreq2 && cdp->dmaen)
		    setirq (nr/*, 2*/);
		cdp->intreq2 = 0;
		cdp->request_word = 1;
		cdp->dat = cdp->dat2;
		if (nr < 3) {
		    if (cdp->dat == 0)
			(cdp+1)->per = PERIOD_MAX;
		    else if (cdp->dat < maxhpos * CYCLE_UNIT / 2 && currprefs.produce_sound < 3)
			(cdp+1)->per = maxhpos * CYCLE_UNIT / 2;
		    else
			(cdp+1)->per = cdp->dat * CYCLE_UNIT;
		}
	    }
#endif
	return;

	case 3:
	    if (currprefs.produce_sound == 0)
		cdp->per = PERIOD_MAX;
	    state23 (cdp);
	    cdp->state = 2;
	    cdp->evtime = cdp->per;
	    newsample (nr, (cdp->dat >> 8) & 0xff);
	    cdp->dat <<= 8;
	    cdp->dat = cdp->dat2;
#ifdef AUDIO_UPDATE
		{
			unsigned int audav = adkcon & (0x01 << nr);
			unsigned int audap = adkcon & (0x10 << nr);
			unsigned int napnav = (!audav && !audap) || audav;
			if (cdp->dmaen) {
				if (napnav)
					cdp->request_word = 1;
				if (cdp->intreq2 && napnav)
					setirq (nr, 3);
			} else {
				if (napnav)
					setirq (nr, 4);
			}
			cdp->intreq2 = 0;

			/* Volume attachment? */
			if (audav) {
				if (nr < 3) {
					(cdp+1)->vol = cdp->dat;
#ifndef MULTIPLICATION_PROFITABLE
		    		(cdp+1)->voltbl = sound_table[cdp->dat];
#endif
				}
			}
		}
#else
		if (cdp->dmaen) {
		if (napnav)
		    cdp->request_word = 1;
		if (cdp->intreq2 && napnav)
		    setirq (nr/*, 3*/);
	    } else {
		if (napnav)
		    setirq (nr/*, 4*/);
	    }
	    cdp->intreq2 = 0;

	    /* Volume attachment? */
	    if (audav) {
		if (nr < 3) {
		    (cdp+1)->vol = cdp->dat;
#ifndef MULTIPLICATION_PROFITABLE
		    (cdp+1)->voltbl = sound_table[cdp->dat];
#endif
		}
	    }
#endif
		return;
    }
}

void audio_reset (void)
{
    int i;
    struct audio_channel_data *cdp;

#ifdef AHI
    ahi_close_sound ();
#endif
    reset_sound ();
    if (savestate_state != STATE_RESTORE) {
	for (i = 0; i < 4; i++) {
	    cdp = &audio_channel[i];
	    memset (cdp, 0, sizeof *audio_channel);
	    cdp->per = PERIOD_MAX - 1;
#ifndef MULTIPLICATION_PROFITABLE
	    cdp->voltbl = sound_table[0];
#endif
	    cdp->vol = 0;
	    cdp->evtime = MAX_EV;
	}
    } else {
	for (i = 0; i < 4; i++) {
	    cdp = &audio_channel[i];
	    cdp->dmaen = (dmacon & DMA_MASTER) && (dmacon & (1 << i));
	}
    }

#ifndef	MULTIPLICATION_PROFITABLE
    for (i = 0; i < 4; i++)
	audio_channel[i].voltbl = sound_table[audio_channel[i].vol];
#endif

    last_cycles = get_cycles ();
    next_sample_evtime = scaled_sample_evtime;
    schedule_audio ();
    events_schedule ();
}

STATIC_INLINE int sound_prefs_changed (void)
{
    return (changed_prefs.produce_sound != currprefs.produce_sound
	    || changed_prefs.sound_stereo != currprefs.sound_stereo
	    || changed_prefs.sound_stereo_separation != currprefs.sound_stereo_separation
	    || changed_prefs.sound_mixed_stereo != currprefs.sound_mixed_stereo
	    || changed_prefs.sound_latency != currprefs.sound_latency
	    || changed_prefs.sound_freq != currprefs.sound_freq
	    || changed_prefs.sound_adjust != currprefs.sound_adjust
	    || changed_prefs.sound_interpol != currprefs.sound_interpol
	    || changed_prefs.sound_volume != currprefs.sound_volume);
}

void check_prefs_changed_audio (void)
{
    if (sound_available && sound_prefs_changed ()) {
	close_sound ();

	currprefs.produce_sound = changed_prefs.produce_sound;
	currprefs.sound_stereo = changed_prefs.sound_stereo;
	currprefs.sound_stereo_separation = changed_prefs.sound_stereo_separation;
	currprefs.sound_mixed_stereo = changed_prefs.sound_mixed_stereo;
	currprefs.sound_adjust = changed_prefs.sound_adjust;
	currprefs.sound_interpol = changed_prefs.sound_interpol;
	currprefs.sound_freq = changed_prefs.sound_freq;
	currprefs.sound_latency = changed_prefs.sound_latency;
	currprefs.sound_volume = changed_prefs.sound_volume;
	if (currprefs.produce_sound >= 2) {
	    if (!init_audio ()) {
		if (! sound_available) {
		    write_log ("Sound is not supported.\n");
		} else {
		    write_log ("Sorry, can't initialize sound.\n");
		    currprefs.produce_sound = 0;
		    /* So we don't do this every frame */
		    changed_prefs.produce_sound = 0;
		}
	    }
	}
	last_cycles = get_cycles () - 1;
	next_sample_evtime = scaled_sample_evtime;
	compute_vsynctime ();
    }
    mixed_mul1 = MIXED_STEREO_MAX / 2 - ((currprefs.sound_stereo_separation * 3) / 2);
    mixed_mul2 = MIXED_STEREO_MAX / 2 + ((currprefs.sound_stereo_separation * 3) / 2);
    mixed_stereo_size = currprefs.sound_mixed_stereo > 0 ? (1 << (currprefs.sound_mixed_stereo - 1)) - 1 : 0;
    mixed_on = (currprefs.sound_stereo_separation > 0 || currprefs.sound_mixed_stereo > 0) ? 1 : 0;

	if (currprefs.produce_sound == 0) {
	eventtab[ev_audio].active = 0;
	events_schedule ();
    }
}

void update_audio_extern (void)
{
	update_audio();
}

STATIC_INLINE void update_audio (void)
{
    unsigned long int n_cycles;

    if (currprefs.produce_sound == 0 || savestate_state == STATE_RESTORE)
	{
	return;
	}
    n_cycles = get_cycles () - last_cycles;
    for (;;) {
	register unsigned long int best_evtime = n_cycles + 1;

	if (audio_channel[0].evtime != MAX_EV && best_evtime > audio_channel[0].evtime)
	    best_evtime = audio_channel[0].evtime;
	if (audio_channel[1].evtime != MAX_EV && best_evtime > audio_channel[1].evtime)
	    best_evtime = audio_channel[1].evtime;
	if (audio_channel[2].evtime != MAX_EV && best_evtime > audio_channel[2].evtime)
	    best_evtime = audio_channel[2].evtime;
	if (audio_channel[3].evtime != MAX_EV && best_evtime > audio_channel[3].evtime)
	    best_evtime = audio_channel[3].evtime;

#ifdef AUDIO_UPDATE
	if (currprefs.produce_sound > 1 && best_evtime > next_sample_evtime)
#else
	if (best_evtime > next_sample_evtime)
#endif
		best_evtime = next_sample_evtime;

	if (best_evtime > n_cycles)
	    break;

	    audio_channel[0].evtime -= best_evtime;
	    audio_channel[1].evtime -= best_evtime;
	    audio_channel[2].evtime -= best_evtime;
	    audio_channel[3].evtime -= best_evtime;

	n_cycles -= best_evtime;
#ifdef INTERRUPT_ON
	if (currprefs.produce_sound > 1) {
#endif
		next_sample_evtime -= best_evtime;
		if (next_sample_evtime == 0) {
		next_sample_evtime = scaled_sample_evtime;
		(*sample_handler) ();
#ifdef INTERRUPT_ON
	    }
#endif
	}
	if (audio_channel[0].evtime == 0)
	    audio_handler (0, 1);
	if (audio_channel[1].evtime == 0)
	    audio_handler (1, 1);
	if (audio_channel[2].evtime == 0)
	    audio_handler (2, 1);
	if (audio_channel[3].evtime == 0)
	    audio_handler (3, 1);
    }
    last_cycles = get_cycles () - n_cycles;
}

void audio_evhandler (void)
{
    schedule_audio ();
	update_audio ();
}

#ifdef CPUEMU_6
extern uae_u8 cycle_line[];
#endif
uae_u16	dmacon;

#ifdef AUDIO_UPDATE
void audio_hsync (unsigned int dmaaction)
#else
void audio_hsync (int dmaaction)
#endif
{
    unsigned int nr, handle;

    if (currprefs.produce_sound == 0)
	{
	return;
	}
//	if (!g_audio_opt)
//	{
	update_audio ();
//	}
	handle = 0;
    /* Sound data is fetched at the beginning of each line */
#ifdef AUDIO_UPDATE
	for( nr=4; nr--; ) {
	struct audio_channel_data *cdp = audio_channel + nr;
	register unsigned int chan_ena = (dmacon & DMA_MASTER) && (dmacon & (1 << nr));
	register unsigned int handle2 = 0;
#else
	for (nr = 0; nr < 4; nr++) {
	struct audio_channel_data *cdp = audio_channel + nr;
	unsigned int chan_ena = (dmacon & DMA_MASTER) && (dmacon & (1 << nr));
	unsigned int handle2 = 0;
#endif

	if (dmaaction && cdp->request_word > 0) {

	    if (cdp->request_word_skip) {
		cdp->request_word_skip = 0;
		continue;
	    }

	    if (cdp->state == 5) {
		cdp->pt = cdp->lc;
#ifdef DEBUG_AUDIO
		if (debugchannel (nr))
		    write_log ("%d:>5: LEN=%d PT=%08.8X\n", nr, cdp->wlen, cdp->pt);
#endif
	    }
	    cdp->dat2 = chipmem_wget (cdp->pt);
	    if (cdp->request_word >= 2)
		handle2 = 1;
	    if (chan_ena) {
#ifdef CPUEMU_6
		cycle_line[13 + nr * 2] |= CYCLE_MISC;
#endif
		if (cdp->request_word == 1 || cdp->request_word == 2)
		    cdp->pt += 2;
	    }
	    cdp->request_word = -1;

	}

	if (cdp->dmaen != chan_ena) {
#ifdef DEBUG_AUDIO
	    if (debugchannel (nr))
		write_log ("AUD%dDMA %d->%d (%d) LEN=%d/%d %08.8X\n", nr, cdp->dmaen, chan_ena,
		    cdp->state, cdp->wlen, cdp->len, m68k_getpc());
#endif
	    cdp->dmaen = chan_ena;
	    if (cdp->dmaen)
		handle2 = 1;
	}
	if (handle2)
	    audio_handler (nr, 0);
	handle |= handle2;
    }
    if (handle) {
	schedule_audio ();
	events_schedule ();
    }
}

void AUDxDAT (unsigned int nr, uae_u16 v)
{
    struct audio_channel_data *cdp = audio_channel + nr;

#ifdef DEBUG_AUDIO
    if (debugchannel (nr))
	write_log ("AUD%dDAT: %04.4X STATE=%d IRQ=%d %08.8X\n", nr,
	    v, cdp->state, isirq(nr) ? 1 : 0, m68k_getpc (&regs));
#endif
#ifdef AUDIO_UPDATE

#else
	update_audio ();
#endif
	cdp->dat2 = v;
    cdp->request_word = -1;
    cdp->request_word_skip = 0;
    if (cdp->state == 0) {
	cdp->state = 2;
	audio_handler (nr, 0);
	schedule_audio ();
	events_schedule ();
    }
}

void AUDxLCH (unsigned int nr, uae_u16 v)
{
#ifdef AUDIO_UPDATE

#else
	update_audio ();
#endif
	audio_channel[nr].lc = (audio_channel[nr].lc & 0xffff) | ((uae_u32)v << 16);
#ifdef DEBUG_AUDIO
    if (debugchannel (nr))
	write_log ("AUD%dLCH: %04.4X %08.8X\n", nr, v, m68k_getpc (&regs));
#endif
}

void AUDxLCL (unsigned int nr, uae_u16 v)
{
#ifdef AUDIO_UPDATE

#else
	update_audio ();
#endif
	audio_channel[nr].lc = (audio_channel[nr].lc & ~0xffff) | (v & 0xFFFE);
#ifdef DEBUG_AUDIO
    if (debugchannel (nr))
	write_log ("AUD%dLCL: %04.4X %08.8X\n", nr, v, m68k_getpc (&regs));
#endif
}

void AUDxPER (unsigned int nr, uae_u16 v)
{
    unsigned long per = v * CYCLE_UNIT;
#ifdef AUDIO_UPDATE

#else
	update_audio ();
#endif

    if (per == 0)
	per = PERIOD_MAX - 1;

    if (per < maxhpos * CYCLE_UNIT / 2 && currprefs.produce_sound < 3)
	per = maxhpos * CYCLE_UNIT / 2;

   if (audio_channel[nr].per == PERIOD_MAX - 1 && per != PERIOD_MAX - 1) {
	audio_channel[nr].evtime = CYCLE_UNIT;
	if (currprefs.produce_sound > 0) {
	    schedule_audio ();
	    events_schedule ();
	}
    }

    audio_channel[nr].per = per;
#ifdef DEBUG_AUDIO
    if (debugchannel (nr))
	write_log ("AUD%dPER: %d %08.8X\n", nr, v, m68k_getpc (&regs));
#endif
}

void AUDxLEN (unsigned int nr, uae_u16 v)
{
#ifdef AUDIO_UPDATE

#else
	update_audio ();
#endif
    audio_channel[nr].len = v;
#ifdef DEBUG_AUDIO
    if (debugchannel (nr))
	write_log ("AUD%dLEN: %d %08.8X\n", nr, v, m68k_getpc (&regs));
#endif
}

void AUDxVOL (unsigned int nr, uae_u16 v)
{
    unsigned int v2 = v & 64 ? 63 : v & 63;
#ifdef AUDIO_UPDATE

#else
	update_audio ();
#endif
    audio_channel[nr].vol = v2;
#ifndef MULTIPLICATION_PROFITABLE
    audio_channel[nr].voltbl = sound_table[v2];
#endif
#ifdef DEBUG_AUDIO
    if (debugchannel (nr))
	write_log ("AUD%dVOL: %d %08.8X\n", nr, v2, m68k_getpc (&regs));
#endif
}

void audio_update_irq (uae_u16 v)
{
#ifdef DEBUG_AUDIO
    uae_u16 v2 = intreq, v3 = intreq;
    unsigned int i;
    if (v & 0x8000)
	v2 |= v & 0x7FFF;
    else
	v2 &= ~v;
    v2 &= (0x80 | 0x100 | 0x200 | 0x400);
    v3 &= (0x80 | 0x100 | 0x200 | 0x400);
    for (i = 0; i < 4; i++) {
	if ((1 << i) & DEBUG_CHANNEL_MASK) {
	    uae_u16 mask = 0x80 << i;
	    if ((v2 & mask) != (v3 & mask))
		write_log ("AUD%dINTREQ %d->%d %08.8X\n", i, !!(v3 & mask), !!(v2 & mask), m68k_getpc (&regs));
	}
    }
#endif
}

void audio_update_adkmasks (void)
{
    unsigned long t = adkcon | (adkcon >> 4);
    audio_channel[0].adk_mask = (((t >> 0) & 1) - 1);
    audio_channel[1].adk_mask = (((t >> 1) & 1) - 1);
    audio_channel[2].adk_mask = (((t >> 2) & 1) - 1);
    audio_channel[3].adk_mask = (((t >> 3) & 1) - 1);
#ifdef DEBUG_AUDIO
    {
	static int prevcon = -1;
	if ((prevcon & 0xff) != (adkcon & 0xff)) {
	    write_log ("ADKCON=%02.2x %08.8X\n", adkcon & 0xff, m68k_getpc (&regs));
	    prevcon = adkcon;
	}
    }
#endif
}

int init_audio (void)
{
    return init_sound ();
}


#ifdef SAVESTATE

const uae_u8 *restore_audio (unsigned int channel, const uae_u8 *src)
{
    struct audio_channel_data *acd = &audio_channel[channel];
    uae_u16 p;

    acd->state = restore_u8 ();
    acd->vol = restore_u8 ();
    acd->intreq2 = restore_u8 ();
    acd->request_word = restore_u8 ();
    acd->len = restore_u16 ();
    acd->wlen = restore_u16 ();
    p = restore_u16 ();
    acd->per = p ? (unsigned)p * CYCLE_UNIT : (unsigned)PERIOD_MAX;
    p = restore_u16 ();
    acd->lc = restore_u32 ();
    acd->pt = restore_u32 ();
    acd->evtime = restore_u32 ();
    return src;
}

#endif /* SAVESTATE */

#if defined SAVESTATE || defined DEBUGGER

uae_u8 *save_audio (unsigned int channel, uae_u32 *len, uae_u8 *dstptr)
{
    const struct audio_channel_data *acd = &audio_channel[channel];
    uae_u8 *dst, *dstbak;
    uae_u16 p;

    if (dstptr)
	dstbak = dst = dstptr;
    else
	dstbak = dst = malloc (100);

    save_u8 ((uae_u8)acd->state);
    save_u8 (acd->vol);
    save_u8 (acd->intreq2);
    save_u8 (acd->request_word);
    save_u16 (acd->len);
    save_u16 (acd->wlen);
    p = acd->per == PERIOD_MAX ? 0 : acd->per / CYCLE_UNIT;
    save_u16 (p);
    save_u16 (acd->dat2);
    save_u32 (acd->lc);
    save_u32 (acd->pt);
    save_u32 (acd->evtime);
    *len = dst - dstbak;
    return dstbak;
}

#endif /* SAVESTATE || DEBUGGER */
