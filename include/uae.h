 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Prototypes for main.c
  *
  * Copyright 1996 Bernd Schmidt
  */

extern void do_start_program (void);
extern void do_leave_program (void);
extern void start_program (void);
extern void leave_program (void);
extern void real_main (int, char **);
extern void usage (void);

extern void sleep_millis (int ms);
extern void sleep_millis_busy (int ms);


extern void uae_reset (int);
extern void uae_quit (void);
extern void uae_restart (int, char*);
extern void reset_all_systems (void);

extern int quit_program;

extern void setup_brkhandler (void);

#ifdef WIN32
extern char warning_buffer[256];
extern char *start_path;

void logging_init (void);
void filesys_init (void);
#endif

#ifdef USE_SDL
int init_sdl (void);
#endif
