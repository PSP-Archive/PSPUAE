/*
  Main for PSP   Christophe and MIB.42 /2005
*/

#ifdef THREE_XX
#include <pspsdk.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pspkerneltypes.h>
#include <pspkernel.h>
#include <pspthreadman.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspmoduleinfo.h>
#include <psppower.h>
#include <pspaudiolib.h>
#include <pspaudio.h>
#include <pspiofilemgr.h>
#include <psputils.h>
#include <malloc.h>
#include <time.h>
#include <pspgu.h>
//#ifdef THREE_XX
//#include <pspgum.h>
//#endif
#include <png.h>

#include "main_text.h"
#include "sysconfig.h"
#include "sysdeps.h"
#include "gensound.h"
#include "uae.h"
#include "xwin.h"
#include "custom.h"
#include "options.h"
#include "savestate.h"
#include "hrtimer.h"
#include "drawing.h"
// the following include allow us to get to the fps counter
#include "gui.h"
// thinkp
#include <keyboard.h>
// thinkp
//ric
#include "profiling.h"
#define PROFILINGINDEX 1200
//#undef PROFILING
//end ric

#define VERSION "0.72B"

#define JSEM_JOYS      		100
#define JSEM_MICE      		200

#define VRAM_ADDR			(0x04000000)
#define VRAM_WRITE 			(0x44000000)
#define SCREEN_WIDTH		480
#define SCREEN_HEIGHT		272

#define	PIXELSIZE	     	 1				//in short
#define	LINESIZE	      	512				//in short
#define	PIXELSIZE2	      	2
#define	LINESIZE2	      	1024
#define AMIGALINESIZE     	480
#define AMIGASCREENHEIGHT 	272

#define ASPECT43 1
#define ASPECT169 4.0f/3.0f
#define ASPECT21 16.0f/9.0f

#define FRAMEBUFFER_SIZE (LINESIZE2*SCREEN_HEIGHT)

#define AS_JOY0		0
#define	AS_JOY1		1
#define	AS_MOUSE	2
// thinkp
#define	AS_KEYS		3

#define JOYBUTTON_CD32_PLAY 3
#define JOYBUTTON_CD32_RWD 4
#define JOYBUTTON_CD32_FFW 5
#define JOYBUTTON_CD32_GREEN 6
#define JOYBUTTON_CD32_YELLOW 7
#define JOYBUTTON_CD32_RED 8
#define JOYBUTTON_CD32_BLUE 9

extern void gprof_cleanup();
extern void sound_callback(void *buf, unsigned int length, void *pdata);
//extern void sound_callback1(void *buf, unsigned int length, void *pdata);

//in drawing.c
extern void	force_update_frame(void);
extern void finish_drawing_frame (void);

extern void	quitprogram();
extern void	uae_reset(int);
extern void	DrawKeyboard(void);
extern void	DrawKeyboardD(void);
extern void	CallKeyboardInput(SceCtrlData ctl);
extern char	ActualKbdString[];
extern unsigned char	SolidKeyboard;
extern int lof_changed;
int CheckActive(int this,int that);
extern void record_key (int);

extern void SetCurSel(int);

// thinkp mousestates
extern void get_mouse(int* mpos, int* mctl);
extern void set_mouse(int mpos, int mctl);
// thinkp

char *bt_strings[] = {"Left Mouse Button   ",
						"Right Mouse Button  ",
						"Middle Mouse Button ",
						"Joystick 0 Fire     ",					
						"Joystick 1 Fire     ",
						"Activate Keyboard   ",
						"HiRes Mouse Movement",
						"Screenshot          ",
						"Joystick 0 UP       ",
						"Joystick 0 DOWN     ",
						"Joystick 0 LEFT     ",
						"Joystick 0 RIGHT    ",
// thinkp
						"Joystick 1 UP       ",
						"Joystick 1 DOWN     ",
						"Joystick 1 LEFT     ",
						"Joystick 1 RIGHT    ",

						"CD32 RED Button     ",
						"CD32 BLUE Button    ",
						"CD32 YELLOW Button  ",
						"CD32 GREEN Button   ",
						"CD32 REWIND Button  ",
						"CD32 FORWARD Button ",
						"CD32 PLAY Button    ",
						"Toggle Mousespeed   "};


enum {BT_LEFT_MOUSE=0,BT_RIGHT_MOUSE,BT_MIDDLE_MOUSE,BT_JOY0_FIRE,BT_JOY1_FIRE,BT_ACTIVATE_KEYBOARD,BT_HIRES_MOUSE,BT_SCREEN_SHOT,BT_JOY0_UP,BT_JOY0_DOWN,BT_JOY0_LEFT,BT_JOY0_RIGHT,BT_KEY_ENTRIES};
// thinkp mousestates
#define BT_MOUSESTATES BT_KEY_ENTRIES+94

enum {BT_JOY1_UP = BT_MOUSESTATES+24,BT_JOY1_DOWN,BT_JOY1_LEFT,BT_JOY1_RIGHT,BT_JOY0_CD32_RED,BT_JOY0_CD32_BLUE,BT_JOY0_CD32_YELLOW,BT_JOY0_CD32_GREEN,BT_JOY0_CD32_RWD,BT_JOY0_CD32_FFW,BT_JOY0_CD32_PLAY, BT_TOGGLEMOUSESPEED};
// thinkp mousestates

typedef struct KeyStruct {	// I am not in the mood of implementing this nicely, just copied it from my kbd.c ...
	int	AmigaKeyCode;
	int	x1,y1,x2,y2;
	char*	Name;
} KeyStruct;
extern KeyStruct AmigaKeyboard[];


static unsigned char	KeyboardActive=0;
static unsigned char	KeyboardActiveMenu=0;
static unsigned char	KeyboardActiveEdit=0;
unsigned char		KeyboardPosition=0;		// 0=Down, 1=Up;
unsigned char		KeyboardPositionOffset=142;

typedef struct
{
	unsigned short u, v;
	unsigned short color;
	short x, y, z;
} Vertex;
typedef struct
{
	unsigned short u, v;
	short x, y, z;
} Vertex2;

char	OneLine[SCREEN_WIDTH*3];

int lastmx, lastmy;
int buttonstate[3];

// this is the debug screen buffer
#define DEBUGLINES 29
#define DEBUGLINELENGTH 24
char debug_buffer [DEBUGLINES][DEBUGLINELENGTH];
int current_debug_line = 0;

#ifdef THREE_XX
/* Define the main thread's attribute value (optional) */
PSP_MODULE_INFO("PSPUAE", 0x0, 1, 0); //USERMODE, means it will work on any PSP Firmware
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU); //Define main thread as USERMODE
//PSP_HEAP_SIZE_MAX(); //Allocate all availible memory for HEAP
PSP_HEAP_SIZE_KB(-256); //Allocate all but 256K memory for HEAP
#else
PSP_MODULE_INFO("PSPUAE", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
#endif


unsigned short __attribute__((aligned(16))) ColorLineColor=0;
unsigned short __attribute__((aligned(16))) ColorLine[60];
static unsigned int __attribute__((aligned(16))) list[8192];

int g_exitUae = 0;
int g_start_active = 0;
int g_square_active = 0;
int g_triangle_active = 0;
int g_cross_active = 0;
int g_circle_active = 0;
int g_lshoulder_active = 0;
int g_rshoulder_active = 0;

#ifdef ADVANCED_FS
int g_frameskip = 0; // 0 = auto frame skip
#else
int g_frameskip = 9; // 9 = auto frame skip
#endif
int g_zoom = 0; //no zoom thinkp
int g_solidkeyboard = 0;
int g_analogstick = AS_MOUSE;
int g_dirbuttons = AS_JOY0;
int g_start = BT_ACTIVATE_KEYBOARD;
int g_square = 88 + BT_KEY_ENTRIES;
int g_triangle = BT_HIRES_MOUSE;	// Enter
int g_cross = BT_JOY0_FIRE;
int g_circle = BT_MIDDLE_MOUSE;
int g_lshoulder = BT_LEFT_MOUSE;
int g_rshoulder = BT_RIGHT_MOUSE;
int g_direct_screen_access = 0;
int changed_g_direct_screen_access = 0;
int g_kickstart_rom = 3; //1.3

// thinkp
int g_up = BT_LEFT_MOUSE;
int g_down = BT_LEFT_MOUSE;
int g_left = BT_LEFT_MOUSE;
int g_right = BT_LEFT_MOUSE;
int g_up_active = 0;
int g_down_active = 0;
int g_left_active = 0;
int g_right_active = 0;
int g_lup = BT_LEFT_MOUSE;
int g_ldown = BT_LEFT_MOUSE;
int g_lleft = BT_LEFT_MOUSE;
int g_lright = BT_LEFT_MOUSE;
int g_lup_active = 0;
int g_ldown_active = 0;
int g_lleft_active = 0;
int g_lright_active = 0;
int g_rup = BT_LEFT_MOUSE;
int g_rdown = BT_LEFT_MOUSE;
int g_rleft = BT_LEFT_MOUSE;
int g_rright = BT_LEFT_MOUSE;
int g_rup_active = 0;
int g_rdown_active = 0;
int g_rleft_active = 0;
int g_rright_active = 0;
int g_lsquare = BT_LEFT_MOUSE;
int g_ltriangle = BT_LEFT_MOUSE;
int g_lcross = BT_LEFT_MOUSE;
int g_lcircle = BT_LEFT_MOUSE;
int g_lsquare_active = 0;
int g_ltriangle_active = 0;
int g_lcross_active = 0;
int g_lcircle_active = 0;
int g_rsquare = BT_LEFT_MOUSE;
int g_rtriangle = BT_LEFT_MOUSE;
int g_rcross = BT_LEFT_MOUSE;
int g_rcircle = BT_LEFT_MOUSE;
int g_rsquare_active = 0;
int g_rtriangle_active = 0;
int g_rcross_active = 0;
int g_rcircle_active = 0;
int g_mouse_speed = 3;
int g_hr_mouse_speed = 5;

char	lastsavestate[255]="";
char	lastdiskname[255]="";
int		g_disablekeycombos=0;
int		g_togglexo=0;
int		g_aspect_ratio=1;		// 16:9
int		g_cpu2chip_ratio=8;
#ifdef CYCLE_SWITCH
int		m68k_speed[20] = {256, 512, 768, 1024, 1280, 1536, 1792, 2048, 2304, 2560, 2816, 3072, 3328, 3584, 3840, 4096, 4352, 4608, 4864, 5120};
#else
int		m68k_speed[16] = {512, 1024, 1536, 2048, 2560, 3072, 3584, 4096, 4608, 5120, 5632, 6144, 6656, 7168, 7680, 8192};
#endif
int		g_ScreenX=-65;
int		g_ScreenY=0;
int		g_amiga_linesize=480;
int		g_screenlock=0;
int		g_autozoom=1;
int		g_bgnd_image=1;
//int		g_audio_opt=0;
#ifdef CYCLE_SWITCH
int     g_cpuspeed_up=1;
int		g_cputurbo=512;
int		g_chipsetturbo=1;
int		g_cpulock=1;
#endif
// thinkp mousestates
int		g_automousespeed=5;		// slow
#ifdef SOUND_DELAY
int     g_sdswitch=1;
int		g_snddelay=1000;
#endif

typedef struct  {
	int		mbutton;
	int		x,y;
	char	name[21];
}MouseState;

MouseState	mousestates[24];
int			domousestatemenu=0;
int			mousestates_available=0;

// thinkp mousestates

int	g_togglemousespeed=0;

//cmf
// cmf: fortunately all sizes-arrays have the size of 5 - update: not so for chipmem
uae_u32 selected_mem(uae_u32 size, const uae_u32* memsizes) {
	uae_u32 i;
	for(i = 0; i < 5; i++) {
		if((size >> 0x0A) == memsizes[i]) return i;
	}
	return 0;
}

// cmf: need an extra routine for chip mem
uae_u32 selected_chipmem(uae_u32 size, const uae_u32* memsizes) {
	uae_u32 i;
	for(i = 0; i < 4; i++) {
		if((size >> 0x0A) == memsizes[i]) return i;
	}
	return 0;
}
//cmf

// thinkp borrowed from snes9xTYL
enum {
  EXT_ADF,
  EXT_ADZ,
  EXT_ZIP,
  EXT_HDF,
  EXT_ASF,
  EXT_UNKNOWN
};

#define MAXPATH 256		//temp, not confirmed
#define MAX_ENTRY 256

enum {
    TYPE_DIR=0x10,
    TYPE_FILE=0x20
};

int nfiles;
SceIoDirent files[MAX_ENTRY];
char path[4096];
int	 browse_folders=0;
// thinkp

static const float zoomfactors[] = { 1.0f, 1.1f, 1.11f, 1.12f, 1.14f, 1.17f, 1.2f, 1.25f, 1.34f, 1.5f };
//ric - to allow auto zoom
extern unsigned int hstrt; // horizontal start
extern unsigned int hstop; // horizontal stop
extern unsigned int vstrt; // vertical start
extern unsigned int vstop; // vertical stop
extern unsigned int minfirstline;  // fist line to display as calculated in calcdiw()
extern int min_diwstart, max_diwstop;
//end ric

int 	do_screen_shot=0;
char	*OPTIONSFILENAME;
char	TmpFileName[1024];
char	g_path[1024];
char	LaunchPath[512];
int		LaunchPathEndPtr=0;
int 	prefs_initialized = 0;
int 	signal_reset = 0;

char copy_memory[1024];

// utility functions - New toolchain breaks this function
//void usleep(unsigned long usec)
//void usleep(useconds_t usecs)
int usleep(useconds_t __useconds)
{
	sceKernelDelayThread(__useconds);
}

time_t time(time_t *t)
{
	return sceKernelLibcTime(NULL);
}

void flush_log (void)
{
}

void write_log (const char *s,...)
{
#ifdef DEBUGGER
	//if (changed_prefs.start_debugger)
	{
SceUID	fdout;
int	c;

#if 0
	for(c=0;((c<1023)&&(s[c]!='\0'));c++) copy_memory[c]=s[c];
	copy_memory[c]='\n';
#else
va_list ap;

	va_start (ap, s);
	vsprintf (copy_memory, s, ap);
	for(c=0;((c<1023)&&(copy_memory[c]!='\0'));c++);
	copy_memory[c++]=13;
	copy_memory[c++]=10;
#endif
	LaunchPath[LaunchPathEndPtr]='\0';
	sprintf(TmpFileName,"%slog.txt",LaunchPath);
	if((fdout = sceIoOpen(TmpFileName, PSP_O_RDWR, 0777))<0)
	{
		if((fdout = sceIoOpen(TmpFileName, PSP_O_WRONLY | PSP_O_CREAT, 0777))<0)
			return;
	}
	else
		sceIoLseek32(fdout, 0, PSP_SEEK_END);
	sceIoWrite(fdout, copy_memory, c);
	sceIoClose(fdout);
	copy_memory[c] = 0;
	pspDebug(copy_memory);
	}
#endif
}


//----------------------- gfx stuff -------------------------
static int nblockframes=0;
static int nblockframes_sav=0;
static unsigned long lockscrstarttime=0;
static unsigned long lockscrstarttime_sav=0;


static int dispBufferNumber;

u16* getVramTextureBuffer()
{
	u16* vram = (u16*) VRAM_WRITE;
	vram += FRAMEBUFFER_SIZE * 2 / sizeof(u16);
	return vram;
}

u16* getVramDisplayBuffer()
{
	u16* vram = (u16*) VRAM_WRITE;
	if (dispBufferNumber == 0) vram += FRAMEBUFFER_SIZE / sizeof(u16);
	return vram;
}

u16* getVramDrawBuffer()
{
	if (g_direct_screen_access)
		return getVramDisplayBuffer();
	else
	{
		u16* vram = (u16*) VRAM_WRITE;
		if (dispBufferNumber == 1) vram += FRAMEBUFFER_SIZE / sizeof(u16);
		return vram;
	}
}

u16* getVramBuffer()
{
	if (g_direct_screen_access)
	{
		u16* vram = getVramDisplayBuffer();
#ifdef DEBUGGER
		if (!changed_prefs.start_debugger) // center screen
			vram += 60;
#endif
		return vram;
	}
	else
		return getVramTextureBuffer();
}

void copyTextureToDrawBuffer()
{
	int i;
	u16* texture = getVramTextureBuffer();
	u16* draw = getVramDrawBuffer();
	sceDisplayWaitVblankStart();
	for (i=0;i<FRAMEBUFFER_SIZE;i++)
		*draw++ = *texture++;
	flipScreen2();
}
void copyDisplayToTextureBuffer()
{
	int i;
	u16* display = getVramDisplayBuffer();
	u16* texture = getVramTextureBuffer();
	sceDisplayWaitVblankStart();
	for (i=0;i<FRAMEBUFFER_SIZE;i++)
		*texture++ = *display++;
}

void *pgGetVramAddr(unsigned long x,unsigned long y)
{
	return((char*)(x*PIXELSIZE2+y*LINESIZE2+((char*)getVramDrawBuffer())));
}

void flipScreen()
{
	if (g_direct_screen_access) return; // dont flip screens if we are writing directly to the displaybuffer
	sceGuSwapBuffers();
	dispBufferNumber ^= 1;
}

void flipScreen2()
{
	sceGuSwapBuffers();
	dispBufferNumber ^= 1;
}
void restoreScreenBuffer()
{
	if (dispBufferNumber != 0)
		flipScreen2();
}
void clearScreen()
{
	sceDisplayWaitVblankStart();
	memset((void*)getVramDrawBuffer(),0,LINESIZE2*SCREEN_HEIGHT);
	flipScreen();
	memset((void*)getVramDrawBuffer(),0,LINESIZE2*SCREEN_HEIGHT);
	flipScreen();
}
void drawNothing()
{
	sceGuStart(GU_DIRECT,list);
    sceGuFinish();
    sceGuSync(0,0);
}

int isGUInit = 0;
void initGU()
{
	sceGuInit();
	sceGuStart(GU_DIRECT,list);
	sceGuDrawBuffer(GU_PSM_5551, (void *)0, LINESIZE);
	sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, (void*)FRAMEBUFFER_SIZE, LINESIZE);
	sceGuTexMode(GU_PSM_8888, 0, 0, 0);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB); // don't get influenced by any vertex colors
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	sceGuOffset(2048 - (SCREEN_WIDTH/2),2048 - (SCREEN_HEIGHT/2));
	sceGuViewport(2048,2048,SCREEN_WIDTH,SCREEN_HEIGHT);
	sceGuDepthRange(65535,0);
	sceGuScissor(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuFrontFace(GU_CW);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
	sceGuFinish();
    sceGuSync(0,0);
    sceDisplayWaitVblankStart();
	sceGuDisplay(1);
	dispBufferNumber = 0;
	isGUInit = 1;
}

void checkGU()
{
	if (!isGUInit) initGU();
}

unsigned char BMPHeader[57] = {
0x42,0x4d,0x38,0xfa,0x05,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0xe0,0x01,0x00,0x00,0x10,0x01,0x00,0x00,0x01,0x00,0x18,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x2e,0x00,0x00,0x20,0x2e,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x27,0x00 };

void SaveScreen(void)
{
	SceUID		fdout;
	int		a,b,c;
	unsigned short	*clrptr,color;

	LaunchPath[LaunchPathEndPtr]='\0';

// thinkp
	// check if the SCREENSHOTS dir exists
	create_dir("SCREENSHOTS");
// thinkp
	sprintf(TmpFileName,"%sSCREENSHOTS/%d.bmp",LaunchPath,(int)(osdep_gethrtime()));
	if((fdout = sceIoOpen(TmpFileName, PSP_O_WRONLY | PSP_O_CREAT, 0777))<0) return;
	sceIoWrite(fdout, BMPHeader, 57);
	for(b=0;b<SCREEN_HEIGHT;b++)
	{
		clrptr=(unsigned short *)(getVramDisplayBuffer()+((SCREEN_HEIGHT-b-1)*LINESIZE));
		c=0;
		for(a=0;a<SCREEN_WIDTH;a++)
		{
			color=*clrptr;
			OneLine[c]=(unsigned char)(((color>>10)&0x1F)<<3);c++;
			OneLine[c]=(unsigned char)(((color>>5)&0x1F)<<3);c++;
			OneLine[c]=(unsigned char)(((color>>0)&0x1F)<<3);c++;
			++clrptr;
		}
		sceIoWrite(fdout, OneLine, (SCREEN_WIDTH*3));
	}
	sceIoClose(fdout);
}

void vsync_callback()
{
#ifdef PROFILING
	PROFILINGBEGIN
#endif
	//The next block shouldn't really be here
	//but in 0.2.29WIP2 for some reason changed prefs isn't picked up
	if (!prefs_initialized)
	{
		DefaultPSPUAEOptions();
		LoadPSPUAEOptions(1,"","", 1);	// load global options
		prefs_initialized = 1;
	}

	//check if a reset was requested
	// reset on 6th vsync to allow e-uae to pick up changed_prefs
	if (signal_reset > 0)
	{
		signal_reset--;
		if (signal_reset == 1)
			uae_reset(1);
	}

#ifdef ADVANCED_FS
	//now here is some logic for the auto frameskip option
	static unsigned int frameskipCounter, lastfps, lasttime;
	//autoframerate
	if(g_frameskip == 0) //auto frameskip on
	{
#ifdef MORE_FS_SAMPLE
#define SAMPLEFRAMES 18
#else
#define SAMPLEFRAMES 8
#endif
		frameskipCounter++;
		if (frameskipCounter == SAMPLEFRAMES)
		{
			int fpslimit = 50;
			if (currprefs.ntscmode) fpslimit = 60;
			unsigned int t = osdep_gethrtime();
			int fps = osdep_gethrtimebase()*SAMPLEFRAMES/(t-lasttime);
			if (fps == 0) fps = 1;
			lasttime = t;
			frameskipCounter = 0;

			if (fps < fpslimit-1 || fps > fpslimit+1)
			{
				int framerate;
				if (changed_prefs.gfx_framerate > 23)
				   framerate = fpslimit * changed_prefs.gfx_framerate/fps;
			    else
				   framerate = 44 - fps * (44 - changed_prefs.gfx_framerate)/fpslimit;

				if (framerate == changed_prefs.gfx_framerate)
				{
					if (fps < fpslimit-1 && fps<=lastfps)
						// fps didnt change so lets adjust the framerate
						changed_prefs.gfx_framerate++;
					else if (fps > fpslimit+1 && fps>=lastfps)
						changed_prefs.gfx_framerate--;
				}
				else changed_prefs.gfx_framerate = framerate;
				if(changed_prefs.gfx_framerate > 44) changed_prefs.gfx_framerate = 44;
				if(changed_prefs.gfx_framerate < 0) changed_prefs.gfx_framerate = 0;
			}
			lastfps = fps;
		}
	}
#else
	//now here is some logic for the auto frameskip option
	static unsigned int frameskipCounter, lastfps, lasttime;
	//autoframerate
	if(g_frameskip == 9) //auto frameskip on
	{
#ifdef MORE_FS_SAMPLE
#define SAMPLEFRAMES 12 //FOL
#else
#define SAMPLEFRAMES 6 //FOL
#endif
		frameskipCounter++;
		if (frameskipCounter == SAMPLEFRAMES)
		{
			int fpslimit = 50;
			if (currprefs.ntscmode) fpslimit = 60;
			unsigned int t = osdep_gethrtime();
			int fps = osdep_gethrtimebase()*SAMPLEFRAMES/(t-lasttime);
			if (fps == 0) fps = 1;
			lasttime = t;
			frameskipCounter = 0;

			if (fps < fpslimit-1 || fps > fpslimit)
			{
				int framerate = fpslimit * changed_prefs.gfx_framerate/fps;
				if (framerate == changed_prefs.gfx_framerate)
				{
					if (fps < fpslimit-1 && fps<=lastfps)
						// fps didnt change so lets adjust the framerate
						changed_prefs.gfx_framerate++;
					else if (fps > fpslimit && fps>=lastfps)
						changed_prefs.gfx_framerate--;
				}
				else changed_prefs.gfx_framerate = framerate;
				if(changed_prefs.gfx_framerate > 9) changed_prefs.gfx_framerate = 9;
#ifdef NO_FS_ZERO
				if(changed_prefs.gfx_framerate < 2) changed_prefs.gfx_framerate = 2 ;
#else
				if(changed_prefs.gfx_framerate < 1) changed_prefs.gfx_framerate = 1 ;
#endif
			}
			lastfps = fps;
		}
	}
#endif
	if (currprefs.leds_on_screen)
		drawStats();
#ifdef PROFILING
	PROFILINGEND(PROFILINGINDEX+10)
#endif
}


