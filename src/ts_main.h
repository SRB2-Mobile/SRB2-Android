// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2022 by Jaime Ita Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ts_main.h
/// \brief Touch screen

#ifndef __TS_MAIN_H__
#define __TS_MAIN_H__

#include "doomdata.h"
#include "doomtype.h"
#include "doomdef.h"

#include "d_event.h"
#include "g_input.h"

#ifdef TOUCHINPUTS
extern boolean ts_ready;

boolean TS_Ready(void);

// Finger structure
#define NUMTOUCHFINGERS 10

typedef boolean (*longpressaction_t) (void *finger);

typedef struct
{
	INT32 x, y;
	INT32 dx, dy;
	float fx, fy;
	float fdx, fdy;
	float pressure;
	boolean down;

	tic_t longpress;
	longpressaction_t longpressaction;

	boolean extrahandling, navinput;
	boolean ignoremotion;

	// A finger has either a game control or a key input down.
	union {
		INT32 gamecontrol;
		INT32 keyinput;
	} u;

	INT32 selection;
	INT32 int_arr[2];
	float float_arr[2];
	void *pointer; // Generic pointer for anything.

	// What kind of finger is this?
	union {
		boolean menu;
		INT32 mouse;
		INT32 joystick;
	} type;
} touchfinger_t;

extern touchfinger_t touchfingers[NUMTOUCHFINGERS];

// Touch screen button structure
typedef struct
{
	fixed_t x, y; // coordinates (normalized)
	fixed_t w, h; // dimensions
	const char *name, *tinyname; // names
	UINT8 color, pressedcolor; // colors

	boolean down; // is down
	boolean dpad; // d-pad button
	boolean hidden; // doesn't exist
	boolean dontscale; // isn't scaled
	boolean dontscaletext; // text isn't scaled

	// touch customization
	boolean modifying;
	struct {
		float x, y; // coordinates (floating-point, not normalized)
		float w, h; // dimensions (floating-point)
		fixed_t fx, fy; // coordinates (fixed-point, not normalized)
		fixed_t fw, fh; // dimensions (fixed-point)
		boolean snap; // snap to grid
	} supposed;
} touchconfig_t;

typedef struct
{
	INT32 key; // key that this button corresponds to
	boolean defined; // is defined

	fixed_t x, y; // coordinates (normalized)
	fixed_t w, h; // dimensions

	const char *name; // name
	const char *patch; // patch
	UINT8 color, pressedcolor; // colors
	boolean shadow; // display a shadow beneath the button
	UINT32 snapflags; // text snap flags

	boolean down; // is down
	tic_t tics; // time held down

	boolean dontscaletext; // text isn't scaled
} touchnavbutton_t;

// Control buttons
extern touchconfig_t touchcontrols[NUM_GAMECONTROLS];
extern touchconfig_t *usertouchcontrols;

// Navigation buttons
enum
{
	TOUCHNAV_BACK,
	TOUCHNAV_CONFIRM,
	TOUCHNAV_CONSOLE,
	TOUCHNAV_DELETE,
	NUMTOUCHNAV
};

#define TS_NAVTICS 10

extern touchnavbutton_t touchnavigation[NUMTOUCHNAV];

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
extern fixed_t touch_preset_scale;
extern boolean touch_scale_meta;

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

// Is the touch screen available for game inputs?
extern boolean touch_useinputs;
extern consvar_t cv_showfingers;

// Finger event handler
extern void (*touch_fingerhandler)(touchfinger_t *, event_t *);

// Touch screen settings
extern touchmovementstyle_e touch_movementstyle;
extern touchpreset_e touch_preset;
extern boolean touch_camera;
extern INT32 touch_corners;

// Touch config. status
typedef struct
{
	struct
	{
		INT32 width, height;
		INT32 dupx, dupy;
	} vid;

	fixed_t presetscale; // touch_preset_scale
	boolean scalemeta; // touch_scale_meta
	INT32 corners; // touch_corners

	touchpreset_e preset; // touch_preset
	touchmovementstyle_e movementstyle; // touch_movementstyle

	boolean altliveshud; // ST_AltLivesHUDEnabled
	boolean ringslinger; // G_RingSlingerGametype
	boolean ctfgametype; // gametyperules & GTR_TEAMFLAGS
	boolean nights; // maptol & TOL_NIGHTS
	boolean specialstage; // G_IsSpecialStage
	boolean tutorialmode;
	boolean splitscreen;
	UINT8 modeattacking;
	boolean canpause;
	boolean canviewpointswitch; // G_CanViewpointSwitch()
	boolean cantalk; // netgame && !CHAT_MUTE
	boolean canteamtalk; // G_GametypeHasTeams() && players[consoleplayer].ctfteam
	boolean promptactive;
	boolean promptblockcontrols;
} touchconfigstatus_t;

// Touch navigation status
typedef struct
{
	struct
	{
		INT32 width, height;
		INT32 dupx, dupy;
	} vid;

	INT32 corners; // cv_touchcorners.value

	boolean canreturn; // M_TSNav_CanShowBack
	boolean canconfirm; // M_TSNav_CanShowConfirm
	boolean canopenconsole; // M_TSNav_CanShowConsole
	INT32 showdelete; // M_TSNav_DeleteButtonAction

	boolean customizingcontrols; // TS_IsCustomizingControls
	boolean layoutsubmenuopen; // TS_IsCustomizationSubmenuOpen
	INT32 returncorner; // M_TSNav_BackCorner
} touchnavstatus_t;

