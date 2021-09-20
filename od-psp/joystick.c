 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Joystick emulation stubs
  *
  * Copyright 1997 Bernd Schmidt
  * Copyright 2003-2005 Richard Drummond
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "inputdevice.h"

//Ric
#define GFX_NAME "none"
#define DIR_LEFT 4
#define DIR_RIGHT 8
#define DIR_UP 1
#define DIR_DOWN 2
//End Ric

static unsigned int nr_joysticks;
extern unsigned int joydir[MAX_INPUT_DEVICE_EVENTS];
int pause_emulation;



static int init_joysticks (void)
{
    nr_joysticks = 2;
    return 1;
}

static void close_joysticks (void)
{
}

static unsigned int get_joystick_num (void)
{
    return nr_joysticks;
}

static const char *get_joystick_name (unsigned int joy)
{
    return strcat("Joystick ", '0' + joy);
}

static int acquire_joystick (unsigned int num, int flags)
{
    return num < get_joystick_num ();
}

static void unacquire_joystick (unsigned int num)
{
}

static void read_joysticks (void)
{
    unsigned int i, buttonstate, joydir;
    for (i = 0; i < get_joystick_num(); i++)
    {
	    read_joystick(i, &joydir, &buttonstate);
	    setjoybuttonstate(i, 0, buttonstate);
	    
	    if (joydir & DIR_LEFT)
	        setjoystickstate(i, 0, -1, 1);
   	    else if (joydir & DIR_RIGHT)
	        setjoystickstate(i, 0, 1, 1);
   	    else
	        setjoystickstate(i, 0, 0, 1); 
	
	    if (joydir & DIR_UP)
	        setjoystickstate(i, 1, -1, 1);
   	    else if (joydir & DIR_DOWN)
	        setjoystickstate(i, 1, 1, 1);	
   	    else
	        setjoystickstate(i, 1, 0, 1); 
    }
}

static unsigned int get_joystick_widget_num (unsigned int joy)
{
    return 2;
}

static int get_joystick_widget_type (unsigned int joy, unsigned int num, char *name, uae_u32 *code)
{
    if (num >= 2)
	return IDEV_WIDGET_BUTTON;	
    else if (num < 2) 
	return IDEV_WIDGET_AXIS;
    else	    
        return IDEV_WIDGET_NONE;
}

static int get_joystick_widget_first (unsigned int joy, int type)
{
    switch (type) {
	case IDEV_WIDGET_BUTTON:
	    return 2;
	case IDEV_WIDGET_AXIS:
	    return 0;
    }	
    return -1;
}

/*
 * Set default inputdevice config
 */
void input_get_default_joystick (struct uae_input_device *uid)
{
    unsigned int i, port;

    for (i = 0; i < nr_joysticks; i++) {
        port = i & 1;
        uid[i].eventid[ID_AXIS_OFFSET + 0][0]   = port ? INPUTEVENT_JOY2_HORIZ : INPUTEVENT_JOY1_HORIZ;
        uid[i].eventid[ID_AXIS_OFFSET + 1][0]   = port ? INPUTEVENT_JOY2_VERT  : INPUTEVENT_JOY1_VERT;     
        uid[i].eventid[ID_BUTTON_OFFSET + 0][0] = port ? INPUTEVENT_JOY2_FIRE_BUTTON : INPUTEVENT_JOY1_FIRE_BUTTON;
        uid[i].enabled = 1;
    }

}


struct inputdevice_functions inputdevicefunc_joystick = {
    init_joysticks,
    close_joysticks,
    acquire_joystick,
    unacquire_joystick,
    read_joysticks,
    get_joystick_num,
    get_joystick_name,
    get_joystick_widget_num,
    get_joystick_widget_type,
    get_joystick_widget_first
};



int mousehack_allowed (void)
{
    return 1;
}

int needmousehack (void)
{ 
  return 0;
}

static int init_mouse (void)
{
   return 1;
}