void psp_flush_screen(int ystart, int ystop)
{
#ifdef PROFILING
	PROFILINGBEGIN
#endif
	if (gfxvidinfo.bufmem == 0) return; // no buffer allocated return
	if (!g_direct_screen_access)
	{
		// direct screen access not enabled
		// so we need to flush the screen
		ystart = 0;ystop = AMIGASCREENHEIGHT;

		sceGuStart(GU_DIRECT,list);
		sceGuTexMode(GU_PSM_5551, 0, 0, 0);
		sceGuTexImage(0, 512, 512, 512, (void*) getVramTextureBuffer());
		sceGuTexFilter(GU_LINEAR,GU_LINEAR);
		int width = AMIGALINESIZE;							//thinkp
		int height = AMIGASCREENHEIGHT;
		int sx = 0;
		int sy = 0;
		static int dx;
		static int dy;
		static float destHeight;
		static float fixedWidth;
		static float xScale;
		static float yScale;
		static int last_g_autozoom;
		if (g_autozoom)
		{
			static int last_vstrt, last_vstop, last_hstrt, last_hstop, last_min_diwstart, last_max_diwstop;
			if (last_g_autozoom != g_autozoom || last_vstrt != vstrt || last_vstop != vstop || last_hstrt != hstrt ||
				last_hstop != hstop || last_min_diwstart != min_diwstart || last_max_diwstop != max_diwstop)
			{
				pspDebug("recalculating auto zoom factors...");
				last_vstrt = vstrt;
				last_vstop = vstop;
				last_hstrt = hstrt;
				last_hstop = hstop;
				dy = 0;

				int start_y = minfirstline;  // minfirstline = first line to be written to screen buffer
				int stop_y = AMIGASCREENHEIGHT + minfirstline; // last line to be written to screen buffer
				if (vstrt > minfirstline)
					start_y = vstrt;		// if vstrt > minfirstline then there is a black border
				if (start_y > 200)
					start_y = minfirstline; // shouldn't happen but does for donkey kong
				if (vstop < stop_y)
					stop_y = vstop;			// if vstop < stop_y then there is a black border

				yScale = ((float)SCREEN_HEIGHT)/((float)(stop_y-start_y));
				int start_x = min_diwstart;
				int stop_x = max_diwstop;
				if (start_x == 10000)
					start_x = hstrt;  // not initialized yet do an approximation
				if (stop_x == 0)
					stop_x = hstop;
				if (!g_aspect_ratio)
				{ // 4:3
					xScale = yScale;
					dx = (SCREEN_WIDTH - ((stop_x-start_x)*SCREEN_HEIGHT/(stop_y-start_y))) >> 1;
					dx -= (coord_diw_to_window_x(start_x)+56)*SCREEN_HEIGHT/(stop_y-start_y);
				}
				else
				{ //16:9
					xScale = ((float)SCREEN_WIDTH)/((float)(stop_x-start_x));
					dx = -((coord_diw_to_window_x(start_x)+56)*SCREEN_WIDTH/(stop_x-start_x));
				}
				if (start_y > minfirstline) // adjust ofset to cater for black border
					dy = -((start_y-minfirstline)*SCREEN_HEIGHT/(stop_y-start_y));
#ifdef PSPDEBUG
					pspDebug("hstrt = %i, hstop = %i, vstrt = %i, vstop = %i", hstrt, hstop, vstrt, vstop);
					pspDebug("minfirstline = %i, min_diwstart = %i, max_diwstop = %i", minfirstline, min_diwstart,max_diwstop);
					pspDebug("xScale = %1.2f yScale = %1.2f dx = %i dy = %i", xScale, yScale, dx, dy);
#endif
				destHeight = height*yScale;
				fixedWidth = 32*xScale;
			}
		}
		else
		{  // zoom decided by end user
			static int last_g_zoom;
			if (last_g_autozoom != g_autozoom || g_zoom != last_g_zoom)
			{
				last_g_zoom = g_zoom;
				float aspect_ratio = ASPECT43;
				if (g_aspect_ratio > 0)
					aspect_ratio = ASPECT169;
				yScale = zoomfactors[g_zoom];
				xScale = yScale*aspect_ratio;
				destHeight = height*yScale;
				fixedWidth = 32*xScale;
			}
			dx = 0;
			dy = g_ScreenY;
/*
#ifdef DEBUGGER
			if (!g_debug) // center screen
			{
#endif
*/
				int extrapixels = g_amiga_linesize/(12 - g_zoom);	//thinkp
				if (g_zoom == 0) extrapixels = 0;
				dx = (g_ScreenX - (extrapixels/2));				//thinkp
/*
#ifdef DEBUGGER
			}
#endif
*/
		}
		last_g_autozoom = g_autozoom;
		float start, end;
		float destWidth;
		int ddx = dx;
		for (start = sx, end = sx+width; start < end; start += 32)
		{
			// allocate memory on the current display list for temporary storage
			// in order to rotate, we use 4 vertices this time
			Vertex* vertices = (Vertex*)sceGuGetMemory(4 * sizeof(Vertex));
			if ((start + 32) < end)
			{
				width = 32;
				destWidth = fixedWidth;
			}
			else
			{
				width = end-start;
				destWidth = width*xScale;
			}

			vertices[0].u = start;
			vertices[0].v = sy;
			vertices[0].x = ddx;
			vertices[0].y = dy;
			vertices[0].z = 0.0f;

			vertices[2].u = start;
			vertices[2].v = sy + height;
			vertices[2].x = ddx;
			vertices[2].y = dy + destHeight;
			vertices[2].z = 0.0f;

			ddx += destWidth;

			vertices[1].u = start + width;
			vertices[1].v = sy;
			vertices[1].x = ddx;
			vertices[1].y = dy;
			vertices[1].z = 0.0f;

			vertices[3].u = start + width;
			vertices[3].v = sy + height;
			vertices[3].x = ddx;
			vertices[3].y = dy + destHeight;
			vertices[3].z = 0.0f;


			sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_16BIT|GU_COLOR_5551|GU_VERTEX_16BIT|GU_TRANSFORM_2D, 4, 0, vertices);
		}


		sceGuFinish();
		sceGuSync(0, 0);
	}

	if(KeyboardActive)
	{
		if (SolidKeyboard)
			DrawKeyboardD();
		else
			DrawKeyboard();
	}

#ifdef DEBUGGER
	if (changed_prefs.start_debugger)
	{
		int j = current_debug_line;
		int i;

		for (i=0; i<DEBUGLINES; i++)
		{
			if (j >= DEBUGLINES) j = 0;
			text_print( 360, i*9, debug_buffer[j++], rgb2col(155,255,155),rgb2col(0,0,0),0);
		}
	}
#endif
	flipScreen();

	if(do_screen_shot==1)
	{
		SaveScreen();
		do_screen_shot=2;
	}
#ifdef PROFILING
	PROFILINGEND(PROFILINGINDEX+11)
#endif
}
static void graphics_subshutdown (void)
{
    if (gfxvidinfo.emergmem) {
		free (gfxvidinfo.emergmem);
		gfxvidinfo.emergmem = 0;
    }
	sceGuTerm();
}

int graphics_init(void)
{
	initGU();
	//we use a 16-bit surface
	gfxvidinfo.width = AMIGALINESIZE;
	gfxvidinfo.height = AMIGASCREENHEIGHT;
	gfxvidinfo.pixbytes = 2;
	gfxvidinfo.rowbytes = gfxvidinfo.width*gfxvidinfo.pixbytes;
	gfxvidinfo.bufmem = getVramBuffer();
	gfxvidinfo.linemem = 0;
	gfxvidinfo.emergmem = 0; //Ric (char *)malloc(gfxvidinfo.rowbytes);
	gfxvidinfo.maxblocklines = gfxvidinfo.height; //Ric 10000;
	currprefs.gfx_lores = 1;
	gfxvidinfo.flush_screen = psp_flush_screen;
	alloc_colors64k(5,5,5,0,5,10,0,0,0,0);
	return 1;
}

int graphics_setup (void)
{
	return 1;
}

void graphics_leave(void)
{
	graphics_subshutdown ();
}
int check_prefs_changed_gfx (void)
{
#ifdef PROFILING
	PROFILINGBEGIN
#endif
	int changed = 0;
#ifdef DEBUGGER
	if (currprefs.start_debugger != changed_prefs.start_debugger)
	{
		if (changed_prefs.start_debugger )
			g_zoom = 0;
		currprefs.start_debugger = changed_prefs.start_debugger;
		changed = 1;
	}
#endif
	if (g_direct_screen_access != changed_g_direct_screen_access)
	{
		g_direct_screen_access = changed_g_direct_screen_access;
		changed = 1;
    }
	if (changed)
	{
		clearScreen();
		graphics_subshutdown ();
		graphics_init ();
		gui_update_gfx ();
		force_update_frame();
	}
#ifdef PROFILING
	PROFILINGEND(PROFILINGINDEX+12)
#endif
    return changed;
}

//controls stuff
void update_analogstick_config()
{
	if (g_analogstick == AS_MOUSE)
		changed_prefs.jport0 = JSEM_MICE;
	else if (g_analogstick == AS_JOY0)
		changed_prefs.jport1 = JSEM_JOYS;
	else if (g_analogstick == AS_JOY1)
		changed_prefs.jport0 = JSEM_JOYS+1;
	inputdevice_config_change();
}
void update_dirbuttons_config()
{
	if (g_dirbuttons == AS_MOUSE)
		changed_prefs.jport0 = JSEM_MICE;
	else if (g_dirbuttons == AS_JOY0)
		changed_prefs.jport1 = JSEM_JOYS;
	else if (g_dirbuttons == AS_JOY1)
		changed_prefs.jport0 = JSEM_JOYS+1;
	inputdevice_config_change();
}

static SceCtrlData ctl={0,};

static int prev_ctl_buttons=0;

int CheckActive(int this,int that)
{
// thinkp

	if (g_disablekeycombos == 0 )
	{
		int notrigger = !(ctl.Buttons & PSP_CTRL_LTRIGGER || ctl.Buttons & PSP_CTRL_RTRIGGER );

		if(g_cross==that) {if(this&PSP_CTRL_CROSS && notrigger) return(1);}
		if(g_circle==that) {if(this&PSP_CTRL_CIRCLE && notrigger) return(1);}
		if(g_square==that) {if(this&PSP_CTRL_SQUARE && notrigger) return(1);}
		if(g_triangle==that) {if(this&PSP_CTRL_TRIANGLE && notrigger) return(1);}

		if(g_lshoulder==that) {if(this==PSP_CTRL_LTRIGGER) return(1);}
		if(g_rshoulder==that) {if(this==PSP_CTRL_RTRIGGER) return(1);}
	}
	else
	{
		if(g_cross==that) {if(this&PSP_CTRL_CROSS) return(1);}
		if(g_circle==that) {if(this&PSP_CTRL_CIRCLE) return(1);}
		if(g_square==that) {if(this&PSP_CTRL_SQUARE) return(1);}
		if(g_triangle==that) {if(this&PSP_CTRL_TRIANGLE) return(1);}

		if(g_lshoulder==that) {if(this&PSP_CTRL_LTRIGGER) return(1);}
		if(g_rshoulder==that) {if(this&PSP_CTRL_RTRIGGER) return(1);}
	}
	if(g_start==that) {if(this&PSP_CTRL_START) return(1);}

	return CheckActive2(this,that);
// thinkp
}
// thinkp
int CheckActive2(int this,int that)
{
	int ltrigger = this & PSP_CTRL_LTRIGGER;
	int rtrigger = this & PSP_CTRL_RTRIGGER;

	if (g_disablekeycombos == 1 )
		ltrigger = rtrigger = 0;

	if (ltrigger && !rtrigger)
	{
		if(g_lcross==that) {if(this&PSP_CTRL_CROSS) return(1);}
		if(g_lcircle==that) {if(this&PSP_CTRL_CIRCLE) return(1);}
		if(g_lsquare==that) {if(this&PSP_CTRL_SQUARE) return(1);}
		if(g_ltriangle==that) {if(this&PSP_CTRL_TRIANGLE) return(1);}
	}

	if (!ltrigger && rtrigger)
	{
		if(g_rcross==that) {if(this&PSP_CTRL_CROSS) return(1);}
		if(g_rcircle==that) {if(this&PSP_CTRL_CIRCLE) return(1);}
		if(g_rsquare==that) {if(this&PSP_CTRL_SQUARE) return(1);}
		if(g_rtriangle==that) {if(this&PSP_CTRL_TRIANGLE) return(1);}
	}

	if(g_dirbuttons == AS_KEYS)
	{
		if (ltrigger && !rtrigger)
		{
			if(g_lup==that) {if(this&PSP_CTRL_UP) return(1);}
			if(g_ldown==that) {if(this&PSP_CTRL_DOWN) return(1);}
			if(g_lleft==that) {if(this&PSP_CTRL_LEFT) return(1);}
			if(g_lright==that) {if(this&PSP_CTRL_RIGHT) return(1);}
		}

		if (!ltrigger && rtrigger)
		{
			if(g_rup==that) {if(this&PSP_CTRL_UP) return(1);}
			if(g_rdown==that) {if(this&PSP_CTRL_DOWN) return(1);}
			if(g_rleft==that) {if(this&PSP_CTRL_LEFT) return(1);}
			if(g_rright==that) {if(this&PSP_CTRL_RIGHT) return(1);}
		}

		if (!ltrigger && !rtrigger)
		{
			if(g_up==that) {if(this&PSP_CTRL_UP) return(1);}
			if(g_down==that) {if(this&PSP_CTRL_DOWN) return(1);}
			if(g_left==that) {if(this&PSP_CTRL_LEFT) return(1);}
			if(g_right==that) {if(this&PSP_CTRL_RIGHT) return(1);}
		}
	}
	return(0);
}
// thinkp
void InvokeKeyPress(int raw_value)
{
	record_key((AmigaKeyboard[(raw_value-BT_KEY_ENTRIES)].AmigaKeyCode)<<1);
}

void ReleaseKeyPress(int raw_value)
{
	record_key(((AmigaKeyboard[(raw_value-BT_KEY_ENTRIES)].AmigaKeyCode)<<1)|1);
}
#define THRESHOLD 30


//---------------------------- Menu stuff --------------------------------
#define A(color) ((u8)(color >> 15 & 0x01))
#define B(color) ((u8)(color >> 10 & 0x1F))
#define G(color) ((u8)(color >> 5 & 0x1F))
#define R(color) ((u8)(color & 0x1F))

#define	MENU_HEIGHT	10
#define NBFILESPERPAGE 21

typedef u32 Color;
typedef struct
{
	int textureWidth;  // the real width of data, 2^n with n>=0
	int textureHeight;  // the real height of data, 2^n with n>=0
	int imageWidth;  // the image width
	int imageHeight;
	Color* data;
} Image;

//FOL
#define MENUIMAGEFILENAME "GUI/menu.png"
Image* menuImage;

#define MENUIMAGEFILENAME2 "GUI/splash.png"
Image* menuImage2;
//FOL

static int getNextPower2(int width)
{
	int b = width;
	int n;
	for (n = 0; b != 0; n++) b >>= 1;
	b = 1 << n;
	if (b == 2 * width) b >>= 1;
	return b;
}

void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg)
{
}

Image* loadImage(const char* filename)
{
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type, x, y;
	u32* line;
	FILE *fp;
	Image* image = (Image*) malloc(sizeof(Image));
	if (!image) return NULL;

	if ((fp = fopen(filename, "rb")) == NULL) return NULL;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		free(image);
		fclose(fp);
		return NULL;;
	}
	png_set_error_fn(png_ptr, (png_voidp) NULL, (png_error_ptr) NULL, user_warning_fn);
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
		return NULL;
	}
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, sig_read);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, (int *) NULL, (int *) NULL);
	if (width > 512 || height > 512) {
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
		return NULL;
	}
	image->imageWidth = width;
	image->imageHeight = height;
	image->textureWidth = getNextPower2(width);
	image->textureHeight = getNextPower2(height);
	png_set_strip_16(png_ptr);
	png_set_packing(png_ptr);
	if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);;
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
	image->data = (Color*) memalign(16, image->textureWidth * image->textureHeight * sizeof(Color));
	if (!image->data) {
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
		return NULL;
	}
	line = (u32*) malloc(width * 4);
	if (!line) {
		free(image->data);
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
		return NULL;
	}
	for (y = 0; y < height; y++) {
		png_read_row(png_ptr, (u8*) line, (png_bytep) NULL);
		for (x = 0; x < width; x++) {
			u32 color = line[x];
			image->data[x + y * image->textureWidth] =  color;
		}
	}
	free(line);
	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
	fclose(fp);
	return image;
}

void blitAlphaImageToScreen(int sx, int sy, int width, int height, Image* source, int dx, int dy)
{
	if (menuImage)
	{
		sceKernelDcacheWritebackInvalidateAll();
		sceGuStart(GU_DIRECT,list);
		sceGuTexMode(GU_PSM_8888, 0, 0, 0);
		sceGuTexImage(0, source->textureWidth, source->textureHeight, source->textureWidth, (void*) source->data);
		float u = 1.0f / ((float)source->textureWidth);
		float v = 1.0f / ((float)source->textureHeight);
		sceGuTexScale(u, v);

		int j = 0;
		while (j < width) {
			Vertex2* vertices = (Vertex2*) sceGuGetMemory(2 * sizeof(Vertex2));
			int sliceWidth = 64;
			if (j + sliceWidth > width) sliceWidth = width - j;
			vertices[0].u = sx + j;
			vertices[0].v = sy;
			vertices[0].x = dx + j;
			vertices[0].y = dy;
			vertices[0].z = 0;
			j += sliceWidth;

			vertices[1].u = sx + j;
			vertices[1].v = sy + height;
			vertices[1].x = dx + j;
			vertices[1].y = dy + height;
			vertices[1].z = 0;
			sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
		}

		sceGuFinish();
		sceGuSync(0, 0);
	}
	else
	{
		int skipX = LINESIZE - width;
		int x,y;
		u16* data = getVramDrawBuffer() + dx + dy * LINESIZE;
		for (y = 0; y < height; y++, data += skipX)
			for (x = 0; x < width; x++, data++)
				*data = 0;
	}
}

void drawMenuBox(u8 darken, int x0, int y0, int width, int height)
{
	int skipX = LINESIZE - width;
	int x, y;
	u16* data = getVramDrawBuffer() + x0 + y0 * LINESIZE;
	for (y = 0; y < height; y++, data += skipX) {
		for (x = 0; x < width; x++, data++) {
			u16 c = *data;
			u8 r = R(c);
			if (r > darken) r-=darken; else r=0;
			c = *data;
			u8 g = G(c);
			if (g > darken) g-=darken; else g=0;
			c = *data;
			u8 b = B(c);
			if (b > darken) b-=darken; else b=0;
			c=0;
			c = b;
			c = c<<5;
			c+= g;
			c=c<<5;
			c+= r;
			*data = c;
		}
	}
}

void fadeInMenu(u16 lineColor, int x, int y, int width, int height, Image* menuImage)
{
	int i;
	for (i=0;i<=lineColor;i+=2)
	{
		sceDisplayWaitVblankStart();
		blitAlphaImageToScreen(x, y, width*i/lineColor, height*i/lineColor, menuImage, x, y);
		drawMenuBox(i, x, y, width*i/lineColor, height*i/lineColor);
		drawLineScreen(x, y, x+width*i/lineColor, y, (Color)i);
		drawLineScreen(x+width*i/lineColor, y, x+width*i/lineColor, y+height*i/lineColor, (Color)i);
		drawLineScreen(x+width*i/lineColor, y+height*i/lineColor, x, y+height*i/lineColor, (Color)i);
		drawLineScreen(x, y+height*i/lineColor, x, y, (Color)i);
		flipScreen();
	}
    // now we need to ensure both buffers are filled with equal data so we run one last time
	sceDisplayWaitVblankStart();
	int j=0;i=18;
	for (j=0;j<2;j++)
	{
		blitAlphaImageToScreen(x, y, width, height, menuImage, x, y);
		drawMenuBox(i, x, y, width, height);
		drawLineScreen(x, y, x+width, y, (Color)i);
		drawLineScreen(x+width, y, x+width, y+height, (Color)i);
		drawLineScreen(x+width, y+height, x, y+height, (Color)i);
		drawLineScreen(x, y+height, x, y, (Color)i);
		flipScreen();
	}
}
void removeMenu(int x, int y, int width, int height, Image* menuImage)
{
	int j=0;
	sceDisplayWaitVblankStart();
	for (j=0;j<2;j++)
	{
		blitAlphaImageToScreen(x, y, width+1, height+1, menuImage, x, y);
		flipScreen();
	}
}

void removeMenu2(const char **menu, int x)
{
	int dx = 0, dy = 0, i;
	for(i=0;menu[i];i++)
	{
// thinkp
		if (strlen((char*)menu[i]) == 0) continue;
// thinkp
		if (i<NBFILESPERPAGE) dy+= MENU_HEIGHT;
		if (strlen((char*)menu[i]) > dx) dx = strlen((char*)menu[i]);
	}
	dx*=5;
	dx+=10;
	dy+=8;
	int y = (272 - dy)/2;  //center the menu vertically
	if (x+dx > 480) // make sure there is room for the menu
	{
		x = 479 - dx;
		if (x < 0)
		{
			x = 0;
			dx = 480;
		}
	}
	removeMenu(x, y, dx, dy, menuImage);
}
void removeMenu3(const char **menu, int x)
{
	int dx = 478-x, dy = 0, i;
	for(i=0;menu[i];i++)
	{
		if (i<NBFILESPERPAGE) dy+= MENU_HEIGHT;
	}
	dy+=8;
	int y = (272 - dy)/2;  //center the menu vertically
	removeMenu(x, y, dx, dy, menuImage);
}

void removeMenu4(int x)
{
	int dx = 0, dy = 0;
	dy = NBFILESPERPAGE*MENU_HEIGHT+8;
	int y = (272 - dy)/2;  //center the menu vertically
	dx = 479-x;
	removeMenu(x, y, dx, dy, menuImage);
}

void inactivateMenu(u16 lineColor, int x, int y, int width, int height, Image* menuImage)
{
	sceDisplayWaitVblankStart();
	int j=0;
	for (j=0;j<2;j++)
	{
		drawLineScreen(x, y, x+width, y, lineColor);
		drawLineScreen(x+width, y, x+width, y+height, lineColor);
		drawLineScreen(x+width, y+height, x, y+height, lineColor);
		drawLineScreen(x, y+height, x, y, lineColor);
		flipScreen();
	}
}
void activateMenu(u16 lineColor, int x, int y, int width, int height, Image* menuImage)
{
	sceDisplayWaitVblankStart();
	int j=0;
	for (j=0;j<2;j++)
	{
		//add 9 pixes to the width to cater for the line length changing
		//when the user switches an option  On/Off
		blitAlphaImageToScreen(x, y, width+9, height+1, menuImage, x, y);
		drawMenuBox(lineColor, x, y, width, height);
		drawLineScreen(x, y, x+width, y, (Color)lineColor);
		drawLineScreen(x+width, y, x+width, y+height, (Color)lineColor);
		drawLineScreen(x+width, y+height, x, y+height, (Color)lineColor);
		drawLineScreen(x, y+height, x, y, (Color)lineColor);
		flipScreen();
	}
}

static void drawLine(int x0, int y0, int x1, int y1, int color, u16* destination, int width)
{
	int dy = y1 - y0;
	int dx = x1 - x0;
	int stepx, stepy;

	if (dy < 0) { dy = -dy;  stepy = -width; } else { stepy = width; }
	if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }
	dy <<= 1;
	dx <<= 1;

	y0 *= width;
	y1 *= width;
	destination[x0+y0] = color;
	if (dx > dy) {
		int fraction = dy - (dx >> 1);
		while (x0 != x1) {
			if (fraction >= 0) {
				y0 += stepy;
				fraction -= dx;
			}
			x0 += stepx;
			fraction += dy;
			destination[x0+y0] = color;
		}
	} else {
		int fraction = dx - (dy >> 1);
		while (y0 != y1) {
			if (fraction >= 0) {
				x0 += stepx;
				fraction -= dy;
			}
			y0 += stepy;
			fraction += dx;
			destination[x0+y0] = color;
		}
	}
}

void drawLineScreen(int x0, int y0, int x1, int y1, u16 color)
{
	drawLine(x0, y0, x1, y1, color, getVramDrawBuffer(), LINESIZE);
}

float __freemem()
{
 void *ptrs[480];
 int mem, x, i;
 float rmem;
 for (x = 0; x < 480; x++)
 {
    void *ptr = malloc(51200);
    if (!ptr) break;

    ptrs[x] = ptr;
 }
 mem = x * 51200;
 for (i = 0; i < x; i++)
  free(ptrs[i]);

 rmem = ((float)mem)/((float)1048576);
 return rmem;
}

void splashScreen()
{
	clearScreen();
	int y0=0;
	if (menuImage)
	{
		int x0 = 0;
		for (y0=0;y0<=138;y0+=6)
		{
			if (y0>136) y0=136;
			sceDisplayWaitVblankStart();
			blitAlphaImageToScreen(480-x0,136-y0 ,x0,y0*2, menuImage, 0, 136-y0);
			flipScreen();
			x0+=24;
			if (x0>480) x0 = 480;
		}
	}
	else
	{
		for (y0=0;y0<10;y0++)
			sceDisplayWaitVblankStart(); //short pause to allow end user to release the select key
	}

	char	msg[255];

	sprintf(msg, "PSPUAE " VERSION " (www.pspuae.com)      FreeMem: %.2f MB", __freemem() );

	sceDisplayWaitVblankStart();
	blitAlphaImageToScreen(0,0 ,480,272, menuImage, 0, 0);
	text_print( 0, 0, msg, rgb2col(155,255,155),rgb2col(0,0,0),0);
	flipScreen();
	text_print( 0, 0, msg, rgb2col(155,255,155),rgb2col(0,0,0),0);
	flipScreen();
}

//FOL
void splashScreen2()
{
	clearScreen();
	int y0=0;
	if (menuImage2)
	{
		int x0 = 0;
		for (y0=0;y0<=138;y0+=6)
		{
			if (y0>136) y0=136;
			sceDisplayWaitVblankStart();
			blitAlphaImageToScreen(480-x0,136-y0 ,x0,y0*2, menuImage2, 0, 136-y0);
			flipScreen();
			x0+=24;
			if (x0>480) x0 = 480;
		}
	}

}
//FOL

void fadeOutImage()
{
	int i;
	for (i=0;i<24;i++)
	{
		sceDisplayWaitVblankStart();
//		blitAlphaImageToScreen(x, y, width*i/lineColor, height*i/lineColor, menuImage, x, y);
		drawMenuBox(3, 0, 0, 480, 272);
		flipScreen();
	}

}

/*
void parse_hardfile_spec (char *spec)
{
    char *x0 = my_strdup (spec);
    char *x1, *x2, *x3, *x4, *x5, *res;
	char	message[255];

    x1 = strchr (x0, ',');
    if (x1 == NULL)
	goto argh;
    *x1++ = '\0';
    x2 = strchr (x1 + 1, ',');
    if (x2 == NULL)
	goto argh;
    *x2++ = '\0';
    x3 = strchr (x2 + 1, ',');
    if (x3 == NULL)
	goto argh;
    *x3++ = '\0';
    x4 = strchr (x3 + 1, ',');
    if (x4 == NULL)
	goto argh;
    *x4++ = '\0';
    x5 = strchr (x4 + 1, ',');
    if (x5 == NULL)
	goto argh;
    *x5++ = '\0';
    res = 0;
	write_log("%s %s %s %s %s %s", x0, x1, x2, x3, x4, x5);

	#ifdef FILESYS
    res = add_filesys_unit (currprefs.mountinfo, NULL, NULL, x1, atoi (x0), atoi (x2), atoi (x3), atoi (x4), atoi (x5), 0, 0);
	//add_filesys_unit (currprefs.mountinfo, 0, "DH0", "host0:/DISKS/DNA.hdf", 1, 32, 1, 2, 512, 0, 0);
	//res = add_filesys_unit (currprefs.mountinfo, "DH0:", "Workbench", "host0:/WB", 0, 0, 0, 0, 0, 0, 0);
#endif
    if (res)
		write_log ("%s\n", res);

    free (x0);
    return;

 argh:
    free (x0);
    write_log ("Bad hardfile parameter specified - type \"uae -h\" for help.\n");
    return;
}
const char *add_filesys_unit (struct uaedev_mount_info *mountinfo,
			char *devname, char *volname, char *rootdir, int readonly,
			int secspertrack, int surfaces, int reserved,
			int blocksize, int bootpri, char *filesysdir)
*/

