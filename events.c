 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Events
  * These are best for low-frequency events. Having too many of them,
  * or using them for events that occur too frequently, can cause massive
  * slowdown.
  *
  * Copyright 1995-1998 Bernd Schmidt
  * Copyright 2004-2005 Richard Drummond
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "events.h"
#include "custom.h"
#include "cia.h"
#include "blitter.h"
#include "disk.h"
#include "audio.h"
#include "hrtimer.h"

/* Current time in cycles */
unsigned long currcycle;

/* Cycles to next event pending */
unsigned long nextevent;

#ifdef JIT
/* For faster cycles handling */
signed long pissoff = 0;
#endif

struct ev eventtab[ev_max];


void init_eventtab (void)
{
    int i;

    nextevent = 0;
    set_cycles (0);

    for (i = 0; i < ev_max; i++) {
	eventtab[i].active = 0;
	eventtab[i].oldcycles = 0;
    }

    eventtab[ev_cia].handler     = CIA_handler;
    eventtab[ev_hsync].handler   = hsync_handler;
    eventtab[ev_hsync].evtime    = get_cycles () + HSYNCTIME;
    eventtab[ev_hsync].active    = 1;
    eventtab[ev_copper].handler  = copper_handler;
    eventtab[ev_copper].active   = 0;
    eventtab[ev_blitter].handler = blitter_handler;
    eventtab[ev_blitter].active  = 0;
    eventtab[ev_disk].handler    = DISK_handler;
    eventtab[ev_disk].active     = 0;
    eventtab[ev_audio].handler   = audio_evhandler;
    eventtab[ev_audio].active    = 0;

    events_schedule ();
}

/*
 * Determine next event pending
 */
void events_schedule (void)
{
//ric perf    int i;

//ric perf      unsigned long int mintime = ~0L;
//ric perf      for (i = 0; i < ev_max; i++) {
//ric perf  	if (eventtab[i].active) {
//ric perf  	    unsigned long int eventtime = eventtab[i].evtime - currcycle;
//ric perf  	    if (eventtime < mintime)
//ric perf  		mintime = eventtime;
//ric perf  	}
//ric perf      }
//ric perf      nextevent = currcycle + mintime;
	
    	unsigned long int mintime = ~0L;
		unsigned long int eventtime;	
	    if (eventtab[ev_hsync].active)
		{
			 eventtime = eventtab[ev_hsync].evtime - currcycle;
			 if (eventtime < mintime)
				 mintime = eventtime;
		 }		 
	     if (eventtab[ev_copper].active)
		 {
			 eventtime = eventtab[ev_copper].evtime - currcycle;
			 if (eventtime < mintime)
				 mintime = eventtime;
		 }		 
	     if (eventtab[ev_audio].active)
		 {
			 eventtime = eventtab[ev_audio].evtime - currcycle;
			 if (eventtime < mintime)
				 mintime = eventtime;
		 }		 
	     if (eventtab[ev_cia].active)
		 {
			 eventtime = eventtab[ev_cia].evtime - currcycle;
			 if (eventtime < mintime)
				 mintime = eventtime;
		 }		 
	     if (eventtab[ev_blitter].active)
		 {
			 eventtime = eventtab[ev_blitter].evtime - currcycle;
			 if (eventtime < mintime)
				 mintime = eventtime;
		 }		 
	     if (eventtab[ev_disk].active)
		 {
			 eventtime = eventtab[ev_disk].evtime - currcycle;
			 if (eventtime < mintime)
				 mintime = eventtime;
		 }		 
    nextevent = currcycle + mintime;

//end ric	
}

/*
 * Handle all events due at current time
 */
void handle_active_events (void)
{
    int i;
    for (i = 0; i < ev_max; i++) {
	if (eventtab[i].active && eventtab[i].evtime == currcycle) {
	    (*eventtab[i].handler)();
	}
    }
}
