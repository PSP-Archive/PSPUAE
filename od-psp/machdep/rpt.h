#ifndef RPT_H
#define RPT_H
/*
  * UAE - The Un*x Amiga Emulator
  *
  * Definitions for accessing cycle counters on a given machine, if possible.
  *
  * Copyright 1998 Bernd Schmidt
  */
//Ric  
#define HAVE_GETTIMEOFDAY 1
//End Ric
typedef unsigned long frame_time_t;

static __inline__ frame_time_t read_processor_time (void)
{
    return 0;
}

#define gettimeofday sceKernelLibcGettimeofday

#endif