int changeKickstart(int version)
{
	strcpy(changed_prefs.romfile, g_path);

	switch (version & 0xff)
	{
	case 0:
		strcat(changed_prefs.romfile, "/KICKS/KICK10.ROM");
		break;
	case 1:
		strcat(changed_prefs.romfile, "/KICKS/KICK11.ROM");
		break;
	case 2:
		strcat(changed_prefs.romfile, "/KICKS/KICK12.ROM");
		break;
	case 4:
		strcat(changed_prefs.romfile, "/KICKS/KICK20.ROM");
		break;
	case 5:
		strcat(changed_prefs.romfile, "/KICKS/KICK204.ROM");
		break;
	case 6:
		strcat(changed_prefs.romfile, "/KICKS/KICK205.ROM");
		break;
	case 7:
		strcat(changed_prefs.romfile, "/KICKS/KICK30.ROM");
		break;
	case 8:
		strcat(changed_prefs.romfile, "/KICKS/KICK31.ROM");
		break;
	case 3:
	default:
		strcat(changed_prefs.romfile, "/KICKS/KICK13.ROM");
		break;
	}
	SceUID fd;
	if ((fd = sceIoOpen(changed_prefs.romfile, PSP_O_RDONLY, 0777))<0)
	{
		char msg[128];
		sprintf(msg,"'%s' not found.", changed_prefs.romfile );
		pspDebug(msg);
		if (version < 256)
			gui_message(msg);
		return -1;
#if 0
		//file not found - try kick13.rom
		g_kickstart_rom = 3;
		strcpy(changed_prefs.romfile, g_path);
		strcat(changed_prefs.romfile, "/KICKS/KICK13.ROM");
		if ((fd = sceIoOpen(changed_prefs.romfile, PSP_O_RDONLY, 0777))<0)
		{
			sprintf(msg,"'%s' not found. You need to provide a valid kickstart rom.", changed_prefs.romfile );
			pspDebug(msg);
			gui_message(msg);
			return -1;
		}
#endif
	}
	sceIoClose(fd);
	return 0;
}

int A500PSPUAEOptions(void)
{
	DefaultPSPUAEOptions();

	g_kickstart_rom = 3; // 1.3
	if(changeKickstart(g_kickstart_rom))
		return -1;

	changed_prefs.collision_level = 2; //Playfields
	changed_prefs.cpu_level = 0; //68000
	changed_prefs.cpu_compatible = 1; //ON
	changed_prefs.m68k_speed = -1; // max speed
	changed_prefs.fastmem_size = 0; //OFF
	changed_prefs.chipmem_size = 512 * 1024; //512
	changed_prefs.bogomem_size = 512 * 1024; //512
	changed_prefs.chipset_mask = 0; //OCS
	changed_prefs.floppy_speed = 100;  //Normal
	changed_prefs.immediate_blits = 1; //ON
	return 0;
}

int A600PSPUAEOptions(void)
{
	DefaultPSPUAEOptions();

	g_kickstart_rom = 6; // 2.05
	if(changeKickstart(g_kickstart_rom))
		return -1;

	changed_prefs.collision_level = 2; //Playfields
	changed_prefs.cpu_level = 1; //68010
	changed_prefs.cpu_compatible = 1; //ON
	changed_prefs.m68k_speed = -1; // max speed
	changed_prefs.fastmem_size = 0; //OFF
	changed_prefs.chipmem_size = 1024 * 1024; //1024
	changed_prefs.bogomem_size = 0; //OFF
	changed_prefs.chipset_mask = 2; //ECS Denise
	changed_prefs.immediate_blits = 1; //ON
	return 0;
}

int A1000PSPUAEOptions(void)
{
	DefaultPSPUAEOptions();

	g_kickstart_rom = 2; // 1.2
	if(changeKickstart(g_kickstart_rom))
		return -1;

	changed_prefs.collision_level = 2; //Playfields
	changed_prefs.cpu_level = 0; //68000
	changed_prefs.cpu_compatible = 1; //ON
	changed_prefs.m68k_speed = -1; // max speed
	changed_prefs.fastmem_size = 0; //OFF
	changed_prefs.chipmem_size = 512 * 1024; //512
	changed_prefs.bogomem_size = 0; //OFF
	changed_prefs.chipset_mask = 0; //OCS
	changed_prefs.immediate_blits = 1; //ON
	return 0;
}

int A1200PSPUAEOptions(void)
{
	DefaultPSPUAEOptions();

	g_kickstart_rom = 7; // 3.0
	if(changeKickstart(g_kickstart_rom))
		return -1;

	changed_prefs.cpu_level = 2; //68020
	changed_prefs.cpu_compatible = 1; //ON
	changed_prefs.m68k_speed = -1; // max speed
	changed_prefs.fastmem_size = 0; //OFF
	changed_prefs.chipmem_size = 1024 * 2048; //2048
	changed_prefs.bogomem_size = 0;
	changed_prefs.chipset_mask = 3; //ECS
	changed_prefs.immediate_blits = 1; //ON
	return 0;
}

void DefaultPSPUAEOptions(void)
{
	g_solidkeyboard = 0;
	g_analogstick = AS_MOUSE;
	g_dirbuttons = AS_JOY0;
	g_start = BT_SCREEN_SHOT;
	g_square = BT_ACTIVATE_KEYBOARD;
	g_triangle = BT_HIRES_MOUSE;
	g_cross = BT_JOY0_FIRE;
	g_circle = BT_MIDDLE_MOUSE;
	g_lshoulder = BT_LEFT_MOUSE;
	g_rshoulder = BT_RIGHT_MOUSE;
#ifdef ADVANCED_FS
	g_frameskip = 0; //auto frameskip
#else
	g_frameskip = 9; //auto frameskip
#endif
	changed_prefs.gfx_framerate=5;
	changed_prefs.produce_sound=2; //normal
	changed_prefs.sound_stereo = 0;	//Stereo
	changed_prefs.sound_interpol = 0; //None FOL (Added Interpol to Sound Menu)
	g_zoom = 0; //no zoom thinkp
	changed_g_direct_screen_access = 0; //OFF
	//g_kickstart_rom = 3; // 1.3
	//changeKickstart(g_kickstart_rom);

	changed_prefs.collision_level = 2; //Playfields
	changed_prefs.cpu_level = 0; //M68k
	changed_prefs.cpu_idle = 0; // OFF FOL
	changed_prefs.dont_busy_wait = 1; //ON FOL
	changed_prefs.cpu_compatible = 0; //OFF
	changed_prefs.m68k_speed = -1; // max speed
	changed_prefs.fastmem_size = 0; //OFF
	changed_prefs.chipmem_size = 512 * 1024; //512kb
	changed_prefs.bogomem_size = 512 * 1024; //512kb
	changed_prefs.chipset_mask = CSMASK_ECS_AGNUS; //ECS Super AGNUS
	changed_prefs.leds_on_screen = 1; //Floppy, Power, FPS, etc etc.
#ifdef DEBUGGER
	//changed_prefs.start_debugger = 0;
#endif
	update_analogstick_config();
	update_dirbuttons_config();
	changed_prefs.floppy_speed = 100; //Floppy Speed Normal
	changed_prefs.immediate_blits = 0; //OFF
	changed_prefs.blitter_cycle_exact = 0; //OFF
	changed_prefs.gfx_correct_aspect = 0; //OFF FOL (This doesnt work any more) FOL
	changed_prefs.dfxtype[0] = 0; //Drive Type Double Density, can be set to High Density 1.7Megs
	changed_prefs.dfxtype[1] = 0; //Drive Type Double Density, can be set to High Density 1.7Megs
	changed_prefs.tod_hack = 1; //ON, Syncronyse Clocks FOL
	changed_prefs.gfx_xcenter = 1; //Simple   Horizontal Center FOL
	changed_prefs.gfx_ycenter = 1;  //Simple   Vertical Center FOL
	changed_prefs.gfx_width_fs = 480;  //FOL
	changed_prefs.gfx_height_fs = 272;  //FOL
	changed_prefs.gfx_afullscreen = 1; //ON    Amiga FullScreen
	changed_prefs.sound_stereo_separation = 0;

	changed_prefs.chipset_refreshrate = 50; //Chipset Refresh Rate

// thinkp
	g_up = -1;
	g_down = -1;
	g_left = -1;
	g_right = -1;
	g_lup = -1;
	g_ldown = -1;
	g_lleft = -1;
	g_lright = -1;
	g_rup = -1;
	g_rdown = -1;
	g_rleft = -1;
	g_rright = -1;
	g_lsquare = -1;
	g_ltriangle = -1;
	g_lcross = -1;
	g_lcircle = -1;
	g_rsquare = -1;
	g_rtriangle = -1;
	g_rcross = -1;
	g_rcircle = -1;
	g_mouse_speed = 3;
	g_hr_mouse_speed = 5;

	g_disablekeycombos=0;	// key combos enabled
	//g_togglexo=0;			global option
	g_cpu2chip_ratio=8;	// 45% chipset
	g_automousespeed=5;		// slow

	g_aspect_ratio=1;		// normal43
	//g_ScreenX=-65; //FOL Added check instead, to solve problem with normal zoom modes being off center.
	if (g_zoom >0)
		g_ScreenX = 15;
	else
		g_ScreenX = -65;
	g_ScreenY=0;
	g_amiga_linesize=480;
	g_screenlock=0;
	g_autozoom=1;
	g_bgnd_image=1;
//	g_audio_opt=0;
#ifdef CYCLE_SWITCH
	g_cpuspeed_up=1;
	g_cputurbo=512;
	g_chipsetturbo=1;
	g_cpulock=1;
#endif

#ifdef SOUND_DELAY
int     g_sdswitch=1;
int		g_snddelay=1000;
#endif

// thinkp mousestates
	InitMouseStates();
// thinkp mousestates
}

int LoadPSPUAEOptions(int slot, char* diskname, char* path, int load_global_options)
{
	SceUID	fdin;
	int	lngth,c;

	LaunchPath[LaunchPathEndPtr]='\0';
	if (slot == -1)
	{
		if (strlen(path) == 0 )
			sprintf(TmpFileName,"%sCONFIGS/%s.options",LaunchPath,diskname);
		else
		{
			sprintf(TmpFileName,"%s%s.options",path,diskname);
		}
	}
	else
		sprintf(TmpFileName,"%sCONFIGS/pspuae%01d.options",LaunchPath,(slot));

	if((fdin = sceIoOpen(TmpFileName, PSP_O_RDONLY, 0777))<0) return(1);

	DefaultPSPUAEOptions();

	memset(TmpFileName,0,sizeof(TmpFileName));

	lngth=sceIoLseek32(fdin,0,PSP_SEEK_END);
	if(lngth>1023) lngth=1023;
	sceIoLseek32(fdin,0,PSP_SEEK_SET);
	sceIoRead(fdin,TmpFileName,lngth);
	sceIoClose(fdin);
	c=0;
// thinkp
	int version=0;
	version=(int)((unsigned char)TmpFileName[c++]);
	if ( version < 10 )
	{
	// pre 0.62 options file
		g_frameskip = version;
		version = 0;
	}
	else
	{
		g_frameskip=(int)((unsigned char)TmpFileName[c++]);
	}
// thinkp
	g_solidkeyboard=(int)((unsigned char)TmpFileName[c++]);
	g_analogstick=(int)((unsigned char)TmpFileName[c++]);
	g_dirbuttons=(int)((unsigned char)TmpFileName[c++]);
	g_start=(int)((unsigned char)TmpFileName[c++]);
	g_square=(int)((unsigned char)TmpFileName[c++]);
	g_triangle=(int)((unsigned char)TmpFileName[c++]);
	g_cross=(int)((unsigned char)TmpFileName[c++]);
	g_circle=(int)((unsigned char)TmpFileName[c++]);
	g_lshoulder=(int)((unsigned char)TmpFileName[c++]);
	g_rshoulder=(int)((unsigned char)TmpFileName[c++]);
	g_zoom= (int)((unsigned char)TmpFileName[c++]);
	changed_g_direct_screen_access = (int)((unsigned char)TmpFileName[c++]);
	g_kickstart_rom = (int)((unsigned char)TmpFileName[c++]);
	changed_prefs.produce_sound=(int)((unsigned char)TmpFileName[c++]);
	changed_prefs.sound_stereo = (int)((unsigned char)TmpFileName[c++]);
	changed_prefs.collision_level = (int)((unsigned char)TmpFileName[c++]);
	changed_prefs.cpu_level = (int)((unsigned char)TmpFileName[c++]);
	int m68kspeed = ((int)((unsigned char)TmpFileName[c++]))-1;
	changed_prefs.fastmem_size = (int)((unsigned char)TmpFileName[c++])*512*1024;
	changed_prefs.chipmem_size = (int)((unsigned char)TmpFileName[c++])*512*1024;
	changed_prefs.bogomem_size = (int)((unsigned char)TmpFileName[c++])*512*1024;
	changed_prefs.chipset_mask = (int)((unsigned char)TmpFileName[c++]);
	changed_prefs.leds_on_screen = (int)((unsigned char)TmpFileName[c++]);
#ifdef DEBUGGER
	changed_prefs.start_debugger = (int)((unsigned char)TmpFileName[c++]);
#else
	TmpFileName[c++];
#endif
	changed_prefs.jport0 = (int)((unsigned char)TmpFileName[c++]);
	changed_prefs.jport1 = (int)((unsigned char)TmpFileName[c++]);
	changed_prefs.floppy_speed = ((int)((unsigned char)TmpFileName[c++]))*100;
	changed_prefs.immediate_blits = (int)((unsigned char)TmpFileName[c++]);
	changed_prefs.blitter_cycle_exact = (int)((unsigned char)TmpFileName[c++]);
	changed_prefs.dfxtype[0] = (int)((unsigned char)TmpFileName[c++]);
	changed_prefs.dfxtype[1] = (int)((unsigned char)TmpFileName[c++]);

// thinkp
	// 0.61
	g_mouse_speed= (int)((unsigned char)TmpFileName[c++]);
	g_hr_mouse_speed= (int)((unsigned char)TmpFileName[c++]);
	g_up= (int)((unsigned char)TmpFileName[c++]);
	g_down= (int)((unsigned char)TmpFileName[c++]);
	g_left= (int)((unsigned char)TmpFileName[c++]);
	g_right= (int)((unsigned char)TmpFileName[c++]);
	g_lup= (int)((unsigned char)TmpFileName[c++]);
	g_ldown= (int)((unsigned char)TmpFileName[c++]);
	g_lleft= (int)((unsigned char)TmpFileName[c++]);
	g_lright= (int)((unsigned char)TmpFileName[c++]);
	g_rup= (int)((unsigned char)TmpFileName[c++]);
	g_rdown= (int)((unsigned char)TmpFileName[c++]);
	g_rleft= (int)((unsigned char)TmpFileName[c++]);
	g_rright= (int)((unsigned char)TmpFileName[c++]);

	// 0.62 - version 1.0
	if(version>=10)
	{
		g_lsquare=(int)((unsigned char)TmpFileName[c++]);
		g_ltriangle=(int)((unsigned char)TmpFileName[c++]);
		g_lcross=(int)((unsigned char)TmpFileName[c++]);
		g_lcircle=(int)((unsigned char)TmpFileName[c++]);
		g_rsquare=(int)((unsigned char)TmpFileName[c++]);
		g_rtriangle=(int)((unsigned char)TmpFileName[c++]);
		g_rcross=(int)((unsigned char)TmpFileName[c++]);
		g_rcircle=(int)((unsigned char)TmpFileName[c++]);
	}

	// 0.62 BETA 4 - version 1.1
	if(version>=11)
	{
	// FOL
		changed_prefs.sound_interpol = (int)((unsigned char)TmpFileName[c++]);
		changed_prefs.gfx_correct_aspect = (int)((unsigned char)TmpFileName[c++]);
	// FOL
	}

	// 0.62 BETA 8 - version 1.2
	int togglexo=0;	// global option

	if(version>=12)
	{
	// FOL
		changed_prefs.cpu_idle = (int)((unsigned char)TmpFileName[c++]);
	// FOL
	// thinkp
		g_disablekeycombos=(int)((unsigned char)TmpFileName[c++]);
		togglexo=(int)((unsigned char)TmpFileName[c++]);
		g_cpu2chip_ratio=(int)((unsigned char)TmpFileName[c++]);
		g_automousespeed=(int)((unsigned char)TmpFileName[c++]);
	// thinkp
	}

	// 0.62 BETA 9 - version 1.3
	if(version>=13)
	{
	// thinkp
		int sign, msb, lsb;
		g_aspect_ratio=(int)((unsigned char)TmpFileName[c++]);
		sign = (int)((unsigned char)TmpFileName[c++]);
		msb = (int)((unsigned char)TmpFileName[c++]);
		lsb = (int)((unsigned char)TmpFileName[c++]);
		g_ScreenX=(int)(msb*255+lsb);
		if(sign==0) g_ScreenX=-g_ScreenX;
		sign = (int)((unsigned char)TmpFileName[c++]);
		msb = (int)((unsigned char)TmpFileName[c++]);
		lsb = (int)((unsigned char)TmpFileName[c++]);
		g_ScreenY=(int)(msb*255+lsb);
		if(sign==0) g_ScreenY=-g_ScreenY;
		msb = (int)((unsigned char)TmpFileName[c++]);
		lsb = (int)((unsigned char)TmpFileName[c++]);
		g_amiga_linesize=(int)(msb*255+lsb);
		g_screenlock=(int)((unsigned char)TmpFileName[c++]);
	// thinkp
	}

	if (version >= 0)
	{
		if (g_zoom >0)	
			g_ScreenX = g_ScreenX;
		else
			g_ScreenX = -65;
	}

	// 0.62 BETA 10 - version 1.4
	if(version>=14)
	{
	// thinkp
		int i=0,j=0;
		int msb, lsb;
		for(i=0; i<24;i++)
		{
			mousestates[i].mbutton=(int)((unsigned char)TmpFileName[c++]);
			msb = (int)((unsigned char)TmpFileName[c++]);
			lsb = (int)((unsigned char)TmpFileName[c++]);
			mousestates[i].x=(int)((msb<<8)|lsb);
			msb = (int)((unsigned char)TmpFileName[c++]);
			lsb = (int)((unsigned char)TmpFileName[c++]);
			mousestates[i].y=(int)((msb<<8)|lsb);

			for(j=0; j<21;j++)
				mousestates[i].name[j]=((unsigned char)TmpFileName[c++]);
		}
	}

	// 0.62 BETA 12 - version 1.5
	if(version>=15)
	{
	// thinkp
		g_autozoom = (int)((unsigned char)TmpFileName[c++]);
		g_bgnd_image = (int)((unsigned char)TmpFileName[c++]);
	// thinkp
	}
	else
		g_autozoom = 0;

#ifdef ADVANCED_FS
	if (version < 16)
	{
		// we need to adjust old frameskip factor to the new frameskip system
		// old frameskip system is show every nth frame
		// new frameskip system is skip n in 50/60 frames
		if (g_frameskip == 9) // autoframeskip
			g_frameskip = 0;
		else if (g_frameskip > 0)
			g_frameskip = 51 - 50/(g_frameskip+1);
	}
#endif

	if(version>=16)
	{
	// FOL
//		g_audio_opt = (int)((unsigned char)TmpFileName[c++]);
		changed_prefs.sound_stereo_separation = (int)((unsigned char)TmpFileName[c++]);
	}

	if (version < 17)
	{
		if (changed_prefs.sound_stereo > 0) // audio
			changed_prefs.sound_stereo = 0;
	}

#ifdef CYCLE_SWITCH
	if(version>=17)
	{
		g_cpuspeed_up = (int)((unsigned char)TmpFileName[c++]);
		g_cpulock = (int)((unsigned char)TmpFileName[c++]);
	}

	if(version<17)
	{
		g_cpulock = 1;
	}

	if (version > 0)
	{
		if (g_cpuspeed_up == 1){
			g_cputurbo = 512;
			g_chipsetturbo = 1;
		}
		else
		if (g_cpuspeed_up == 0){
			g_cputurbo = 256;
#ifdef HIGH_CHIPSET
			g_chipsetturbo = 16384;
#else
			g_chipsetturbo = 8192;
#endif
		}
	}
#endif

#ifdef SOUND_DELAY
	if(version>=17)
	{
		g_sdswitch = (int)((unsigned char)TmpFileName[c++]);
	}

	if (version<=16)
	{
		if (g_sdswitch == 1){
			g_snddelay = 9000;
		}
		else
		if (g_sdswitch == 0){
			g_cputurbo = 1000;
		}
	}

#endif

	if(g_bgnd_image == 0)
	{
//FOL - No background present fix
		free(menuImage);
		menuImage=NULL;
//FOL
	}
	else
	if(menuImage==NULL)
	{
		menuImage = loadImage(MENUIMAGEFILENAME);
	}

	if (m68kspeed>0)
		changed_prefs.m68k_speed = m68k_speed[g_cpu2chip_ratio];
	else
		changed_prefs.m68k_speed = m68kspeed;

	if(load_global_options==1)
	{
	// load global options
		g_togglexo = togglexo;
	}

// range checking
	if ( g_mouse_speed <= 0 || g_mouse_speed > 5 )
		g_mouse_speed = 3;
	if ( g_hr_mouse_speed <= 0 || g_hr_mouse_speed > 5 )
		g_hr_mouse_speed = 5;
	if ( g_automousespeed <= 0 || g_automousespeed > 5 )
		g_automousespeed = 5;

	int maxkey = BT_KEY_ENTRIES + 129;

	if (g_start > maxkey)
		g_start = -1;
	if (g_lshoulder > maxkey)
		g_lshoulder = -1;
	if (g_rshoulder > maxkey)
		g_rshoulder = -1;
	if (g_up > maxkey)
		g_up = -1;
	if (g_down > maxkey)
		g_down = -1;
	if (g_left > maxkey)
		g_left = -1;
	if (g_right > maxkey)
		g_right = -1;
	if (g_lup > maxkey)
		g_lup = -1;
	if (g_ldown > maxkey)
		g_ldown = -1;
	if (g_lleft > maxkey)
		g_lleft = -1;
	if (g_lright > maxkey)
		g_lright = -1;
	if (g_rup > maxkey)
		g_rup = -1;
	if (g_rdown > maxkey)
		g_rdown = -1;
	if (g_rleft > maxkey)
		g_rleft = -1;
	if (g_rright > maxkey)
		g_rright = -1;

	if (g_square > maxkey)
		g_square = -1;
	if (g_triangle > maxkey)
		g_triangle = -1;
	if (g_cross > maxkey)
		g_cross = -1;
	if (g_circle > maxkey)
		g_circle = -1;
	if (g_lsquare > maxkey)
		g_lsquare = -1;
	if (g_ltriangle > maxkey)
		g_ltriangle = -1;
	if (g_lcross > maxkey)
		g_lcross = -1;
	if (g_lcircle > maxkey)
		g_lcircle = -1;
	if (g_rsquare > maxkey)
		g_rsquare = -1;
	if (g_rtriangle > maxkey)
		g_rtriangle = -1;
	if (g_rcross > maxkey)
		g_rcross = -1;
	if (g_rcircle > maxkey)
		g_rcircle = -1;
// thinkp

	MouseStatesAvailable();
#ifdef ADVANCED_FS
	changed_prefs.gfx_framerate	= g_frameskip-1;
	if (changed_prefs.gfx_framerate < 0)
		changed_prefs.gfx_framerate = 0;
#else
	changed_prefs.gfx_framerate = g_frameskip+1;
#endif
	currprefs.leds_on_screen = changed_prefs.leds_on_screen;
	update_analogstick_config();
	update_dirbuttons_config();
	changeKickstart(g_kickstart_rom);
	return(0);
}