static void close_mouse (void)
{
   return;
}

static int acquire_mouse (unsigned int num, int flags)
{
   return 1;
}

static void unacquire_mouse (unsigned int num)
{
   return;
}

static unsigned int get_mouse_num (void)
{
    return 1;
}

static const char *get_mouse_name (unsigned int mouse)
{
    return "Default mouse";
}

static unsigned int get_mouse_widget_num (unsigned int mouse)
{
    return 2 + 3; // 2 axis and 3 buttons
}

static int get_mouse_widget_first (unsigned int mouse, int type)
{
    switch (type) {
	case IDEV_WIDGET_BUTTON:
	    return 2;
	case IDEV_WIDGET_AXIS:
	    return 0;
    }	
    return -1;
}

static int get_mouse_widget_type (unsigned int mouse, unsigned int num, char *name, uae_u32 *code)
{
   if (num >= 2)
	return IDEV_WIDGET_BUTTON;	
    else if (num < 2) 
	return IDEV_WIDGET_AXIS;
    else	    
        return IDEV_WIDGET_NONE;

}

static void read_mouse (void)
{
    /* We handle mouse input in handle_events() */
}


/*
 * Keyboard inputdevice functions
 */
static unsigned int get_kb_num (void)
{
    /* SDL supports only one keyboard */
    return 1;
}

static const char *get_kb_name (unsigned int kb)
{
    return "Default keyboard";
}

static unsigned  int get_kb_widget_num (unsigned int kb)
{
    return 255; // fix me
}

static int get_kb_widget_first (unsigned int kb, int type)
{
    return 0;
}

static int get_kb_widget_type (unsigned int kb, unsigned int num, char *name, uae_u32 *code)
{

    return 0;
}

static int init_kb (void)
{


    return 1;
}

static void close_kb (void)
{
}

static int keyhack (int scancode, int pressed, int num)
{
    return scancode;
}

static void read_kb (void)
{
}

static int acquire_kb (unsigned int num, int flags)
{
    return 1;
}

static void unacquire_kb (unsigned int num)
{
}



struct inputdevice_functions inputdevicefunc_mouse = {
    init_mouse,
    close_mouse,
    acquire_mouse,
    unacquire_mouse,
    read_mouse,
    get_mouse_num,
    get_mouse_name,
    get_mouse_widget_num,
    get_mouse_widget_type,
    get_mouse_widget_first
};

struct inputdevice_functions inputdevicefunc_keyboard =
{
    init_kb,
    close_kb,
    acquire_kb,
    unacquire_kb,
    read_kb,
    get_kb_num,
    get_kb_name,
    get_kb_widget_num,
    get_kb_widget_type,
    get_kb_widget_first
};

void input_get_default_mouse (struct uae_input_device *uid)
{
    /* SDL supports only one mouse */
    uid[0].eventid[ID_AXIS_OFFSET + 0][0]   = INPUTEVENT_MOUSE1_HORIZ;
    uid[0].eventid[ID_AXIS_OFFSET + 1][0]   = INPUTEVENT_MOUSE1_VERT;
    uid[0].eventid[ID_AXIS_OFFSET + 2][0]   = INPUTEVENT_MOUSE1_WHEEL;
    uid[0].eventid[ID_BUTTON_OFFSET + 0][0] = INPUTEVENT_JOY1_FIRE_BUTTON;
    uid[0].eventid[ID_BUTTON_OFFSET + 1][0] = INPUTEVENT_JOY1_2ND_BUTTON;
    uid[0].eventid[ID_BUTTON_OFFSET + 2][0] = INPUTEVENT_JOY1_3RD_BUTTON;
    uid[0].enabled = 1;
}


int getcapslockstate (void)
{
// TODO
//    return capslockstate;
    return 0;
}
void setcapslockstate (int state)
{
// TODO
//    capslockstate = state;
}


void toggle_mousegrab (void)
{

}
