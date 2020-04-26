// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  g_input.h
/// \brief handle mouse/keyboard/joystick inputs,
///        maps inputs to game controls (forward, spin, jump...)

#ifndef __G_INPUT__
#define __G_INPUT__

#include "d_event.h"
#include "d_player.h"
#include "keys.h"
#include "command.h"

// number of total 'button' inputs, include keyboard keys, plus virtual
// keys (mousebuttons and joybuttons becomes keys)
#define NUMKEYS 256

#define MOUSEBUTTONS 8
#define JOYBUTTONS   32 // 32 buttons
#define JOYHATS      4  // 4 hats
#define JOYAXISSET   4  // 4 Sets of 2 axises

//
// mouse and joystick buttons are handled as 'virtual' keys
//
typedef enum
{
	KEY_MOUSE1 = NUMKEYS,
	KEY_JOY1 = KEY_MOUSE1 + MOUSEBUTTONS,
	KEY_HAT1 = KEY_JOY1 + JOYBUTTONS,

	KEY_DBLMOUSE1 =KEY_HAT1 + JOYHATS*4, // double clicks
	KEY_DBLJOY1 = KEY_DBLMOUSE1 + MOUSEBUTTONS,
	KEY_DBLHAT1 = KEY_DBLJOY1 + JOYBUTTONS,

	KEY_2MOUSE1 = KEY_DBLHAT1 + JOYHATS*4,
	KEY_2JOY1 = KEY_2MOUSE1 + MOUSEBUTTONS,
	KEY_2HAT1 = KEY_2JOY1 + JOYBUTTONS,

	KEY_DBL2MOUSE1 = KEY_2HAT1 + JOYHATS*4,
	KEY_DBL2JOY1 = KEY_DBL2MOUSE1 + MOUSEBUTTONS,
	KEY_DBL2HAT1 = KEY_DBL2JOY1 + JOYBUTTONS,

	KEY_MOUSEWHEELUP = KEY_DBL2HAT1 + JOYHATS*4,
	KEY_MOUSEWHEELDOWN = KEY_MOUSEWHEELUP + 1,
	KEY_2MOUSEWHEELUP = KEY_MOUSEWHEELDOWN + 1,
	KEY_2MOUSEWHEELDOWN = KEY_2MOUSEWHEELUP + 1,

	NUMINPUTS = KEY_2MOUSEWHEELDOWN + 1,
} key_input_e;

typedef enum
{
	gc_null = 0, // a key/button mapped to gc_null has no effect
	gc_forward,
	gc_backward,
	gc_strafeleft,
	gc_straferight,
	gc_turnleft,
	gc_turnright,
	gc_weaponnext,
	gc_weaponprev,
	gc_wepslot1,
	gc_wepslot2,
	gc_wepslot3,
	gc_wepslot4,
	gc_wepslot5,
	gc_wepslot6,
	gc_wepslot7,
	gc_wepslot8,
	gc_wepslot9,
	gc_wepslot10,
	gc_fire,
	gc_firenormal,
	gc_tossflag,
	gc_use,
	gc_camtoggle,
	gc_camreset,
	gc_lookup,
	gc_lookdown,
	gc_centerview,
	gc_mouseaiming, // mouse aiming is momentary (toggleable in the menu)
	gc_talkkey,
	gc_teamkey,
	gc_scores,
	gc_jump,
	gc_console,
	gc_pause,
	gc_systemmenu,
	gc_screenshot,
	gc_recordgif,
	gc_viewpoint,
	gc_custom1, // Lua scriptable
	gc_custom2, // Lua scriptable
	gc_custom3, // Lua scriptable
	num_gamecontrols
} gamecontrols_e;

typedef enum
{
	gcs_custom,
	gcs_fps,
	gcs_platform,
	num_gamecontrolschemes
} gamecontrolschemes_e;

// mouse values are used once
extern consvar_t cv_mousesens, cv_mouseysens;
extern consvar_t cv_mousesens2, cv_mouseysens2;
extern consvar_t cv_controlperkey;