int SavePSPUAEOptions(int slot, char* diskname, char* path)
{
	SceUID	fdout;
	int	c;

	LaunchPath[LaunchPathEndPtr]='\0';

// thinkp
	// check if the configs dir exists
	create_dir("CONFIGS");
// thinkp

	if ( strlen(path) > 0 )
	{
	// save options in /path
		if (slot == -1)
		// save named options
			sprintf(TmpFileName,"%s%s%s.options",LaunchPath,path,diskname);
		else
		// save numbered options
			sprintf(TmpFileName,"%s%spspuae%01d.options",LaunchPath,path,(slot));
	}
	else
	{
	// save options in /CONFIGS
		if (slot == -1)
		// save named options
			sprintf(TmpFileName,"%sCONFIGS/%s.options",LaunchPath,diskname);
		else
		// save numbered options
			sprintf(TmpFileName,"%sCONFIGS/pspuae%01d.options",LaunchPath,(slot));
	}

	if((fdout = sceIoOpen(TmpFileName, PSP_O_WRONLY | PSP_O_CREAT, 0777))<0) return(1);

	c=0;
// thinkp
	// 0.62				- version 1.0 : int version = 10;
	// 0.62 BETA 4		- version 1.1 : int version = 11;
	// 0.62 BETA 8		- version 1.2 : int version = 12;
	// 0.62 BETA 9		- version 1.3 : int version = 13;
	// 0.62 BETA 10		- version 1.4 : int version = 14;
	// 0.62 BETA 12		- version 1.5 : int version = 15;
	// 0.6x				- version 1.y : int version = 1y;
	int version = 17;
	// save version number
	TmpFileName[c++]=(unsigned char)version;
// thinkp

	TmpFileName[c++]=(unsigned char)g_frameskip;
	TmpFileName[c++]=(unsigned char)g_solidkeyboard;
	TmpFileName[c++]=(unsigned char)g_analogstick;
	TmpFileName[c++]=(unsigned char)g_dirbuttons;
	TmpFileName[c++]=(unsigned char)g_start;
	TmpFileName[c++]=(unsigned char)g_square;
	TmpFileName[c++]=(unsigned char)g_triangle;
	TmpFileName[c++]=(unsigned char)g_cross;
	TmpFileName[c++]=(unsigned char)g_circle;
	TmpFileName[c++]=(unsigned char)g_lshoulder;
	TmpFileName[c++]=(unsigned char)g_rshoulder;
	TmpFileName[c++]=(unsigned char)g_zoom;
	TmpFileName[c++]=(unsigned char)changed_g_direct_screen_access;
	TmpFileName[c++]=(unsigned char)g_kickstart_rom;
	TmpFileName[c++]=(unsigned char)changed_prefs.produce_sound;
	TmpFileName[c++]=(unsigned char)changed_prefs.sound_stereo;
	TmpFileName[c++]=(unsigned char)changed_prefs.collision_level;
	TmpFileName[c++]=(unsigned char)changed_prefs.cpu_level;
	if (changed_prefs.m68k_speed > 0)
		TmpFileName[c++]=(unsigned char)2;
	else
		TmpFileName[c++]=(unsigned char)(changed_prefs.m68k_speed+1);
	TmpFileName[c++]=(unsigned char)(changed_prefs.fastmem_size/(512*1024));
	TmpFileName[c++]=(unsigned char)(changed_prefs.chipmem_size/(512*1024));
	TmpFileName[c++]=(unsigned char)(changed_prefs.bogomem_size/(512*1024));
	TmpFileName[c++]=(unsigned char)changed_prefs.chipset_mask;
	TmpFileName[c++]=(unsigned char)changed_prefs.leds_on_screen;
#ifdef DEBUGGER
	TmpFileName[c++]=(unsigned char)changed_prefs.start_debugger;
#else
	TmpFileName[c++]=(unsigned char)0;
#endif
	TmpFileName[c++]=(unsigned char)changed_prefs.jport0;
	TmpFileName[c++]=(unsigned char)changed_prefs.jport1;
	TmpFileName[c++]=(unsigned char)(changed_prefs.floppy_speed/100);
	TmpFileName[c++]=(unsigned char)(changed_prefs.immediate_blits);
	TmpFileName[c++]=(unsigned char)(changed_prefs.blitter_cycle_exact);
	TmpFileName[c++]=(unsigned char)(changed_prefs.dfxtype[0]);
	TmpFileName[c++]=(unsigned char)(changed_prefs.dfxtype[1]);
// thinkp
	TmpFileName[c++]=(unsigned char)g_mouse_speed;
	TmpFileName[c++]=(unsigned char)g_hr_mouse_speed;
	TmpFileName[c++]=(unsigned char)g_up;
	TmpFileName[c++]=(unsigned char)g_down;
	TmpFileName[c++]=(unsigned char)g_left;
	TmpFileName[c++]=(unsigned char)g_right;
	TmpFileName[c++]=(unsigned char)g_lup;
	TmpFileName[c++]=(unsigned char)g_ldown;
	TmpFileName[c++]=(unsigned char)g_lleft;
	TmpFileName[c++]=(unsigned char)g_lright;
	TmpFileName[c++]=(unsigned char)g_rup;
	TmpFileName[c++]=(unsigned char)g_rdown;
	TmpFileName[c++]=(unsigned char)g_rleft;
	TmpFileName[c++]=(unsigned char)g_rright;

	// 0.62 - version 1.0
	TmpFileName[c++]=(unsigned char)g_lsquare;
	TmpFileName[c++]=(unsigned char)g_ltriangle;
	TmpFileName[c++]=(unsigned char)g_lcross;
	TmpFileName[c++]=(unsigned char)g_lcircle;
	TmpFileName[c++]=(unsigned char)g_rsquare;
	TmpFileName[c++]=(unsigned char)g_rtriangle;
	TmpFileName[c++]=(unsigned char)g_rcross;
	TmpFileName[c++]=(unsigned char)g_rcircle;
// thinkp

// FOL
	// 0.62 BETA 4 - version 1.1
	TmpFileName[c++]=(unsigned char)changed_prefs.sound_interpol;
	TmpFileName[c++]=(unsigned char)(changed_prefs.gfx_correct_aspect);
// FOL

	// 0.62 BETA 8 - version 1.2
	TmpFileName[c++]=(unsigned char)(changed_prefs.cpu_idle);
	TmpFileName[c++]=(unsigned char)(g_disablekeycombos);
	TmpFileName[c++]=(unsigned char)(g_togglexo);
	TmpFileName[c++]=(unsigned char)(g_cpu2chip_ratio);
	TmpFileName[c++]=(unsigned char)(g_automousespeed);


	// 0.62 BETA 9 - version 1.3
	TmpFileName[c++]=(unsigned char)(g_aspect_ratio);
	TmpFileName[c++]=(unsigned char)(g_ScreenX<0?0:1);
	TmpFileName[c++]=(unsigned char)(abs(g_ScreenX)/255);
	TmpFileName[c++]=(unsigned char)(abs(g_ScreenX)%255);
	TmpFileName[c++]=(unsigned char)(g_ScreenY<0?0:1);
	TmpFileName[c++]=(unsigned char)(abs(g_ScreenY)/255);
	TmpFileName[c++]=(unsigned char)(abs(g_ScreenY)%255);
	TmpFileName[c++]=(unsigned char)(g_amiga_linesize/255);
	TmpFileName[c++]=(unsigned char)(g_amiga_linesize%255);
	TmpFileName[c++]=(unsigned char)(g_screenlock);

	// 0.62 BETA 10 - version 1.4
	int i=0,j=0;
	for(i=0; i<24;i++)
	{
		TmpFileName[c++]=(unsigned char)(mousestates[i].mbutton);
		TmpFileName[c++]=(unsigned char)(mousestates[i].x>>8);
		TmpFileName[c++]=(unsigned char)(mousestates[i].x&0xFF);
		TmpFileName[c++]=(unsigned char)(mousestates[i].y>>8);
		TmpFileName[c++]=(unsigned char)(mousestates[i].y&0xFF);
		for(j=0; j<21;j++)
			TmpFileName[c++]=(unsigned char)(mousestates[i].name[j]);
	}

	// 0.62 BETA 12 - version 1.5
	TmpFileName[c++]=(unsigned char)(g_autozoom);
	TmpFileName[c++]=(unsigned char)(g_bgnd_image);

	// 0.70B BETA 2 - version 1.6
//	TmpFileName[c++]=(unsigned char)(g_audio_opt);
	TmpFileName[c++]=(unsigned char)(changed_prefs.sound_stereo_separation);

#ifdef CYCLE_SWITCHUI
	// 0.71B BETA 7 - version 1.7
	TmpFileName[c++]=(unsigned char)(g_cpuspeed_up);

	// 0.71B BETA 8 - version 1.8
	TmpFileName[c++]=(unsigned char)(g_cpulock);
#endif

#ifdef SOUND_DELAY
	// 0.72 BETA 2 - version 1.9
	TmpFileName[c++]=(unsigned char)(g_sdswitch);
#endif

	sceIoWrite(fdout,TmpFileName,c);
	sceIoClose(fdout);
	return(0);
}

void backspace (char *name)
{
	int last;
	last = strlen (name) - 1;
	if(last>=0)
		name[last]='\0';
}

void remove_path (char *path)
{

	int last;
	char filename[255];
	int j=0;
	int i=0;

	last = strlen (path) - 1;
	while (last > 0 )
	{
		if (path[last] == '/' || path[last] == '\\')
		break;
		last--;
	}

	if ( last > 0 )
	{
		last++;
		for (i=last; i <= strlen(path); i++)
			filename[j++] = path[i];

		filename[j] = '\0';
		strcpy(path, filename);
		path[i] = '\0';
	}

	return;
}

int remove_extension (char *path)
{

	int last;
	char filename[255];
	int j=0;
	int i=0;

	last = strlen (path) - 1;
	while (last > 0 )
	{
		if (path[last] == '.')
		break;
		last--;
	}

	if ( last > 0 )
	{
		last--;
		for (i=0; i <= last; i++)
			filename[j++] = path[i];

		filename[j] = '\0';
		strcpy(path, filename);
		path[i] = '\0';

		return 1;
	}

	return 0;
}

int	MenuGo=0;

int	Select_Exit=0;
static int currentMenuLevel = 0;
static int lastdx[5] = {0,0,0,0,0};

const char *buttonname[]={
	"Start",
	"Square",
	"Triangle",
	"Cross",
	"Circle",
	"Left Shoulder Button",
	"Right Shoulder Button",
	"L+Square",
	"L+Triangle",
	"L+Cross",
	"L+Circle",
	"R+Square",
	"R+Triangle",
	"R+Cross",
	"R+Circle",
	"Up",
	"Down",
	"Left",
	"Right",
	"L+Up",
	"L+Down",
	"L+Left",
	"L+Right",
	"R+Up",
	"R+Down",
	"R+Left",
	"R+Right",
	NULL
};


int doMenu(const char **menu, int defsel, char *extrainfo, int x, int menuLevel, int *key )
{
// thinkp

	int		i;
	int		sel = defsel;
	int		waitForKey = PSP_CTRL_CROSS | PSP_CTRL_CIRCLE;
	int		ofs = 0, cntr, cntr_limit, go1;
	int		key2=-1;
	char	message[255];
  	int		dx = 0, dy = 0;

	for(i=0;menu[i];i++)
	{

		if (strlen((char*)menu[i]) == 0) continue;

		if (i<NBFILESPERPAGE) dy+= MENU_HEIGHT;
		if (strlen((char*)menu[i]) > dx) dx = strlen((char*)menu[i]);
	}
	dx*=5;
	dx+=10;
	dy+=8;

	if (browse_folders)
		dy = NBFILESPERPAGE*MENU_HEIGHT+8;

	int y = (272 - dy)/2;  //center the menu vertically
	if (dx < lastdx[menuLevel]) dx = lastdx[menuLevel];

	if (browse_folders)
		dx = 479-x;
	else
	if (x+dx > 480) // make sure there is room for the menu
	{
		x = 479 - dx;
		if (x < 0)
		{
			x = 0;
			dx = 480;
		}
	}

	char	msg[255];

	sprintf(msg, "PSPUAE " VERSION " (www.pspuae.com)      FreeMem: %.2f MB", __freemem() );

	lastdx[menuLevel]=0;
	sceDisplayWaitVblankStart();
	if (menuLevel == 0)
	{
		blitAlphaImageToScreen(0,0 ,480,272, menuImage, 0, 0);
		text_print( 0, 0, msg, rgb2col(155,255,155),rgb2col(0,0,0),0);
		flipScreen();
		blitAlphaImageToScreen(0,0 ,480,272, menuImage, 0, 0);
		text_print( 0, 0, msg, rgb2col(155,255,155),rgb2col(0,0,0),0);
		flipScreen();
	}

	if (menuLevel > currentMenuLevel)
		fadeInMenu(18,x,y,dx,dy, menuImage);
	else
		activateMenu(18,x,y,dx,dy, menuImage);

	currentMenuLevel = menuLevel;

	cntr_limit=25;
	prev_ctl_buttons=0;
	while(!quit_program)
	{
		if ( (key!=NULL) && (key2 >= 0) )
		{
			if ( key2 > 94 )
			{
			// unassign key
				*key = -1;
				*extrainfo=-2;
			}
			else
			{
			// assign key
				*key = key2;
				*extrainfo=0;
			}
			break;
		}

		if(sel >= (ofs+NBFILESPERPAGE-1)) ofs=sel-NBFILESPERPAGE+1;
		if(sel < ofs) ofs=sel;
		sceDisplayWaitVblankStart();
		blitAlphaImageToScreen(x+1,y+1 ,dx-2,dy-2, menuImage, x+1, y+1);
		drawMenuBox(18,x+1,y+1,dx-2,dy-2);
		char  entry[255];
		for(i=0;menu[i+ofs] && i<NBFILESPERPAGE;i++)
		{
			strcpy(entry,menu[i+ofs]);

			if (browse_folders && nfiles > 0 )
			{
				if (strlen(entry) > 60 )
					entry[61]=0;

				int ofs2 = (browse_folders == 2) ? 0 : 1;

				if (files[i+ofs-ofs2].d_stat.st_attr == TYPE_DIR && strcmp(files[i+ofs-ofs2].d_name,".."))
				{
				// directory
					if (strchr(entry,'/') != NULL )
					// remove slash
						entry[strlen(entry)-1]=0;
					text_print( x+5, i*MENU_HEIGHT+y+4, entry, (((i+ofs)==sel)?rgb2col(255,0,0):rgb2col(0,0,192)),rgb2col(0,0,0),0);
				}
				else
				// file
					text_print( x+5, i*MENU_HEIGHT+y+4, entry, (((i+ofs)==sel)?rgb2col(255,0,0):rgb2col(192,192,192)),rgb2col(0,0,0),0);
			}
// thinkp mousestates
			else
			if(domousestatemenu)
			{
			// mousestate entry
				if(mousestates[i+ofs].x != 0 && mousestates[i+ofs].y != 0 )
				// mousestate slot NOT empty
					text_print( x+5, i*MENU_HEIGHT+y+4, entry, (((i+ofs)==sel)?rgb2col(128,128,255):rgb2col(0,0,255)),rgb2col(0,0,0),0);
				else
				// mousestate slot empty
					text_print( x+5, i*MENU_HEIGHT+y+4, entry, (((i+ofs)==sel)?rgb2col(255,255,255):rgb2col(192,192,192)),rgb2col(0,0,0),0);

			}
// thinkp mousestates
			else
			// menu entry
				text_print( x+5, i*MENU_HEIGHT+y+4, entry, (((i+ofs)==sel)?rgb2col(255,255,255):rgb2col(192,192,192)),rgb2col(0,0,0),0);
		}
		flipScreen();

		if(waitForKey)
		{
			cntr=0;go1=1;
			while(go1)
			{
				++cntr;
#ifdef NO_VSYNC
				sceCtrlPeekBufferPositive(&ctl,1);
#else
				sceCtrlReadBufferPositive(&ctl,1);
#endif
				if(!(ctl.Buttons & waitForKey)) go1=0;
				if(cntr>cntr_limit)
				{
					go1=0;
					cntr_limit=5;
				}
				sceDisplayWaitVblankStart();

			}
			waitForKey = 0;
		}

		while(!quit_program)
		{
#ifdef NO_VSYNC
				sceCtrlPeekBufferPositive(&ctl,1);
#else
				sceCtrlReadBufferPositive(&ctl,1);
#endif

			if(CheckActive(ctl.Buttons,BT_SCREEN_SHOT)) SaveScreen();

			if((sel>1) && (key!=NULL) && (ctl.Buttons & PSP_CTRL_SQUARE)&&(!(prev_ctl_buttons & PSP_CTRL_SQUARE)))
			{
				if(KeyboardActiveMenu)
				{
					KeyboardActiveMenu=0;
					KeyboardPositionOffset = 142;
					restoreScreen();
					prev_ctl_buttons=ctl.Buttons;
					break;
				}
				else
				{
					saveScreen();
					if ( *key >= 0 && *key <= 93)
					{
						if ( *key ==  93 )	// AK_RET
							*key = 100; //SPECIAL_CODE_FOR_ENTER
						SetCurSel(*key);
					}
					*key = -1;
					KeyboardActiveMenu=1;
					KeyboardPositionOffset = 71;

					if(sel>1)
					{
						sprintf(message, "Press key for %s", (char*)buttonname[sel-2]);
					}
					clearScreen();
				}
			}

// thinkp mousestates
			if(domousestatemenu && (ctl.Buttons & PSP_CTRL_SQUARE)&&(!(prev_ctl_buttons & PSP_CTRL_SQUARE)))
			{
			// edit mousestate name

				if(KeyboardActiveEdit)
				{
					KeyboardActiveEdit=0;
					KeyboardPositionOffset = 142;
					restoreScreen();
					prev_ctl_buttons=ctl.Buttons;
					break;
				}
				else
				{
					saveScreen();
					if(key2==-1)
						SetCurSel(25);	// Backspace

					KeyboardActiveEdit=1;
					KeyboardPositionOffset = 71;

					clearScreen();
				}
			}
// thinkp mousestates

			prev_ctl_buttons=ctl.Buttons;

			if(KeyboardActiveMenu)
			{
				blitAlphaImageToScreen(0,0 ,480,272, menuImage, 0, 0);
				text_print( 0, 0, "PSPUAE " VERSION " (www.pspuae.com)", rgb2col(155,255,155),rgb2col(0,0,0),0);
				text_print( 0, 15, message, rgb2col(155,255,155),rgb2col(0,0,0),0);
				DrawKeyboardD();
				flipScreen();
				blitAlphaImageToScreen(0,0 ,480,272, menuImage, 0, 0);
				text_print( 0, 0, "PSPUAE " VERSION " (www.pspuae.com)", rgb2col(155,255,155),rgb2col(0,0,0),0);
				text_print( 0, 15, message, rgb2col(155,255,155),rgb2col(0,0,0),0);
				DrawKeyboardD();
				flipScreen();

				if ( (key2 = CallKeyboardInputMenu(ctl)) >= 0 )
				{
					KeyboardActiveMenu=0;
					KeyboardPositionOffset = 142;
					restoreScreen();
					prev_ctl_buttons=ctl.Buttons;
					break;
				}

				continue;
			}

// thinkp mousestates
			if(KeyboardActiveEdit)
			{
				sprintf(message, "Edit mousestate name: %s", (char*)menu[sel]);

				blitAlphaImageToScreen(0,0 ,480,272, menuImage, 0, 0);
				text_print( 0, 0, "PSPUAE " VERSION " (www.pspuae.com)", rgb2col(155,255,155),rgb2col(0,0,0),0);
				text_print( 0, 15, message, rgb2col(155,255,155),rgb2col(0,0,0),0);
				DrawKeyboardD();
				flipScreen();
				blitAlphaImageToScreen(0,0 ,480,272, menuImage, 0, 0);
				text_print( 0, 0, "PSPUAE " VERSION " (www.pspuae.com)", rgb2col(155,255,155),rgb2col(0,0,0),0);
				text_print( 0, 15, message, rgb2col(155,255,155),rgb2col(0,0,0),0);
				DrawKeyboardD();
				flipScreen();

				if ( (key2 = CallKeyboardInputMenu(ctl)) >= 0 )
				{
					if(key2==25 || key2==100)
					// backspace
						backspace((char*)menu[sel]);

					if(	(key2>=11 && key2<=24)	||
						(key2>=28 && key2<=31)	||
						(key2>=33 && key2<=44)	||
						(key2>=51 && key2<=61)	||
						(key2>=68 && key2<=77)	)
					{
						if(strlen(menu[sel]) <=20 )
							strcat (menu[sel],AmigaKeyboard[key2].Name);
					}
				}

				continue;
			}
// thinkp mousestates

			if((g_togglexo == 0 && ctl.Buttons & PSP_CTRL_CIRCLE) || (g_togglexo == 1 && ctl.Buttons & PSP_CTRL_CROSS))
			{
				if(extrainfo!=NULL) *extrainfo=0;
				if(key!=NULL) *key=-1;
				Select_Exit=0;
				removeMenu(x,y,dx,dy,menuImage);
				return -1;
			}

			if(ctl.Buttons & PSP_CTRL_SELECT)
			{
				if(extrainfo!=NULL) *extrainfo=0;
				if(key!=NULL) *key=-1;
				Select_Exit=1;
				MenuGo=0;
				removeMenu(x,y,dx,dy,menuImage);
				return -1;
			}

			if((g_togglexo == 0 && ctl.Buttons & PSP_CTRL_CROSS) || (g_togglexo == 1 && ctl.Buttons & PSP_CTRL_CIRCLE))
			{
				if(extrainfo!=NULL) *extrainfo=1;
				if(key!=NULL) *key=-1;
				Select_Exit=0;
				sceDisplayWaitVblankStart();
				inactivateMenu(rgb2col(80,80,80), x,y,dx,dy, menuImage);
				char  entry[255];
				for(i=0;menu[i+ofs] && i<NBFILESPERPAGE;i++)
				{
					strcpy(entry,menu[i+ofs]);
					if (browse_folders && nfiles > 0 && strlen(entry) > 60 )
						entry[61]=0;
					text_print( x+5, i*MENU_HEIGHT+y+4, entry, (((i+ofs)==sel)?rgb2col(160,160,160):rgb2col(96,96,96)),rgb2col(0,0,0),0);
				}
				flipScreen();
				inactivateMenu(rgb2col(80,80,80), x,y,dx,dy, menuImage);
				for(i=0;menu[i+ofs] && i<NBFILESPERPAGE;i++)
				{
					strcpy(entry,menu[i+ofs]);
					if (browse_folders && nfiles > 0 && strlen(entry) > 60 )
						entry[61]=0;
					text_print( x+5, i*MENU_HEIGHT+y+4, entry, (((i+ofs)==sel)?rgb2col(160,160,160):rgb2col(96,96,96)),rgb2col(0,0,0),0);
				}
				flipScreen();
				lastdx[menuLevel] = dx;

				return sel;
			}
/*
			if((ctl.Buttons & PSP_CTRL_SQUARE)&&(ctl.Buttons & PSP_CTRL_TRIANGLE))
			{
				sceDisplayWaitVblankStart();
				text_print( 0, 0, "PSPUAE " VERSION " by MIB.42", rgb2col(155,255,155),rgb2col(0,0,0),0);
				flipScreen();
			}
*/
			if(extrainfo!=NULL)
			{
				if(ctl.Buttons & PSP_CTRL_LEFT)
				{
					go1=1;
					while(go1)
					{
#ifdef NO_VSYNC
				sceCtrlPeekBufferPositive(&ctl,1);
#else
				sceCtrlReadBufferPositive(&ctl,1);
#endif
						if(!(ctl.Buttons & PSP_CTRL_LEFT)) go1=0;
					}
					*extrainfo=-1;
					Select_Exit=0;
		//			sceDisplayWaitVblankStart();
					//blitAlphaImageToScreen(x+1,y+1 ,dx-2,dy-2, menuImage, x+1, y+1);
		//			for(i=0;menu[i+ofs] && i<NBFILESPERPAGE;i++)
		//			{
		//				text_print( x+5, i*MENU_HEIGHT+y+4, (char*)(menu[i+ofs]), (((i+ofs)==sel)?rgb2col(255,255,255):rgb2col(192,192,192)),rgb2col(0,0,0),0);
		//			}
		//			flipScreen();
					lastdx[menuLevel] = dx;
					if(key!=NULL) *key=-1;
					return sel;
				}
				if(ctl.Buttons & PSP_CTRL_RIGHT)
				{
					go1=1;
					while(go1)
					{
#ifdef NO_VSYNC
				sceCtrlPeekBufferPositive(&ctl,1);
#else
				sceCtrlReadBufferPositive(&ctl,1);
#endif
						if(!(ctl.Buttons & PSP_CTRL_RIGHT)) go1=0;
					}
					*extrainfo=1;
					Select_Exit=0;
		//			sceDisplayWaitVblankStart();
		//			for(i=0;menu[i+ofs] && i<NBFILESPERPAGE;i++)
		//			{
		//				text_print( x+5, i*MENU_HEIGHT+y+4, (char*)(menu[i+ofs]), (((i+ofs)==sel)?rgb2col(255,255,255):rgb2col(192,192,192)),rgb2col(0,0,0),0);
		//			}
		//			flipScreen();
					lastdx[menuLevel] = dx;
					if(key!=NULL) *key=-1;
					return sel;
				}
			}
			if(ctl.Buttons & PSP_CTRL_DOWN || ctl.Buttons & PSP_CTRL_RTRIGGER)
			{
				if(!menu[sel+1]) continue;
				if(strlen((char*)menu[sel+1]) == 0) continue;
				sel++;
				waitForKey = PSP_CTRL_DOWN;
				if(key!=NULL) *key=GetCurrentKey(sel);
				break;
			}
			else
			{
				if(ctl.Buttons & PSP_CTRL_UP || ctl.Buttons & PSP_CTRL_LTRIGGER)
				{
					if(sel>0) sel --;
					waitForKey = PSP_CTRL_UP;
					if(key!=NULL) *key=GetCurrentKey(sel);
					break;
				}
				else cntr_limit=25;
			}


		}
	}
//	sceDisplayWaitVblankStart();
//	for(i=0;menu[i+ofs] && i<NBFILESPERPAGE;i++)
//	{
//		text_print( x+5, i*MENU_HEIGHT+y+4, (char*)(menu[i+ofs]), (((i+ofs)==sel)?rgb2col(255,255,255):rgb2col(192,192,192)),rgb2col(0,0,0),0);
//	}
//	flipScreen();
	return sel;

// thinkp
}

//Main Menu FOL
static const char *mainmenu[]={
  "States",
  "Drives",
  "Amiga",
  "PSP",
  "Configuration",
  "Soft Reset",
#ifdef HARD_RESETCOM
  "Hard Reset",
#endif
  "Exit",
  NULL
};

//States Menu FOL
static const char *statesmenu[]={
  "Load State",
  "Save State",
  NULL
};

// Floppy Main Menu
char floppyitems[5][256];
char floppynr[256];
const char *floppymenu[]={
  floppyitems[0],
  floppyitems[1],
  floppyitems[2],
  floppyitems[3],
#ifdef HARDDRIVE
  floppyitems[4],
#endif
  //  floppynr,
  NULL
};

// Config Main Menu FOL
static const char *configmenu[]={
    "QuickStart Config",
    "Load Config",
    "Save Config",
NULL
};

// Preset Configs Menu FOL
static const char *presetoptionsmenu[]={
	"Load Config A500  (OCS)",
	"Load Config A600  (ECS Denise)",
	"Load Config A1000 (OCS)",
	"Load Config A1200 (ECS)",
	"Load Default Config",
    NULL
};

//Hardware Menu FOL
static const char *hardwareoptionsmenu[]={
    "CPU",
    "CHIPSET",
    "RAM",
    "KICKSTART ROM xxx",
    "Drives Config",
NULL
};

// CPU Main Menu FOL
static const char *cpuoptionsmenu[]={
#ifndef CPUEMU_68000_ONLY
	"CPU Type            : xxxxxxxxxxx",
#endif
#ifdef CYCLE_SWITCHUI
	"CPU Cycles          : xxxxxx",
	"CPU Cycles Option   : xxxxxxxxx",
#endif
	"CPU Speed           : xxxxxxx",
	"",
	NULL
};

static const char *cpumenu[] = {
    "68000",
#ifndef CPUEMU_68000_ONLY
	"68010",
	"68020",
//	"68020/68881",
//	"68040",
#endif
	0
};

static const char *cpuspeedmenu[] = {
    "max",
    "real",
    "chipset",
    0
};

// FOL
#ifdef CYCLE_SWITCH
static const char *cpu2chipmenu[] = {
	"  5%",
    " 10%",
	" 15%",
	" 20%",
	" 25%",
    " 30%",
	" 35%",
    " 40%",
	" 45%",
	" 50%",
	" 55%",
    " 60%",
	" 65%",
	" 70%",
	" 75%",
    " 80%",
	" 85%",
	" 90%",
	" 95%",
    "100%",
	0
};
#else
static const char *cpu2chipmenu[] = {
	"7%",
    "13%",
	"19%",
	"26%",
	"31%",
    "37%",
	"43%",
    "49%",
	"55%",
	"61%",
	"67%",
    "73%",
	"79%",
	"85%",
	"93%",
    "100%",
	0
};
#endif
// FOL

// Chipset main menu FOL
static const char *chipsetoptionsmenu[] = {
    "Blitter Cycle Exact: xxx",
    "Immediate Blits    : xxx",
    "Collision Mode     : xxxxxxxxxx",
    "Chipset            : xxxxxxxxxx",
    "Sound Emulation",
    0
};

static const char *collmenu[] = {
       "none",
       "sprites",
       "playfields",
       "full",
       0
       };

static const char *csmode[] = {
       "ocs",
       "ecs_agnus",
       "ecs_denise",
       "ecs",
#ifdef AGA
	   "aga",
#endif
	   0 };

// Sound Menu FOL
static const char *audiooptionsmenu[]={
	"Sound Emulation: xxxxxxxxxx",
	"Snd Seperation : xxx",
//	"Sound Optimise : xxx",
#ifdef SOUND_DELAY
	"Sound Delay    : xxx",
#endif
#ifdef INTERPOLOLD	//FOL made this a define, as the code was removed to gain speed
	"Interpol       : xxxx",
#endif
	NULL
};

static const char *soundmenu[] = {
       "none",
#ifdef INTERRUPT_ON
	   "interrupts",
#endif
	   "normal",
       "exact",
       0
       };

