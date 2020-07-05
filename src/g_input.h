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
#ifdef TOUCHINPUTS
	gc_joystick,
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
boolean G_ToggleChaseCam(void);
boolean G_ToggleChaseCam2(void);

// Lactozilla: Touch input buttons
#ifdef TOUCHINPUTS

// Finger structure
#define NUMTOUCHFINGERS 20

typedef boolean (*longpressaction_t) (void *finger);

typedef struct
{
	INT32 x, y;
	float pressure;

	// A finger has either a game control or a key input down.
	union {
		INT32 gamecontrol;
		INT32 keyinput;
	} u;

	// Alternate selections that don't interfere with the above.
	union {
		INT32 selection;
	} extra;

	// What kind of finger is this?
	union {
		boolean menu;
		INT32 mouse;
		INT32 joystick;
	} type;

	tic_t longpress;
	longpressaction_t longpressaction;
	boolean ignoremotion;
} touchfinger_t;
extern touchfinger_t touchfingers[NUMTOUCHFINGERS];
extern UINT8 touchcontroldown[num_gamecontrols];

// Touch screen button structure
typedef struct
{
	fixed_t x, y; // coordinates (normalized)
	fixed_t w, h; // dimensions
	const char *name, *tinyname; // names
	UINT8 color; // color

	boolean dpad; // d-pad button
	tic_t pressed; // touch navigation
	boolean hidden; // doesn't exist
	boolean dontscale; // isn't scaled
} touchconfig_t;

// Screen buttons
extern touchconfig_t touchcontrols[num_gamecontrols]; // Game inputs
extern touchconfig_t touchnavigation[NUMKEYS]; // Menu inputs
extern touchconfig_t *usertouchcontrols;

// Button presets
typedef enum
{
	touchpreset_none,
	touchpreset_normal,
	touchpreset_tiny,
	num_touchpresets,
} touchpreset_e;

// Input variables
extern fixed_t touch_joystick_x, touch_joystick_y, touch_joystick_w, touch_joystick_h;
extern fixed_t touch_gui_scale;

// Touch movement style
typedef enum
{
	tms_joystick = 1,
	tms_dpad,

	num_touchmovementstyles
} touchmovementstyle_e;

// Finger motion type
enum
{
	FINGERMOTION_NONE,
	FINGERMOTION_JOYSTICK,
	FINGERMOTION_MOUSE,
};

// Is the touch screen available?
extern boolean touch_screenexists;

// Touch screen settings
extern touchmovementstyle_e touch_movementstyle;
extern touchpreset_e touch_preset;
extern boolean touch_camera;

// Touch config. status
typedef struct
{
	INT32 vidwidth; // vid.width
	INT32 vidheight; // vid.height
	fixed_t guiscale; // touch_gui_scale

	touchpreset_e preset; // touch_preset
	touchmovementstyle_e movementstyle; // touch_movementstyle

	boolean ringslinger; // G_RingSlingerGametype
	boolean ctfgametype; // gametyperules & GTR_TEAMFLAGS
	boolean canpause;
	boolean canviewpointswitch; // G_CanViewpointSwitch()
	boolean cantalk; // netgame && !CHAT_MUTE
	boolean canteamtalk; // G_GametypeHasTeams() && players[consoleplayer].ctfteam
	boolean promptblockcontrols; // promptblockcontrols
	boolean canopenconsole; // modeattacking || metalrecording
	boolean customizingcontrols; // TS_IsCustomizingControls
} touchconfigstatus_t;

extern touchconfigstatus_t touchcontrolstatus;
extern touchconfigstatus_t touchnavigationstatus;

// Console variables for the touch screen
extern consvar_t cv_touchstyle;
extern consvar_t cv_touchpreset;
extern consvar_t cv_touchlayout;
extern consvar_t cv_touchcamera;
extern consvar_t cv_touchtrans, cv_touchmenutrans;
extern consvar_t cv_touchguiscale;

// Touch layout options
extern consvar_t cv_touchlayoutusegrid;

// Touch screen sensitivity
extern consvar_t cv_touchsens, cv_touchvertsens;
extern consvar_t cv_touchjoyhorzsens, cv_touchjoyvertsens;

// Screen joystick movement
#define TOUCHJOYEXTENDX (touch_joystick_w / 2)
#define TOUCHJOYEXTENDY (touch_joystick_h / 2)
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

const char *gamecontrolname[num_gamecontrols];

void G_DefineDefaultControls(void);

#ifdef TOUCHINPUTS
void G_SetupTouchSettings(void);
void G_UpdateTouchControls(void);
void G_DefineTouchButtons(void);

void G_PositionTouchButtons(void);
void G_PositionTouchNavigation(void);
void G_PositionExtraUserTouchButtons(void);

void G_TouchPresetChanged(void);
void G_BuildTouchPreset(touchconfig_t *controls, touchconfigstatus_t *status, touchmovementstyle_e tms, fixed_t scale, boolean tiny);

// Returns the names of a touch button
const char *G_GetTouchButtonName(INT32 gc);
const char *G_GetTouchButtonShortName(INT32 gc);

// Sets all button names for a touch config
void G_SetTouchButtonNames(touchconfig_t *controls);

// Returns true if a touch preset is active
boolean G_TouchPresetActive(void);

// Updates touch fingers
void G_UpdateFingers(INT32 realtics);

// Checks if the finger (x, y) is touching the specified button (btn)
boolean G_FingerTouchesButton(INT32 x, INT32 y, touchconfig_t *btn);

// Checks if the finger (x, y) is touching the specified navigation button (btn)
boolean G_FingerTouchesNavigationButton(INT32 x, INT32 y, touchconfig_t *btn);

// Checks if the finger is touching the joystick area.
boolean G_FingerTouchesJoystickArea(INT32 x, INT32 y);

// Checks if the game control is a player control key
boolean G_TouchButtonIsPlayerControl(INT32 gamecontrol);

// Scales a touch button
void G_ScaleTouchCoords(INT32 *x, INT32 *y, INT32 *w, INT32 *h, boolean normalized, boolean screenscale);

// Normalizes a touch button
void G_NormalizeTouchButton(touchconfig_t *button);

// Normalizes a touch config
void G_NormalizeTouchConfig(touchconfig_t *config, int configsize);

// Denormalizes XY coordinates
void G_DenormalizeCoords(fixed_t *x, fixed_t *y);

// Centers coordinates
void G_CenterCoords(fixed_t *x, fixed_t *y);

// Centers integer coordinates
void G_CenterIntegerCoords(INT32 *x, INT32 *y);

// Defines a d-pad
void G_DPadPreset(touchconfig_t *controls, fixed_t xscale, fixed_t yscale, fixed_t dw, boolean tiny);
#endif

INT32 G_GetControlScheme(INT32 (*fromcontrols)[2], const INT32 *gclist, INT32 gclen);
void G_CopyControls(INT32 (*setupcontrols)[2], INT32 (*fromcontrols)[2], const INT32 *gclist, INT32 gclen);
void G_SaveKeySetting(FILE *f, INT32 (*fromcontrols)[2], INT32 (*fromcontrolsbis)[2]);
INT32 G_CheckDoubleUsage(INT32 keynum, boolean modify);

#endif
