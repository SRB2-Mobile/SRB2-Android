// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2022 by Sonic Team Junior.
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

// Current input method
typedef enum
{
	INPUTMETHOD_NONE,
	INPUTMETHOD_KEYBOARD,
	INPUTMETHOD_MOUSE,
	INPUTMETHOD_JOYSTICK,
	INPUTMETHOD_TOUCH,
	INPUTMETHOD_TVREMOTE
} inputmethod_e;

extern INT32 inputmethod, controlmethod;

#if defined(__ANDROID__)
#define ACCELEROMETER
#define ACCELEROMETER_MAX_TILT_OFFSET (90*FRACUNIT)
#define ANDROID_ACCELEROMETER_DEVICE "Android Accelerometer"
#endif

//
// Mouse, joystick, and TV remote buttons are handled as 'virtual' keys
//

#define NUMJOYHATS (JOYHATS*4)

typedef enum
{
	KEY_MOUSE1 = NUMKEYS,
	KEY_JOY1 = KEY_MOUSE1 + MOUSEBUTTONS,
	KEY_HAT1 = KEY_JOY1 + JOYBUTTONS,

	KEY_DBLMOUSE1 = KEY_HAT1 + NUMJOYHATS, // double clicks
	KEY_DBLJOY1 = KEY_DBLMOUSE1 + MOUSEBUTTONS,
	KEY_DBLHAT1 = KEY_DBLJOY1 + JOYBUTTONS,

	KEY_2MOUSE1 = KEY_DBLHAT1 + NUMJOYHATS,
	KEY_2JOY1 = KEY_2MOUSE1 + MOUSEBUTTONS,
	KEY_2HAT1 = KEY_2JOY1 + JOYBUTTONS,

	KEY_DBL2MOUSE1 = KEY_2HAT1 + NUMJOYHATS,
	KEY_DBL2JOY1 = KEY_DBL2MOUSE1 + MOUSEBUTTONS,
	KEY_DBL2HAT1 = KEY_DBL2JOY1 + JOYBUTTONS,

	KEY_MOUSEWHEELUP = KEY_DBL2HAT1 + NUMJOYHATS,
	KEY_MOUSEWHEELDOWN = KEY_MOUSEWHEELUP + 1,
	KEY_2MOUSEWHEELUP = KEY_MOUSEWHEELDOWN + 1,
	KEY_2MOUSEWHEELDOWN = KEY_2MOUSEWHEELUP + 1,

	KEY_REMOTEUP = KEY_2MOUSEWHEELDOWN + 1,
	KEY_REMOTEDOWN,
	KEY_REMOTELEFT,
	KEY_REMOTERIGHT,
	KEY_REMOTECENTER,
	KEY_REMOTEBACK,

	NUMINPUTS = KEY_REMOTEBACK + 1,
} key_input_e;

// Macros for checking if a virtual key is a mouse key or wheel.
#define G_KeyIsMouse1(key) ((key) >= KEY_MOUSE1 && (key) < KEY_MOUSE1 + MOUSEBUTTONS)
#define G_KeyIsMouse2(key) ((key) >= KEY_2MOUSE1 && (key) < KEY_2MOUSE1 + MOUSEBUTTONS)
#define G_KeyIsAnyMouse(key) (G_KeyIsMouse1(key) || G_KeyIsMouse2(key))

#define G_KeyIsMouseDoubleClick1(key) ((key) >= KEY_DBLMOUSE1 && (key) < KEY_DBLMOUSE1 + MOUSEBUTTONS)
#define G_KeyIsMouseDoubleClick2(key) ((key) >= KEY_DBL2MOUSE1 && (key) < KEY_DBL2MOUSE1 + MOUSEBUTTONS)
#define G_KeyIsAnyMouseDoubleClick(key) (G_KeyIsMouseDoubleClick1(key) || G_KeyIsMouseDoubleClick2(key))

#define G_KeyIsMouseWheel1(key) ((key) == KEY_MOUSEWHEELUP || (key) == KEY_MOUSEWHEELDOWN)
#define G_KeyIsMouseWheel2(key) ((key) == KEY_2MOUSEWHEELUP || (key) == KEY_2MOUSEWHEELDOWN)
#define G_KeyIsAnyMouseWheel(key) (G_KeyIsMouseWheel1(key) || G_KeyIsMouseWheel2(key))

#define G_KeyIsMouse(key) (G_KeyIsAnyMouse(key) || G_KeyIsAnyMouseDoubleClick(key) || G_KeyIsAnyMouseWheel(key))
#define G_KeyIsPlayer1Mouse(key) (G_KeyIsMouse1(key) || G_KeyIsMouseDoubleClick1(key) || G_KeyIsMouseWheel1(key))
#define G_KeyIsPlayer2Mouse(key) (G_KeyIsMouse2(key) || G_KeyIsMouseDoubleClick2(key) || G_KeyIsMouseWheel2(key))

