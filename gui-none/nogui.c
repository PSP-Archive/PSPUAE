 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Interface to the Tcl/Tk GUI
  *
  * Copyright 1996 Bernd Schmidt
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "gui.h"

#include <psputility.h>
#include <pspgu.h>
#ifdef THREE_XX
#include <pspgum.h>
#endif

static void sigchldhandler(int foo)
{
}

int gui_init (void)
{
    return -1;
}

int gui_update (void)
{
    return 0;
}

void gui_exit (void)
{
}

void gui_fps (int fps, int idle)
{
    gui_data.fps  = fps;
    gui_data.idle = idle;
}

void gui_led (int led, int on)
{
}

void gui_hd_led (int led)
{
    static int resetcounter;

    int old = gui_data.hd;

    if (led == 0) {
	resetcounter--;
	if (resetcounter > 0)
	    return;
    }

    gui_data.hd = led;
    resetcounter = 6;
    if (old != gui_data.hd)
	gui_led (5, gui_data.hd);
}

void gui_cd_led (int led)
{
    static int resetcounter;

    int old = gui_data.cd;
    if (led == 0) {
	resetcounter--;
	if (resetcounter > 0)
	    return;
    }

    gui_data.cd = led;
    resetcounter = 6;
    if (old != gui_data.cd)
	gui_led (6, gui_data.cd);
}

void gui_filename (int num, const char *name)
{
}

void gui_handle_events (void)
{
}

void gui_changesettings (void)
{
}

void gui_update_gfx (void)
{
}

void gui_lock (void)
{
}

void gui_unlock (void)
{
}

void gui_display(int shortcut)
{
}

/* Utility dialog functions */

pspUtilityMsgDialogParams dialog; //FOL, Needed by new Toolchain.

static void ConfigureDialog(pspUtilityMsgDialogParams *dialog, size_t dialog_size)
{
    memset(dialog, 0, dialog_size);

    dialog->base.size = dialog_size;
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,
				&dialog->base.language); // Prompt language
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN,
				&dialog->base.buttonSwap); // X/O button swap

    dialog->base.graphicsThread = 0x11;
    dialog->base.accessThread = 0x13;
    dialog->base.fontThread = 0x12;
    dialog->base.soundThread = 0x10;
}

static void ShowMessageDialog(const char *message)
{
    ConfigureDialog(&dialog, sizeof(dialog));
    dialog.mode = PSP_UTILITY_MSGDIALOG_MODE_TEXT; //FOL, Needed by new toolchain.
    strcpy(dialog.message, message);
	
	copyDisplayToTextureBuffer(); //store old screen contents
//	clearScreen();
    sceUtilityMsgDialogInitStart(&dialog);
		
    for(;;) {
		drawNothing(); //this is necessary to force a buffer swap
		switch(sceUtilityMsgDialogGetStatus()) {
	    
		case 2:
			sceUtilityMsgDialogUpdate(2);	
	    break;
	    
		case 3:
			sceUtilityMsgDialogShutdownStart();
	    break;
	    
		case 0:
		//	clearScreen();
			copyTextureToDrawBuffer();		//restore screen contents
			copyTextureToDrawBuffer();		// twice to fill both buffers	
	    return;   
		}

		sceDisplayWaitVblankStart();
		flipScreen2();	
    }

	
}

void gui_message (const char *format,...)
{   
	checkGU();
    char msg[2048];
    va_list parms;
    va_start (parms,format);
    vsprintf ( msg, format, parms);
    va_end (parms);

    write_log (msg);
	   
	//ric 
	ShowMessageDialog(msg);			
	//end ric
}
