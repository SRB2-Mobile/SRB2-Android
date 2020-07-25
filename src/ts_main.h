// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ts_main.h
/// \brief Touch controls

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
#define NUMTOUCHFINGERS 20

typedef boolean (*longpressaction_t) (void *finger);

typedef struct
{
	INT32 x, y;
	float pressure;
	boolean down;

	tic_t longpress;
	longpressaction_t longpressaction;

	INT32 lastx, lasty;
	boolean ignoremotion;

	// A finger has either a game control or a key input down.
	union {
		INT32 gamecontrol;
		INT32 keyinput;
	} u;

	// Alternate selections that don't interfere with the above.
	union {
		INT32 selection;
		INT32 snake;
	} extra;

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
	UINT8 color; // color

	boolean down; // is down
	boolean dpad; // d-pad button
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
extern consvar_t cv_showfingers;

// Finger event handler
extern void (*touch_fingerhandler)(touchfinger_t *, event_t *);

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

void TS_HandleFingerEvent(event_t *ev);

void TS_GetSettings(void);
void TS_UpdateControls(void);
void TS_DefineButtons(void);

void TS_PositionButtons(void);
void TS_PositionNavigation(void);
void TS_PositionExtraUserButtons(void);

boolean TS_IsDPadButton(INT32 gc);
void TS_MarkDPadButtons(touchconfig_t *controls);

fixed_t TS_GetDefaultScale(void);
void TS_GetJoystick(INT32 *x, INT32 *y, INT32 *w, INT32 *h, boolean tiny);

void TS_PresetChanged(void);
void TS_BuildPreset(touchconfig_t *controls, touchconfigstatus_t *status, touchmovementstyle_e tms, fixed_t scale, boolean tiny);

// Returns the names of a touch button
const char *TS_GetButtonName(INT32 gc);
const char *TS_GetButtonShortName(INT32 gc);

// Sets all button names for a touch config
void TS_SetButtonNames(touchconfig_t *controls);

// Returns true if a touch preset is active
boolean TS_IsPresetActive(void);

// Updates touch fingers
void TS_UpdateFingers(INT32 realtics);

// Finger event received
void TS_PostFingerEvent(event_t *event);

// Gets a key from a finger event
INT32 TS_MapFingerEventToKey(event_t *event);

// Checks if the finger (x, y) is touching the specified button (btn)
boolean TS_FingerTouchesButton(INT32 x, INT32 y, touchconfig_t *btn);

// Checks if the finger (x, y) is touching the specified navigation button (btn)
boolean TS_FingerTouchesNavigationButton(INT32 x, INT32 y, touchconfig_t *btn);

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