// Macros for checking if a virtual key is a joystick button or hat.
#define G_KeyIsJoystick1(key) ((key) >= KEY_JOY1 && (key) < KEY_JOY1 + JOYBUTTONS)
#define G_KeyIsJoystick2(key) ((key) >= KEY_2JOY1 && (key) < KEY_2JOY1 + JOYBUTTONS)
#define G_KeyIsAnyJoystickButton(key) (G_KeyIsJoystick1(key) || G_KeyIsJoystick2(key))

#define G_KeyIsJoystickHat1(key) ((key) >= KEY_HAT1 && (key) < KEY_HAT1 + NUMJOYHATS)
#define G_KeyIsJoystickHat2(key) ((key) >= KEY_2HAT1 && (key) < KEY_2HAT1 + NUMJOYHATS)
#define G_KeyIsAnyJoystickHat(key) (G_KeyIsJoystickHat1(key) || G_KeyIsJoystickHat2(key))

#define G_KeyIsJoystick(key) (G_KeyIsAnyJoystickButton(key) || G_KeyIsAnyJoystickHat(key))
#define G_KeyIsPlayer1Joystick(key) (G_KeyIsJoystick1(key) || G_KeyIsJoystickHat1(key))
#define G_KeyIsPlayer2Joystick(key) (G_KeyIsJoystick2(key) || G_KeyIsJoystickHat2(key))

// Macro for checking if a virtual key is a TV remote key.
#define G_KeyIsTVRemote(key) ((key) >= KEY_REMOTEUP && (key) <= KEY_REMOTEBACK)

// Check if a given key is a virtual key.
#define G_KeyIsVirtual(key) (G_KeyIsMouse(key) || G_KeyIsJoystick(key) || G_KeyIsTVRemote(key))

// Checks if a given virtual key is for the first player.
#define G_VirtualKeyIsPlayer1(key) (G_KeyIsVirtual(key) && (G_KeyIsPlayer1Mouse(key) || G_KeyIsPlayer1Joystick(key)))

// Checks if a given virtual key is for the second player.
#define G_VirtualKeyIsPlayer2(key) (G_KeyIsVirtual(key) && (G_KeyIsPlayer2Mouse(key) || G_KeyIsPlayer2Joystick(key)))

// Checks if a given event type is a touch screen event.
#define G_EventIsTouch(ev) ((ev) == ev_touchdown || (ev) == ev_touchmotion || (ev) == ev_touchup)

