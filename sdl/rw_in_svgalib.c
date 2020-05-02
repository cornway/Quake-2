#include <stdarg.h>

#include "../ref_soft/r_local.h"
#include "../client/keys.h"
#include "../sdl/rw_sdl.h"

/*****************************************************************************/
/* KEYBOARD                                                                  */
/*****************************************************************************/

static unsigned char scantokey[128];
Key_Event_fp_t Key_Event_fp;

const kbdmap_t gamepad_to_kbd_map[JOY_STD_MAX] =
{
    [JOY_UPARROW]       = {K_UPARROW, 0},
    [JOY_DOWNARROW]     = {K_DOWNARROW, 0},
    [JOY_LEFTARROW]     = {K_LEFTARROW,0},
    [JOY_RIGHTARROW]    = {K_RIGHTARROW, 0},
    [JOY_K1]            = {'/', PAD_FREQ_LOW},
    [JOY_K4]            = {K_END,  0},
    [JOY_K3]            = {K_CTRL, 0},
    [JOY_K2]            = {K_SPACE,    0},
    [JOY_K5]            = {'a',    0},
    [JOY_K6]            = {'d',    0},
    [JOY_K7]            = {K_DEL,  0},
    [JOY_K8]            = {K_PGDN, 0},
    [JOY_K9]            = {K_ENTER, 0},
    [JOY_K10]           = {K_ESCAPE, PAD_FREQ_LOW},
};

static void keyhandler(int scancode, int state)
{
	int sc;

	sc = scancode & 0x7f;
//ri.Con_Printf(PRINT_ALL, "scancode=%x (%d%s)\n", scancode, sc, scancode&0x80?"+128":"");
	Key_Event_fp(scantokey[sc], state == keydown);
}

static i_event_t *__post_key (i_event_t *events, i_event_t *event)
{
    keyhandler(event->sym, event->state);
    return NULL;
}

void KBD_Init(Key_Event_fp_t fp)
{
	Key_Event_fp = fp;

    input_soft_init(__post_key, gamepad_to_kbd_map);
}

void KBD_Update(void)
{
	input_proc_keys(NULL);
}

void KBD_Close(void)
{
	input_bsp_deinit();
}

/*****************************************************************************/
/* MOUSE                                                                     */
/*****************************************************************************/

// this is inside the renderer shared lib, so these are called from vid_so

static qboolean	UseMouse = false;

static cvar_t	*m_filter;
static cvar_t	*in_mouse;

static cvar_t	*mdev;
static cvar_t	*mrate;

static qboolean	mlooking;

// state struct passed in Init
static in_state_t	*in_state;

static cvar_t *sensitivity;
static cvar_t *lookstrafe;
static cvar_t *m_side;
static cvar_t *m_yaw;
static cvar_t *m_pitch;
static cvar_t *m_forward;
static cvar_t *freelook;

static void Force_CenterView_f (void)
{
	in_state->viewangles[PITCH] = 0;
}

static void RW_IN_MLookDown (void) 
{ 
	mlooking = true; 
}

static void RW_IN_MLookUp (void) 
{
	mlooking = false;
	in_state->IN_CenterView_fp ();
}

static void mousehandler(int buttonstate, int dx, int dy)
{
}

void RW_IN_Init(in_state_t *in_state_p)
{
	int mtype;
	int i;

	in_state = in_state_p;

	// mouse variables
	m_filter = ri.Cvar_Get ("m_filter", "0", 0);
    in_mouse = ri.Cvar_Get ("in_mouse", "1", CVAR_ARCHIVE);
	freelook = ri.Cvar_Get( "freelook", "0", 0 );
	lookstrafe = ri.Cvar_Get ("lookstrafe", "0", 0);
	sensitivity = ri.Cvar_Get ("sensitivity", "3", 0);
	m_pitch = ri.Cvar_Get ("m_pitch", "0.022", 0);
	m_yaw = ri.Cvar_Get ("m_yaw", "0.022", 0);
	m_forward = ri.Cvar_Get ("m_forward", "1", 0);
	m_side = ri.Cvar_Get ("m_side", "0.8", 0);

	ri.Cmd_AddCommand ("+mlook", RW_IN_MLookDown);
	ri.Cmd_AddCommand ("-mlook", RW_IN_MLookUp);

	ri.Cmd_AddCommand ("force_centerview", Force_CenterView_f);
}

void RW_IN_Shutdown(void)
{
}

/*
===========
IN_Commands
===========
*/
void RW_IN_Commands (void)
{
}

/*
===========
IN_Move
===========
*/
void RW_IN_Move (usercmd_t *cmd)
{
}

void RW_IN_Frame (void)
{
}

void RW_IN_Activate(void)
{
}



