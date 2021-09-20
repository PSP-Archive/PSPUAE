#
# Makefile.in for PSPUAE
#
# ric/thinkp/FOL
# GnoStiC/cmf
#
# -------------------------------------------------

INCDIR = include od-psp dms
CFLAGS  = -mno-gpopt -march=allegrex -O3 -G8 -g -DSAVESTATE -DCPUEMU_0 -DAUTOCONFIG -DSUPPORT_THREADS -DFILESYS -DMULTIPLICATION_PROFITABLE
CFLAGS += -mno-gpopt -march=allegrex -O3 -G8 -g -DSHM_SUPPORT_LINKS=0 -D__inline__=__inline__ -DREGPARAM="__attribute__((regparm(3)))" -DGCCCONSTFUNC="__attribute__((const))" -DUSE_UNDERSCORE -DOPTIMIZED_FLAGS -D__inline__=__inline__ -DSHM_SUPPORT_LINKS=0 -fomit-frame-pointer
CXXFLAGS = $(CFLAGS) -mno-gpopt -march=allegrex -O3 -G8 -g -fno-exceptions -Wall -non_shared -Wno-unused -Wno-format -mhard-float -msingle-float 
ASFLAGS = $(CFLAGS) -mno-gpopt -march=allegrex -O3 -G8 -g -Wall
USE_NEWLIB_LIBC = 1

#CFLAGS  = -O3 -G8 -DFPUEMU -DSAVESTATE -DCPUEMU_0 -DCPUEMU_5 -DAUTOCONFIG -DSUPPORT_THREADS -DFILESYS -DOPTIMIZED_FLAGS
#CFLAGS += -O3 -G8 -fomit-frame-pointer -Wall -non_shared -Wno-unused -Wno-format -march=allegrex -mhard-float -msingle-float
#CXXFLAGS = $(CFLAGS) -O3 -G8 -fno-exceptions -fno-rtti
#ASFLAGS = $(CFLAGS)-O3 -G8

# -DTHREE_XX -DPROFILING -DAGA -DSINGLEFILE -DDRIVESOUND -DCPUEMU_68000_ONLY -DCUSTOM_SIMPLE -DDEBUGGER -DHARDDRIVE

LIBDIR =
LIBS = -lpng -lc -lpspwlan -lglut -lGLU -lGL -lpspvfpu -lm -lstdc++ -lpsppower -lpsprtc -lpspaudiocodec -lpspaudiolib -lpspaudio -lpspnet_adhocmatching -lpspnet_adhoc -lpspnet_adhocctl -lSDLmain -lSDL -lSDL_mixer -lz -lvorbisfile -lvorbisenc -lvorbis -ogg -lpspgu

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PSPUAE V0.72B
PSP_EBOOT_ICON1 = ICON1.PMF
PSP_EBOOT_ICON = ICON0.PNG
PSP_EBOOT_PIC1 = PIC1.PNG
PSP_EBOOT_SND0 = SND0.AT3

TARGET    = pspuae
LIBRARIES = 
MATHLIB   = 
RESOBJS   = @RESOBJS@

NO_SCHED_CFLAGS = @NO_SCHED_CFLAGS@

INSTALL         = @INSTALL@
INSTALL_PROGRAM = @INSTALL_PROGRAM@
INSTALL_DATA    = @INSTALL_DATA@
prefix          = @prefix@
exec_prefix     = @exec_prefix@
bindir          = @bindir@
sysconfdir      = @sysconfdir@

VPATH = @top_srcdir@/src

.SUFFIXES: .o .c .h .m .i .S .rc .res

INCLUDES=-I. -I./include/ $(shell psp-config --psp-prefix)/include

LINK=psp-g++
CPP=psp-g++

CPUOBJS	  = cpuemu_0.o newcpu.o cpustbl.o cpudefs.o readcpu.o
DMSOBJ = dms/pfile.o dms/tables.o dms/u_deep.o dms/u_heavy.o dms/u_init.o dms/u_medium.o dms/u_quick.o dms/u_rle.o dms/maketbl.o dms/getbits.o dms/crc_csum.o

BUILT_SOURCES = \
	blit.h blitfunc.h blitfunc.c blittable.c \
	cpudefs.c \
	cpuemu_0.c \
	cpustbl.c cputbl.h \
	compemu.c \
	compstbl.c comptbl.h \
	linetoscr.c

OBJS = main.o memory.o $(CPUOBJS) custom.o cia.o serial.o blitter.o \
       autoconf.o ersatz.o hardfile.o keybuf.o expansion.o zfile.o \
       gfxutil.o blitfunc.o blittable.o events.o \
       disk.o audio.o uaelib.o drawing.o inputdevice.o \
       uaeexe.o bsdsocket.o missing.o crc32.o unzip.o fdi2raw.o \
       cfgfile.o native2amiga.o identify.o sleep.o hotkeys.o \
       enforcer.o misc.o savestate.o traps.o \
       writelog.o filesys.o filesys_unix.o fsdb.o fsdb_unix.o \
       od-psp/psp.o od-psp/kbd.o od-psp/machdep/support.o od-psp/hardfile_psp.o \
       gui-none/nogui.o od-psp/libcglue.o od-psp/sbrk.o od-psp/main_text.o od-psp/joystick.o \
       od-psp/sound.o od-psp/threaddep/thread.o $(DMSOBJ)\
	   
      

all:  $(BUILT_SOURCES) $(EXTRA_TARGETS) $(FINAL_TARGET)
       
#create dir structure for 1.0 / 2.0+ Release - BINDOWS//FOL	
#	mkdir pspuae
#	mkdir pspuae\CONFIGS	
#	mkdir pspuae\STATE
#	mkdir pspuae\DISKS
#	mkdir pspuae\GUI	
#	mkdir pspuae\KICKS
#	mkdir pspuae\SCREENSHOTS	
#	copy EBOOT.PBP pspuae\EBOOT.PBP
#	copy gui-none\splash.png .\pspuae\GUI\
#	copy gui-none\menu.png .\pspuae\GUI\

#create dir structure for 1.0 / 2.0+ Release - LINUX//FOL	
#	mkdir -p pspuae/
#	mkdir -p pspuae/CONFIGS/	
#	mkdir -p pspuae/STATE/
#	mkdir -p pspuae/DISKS/
#	mkdir -p pspuae/GUI/	
#	mkdir -p pspuae/KICKS/
#	mkdir -p pspuae/SCREENSHOTS/	
#	cp EBOOT.PBP pspuae/EBOOT.PBP
#	cp gui-none/splash.png pspuae/GUI/
#	cp gui-none/menu.png pspuae/GUI/

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak