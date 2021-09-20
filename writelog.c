 /*
  * E-UAE - The portable Amiga emulator
  *
  * Standard write_log that writes to the console or to a file.
  *
  * Copyright 2001 Bernd Schmidt
  * Copyright 2006 Richard Drummond
  */
#include "sysconfig.h"
#include "sysdeps.h"

static FILE *logfile;

/*
 * By default write-log and friends access the stderr stream.
 * This function allows you to specify a file to be used for logging
 * instead.
 * 
 * Call with NULL to close a previously opened log file.
 */
void set_logfile_standard (const char *logfile_name)
{
    if (logfile_name && strlen (logfile_name)) {
	FILE *newfile = fopen (logfile_name, "w");

	if (newfile)
	    logfile = newfile;
    } else {
	if (logfile) {
	    fclose (logfile);

	    logfile = 0;
	}
    }
}

void write_log_standard (const char *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
#ifdef HAVE_VFPRINTF
    vfprintf (logfile ? logfile : stderr, fmt, ap);
#else
    /* Technique stolen from GCC.  */
    {
	int x1, x2, x3, x4, x5, x6, x7, x8;
	x1 = va_arg (ap, int);
	x2 = va_arg (ap, int);
	x3 = va_arg (ap, int);
	x4 = va_arg (ap, int);
	x5 = va_arg (ap, int);
	x6 = va_arg (ap, int);
	x7 = va_arg (ap, int);
	x8 = va_arg (ap, int);
	fprintf (logfile ? logfile : stderr, fmt, x1, x2, x3, x4, x5, x6, x7, x8);
    }
#endif
}

void flush_log_standard (void)
{
    fflush (logfile ? logfile : stderr);
}
