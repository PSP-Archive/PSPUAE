 /*
  * UAE - The Un*x Amiga Emulator
  *
  * PSP Threading support, using pthreads
  *
  * Copyright 1997 Bernd Schmidt
  * Copyright 2004 Richard Drummond
  * 
  * PSP thread support created by Ric
  * This handles initialization when using named semaphores.
  * 
  */
#include "sysconfig.h"
#include "sysdeps.h"

#include "threaddep/thread.h"

int uae_sem_init (uae_sem_t *sem, int pshared, unsigned int value)
{
	write_log("uae_sem_init");
	//generate a unique name
    char name[32];
    static int semno = 0; // hold the current no
    sprintf (name, "/uaesem-%d", semno++);
	
	*sem = sceKernelCreateSema(name, 0, 1, 1, 0);
    return *sem;
}