static const char *sepmenu[] = {
       "0",
       "1",
       "2",
       "3",
       "4",
       "5",
       "6",
       "7",
       "8",
       "9",
       "10",
	   0
       };

// RAM Main Menu
static const char *ramoptionsmenu[] = {
	"Fastmem Size: xxxxxxx",
	"Chipmem Size: xxxxxxx",
	"Bogomem Size: xxxxxxx",
    NULL
};
// RAM Menu
//FOL
static const char *chipmemmenu[] = {
       "256 KB",
       "512 KB",
       "1024 KB",
       "2048 KB",
       0
       };

static const char *bogomemmenu[] = {
       "none",
       "512 KB",
       "1024 KB",
       "1536 KB",
       //"1792 KB",
       0
       };

static const char *fastmemmenu[] = {
       "none",
       "1024 KB",
       "2048 KB",
       "4096 KB",
       "8192 KB",
       0
       };
//FOL
//cmf
static const uae_u32 *chipmemsizes[] = {
       256,
       512,
       1024,
       2048,
       0
       };

static const uae_u32 *bogomemsizes[] = {
       0,
       512,
       1024,
       1536,
       //1792,
       0
       };

static const uae_u32 *fastmemsizes[] = {
       0,
       1024,
       2048,
       4096,
       8192,
       0
       };
//cmf

//Kickstart Menu
static const char *ksmenu[] = {
       "1.0",
       "1.1",
       "1.2",
       "1.3",
       "2.0",
       "2.04",
       "2.05",
       "3.0",
       "3.1",
       0
       };

//Drives Config Menu FOL
static const char *ddmenu[] = {
	   "Floppy Speed : xxxxxx ",
#ifdef FOUR_DRIVES
       "DF0 disk type: 3.5DD",
       "DF1 disk type: 3.5DD",
       "DF2 disk type: 3.5DD",
       "DF3 disk type: 3.5DD",
#else
	   "DF0 disk type: 3.5DD",
       "DF1 disk type: 3.5DD",
#endif
	   0
       };

//Host Main Menu FOL
static const char *hostoptionsmenu[]={
    "Display",
    "Input Devices",
    "Misc",
    NULL
    };

static const char *videooptionsmenu[]={
	"Direct VRAM Access: xxx",
    "Frameskip         : xxxx",
	"Zoom              : xxxxxxxxx",
	"Aspect Ratio      : xxxxxxx",
	"Auto Zoom         : xxx",
	"",
	NULL
    };

#ifdef ADVANCED_FS
static const char *frskipmenu[]={ "auto","0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20","21","22","23","24","25","26","27","28","29","30","31","32","33","34","35","36","37","38","39","40","41","42","43","44","45","46","47","48","49",0 };
#else
static const char *frskipmenu[]={
      "0",
      "1",
      "2",
      "3",
      "4",
      "5",
      "6",
      "7",
      "8",
      "auto",
      0
      };
#endif

static const char *zoommenu[] = {
       "No Zoom", //FOL
       "10%",
       "11%",
       "12%",
       "14%",
       "17%",
       "20%",
       "25%",
	   "34%",
       "Full Zoom",  //FOL
       0
       };

static const char *aspectratiomenu[] = {
       "4:3",
       "16:9",
       0
       };

//misc Menu FOL
static const char *miscmenu[]={
    "Mouse Speed         : xxxxxxxxxxxx",
    "Hi-Res Mouse Speed  : xxxxxxxxxxxx",
	"Auto Mouse Speed    : xxxxxxxxxxxx",
	"Show Leds           : xxx",
    "Transparent Keyboard: xxx",
	"Disable Key Combos  : xxx",
	"Toggle X+O          : xxx",
	"Background Image    : xxx",
    NULL
    };

// Input Devices Menu
static const char *controloptionsmenu[]={
	"Analog Stick         : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	"Directional Buttons  : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	"Start                : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	"Square               : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	"Triangle             : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	"Cross                : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	"Circle               : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	"Left Shoulder Button : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	"Right Shoulder Button: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	"L+Square             : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	"L+Triangle           : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	"L+Cross              : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	"L+Circle             : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	"R+Square             : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	"R+Triangle           : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	"R+Cross              : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	"R+Circle             : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	NULL
};

static const char *inputselection[]={	// g_analogstick
"Joystick 0     ",
"Joystick 1     ",
"Mouse          ",
0
};
// thinkp

static const char *inputselection2[]={	// dirbuttons
"Joystick 0     ",
"Joystick 1     ",
"Mouse          ",
"Keys	        ",
0
};
// thinkp

static const char *mousespeedmenu[] = {
       "5 (fast)",
       "4",
       "3",
       "2",
       "1 (slow)",
       0
};

// thinkp mousestates
static const char *mousestatemenu[] = {
	"MouseState 1        ",
	"MouseState 2        ",
	"MouseState 3        ",
	"MouseState 4        ",
	"MouseState 5        ",
	"MouseState 6        ",
	"MouseState 7        ",
	"MouseState 8        ",
	"MouseState 9        ",
	"MouseState 10       ",
	"MouseState 11       ",
	"MouseState 12       ",
	"MouseState 13       ",
	"MouseState 14       ",
	"MouseState 15       ",
	"MouseState 16       ",
	"MouseState 17       ",
	"MouseState 18       ",
	"MouseState 19       ",
	"MouseState 20       ",
	"MouseState 21       ",
	"MouseState 22       ",
	"MouseState 23       ",
	"MouseState 24       ",
    0
};
// thinkp mousestates

SceIoDirent foundfile;
char g_path_hardfile[256] = "";

// thinkp borrowed from snes9xTYL

char *strcasechr(const char *s, int c){
	int		size;
	int		i;

	size = strlen( s );
	for ( i = 0; i < size; i++ ){
		if ( tolower(s[i]) == tolower(c) ){
			return (char*)&s[i];
		}
	}
	return 0;
}


char *strcasestr (const char *s1, const char*s2){
  register char *p = s1;
  register int len = strlen (s2);

  for (; (p = strcasechr (p, *s2)) != 0; p++)
    {
      if (strncasecmp (p, s2, len) == 0)
	{
	  return (p);
	}
    }
  return (0);
}

void SJISCopy(struct SceIoDirent *a, unsigned char *file)
{
	unsigned char ca;
	int i;

	for(i=0;i<=strlen(a->d_name);i++){
		ca = a->d_name[i];
		if (((0x81 <= ca)&&(ca <= 0x9f))
		|| ((0xe0 <= ca)&&(ca <= 0xef))){
			file[i++] = ca;
			file[i] = a->d_name[i];
		}
		else{
			if(ca>='a' && ca<='z') ca-=0x20;
			file[i] = ca;
		}
	}
}

int cmpFile(SceIoDirent *a, SceIoDirent *b) {
	char file1[0x108];
  char file2[0x108];
	char ca, cb;
	int i, n, ret;
	if(a->d_stat.st_attr==b->d_stat.st_attr) {
		SJISCopy(a, (unsigned char *)file1);
		SJISCopy(b, (unsigned char *)file2);
		n=strlen(file1);
		for(i=0; i<=n; i++){
			ca=file1[i]; cb=file2[i];
			ret = ca-cb;
			if(ret!=0) return ret;
		}
		return 0;
	}

	if(a->d_stat.st_attr & FIO_SO_IFDIR)	return -1;
	else					return 1;
}

void sort(SceIoDirent *a, int left, int right) {
	SceIoDirent tmp, pivot;
	int i, p;

	if (left < right) {
		pivot = a[left];
		p = left;
		for (i=left+1; i<=right; i++) {
			if (cmpFile(&a[i],&pivot)<0){
				p=p+1;
				tmp=a[p];
				a[p]=a[i];
				a[i]=tmp;
			}
		}
		a[left] = a[p];
		a[p] = pivot;
		sort(a, left, p-1);
		sort(a, p+1, right);
	}
}

typedef struct {
	char *szExt;
	int nExtId;
} structExtentions;

const structExtentions stExtentions[3][4] = {
	{ {"adf",EXT_ADF}, {"adz",EXT_ADZ}, {"zip",EXT_ZIP}, {NULL, EXT_UNKNOWN} },
	{ {"hdf",EXT_HDF}, {NULL, EXT_UNKNOWN}, {NULL, EXT_UNKNOWN}, {NULL, EXT_UNKNOWN} },
	{ {"asf",EXT_ASF}, {NULL, EXT_UNKNOWN}, {NULL, EXT_UNKNOWN}, {NULL, EXT_UNKNOWN} }
};

enum {EXT_DISK=0,EXT_HD, EXT_STATE};

int getExtId( char *szFilePath, int exttype) {
	char *pszExt;
	int i;
	if((pszExt = strrchr(szFilePath, '.'))) {
		pszExt++;
		for (i = 0; stExtentions[exttype][i].nExtId != EXT_UNKNOWN; i++) {
			if (!strcasecmp(stExtentions[exttype][i].szExt,pszExt)) {
				return stExtentions[exttype][i].nExtId;
			}
		}
	}
	return EXT_UNKNOWN;
}


char *find_file(char *pattern,char *path){
	int fd,found;
	fd = sceIoDopen(path);
	if (fd<0) {
//		psp_msg(ERR_READ_MEMSTICK,MSG_DEFAULT);
		return NULL;
	}
	found=0;
	while(1){
		if(sceIoDread(fd, &foundfile)<=0) break;
		if (strcasestr(foundfile.d_name,pattern)) {found=1;break;}
	}
	sceIoDclose(fd);
	if (found) return foundfile.d_name;
	return NULL;
}

void getDir(const char *path, int exttype) {
	int fd, b=0;
//	char *p;

	nfiles = 0;

	if(strcmp(path,"ms0:/") && strcmp(path,"host0:/")){
		strcpy(files[nfiles].d_name,"..");
		files[nfiles].d_stat.st_attr = TYPE_DIR;
		nfiles++;
		b=1;
	}

	fd = sceIoDopen(path);
	if (fd<0){
//		psp_msg(ERR_READ_MEMSTICK,MSG_DEFAULT);
		return ;
	}

	while(nfiles<MAX_ENTRY){
		if(sceIoDread(fd, &files[nfiles])<=0) break;

		if(files[nfiles].d_name[0] == '.') continue;

		if(files[nfiles].d_stat.st_attr == TYPE_DIR){
			strcat(files[nfiles].d_name, "/");
			nfiles++;
			continue;
		}
		if(getExtId(files[nfiles].d_name, exttype) != EXT_UNKNOWN) nfiles++;
	}

	sceIoDclose(fd);

	if (nfiles) {
		if(b)
			sort(files+1, 0, nfiles-2);
		else
			sort(files, 0, nfiles-1);
	}
}

void statesMenu()
{
	static int lastsel;
	while(MenuGo)
	{
		if (quit_program)
			break;
		int option = 0;
		sprintf((char*)(statesmenu[option++]), "Load State");
		sprintf((char*)(statesmenu[option++]), "Save State");

		int res = doMenu(statesmenu, lastsel, NULL,100,1,NULL);
		if(res == -1) break;
		lastsel = res;
		if(res == 0)
		{
			strcpy(path, g_path);
			strcat(path, "/STATE/");

			//load state menu
			char *tmpnames[4096];
			char  entry[255];
			char  statename[255];
			char  optionsname[255];

			int exit=0;
			int i=0;

			while(!quit_program)
			{
				browse_folders=2;
				memset(tmpnames,0,sizeof(tmpnames));

				int nb=0;

				printpath(150);

				//scan directory
				getDir(path, EXT_STATE);

				for (i=0; i < nfiles; i++)
				{
					if (files[i].d_stat.st_attr == TYPE_DIR)
					{
						tmpnames[nb++]=strdup(files[i].d_name);
					}
					else
					{
						strcpy(statename,files[i].d_name);
						remove_extension(statename);
						sprintf(entry, "Load State %s", statename);
						tmpnames[nb++]=strdup(entry);
					}
				}

				//load state
				int res=doMenu((const char **)&tmpnames,0,NULL,150,2,NULL);
				if(res == -1)
					exit = 1;
				else
				{
					if(res == 0 && !strcmp(tmpnames[res],".."))
					{
					// directory up
						if(strcmp(path,"ms0:/") && strcmp(path,"host0:/"))
						{
							char *p;
							p=strrchr(path,'/');
							*p=0;

							if(strcmp(path,"ms0:/") && strcmp(path,"host0:/"))
							{
								p=strrchr(path,'/');
								p++;
								*p=0;
							}
						}
					}

					if (res > 0 && nfiles > 0 && files[res].d_stat.st_attr == TYPE_DIR)
					{
					// directory down
						strcat(path,files[res].d_name);
					}

					if (res > 0 && nfiles > 0 && files[res].d_stat.st_attr == TYPE_FILE)
					{
					// named state
							remove_extension(files[res].d_name);

							if ( strlen(files[res].d_name) == 1 && strcspn (files[res].d_name,"12345") == 0 )
							{
							// numbered state
								strcpy(optionsname, "pspuae");
								strcat(optionsname, files[res].d_name);
								strcpy(statename, files[res].d_name);
								strcpy(lastsavestate, "");
							}
							else
							{
							// named state
								strcpy(optionsname, files[res].d_name);
								strcpy(statename, files[res].d_name);
								strcpy(lastsavestate, files[res].d_name);
							}
							sprintf(savestate_fname,"%s%s.asf", path, statename);
							LoadPSPUAEOptions(-1,optionsname,"",0);
							savestate_state = STATE_DORESTORE;
	                        removeMenu2(statesmenu,150);
							exit = 1;
							MenuGo=0;
					}
				}	// end else if(res == -1)

				i=0;
				for (i=0; i < nb; i++)
					free(tmpnames[i]);

				if (exit)
				{
					break;
				}

			}	// while(1)

			lastdx[2] = 0;
			removepath(150);
			browse_folders=0;

		}
		else if (res == 1)
		{
			//save state menu
			char *tmpnames[4096];
			memset(tmpnames,0,sizeof(tmpnames));

			int	  nb=0;
			char  diskname[255];
			char  entry[255];
			int  addentry=0;

			if ( !(strlen(changed_prefs.df[0]) == 0 && strlen(changed_prefs.df[1]) == 0))
			{
			// disk inserted
				if(strlen(changed_prefs.df[1]) != 0)
					strcpy(diskname,changed_prefs.df[1]);
				if(strlen(changed_prefs.df[0]) != 0)
					strcpy(diskname,changed_prefs.df[0]);

				remove_extension(diskname);
				remove_path(diskname);
				addentry=1;
			}
			else
			if (strlen(lastsavestate) > 0 )
			{
			// previous savestate name
				strcpy(diskname, lastsavestate);
				addentry=1;
			}
			else
			if (strlen(lastdiskname) > 0 )
			{
			// previous inserted diskname
				strcpy(diskname, lastdiskname);
				remove_extension(diskname);
				remove_path(diskname);
				addentry=1;
			}

			if (addentry)
			{
				sprintf(entry, "Save State %s", diskname);
				tmpnames[nb++]=strdup(entry);
			}

			tmpnames[nb++]=strdup("Save State #1");
			tmpnames[nb++]=strdup("Save State #2");
			tmpnames[nb++]=strdup("Save State #3");
			tmpnames[nb++]=strdup("Save State #4");
			tmpnames[nb++]=strdup("Save State #5");

			//save state
			int res = doMenu((const char **)&tmpnames,0,NULL,150,2,NULL);
			if (res >= 0)
			{
				// check if the savestate dir exists
				create_dir("STATE");

				char dirName[1024];
				// save state
				if(res == 0 && addentry)
				{
				// save named state
					sprintf(dirName,"%sSTATE/%s.asf", LaunchPath, diskname);
					SavePSPUAEOptions(-1,diskname,"");
				}
				else
				{
				// save numbered state
					int offset=1;
					if(addentry)
						offset=0;
					sprintf(dirName,"%sSTATE/%d.asf", LaunchPath, res+offset);
					SavePSPUAEOptions(res+offset,"","");
				}
				save_state(dirName, "");
				int i=0;
				for (i=0; i < nb; i++)
					free(tmpnames[i]);
				break;
			}
			int i=0;
			for (i=0; i < nb; i++)
				free(tmpnames[i]);
		    }

		}	// end else if (res == 1)

	}	// end while(*MenuGo)

void diskMenu()
{
	strcpy(path, g_path);
	strcat(path, "/DISKS/");

	//insert disk
	while(!quit_program)
	{
		int i;
		for(i=0;i<currprefs.nr_floppies;i++)
		sprintf(floppyitems[i],"Insert in DF%i: (%s)",i,changed_prefs.df[i]);

#ifdef HARDDRIVE
		sprintf(floppyitems[i++],"Insert in DH0: (%s)",g_path_hardfile);
#endif
		floppymenu[i] == 0;
		int res = doMenu(floppymenu, 0, NULL,100,1,NULL);
		if(res == -1) break;

		//file menu_
		char *tmpnames[4096];
		char  diskname[255];

		if ( !(strlen(changed_prefs.df[0]) == 0 && strlen(changed_prefs.df[1]) == 0))
		{
			if(strlen(changed_prefs.df[1]) != 0)
				strcpy(diskname,changed_prefs.df[1]);
			if(strlen(changed_prefs.df[0]) != 0)
				strcpy(diskname,changed_prefs.df[0]);

			remove_path(diskname);
		}

		int exit=0, lastsel=0;

		while(!quit_program)
		{
			browse_folders=1;
			memset(tmpnames,0,sizeof(tmpnames));

			int nb=0;

			printpath(140);

			//scan directory
			if (res == currprefs.nr_floppies)
				getDir(path, EXT_HD);
			else
				getDir(path, EXT_DISK);



			if ( nfiles == 0 || strcmp(files[0].d_name,"..") )
			// root -> empty at first position
				tmpnames[nb++]=strdup("<empty>");

			for (i=0; i < nfiles; i++)
			{
				if(!strcmp(diskname, files[i].d_name) )
				// auto select the disk currently in the drive
					lastsel = nb;

				tmpnames[nb++]=strdup(files[i].d_name);
				if(!strcmp(files[i].d_name,".."))
				// empty behind dir up
					tmpnames[nb++]=strdup("<empty>");
			}

			int res2=doMenu((const char **)&tmpnames,lastsel,NULL,140,2,NULL);

			if(res2 == -1)
				exit = 1;
			else
			{
				if(!strcmp(tmpnames[res2],"<empty>"))
				{
					if (res == currprefs.nr_floppies)
					{
					// eject hardfile
						g_path_hardfile[0] = 0;
						changed_prefs.path_hardfile[0] = 0;
					}
					else
					{
						if (strlen(changed_prefs.df[res]) >0 )
							strcpy(lastdiskname, changed_prefs.df[res]);

						//eject floppy
						changed_prefs.df[res][0]=0;
					}

					exit = 1;
				}
				else
				{
					if(res2 == 0 && !strcmp(tmpnames[res2],".."))
					{
					// up  directory
						if(strcmp(path,"ms0:/") && strcmp(path,"host0:/"))
						{
							char *p;
							p=strrchr(path,'/');
							*p=0;

							if(strcmp(path,"ms0:/") && strcmp(path,"host0:/"))
							{
								p=strrchr(path,'/');
								p++;
								*p=0;
							}
						}
					}

					if (res2 > 0 && nfiles > 0 && files[res2-1].d_stat.st_attr == TYPE_DIR)
					{
					// down directory
						strcat(path,files[res2-1].d_name);
					}

					if (res2 > 0 && nfiles > 0 && files[res2-1].d_stat.st_attr == TYPE_FILE)
					{
					// file
						if (res == currprefs.nr_floppies)
						{
						// insert hardfile
							strcpy(g_path_hardfile, path);
							strcat(g_path_hardfile, tmpnames[res2]);
							// now set the correct parameters for a new hardfile
							strcpy(changed_prefs.path_hardfile,"0,");
							strcat(changed_prefs.path_hardfile, g_path_hardfile);
							strcat(changed_prefs.path_hardfile,",32,1,2,512");

							//parse_hardfile_spec(changed_prefs.path_hardfile);
							exit = 1;
						}
						else
						{						
							// insert floppy
							strcpy(changed_prefs.df[res], path);
							strcat(changed_prefs.df[res], tmpnames[res2]);

							// auto load options
							strcpy(diskname,tmpnames[res2]);
							remove_extension(diskname);
							LoadPSPUAEOptions(-1,diskname,"",0);
							exit = 1;
						}
					}
				}

			}	// end else if(res2 == -1)

			i=0;
			for (i=0; i < nb; i++)
				free(tmpnames[i]);

			if (exit)
			{
				//removeMenu2((const char **)&tmpnames,140);
				removeMenu4(140);

				removeMenu2(floppymenu,100);
				break;
			}

		}	// end inner while(1)

		lastdx[2]=0;
		removepath(140);
		browse_folders=0;

	}	// end outer while(1)

	removeMenu2(floppymenu,100);
}
// thinkp

void loadOptionsMenu()
{
// thinkp
	//loadOptions menu
	char *tmpnames[4096];
	memset(tmpnames,0,sizeof(tmpnames));
	char *tmpnames2[4096];
	memset(tmpnames2,0,sizeof(tmpnames2));

	//scan directory
	int nb=0,count=0;
	char path[4096];
	strcpy(path, g_path);
	strcat(path, "/CONFIGS/");
	int fd = sceIoDopen(path);
	if(fd>=0)
	{
		while(nb<4000)
		{
			if(sceIoDread(fd, &foundfile)<=0) break;
			if(foundfile.d_name[0] == '.') continue;
			if(FIO_SO_ISDIR(foundfile.d_stat.st_mode)) continue;
			char * pch;
			if((pch = strstr (foundfile.d_name,".options")) == NULL ) continue;
			if((pch = strstr (foundfile.d_name,"pspuae")) != NULL ) continue;
			char  entry[255];
			char  diskname[255];
			strcpy(diskname,foundfile.d_name);
			remove_extension(diskname);
			sprintf(entry, "Load Options %s", diskname);
			tmpnames[nb++]=strdup(entry);
			tmpnames2[count++]=strdup(diskname);

		}
		sceIoDclose(fd);
	}

	tmpnames[nb++]=strdup("Load Options #1");
	tmpnames[nb++]=strdup("Load Options #2");
	tmpnames[nb++]=strdup("Load Options #3");
	tmpnames[nb++]=strdup("Load Options #4");
	tmpnames[nb++]=strdup("Load Options #5");

	int res2=doMenu((const char **)&tmpnames,0,NULL,150,2,NULL);
	if(res2 != -1)
	{	
		if(res2 >= count)
		{
			LoadPSPUAEOptions(res2-count+1,"","",0);			
		}
		else
		{		
			LoadPSPUAEOptions(-1,tmpnames2[res2],"",0);	
		}
		removeMenu2((const char **)&tmpnames,150);
	}

	int i=0;
	for (i=0; i < nb; i++)
		free(tmpnames[i]);
	for (i=0; i < count; i++)
		free(tmpnames2[i]);

	lastdx[2]=0;
// thinkp
}

void saveOptionsMenu()
{
// thinkp
	//saveOptions menu
	char *tmpnames[10];
	memset(tmpnames,0,sizeof(tmpnames));
	int entrytype[10];
	memset(entrytype,0,sizeof(entrytype));

	int nb=0;
	char  diskname[255];
	char  statename[255];

	if ( !(strlen(changed_prefs.df[0]) == 0 && strlen(changed_prefs.df[1]) == 0))
	{
		if(strlen(changed_prefs.df[1]) != 0)
			strcpy(diskname,changed_prefs.df[1]);
		if(strlen(changed_prefs.df[0]) != 0)
			strcpy(diskname,changed_prefs.df[0]);

		remove_extension(diskname);
		remove_path(diskname);
		char  entry[255];
		sprintf(entry, "Save Options %s", diskname);
		entrytype[nb]=1;
		tmpnames[nb++]=strdup(entry);
	}

	entrytype[nb]=0;
	tmpnames[nb++]=strdup("Save Options #1");
	entrytype[nb]=0;
	tmpnames[nb++]=strdup("Save Options #2");
	entrytype[nb]=0;
	tmpnames[nb++]=strdup("Save Options #3");
	entrytype[nb]=0;
	tmpnames[nb++]=strdup("Save Options #4");
	entrytype[nb]=0;
	tmpnames[nb++]=strdup("Save Options #5");

	if (strlen(lastsavestate) > 0 )
	{
	// previous savestate name
		strcpy(statename, lastsavestate);
		remove_extension(statename);
		char  entry[255];
		sprintf(entry, "Save Options for Savestate %s", statename);
		entrytype[nb]=2;
		tmpnames[nb++]=strdup(entry);
	}


	int res2=doMenu((const char **)&tmpnames,0,NULL,150,2,NULL);
	if(res2 != -1)
	{
		if(res2 == 0 && entrytype[res2]==1)
		{
		// save named options in /CONFIGS
			SavePSPUAEOptions(-1,diskname,"");
		}
		else
		{
			if (entrytype[res2]==2)
			// save named options for named savestate in /CONFIGS
				SavePSPUAEOptions(-1,statename,"");
			else
			// save numbered options in /CONFIGS
			SavePSPUAEOptions(res2+((entrytype[0]==1)?0:1),"","");
		}
		removeMenu2((const char **)&tmpnames,150);
	}
	int i=0;
	for (i=0; i < nb; i++)
		free(tmpnames[i]);
	lastdx[2]=0;
// thinkp
}

