 /*
  * UAE - The Un*x Amiga Emulator
  * 
  * Generic thread support doesn't exist.
  * 
  * Copyright 1997 Bernd Schmidt
  */

#undef SUPPORT_PENGUINS


typedef int uae_sem_t;

extern int uae_sem_init (uae_sem_t *sem, int pshared, unsigned int value);

STATIC_INLINE int uae_sem_destroy (uae_sem_t *sem)
{
	write_log("uae_sem_destroy");
	return sceKernelDeleteSema(*sem);
}

STATIC_INLINE int uae_sem_post (uae_sem_t *sem)
{
	write_log("uae_sem_post");
	return sceKernelSignalSema(*sem, 1);
}

STATIC_INLINE int uae_sem_wait (uae_sem_t *sem)
{
	write_log("uae_sem_wait");
#ifdef THREAD_SEMA_WAIT
	return sceKernelWaitSema(*sem, 1, 1000); //1000 = 10 ms
#else
	return sceKernelWaitSema(*sem, 1, 0); //0 ms
#endif
}

STATIC_INLINE int uae_sem_trywait (uae_sem_t *sem)
{
	write_log("uae_sem_trywait");
#ifdef THREAD_SEMA_WAIT
	return sceKernelWaitSema(*sem, 1, 10000); //10000 = 10 ms
#else
	return sceKernelWaitSema(*sem, 1, 0); //0 ms
#endif
}

STATIC_INLINE int uae_sem_getvalue (uae_sem_t *sem, int *sval)
{
	write_log("uae_sem_getvalue");
    return sceKernelPollSema(*sem, 1);
}


typedef int uae_thread_id;


STATIC_INLINE int uae_start_thread (void *(*f) (void *), void *arg, uae_thread_id *thread)
{
	write_log("uae_start_thread");
    *thread = sceKernelCreateThread("uae_thread", (int (*)(void *))f, 0x10, 0xFA0, 0, 0);
    if(*thread >= 0)
    {
	sceKernelStartThread(*thread, 0, arg);
    }
    return *thread == 0;
}

STATIC_INLINE int uae_wait_thread (uae_thread_id thread)
{
	write_log("uae_wait_thread");	
    return sceKernelWaitThreadEndCB(thread, 0);
}

#define uae_set_thread_priority(pri)

#include "commpipe.h"
