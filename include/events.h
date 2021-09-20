#ifndef EVENTS_H
#define EVENTS_H

 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Events
  * These are best for low-frequency events. Having too many of them,
  * or using them for events that occur too frequently, can cause massive
  * slowdown.
  *
  * Copyright 1995-1998 Bernd Schmidt
  */

#undef EVENT_DEBUG

#include "machdep/rpt.h"
#include "hrtimer.h"

//ric
#include "profiling.h"
#define PROFILINGINDEX 1000
#undef PROFILING
//end ric

extern volatile frame_time_t vsynctime, vsyncmintime;
extern void reset_frame_rate_hack (void);
extern int rpt_available;
extern frame_time_t syncbase;

extern void compute_vsynctime (void);
extern void do_cycles_ce (long cycles);


extern unsigned long currcycle;
extern unsigned int is_lastline;

extern unsigned long nextevent;

typedef void (*evfunc)(void);

struct ev
{
    int active;
    unsigned long int evtime, oldcycles;
    evfunc handler;
};

enum {
    ev_hsync, ev_copper, ev_audio, ev_cia, ev_blitter, ev_disk,
    ev_max
};

extern struct ev eventtab[ev_max];

extern void init_eventtab (void);
extern void events_schedule (void);
extern void handle_active_events (void);

#ifdef JIT
/* For faster cycles handling */
extern signed long pissoff;
#endif

/*
 * Handle all events pending within the next cycles_to_add cycles
 */
STATIC_INLINE void do_cycles (unsigned int cycles_to_add)
{
#ifdef PROFILING
	PROFILINGBEGIN
#endif
#ifdef JIT
    if ((pissoff -= cycles_to_add) >= 0)
	return;

    cycles_to_add = -pissoff;
    pissoff = 0;
#endif
/*ric perf
    if (is_lastline && eventtab[ev_hsync].evtime - currcycle <= cycles_to_add) {
	frame_time_t rpt = uae_gethrtime ();
		frame_time_t v   = rpt - vsyncmintime;
		if (v > syncbase || v < -(syncbase))
			vsyncmintime = rpt;
		if (v < 0) {
#ifdef JIT
            pissoff = 3000 * CYCLE_UNIT;
#endif
		    return;
		}

	}
	*/


    while ( (nextevent - currcycle) <= cycles_to_add) {
	unsigned long int eventtime;
//ric		int i;
#ifdef PROFILING
	PROFILINGBEGIN
#endif

		cycles_to_add -= (nextevent - currcycle);
		currcycle = nextevent;

//ric perf	for (i = 0; i < ev_max; i++) {
//ric perf	     if (eventtab[i].active && eventtab[i].evtime == currcycle)
//ric perf		  (*eventtab[i].handler)();
//ric perf	}
//ric perf	events_schedule ();
    	unsigned long int mintime = ~0L;
//ric perf	for (i = 0; i < ev_max; i++)
//ric perf	{
	    if (eventtab[ev_hsync].active)
		{
			 if (eventtab[ev_hsync].evtime == currcycle)
				 hsync_handler();
			 eventtime = eventtab[ev_hsync].evtime - currcycle;
			 if (eventtime < mintime)
				 mintime = eventtime;
		 }
#ifdef PROFILING
	PROFILINGENDNAME(PROFILINGINDEX+84, "events_hsync")
#endif
	     if (eventtab[ev_copper].active)
		 {
			 if (eventtab[ev_copper].evtime == currcycle)
				 copper_handler();
			 eventtime = eventtab[ev_copper].evtime - currcycle;
			 if (eventtime < mintime)
				 mintime = eventtime;
		 }
#ifdef PROFILING
	PROFILINGENDNAME(PROFILINGINDEX+85, "events_copper")
#endif
	     if (eventtab[ev_audio].active)
		 {
			 if (eventtab[ev_audio].evtime == currcycle)
				 audio_evhandler();
			 eventtime = eventtab[ev_audio].evtime - currcycle;
			 if (eventtime < mintime)
				 mintime = eventtime;
		 }
#ifdef PROFILING
	PROFILINGENDNAME(PROFILINGINDEX+86, "events_audio")
#endif
	     if (eventtab[ev_cia].active)
		 {
			 if (eventtab[ev_cia].evtime == currcycle)
				 CIA_handler();
			 eventtime = eventtab[ev_cia].evtime - currcycle;
			 if (eventtime < mintime)
				 mintime = eventtime;
		 }
#ifdef PROFILING
	PROFILINGENDNAME(PROFILINGINDEX+87, "events_CIA")
#endif

	     if (eventtab[ev_blitter].active)
		 {
			 if (eventtab[ev_blitter].evtime == currcycle)
				 blitter_handler();
			 eventtime = eventtab[ev_blitter].evtime - currcycle;
			 if (eventtime < mintime)
				 mintime = eventtime;
		 }
#ifdef PROFILING
	PROFILINGENDNAME(PROFILINGINDEX+88, "events_blitter")
#endif
	     if (eventtab[ev_disk].active)
		 {
			 if (eventtab[ev_disk].evtime == currcycle)
				 DISK_handler();
			 eventtime = eventtab[ev_disk].evtime - currcycle;
			 if (eventtime < mintime)
				 mintime = eventtime;
		 }
#ifdef PROFILING
	PROFILINGENDNAME(PROFILINGINDEX+89, "events_disk")
#endif

//	}
    nextevent = currcycle + mintime;
	}
//end ric
    currcycle += cycles_to_add;
#ifdef PROFILING
	PROFILINGEND(PROFILINGINDEX+90)
#endif
}

STATIC_INLINE unsigned long get_cycles (void)
{
    return currcycle;
}

STATIC_INLINE void set_cycles (unsigned long x)
{
#ifdef JIT
    currcycle = x;
#endif
}

STATIC_INLINE void cycles_do_special (void)
{
#ifdef JIT
    if (pissoff >= 0)
        pissoff = -1;
#endif
}

STATIC_INLINE void do_extra_cycles (unsigned long cycles_to_add)
{
#ifdef JIT
    pissoff -= cycles_to_add;
#endif
}

#define countdown pissoff

#endif