enum
{
	TSNAV_CORNER_BOTTOM = 1,
	TSNAV_CORNER_RIGHT = 1<<1,
	TSNAV_CORNER_TOP_TEST = 1<<2,
};

extern touchconfigstatus_t touchcontrolstatus;
extern touchnavstatus_t touchnavigationstatus;

// Console variables for the touch screen
extern consvar_t cv_touchinputs;
extern consvar_t cv_touchstyle;
extern consvar_t cv_touchpreset;
extern consvar_t cv_touchlayout;
extern consvar_t cv_touchcamera;
extern consvar_t cv_touchtrans, cv_touchmenutrans;
extern consvar_t cv_touchpresetscale, cv_touchscalemeta;
extern consvar_t cv_touchcorners;
extern consvar_t cv_touchnavmethod;

// Touch layout options
extern consvar_t cv_touchlayoutusegrid;
extern consvar_t cv_touchlayoutwidescreen;

// Touch screen sensitivity
extern consvar_t cv_touchcamhorzsens, cv_touchcamvertsens;
extern consvar_t cv_touchjoyhorzsens, cv_touchjoyvertsens;
extern consvar_t cv_touchjoydeadzone;

// Miscellaneous options
extern consvar_t cv_touchscreenshots;

// Screen joystick movement
#define TOUCHJOYEXTENDX (touch_joystick_w / 2)
#define TOUCHJOYEXTENDY (touch_joystick_h / 2)

extern float touchxmove, touchymove, touchpressure;

void TS_HandleFingerEvent(event_t *ev);

void TS_GetSettings(void);
void TS_UpdateControls(void);

void TS_DefineButtons(void);
void TS_DefineNavigationButtons(void);

void TS_PositionButtons(void);
void TS_PositionNavigation(void);
void TS_PositionExtraUserButtons(void);

void TS_HideNavigationButtons(void);

boolean TS_IsDPadButton(INT32 gc);
void TS_MarkDPadButtons(touchconfig_t *controls);

fixed_t TS_GetDefaultScale(void);
void TS_GetJoystick(INT32 *x, INT32 *y, INT32 *w, INT32 *h, boolean tiny);

void TS_PresetChanged(void);
void TS_BuildPreset(touchconfig_t *controls, touchconfigstatus_t *status,
					touchmovementstyle_e tms, fixed_t scale,
					boolean tiny, boolean widescreen);

// Returns the names of a touch button
const char *TS_GetButtonName(INT32 gc, touchconfigstatus_t *status);
const char *TS_GetButtonShortName(INT32 gc, touchconfigstatus_t *status);

// Sets all button names for a touch config
void TS_SetButtonNames(touchconfig_t *controls, touchconfigstatus_t *status);

// Returns true if a touch preset is active
boolean TS_IsPresetActive(void);

// Updates touch fingers
void TS_UpdateFingers(INT32 realtics);

// Updates touch navigation
void TS_UpdateNavigation(INT32 realtics);

// Touch event received
void TS_OnTouchEvent(INT32 id, evtype_t type, touchevent_t *event);

// Clears touch fingers
void TS_ClearFingers(void);

// Clears navigation buttons
void TS_ClearNavigation(void);

// Sets navigation buttons up
void TS_NavigationFingersUp(void);

// Remaps a finger event to a key
INT32 TS_MapFingerEventToKey(event_t *event, INT32 *nav);

// Checks if the finger (x, y) is touching the specified button (btn)
boolean TS_FingerTouchesButton(INT32 x, INT32 y, touchconfig_t *btn);

// Checks if the finger (x, y) is touching the specified navigation button (btn)
boolean TS_FingerTouchesNavigationButton(INT32 x, INT32 y, touchnavbutton_t *btn);

// Checks if the finger is touching the joystick area.
boolean TS_FingerTouchesJoystickArea(INT32 x, INT32 y);

// Checks if the game control is a player control key
boolean TS_ButtonIsPlayerControl(INT32 gc);

// Scales a touch button
void TS_ScaleCoords(INT32 *x, INT32 *y, INT32 *w, INT32 *h, boolean normalized, boolean screenscale);

// Normalizes a touch button
void TS_NormalizeButton(touchconfig_t *button);

// Normalizes a touch config
void TS_NormalizeConfig(touchconfig_t *config, int configsize);

// Denormalizes XY coordinates
void TS_DenormalizeCoords(fixed_t *x, fixed_t *y);

// Centers coordinates
void TS_CenterCoords(fixed_t *x, fixed_t *y);

// Centers integer coordinates
void TS_CenterIntegerCoords(INT32 *x, INT32 *y);

// Defines a d-pad
void TS_DPadPreset(touchconfig_t *controls, fixed_t xscale, fixed_t yscale, fixed_t dw, boolean tiny);
#endif // TOUCHINPUTS
#endif // __TS_MAIN_H__
