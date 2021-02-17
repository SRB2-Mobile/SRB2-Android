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

// Current input method
typedef enum
{
	INPUTMETHOD_NONE,
	INPUTMETHOD_KEYBOARD,
	INPUTMETHOD_MOUSE,
	INPUTMETHOD_JOYSTICK,
	INPUTMETHOD_TOUCH
} inputmethod_e;

extern INT32 inputmethod, controlmethod;

//
// Mouse and joystick buttons are handled as 'virtual' keys
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

	NUMINPUTS = KEY_2MOUSEWHEELDOWN + 1,
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

// Check if a given key is a virtual key.
#define G_KeyIsVirtual(key) (G_KeyIsMouse(key) || G_KeyIsJoystick(key))

// Checks if a given virtual key is for the first player.
#define G_VirtualKeyIsPlayer1(key) (G_KeyIsVirtual(key) && (G_KeyIsPlayer1Mouse(key) || G_KeyIsPlayer1Joystick(key)))

// Checks if a given virtual key is for the second player.
#define G_VirtualKeyIsPlayer2(key) (G_KeyIsVirtual(key) && (G_KeyIsPlayer2Mouse(key) || G_KeyIsPlayer2Joystick(key)))

// Checks if a given event type is a touch screen event.
#define G_EventIsTouch(ev) ((ev) == ev_touchdown || (ev) == ev_touchmotion || (ev) == ev_touchup)

typedef enum
{
	gc_null = 0, // a key/button mapped to gc_null has no effect
	gc_forward,
	gc_backward,
	gc_strafeleft,
	gc_straferight,
	gc_turnleft,
	gc_turnright,
#ifdef TOUCHINPUTS
	gc_joystick,
	gc_dpadul,
	gc_dpadur,
	gc_dpaddl,
	gc_dpaddr,
#endif
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
	gc_spin,
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
extern joystickvector2_t movejoystickvectors[2], lookjoystickvectors[2];

// current state of the keys: true if pushed
extern UINT8 gamekeydown[NUMINPUTS];

boolean G_InGameInput(void);
INT32 G_InputMethodFromKey(INT32 key);

void G_DetectInputMethod(INT32 key); // UI
void G_DetectControlMethod(INT32 key); // Game

void G_ResetInputs(void);
void G_ResetJoysticks(void);
void G_ResetMice(void);

boolean G_HandlePauseKey(boolean ispausebreak);
boolean G_CanRetryModeAttack(void);
boolean G_DoViewpointSwitch(void);
boolean G_ToggleChaseCam(void);
boolean G_ToggleChaseCam2(void);

// two key codes (or virtual key) per game control
extern INT32 gamecontrol[num_gamecontrols][2];
extern INT32 gamecontrolbis[num_gamecontrols][2]; // secondary splitscreen player
extern INT32 gamecontroldefault[num_gamecontrolschemes][num_gamecontrols][2]; // default control storage, use 0 (gcs_custom) for memory retention
extern INT32 gamecontrolbisdefault[num_gamecontrolschemes][num_gamecontrols][2];

// Game control names
extern const char *gamecontrolname[num_gamecontrols];

// Is there a touch screen in the device?
// (Moved from ts_main.h)
extern boolean touchscreenavailable;

#ifdef TOUCHINPUTS
extern UINT8 touchcontroldown[num_gamecontrols];
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
const char *G_KeynumToString(INT32 keynum);
INT32 G_KeyStringtoNum(const char *keystr);

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

#endif