extern INT32 mousex, mousey;
extern INT32 mlooky; //mousey with mlookSensitivity
extern INT32 mouse2x, mouse2y, mlook2y;

extern INT32 joyxmove[JOYAXISSET], joyymove[JOYAXISSET], joy2xmove[JOYAXISSET], joy2ymove[JOYAXISSET];

typedef enum
{
	AXISNONE = 0,
	AXISTURN,
	AXISMOVE,
	AXISLOOK,
	AXISSTRAFE,

	AXISDIGITAL, // axes below this use digital deadzone

	AXISJUMP,
	AXISSPIN,
	AXISFIRE,
	AXISFIRENORMAL,
} axis_input_e;

typedef struct joystickvector2_s
{
	INT32 xaxis;
	INT32 yaxis;
} joystickvector2_t;
extern joystickvector2_t movejoystickvectors[2], lookjoystickvectors[2];

// current state of the keys: true if pushed
extern UINT8 gamekeydown[NUMINPUTS];

boolean G_InGameInput(void);

void G_ResetInputs(void);
void G_ResetJoysticks(void);
void G_ResetMice(void);

boolean G_HandlePauseKey(boolean ispausebreak);

boolean G_DoViewpointSwitch(void);
boolean G_CanViewpointSwitch(void);
boolean G_CanViewpointSwitchToPlayer(player_t *player);

// Lactozilla: Touch input buttons
#ifdef TOUCHINPUTS

// Finger structure
#define NUMTOUCHFINGERS 20
typedef struct
{
	INT32 x, y;
	float pressure;
	union {
		INT32 gamecontrol;
		INT32 keyinput;
	} u;
	union {
		boolean menu;
		INT32 mouse;
		INT32 joystick;
	} type;
	boolean ignoremotion;
} touchfinger_t;
extern touchfinger_t touchfingers[NUMTOUCHFINGERS];
extern UINT8 touchcontroldown[num_gamecontrols];

// Touch screen button structure
typedef struct
{
	INT32 x, y;
	INT32 w, h;
	tic_t pressed; // touch navigation
	boolean dpad; // d-pad key
	boolean hidden; // hidden key?
	const char *name; // key name
} touchconfig_t;

// Screen buttons
extern touchconfig_t touchcontrols[num_gamecontrols]; // Game inputs
extern touchconfig_t touchnavigation[NUMKEYS]; // Menu inputs

// Input variables
extern INT32 touch_dpad_x, touch_dpad_y, touch_dpad_w, touch_dpad_h;

// Touch movement style
typedef enum
{
	tms_dpad,
	tms_joystick,
	num_touchmovementstyles
} touchmovementstyle_e;

// Finger motion type
enum
{
	FINGERMOTION_JOYSTICK = 1,
	FINGERMOTION_MOUSE = 2,
};

// Is the touch screen available?
extern boolean touch_screenexists;

// Touch screen settings
extern touchmovementstyle_e touch_movementstyle;
extern boolean touch_tinycontrols;
extern boolean touch_camera;

// Console variables for the touch screen
extern consvar_t cv_touchstyle;
extern consvar_t cv_touchtiny;
extern consvar_t cv_touchcamera;
extern consvar_t cv_touchtrans, cv_touchmenutrans;

// Touch screen sensitivity
extern consvar_t cv_touchsens, cv_touchysens;

// Screen joystick movement
#define TOUCHJOYEXTENDX (touch_dpad_w / 2)
#define TOUCHJOYEXTENDY (touch_dpad_h / 2)
extern float touchxmove, touchymove, touchpressure;
#endif

// two key codes (or virtual key) per game control
extern INT32 gamecontrol[num_gamecontrols][2];
extern INT32 gamecontrolbis[num_gamecontrols][2]; // secondary splitscreen player
extern INT32 gamecontroldefault[num_gamecontrolschemes][num_gamecontrols][2]; // default control storage, use 0 (gcs_custom) for memory retention
extern INT32 gamecontrolbisdefault[num_gamecontrolschemes][num_gamecontrols][2];

