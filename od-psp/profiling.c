#include "profiling.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pspiofilemgr.h>

Function samples[NO_FUNCTIONS];

extern char LaunchPath[];

unsigned long timer;

int saveProfilingResults()
{
	SceUID	fdout;	
	char fileName[512];
	char line[80];
	sprintf(fileName,"%sprofiling.samples",LaunchPath);
	if((fdout = sceIoOpen(fileName, PSP_O_WRONLY | PSP_O_CREAT, 0777))<0) return(-1);
	int i;
	for ( i=0; i< NO_FUNCTIONS;i++)
	{
		if (samples[i].time != 0)
		{
			sprintf(line, "%10ld %10ld %5ld   %s - %s\n", samples[i].time, samples[i].time/samples[i].calls, samples[i].calls, samples[i].file, samples[i].name);
			sceIoWrite(fdout,line,strlen(line));			
		}
	}
	sceIoClose(fdout);
	return(0);	
}

void clearProfilingBuffer()
{
	int i;	
	for ( i=0; i< NO_FUNCTIONS;i++)
	{
		samples[i].time = 0;
		samples[i].calls = 0;
	}	

}


/* Profiler timer thread 
int ProfilerTimerThread(SceSize args, void *argp)
{
	
	return 0;
}

int createProfilerThread()
{
	int thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}
	return thid;
}
*/
