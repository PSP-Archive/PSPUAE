#ifndef PROFILING_H
#define PROFILING_H
#include <psputils.h>
#include <pspthreadman.h>
#include <time.h>

typedef struct 
{
	char name[40];
	char file[30];
	unsigned long time;
	unsigned int  calls;
} Function;
#define NO_FUNCTIONS 2000

extern Function samples[];

extern int saveProfilingResults();
extern void clearProfilingBuffer();

#define PROFILINGBEGIN  unsigned long _pp_t1 = sceKernelGetSystemTimeLow();	
					   
#define PROFILINGEND(i) samples[i].time += sceKernelGetSystemTimeLow() - _pp_t1; if (samples[i].name[0]==0){strcpy(samples[i].file, __FILE__); strcpy(samples[i].name,  __FUNCTION__);} samples[i].calls++;
#define PROFILINGENDNAME(i,funcname) samples[i].time += sceKernelGetSystemTimeLow() - _pp_t1; if (samples[i].name[0]==0){strcpy(samples[i].file, __FILE__); strcpy(samples[i].name, funcname);} samples[i].calls++;
#define PROFILINGCOUNT(i,c,funcname) samples[i].time += c; if (samples[i].name[0]==0){strcpy(samples[i].file, __FILE__); strcpy(samples[i].name, funcname);} samples[i].calls++;
#define PROFILINGMAXCOUNT(i,c,funcname) if (samples[i].time < c) samples[i].time = c; if (samples[i].name[0]==0){strcpy(samples[i].file, __FILE__); strcpy(samples[i].name, funcname);} samples[i].calls++;

//#define PROFILINGBEGIN  unsigned long _pp_t1 = sceKernelLibcClock();	
					   
//#define PROFILINGEND(i) samples[i].time += sceKernelLibcClock() - _pp_t1; if (samples[i].name[0]==0){strcpy(samples[i].file, __FILE__); strcpy(samples[i].name,  __FUNCTION__);} samples[i].calls++;
				   
//#define PROFILINGBEGIN  struct timeval tp; struct timezone tzp; sceKernelLibcGettimeofday(&tp,&tzp); unsigned long _pp_t1 = tp.tv_usec;
					   
//#define PROFILINGEND(i) sceKernelLibcGettimeofday(&tp,&tzp); samples[i].time += tp.tv_usec - _pp_t1; if (samples[i].name[0]==0){strcpy(samples[i].file, __FILE__); strcpy(samples[i].name,  __FUNCTION__);} samples[i].calls++;
#endif