typedef enum
{
	GC_NULL = 0, // a key/button mapped to GC_NULL has no effect
	GC_FORWARD,
	GC_BACKWARD,
	GC_STRAFELEFT,
	GC_STRAFERIGHT,
	GC_TURNLEFT,
	GC_TURNRIGHT,
#ifdef TOUCHINPUTS
	GC_JOYSTICK,
	GC_DPADUL,
	GC_DPADUR,
	GC_DPADDL,
	GC_DPADDR,
#endif
	GC_WEAPONNEXT,
	GC_WEAPONPREV,
	GC_WEPSLOT1,
	GC_WEPSLOT2,
	GC_WEPSLOT3,
	GC_WEPSLOT4,
	GC_WEPSLOT5,
	GC_WEPSLOT6,
	GC_WEPSLOT7,
	GC_WEPSLOT8,
	GC_WEPSLOT9,
	GC_WEPSLOT10,
	GC_FIRE,
	GC_FIRENORMAL,
	GC_TOSSFLAG,
	GC_SPIN,
	GC_CAMTOGGLE,
	GC_CAMRESET,
	GC_LOOKUP,
	GC_LOOKDOWN,
	GC_CENTERVIEW,
	GC_MOUSEAIMING, // mouse aiming is momentary (toggleable in the menu)
	GC_TALKKEY,
	GC_TEAMKEY,
	GC_SCORES,
	GC_JUMP,
	GC_CONSOLE,
	GC_PAUSE,
	GC_SYSTEMMENU,
	GC_SCREENSHOT,
	GC_RECORDGIF,
	GC_VIEWPOINTNEXT,
	GC_VIEWPOINTPREV,
	GC_CUSTOM1, // Lua scriptable
	GC_CUSTOM2, // Lua scriptable
	GC_CUSTOM3, // Lua scriptable
	NUM_GAMECONTROLS
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

typedef struct
{
	INT32 dx; // deltas with mousemove sensitivity
	INT32 dy;
	INT32 mlookdy; // dy with mouselook sensitivity
	INT32 rdx; // deltas without sensitivity
	INT32 rdy;
	UINT16 buttons;
} mouse_t;

#define MB_BUTTON1    0x0001
#define MB_BUTTON2    0x0002
#define MB_BUTTON3    0x0004
#define MB_BUTTON4    0x0008
#define MB_BUTTON5    0x0010
#define MB_BUTTON6    0x0020
#define MB_BUTTON7    0x0040
#define MB_BUTTON8    0x0080
#define MB_SCROLLUP   0x0100
#define MB_SCROLLDOWN 0x0200

extern mouse_t mouse;
extern mouse_t mouse2;

extern INT32 joyxmove[JOYAXISSET], joyymove[JOYAXISSET], joy2xmove[JOYAXISSET], joy2ymove[JOYAXISSET];

#ifdef ACCELEROMETER
extern INT32 accelxmove, accelymove, acceltilt;
#endif

extern CV_PossibleValue_t zerotoone_cons_t[];

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

#ifdef TOUCHINPUTS
extern joystickvector2_t touchmovevector;
#endif

#ifdef ACCELEROMETER
extern joystickvector2_t accelmovevector;
#endif

// current state of the keys: true if pushed
extern UINT8 gamekeydown[NUMINPUTS];

boolean G_InGameInput(void);
INT32 G_InputMethodFromKey(INT32 key);

void G_DetectInputMethod(INT32 key); // UI
void G_DetectControlMethod(INT32 key); // Game

void G_ResetInputs(void);
void G_ResetJoysticks(void);
void G_ResetAccelerometer(void);
void G_ResetMice(void);

boolean G_HandlePauseKey(boolean ispausebreak);
boolean G_CanRetryModeAttack(void);
boolean G_DoViewpointSwitch(INT32 direction);
boolean G_ToggleChaseCam(void);
boolean G_ToggleChaseCam2(void);

boolean G_CanUseAccelerometer(void);

// two key codes (or virtual key) per game control
extern INT32 gamecontrol[NUM_GAMECONTROLS][2];
extern INT32 gamecontrolbis[NUM_GAMECONTROLS][2]; // secondary splitscreen player
extern INT32 gamecontroldefault[num_gamecontrolschemes][NUM_GAMECONTROLS][2]; // default control storage, use 0 (gcs_custom) for memory retention
extern INT32 gamecontrolbisdefault[num_gamecontrolschemes][NUM_GAMECONTROLS][2];

// Game control names
extern const char *gamecontrolname[NUM_GAMECONTROLS];

// Is there a touch screen in the device?
// (Moved from ts_main.h)
extern boolean touchscreenavailable;

#ifdef TOUCHINPUTS
extern UINT8 touchcontroldown[NUM_GAMECONTROLS];
#endif

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
#define PLAYER2INPUTDOWN(gc) GC2KEYDOWN(gc)

// Check inputs for the specified player
#define PLAYERINPUTDOWN(p, gc) ((p) == 2 ? PLAYER2INPUTDOWN(gc) : PLAYER1INPUTDOWN(gc))

#define num_gcl_tutorial_check 6
#define num_gcl_tutorial_used 8
#define num_gcl_tutorial_full 13
#define num_gcl_movement 4
#define num_gcl_camera 2
#define num_gcl_movement_camera 6
#define num_gcl_jump 1
#define num_gcl_spin 1
#define num_gcl_jump_spin 2

extern const INT32 gcl_tutorial_check[num_gcl_tutorial_check];
extern const INT32 gcl_tutorial_used[num_gcl_tutorial_used];
extern const INT32 gcl_tutorial_full[num_gcl_tutorial_full];
extern const INT32 gcl_movement[num_gcl_movement];
extern const INT32 gcl_camera[num_gcl_camera];
extern const INT32 gcl_movement_camera[num_gcl_movement_camera];
extern const INT32 gcl_jump[num_gcl_jump];
extern const INT32 gcl_spin[num_gcl_spin];
extern const INT32 gcl_jump_spin[num_gcl_jump_spin];

// peace to my little coder fingers!
// check a gamecontrol being active or not

// remaps the input event to a game control.
void G_MapEventsToControls(event_t *ev);

// returns true if a key is assigned to a game control
boolean G_KeyAssignedToControl(INT32 key);

// returns the name of a key
const char *G_KeyNumToName(INT32 keynum);
INT32 G_KeyNameToNum(const char *keystr);

// detach any keys associated to the given game control
void G_ClearControlKeys(INT32 (*setupcontrols)[2], INT32 control);
void G_ClearAllControlKeys(void);

void Command_Setcontrol_f(void);
void Command_Setcontrol2_f(void);

void G_DefineDefaultControls(void);

INT32 G_GetControlScheme(INT32 (*fromcontrols)[2], const INT32 *gclist, INT32 gclen);
void G_CopyControls(INT32 (*setupcontrols)[2], INT32 (*fromcontrols)[2], const INT32 *gclist, INT32 gclen);
void G_SaveKeySetting(FILE *f, INT32 (*fromcontrols)[2], INT32 (*fromcontrolsbis)[2]);
INT32 G_CheckDoubleUsage(INT32 keynum, boolean modify);

// sets the members of a mouse_t given position deltas
void G_SetMouseDeltas(INT32 dx, INT32 dy, UINT8 ssplayer);

#endif