void cpuOptionsMenu()
{
// thinkp
	static int lastsel;
	while(!quit_program)
	{
		int option = 0, res2;
		int extraoption = 0;

#ifndef CPUEMU_68000_ONLY
		sprintf((char*)(cpuoptionsmenu[option++]), "CPU Type            : %s", cpumenu[changed_prefs.cpu_level]);
#endif
#ifdef CYCLE_SWITCHUI
		sprintf((char*)(cpuoptionsmenu[option++]), "CPU Cycles          : %s", g_cpuspeed_up?"Normal":"Turbo");
		sprintf((char*)(cpuoptionsmenu[option++]), "CPU Cycles Option   : %s", g_cpulock?"Locked":"Un-Locked");
#endif
		int temp = changed_prefs.m68k_speed+1;
		if (temp>1) temp = 2;
		sprintf((char*)(cpuoptionsmenu[option++]), "CPU Speed           : %s", cpuspeedmenu[temp]);

		extraoption = option;

		if(temp == 2)
		{
			cpuoptionsmenu[option]=strdup("CPU to Chipset Ratio: xxxx");
			sprintf((char*)(cpuoptionsmenu[option++]), "CPU to Chipset Ratio: %s", cpu2chipmenu[g_cpu2chip_ratio]);
		}
		else
			cpuoptionsmenu[option++]=strdup("");

		int res = doMenu(cpuoptionsmenu, lastsel, NULL,150,2,NULL);
		if(res == -1)
		{
			int i;
			for (i=extraoption; i < option; i++)
					free(cpuoptionsmenu[i]);
			break;
		}

		lastsel = res;
#ifdef CPUEMU_68000_ONLY
		if (res > 0) res++; // we dont have the ability to select CPU
#endif
		switch(res)
		{
		case 0 :
			res2 = doMenu(cpumenu, changed_prefs.cpu_level,NULL,200,3,NULL);
			if(res2 != -1)
			{
				changed_prefs.cpu_level = res2;
				if (res2>0) changed_prefs.cpu_compatible=0;
				if (res2>2) changed_prefs.address_space_24=0;
				else changed_prefs.address_space_24=1;
				removeMenu2(cpumenu,200);
			}
			lastdx[3]=0;
			break;
#ifdef CYCLE_SWITCHUI
		case 1 :
			if (!g_cpulock) {
			g_cpuspeed_up = !g_cpuspeed_up;
			if(!g_cpuspeed_up) {
				g_cputurbo = 256;
#ifdef HIGH_CHIPSET
				g_chipsetturbo = 16384;
#else
				g_chipsetturbo = 8192;
#endif
			}
			else if(g_cpuspeed_up) {
				g_cputurbo = 512;
				g_chipsetturbo = 1;
			}
			uae_restart (1, NULL);
			if(g_autozoom = 1) g_autozoom = 0;
			}
			break;
		case 2 :
			g_cpulock = !g_cpulock;
			break;
		case 3 :
			temp = changed_prefs.m68k_speed+1;
			if (temp>1) temp = 2;
			res2 = doMenu(cpuspeedmenu, temp,NULL,200,3,NULL);
			if(res2 != -1)
			{
				if (res2 > 1)
					changed_prefs.m68k_speed = m68k_speed[g_cpu2chip_ratio];
				else
					changed_prefs.m68k_speed = res2-1;

				removeMenu2(cpuspeedmenu,200);
				removeMenu2(cpuoptionsmenu,150);
			}
			lastdx[3]=0;
			break;
		case 4 :
			res2 = doMenu(cpu2chipmenu, g_cpu2chip_ratio,NULL,200,3,NULL);
			if(res2 != -1)
			{
				g_cpu2chip_ratio = res2;
				changed_prefs.m68k_speed = m68k_speed[g_cpu2chip_ratio];
				removeMenu2(cpu2chipmenu,200);
			}
			lastdx[3]=0;
			break;
#else
		case 1 :
			temp = changed_prefs.m68k_speed+1;
			if (temp>1) temp = 2;
			res2 = doMenu(cpuspeedmenu, temp,NULL,200,3,NULL);
			if(res2 != -1)
			{
				if (res2 > 1)
					changed_prefs.m68k_speed = m68k_speed[g_cpu2chip_ratio];
				else
					changed_prefs.m68k_speed = res2-1;

				removeMenu2(cpuspeedmenu,200);
				removeMenu2(cpuoptionsmenu,150);
			}
			lastdx[3]=0;
			break;
		case 2 :
			res2 = doMenu(cpu2chipmenu, g_cpu2chip_ratio,NULL,200,3,NULL);
			if(res2 != -1)
			{
				g_cpu2chip_ratio = res2;
				changed_prefs.m68k_speed = m68k_speed[g_cpu2chip_ratio];
				removeMenu2(cpu2chipmenu,200);
			}
			lastdx[3]=0;
			break;
#endif
		}

		int i;
		for (i=extraoption; i < option; i++)
				free(cpuoptionsmenu[i]);
    }
	lastdx[2]=0;
// thinkp
}

void audioOptionsMenu()
{
	static int lastsel;
	while(!quit_program)
	{
		int option = 0, res2;
		sprintf((char*)(audiooptionsmenu[option++]), "Sound Emulation: %s", soundmenu[changed_prefs.produce_sound]);
		sprintf((char*)(audiooptionsmenu[option++]), "Snd Seperation : %s", sepmenu[changed_prefs.sound_stereo_separation]);
//		sprintf((char*)(audiooptionsmenu[option++]), "Sound Optimise : %s", g_audio_opt?"ON":"OFF");
#ifdef SOUND_DELAY
		sprintf((char*)(audiooptionsmenu[option++]), "Sound Delay    : %s", g_sdswitch?"1ms":"9ms");
#endif
		int res = doMenu(audiooptionsmenu, lastsel, NULL,200,3,NULL);
		if(res == -1) break;
		lastsel = res;
		switch(res)
		{
		case 0 :
			res2 = doMenu(soundmenu, changed_prefs.produce_sound,NULL,250,4,NULL);
			if(res2 != -1)
			{
				changed_prefs.produce_sound = res2;
				removeMenu2(soundmenu,250);
			}
			lastdx[4]=0;
			break;
		case 1 :
			res2 = doMenu(sepmenu, changed_prefs.sound_stereo_separation,NULL,250,4,NULL);
			if(res2 != -1)
			{
				changed_prefs.sound_stereo_separation = res2;
				removeMenu2(sepmenu,250);
			}
			lastdx[4]=0;
			break;
//		case 2 :
//			g_audio_opt = !g_audio_opt;
//			break;
#ifdef SOUND_DELAY
		case 2 :
			g_sdswitch = !g_sdswitch;
			if(!g_sdswitch) {
				g_snddelay = 9000;
			}
			else if(g_sdswitch) {
				g_snddelay = 1000;
			}
			break;
#endif
		}
	}
	lastdx[3]=0;
}

void kickstartMenu()
{
	int res = doMenu(ksmenu, g_kickstart_rom,NULL,150,2,NULL);
	if(res != -1)
	{
		g_kickstart_rom = res;
		changeKickstart(g_kickstart_rom);
		removeMenu2(ksmenu,150);
	}
	lastdx[2]=0;
}

void ddMenu()
{
	static int lastsel;
	while(!quit_program)
	{
		int option = 0;
		sprintf((char*)(ddmenu[option++]), "Floppy Speed : %s", changed_prefs.floppy_speed?"Normal":"Turbo");
#ifdef FOUR_DRIVES
		sprintf((char*)(ddmenu[option++]), "DF0 disk type: %s", changed_prefs.dfxtype[0]?"3.5 HD":"3.5 DD");
		sprintf((char*)(ddmenu[option++]), "DF1 disk type: %s", changed_prefs.dfxtype[1]?"3.5 HD":"3.5 DD");
		sprintf((char*)(ddmenu[option++]), "DF2 disk type: %s", changed_prefs.dfxtype[2]?"3.5 HD":"3.5 DD");
		sprintf((char*)(ddmenu[option++]), "DF3 disk type: %s", changed_prefs.dfxtype[3]?"3.5 HD":"3.5 DD");
#else
		sprintf((char*)(ddmenu[option++]), "DF0 disk type: %s", changed_prefs.dfxtype[0]?"3.5 HD":"3.5 DD");
		sprintf((char*)(ddmenu[option++]), "DF1 disk type: %s", changed_prefs.dfxtype[1]?"3.5 HD":"3.5 DD");
#endif
		int res = doMenu(ddmenu, lastsel, NULL,150,2,NULL);
		if(res == -1) break;
		lastsel = res;
		switch(res)
		{
		case 0 :
			if(changed_prefs.floppy_speed == 100) changed_prefs.floppy_speed = 0;
			else changed_prefs.floppy_speed = 100;
			break;
#ifdef FOUR_DRIVES
		case 1 :
			if(changed_prefs.dfxtype[0] == 1) changed_prefs.dfxtype[0] = 0;
			else changed_prefs.dfxtype[0] = 1;
			break;
		case 2 :
			if(changed_prefs.dfxtype[1] == 1) changed_prefs.dfxtype[1] = 0;
			else changed_prefs.dfxtype[1] = 1;
			break;
		case 3 :
			if(changed_prefs.dfxtype[2] == 1) changed_prefs.dfxtype[2] = 0;
			else changed_prefs.dfxtype[2] = 1;
			break;
		case 4 :
			if(changed_prefs.dfxtype[3] == 1) changed_prefs.dfxtype[3] = 0;
			else changed_prefs.dfxtype[3] = 1;
			break;
#else
		case 1 :
			if(changed_prefs.dfxtype[0] == 1) changed_prefs.dfxtype[0] = 0;
			else changed_prefs.dfxtype[0] = 1;
			break;
		case 2 :
			if(changed_prefs.dfxtype[1] == 1) changed_prefs.dfxtype[1] = 0;
			else changed_prefs.dfxtype[1] = 1;
			break;
#endif
		}
	    removeMenu2(ddmenu,150);
    }
	lastdx[2]=0;
}

