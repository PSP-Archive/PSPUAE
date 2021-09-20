/* The number of bytes in a char.  */
#define SIZEOF_CHAR 1

/* The number of bytes in a int.  */
#define SIZEOF_INT 4

/* The number of bytes in a long.  */
#define SIZEOF_LONG 4

#ifdef __GNUC__
/* The number of bytes in a long long.  */
#define SIZEOF_LONG_LONG 8
#define SIZEOF___INT64 0
#else
#define SIZEOF_LONG_LONG 0
#define SIZEOF___INT64 8
#endif

/* The number of bytes in a short.  */
#define SIZEOF_SHORT 2
//Ric
#define SIZEOF_VOID_P 4
//End Ric
/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/stat.h> header file.  */
#define HAVE_SYS_STAT_H 1

/* Define if you have the <sys/types.h> header file.  */
#define HAVE_SYS_TYPES_H 1


//FOL, Cleaned up the defines, as they were a mess before. So of these defines proberly do nothing.
//Im slowly learning by trial an error, im damn sure alot that I have changed, was never ment to be.

//E-UAE Defines
//Firmware Kernel
//#define THREE_XX - in makefile

//CPU Defines
#define CYCLE_SWITCH //Undefined, puts it back to E-UAE standard
#define CYCLE_SWITCHUI //Undefined removes the menu options for cycle switch
#define HARD_RESETCOM //Enables Hard Restart option for PSPUAE

//VIDEO Defines
#define MORE_FS_SAMPLE //Alters AutoFS Sample Frames value
#define ADVANCED_CUST //Adds 1 of rics FAMEC optimisations
#define ADVANCED_DRAW //Adds 1 of rics FAMEC optimisations
#define ADVANCED_FS //Adds rics FAMEC FrameSkip logic
//#define NO_FS_ZERO //Define this when AVANCED_FS is not defined, removes 0 from AutoFS
#define UNSAFE_OPT //Adds 1 of rics FAMEC optimisations
#define SPEEDUP

//AUDIO Defines
//#define AUDIO_LATENCY //Undefined = 0 Latency
#define INTERRUPT_ON //Fixes bug with interrupts option still outputing audio
#define AUDIO_UPDATE //Adds sound bug from 070 as an option, removes 1 audio update when used

//MEMORY Defines
#define OS_WITHOUT_MEMORY_MANAGEMENT //Allows higher memory usage in PSPUAE
//FOL, Not sure on below defines
#define USE_MAPPED_MEMORY
#define HAVE_BYTESWAP_H
#define SWAP_W(A) (A)
#define SWAP_L(A) (A)
#define swab_w(A) (A)
#define swab_l(A) (A)

//Storage Defines
#define FLOPPY_DRIVE_HD //Not sure on this 1, fixes HD Floppy crash?
#define DISK_OPT //Try to help speed, by removing calculations and replacing with final figure

//PSP Defines
//#define HIGH_CHIPSET //Makes Official Cycle unit Higher
#define SOUND_DELAY //Adds option to switch the delay, either 1ms or 9ms
#define THREAD_SEMA_WAIT //This doesnt really effect much
#define PSP_MEM_SIZE // Undefined = Old value of 16Megs
#define NO_VSYNC //Undefined = Vertical Sync
#define GP2X //Credit to the UAE4ALL guys, borrowed some of their code
//END PSP Defines
//END E-UAE Defines

#define lseek _lseek

//#define R_OK 1

//Ric
#undef RELY_ON_LOADSEG_DETECTION
#undef USE_COMPILER
#define HAVE_UTIME_NULL 1
#define HAVE_GETMNTENT 1
#define _ALL_SOURCE
#define TIME_WITH_SYS_TIME 1
#define MOUNTED_GETMNTENT1 1
#define STAT_STATFS2_BSIZE 1

#define REGPARAM
#define HAVE_INTTYPES_H 1
#define HAVE_LIBZ 1
#define HAVE_STDBOOL_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRDUP 1
#define HAVE_STRUCT_STAT_ST_BLOCKS 1
#define HAVE_ST_BLOCKS 1
#define HAVE_SYS_IOCTL_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_UNISTD_H 1
#define HAVE_UTIME_H 1
#define HAVE__BOOL 1
#define MULTIPLICATION_PROFITABLE 1

#define HAVE_DIRENT_H 1
#define HAVE_SYS_UTIME_H 1

#define RETSIGTYPE void

#define STDC_HEADERS 1
#define TIME_WITH_SYS_TIME 1
#define X_DISPLAY_MISSING 1

#define DONT_HAVE_POSIX 1
//End Ric

//Ric
#define MACHDEP_GENERIC
#define MACHDEP_NAME            "cpu"
#define GFX_NAME                "none"
#define TARGET_ROM_PATH         "./"
#define TARGET_FLOPPY_PATH      "./"
#define TARGET_HARDFILE_PATH    "./"
#define TARGET_SAVESTATE_PATH   "./"
//End Ric