// Checks if any game control key is down
#define GAMECONTROLDOWN(keyref, gc) (gamekeydown[keyref[gc][0]] || gamekeydown[keyref[gc][1]])
#define GC1KEYDOWN(gc) GAMECONTROLDOWN(gamecontrol, gc)
#define GC2KEYDOWN(gc) GAMECONTROLDOWN(gamecontrolbis, gc)

// Player 1 input
#ifdef TOUCHINPUTS
	#define PLAYER1INPUTDOWN(gc) (GC1KEYDOWN(gc) || touchcontroldown[gc])
#else
	#define PLAYER1INPUTDOWN(gc) GC1KEYDOWN(gc)
#endif

// Player 2 input
#define PLAYER2INPUTDOWN(gc) GAMECONTROLDOWN(gamecontrolbis, gc)

// Check inputs for the specified player
#define PLAYERINPUTDOWN(p, gc) ((p) == 2 ? PLAYER2INPUTDOWN(gc) : PLAYER1INPUTDOWN(gc))

#define num_gcl_tutorial_check 6
#define num_gcl_tutorial_used 8
#define num_gcl_tutorial_full 13
#define num_gcl_movement 4
#define num_gcl_camera 2
#define num_gcl_movement_camera 6
#define num_gcl_jump 1
#define num_gcl_use 1
#define num_gcl_jump_use 2

extern const INT32 gcl_tutorial_check[num_gcl_tutorial_check];
extern const INT32 gcl_tutorial_used[num_gcl_tutorial_used];
extern const INT32 gcl_tutorial_full[num_gcl_tutorial_full];
extern const INT32 gcl_movement[num_gcl_movement];
extern const INT32 gcl_camera[num_gcl_camera];
extern const INT32 gcl_movement_camera[num_gcl_movement_camera];
extern const INT32 gcl_jump[num_gcl_jump];
extern const INT32 gcl_use[num_gcl_use];
extern const INT32 gcl_jump_use[num_gcl_jump_use];

// peace to my little coder fingers!
// check a gamecontrol being active or not

// remaps the input event to a game control.
void G_MapEventsToControls(event_t *ev);

// returns the name of a key
const char *G_KeynumToString(INT32 keynum);
INT32 G_KeyStringtoNum(const char *keystr);

// detach any keys associated to the given game control
void G_ClearControlKeys(INT32 (*setupcontrols)[2], INT32 control);
void G_ClearAllControlKeys(void);
void Command_Setcontrol_f(void);
void Command_Setcontrol2_f(void);

void G_DefineDefaultControls(void);

#ifdef TOUCHINPUTS
// Define/update touch controls
void G_SetupTouchSettings(void);
void G_UpdateTouchControls(void);
void G_DefineTouchButtons(void);

// Button presets
void G_TouchControlPreset(void);
void G_TouchNavigationPreset(void);

// Check if the finger (x, y) is touching the specified button (butt)
boolean G_FingerTouchesButton(INT32 x, INT32 y, touchconfig_t *butt);

// Check if the gamecontrol is a player control key
boolean G_TouchButtonIsPlayerControl(INT32 gamecontrol);

// Scale a d-pad
void G_ScaleDPadCoords(INT32 *x, INT32 *y, INT32 *w, INT32 *h);
#endif

INT32 G_GetControlScheme(INT32 (*fromcontrols)[2], const INT32 *gclist, INT32 gclen);
void G_CopyControls(INT32 (*setupcontrols)[2], INT32 (*fromcontrols)[2], const INT32 *gclist, INT32 gclen);
void G_SaveKeySetting(FILE *f, INT32 (*fromcontrols)[2], INT32 (*fromcontrolsbis)[2]);
INT32 G_CheckDoubleUsage(INT32 keynum, boolean modify);

#endif