void controlOptionsMenu()
{
// thinkp

// macros
	#define cmenu_assign_key(KEY)						\
	{									\
		if ( key >= 0 )							\
			KEY = key+BT_KEY_ENTRIES;				\
		else								\
		{								\
			if(extrainfo==-2) KEY=-1;				\
			else							\
			if((KEY+=extrainfo)>(BT_KEY_ENTRIES+129)) KEY=-1;	\
			if(KEY<-1) KEY=(BT_KEY_ENTRIES+129);			\
		}								\
	}


	#define cmenu_add_entry(KEY, ENTRY1, ENTRY2)										\
	{															\
		if(KEY<BT_KEY_ENTRIES)												\
		{														\
			if(KEY<0)												\
				sprintf((char*)(controloptionsmenu[option++]),ENTRY1);						\
			else													\
				sprintf((char*)(controloptionsmenu[option++]),ENTRY2,bt_strings[KEY]);				\
		}														\
		else														\
		if(KEY<BT_KEY_ENTRIES+94)											\
			sprintf((char*)(controloptionsmenu[option++]),ENTRY2,AmigaKeyboard[KEY-BT_KEY_ENTRIES].Name);		\
		else														\
		if(KEY<BT_KEY_ENTRIES+118)											\
			sprintf((char*)(controloptionsmenu[option++]),ENTRY2,mousestates[KEY-(BT_KEY_ENTRIES+94)].name);	\
		else														\
		if(KEY>=BT_KEY_ENTRIES+118 && KEY<BT_KEY_ENTRIES+130)								\
			sprintf((char*)(controloptionsmenu[option++]),ENTRY2,bt_strings[12+KEY-(BT_KEY_ENTRIES+118)]);		\
		else														\
			sprintf((char*)(controloptionsmenu[option++]),ENTRY2,"not defined");					\
	}

	char	extrainfo=0;
	int		key;
	 int lastsel=0;
	int		old_dirbuttons=0;

	if ( g_dirbuttons != AS_KEYS && lastsel > 8 )
		lastsel = 0;

	while(!quit_program)
	{
		int option = 0;
		int extraoption = 0;
		sprintf((char*)(controloptionsmenu[option++]), "Analog Stick         : %s",inputselection[g_analogstick]);
		sprintf((char*)(controloptionsmenu[option++]), "Directional Buttons  : %s",inputselection2[g_dirbuttons]);

		cmenu_add_entry(g_start,	"Start                :","Start                : %s")
		cmenu_add_entry(g_square,	"Square               :","Square               : %s")
		cmenu_add_entry(g_triangle,	"Triangle             :","Triangle             : %s")
		cmenu_add_entry(g_cross,	"Cross                :","Cross                : %s")
		cmenu_add_entry(g_circle,	"Circle               :","Circle               : %s")
		cmenu_add_entry(g_lshoulder,"Left Shoulder Button :","Left Shoulder Button : %s")
		cmenu_add_entry(g_rshoulder,"Right Shoulder Button:","Right Shoulder Button: %s")
		if (g_disablekeycombos == 0)
		{
			cmenu_add_entry(g_lsquare,	"L+Square             :","L+Square             : %s")
			cmenu_add_entry(g_ltriangle,"L+Triangle           :","L+Triangle           : %s")
			cmenu_add_entry(g_lcross,	"L+Cross              :","L+Cross              : %s")
			cmenu_add_entry(g_lcircle,	"L+Circle             :","L+Circle             : %s")
			cmenu_add_entry(g_rsquare,	"R+Square             :","R+Square             : %s")
			cmenu_add_entry(g_rtriangle,"R+Triangle           :","R+Triangle           : %s")
			cmenu_add_entry(g_rcross,	"R+Cross              :","R+Cross              : %s")
			cmenu_add_entry(g_rcircle,	"R+Circle             :","R+Circle             : %s")
		}
		else
		{
			cmenu_add_entry(g_lsquare,	"n/a L+Square         :","n/a L+Square         : %s")
			cmenu_add_entry(g_ltriangle,"n/a L+Triangle       :","n/a L+Triangle       : %s")
			cmenu_add_entry(g_lcross,	"n/a L+Cross          :","n/a L+Cross          : %s")
			cmenu_add_entry(g_lcircle,	"n/a L+Circle         :","n/a L+Circle         : %s")
			cmenu_add_entry(g_rsquare,	"n/a R+Square         :","n/a R+Square         : %s")
			cmenu_add_entry(g_rtriangle,"n/a R+Triangle       :","n/a R+Triangle       : %s")
			cmenu_add_entry(g_rcross,	"n/a R+Cross          :","n/a R+Cross          : %s")
			cmenu_add_entry(g_rcircle,	"n/a R+Circle         :","n/a R+Circle         : %s")
		}

		extraoption = option;

		if(g_dirbuttons==AS_KEYS)
		{
			controloptionsmenu[option]=strdup("Up      : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
			cmenu_add_entry(g_up	,	"Up                   :","Up                   : %s")
			controloptionsmenu[option]=strdup("Down    : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
			cmenu_add_entry(g_down	,	"Down                 :","Down                 : %s")
			controloptionsmenu[option]=strdup("Left    : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
			cmenu_add_entry(g_left	,	"Left                 :","Left                 : %s")
			controloptionsmenu[option]=strdup("Right   : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
			cmenu_add_entry(g_right	,	"Right                :","Right                : %s")

			if (g_disablekeycombos == 0)
			{
				controloptionsmenu[option]=strdup("L+Up    : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
				cmenu_add_entry(g_lup	,	"L+Up                 :","L+Up                 : %s")
				controloptionsmenu[option]=strdup("L+Down  : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
				cmenu_add_entry(g_ldown	,	"L+Down               :","L+Down               : %s")
				controloptionsmenu[option]=strdup("L+Left  : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
				cmenu_add_entry(g_lleft	,	"L+Left               :","L+Left               : %s")
				controloptionsmenu[option]=strdup("L+Right : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
				cmenu_add_entry(g_lright,	"L+Right              :","L+Right              : %s")
				controloptionsmenu[option]=strdup("R+Up    : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
				cmenu_add_entry(g_rup	,	"R+Up                 :","R+Up                 : %s")
				controloptionsmenu[option]=strdup("R+Down  : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
				cmenu_add_entry(g_rdown	,	"R+Down               :","R+Down               : %s")
				controloptionsmenu[option]=strdup("R+Left  : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
				cmenu_add_entry(g_rleft	,	"R+Left               :","R+Left               : %s")
				controloptionsmenu[option]=strdup("R+Right : xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
				cmenu_add_entry(g_rright,	"R+Right              :","R+Right              : %s")
			}
			else
			{
				controloptionsmenu[option++]=strdup("");
				controloptionsmenu[option++]=strdup("");
				controloptionsmenu[option++]=strdup("");
				controloptionsmenu[option++]=strdup("");
				controloptionsmenu[option++]=strdup("");
				controloptionsmenu[option++]=strdup("");
				controloptionsmenu[option++]=strdup("");
				controloptionsmenu[option++]=strdup("");
			}
		}
		else
		{
			controloptionsmenu[option++]=strdup("");
			controloptionsmenu[option++]=strdup("");
			controloptionsmenu[option++]=strdup("");
			controloptionsmenu[option++]=strdup("");
			controloptionsmenu[option++]=strdup("");
			controloptionsmenu[option++]=strdup("");
			controloptionsmenu[option++]=strdup("");
			controloptionsmenu[option++]=strdup("");
			controloptionsmenu[option++]=strdup("");
			controloptionsmenu[option++]=strdup("");
			controloptionsmenu[option++]=strdup("");
			controloptionsmenu[option++]=strdup("");
		}

		key=GetCurrentKey(lastsel);

		int res = doMenu(controloptionsmenu, lastsel, &extrainfo,150,2,&key);
		if(res == -1)
		{
			int i;
			for (i=extraoption; i < option; i++)
					free(controloptionsmenu[i]);
			break;
		}

		lastsel = res;
		old_dirbuttons = g_dirbuttons;

		switch(res)
		{
		case 0 :
			if((g_analogstick+=extrainfo)>AS_MOUSE) g_analogstick=AS_JOY0;
			if(g_analogstick<0) g_analogstick=AS_MOUSE;
			update_analogstick_config();
			break;
		case 1 :
			if((g_dirbuttons+=extrainfo)>AS_KEYS) g_dirbuttons=AS_JOY0;
			if(g_dirbuttons<0) g_dirbuttons=AS_KEYS;
			update_dirbuttons_config();
			break;
		case 2 :
			cmenu_assign_key(g_start)
			break;
		case 3 :
			cmenu_assign_key(g_square)
			break;
		case 4 :
			cmenu_assign_key(g_triangle)
			break;
		case 5 :
			cmenu_assign_key(g_cross)
			break;
		case 6 :
			cmenu_assign_key(g_circle)
			break;
		case 7 :
			cmenu_assign_key(g_lshoulder)
			break;
		case 8 :
			cmenu_assign_key(g_rshoulder)
			break;
		case 9 :
			if (g_disablekeycombos == 0)
				cmenu_assign_key(g_lsquare)
			break;
		case 10 :
			if (g_disablekeycombos == 0)
				cmenu_assign_key(g_ltriangle)
			break;
		case 11 :
			if (g_disablekeycombos == 0)
				cmenu_assign_key(g_lcross)
			break;
		case 12 :
			if (g_disablekeycombos == 0)
				cmenu_assign_key(g_lcircle)
			break;
		case 13 :
			if (g_disablekeycombos == 0)
				cmenu_assign_key(g_rsquare)
			break;
		case 14 :
			if (g_disablekeycombos == 0)
				cmenu_assign_key(g_rtriangle)
			break;
		case 15 :
			if (g_disablekeycombos == 0)
				cmenu_assign_key(g_rcross)
			break;
		case 16 :
			if (g_disablekeycombos == 0)
				cmenu_assign_key(g_rcircle)
			break;
		case 17 :
			cmenu_assign_key(g_up)
			break;
		case 18 :
			cmenu_assign_key(g_down)
			break;
		case 19 :
			cmenu_assign_key(g_left)
			break;
		case 20 :
			cmenu_assign_key(g_right)
			break;
		case 21 :
			cmenu_assign_key(g_lup)
			break;
		case 22 :
			cmenu_assign_key(g_ldown)
			break;
		case 23 :
			cmenu_assign_key(g_lleft)
			break;
		case 24 :
			cmenu_assign_key(g_lright)
			break;
		case 25 :
			cmenu_assign_key(g_rup)
			break;
		case 26 :
			cmenu_assign_key(g_rdown)
			break;
		case 27 :
			cmenu_assign_key(g_rleft)
			break;
		case 28 :
			cmenu_assign_key(g_rright)
			break;
		}

		if ((old_dirbuttons == AS_KEYS && old_dirbuttons != g_dirbuttons) ||
			(g_dirbuttons == AS_KEYS && old_dirbuttons != g_dirbuttons)	 )
			removeMenu2(controloptionsmenu,150);

		int i;
		for (i=extraoption; i < option; i++)
				free(controloptionsmenu[i]);
// thinkp
	}
}

void chipsetOptionsMenu()
{
	static int lastsel;
	while(!quit_program)
	{
		int option = 0, res2;
		sprintf((char*)(chipsetoptionsmenu[option++]), "Blitter Cycle Exact: %s", changed_prefs.blitter_cycle_exact?"ON":"OFF");
		sprintf((char*)(chipsetoptionsmenu[option++]), "Immediate Blits    : %s", changed_prefs.immediate_blits?"ON":"OFF");
		sprintf((char*)(chipsetoptionsmenu[option++]), "Collision Mode     : %s", collmenu[changed_prefs.collision_level]);
		sprintf((char*)(chipsetoptionsmenu[option++]), "Chipset            : %s", csmode[changed_prefs.chipset_mask]);
		sprintf((char*)(chipsetoptionsmenu[option++]), "Sound Emulation");
		int res = doMenu(chipsetoptionsmenu, lastsel, NULL,150,2,NULL);
		if(res == -1) break;
		lastsel = res;
		switch(res)
		{
		case 0 :
			if( changed_prefs.blitter_cycle_exact == 0)
			{
				changed_prefs.blitter_cycle_exact = 1;
				changed_prefs.immediate_blits = 0;
			}
			else changed_prefs.blitter_cycle_exact = 0;
			break;
		case 1 :
			if(changed_prefs.immediate_blits == 0)
			{
				changed_prefs.immediate_blits = 1;
				changed_prefs.blitter_cycle_exact = 0;
			}
			else changed_prefs.immediate_blits = 0;
			break;
		case 2 :
			res2 = doMenu(collmenu, changed_prefs.collision_level,NULL,200,3,NULL);
			if(res2 != -1)
			{
				changed_prefs.collision_level = res2;
				removeMenu2(collmenu,200);
			}
			lastdx[3]=0;
			break;
		case 3 :
			res2 = doMenu(csmode, changed_prefs.chipset_mask,NULL,200,3,NULL);
			if(res2 != -1)
			{
				changed_prefs.chipset_mask = res2;
				removeMenu2(csmode,200);
			}
			lastdx[3]=0;
			break;
        case 4 :
               audioOptionsMenu();
			break;
		}
	}
	lastdx[2]=0;
}

void videoOptionsMenu()
{
	static int lastsel;
	while(!quit_program)
	{
		int option = 0, res2;
		int extraoption = 0;
		sprintf((char*)(videooptionsmenu[option++]), "Direct VRAM Access: %s", changed_g_direct_screen_access?"ON":"OFF");
        sprintf((char*)(videooptionsmenu[option++]), "Frameskip         : %s", frskipmenu[g_frameskip]);
		sprintf((char*)(videooptionsmenu[option++]), "Zoom              : %s", zoommenu[g_zoom]);
		// thinkp
		sprintf((char*)(videooptionsmenu[option++]), "Aspect Ratio      : %s", aspectratiomenu[g_aspect_ratio]);
		sprintf((char*)(videooptionsmenu[option++]), "Auto Zoom         : %s", g_autozoom?"ON":"OFF");

		extraoption = option;
		if(g_autozoom == 0)
		{
			videooptionsmenu[option]=strdup("Screen Lock       : xxx");
			sprintf((char*)(videooptionsmenu[option++]), "Screen Lock       : %s", g_screenlock?"ON":"OFF");
		}
		else
			videooptionsmenu[option++]=strdup("");


		int res = doMenu(videooptionsmenu, lastsel, NULL,150,2,NULL);
		if(res == -1)
		{
			int i;
			for (i=extraoption; i < option; i++)
					free(videooptionsmenu[i]);
			break;
		}

		lastsel = res;
		switch(res)
		{
		case 0 :
			changed_g_direct_screen_access = !changed_g_direct_screen_access;
			g_zoom = 0;
			break;
        case 1 :
			res2 = doMenu(frskipmenu, g_frameskip,NULL,200,3,NULL);
			if(res2 != -1)
			{
				g_frameskip = res2;
#ifdef ADVANCED_FS
				changed_prefs.gfx_framerate	= g_frameskip-1;
				if (changed_prefs.gfx_framerate < 0)
					changed_prefs.gfx_framerate = 0;
#else
				changed_prefs.gfx_framerate	= g_frameskip+1;
#endif
				removeMenu2(frskipmenu,200);
			}
			lastdx[3]=0;
			break;
		case 2 :
			res2 = doMenu(zoommenu, g_zoom,NULL,200,3,NULL);
			if(res2 != -1)
			{
				g_zoom = res2;
				removeMenu2(zoommenu,200);
			}
			if (g_zoom > 0)
			{
				changed_g_direct_screen_access = 0;
			}
			lastdx[3]=0;
			break;
// thinkp
		case 3 :
			if (g_aspect_ratio == 0)
			{
				// 16:9
				g_aspect_ratio = 1;
				g_amiga_linesize=480;
			}
			else
			{
				// normal 4:3
				g_aspect_ratio = 0;
				g_ScreenX=-65;
				g_ScreenY=0-g_zoom*3;
				g_amiga_linesize=360;
			}
			lastdx[3]=0;
			break;
		case 4 :
			g_autozoom = !g_autozoom;
			if(g_autozoom)
				changed_g_direct_screen_access = 0;
			break;
		case 5 :
			g_screenlock = !g_screenlock;
			break;
// thinkp
		}

		int i;
		for (i=extraoption; i < option; i++)
				free(videooptionsmenu[i]);

	}

	lastdx[2]=0;
}

void miscMenu()
{
	static int lastsel;
	while(!quit_program)
	{
		int option = 0, res2;
		sprintf((char*)(miscmenu[option++]), "Mouse Speed         : %s", mousespeedmenu[g_mouse_speed-1]);
		sprintf((char*)(miscmenu[option++]), "Hi-Res Mouse Speed  : %s", mousespeedmenu[g_hr_mouse_speed-1]);
		sprintf((char*)(miscmenu[option++]), "Auto Mouse Speed    : %s", mousespeedmenu[g_automousespeed-1]);
		sprintf((char*)(miscmenu[option++]), "Show Leds           : %s", changed_prefs.leds_on_screen?"ON":"OFF");
		sprintf((char*)(miscmenu[option++]), "Transparent Keyboard: %s", g_solidkeyboard?"OFF":"ON");
		sprintf((char*)(miscmenu[option++]), "Disable Key Combos  : %s", g_disablekeycombos?"ON":"OFF");
		sprintf((char*)(miscmenu[option++]), "Toggle X+O          : %s", g_togglexo?"ON":"OFF");
		sprintf((char*)(miscmenu[option++]), "Background Image    : %s", g_bgnd_image?"ON":"OFF");

		int res = doMenu(miscmenu, lastsel, NULL,150,2,NULL);
		if(res == -1) break;
		lastsel = res;
		switch(res)
		{
		case 0 :
			res2 = doMenu(mousespeedmenu, g_mouse_speed-1,NULL,200,3,NULL);
			if(res2 != -1)
			{
				g_mouse_speed = res2+1;
				removeMenu2(mousespeedmenu,200);
			}
			lastdx[3]=0;
			break;
		case 1 :
			res2 = doMenu(mousespeedmenu, g_hr_mouse_speed-1,NULL,200,3,NULL);
			if(res2 != -1)
			{
				g_hr_mouse_speed = res2+1;
				removeMenu2(mousespeedmenu,200);
			}
			lastdx[3]=0;
			break;
		case 2 :
			res2 = doMenu(mousespeedmenu, g_automousespeed-1,NULL,200,3,NULL);
			if(res2 != -1)
			{
				g_automousespeed = res2+1;
				removeMenu2(mousespeedmenu,200);
			}
			lastdx[3]=0;
			break;
		case 3 :
			if(changed_prefs.leds_on_screen) changed_prefs.leds_on_screen = 0;
			else changed_prefs.leds_on_screen = 1;
			currprefs.leds_on_screen = changed_prefs.leds_on_screen;
			break;
		case 4 :
			if(++g_solidkeyboard>1) g_solidkeyboard=0;
			SolidKeyboard=g_solidkeyboard;
			break;
		case 5 :
			if(++g_disablekeycombos>1) g_disablekeycombos=0;
			break;
		case 6 :
			if(++g_togglexo>1) g_togglexo=0;
			break;
		case 7 :
			//FOL (Altered to fix crash, when no BackDrop Exists)
			if(g_bgnd_image == 1) g_bgnd_image = 0;
			else g_bgnd_image = 1;
			if(menuImage==NULL)
			{
				menuImage = loadImage(MENUIMAGEFILENAME);
			}
			else
			{
				free(menuImage->data);
				menuImage=NULL;
			}
			//FOL
			break;
		}
	}
	lastdx[2]=0;
}

//cmf
void ramOptionsMenu()
{
    int selected;
    static int lastsel;
	while(!quit_program)
	{
		int option = 0, res2, tmp;
		char tmp_str[8];

		tmp = changed_prefs.fastmem_size/1024;
		if(!tmp) sprintf(tmp_str, "none");
		else sprintf(tmp_str, "%iKB", tmp);
		sprintf((char*)(ramoptionsmenu[option++]), "Fastmem Size: %s", tmp_str);

		sprintf((char*)(ramoptionsmenu[option++]), "Chipmem Size: %iKB", changed_prefs.chipmem_size/1024);

		tmp = changed_prefs.bogomem_size/1024;
		if(!tmp) sprintf(tmp_str, "none");
		else sprintf(tmp_str, "%iKB", tmp);
		sprintf((char*)(ramoptionsmenu[option++]), "Bogomem Size: %s", tmp_str);

		int res = doMenu(ramoptionsmenu, lastsel, NULL,150,2,NULL);
		if(res == -1) break;
		lastsel = res;
		switch(res)
		{
        case 0 :
			selected = selected_mem(changed_prefs.fastmem_size, fastmemsizes);
			res2 = doMenu(fastmemmenu, selected, NULL,200,3,NULL);
			if (res2 >=0)
			{
			    changed_prefs.fastmem_size = ((uae_u32)fastmemsizes[res2] << 0x0A);
				removeMenu2(fastmemmenu,200);
			}
			break;
		case 1 :
			selected = selected_chipmem(changed_prefs.chipmem_size, chipmemsizes);
			res2 = doMenu(chipmemmenu, selected ,NULL,200,3,NULL);
			if (res2 >=0)
			{
			    changed_prefs.chipmem_size = ((uae_u32)chipmemsizes[res2] << 0x0A);
				removeMenu2(chipmemmenu,200);
			}
			break;
		case 2 :
			selected = selected_mem(changed_prefs.bogomem_size, bogomemsizes);
			res2 = doMenu(bogomemmenu, selected ,NULL,200,3,NULL);
			if (res2 >=0)
			{
			    changed_prefs.bogomem_size = ((uae_u32)bogomemsizes[res2] << 0x0A);
				removeMenu2(bogomemmenu,200);
			}
			break;
        }
	}
	lastdx[2]=0;
}
//cmf

void hardwareOptionsMenu()
{
	static int lastsel;
	while(!quit_program)
	{
		int option = 0;
        sprintf((char*)(hardwareoptionsmenu[option++]), "CPU");
        sprintf((char*)(hardwareoptionsmenu[option++]), "CHIPSET");
        sprintf((char*)(hardwareoptionsmenu[option++]), "RAM");
        sprintf((char*)(hardwareoptionsmenu[option++]), "Kickstart %s", ksmenu[g_kickstart_rom]);
        sprintf((char*)(hardwareoptionsmenu[option++]), "Drives Config");

		int res = doMenu(hardwareoptionsmenu, lastsel, NULL,100,1,NULL);
		if(res == -1) break;
		lastsel = res;
		switch(res)
		{
		case 0 :
               cpuOptionsMenu();
			break;
		case 1 :
               chipsetOptionsMenu();
			break;
		case 2 :
               ramOptionsMenu();
			break;
        case 3 :
               kickstartMenu();
			break;
        case 4 :
               ddMenu();
			break;
        }
	}
	lastdx[1]=0;
}

void hostOptionsMenu()
{
	static int lastsel;
	while(!quit_program)
	{
/*
		int option = 0;
        sprintf((char*)(hostoptionsmenu[option++]), "Display");
        sprintf((char*)(hostoptionsmenu[option++]), "Input Devices");
        sprintf((char*)(hostoptionsmenu[option++]), "Misc");
*/
		int res = doMenu(hostoptionsmenu, lastsel, NULL,100,1,NULL);
		if(res == -1) break;
		lastsel = res;
		switch(res)
		{
		case 0 :
               videoOptionsMenu();
			break;
		case 1 :
               controlOptionsMenu();
			break;
		case 2 :
               miscMenu();
			break;
		}
	}
	lastdx[1]=0;
}

void presetOptionsMenu()
{
	static int lastsel;
	while(!quit_program)
	{
/*
		int option = 0;
        sprintf((char*)(presetoptionsmenu[option++]), "Load Config A500  (OCS)");
		sprintf((char*)(presetoptionsmenu[option++]), "Load Config A600  (ECS Denise)");
		sprintf((char*)(presetoptionsmenu[option++]), "Load Config A1000 (OCS)");
		sprintf((char*)(presetoptionsmenu[option++]), "Load Config A1200 (ECS)");
		sprintf((char*)(presetoptionsmenu[option++]), "Load Default Config");
*/
    int res = doMenu(presetoptionsmenu, lastsel, NULL,150,2,NULL);
		if(res == -1) break;
		lastsel = res;
		switch(res)
		{
		case 0 :
			A500PSPUAEOptions();
			break;
		case 1 :
			A600PSPUAEOptions();				
			break;
		case 2 :
			A1000PSPUAEOptions();				
			break;
		case 3 :
			A1200PSPUAEOptions();					
			break;
		case 4 :
			DefaultPSPUAEOptions();		
			break;
		}
	}
	lastdx[1]=0;
}

void configMenu()
{
	static int lastsel;
	while(!quit_program)
	{
/*
		int option = 0;
        sprintf((char*)(configmenu[option++]), "Quick Start");
        sprintf((char*)(configmenu[option++]), "Load Config");
        sprintf((char*)(configmenu[option++]), "Save Config");
*/
		int res = doMenu(configmenu, lastsel, NULL,100,1,NULL);
		if(res == -1) break;
		lastsel = res;
		switch(res)
		{
		case 0 :
               presetOptionsMenu();
			break;
		case 1 :
               loadOptionsMenu();
			break;
		case 2 :
               saveOptionsMenu();
			break;
        }
	}
	lastdx[1]=0;
}

void showMenu()
{
// thinkp
	static int lastsel = 0;
	//menu
	MenuGo=1;
	int old_g_direct_screen_access = g_direct_screen_access;
	g_direct_screen_access = 0;
	splashScreen();
	while(MenuGo)
	{
		if (quit_program)
			break;
		int res = doMenu(mainmenu, lastsel, NULL,50,0,NULL);
		if(res == -1)
		{
			MenuGo=0;
			#ifdef PROFILING
			clearProfilingBuffer();
			return;
			#endif
			break;
		}
		lastsel = res;
		if(res == 0)
        {
            statesMenu();
		}
		else if (res == 1)
        {
        	diskMenu();
		}
        else if (res == 2)
        {
			hardwareOptionsMenu();
		}
        else if (res == 3)
        {
			hostOptionsMenu();
		}
		else if (res == 4)
        {
			configMenu();
		}
        else if (res == 5)
        {
			//reset amiga
			signal_reset = 7; // reset on 6th vsync to allow e-uae to pick up changed_prefs
			strcpy(lastsavestate, "");
			lastsel = 0;
			//FOL Added to stop the crash when reseting with autozoom
			if(g_autozoom == 1) g_autozoom = 0;
			//FOL
			break;
		}
#ifdef HARD_RESETCOM
		else if (res == 6)
        {
			//hard reset amiga
			uae_restart (1, NULL);
			strcpy(lastsavestate, "");
			lastsel = 0;
			//FOL Added to stop the crash when reseting with autozoom
			if(g_autozoom == 1) g_autozoom = 0;
			//FOL
			break;
		}
#else
		else if (res == 6)
		{
			//quit
			quitprogram();
			break;
		}
#endif
#ifdef HARD_RESETCOM
		else if (res == 7)
		{
			//quit
			quitprogram();
			break;
		}
#endif
	}
	clearScreen();

	ColorLineColor=0xFFFF;
	lof_changed=1;
	// restore drawing mode
	g_direct_screen_access = old_g_direct_screen_access;
	if (g_direct_screen_access) restoreScreenBuffer();
	force_update_frame();
	if(Select_Exit)
	{
		int res2=1;
		while(res2)
		{
#ifdef NO_VSYNC
				sceCtrlPeekBufferPositive(&ctl,1);
#else
				sceCtrlReadBufferPositive(&ctl,1);
#endif
			if(!(ctl.Buttons&PSP_CTRL_SELECT)) res2=0;
			sceDisplayWaitVblankStart();
		}
	}
}

static int g_t1 = 0;
static int mctl=0;
static int mpos=0;
int	ax;
int	ay;
int	prev_ax=-1;
int	prev_ay=-1;

void handle_events (void)	//update mouse pos.
{
#ifdef PROFILING
	PROFILINGBEGIN
#endif

// thinkp
static int old_changed_g_direct_screen_access;
// thinkp

#ifdef NO_VSYNC
				sceCtrlPeekBufferPositive(&ctl,1);
#else
				sceCtrlReadBufferPositive(&ctl,1);
#endif

#ifdef PROFILING
PROFILINGENDNAME(PROFILINGINDEX,"handle_events_sceCtrlPeekBufferPositive")
#endif

	int ltrigger = ctl.Buttons & PSP_CTRL_LTRIGGER;
	int rtrigger = ctl.Buttons & PSP_CTRL_RTRIGGER;


// thinkp mousestates
	static int setx=0;
	static int sety=0;
	static int mcount=0;
	static int mbcount=0;
	static int releasembutton=0;

	if(releasembutton==1 && mbcount==0)
	{
		setmousebuttonstate(0, 0, 0);
		releasembutton=0;
		return;
	}

	if(mbcount>0)
	{
		mbcount--;
		return;
	}

	if(setx!=0 && sety!=0)
	{
		int x=0,y=0,mposnew, mctlnew;
		int automousespeed=0;

		automousespeed = 10+(5-g_automousespeed)*5;

		get_mouse(&mposnew, &mctlnew);
		x = (mposnew & 0xFF) * 2 + (mctlnew & 1);
		y = (mposnew >> 8) | ((mctlnew << 6) & 0x100);

		mcount--;

		if( mcount ==0 || (abs(setx-x)<2 && abs(sety-y)<2))
		{
			setx=0;
			sety=0;
			setmousebuttonstate(0, 0, 1);
			releasembutton=1;
			mbcount=7;
			return;
		}
		else
		{
			if ( x != setx )
			{
				if ( x > setx )
				{
					if ( abs(setx-x)>automousespeed )
						ax = automousespeed;
					else
						ax=1;
					lastmx-=(ax % 127);
				}
				else
				{
					if ( abs(setx-x)>automousespeed )
						ax = automousespeed;
					else
						ax=1;
					lastmx+=(ax % 127);
				}
				setmousestate (0, 0, lastmx, 255);
			}

			if ( y != sety )
			{
				if ( y > sety )
				{
					if(abs(sety-y)>automousespeed)
						ay = automousespeed;
					else
						ay=1;
					lastmy-=(ay % 127);

				}
				else
				{
					if(abs(sety-y)>automousespeed)
						ay = automousespeed;
					else
						ay=1;
					lastmy+=(ay % 127);
				}

				setmousestate (0, 1, lastmy, 255);
			}

			return;
		}
	}
// thinkp mousestates


// thinkp
	ax = (int)ctl.Lx-127;
	ay = (int)ctl.Ly-127;

	if(KeyboardActive == 0 && !(ltrigger && rtrigger && !g_screenlock))
	{
	// no mouse while keyboard active or moving screen
		if(g_analogstick==AS_MOUSE)		// Mouse on Analog
		{

			if(g_togglemousespeed || CheckActive(ctl.Buttons,BT_HIRES_MOUSE))	// If HiresMouse button is pressed, extra precision is required
			{
				switch (g_hr_mouse_speed)
				{
				case 1:
					if(ax < -THRESHOLD) lastmx+= ((ax+THRESHOLD)>>2);
					if(ax > THRESHOLD) lastmx+= ((ax-THRESHOLD)>>2);
					if(ay < -THRESHOLD) lastmy+= ((ay+THRESHOLD)>>2);
					if(ay > THRESHOLD) lastmy+= ((ay-THRESHOLD)>>2);
					break;
				case 2:
					if(ax < -THRESHOLD) lastmx+= ((ax+THRESHOLD)>>3);
					if(ax > THRESHOLD) lastmx+= ((ax-THRESHOLD)>>3);
					if(ay < -THRESHOLD) lastmy+= ((ay+THRESHOLD)>>3);
					if(ay > THRESHOLD) lastmy+= ((ay-THRESHOLD)>>3);
					break;
				case 3:
					if(ax < -THRESHOLD) lastmx+= ((ax+THRESHOLD)>>4);
					if(ax > THRESHOLD) lastmx+= ((ax-THRESHOLD)>>4);
					if(ay < -THRESHOLD) lastmy+= ((ay+THRESHOLD)>>4);
					if(ay > THRESHOLD) lastmy+= ((ay-THRESHOLD)>>4);
					break;
				case 4:
					if(ax < -THRESHOLD) lastmx+= ((ax+THRESHOLD)>>5);
					if(ax > THRESHOLD) lastmx+= ((ax-THRESHOLD)>>5);
					if(ay < -THRESHOLD) lastmy+= ((ay+THRESHOLD)>>5);
					if(ay > THRESHOLD) lastmy+= ((ay-THRESHOLD)>>5);
					break;
				case 5:
					if(ax < -THRESHOLD) lastmx+= ((ax+THRESHOLD)>>6);
					if(ax > THRESHOLD) lastmx+= ((ax-THRESHOLD)>>6);
					if(ay < -THRESHOLD) lastmy+= ((ay+THRESHOLD)>>6);
					if(ay > THRESHOLD) lastmy+= ((ay-THRESHOLD)>>6);
					break;
				}
			}
			else
			{
				switch (g_mouse_speed)
				{
				case 1:
					if(ax < -THRESHOLD) lastmx+= ((ax+THRESHOLD)>>2);
					if(ax > THRESHOLD) lastmx+= ((ax-THRESHOLD)>>2);
					if(ay < -THRESHOLD) lastmy+= ((ay+THRESHOLD)>>2);
					if(ay > THRESHOLD) lastmy+= ((ay-THRESHOLD)>>2);
					break;
				case 2:
					if(ax < -THRESHOLD) lastmx+= ((ax+THRESHOLD)>>3);
					if(ax > THRESHOLD) lastmx+= ((ax-THRESHOLD)>>3);
					if(ay < -THRESHOLD) lastmy+= ((ay+THRESHOLD)>>3);
					if(ay > THRESHOLD) lastmy+= ((ay-THRESHOLD)>>3);
					break;
				case 3:
					if(ax < -THRESHOLD) lastmx+= ((ax+THRESHOLD)>>4);
					if(ax > THRESHOLD) lastmx+= ((ax-THRESHOLD)>>4);
					if(ay < -THRESHOLD) lastmy+= ((ay+THRESHOLD)>>4);
					if(ay > THRESHOLD) lastmy+= ((ay-THRESHOLD)>>4);
					break;
				case 4:
					if(ax < -THRESHOLD) lastmx+= ((ax+THRESHOLD)>>5);
					if(ax > THRESHOLD) lastmx+= ((ax-THRESHOLD)>>5);
					if(ay < -THRESHOLD) lastmy+= ((ay+THRESHOLD)>>5);
					if(ay > THRESHOLD) lastmy+= ((ay-THRESHOLD)>>5);
					break;
				case 5:
					if(ax < -THRESHOLD) lastmx+= ((ax+THRESHOLD)>>6);
					if(ax > THRESHOLD) lastmx+= ((ax-THRESHOLD)>>6);
					if(ay < -THRESHOLD) lastmy+= ((ay+THRESHOLD)>>6);
					if(ay > THRESHOLD) lastmy+= ((ay-THRESHOLD)>>6);
					break;
				}
			}
		}
		// else thinkp dual mouse

		if(g_dirbuttons==AS_MOUSE)
		{
			if(CheckActive(ctl.Buttons,BT_HIRES_MOUSE)) ax=(6-g_hr_mouse_speed)*5; else ax=(6-g_mouse_speed)*10;
			if(ctl.Buttons&PSP_CTRL_LEFT) lastmx-=ax;
			if(ctl.Buttons&PSP_CTRL_RIGHT) lastmx+=ax;
			if(ctl.Buttons&PSP_CTRL_UP) lastmy-=ax;
			if(ctl.Buttons&PSP_CTRL_DOWN) lastmy+=ax;
		}

#ifdef PROFILING
PROFILINGENDNAME(PROFILINGINDEX+1,"handle_events_mouse")
#endif

		setmousestate (0, 0, lastmx, 255);
		setmousestate (0, 1, lastmy, 255);
	}

#ifdef PROFILING
PROFILINGENDNAME(PROFILINGINDEX+2,"handle_events_mousestate")
#endif

	if((CheckActive(ctl.Buttons,BT_ACTIVATE_KEYBOARD))&&(!(CheckActive(prev_ctl_buttons,BT_ACTIVATE_KEYBOARD))))	// Turn ON/OFF Keyboard
	{
		if(KeyboardActive)
		{
// thinkp
			changed_g_direct_screen_access = old_changed_g_direct_screen_access;
// thinkp
			KeyboardActive=0;
			clearScreen();
			force_update_frame();
		}
		else
		{
// thinkp
			old_changed_g_direct_screen_access = changed_g_direct_screen_access;
			changed_g_direct_screen_access = 0;
// thinkp
			KeyboardActive=1;
			force_update_frame();
		}
	}

	if(KeyboardActive)
	{
		CallKeyboardInput(ctl);
		force_update_frame();
		if(g_analogstick==AS_MOUSE)	// mouse is on analog, so temporarily transfer dir_buttons
		{
			buttonstate[0] = (ctl.Buttons & PSP_CTRL_LTRIGGER )>0;			//mouse left button
			buttonstate[2] = (ctl.Buttons & PSP_CTRL_RTRIGGER )>0;			//mouse right button
		}
	}
	else
	{
// thinkp

		if (ltrigger && rtrigger)
		{
	// quick config combos
			if (ctl.Buttons & PSP_CTRL_UP && !(prev_ctl_buttons & PSP_CTRL_UP))
			{
			// zoom in
				g_zoom++;

				if (g_zoom > 9)
					g_zoom = 0;

				if (g_zoom > 0)
					changed_g_direct_screen_access = 0;

				clearScreen();
				force_update_frame();
				prev_ctl_buttons=ctl.Buttons;
				return;
			}

			if (ctl.Buttons & PSP_CTRL_DOWN && !(prev_ctl_buttons & PSP_CTRL_DOWN))
			{
			// zoom out
				g_zoom--;

				if (g_zoom < 0)
					g_zoom = 9;

				if (g_zoom > 0)
					changed_g_direct_screen_access = 0;

				clearScreen();
				force_update_frame();
				prev_ctl_buttons=ctl.Buttons;
				return;
			}

			if (ctl.Buttons & PSP_CTRL_RIGHT && !(prev_ctl_buttons & PSP_CTRL_RIGHT))
			{
			// more frameskip
				g_frameskip++;

#ifdef ADVANCED_FS
				if (g_frameskip > 50)
					g_frameskip = 0;

				changed_prefs.gfx_framerate	= g_frameskip-1;
				if (changed_prefs.gfx_framerate < 0)
					changed_prefs.gfx_framerate = 0;
#else
				if (g_frameskip > 9)
					g_frameskip = 0;

				changed_prefs.gfx_framerate	= g_frameskip+1;
#endif
				prev_ctl_buttons=ctl.Buttons;
				return;
			}

			if (ctl.Buttons & PSP_CTRL_LEFT && !(prev_ctl_buttons & PSP_CTRL_LEFT))
			{
			// less frameskip
				g_frameskip--;

#ifdef ADVANCED_FS
				if (g_frameskip < 0)
					g_frameskip = 50;

				changed_prefs.gfx_framerate	= g_frameskip-1;
				if (changed_prefs.gfx_framerate < 0)
					changed_prefs.gfx_framerate = 0;
#else
				if (g_frameskip < 0)
					g_frameskip = 9;

				changed_prefs.gfx_framerate	= g_frameskip+1;
#endif
				prev_ctl_buttons=ctl.Buttons;
				return;
			}

			if (ctl.Buttons & PSP_CTRL_TRIANGLE && !(prev_ctl_buttons & PSP_CTRL_TRIANGLE))
			{
			// toogle CPU speed
				int temp = changed_prefs.m68k_speed+1;
				if (temp>1) temp = 2;

				temp++;

				if (temp > 2)
					temp=0;

				if (temp > 1)
					changed_prefs.m68k_speed = m68k_speed[g_cpu2chip_ratio];
				else
					changed_prefs.m68k_speed = temp-1;
				prev_ctl_buttons=ctl.Buttons;
				return;
			}

			if (ctl.Buttons & PSP_CTRL_SQUARE && !(prev_ctl_buttons & PSP_CTRL_SQUARE))
			{
			// toggle Floppy speed
				if(changed_prefs.floppy_speed == 100) changed_prefs.floppy_speed = 0;
				else changed_prefs.floppy_speed = 100;
				prev_ctl_buttons=ctl.Buttons;
				return;
			}

			if (ctl.Buttons & PSP_CTRL_CIRCLE && !(prev_ctl_buttons & PSP_CTRL_CIRCLE))
			{
			// toogle leds on screen
				if(changed_prefs.leds_on_screen) changed_prefs.leds_on_screen = 0;
				else changed_prefs.leds_on_screen = 1;
				currprefs.leds_on_screen = changed_prefs.leds_on_screen;
				clearScreen();
				force_update_frame();
				prev_ctl_buttons=ctl.Buttons;
				return;
			}

// thinkp mousestates
			if (ctl.Buttons & PSP_CTRL_CROSS && !(prev_ctl_buttons & PSP_CTRL_CROSS))
			{
			// save mouse position
				get_mouse(&mpos, &mctl);
				DoMouseStateMenu();
				MouseStatesAvailable();
				return;
			}
// thinkp mousestates

			if ((ctl.Buttons & PSP_CTRL_SELECT) && !(prev_ctl_buttons & PSP_CTRL_SELECT))
			// start timer
				g_t1 = sceKernelGetSystemTimeLow();

			if (!g_autozoom && g_t1 > 0 && (ctl.Buttons & PSP_CTRL_SELECT) && sceKernelGetSystemTimeLow() - g_t1 > 1000000)
			{
			// toggle screen lock
				g_t1 = -1;
				if (g_screenlock)
					g_screenlock=0;
				else
					g_screenlock=1;

				clearScreen();
				force_update_frame();

				prev_ctl_buttons=ctl.Buttons;
				return;
			}

			if (!(ctl.Buttons & PSP_CTRL_SELECT) && (prev_ctl_buttons & PSP_CTRL_SELECT))
			{
			// toggle auto/manual zoom
				if(g_t1 != -1)
				{
					if (g_autozoom)
						g_autozoom=0;
					else
						g_autozoom=1;

					g_screenlock=1;
					clearScreen();
					force_update_frame();

				}
				g_t1 = 0;
				prev_ctl_buttons=ctl.Buttons;
				return;
			}

			if (g_autozoom==0 && g_screenlock==0 && ((prev_ax!=ax) || (prev_ay!=ay)))
			{
				if(ax < -THRESHOLD)
				{
					g_ScreenX--;
					force_update_frame();
					prev_ctl_buttons=ctl.Buttons;
					return;
				}
				if(ax > THRESHOLD)
				{
					g_ScreenX++;
					force_update_frame();
					prev_ctl_buttons=ctl.Buttons;
					return;
				}
				if(ay < -THRESHOLD)
				{
					g_ScreenY--;
					force_update_frame();
					prev_ctl_buttons=ctl.Buttons;
					return;
				}
				if(ay > THRESHOLD)
				{
					g_ScreenY++;
					force_update_frame();
					prev_ctl_buttons=ctl.Buttons;
					return;
				}
			}

		}	// end if (ltrigger && rtrigger)
		else
			g_t1 = 0;

		prev_ax=ax;
		prev_ay=ay;

		if ( ctl.Buttons == 0 &&
			 (buttonstate[0]+buttonstate[1]+buttonstate[2]+
			 g_cross_active+g_circle_active+g_square_active+g_triangle_active+
			 g_lcross_active+g_lcircle_active+g_lsquare_active+g_ltriangle_active+
			 g_rcross_active+g_rcircle_active+g_rsquare_active+g_rtriangle_active+
			 g_lshoulder_active+g_rshoulder_active+g_start_active+
			 g_up_active+g_down_active+g_left_active+g_right_active+
			 g_lup_active+g_ldown_active+g_lleft_active+g_lright_active+
			 g_rup_active+g_rdown_active+g_rleft_active+g_rright_active==0)
		   )
		{
			prev_ctl_buttons=ctl.Buttons;
			if(do_screen_shot==2) do_screen_shot=0;		// Can take an other if previous is done...
			return;
		}

// thinkp

		if (g_disablekeycombos == 1)
		// disable key combos
			ltrigger = rtrigger = 0;

		if(CheckActive(ctl.Buttons,BT_LEFT_MOUSE))
		{
			if (buttonstate[0] == 0)
			{
				//mouse left button
				buttonstate[0] = 1;
				setmousebuttonstate(0, 0, buttonstate[0]);
			}
		}
		else
		{
			buttonstate[0] = 0;
			setmousebuttonstate(0, 0, buttonstate[0]);
		}

		if(CheckActive(ctl.Buttons,BT_MIDDLE_MOUSE))			//mouse middle button
		{
			if (buttonstate[1] == 0)
			{
				//mouse middle button
				buttonstate[1] = 1;
				setmousebuttonstate(0, 2, buttonstate[1]);
			}
		}
		else
		{
			buttonstate[1] = 0;
			setmousebuttonstate(0, 2, buttonstate[1]);
		}


		if(CheckActive(ctl.Buttons,BT_RIGHT_MOUSE))			//mouse right button
		{
			if (buttonstate[2] == 0)
			{
				//mouse right button
				buttonstate[2] = 1;
				setmousebuttonstate(0, 1, buttonstate[2]);
			}
		}
		else
		{
			buttonstate[2] = 0;
			setmousebuttonstate(0, 1, buttonstate[2]);
		}
#ifdef PROFILING
PROFILINGENDNAME(PROFILINGINDEX+3,"handle_events_mousebuttonstate")
#endif
// macro
	#define key_press(KEY, KEYACTIVE, BUTTON)	\
		if(KEY>=BT_KEY_ENTRIES && KEY<=BT_KEY_ENTRIES+93)					\
		{										\
			if(ctl.Buttons&BUTTON)				\
			{									\
				if (KEYACTIVE==0)				\
				{								\
					InvokeKeyPress(KEY);		\
					KEYACTIVE = 1;				\
				}								\
			}									\
			else if (KEYACTIVE)					\
			{									\
				ReleaseKeyPress(KEY);			\
				KEYACTIVE = 0;					\
			}									\
		}

	#define key_release(KEY, KEYACTIVE)											\
		if(KEYACTIVE == 1 && KEY>=BT_KEY_ENTRIES && KEY<=BT_KEY_ENTRIES+93)		\
		{																		\
			ReleaseKeyPress(KEY);												\
			KEYACTIVE = 0;														\
		}

// thinkp mousestates
	#define restore_mousestate(KEY, BUTTON)																		\
		if((ctl.Buttons&BUTTON) && setx+sety==0 && KEY>=BT_KEY_ENTRIES+94 && KEY<=BT_KEY_ENTRIES+117)			\
		{																										\
			setx = mousestates[KEY-BT_KEY_ENTRIES-94].x;														\
			sety = mousestates[KEY-BT_KEY_ENTRIES-94].y;														\
			if(setx+sety > 0)																					\
				mcount=100;																						\
			return;																								\
		}
// thinkp mousestates

		if (ltrigger && !rtrigger)
		{
			key_press(g_lcross,g_lcross_active,PSP_CTRL_CROSS)
			key_press(g_lcircle,g_lcircle_active,PSP_CTRL_CIRCLE)
			key_press(g_lsquare,g_lsquare_active,PSP_CTRL_SQUARE)
			key_press(g_ltriangle,g_ltriangle_active,PSP_CTRL_TRIANGLE)
			key_release(g_rcross,g_rcross_active)
			key_release(g_rcircle,g_rcircle_active)
			key_release(g_rsquare,g_rsquare_active)
			key_release(g_rtriangle,g_rtriangle_active)
// thinkp mousestates
			if(mousestates_available)
			{
				restore_mousestate(g_lcross,PSP_CTRL_CROSS)
				restore_mousestate(g_lcircle,PSP_CTRL_CIRCLE)
				restore_mousestate(g_lsquare,PSP_CTRL_SQUARE)
				restore_mousestate(g_ltriangle,PSP_CTRL_TRIANGLE)
			}
// thinkp mousestates
		}

		if (!ltrigger && rtrigger)
		{
			key_press(g_rcross,g_rcross_active,PSP_CTRL_CROSS)
			key_press(g_rcircle,g_rcircle_active,PSP_CTRL_CIRCLE)
			key_press(g_rsquare,g_rsquare_active,PSP_CTRL_SQUARE)
			key_press(g_rtriangle,g_rtriangle_active,PSP_CTRL_TRIANGLE)
			key_release(g_lcross,g_lcross_active)
			key_release(g_lcircle,g_lcircle_active)
			key_release(g_lsquare,g_lsquare_active)
			key_release(g_ltriangle,g_ltriangle_active)
// thinkp mousestates
			if(mousestates_available)
			{
				restore_mousestate(g_rcross,PSP_CTRL_CROSS)
				restore_mousestate(g_rcircle,PSP_CTRL_CIRCLE)
				restore_mousestate(g_rsquare,PSP_CTRL_SQUARE)
				restore_mousestate(g_rtriangle,PSP_CTRL_TRIANGLE)
			}
// thinkp mousestates
		}


		if (!ltrigger && !rtrigger)
		{
			key_press(g_cross,g_cross_active,PSP_CTRL_CROSS)
			key_press(g_circle,g_circle_active,PSP_CTRL_CIRCLE)
			key_press(g_square,g_square_active,PSP_CTRL_SQUARE)
			key_press(g_triangle,g_triangle_active,PSP_CTRL_TRIANGLE)
			key_release(g_lcross,g_lcross_active)
			key_release(g_lcircle,g_lcircle_active)
			key_release(g_lsquare,g_lsquare_active)
			key_release(g_ltriangle,g_ltriangle_active)
			key_release(g_rcross,g_rcross_active)
			key_release(g_rcircle,g_rcircle_active)
			key_release(g_rsquare,g_rsquare_active)
			key_release(g_rtriangle,g_rtriangle_active)
// thinkp mousestates
			if(mousestates_available)
			{
				restore_mousestate(g_cross,PSP_CTRL_CROSS)
				restore_mousestate(g_circle,PSP_CTRL_CIRCLE)
				restore_mousestate(g_square,PSP_CTRL_SQUARE)
				restore_mousestate(g_triangle,PSP_CTRL_TRIANGLE)
			}
// thinkp mousestates
		}

		if(g_lshoulder>=BT_KEY_ENTRIES && g_lshoulder<=BT_KEY_ENTRIES+93)
		{
			if( (g_disablekeycombos == 0 && ctl.Buttons==PSP_CTRL_LTRIGGER)	||
				(g_disablekeycombos == 1 && ctl.Buttons& PSP_CTRL_LTRIGGER)
			  )
			{
				if (g_lshoulder_active==0)
				{
					InvokeKeyPress(g_lshoulder);
					g_lshoulder_active = 1;
				}
			}
			else if (g_lshoulder_active)
			{
				//deactivate key
				ReleaseKeyPress(g_lshoulder);
				g_lshoulder_active = 0;
			}
		}

		if(g_rshoulder>=BT_KEY_ENTRIES && g_rshoulder<=BT_KEY_ENTRIES+93)
		{
			if( (g_disablekeycombos == 0 && ctl.Buttons==PSP_CTRL_RTRIGGER)	||
				(g_disablekeycombos == 1 && ctl.Buttons& PSP_CTRL_RTRIGGER)
			  )
			{
				if (g_rshoulder_active==0)
				{
					InvokeKeyPress(g_rshoulder);
					g_rshoulder_active = 1;
				}
			}
			else if (g_rshoulder_active)
			{
				//deactivate key
				ReleaseKeyPress(g_rshoulder);
				g_rshoulder_active = 0;
			}
		}

		key_press(g_start,g_start_active,PSP_CTRL_START)

// thinkp
		if(g_dirbuttons==AS_KEYS)
		{
			if (ltrigger && !rtrigger)
			{
				key_press(g_lup,g_lup_active,PSP_CTRL_UP)
				key_press(g_ldown,g_ldown_active,PSP_CTRL_DOWN)
				key_press(g_lleft,g_lleft_active,PSP_CTRL_LEFT)
				key_press(g_lright,g_lright_active,PSP_CTRL_RIGHT)
				key_release(g_rup,g_rup_active)
				key_release(g_rdown,g_rdown_active)
				key_release(g_rleft,g_rleft_active)
				key_release(g_rright,g_rright_active)
// thinkp mousestates
				if(mousestates_available)
				{
					restore_mousestate(g_lup,PSP_CTRL_UP)
					restore_mousestate(g_ldown,PSP_CTRL_DOWN)
					restore_mousestate(g_lleft,PSP_CTRL_LEFT)
					restore_mousestate(g_lright,PSP_CTRL_RIGHT)
				}
// thinkp mousestates
			}

			if (!ltrigger && rtrigger)
			{
				key_press(g_rup,g_rup_active,PSP_CTRL_UP)
				key_press(g_rdown,g_rdown_active,PSP_CTRL_DOWN)
				key_press(g_rleft,g_rleft_active,PSP_CTRL_LEFT)
				key_press(g_rright,g_rright_active,PSP_CTRL_RIGHT)
				key_release(g_lup,g_lup_active)
				key_release(g_ldown,g_ldown_active)
				key_release(g_lleft,g_lleft_active)
				key_release(g_lright,g_lright_active)
// thinkp mousestates
				if(mousestates_available)
				{
					restore_mousestate(g_rup,PSP_CTRL_UP)
					restore_mousestate(g_rdown,PSP_CTRL_DOWN)
					restore_mousestate(g_rleft,PSP_CTRL_LEFT)
					restore_mousestate(g_rright,PSP_CTRL_RIGHT)
				}
// thinkp mousestates
			}

			if (!ltrigger && !rtrigger)
			{
				key_press(g_up,g_up_active,PSP_CTRL_UP)
				key_press(g_down,g_down_active,PSP_CTRL_DOWN)
				key_press(g_left,g_left_active,PSP_CTRL_LEFT)
				key_press(g_right,g_right_active,PSP_CTRL_RIGHT)
				key_release(g_lup,g_lup_active)
				key_release(g_ldown,g_ldown_active)
				key_release(g_lleft,g_lleft_active)
				key_release(g_lright,g_lright_active)
				key_release(g_rup,g_rup_active)
				key_release(g_rdown,g_rdown_active)
				key_release(g_rleft,g_rleft_active)
				key_release(g_rright,g_rright_active)
// thinkp mousestates
				if(mousestates_available)
				{
					restore_mousestate(g_up,PSP_CTRL_UP)
					restore_mousestate(g_down,PSP_CTRL_DOWN)
					restore_mousestate(g_left,PSP_CTRL_LEFT)
					restore_mousestate(g_right,PSP_CTRL_RIGHT)
				}
// thinkp mousestates
			}
		}

// thinkp
	}
#ifdef PROFILING
PROFILINGENDNAME(PROFILINGINDEX+4,"handle_events_mouse")
#endif

	if(CheckActive(ctl.Buttons,BT_TOGGLEMOUSESPEED) && !CheckActive(prev_ctl_buttons,BT_TOGGLEMOUSESPEED))
		if(++g_togglemousespeed>1) g_togglemousespeed=0;

	if(CheckActive(ctl.Buttons,BT_SCREEN_SHOT))
	{
#ifdef PROFILING
	quitprogram();
	return;
#endif
		if(do_screen_shot==0)
		{
			do_screen_shot=1;
		}
		else
			psp_flush_screen(0,SCREEN_HEIGHT);
	}
	else
	{
		if(do_screen_shot==2) do_screen_shot=0;		// Can take an other if previous is done...
	}

	if(ctl.Buttons == PSP_CTRL_SELECT)		// MENU
	{
		showMenu();
	}

	prev_ctl_buttons=ctl.Buttons;
#ifdef PROFILING
	PROFILINGEND(PROFILINGINDEX+5)
#endif
}

void quitprogram()
{
	//set_special(regs, SPCFLAG_BRK);
	quit_program = -1;
}

#define JOYTHRESHOLD 96
void read_joystick(int nr, unsigned int *dir, int *buttonstate)	// 0, 1
{
	int	left = 0, right = 0, up = 0, down = 0;
	int ltrigger = ctl.Buttons & PSP_CTRL_LTRIGGER;
	int rtrigger = ctl.Buttons & PSP_CTRL_RTRIGGER;

	if(g_analogstick==nr && !(ltrigger && rtrigger))		// Joy0 / Joy1
	{
		if(ctl.Lx<(128-JOYTHRESHOLD)) left=1;
		if(ctl.Lx>(128+JOYTHRESHOLD)) right=1;
		if(ctl.Ly<(128-JOYTHRESHOLD)) up=1;
		if(ctl.Ly>(128+JOYTHRESHOLD)) down=1;

	}
// thinkp Dual Joystick control
//	else

	if((g_dirbuttons==nr) && (!KeyboardActive) && !(ltrigger && rtrigger))	// Joy0 / Joy1
	{
		if(ctl.Buttons&PSP_CTRL_LEFT) left=1;
		if(ctl.Buttons&PSP_CTRL_RIGHT) right=1;
		if(ctl.Buttons&PSP_CTRL_UP) up=1;
		if(ctl.Buttons&PSP_CTRL_DOWN) down=1;
	}
	/* handle regular joypad buttons */
	if(CheckActive(ctl.Buttons,(BT_JOY0_CD32_RED)) || CheckActive(ctl.Buttons,(BT_JOY0_FIRE+nr)))
		*buttonstate=1;
	else
		*buttonstate=0;
	if(CheckActive(ctl.Buttons,(BT_JOY0_CD32_BLUE)))
		setjoybuttonstate(0, 1, 1);
	else
		setjoybuttonstate(nr, 1, 0);

	/* handle cd32 joypad buttons */
	if(CheckActive(ctl.Buttons,(BT_JOY0_CD32_PLAY)))
		setjoybuttonstate(0, JOYBUTTON_CD32_PLAY, 1);
	else
		setjoybuttonstate(0, JOYBUTTON_CD32_PLAY, 0);
	if(CheckActive(ctl.Buttons,(BT_JOY0_CD32_RWD)))
		setjoybuttonstate(0, JOYBUTTON_CD32_RWD, 1);
	else
		setjoybuttonstate(nr, JOYBUTTON_CD32_RWD, 0);
	if(CheckActive(ctl.Buttons,(BT_JOY0_CD32_FFW)))
		setjoybuttonstate(0, JOYBUTTON_CD32_FFW, 1);
	else
		setjoybuttonstate(nr, JOYBUTTON_CD32_FFW, 0);
	if(CheckActive(ctl.Buttons,(BT_JOY0_CD32_GREEN)))
		setjoybuttonstate(0, JOYBUTTON_CD32_GREEN, 1);
	else
		setjoybuttonstate(nr, JOYBUTTON_CD32_GREEN, 0);
	if(CheckActive(ctl.Buttons,(BT_JOY0_CD32_YELLOW)))
		setjoybuttonstate(0, JOYBUTTON_CD32_YELLOW, 1);
	else
		setjoybuttonstate(nr, JOYBUTTON_CD32_YELLOW, 0);
	if(CheckActive(ctl.Buttons,(BT_JOY0_CD32_RED)))
		setjoybuttonstate(0, JOYBUTTON_CD32_RED, 1);
	else
		setjoybuttonstate(nr, JOYBUTTON_CD32_RED, 0);
	if(CheckActive(ctl.Buttons,(BT_JOY0_CD32_BLUE)))
		setjoybuttonstate(0, JOYBUTTON_CD32_BLUE, 1);
	else
		setjoybuttonstate(nr, JOYBUTTON_CD32_BLUE, 0);

	switch(nr)
	{
		case 0:
			//Mr.Modem
			if(CheckActive(ctl.Buttons,(BT_JOY0_UP)))up=1;
			if(CheckActive(ctl.Buttons,(BT_JOY0_DOWN)))down=1;
			if(CheckActive(ctl.Buttons,(BT_JOY0_LEFT)))left=1;
			if(CheckActive(ctl.Buttons,(BT_JOY0_RIGHT)))right=1;
			//End Mr.Modem
		break;

		case 1:
			//thinkp
			if(CheckActive(ctl.Buttons,(BT_JOY1_UP)))up=1;
			if(CheckActive(ctl.Buttons,(BT_JOY1_DOWN)))down=1;
			if(CheckActive(ctl.Buttons,(BT_JOY1_LEFT)))left=1;
			if(CheckActive(ctl.Buttons,(BT_JOY1_RIGHT)))right=1;
			//End thinkp
		break;
	}

	//Ric
	//if(left) up = !up;
	//if(right) down = !down;
	//*dir = down | (right << 1) | (up << 8) | (left << 9);

	*dir = up | (down << 1) | (left <<2) | (right <<3);
	//End Ric
}

void pspDebug(char *msg)
{
#ifdef DEBUGGER
	if (!changed_prefs.start_debugger) return;

	int i,j=0,len;
	len = strlen(msg);
	for (i=0; i<len; i++)
	{
		debug_buffer[current_debug_line][j++] = msg[i];
		if (j == DEBUGLINELENGTH -1)
		{
			debug_buffer[current_debug_line][DEBUGLINELENGTH-1] = 0;
			if (i != (len -1))
			{
				j = 1;
				current_debug_line++;
				if (current_debug_line == DEBUGLINES)
					current_debug_line = 0;
				// when one message spans multiple lines start with a space
				debug_buffer[current_debug_line][0] = ' ';
			}
		}
	}
	while (j < (DEBUGLINELENGTH -1))
	{
		debug_buffer[current_debug_line][j++] = ' ';
	}
	debug_buffer[current_debug_line][DEBUGLINELENGTH-1] = 0;
	current_debug_line++;
	if (current_debug_line == DEBUGLINES)
		current_debug_line = 0;
#endif
}

//Initializsation and main

/* Exit callback */
int exit_callback(int arg1, int arg2, void *common)
{
	quitprogram();
	return(0);
}

/* Callback thread */
int CallbackThread(SceSize args, void *argp)
{
int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return(0);
}

/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void)
{
	int thid = 0;

	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}

	return thid;
}

int main(int argc, char* argv[])
{
	int a, i;

	pspDebugScreenInit();
	SetupCallbacks();

//	DefaultPSPUAEOptions();

	for(a=0;((a<512)&&(argv[0][a]!='\0'));a++)
	{
		LaunchPath[a]=argv[0][a];
	}
	while((a>0)&&(LaunchPath[a]!='/')) --a;
	LaunchPath[a++]='/';
	LaunchPathEndPtr=a;
	LaunchPath[a]='\0';

//	sceDisplaySetMode( 0, SCREEN_WIDTH, SCREEN_HEIGHT );
//	sceDisplaySetFrameBuf( (char*)VRAM_ADDR, 512, 1, 1 );

	//update configuration to point to rom, floppies and hardfile

	char optionsPath[1024];
	{
	  strcpy(g_path, LaunchPath);
  	  char *p=g_path+strlen(g_path)-1;
	  while(*p!='/') p--;
	  *p=0;
	}
	strcpy(optionsPath, g_path);
	strcat(optionsPath, "/config.uae");

	OPTIONSFILENAME = optionsPath;

	SceUID fd;
	if ((fd = sceIoOpen(optionsPath, PSP_O_RDONLY, 0777))<0)
	{
		// no config file - let's try for ks 1.3
		i = 3; // 1.3
		if(changeKickstart(i+256))
		{
			// no ks 1.3 - let's just take the first valid ks
			for (i = 0; i < 9; i++)
				if (!changeKickstart(i+256))
					break;
		}
		g_kickstart_rom = (i<9) ? i : 3;
	} else
		sceIoClose(fd);

	initGU();
	//load the menu background image
	menuImage = loadImage(MENUIMAGEFILENAME);
	if (!menuImage) {
		//Image load failed
		// just use a black screen
	} else {
		//FOL
		if ((menuImage2 = loadImage(MENUIMAGEFILENAME2)))
		{
			splashScreen2();
			//FOL
			sceKernelDelayThread(3000000);	// FOL Changed to 3000000, keep splash up a bit longer
			fadeOutImage();
			clearScreen();
			free(menuImage2->data);
			free(menuImage2);
			menuImage2=NULL;
		}
	}

	pspAudioInit();
	pspAudioSetChannelCallback(0, (void *)&sound_callback, 0);
	//sceAudioChangeChannelConfig(0, PSP_AUDIO_FORMAT_MONO);
	pspAudioSetVolume(0, 0x8000, 0x8000);
	//LoadPSPUAEOptions(1);

	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(1);
	scePowerSetClockFrequency(333,333,166);

	real_main (0, NULL);
	//uae_restart (1, NULL);
#ifdef PROFILING
	saveProfilingResults();
#endif
	pspAudioEnd();
	sceKernelExitGame();
	return 0;
}

// --------------------- misc ------------------------

int is_fullscreen (void)
{
    return 0;
}

void toggle_fullscreen (void)
{
    /* FIXME: Add support for separate full-screen/windowed sizes */
};

void screenshot (int mode)
{

}

void gfx_default_options (struct uae_prefs *p)
{

}


void gfx_save_options (FILE *f, const struct uae_prefs *p)
{
}

int gfx_parse_option (struct uae_prefs *p, const char *option, const char *value)
{
  return 0;
}

void force_update_frame(void)
{
	notice_screen_contents_lost();
}

// configuration stuff
void target_save_options (FILE *f, const struct uae_prefs *p)
{
}

int target_parse_option (struct uae_prefs *p, const char *option, const char *value)
{
	return 0;
}

void parse_cmdline (int argc, char **argv)
{
}

uaecptr scsidev_startup (uaecptr resaddr)
{
	return 0;
}
void target_default_options (struct uae_prefs *p)
{
}

void setup_brkhandler (void)
{
}

int debuggable (void)
{
  return 0;
}

void usage (void)
{
}

// thinkp
u32* memsc;

void saveScreen() {
	memsc = (u32*) malloc(512 * 272 * sizeof(short));
	if (!memsc) return;
	u32* vram32;
    vram32 = (u32 *)getVramDrawBuffer();
    memcpy(memsc, vram32, 512 * 272 * sizeof(short));
}

void restoreScreen() {
	if (!memsc) return;
	sceDisplayWaitVblankStart();
	psp_pg_blit_background(memsc);
	flipScreen();
	sceDisplayWaitVblankStart();
	psp_pg_blit_background(memsc);
	flipScreen();
	free(memsc);
}

void psp_pg_blit_background(u32* bitmap)
{
  u32* vram32;
  if (bitmap) {
    vram32 = (u32 *)getVramDrawBuffer();
    memcpy(vram32, bitmap, 512 * 272 * sizeof(short));
  }
}

int GetCurrentKey(int sel)
{
	int key=0;

	switch(sel)
	{
	case 2 :
		key = g_start - BT_KEY_ENTRIES;
		break;
	case 3 :
		key = g_square - BT_KEY_ENTRIES;
		break;
	case 4 :
		key = g_triangle - BT_KEY_ENTRIES;
		break;
	case 5 :
		key = g_cross - BT_KEY_ENTRIES;
		break;
	case 6 :
		key = g_circle - BT_KEY_ENTRIES;
		break;
	case 7 :
		key = g_lshoulder - BT_KEY_ENTRIES;
		break;
	case 8 :
		key = g_rshoulder - BT_KEY_ENTRIES;
		break;
	case 9 :
		key = g_lsquare - BT_KEY_ENTRIES;
		break;
	case 10 :
		key = g_ltriangle - BT_KEY_ENTRIES;
		break;
	case 11 :
		key = g_lcross - BT_KEY_ENTRIES;
		break;
	case 12 :
		key = g_lcircle - BT_KEY_ENTRIES;
		break;
	case 13 :
		key = g_rsquare - BT_KEY_ENTRIES;
		break;
	case 14 :
		key = g_rtriangle - BT_KEY_ENTRIES;
		break;
	case 15 :
		key = g_rcross - BT_KEY_ENTRIES;
		break;
	case 16 :
		key = g_rcircle - BT_KEY_ENTRIES;
		break;
	case 17 :
		key = g_up - BT_KEY_ENTRIES;
		break;
	case 18 :
		key = g_down - BT_KEY_ENTRIES;
		break;
	case 19 :
		key = g_left - BT_KEY_ENTRIES;
		break;
	case 20 :
		key = g_right - BT_KEY_ENTRIES;
		break;
	case 21 :
		key = g_lup - BT_KEY_ENTRIES;
		break;
	case 22 :
		key = g_ldown - BT_KEY_ENTRIES;
		break;
	case 23 :
		key = g_lleft - BT_KEY_ENTRIES;
		break;
	case 24 :
		key = g_lright - BT_KEY_ENTRIES;
		break;
	case 25 :
		key = g_rup - BT_KEY_ENTRIES;
		break;
	case 26 :
		key = g_rdown - BT_KEY_ENTRIES;
		break;
	case 27 :
		key = g_rleft - BT_KEY_ENTRIES;
		break;
	case 28 :
		key = g_rright - BT_KEY_ENTRIES;
		break;

	default:
		key = -1;
		break;
	}

	return key;
}

void create_dir(char* path)
{
	// check if the dir exists
	char dirName[1024];
	sprintf(dirName, "%s%s", LaunchPath, path);
	int dfd  = sceIoDopen(dirName);
	if (dfd < 0)
	{
		// create savestate dir
		dfd = sceIoMkdir(dirName,0777);
	}
	else sceIoDclose(dfd);

}

void printpath(int x)
{
	blitAlphaImageToScreen(x, 15, 480, 10, menuImage, x, 15);
	text_print( x, 15, path, rgb2col(155,255,155),rgb2col(0,0,0),0);
	flipScreen();
	blitAlphaImageToScreen(x, 15, 480, 10, menuImage, x, 15);
	text_print( x, 15, path, rgb2col(155,255,155),rgb2col(0,0,0),0);
	flipScreen();
}

void removepath(int x)
{
	blitAlphaImageToScreen(x, 15, 480, 10, menuImage, x, 15);
	flipScreen();
	blitAlphaImageToScreen(x, 15, 480, 10, menuImage, x, 15);
	flipScreen();
}

void getax(int* x, int* y)
{
	*x = lastmx;
	*y = lastmy;
}

// thinkp mousestates
void InitMouseStates()
{
	int i=0;

	for(i=0; i<24; i++)
	{
		mousestates[i].x=0;
		mousestates[i].y=0;
		mousestates[i].mbutton=0;
		sprintf(mousestates[i].name,"Mousestate %d",i+1);
	}
	mousestates_available=0;
}

void MouseStatesAvailable()
{
	int i=0;
	mousestates_available=0;
	for(i=0; i<24; i++)
	{
		if(mousestates[i].x+mousestates[i].y>0)
		{
			mousestates_available=1;
			return;
		}
	}
}

void DoMouseStateMenu()
{
	static int lastsel;
	int old_g_direct_screen_access = g_direct_screen_access;
	g_direct_screen_access = 0;
	splashScreen();

	while(1)
	{
		int option = 0;
		int i=0;

		for(i=0; i<24; i++)
		{
			sprintf((char*)(mousestatemenu[option++]), "%s",mousestates[i].name);
		}

		domousestatemenu=1;
		int res = doMenu(mousestatemenu, lastsel, NULL,100,1,NULL);
		domousestatemenu=0;
		if(res == -1)
		{
			for(i=0; i<24; i++)
			// transfer edited mousestates names
				strcpy(mousestates[i].name, mousestatemenu[i]);
			break;
		}
		lastsel = res;
		if(res>=0 && res<24)
		{
			mousestates[res].x = (mpos & 0xFF) * 2 + (mctl & 1);
			mousestates[res].y = (mpos >> 8) | ((mctl << 6) & 0x100);
			mousestates[res].mbutton=1;
			for(i=0; i<24; i++)
			// transfer edited mousestates names
				strcpy(mousestates[i].name, mousestatemenu[i]);
			break;
		}
	}
	lastdx[1]=0;
	clearScreen();

	ColorLineColor=0xFFFF;
	lof_changed=1;
	// restore drawing mode
	g_direct_screen_access = old_g_direct_screen_access;
	if (g_direct_screen_access) restoreScreenBuffer();
	force_update_frame();

}
// thinkp mousestates
