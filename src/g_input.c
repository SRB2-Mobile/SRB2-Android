// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  g_input.c
/// \brief handle mouse/keyboard/joystick inputs,
///        maps inputs to game controls (forward, spin, jump...)

#include "doomdef.h"
#include "doomstat.h"
#include "m_menu.h"
#include "m_misc.h"
#include "p_tick.h"
#include "g_game.h"
#include "g_input.h"
#include "keys.h"
#include "hu_stuff.h" // need HUFONT start & end
#include "st_stuff.h"
#include "d_net.h"
#include "d_player.h"
#include "r_main.h"
#include "z_zone.h"
#include "console.h"
#include "i_system.h"
#include "lua_hook.h"

#ifdef TOUCHINPUTS
#include "ts_custom.h"
#endif

#define MAXMOUSESENSITIVITY 100 // sensitivity steps

static CV_PossibleValue_t mousesens_cons_t[] = {{1, "MIN"}, {MAXMOUSESENSITIVITY, "MAX"}, {0, NULL}};
static CV_PossibleValue_t onecontrolperkey_cons_t[] = {{1, "One"}, {2, "Several"}, {0, NULL}};

// mouse values are used once
consvar_t cv_mousesens = {"mousesens", "20", CV_SAVE, mousesens_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mousesens2 = {"mousesens2", "20", CV_SAVE, mousesens_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mouseysens = {"mouseysens", "20", CV_SAVE, mousesens_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mouseysens2 = {"mouseysens2", "20", CV_SAVE, mousesens_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_controlperkey = {"controlperkey", "One", CV_SAVE, onecontrolperkey_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

INT32 mousex, mousey;
INT32 mlooky; // like mousey but with a custom sensitivity for mlook

INT32 mouse2x, mouse2y, mlook2y;

// joystick values are repeated
INT32 joyxmove[JOYAXISSET], joyymove[JOYAXISSET], joy2xmove[JOYAXISSET], joy2ymove[JOYAXISSET];

#ifdef TOUCHINPUTS
float touchxmove, touchymove, touchpressure;
#endif

// current state of the keys: true if pushed
UINT8 gamekeydown[NUMINPUTS];

#ifdef TOUCHINPUTS
// Finger data
touchfinger_t touchfingers[NUMTOUCHFINGERS];
UINT8 touchcontroldown[num_gamecontrols];

// Screen buttons
touchconfig_t touchcontrols[num_gamecontrols];
touchconfig_t touchnavigation[NUMKEYS];
touchconfig_t *usertouchcontrols = NULL;

// Touch screen config. status
touchconfigstatus_t touchcontrolstatus;
touchconfigstatus_t touchnavigationstatus;

// Input variables
INT32 touch_joystick_x, touch_joystick_y, touch_joystick_w, touch_joystick_h;
fixed_t touch_gui_scale;

// Is the touch screen available?
boolean touch_screenexists = false;

// Touch screen settings
touchmovementstyle_e touch_movementstyle;
touchpreset_e touch_preset;
boolean touch_camera;

// Console variables for the touch screen
static CV_PossibleValue_t touchstyle_cons_t[] = {
	{tms_joystick, "Joystick"},
	{tms_dpad, "D-Pad"},
	{0, NULL}};

static CV_PossibleValue_t touchpreset_cons_t[] = {
	{touchpreset_none, "None"},
	{touchpreset_normal, "Default"},
	{touchpreset_tiny, "Tiny"},
	{0, NULL}};

consvar_t cv_touchstyle = {"touch_movementstyle", "Joystick", CV_SAVE|CV_CALL|CV_NOINIT, touchstyle_cons_t, G_UpdateTouchControls, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_touchpreset = {"touch_preset", "Default", CV_SAVE|CV_CALL|CV_NOINIT, touchpreset_cons_t, G_TouchPresetChanged, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_touchlayout = {"touch_layout", "None", CV_SAVE|CV_CALL|CV_NOINIT, NULL, TS_LoadLayoutFromCVar, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_touchcamera = {"touch_camera", "On", CV_SAVE|CV_CALL|CV_NOINIT, CV_OnOff, G_UpdateTouchControls, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t touchguiscale_cons_t[] = {{FRACUNIT/2, "MIN"}, {3 * FRACUNIT, "MAX"}, {0, NULL}};
consvar_t cv_touchguiscale = {"touch_guiscale", "0.75", CV_FLOAT|CV_SAVE|CV_CALL|CV_NOINIT, touchguiscale_cons_t, G_UpdateTouchControls, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t touchtrans_cons_t[] = {{0, "MIN"}, {10, "MAX"}, {0, NULL}};
consvar_t cv_touchtrans = {"touch_transinput", "10", CV_SAVE, touchtrans_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_touchmenutrans = {"touch_transmenu", "10", CV_SAVE, touchtrans_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

// Touch layout options
consvar_t cv_touchlayoutusegrid = {"touch_layoutusegrid", "On", CV_CALL|CV_NOINIT, CV_OnOff, TS_SynchronizeCurrentLayout, 0, NULL, NULL, 0, 0, NULL};

// Touch screen sensitivity
#define MAXTOUCHSENSITIVITY 100 // sensitivity steps
static CV_PossibleValue_t touchsens_cons_t[] = {{1, "MIN"}, {MAXTOUCHSENSITIVITY, "MAX"}, {0, NULL}};
consvar_t cv_touchsens = {"touch_sens", "40", CV_SAVE, touchsens_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_touchvertsens = {"touch_vertsens", "45", CV_SAVE, touchsens_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t touchjoysens_cons_t[] = {{FRACUNIT/100, "MIN"}, {4 * FRACUNIT, "MAX"}, {0, NULL}};
consvar_t cv_touchjoyhorzsens = {"touch_joyhorzsens", "0.5", CV_FLOAT|CV_SAVE, touchjoysens_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_touchjoyvertsens = {"touch_joyvertsens", "0.5", CV_FLOAT|CV_SAVE, touchjoysens_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#endif

// two key codes (or virtual key) per game control
INT32 gamecontrol[num_gamecontrols][2];
INT32 gamecontrolbis[num_gamecontrols][2]; // secondary splitscreen player
INT32 gamecontroldefault[num_gamecontrolschemes][num_gamecontrols][2]; // default control storage, use 0 (gcs_custom) for memory retention
INT32 gamecontrolbisdefault[num_gamecontrolschemes][num_gamecontrols][2];

// lists of GC codes for selective operation
const INT32 gcl_tutorial_check[num_gcl_tutorial_check] = {
	gc_forward, gc_backward, gc_strafeleft, gc_straferight,
	gc_turnleft, gc_turnright
};

const INT32 gcl_tutorial_used[num_gcl_tutorial_used] = {
	gc_forward, gc_backward, gc_strafeleft, gc_straferight,
	gc_turnleft, gc_turnright,
	gc_jump, gc_use
};

const INT32 gcl_tutorial_full[num_gcl_tutorial_full] = {
	gc_forward, gc_backward, gc_strafeleft, gc_straferight,
	gc_lookup, gc_lookdown, gc_turnleft, gc_turnright, gc_centerview,
	gc_jump, gc_use,
	gc_fire, gc_firenormal
};

const INT32 gcl_movement[num_gcl_movement] = {
	gc_forward, gc_backward, gc_strafeleft, gc_straferight
};

const INT32 gcl_camera[num_gcl_camera] = {
	gc_turnleft, gc_turnright
};

const INT32 gcl_movement_camera[num_gcl_movement_camera] = {
	gc_forward, gc_backward, gc_strafeleft, gc_straferight,
	gc_turnleft, gc_turnright
};

const INT32 gcl_jump[num_gcl_jump] = { gc_jump };

const INT32 gcl_use[num_gcl_use] = { gc_use };

const INT32 gcl_jump_use[num_gcl_jump_use] = {
	gc_jump, gc_use
};

typedef struct
{
	UINT8 time;
	UINT8 state;
	UINT8 clicks;
} dclick_t;
static dclick_t mousedclicks[MOUSEBUTTONS];
static dclick_t joydclicks[JOYBUTTONS + JOYHATS*4];
static dclick_t mouse2dclicks[MOUSEBUTTONS];
static dclick_t joy2dclicks[JOYBUTTONS + JOYHATS*4];

// protos
static UINT8 G_CheckDoubleClick(UINT8 state, dclick_t *dt);

#ifdef TOUCHINPUTS
boolean G_TouchPresetActive(void)
{
	return (touch_preset != touchpreset_none);
}

void G_ScaleTouchCoords(INT32 *x, INT32 *y, INT32 *w, INT32 *h, boolean normalized, boolean screenscale)
{
	fixed_t xs = FRACUNIT;
	fixed_t ys = FRACUNIT;

	if (normalized)
		G_DenormalizeCoords(&xs, &ys);

	if (screenscale)
	{
		xs *= vid.dupx;
		ys *= vid.dupy;
		if (w)
			*w = FixedMul((*w), vid.dupx * FRACUNIT) / FRACUNIT;
		if (h)
			*h = FixedMul((*h), vid.dupy * FRACUNIT) / FRACUNIT;
	}
	else
	{
		if (w)
			*w = FixedInt(*w);
		if (h)
			*h = FixedInt(*h);
	}

	*x = FixedMul((*x), xs) / FRACUNIT;
	*y = FixedMul((*y), ys) / FRACUNIT;
}

static boolean IsFingerTouchingButton(INT32 x, INT32 y, touchconfig_t *btn, boolean center)
{
	INT32 tx = btn->x, ty = btn->y, tw = btn->w, th = btn->h;
	G_ScaleTouchCoords(&tx, &ty, &tw, &th, true, (!btn->dontscale));
	if (center && (!btn->dontscale))
		G_CenterIntegerCoords(&tx, &ty);
	return (x >= tx && x <= tx + tw && y >= ty && y <= ty + th);
}

boolean G_FingerTouchesButton(INT32 x, INT32 y, touchconfig_t *btn)
{
	return IsFingerTouchingButton(x, y, btn, true);
}

boolean G_FingerTouchesNavigationButton(INT32 x, INT32 y, touchconfig_t *btn)
{
	return IsFingerTouchingButton(x, y, btn, false);
}

boolean G_FingerTouchesJoystickArea(INT32 x, INT32 y)
{
	INT32 tx = touch_joystick_x, ty = touch_joystick_y, tw = touch_joystick_w, th = touch_joystick_h;
	G_ScaleTouchCoords(&tx, &ty, &tw, &th, false, true);
	G_CenterIntegerCoords(&tx, &ty);
	return (x >= tx && x <= tx + tw && y >= ty && y <= ty + th);
}

boolean G_TouchButtonIsPlayerControl(INT32 gamecontrol)
{
	switch (gamecontrol)
	{
		case gc_talkkey:
		case gc_teamkey:
		case gc_scores:
		case gc_console:
		case gc_pause:
		case gc_systemmenu:
		case gc_screenshot:
		case gc_recordgif:
		case gc_viewpoint:
		case gc_camtoggle:
		case gc_camreset:
			return false;
		default:
			break;
	}
	return true;
}

static void G_HandleNonPlayerControlButton(INT32 gamecontrol)
{
	// Handle menu button
	if (gamecontrol == gc_systemmenu)
		M_StartControlPanel();
	// Handle console button
	else if (gamecontrol == gc_console)
		CON_Toggle();
	// Handle pause button
	else if (gamecontrol == gc_pause)
		G_HandlePauseKey(true);
	// Handle spy mode
	else if (gamecontrol == gc_viewpoint)
		G_DoViewpointSwitch();
	// Handle screenshot
	else if (gamecontrol == gc_screenshot)
		M_ScreenShot();
	// Handle movie mode
	else if (gamecontrol == gc_recordgif)
		((moviemode) ? M_StopMovie : M_StartMovie)();
	// Handle chasecam toggle
	else if (gamecontrol == gc_camtoggle)
		G_ToggleChaseCam();
	// Handle talk buttons
	else if ((gamecontrol == gc_talkkey || gamecontrol == gc_teamkey) && netgame)
	{
		// Raise the screen keyboard if not muted
		boolean raise = (!CHAT_MUTE);

		// Only raise the screen keyboard in team games
		// if you're assigned to any team
		if (raise && (gamecontrol == gc_teamkey))
			raise = (G_GametypeHasTeams() && (players[consoleplayer].ctfteam != 0));

		// Do it (works with console chat)
		if (raise)
		{
			if (!HU_IsChatOpen())
				HU_OpenChat();
			else
				HU_CloseChat();
		}
	}
}

//
// Remaps touch input events to game controls.
// In case of moving the joystick, it sets touch axes.
// Otherwise, it moves the camera.
//
static void G_HandleFingerEvent(event_t *ev)
{
	INT32 x = ev->x;
	INT32 y = ev->y;
	touchfinger_t *finger = &touchfingers[ev->key];
	boolean touchmotion = (ev->type == ev_touchmotion);
	boolean foundbutton = false;
	boolean movecamera = true;
	INT32 gc = finger->u.gamecontrol, i;

	if (TS_IsCustomizingControls())
		return;

	switch (ev->type)
	{
		case ev_touchdown:
		case ev_touchmotion:
			// Ignore when the menu, console, or chat window are open
			if (!G_InGameInput())
			{
				touchxmove = touchymove = touchpressure = 0.0f;
				break;
			}

			// This finger ignores touch motion events, so don't do anything.
			// Non-control keys ignore touch motion events.
			if (touchmotion && (finger->ignoremotion || (!G_TouchButtonIsPlayerControl(gc))))
			{
				// Update finger position at least
				if (!finger->ignoremotion)
				{
					finger->x = x;
					finger->y = y;
				}
				break;
			}

			// Lactozilla: Find every on-screen button and
			// check if they are below your finger.
			for (i = 0; i < num_gamecontrols; i++)
			{
				touchconfig_t *btn = &touchcontrols[i];

				// Ignore camera and joystick movement
				if (finger->type.mouse)
					break;

				// Ignore hidden buttons
				if (btn->hidden)
					continue;

				// Ignore mismatching movement styles
				if ((touch_movementstyle != tms_dpad) && btn->dpad)
					continue;

				// Ignore joystick
				if (i == gc_joystick)
					continue;

				// Ignore d-pad if joystick is hidden
				if (btn->dpad && (touchcontrols[gc_joystick].hidden))
					continue;

				// In a touch motion event, simulate a key up event by clearing touchcontroldown.
				// This is done so that the buttons that are down don't 'stick'
				// if you move your finger from a button to another.
				if (touchmotion && (gc > gc_null))
				{
					// Let go of this button.
					touchcontroldown[gc] = 0;
					movecamera = false;
				}

				// Check if your finger touches this button.
				if (G_FingerTouchesButton(x, y, btn) && (!touchcontroldown[i]))
				{
					finger->x = x;
					finger->y = y;
					finger->pressure = ev->pressure;
					finger->u.gamecontrol = i;
					touchcontroldown[i] = 1;
					foundbutton = true;
					break;
				}
			}

			// Check if your finger touches the joystick area.
			if (!foundbutton)
			{
				if (G_FingerTouchesJoystickArea(x, y) && (!touchcontrols[gc_joystick].hidden))
				{
					// Joystick
					if (touch_movementstyle == tms_joystick && (!touchmotion))
					{
						finger->x = x;
						finger->y = y;
						finger->pressure = ev->pressure;
						finger->type.joystick = FINGERMOTION_JOYSTICK;
						finger->u.gamecontrol = -1;
						foundbutton = true;
						break;
					}
					else if (touch_movementstyle == tms_dpad)
					{
						movecamera = false;
						break;
					}
				}
			}

			// In some gamestates, set a specific gamecontrol down.
			if (!foundbutton)
			{
				gc = gc_null;

				if (gamestate == GS_INTERMISSION || gamestate == GS_CUTSCENE)
					gc = gc_use;
				else if (promptblockcontrols && F_GetPromptHideHud(y / vid.dupy))
					gc = gc_jump;

				if (gc != gc_null)
				{
					finger->x = x;
					finger->y = y;
					finger->pressure = ev->pressure;
					finger->ignoremotion = true;
					finger->u.gamecontrol = gc;
					touchcontroldown[gc] = 1;
					foundbutton = true;
				}
			}

			// The finger is moving either the joystick or the camera.
			if (!foundbutton)
			{
				INT32 dx = ev->dx;
				INT32 dy = ev->dy;

				if (touchmotion && finger->type.joystick) // Remember that this is an union!
				{
					INT32 movex = (INT32)(dx*((cv_touchsens.value*cv_touchsens.value)/110.0f + 0.1f));
					INT32 movey = (INT32)(dy*((cv_touchsens.value*cv_touchsens.value)/110.0f + 0.1f));

					// Joystick
					if (finger->type.joystick == FINGERMOTION_JOYSTICK)
					{
						float xsens = FIXED_TO_FLOAT(cv_touchjoyhorzsens.value);
						float ysens = FIXED_TO_FLOAT(cv_touchjoyvertsens.value);
						INT32 padx = touch_joystick_x, pady = touch_joystick_y;
						INT32 padw = touch_joystick_w, padh = touch_joystick_h;

						G_ScaleTouchCoords(&padx, &pady, &padw, &padh, false, true);
						G_CenterIntegerCoords(&padx, &pady);

						dx = x - (padx + (padw / 2));
						dy = y - (pady + (padh / 2));

						touchxmove = (((float)dx * xsens) / FIXED_TO_FLOAT(TOUCHJOYEXTENDX));
						touchymove = (((float)dy * ysens) / FIXED_TO_FLOAT(TOUCHJOYEXTENDY));
						touchpressure = ev->pressure;
					}
					// Mouse
					else if (finger->type.mouse == FINGERMOTION_MOUSE && (touch_camera && movecamera))
					{
						mousex = movex;
						mousey = movey;
						mlooky = (INT32)(dy*((cv_touchvertsens.value*cv_touchsens.value)/110.0f + 0.1f));
					}

					finger->x = x;
					finger->y = y;
					finger->pressure = ev->pressure;
				}
				else if (touch_camera && movecamera)
				{
					finger->x = x;
					finger->y = y;
					finger->pressure = ev->pressure;
					finger->type.mouse = FINGERMOTION_MOUSE;
					finger->u.gamecontrol = gc_null;
				}
			}
			break;

		case ev_touchup:
			// Let go of this finger.
			gc = finger->u.gamecontrol;
			if (gc > gc_null)
			{
				if (!G_TouchButtonIsPlayerControl(gc) && G_FingerTouchesButton(finger->x, finger->y, &touchcontrols[gc]))
					G_HandleNonPlayerControlButton(gc);
				touchcontroldown[gc] = 0;
			}

			finger->u.gamecontrol = gc_null;
			finger->ignoremotion = false;

			// Reset joystick movement.
			if (finger->type.joystick == FINGERMOTION_JOYSTICK)
				touchxmove = touchymove = touchpressure = 0.0f;

			// Remember that this is an union!
			finger->type.mouse = FINGERMOTION_NONE;
			break;

		default:
			break;
	}
}

void G_UpdateFingers(INT32 realtics)
{
	INT32 i;

	for (i = 0; i < NUMTOUCHFINGERS; i++)
	{
		touchfinger_t *finger = &touchfingers[i];

		// Run finger long press action
		if (finger->longpressaction)
		{
			boolean done = finger->longpressaction(finger);
			finger->longpress += realtics;

			if (done)
			{
				finger->longpressaction = NULL;
				finger->longpress = 0;
			}
		}
	}
}
#endif

//
// Remaps the inputs to game controls.
//
// A game control can be triggered by one or more keys/buttons.
//
// Each key/mousebutton/joybutton/finger triggers ONLY ONE game control.
//
void G_MapEventsToControls(event_t *ev)
{
	INT32 i;
	UINT8 flag;

	switch (ev->type)
	{
		case ev_keydown:
			if (ev->key < NUMINPUTS)
				gamekeydown[ev->key] = 1;
#ifdef PARANOIA
			else
			{
				CONS_Debug(DBG_GAMELOGIC, "Bad downkey input %d\n",ev->key);
			}

#endif
			break;

		case ev_keyup:
			if (ev->key < NUMINPUTS)
				gamekeydown[ev->key] = 0;
#ifdef PARANOIA
			else
			{
				CONS_Debug(DBG_GAMELOGIC, "Bad upkey input %d\n",ev->key);
			}
#endif
			break;

#ifdef TOUCHINPUTS
		case ev_touchdown:
		case ev_touchmotion:
		case ev_touchup:
			G_HandleFingerEvent(ev);
			break;
#endif

		case ev_mouse: // buttons are virtual keys
			if (!G_InGameInput())
				break;
			mousex = (INT32)(ev->x*((cv_mousesens.value*cv_mousesens.value)/110.0f + 0.1f));
			mousey = (INT32)(ev->y*((cv_mousesens.value*cv_mousesens.value)/110.0f + 0.1f));
			mlooky = (INT32)(ev->y*((cv_mouseysens.value*cv_mousesens.value)/110.0f + 0.1f));
			break;

		case ev_joystick: // buttons are virtual keys
			i = ev->key;
			if (i >= JOYAXISSET || !G_InGameInput())
				break;
			if (ev->x != INT32_MAX) joyxmove[i] = ev->x;
			if (ev->y != INT32_MAX) joyymove[i] = ev->y;
			break;

		case ev_joystick2: // buttons are virtual keys
			i = ev->key;
			if (i >= JOYAXISSET || !G_InGameInput())
				break;
			if (ev->x != INT32_MAX) joy2xmove[i] = ev->x;
			if (ev->y != INT32_MAX) joy2ymove[i] = ev->y;
			break;

		case ev_mouse2: // buttons are virtual keys
			if (!G_InGameInput())
				break;
			mouse2x = (INT32)(ev->x*((cv_mousesens2.value*cv_mousesens2.value)/110.0f + 0.1f));
			mouse2y = (INT32)(ev->y*((cv_mousesens2.value*cv_mousesens2.value)/110.0f + 0.1f));
			mlook2y = (INT32)(ev->y*((cv_mouseysens2.value*cv_mousesens2.value)/110.0f + 0.1f));
			break;

		default:
			break;
	}

	// ALWAYS check for mouse & joystick double-clicks even if no mouse event
	for (i = 0; i < MOUSEBUTTONS; i++)
	{
		flag = G_CheckDoubleClick(gamekeydown[KEY_MOUSE1+i], &mousedclicks[i]);
		gamekeydown[KEY_DBLMOUSE1+i] = flag;
	}

	for (i = 0; i < JOYBUTTONS + JOYHATS*4; i++)
	{
		flag = G_CheckDoubleClick(gamekeydown[KEY_JOY1+i], &joydclicks[i]);
		gamekeydown[KEY_DBLJOY1+i] = flag;
	}

	for (i = 0; i < MOUSEBUTTONS; i++)
	{
		flag = G_CheckDoubleClick(gamekeydown[KEY_2MOUSE1+i], &mouse2dclicks[i]);
		gamekeydown[KEY_DBL2MOUSE1+i] = flag;
	}

	for (i = 0; i < JOYBUTTONS + JOYHATS*4; i++)
	{
		flag = G_CheckDoubleClick(gamekeydown[KEY_2JOY1+i], &joy2dclicks[i]);
		gamekeydown[KEY_DBL2JOY1+i] = flag;
	}
}

//
// General double-click detection routine for any kind of input.
//
static UINT8 G_CheckDoubleClick(UINT8 state, dclick_t *dt)
{
	if (state != dt->state && dt->time > 1)
	{
		dt->state = state;
		if (state)
			dt->clicks++;
		if (dt->clicks == 2)
		{
			dt->clicks = 0;
			return true;
		}
		else
			dt->time = 0;
	}
	else
	{
		dt->time++;
		if (dt->time > 20)
		{
			dt->clicks = 0;
			dt->state = 0;
		}
	}
	return false;
}

boolean G_HandlePauseKey(boolean ispausebreak)
{
	if (modeattacking && !demoplayback && (gamestate == GS_LEVEL))
	{
		pausebreakkey = ispausebreak;
		if (menuactive || pausedelay < 0 || leveltime < 2)
			return true;

		if (pausedelay < 1+(NEWTICRATE/2))
			pausedelay = 1+(NEWTICRATE/2);
		else if (++pausedelay > 1+(NEWTICRATE/2)+(NEWTICRATE/3))
		{
			G_SetModeAttackRetryFlag();
			return true;
		}
		pausedelay++; // counteract subsequent subtraction this frame
	}
	else
	{
		INT32 oldpausedelay = pausedelay;
		pausedelay = (NEWTICRATE/7);
		if (!oldpausedelay)
		{
			// command will handle all the checks for us
			COM_ImmedExecute("pause");
			return true;
		}
	}

	return false;
}

// Handles the viewpoint switch key being pressed.
boolean G_DoViewpointSwitch(void)
{
	// ViewpointSwitch Lua hook.
	UINT8 canSwitchView = 0;

	if (splitscreen || !netgame)
		displayplayer = consoleplayer;
	else
	{
		do
		{
			displayplayer++;
			if (displayplayer == MAXPLAYERS)
				displayplayer = 0;

			if (!playeringame[displayplayer])
				continue;

			// Call ViewpointSwitch hooks here.
			canSwitchView = LUAh_ViewpointSwitch(&players[consoleplayer], &players[displayplayer], false);
			if (canSwitchView == 1) // Set viewpoint to this player
				break;
			else if (canSwitchView == 2) // Skip this player
				continue;

			if (!G_CanViewpointSwitchToPlayer(&players[displayplayer]))
				continue;

			break;
		} while (displayplayer != consoleplayer);

		// change statusbar also if playing back demo
		if (singledemo)
			ST_changeDemoView();

		// tell who's the view
		CONS_Printf(M_GetText("Viewpoint: %s\n"), player_names[displayplayer]);

		return true;
	}

	return false;
}

// Handles the camera toggle key being pressed.
boolean G_ToggleChaseCam(void)
{
	if (!camtoggledelay)
	{
		camtoggledelay = NEWTICRATE / 7;
		CV_SetValue(&cv_chasecam, cv_chasecam.value ? 0 : 1);
		return true;
	}
	return false;
}

// Handles the camera toggle key being pressed by the second player.
boolean G_ToggleChaseCam2(void)
{
	if (!camtoggledelay2)
	{
		camtoggledelay2 = NEWTICRATE / 7;
		CV_SetValue(&cv_chasecam2, cv_chasecam2.value ? 0 : 1);
		return true;
	}
	return false;
}

typedef struct
{
	INT32 keynum;
	const char *name;
} keyname_t;

static keyname_t keynames[] =
{
	{KEY_SPACE, "SPACE"},
	{KEY_CAPSLOCK, "CAPS LOCK"},
	{KEY_ENTER, "ENTER"},
	{KEY_TAB, "TAB"},
	{KEY_ESCAPE, "ESCAPE"},
	{KEY_BACKSPACE, "BACKSPACE"},

	{KEY_NUMLOCK, "NUMLOCK"},
	{KEY_SCROLLLOCK, "SCROLLLOCK"},

	// bill gates keys
	{KEY_LEFTWIN, "LEFTWIN"},
	{KEY_RIGHTWIN, "RIGHTWIN"},
	{KEY_MENU, "MENU"},

	{KEY_LSHIFT, "LSHIFT"},
	{KEY_RSHIFT, "RSHIFT"},
	{KEY_LSHIFT, "SHIFT"},
	{KEY_LCTRL, "LCTRL"},
	{KEY_RCTRL, "RCTRL"},
	{KEY_LCTRL, "CTRL"},
	{KEY_LALT, "LALT"},
	{KEY_RALT, "RALT"},
	{KEY_LALT, "ALT"},

	// keypad keys
	{KEY_KPADSLASH, "KEYPAD /"},
	{KEY_KEYPAD7, "KEYPAD 7"},
	{KEY_KEYPAD8, "KEYPAD 8"},
	{KEY_KEYPAD9, "KEYPAD 9"},
	{KEY_MINUSPAD, "KEYPAD -"},
	{KEY_KEYPAD4, "KEYPAD 4"},
	{KEY_KEYPAD5, "KEYPAD 5"},
	{KEY_KEYPAD6, "KEYPAD 6"},
	{KEY_PLUSPAD, "KEYPAD +"},
	{KEY_KEYPAD1, "KEYPAD 1"},
	{KEY_KEYPAD2, "KEYPAD 2"},
	{KEY_KEYPAD3, "KEYPAD 3"},
	{KEY_KEYPAD0, "KEYPAD 0"},
	{KEY_KPADDEL, "KEYPAD ."},

	// extended keys (not keypad)
	{KEY_HOME, "HOME"},
	{KEY_UPARROW, "UP ARROW"},
	{KEY_PGUP, "PGUP"},
	{KEY_LEFTARROW, "LEFT ARROW"},
	{KEY_RIGHTARROW, "RIGHT ARROW"},
	{KEY_END, "END"},
	{KEY_DOWNARROW, "DOWN ARROW"},
	{KEY_PGDN, "PGDN"},
	{KEY_INS, "INS"},
	{KEY_DEL, "DEL"},

	// other keys
	{KEY_F1, "F1"},
	{KEY_F2, "F2"},
	{KEY_F3, "F3"},
	{KEY_F4, "F4"},
	{KEY_F5, "F5"},
	{KEY_F6, "F6"},
	{KEY_F7, "F7"},
	{KEY_F8, "F8"},
	{KEY_F9, "F9"},
	{KEY_F10, "F10"},
	{KEY_F11, "F11"},
	{KEY_F12, "F12"},

	// KEY_CONSOLE has an exception in the keyname code
	{'`', "TILDE"},
	{KEY_PAUSE, "PAUSE/BREAK"},

	// virtual keys for mouse buttons and joystick buttons
	{KEY_MOUSE1+0,"MOUSE1"},
	{KEY_MOUSE1+1,"MOUSE2"},
	{KEY_MOUSE1+2,"MOUSE3"},
	{KEY_MOUSE1+3,"MOUSE4"},
	{KEY_MOUSE1+4,"MOUSE5"},
	{KEY_MOUSE1+5,"MOUSE6"},
	{KEY_MOUSE1+6,"MOUSE7"},
	{KEY_MOUSE1+7,"MOUSE8"},
	{KEY_2MOUSE1+0,"SEC_MOUSE2"}, // BP: sorry my mouse handler swap button 1 and 2
	{KEY_2MOUSE1+1,"SEC_MOUSE1"},
	{KEY_2MOUSE1+2,"SEC_MOUSE3"},
	{KEY_2MOUSE1+3,"SEC_MOUSE4"},
	{KEY_2MOUSE1+4,"SEC_MOUSE5"},
	{KEY_2MOUSE1+5,"SEC_MOUSE6"},
	{KEY_2MOUSE1+6,"SEC_MOUSE7"},
	{KEY_2MOUSE1+7,"SEC_MOUSE8"},
	{KEY_MOUSEWHEELUP, "Wheel 1 UP"},
	{KEY_MOUSEWHEELDOWN, "Wheel 1 Down"},
	{KEY_2MOUSEWHEELUP, "Wheel 2 UP"},
	{KEY_2MOUSEWHEELDOWN, "Wheel 2 Down"},

	{KEY_JOY1+0, "JOY1"},
	{KEY_JOY1+1, "JOY2"},
	{KEY_JOY1+2, "JOY3"},
	{KEY_JOY1+3, "JOY4"},
	{KEY_JOY1+4, "JOY5"},
	{KEY_JOY1+5, "JOY6"},
	{KEY_JOY1+6, "JOY7"},
	{KEY_JOY1+7, "JOY8"},
	{KEY_JOY1+8, "JOY9"},
#if !defined (NOMOREJOYBTN_1S)
	// we use up to 32 buttons in DirectInput
	{KEY_JOY1+9, "JOY10"},
	{KEY_JOY1+10, "JOY11"},
	{KEY_JOY1+11, "JOY12"},
	{KEY_JOY1+12, "JOY13"},
	{KEY_JOY1+13, "JOY14"},
	{KEY_JOY1+14, "JOY15"},
	{KEY_JOY1+15, "JOY16"},
	{KEY_JOY1+16, "JOY17"},
	{KEY_JOY1+17, "JOY18"},
	{KEY_JOY1+18, "JOY19"},
	{KEY_JOY1+19, "JOY20"},
	{KEY_JOY1+20, "JOY21"},
	{KEY_JOY1+21, "JOY22"},
	{KEY_JOY1+22, "JOY23"},
	{KEY_JOY1+23, "JOY24"},
	{KEY_JOY1+24, "JOY25"},
	{KEY_JOY1+25, "JOY26"},
	{KEY_JOY1+26, "JOY27"},
	{KEY_JOY1+27, "JOY28"},
	{KEY_JOY1+28, "JOY29"},
	{KEY_JOY1+29, "JOY30"},
	{KEY_JOY1+30, "JOY31"},
	{KEY_JOY1+31, "JOY32"},
#endif
	// the DOS version uses Allegro's joystick support
	{KEY_HAT1+0, "HATUP"},
	{KEY_HAT1+1, "HATDOWN"},
	{KEY_HAT1+2, "HATLEFT"},
	{KEY_HAT1+3, "HATRIGHT"},
	{KEY_HAT1+4, "HATUP2"},
	{KEY_HAT1+5, "HATDOWN2"},
	{KEY_HAT1+6, "HATLEFT2"},
	{KEY_HAT1+7, "HATRIGHT2"},
	{KEY_HAT1+8, "HATUP3"},
	{KEY_HAT1+9, "HATDOWN3"},
	{KEY_HAT1+10, "HATLEFT3"},
	{KEY_HAT1+11, "HATRIGHT3"},
	{KEY_HAT1+12, "HATUP4"},
	{KEY_HAT1+13, "HATDOWN4"},
	{KEY_HAT1+14, "HATLEFT4"},
	{KEY_HAT1+15, "HATRIGHT4"},

	{KEY_DBLMOUSE1+0, "DBLMOUSE1"},
	{KEY_DBLMOUSE1+1, "DBLMOUSE2"},
	{KEY_DBLMOUSE1+2, "DBLMOUSE3"},
	{KEY_DBLMOUSE1+3, "DBLMOUSE4"},
	{KEY_DBLMOUSE1+4, "DBLMOUSE5"},
	{KEY_DBLMOUSE1+5, "DBLMOUSE6"},
	{KEY_DBLMOUSE1+6, "DBLMOUSE7"},
	{KEY_DBLMOUSE1+7, "DBLMOUSE8"},
	{KEY_DBL2MOUSE1+0, "DBLSEC_MOUSE2"}, // BP: sorry my mouse handler swap button 1 and 2
	{KEY_DBL2MOUSE1+1, "DBLSEC_MOUSE1"},
	{KEY_DBL2MOUSE1+2, "DBLSEC_MOUSE3"},
	{KEY_DBL2MOUSE1+3, "DBLSEC_MOUSE4"},
	{KEY_DBL2MOUSE1+4, "DBLSEC_MOUSE5"},
	{KEY_DBL2MOUSE1+5, "DBLSEC_MOUSE6"},
	{KEY_DBL2MOUSE1+6, "DBLSEC_MOUSE7"},
	{KEY_DBL2MOUSE1+7, "DBLSEC_MOUSE8"},

	{KEY_DBLJOY1+0, "DBLJOY1"},
	{KEY_DBLJOY1+1, "DBLJOY2"},
	{KEY_DBLJOY1+2, "DBLJOY3"},
	{KEY_DBLJOY1+3, "DBLJOY4"},
	{KEY_DBLJOY1+4, "DBLJOY5"},
	{KEY_DBLJOY1+5, "DBLJOY6"},
	{KEY_DBLJOY1+6, "DBLJOY7"},
	{KEY_DBLJOY1+7, "DBLJOY8"},
#if !defined (NOMOREJOYBTN_1DBL)
	{KEY_DBLJOY1+8, "DBLJOY9"},
	{KEY_DBLJOY1+9, "DBLJOY10"},
	{KEY_DBLJOY1+10, "DBLJOY11"},
	{KEY_DBLJOY1+11, "DBLJOY12"},
	{KEY_DBLJOY1+12, "DBLJOY13"},
	{KEY_DBLJOY1+13, "DBLJOY14"},
	{KEY_DBLJOY1+14, "DBLJOY15"},
	{KEY_DBLJOY1+15, "DBLJOY16"},
	{KEY_DBLJOY1+16, "DBLJOY17"},
	{KEY_DBLJOY1+17, "DBLJOY18"},
	{KEY_DBLJOY1+18, "DBLJOY19"},
	{KEY_DBLJOY1+19, "DBLJOY20"},
	{KEY_DBLJOY1+20, "DBLJOY21"},
	{KEY_DBLJOY1+21, "DBLJOY22"},
	{KEY_DBLJOY1+22, "DBLJOY23"},
	{KEY_DBLJOY1+23, "DBLJOY24"},
	{KEY_DBLJOY1+24, "DBLJOY25"},
	{KEY_DBLJOY1+25, "DBLJOY26"},
	{KEY_DBLJOY1+26, "DBLJOY27"},
	{KEY_DBLJOY1+27, "DBLJOY28"},
	{KEY_DBLJOY1+28, "DBLJOY29"},
	{KEY_DBLJOY1+29, "DBLJOY30"},
	{KEY_DBLJOY1+30, "DBLJOY31"},
	{KEY_DBLJOY1+31, "DBLJOY32"},
#endif
	{KEY_DBLHAT1+0, "DBLHATUP"},
	{KEY_DBLHAT1+1, "DBLHATDOWN"},
	{KEY_DBLHAT1+2, "DBLHATLEFT"},
	{KEY_DBLHAT1+3, "DBLHATRIGHT"},
	{KEY_DBLHAT1+4, "DBLHATUP2"},
	{KEY_DBLHAT1+5, "DBLHATDOWN2"},
	{KEY_DBLHAT1+6, "DBLHATLEFT2"},
	{KEY_DBLHAT1+7, "DBLHATRIGHT2"},
	{KEY_DBLHAT1+8, "DBLHATUP3"},
	{KEY_DBLHAT1+9, "DBLHATDOWN3"},
	{KEY_DBLHAT1+10, "DBLHATLEFT3"},
	{KEY_DBLHAT1+11, "DBLHATRIGHT3"},
	{KEY_DBLHAT1+12, "DBLHATUP4"},
	{KEY_DBLHAT1+13, "DBLHATDOWN4"},
	{KEY_DBLHAT1+14, "DBLHATLEFT4"},
	{KEY_DBLHAT1+15, "DBLHATRIGHT4"},

	{KEY_2JOY1+0, "SEC_JOY1"},
	{KEY_2JOY1+1, "SEC_JOY2"},
	{KEY_2JOY1+2, "SEC_JOY3"},
	{KEY_2JOY1+3, "SEC_JOY4"},
	{KEY_2JOY1+4, "SEC_JOY5"},
	{KEY_2JOY1+5, "SEC_JOY6"},
	{KEY_2JOY1+6, "SEC_JOY7"},
	{KEY_2JOY1+7, "SEC_JOY8"},
#if !defined (NOMOREJOYBTN_2S)
	// we use up to 32 buttons in DirectInput
	{KEY_2JOY1+8, "SEC_JOY9"},
	{KEY_2JOY1+9, "SEC_JOY10"},
	{KEY_2JOY1+10, "SEC_JOY11"},
	{KEY_2JOY1+11, "SEC_JOY12"},
	{KEY_2JOY1+12, "SEC_JOY13"},
	{KEY_2JOY1+13, "SEC_JOY14"},
	{KEY_2JOY1+14, "SEC_JOY15"},
	{KEY_2JOY1+15, "SEC_JOY16"},
	{KEY_2JOY1+16, "SEC_JOY17"},
	{KEY_2JOY1+17, "SEC_JOY18"},
	{KEY_2JOY1+18, "SEC_JOY19"},
	{KEY_2JOY1+19, "SEC_JOY20"},
	{KEY_2JOY1+20, "SEC_JOY21"},
	{KEY_2JOY1+21, "SEC_JOY22"},
	{KEY_2JOY1+22, "SEC_JOY23"},
	{KEY_2JOY1+23, "SEC_JOY24"},
	{KEY_2JOY1+24, "SEC_JOY25"},
	{KEY_2JOY1+25, "SEC_JOY26"},
	{KEY_2JOY1+26, "SEC_JOY27"},
	{KEY_2JOY1+27, "SEC_JOY28"},
	{KEY_2JOY1+28, "SEC_JOY29"},
	{KEY_2JOY1+29, "SEC_JOY30"},
	{KEY_2JOY1+30, "SEC_JOY31"},
	{KEY_2JOY1+31, "SEC_JOY32"},
#endif
	// the DOS version uses Allegro's joystick support
	{KEY_2HAT1+0,  "SEC_HATUP"},
	{KEY_2HAT1+1,  "SEC_HATDOWN"},
	{KEY_2HAT1+2,  "SEC_HATLEFT"},
	{KEY_2HAT1+3,  "SEC_HATRIGHT"},
	{KEY_2HAT1+4, "SEC_HATUP2"},
	{KEY_2HAT1+5, "SEC_HATDOWN2"},
	{KEY_2HAT1+6, "SEC_HATLEFT2"},
	{KEY_2HAT1+7, "SEC_HATRIGHT2"},
	{KEY_2HAT1+8, "SEC_HATUP3"},
	{KEY_2HAT1+9, "SEC_HATDOWN3"},
	{KEY_2HAT1+10, "SEC_HATLEFT3"},
	{KEY_2HAT1+11, "SEC_HATRIGHT3"},
	{KEY_2HAT1+12, "SEC_HATUP4"},
	{KEY_2HAT1+13, "SEC_HATDOWN4"},
	{KEY_2HAT1+14, "SEC_HATLEFT4"},
	{KEY_2HAT1+15, "SEC_HATRIGHT4"},

	{KEY_DBL2JOY1+0, "DBLSEC_JOY1"},
	{KEY_DBL2JOY1+1, "DBLSEC_JOY2"},
	{KEY_DBL2JOY1+2, "DBLSEC_JOY3"},
	{KEY_DBL2JOY1+3, "DBLSEC_JOY4"},
	{KEY_DBL2JOY1+4, "DBLSEC_JOY5"},
	{KEY_DBL2JOY1+5, "DBLSEC_JOY6"},
	{KEY_DBL2JOY1+6, "DBLSEC_JOY7"},
	{KEY_DBL2JOY1+7, "DBLSEC_JOY8"},
#if !defined (NOMOREJOYBTN_2DBL)
	{KEY_DBL2JOY1+8, "DBLSEC_JOY9"},
	{KEY_DBL2JOY1+9, "DBLSEC_JOY10"},
	{KEY_DBL2JOY1+10, "DBLSEC_JOY11"},
	{KEY_DBL2JOY1+11, "DBLSEC_JOY12"},
	{KEY_DBL2JOY1+12, "DBLSEC_JOY13"},
	{KEY_DBL2JOY1+13, "DBLSEC_JOY14"},
	{KEY_DBL2JOY1+14, "DBLSEC_JOY15"},
	{KEY_DBL2JOY1+15, "DBLSEC_JOY16"},
	{KEY_DBL2JOY1+16, "DBLSEC_JOY17"},
	{KEY_DBL2JOY1+17, "DBLSEC_JOY18"},
	{KEY_DBL2JOY1+18, "DBLSEC_JOY19"},
	{KEY_DBL2JOY1+19, "DBLSEC_JOY20"},
	{KEY_DBL2JOY1+20, "DBLSEC_JOY21"},
	{KEY_DBL2JOY1+21, "DBLSEC_JOY22"},
	{KEY_DBL2JOY1+22, "DBLSEC_JOY23"},
	{KEY_DBL2JOY1+23, "DBLSEC_JOY24"},
	{KEY_DBL2JOY1+24, "DBLSEC_JOY25"},
	{KEY_DBL2JOY1+25, "DBLSEC_JOY26"},
	{KEY_DBL2JOY1+26, "DBLSEC_JOY27"},
	{KEY_DBL2JOY1+27, "DBLSEC_JOY28"},
	{KEY_DBL2JOY1+28, "DBLSEC_JOY29"},
	{KEY_DBL2JOY1+29, "DBLSEC_JOY30"},
	{KEY_DBL2JOY1+30, "DBLSEC_JOY31"},
	{KEY_DBL2JOY1+31, "DBLSEC_JOY32"},
#endif
	{KEY_DBL2HAT1+0, "DBLSEC_HATUP"},
	{KEY_DBL2HAT1+1, "DBLSEC_HATDOWN"},
	{KEY_DBL2HAT1+2, "DBLSEC_HATLEFT"},
	{KEY_DBL2HAT1+3, "DBLSEC_HATRIGHT"},
	{KEY_DBL2HAT1+4, "DBLSEC_HATUP2"},
	{KEY_DBL2HAT1+5, "DBLSEC_HATDOWN2"},
	{KEY_DBL2HAT1+6, "DBLSEC_HATLEFT2"},
	{KEY_DBL2HAT1+7, "DBLSEC_HATRIGHT2"},
	{KEY_DBL2HAT1+8, "DBLSEC_HATUP3"},
	{KEY_DBL2HAT1+9, "DBLSEC_HATDOWN3"},
	{KEY_DBL2HAT1+10, "DBLSEC_HATLEFT3"},
	{KEY_DBL2HAT1+11, "DBLSEC_HATRIGHT3"},
	{KEY_DBL2HAT1+12, "DBLSEC_HATUP4"},
	{KEY_DBL2HAT1+13, "DBLSEC_HATDOWN4"},
	{KEY_DBL2HAT1+14, "DBLSEC_HATLEFT4"},
	{KEY_DBL2HAT1+15, "DBLSEC_HATRIGHT4"},

};

const char *gamecontrolname[num_gamecontrols] =
{
	"nothing", // a key/button mapped to gc_null has no effect
	"forward",
	"backward",
	"strafeleft",
	"straferight",
	"turnleft",
	"turnright",
#ifdef TOUCHINPUTS
	"touchjoystick",
	"dpadul",
	"dpadur",
	"dpaddl",
	"dpaddr",
#endif
	"weaponnext",
	"weaponprev",
	"weapon1",
	"weapon2",
	"weapon3",
	"weapon4",
	"weapon5",
	"weapon6",
	"weapon7",
	"weapon8",
	"weapon9",
	"weapon10",
	"fire",
	"firenormal",
	"tossflag",
	"use",
	"camtoggle",
	"camreset",
	"lookup",
	"lookdown",
	"centerview",
	"mouseaiming",
	"talkkey",
	"teamtalkkey",
	"scores",
	"jump",
	"console",
	"pause",
	"systemmenu",
	"screenshot",
	"recordgif",
	"viewpoint",
	"custom1",
	"custom2",
	"custom3",
};

#define NUMKEYNAMES (sizeof (keynames)/sizeof (keyname_t))

//
// Detach any keys associated to the given game control
// - pass the pointer to the gamecontrol table for the player being edited
void G_ClearControlKeys(INT32 (*setupcontrols)[2], INT32 control)
{
	setupcontrols[control][0] = KEY_NULL;
	setupcontrols[control][1] = KEY_NULL;
}

void G_ClearAllControlKeys(void)
{
	INT32 i;
	for (i = 0; i < num_gamecontrols; i++)
	{
		G_ClearControlKeys(gamecontrol, i);
		G_ClearControlKeys(gamecontrolbis, i);
	}
}

//
// Returns the name of a key (or virtual key for mouse and joy)
// the input value being an keynum
//
const char *G_KeynumToString(INT32 keynum)
{
	static char keynamestr[8];

	UINT32 j;

	// return a string with the ascii char if displayable
	if (keynum > ' ' && keynum <= 'z' && keynum != KEY_CONSOLE)
	{
		keynamestr[0] = (char)keynum;
		keynamestr[1] = '\0';
		return keynamestr;
	}

	// find a description for special keys
	for (j = 0; j < NUMKEYNAMES; j++)
		if (keynames[j].keynum == keynum)
			return keynames[j].name;

	// create a name for unknown keys
	sprintf(keynamestr, "KEY%d", keynum);
	return keynamestr;
}

INT32 G_KeyStringtoNum(const char *keystr)
{
	UINT32 j;

	if (!keystr[1] && keystr[0] > ' ' && keystr[0] <= 'z')
		return keystr[0];

	if (!strncmp(keystr, "KEY", 3) && keystr[3] >= '0' && keystr[3] <= '9')
	{
		/* what if we out of range bruh? */
		j = atoi(&keystr[3]);
		if (j < NUMINPUTS)
			return j;
		return 0;
	}

	for (j = 0; j < NUMKEYNAMES; j++)
		if (!stricmp(keynames[j].name, keystr))
			return keynames[j].keynum;

	return 0;
}

void G_DefineDefaultControls(void)
{
	INT32 i;

	// FPS game controls (WASD)
	gamecontroldefault[gcs_fps][gc_forward    ][0] = 'w';
	gamecontroldefault[gcs_fps][gc_backward   ][0] = 's';
	gamecontroldefault[gcs_fps][gc_strafeleft ][0] = 'a';
	gamecontroldefault[gcs_fps][gc_straferight][0] = 'd';
	gamecontroldefault[gcs_fps][gc_lookup     ][0] = KEY_UPARROW;
	gamecontroldefault[gcs_fps][gc_lookdown   ][0] = KEY_DOWNARROW;
	gamecontroldefault[gcs_fps][gc_turnleft   ][0] = KEY_LEFTARROW;
	gamecontroldefault[gcs_fps][gc_turnright  ][0] = KEY_RIGHTARROW;
	gamecontroldefault[gcs_fps][gc_centerview ][0] = KEY_END;
	gamecontroldefault[gcs_fps][gc_jump       ][0] = KEY_SPACE;
	gamecontroldefault[gcs_fps][gc_use        ][0] = KEY_LSHIFT;
	gamecontroldefault[gcs_fps][gc_fire       ][0] = KEY_RCTRL;
	gamecontroldefault[gcs_fps][gc_fire       ][1] = KEY_MOUSE1+0;
	gamecontroldefault[gcs_fps][gc_firenormal ][0] = 'c';

	// Platform game controls (arrow keys)
	gamecontroldefault[gcs_platform][gc_forward    ][0] = KEY_UPARROW;
	gamecontroldefault[gcs_platform][gc_backward   ][0] = KEY_DOWNARROW;
	gamecontroldefault[gcs_platform][gc_strafeleft ][0] = 'a';
	gamecontroldefault[gcs_platform][gc_straferight][0] = 'd';
	gamecontroldefault[gcs_platform][gc_lookup     ][0] = KEY_PGUP;
	gamecontroldefault[gcs_platform][gc_lookdown   ][0] = KEY_PGDN;
	gamecontroldefault[gcs_platform][gc_turnleft   ][0] = KEY_LEFTARROW;
	gamecontroldefault[gcs_platform][gc_turnright  ][0] = KEY_RIGHTARROW;
	gamecontroldefault[gcs_platform][gc_centerview ][0] = KEY_END;
	gamecontroldefault[gcs_platform][gc_jump       ][0] = KEY_SPACE;
	gamecontroldefault[gcs_platform][gc_use        ][0] = KEY_LSHIFT;
	gamecontroldefault[gcs_platform][gc_fire       ][0] = 's';
	gamecontroldefault[gcs_platform][gc_fire       ][1] = KEY_MOUSE1+0;
	gamecontroldefault[gcs_platform][gc_firenormal ][0] = 'w';

	for (i = 1; i < num_gamecontrolschemes; i++) // skip gcs_custom (0)
	{
		gamecontroldefault[i][gc_weaponnext ][0] = KEY_MOUSEWHEELUP+0;
		gamecontroldefault[i][gc_weaponprev ][0] = KEY_MOUSEWHEELDOWN+0;
		gamecontroldefault[i][gc_wepslot1   ][0] = '1';
		gamecontroldefault[i][gc_wepslot2   ][0] = '2';
		gamecontroldefault[i][gc_wepslot3   ][0] = '3';
		gamecontroldefault[i][gc_wepslot4   ][0] = '4';
		gamecontroldefault[i][gc_wepslot5   ][0] = '5';
		gamecontroldefault[i][gc_wepslot6   ][0] = '6';
		gamecontroldefault[i][gc_wepslot7   ][0] = '7';
		gamecontroldefault[i][gc_wepslot8   ][0] = '8';
		gamecontroldefault[i][gc_wepslot9   ][0] = '9';
		gamecontroldefault[i][gc_wepslot10  ][0] = '0';
		gamecontroldefault[i][gc_tossflag   ][0] = '\'';
		gamecontroldefault[i][gc_camtoggle  ][0] = 'v';
		gamecontroldefault[i][gc_camreset   ][0] = 'r';
		gamecontroldefault[i][gc_talkkey    ][0] = 't';
		gamecontroldefault[i][gc_teamkey    ][0] = 'y';
		gamecontroldefault[i][gc_scores     ][0] = KEY_TAB;
		gamecontroldefault[i][gc_console    ][0] = KEY_CONSOLE;
		gamecontroldefault[i][gc_pause      ][0] = 'p';
		gamecontroldefault[i][gc_screenshot ][0] = KEY_F8;
		gamecontroldefault[i][gc_recordgif  ][0] = KEY_F9;
		gamecontroldefault[i][gc_viewpoint  ][0] = KEY_F12;

		// Gamepad controls -- same for both schemes
		gamecontroldefault[i][gc_weaponnext ][1] = KEY_JOY1+1; // B
		gamecontroldefault[i][gc_weaponprev ][1] = KEY_JOY1+2; // X
		gamecontroldefault[i][gc_tossflag   ][1] = KEY_JOY1+0; // A
		gamecontroldefault[i][gc_use        ][1] = KEY_JOY1+4; // LB
		gamecontroldefault[i][gc_camtoggle  ][1] = KEY_HAT1+0; // D-Pad Up
		gamecontroldefault[i][gc_camreset   ][1] = KEY_JOY1+3; // Y
		gamecontroldefault[i][gc_centerview ][1] = KEY_JOY1+9; // Right Stick
		gamecontroldefault[i][gc_talkkey    ][1] = KEY_HAT1+2; // D-Pad Left
		gamecontroldefault[i][gc_scores     ][1] = KEY_HAT1+3; // D-Pad Right
		gamecontroldefault[i][gc_jump       ][1] = KEY_JOY1+5; // RB
		gamecontroldefault[i][gc_pause      ][1] = KEY_JOY1+6; // Back
		gamecontroldefault[i][gc_screenshot ][1] = KEY_HAT1+1; // D-Pad Down
		gamecontroldefault[i][gc_systemmenu ][0] = KEY_JOY1+7; // Start

		// Second player controls only have joypad defaults
		gamecontrolbisdefault[i][gc_weaponnext][0] = KEY_2JOY1+1; // B
		gamecontrolbisdefault[i][gc_weaponprev][0] = KEY_2JOY1+2; // X
		gamecontrolbisdefault[i][gc_tossflag  ][0] = KEY_2JOY1+0; // A
		gamecontrolbisdefault[i][gc_use       ][0] = KEY_2JOY1+4; // LB
		gamecontrolbisdefault[i][gc_camreset  ][0] = KEY_2JOY1+3; // Y
		gamecontrolbisdefault[i][gc_centerview][0] = KEY_2JOY1+9; // Right Stick
		gamecontrolbisdefault[i][gc_jump      ][0] = KEY_2JOY1+5; // RB
		//gamecontrolbisdefault[i][gc_pause     ][0] = KEY_2JOY1+6; // Back
		//gamecontrolbisdefault[i][gc_systemmenu][0] = KEY_2JOY1+7; // Start
		gamecontrolbisdefault[i][gc_camtoggle ][0] = KEY_2HAT1+0; // D-Pad Up
		gamecontrolbisdefault[i][gc_screenshot][0] = KEY_2HAT1+1; // D-Pad Down
		//gamecontrolbisdefault[i][gc_talkkey   ][0] = KEY_2HAT1+2; // D-Pad Left
		//gamecontrolbisdefault[i][gc_scores    ][0] = KEY_2HAT1+3; // D-Pad Right
	}

#ifdef TOUCHINPUTS
	TS_RegisterVariables();
#endif
}

// Lactozilla: Touch input
#ifdef TOUCHINPUTS
void G_SetupTouchSettings(void)
{
	if (!TS_Ready())
		return;

	touch_movementstyle = cv_touchstyle.value;
	touch_camera = (cv_usemouse.value ? false : (!!cv_touchcamera.value));
	touch_preset = cv_touchpreset.value;
	touch_gui_scale = cv_touchguiscale.value;
}

#define SCALECOORD(coord) FixedMul(coord, scale)

void G_UpdateTouchControls(void)
{
	G_SetupTouchSettings();
	G_DefineTouchButtons();
}

void G_DPadPreset(touchconfig_t *controls, fixed_t xscale, fixed_t yscale, fixed_t dw, boolean tiny)
{
	fixed_t w, h;
	fixed_t scaledw, scaledh;
	fixed_t scaledxoffs = FixedMul(16*FRACUNIT, xscale);
	fixed_t scaledyoffs = FixedMul(14*FRACUNIT, yscale);
	fixed_t diagxoffs = FixedMul(48*FRACUNIT, xscale);
	fixed_t diagyoffs = FixedMul(16*FRACUNIT, yscale);
	fixed_t middlealign = xscale;

	if (tiny)
	{
		w = 16 * FRACUNIT;
		h = 16 * FRACUNIT;
		middlealign = 0;
	}
	else
	{
		w = 32 * FRACUNIT;
		h = 32 * FRACUNIT;
		scaledxoffs *= 2;
		scaledyoffs *= 2;
	}

	scaledw = FixedMul(w, xscale);
	scaledh = FixedMul(h, yscale);

	// Up
	controls[gc_forward].w = scaledw;
	controls[gc_forward].h = scaledh;
	controls[gc_forward].x = (touch_joystick_x + (dw / 2)) - FixedFloor(FixedMul(w, xscale) / 2) + middlealign;
	controls[gc_forward].y = touch_joystick_y - scaledyoffs;

	// Down
	controls[gc_backward].w = scaledw;
	controls[gc_backward].h = scaledh;
	controls[gc_backward].x = (touch_joystick_x + (dw / 2)) - FixedFloor(FixedMul(w, xscale) / 2) + middlealign;
	controls[gc_backward].y = ((touch_joystick_y + touch_joystick_h) - controls[gc_backward].h) + scaledyoffs;

	// Left
	controls[gc_strafeleft].w = scaledw;
	controls[gc_strafeleft].h = scaledh;
	controls[gc_strafeleft].x = touch_joystick_x - scaledxoffs;
	controls[gc_strafeleft].y = (touch_joystick_y + (touch_joystick_h / 2)) - (controls[gc_strafeleft].h / 2);

	// Right
	controls[gc_straferight].w = scaledw;
	controls[gc_straferight].h = scaledh;
	controls[gc_straferight].x = ((touch_joystick_x + touch_joystick_w) - controls[gc_straferight].w) + scaledxoffs;
	controls[gc_straferight].y = controls[gc_strafeleft].y;

	// Up left
	controls[gc_dpadul].w = scaledw;
	controls[gc_dpadul].h = scaledh;
	controls[gc_dpadul].x = controls[gc_forward].x - diagxoffs;
	controls[gc_dpadul].y = controls[gc_forward].y + diagyoffs;

	// Up right
	controls[gc_dpadur].w = scaledw;
	controls[gc_dpadur].h = scaledh;
	controls[gc_dpadur].x = controls[gc_forward].x + diagxoffs;
	controls[gc_dpadur].y = controls[gc_forward].y + diagyoffs;

	// Down left
	controls[gc_dpaddl].w = scaledw;
	controls[gc_dpaddl].h = scaledh;
	controls[gc_dpaddl].x = controls[gc_backward].x - diagxoffs;
	controls[gc_dpaddl].y = controls[gc_backward].y - diagyoffs;

	// Down right
	controls[gc_dpaddr].w = scaledw;
	controls[gc_dpaddr].h = scaledh;
	controls[gc_dpaddr].x = controls[gc_backward].x + diagxoffs;
	controls[gc_dpaddr].y = controls[gc_backward].y - diagyoffs;
}

static void G_ScaleDPadBase(touchmovementstyle_e tms, boolean ringslinger, boolean tiny, fixed_t dx, fixed_t dy, fixed_t dw, fixed_t dh, fixed_t scale, fixed_t offs, fixed_t bottomalign)
{
	touch_joystick_w = SCALECOORD(dw);
	touch_joystick_h = SCALECOORD(dh);
	touch_joystick_x = max(dx, ((dx + FixedDiv(dw, FRACUNIT * 2)) - FixedDiv(touch_joystick_w, FRACUNIT * 2)));

	if (scale < FRACUNIT)
		touch_joystick_y = ((dy + FixedDiv(dh, FRACUNIT * 2)) - FixedDiv(touch_joystick_h, FRACUNIT * 2));
	else
		touch_joystick_y = ((dy + dh) - touch_joystick_h);
	touch_joystick_y += (offs + bottomalign);

	// Offset joystick / d-pad base
	if (tiny)
	{
		if (tms == tms_joystick)
		{
			touch_joystick_x -= 4 * FRACUNIT;
			touch_joystick_y += 8 * FRACUNIT;
		}

		if (ringslinger)
			touch_joystick_y -= 4 * FRACUNIT;
	}
	else
	{
		if (tms == tms_joystick)
		{
			touch_joystick_x -= 12 * FRACUNIT;
			touch_joystick_y += 16 * FRACUNIT;
		}

		if (ringslinger)
			touch_joystick_y -= 8 * FRACUNIT;
	}
}

void G_NormalizeTouchButton(touchconfig_t *button)
{
	button->x = FixedDiv(button->x, BASEVIDWIDTH * FRACUNIT);
	button->y = FixedDiv(button->y, BASEVIDHEIGHT * FRACUNIT);
}

void G_NormalizeTouchConfig(touchconfig_t *config, int configsize)
{
	INT32 i;
	for (i = 0; i < configsize; i++)
		G_NormalizeTouchButton(&config[i]);
}

void G_DenormalizeCoords(fixed_t *x, fixed_t *y)
{
	*x *= BASEVIDWIDTH;
	*y *= BASEVIDHEIGHT;
}

void G_CenterCoords(fixed_t *x, fixed_t *y)
{
	if (!G_TouchPresetActive())
	{
		INT32 dup = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
		if (vid.width != BASEVIDWIDTH * dup)
			*x += ((vid.width - (BASEVIDWIDTH * dup)) / 2) * FRACUNIT;
		if (vid.height != BASEVIDHEIGHT * dup)
			*y += ((vid.height - (BASEVIDHEIGHT * dup)) / 2) * FRACUNIT;
	}
}

void G_CenterIntegerCoords(INT32 *x, INT32 *y)
{
	if (!G_TouchPresetActive())
	{
		INT32 dup = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
		if (vid.width != BASEVIDWIDTH * dup)
			*x += (vid.width - (BASEVIDWIDTH * dup)) / 2;
		if (vid.height != BASEVIDHEIGHT * dup)
			*y += (vid.height - (BASEVIDHEIGHT * dup)) / 2;
	}
}

struct {
	const char *name;
	const char *tinyname;
} const touchbuttonnames[num_gamecontrols] = {
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{"WEP.NEXT", "WNX"},
	{"WEP.PREV", "WPV"},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{"FIRE", "FRE"},
	{"F.NORMAL", "FRN"},
	{"TOSSFLAG", "FLG"},
	{"SPIN", "SPN"},
	{"CHASECAM", "CHASE"},
	{"RESET CAM", "R.CAM"},
	{"LOOK UP", "L.UP"},
	{"LOOK DOWN", "L.DW"},
	{"CENTER VIEW", "CVW"},
	{"MOUSEAIM", "AIM"},
	{"TALK", "TLK"},
	{"TEAM", "TTK"},
	{"SCORES", "TAB"},
	{"JUMP", "JMP"},
	{"CONSOLE", "CON"},
	{NULL, NULL},
	{"MENU", "MNU"},
	{"SCRCAP", "SCR"},
	{"REC", NULL},
	{"F12", NULL},
	{"CUSTOM1", "C1"},
	{"CUSTOM2", "C2"},
	{"CUSTOM3", "C3"},
};

const char *G_GetTouchButtonName(INT32 gc)
{
	return touchbuttonnames[gc].name;
}

const char *G_GetTouchButtonShortName(INT32 gc)
{
	return touchbuttonnames[gc].tinyname;
}

void G_SetTouchButtonNames(touchconfig_t *controls)
{
	INT32 i;

	for (i = 0; i < num_gamecontrols; i++)
	{
		controls[i].name = G_GetTouchButtonName(i);
		controls[i].tinyname = G_GetTouchButtonShortName(i);
	}
}

boolean G_IsDPadButton(INT32 gc)
{
	switch (gc)
	{
		case gc_forward:
		case gc_backward:
		case gc_strafeleft:
		case gc_straferight:
		case gc_dpadul:
		case gc_dpadur:
		case gc_dpaddl:
		case gc_dpaddr:
			return true;
		default:
			break;
	}

	return false;
}

void G_MarkDPadButtons(touchconfig_t *controls)
{
	INT32 i;

	for (i = 0; i < num_gamecontrols; i++)
	{
		if (G_IsDPadButton(i))
			controls[i].dpad = true;
	}
}

void G_BuildTouchPreset(touchconfig_t *controls, touchconfigstatus_t *status, touchmovementstyle_e tms, fixed_t scale, boolean tiny)
{
	fixed_t x, y, w, h;
	fixed_t dx, dy, dw, dh;
	fixed_t corneroffset = 4 * FRACUNIT;
	fixed_t rightcorner = ((vid.width / vid.dupx) * FRACUNIT);
	fixed_t bottomcorner = ((vid.height / vid.dupy) * FRACUNIT);
	fixed_t jsoffs = status->ringslinger ? (-4 * FRACUNIT) : 0, jumph;
	fixed_t offs = (promptactive ? -16 : 0);
	fixed_t nonjoyoffs = -12 * FRACUNIT;
	fixed_t bottomalign = 0;

	touchconfig_t *ref;
	boolean bothvisible;
	boolean eithervisible;

	// For the D-Pad
	if (vid.height != BASEVIDHEIGHT * vid.dupy)
		bottomalign = ((vid.height - (BASEVIDHEIGHT * vid.dupy)) / vid.dupy) * FRACUNIT;

	TS_GetJoystick(&dx, &dy, &dw, &dh, tiny);

	// D-Pad
	G_ScaleDPadBase(tms, status->ringslinger, tiny, dx, dy, dw, dh, scale, offs, bottomalign);
	G_DPadPreset(
		controls,
		FixedMul(FixedDiv(touch_joystick_w, dw), scale),
		FixedMul(FixedDiv(touch_joystick_h, dh), scale),
		touch_joystick_w, tiny);

	controls[gc_joystick].x = touch_joystick_x;
	controls[gc_joystick].y = touch_joystick_y;
	controls[gc_joystick].w = touch_joystick_w;
	controls[gc_joystick].h = touch_joystick_h;

	// Jump and spin
	if (tiny)
	{
		// Jump
		w = 40 * FRACUNIT;
		h = jumph = 32 * FRACUNIT;
		controls[gc_jump].w = SCALECOORD(w);
		controls[gc_jump].h = SCALECOORD(h);
		controls[gc_jump].x = (rightcorner - controls[gc_jump].w - corneroffset - (12 * FRACUNIT));
		controls[gc_jump].y = (bottomcorner - controls[gc_jump].h - corneroffset - (12 * FRACUNIT)) + jsoffs + offs + nonjoyoffs;

		// Spin
		w = 32 * FRACUNIT;
		h = 24 * FRACUNIT;
		controls[gc_use].w = SCALECOORD(w);
		controls[gc_use].h = SCALECOORD(h);
		controls[gc_use].x = (controls[gc_jump].x - controls[gc_use].w - (12 * FRACUNIT));
		controls[gc_use].y = controls[gc_jump].y + (8 * FRACUNIT);
	}
	else
	{
		// Jump
		w = 48 * FRACUNIT;
		h = jumph = 48 * FRACUNIT;
		controls[gc_jump].w = SCALECOORD(w);
		controls[gc_jump].h = SCALECOORD(h);
		controls[gc_jump].x = (rightcorner - controls[gc_jump].w - corneroffset - (12 * FRACUNIT));
		controls[gc_jump].y = (bottomcorner - controls[gc_jump].h - corneroffset - (12 * FRACUNIT)) + jsoffs + offs + nonjoyoffs;

		// Spin
		w = 40 * FRACUNIT;
		h = 32 * FRACUNIT;
		controls[gc_use].w = SCALECOORD(w);
		controls[gc_use].h = SCALECOORD(h);
		controls[gc_use].x = (controls[gc_jump].x - controls[gc_use].w - (12 * FRACUNIT));
		controls[gc_use].y = controls[gc_jump].y + (12 * FRACUNIT);
	}

	// Fire, fire normal, and toss flag
	if (status->ringslinger)
	{
		offs = SCALECOORD(8 * FRACUNIT);
		h = SCALECOORD(jumph / 2) + SCALECOORD(4 * FRACUNIT);
		controls[gc_use].h = h;
		controls[gc_use].y = ((controls[gc_jump].y + controls[gc_jump].h) - h) + offs;

		controls[gc_fire].w = controls[gc_use].w;
		controls[gc_fire].h = h;
		controls[gc_fire].x = controls[gc_use].x;
		controls[gc_fire].y = controls[gc_jump].y - offs;

		if (status->ctfgametype)
		{
			ref = &controls[gc_tossflag];
			controls[gc_firenormal].hidden = true;
		}
		else
		{
			ref = &controls[gc_firenormal];
			controls[gc_tossflag].hidden = true;
		}

		ref->w = controls[gc_jump].w;
		ref->h = controls[gc_fire].h;
		if (!tiny)
			ref->h = (ref->h / 2);

		ref->x = controls[gc_jump].x;
		ref->y = controls[gc_jump].y - ref->h - SCALECOORD(4 * FRACUNIT);
	}
	else
	{
		controls[gc_fire].hidden = true;
		controls[gc_firenormal].hidden = true;
		controls[gc_tossflag].hidden = true;
	}

	//
	// Non-control buttons
	//

	offs = SCALECOORD(8 * FRACUNIT);

	// Menu
	controls[gc_systemmenu].w = SCALECOORD(32 * FRACUNIT);
	controls[gc_systemmenu].h = SCALECOORD(32 * FRACUNIT);
	controls[gc_systemmenu].x = (rightcorner - controls[gc_systemmenu].w - corneroffset);
	controls[gc_systemmenu].y = corneroffset;

	// Pause
	controls[gc_pause].x = controls[gc_systemmenu].x;
	controls[gc_pause].y = controls[gc_systemmenu].y;
	controls[gc_pause].w = SCALECOORD(24 * FRACUNIT);
	controls[gc_pause].h = SCALECOORD(24 * FRACUNIT);
	if (status->canpause)
		controls[gc_pause].x -= (controls[gc_pause].w + SCALECOORD(4 * FRACUNIT));
	else
		controls[gc_pause].hidden = true;

	// Spy mode
	controls[gc_viewpoint].hidden = true;
	controls[gc_viewpoint].x = controls[gc_pause].x;
	controls[gc_viewpoint].y = controls[gc_pause].y;
	if (status->canviewpointswitch)
	{
		controls[gc_viewpoint].w = SCALECOORD(32 * FRACUNIT);
		controls[gc_viewpoint].h = SCALECOORD(24 * FRACUNIT);
		controls[gc_viewpoint].x -= (controls[gc_viewpoint].w + SCALECOORD(4 * FRACUNIT));
		controls[gc_viewpoint].hidden = false;
	}

	// Align screenshot and movie mode buttons
	w = SCALECOORD(40 * FRACUNIT);
	h = SCALECOORD(24 * FRACUNIT);

	bothvisible = ((!controls[gc_viewpoint].hidden) && (!controls[gc_pause].hidden));
	eithervisible = (controls[gc_viewpoint].hidden ^ controls[gc_pause].hidden);

	if (bothvisible || eithervisible)
	{
		if (bothvisible)
			ref = &controls[gc_pause];
		else // only if one of either are visible, but not both
			ref = (controls[gc_viewpoint].hidden ? &controls[gc_pause] : &controls[gc_viewpoint]);

		x = ref->x - (w - ref->w);
		y = ref->y + ref->h + offs;
	}
	else
	{
		x = (controls[gc_viewpoint].x - w - SCALECOORD(4 * FRACUNIT));
		y = controls[gc_viewpoint].y;
	}

	controls[gc_screenshot].w = controls[gc_recordgif].w = w;
	controls[gc_screenshot].h = controls[gc_recordgif].h = h;

	// Screenshot
	controls[gc_screenshot].x = x;
	controls[gc_screenshot].y = y;

	// Movie mode
	controls[gc_recordgif].x = x;
	controls[gc_recordgif].y = (controls[gc_screenshot].y + controls[gc_screenshot].h + offs);

	// Talk key and team talk key
	controls[gc_talkkey].hidden = true; // hidden by default
	controls[gc_teamkey].hidden = true; // hidden outside of team games

	// if netgame + chat not muted
	if (status->cantalk)
	{
		controls[gc_talkkey].w = SCALECOORD(32 * FRACUNIT);
		controls[gc_talkkey].h = SCALECOORD(24 * FRACUNIT);
		controls[gc_talkkey].x = (rightcorner - controls[gc_talkkey].w - corneroffset);
		controls[gc_talkkey].y = (controls[gc_systemmenu].y + controls[gc_systemmenu].h + offs);
		controls[gc_talkkey].hidden = false;

		if (status->canteamtalk)
		{
			controls[gc_teamkey].w = SCALECOORD(32 * FRACUNIT);
			controls[gc_teamkey].h = SCALECOORD(24 * FRACUNIT);
			controls[gc_teamkey].x = controls[gc_talkkey].x;
			controls[gc_teamkey].y = controls[gc_talkkey].y + controls[gc_talkkey].h + offs;
			controls[gc_teamkey].hidden = false;
		}
	}

	// Normalize all buttons
	G_NormalizeTouchConfig(controls, num_gamecontrols);

	// Mark movement controls as d-pad buttons
	G_MarkDPadButtons(controls);

	// Set button names
	G_SetTouchButtonNames(controls);

	// Mark undefined buttons as hidden
	for (x = 0; x < num_gamecontrols; x++)
	{
		if (!controls[x].w)
			controls[x].hidden = true;
	}
}

static void G_BuildWeaponButtons(touchconfigstatus_t *status)
{
	INT32 i, wep, dup = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
	fixed_t x, y, w, h;

	x = (ST_WEAPONS_X * FRACUNIT) + (6 * FRACUNIT);
	y = (ST_WEAPONS_Y * FRACUNIT) - (2 * FRACUNIT);
	w = ST_WEAPONS_W * FRACUNIT;
	h = ST_WEAPONS_H * FRACUNIT;

	wep = gc_wepslot1;

	for (i = 0; i < NUM_WEAPONS; i++)
	{
		fixed_t wepx = x + (w * i);
		fixed_t wepy = y;

		wepx = FixedMul(wepx, dup * FRACUNIT);
		wepy = FixedMul(wepy, dup * FRACUNIT);

		if (vid.height != BASEVIDHEIGHT * dup)
			wepy += (vid.height - (BASEVIDHEIGHT * dup)) * FRACUNIT;
		if (vid.width != BASEVIDWIDTH * dup)
			wepx += (vid.width - (BASEVIDWIDTH * dup)) * (FRACUNIT / 2);

		if (status->ringslinger)
		{
			touchcontrols[wep].x = wepx;
			touchcontrols[wep].y = wepy;
			touchcontrols[wep].w = FixedMul(w, dup * FRACUNIT);
			touchcontrols[wep].h = FixedMul(h, dup * FRACUNIT);
			touchcontrols[wep].dontscale = true;
		}

		touchcontrols[wep].hidden = (!status->ringslinger);

		G_NormalizeTouchButton(&touchcontrols[wep]);

		wep++;
	}
}

static void G_HidePlayerControlButtons(touchconfigstatus_t *status)
{
	if (status->promptblockcontrols)
	{
		INT32 i;
		for (i = 0; i < num_gamecontrols; i++)
		{
			if (G_TouchButtonIsPlayerControl(i))
				touchcontrols[i].hidden = true;
		}
	}
}

void G_PositionTouchButtons(void)
{
	touchconfigstatus_t *status = &touchcontrolstatus;

	// Build preset
	if (G_TouchPresetActive())
	{
		memset(touchcontrols, 0x00, sizeof(touchconfig_t) * num_gamecontrols);
		G_BuildTouchPreset(touchcontrols, status, touch_movementstyle, touch_gui_scale, (touch_preset == touchpreset_tiny));
	}

	// Weapon select slots
	G_BuildWeaponButtons(status);

	// Hide movement controls in prompts that block controls
	G_HidePlayerControlButtons(status);

	// Mark movement controls as d-pad buttons
	G_MarkDPadButtons(touchcontrols);
}

void G_PositionExtraUserTouchButtons(void)
{
	// Position joystick
	touch_joystick_x = touchcontrols[gc_joystick].x;
	touch_joystick_y = touchcontrols[gc_joystick].y;

	G_DenormalizeCoords(&touch_joystick_x, &touch_joystick_y);

	touch_joystick_w = touchcontrols[gc_joystick].w;
	touch_joystick_h = touchcontrols[gc_joystick].h;

	// Weapon select slots
	G_BuildWeaponButtons(&usertouchconfigstatus);

	// Hide movement controls in prompts that block controls
	G_HidePlayerControlButtons(&usertouchconfigstatus);

	// Mark movement controls as d-pad buttons
	G_MarkDPadButtons(usertouchcontrols);
}

void G_TouchPresetChanged(void)
{
	if (!TS_Ready())
		return;

	// set touch_preset
	G_SetupTouchSettings();

	if (!G_TouchPresetActive())
	{
		// Make a default custom controls set
		TS_DefaultControlLayout();

		// Copy custom controls
		M_Memcpy(&touchcontrols, usertouchcontrols, sizeof(touchconfig_t) * num_gamecontrols);

		// Position joystick, weapon buttons, etc.
		G_PositionExtraUserTouchButtons();

		// Mark movement controls as d-pad buttons
		G_MarkDPadButtons(touchcontrols);
	}
	else
		G_DefineTouchButtons();
}

#undef SCALECOORD

void G_PositionTouchNavigation(void)
{
	INT32 i;
	touchconfig_t *nav = touchnavigation;
	INT32 corneroffset = 4 * FRACUNIT;

	// clear all
	memset(touchnavigation, 0x00, sizeof(touchconfig_t) * NUMKEYS);

	for (i = 0; i < NUMKEYS; i++)
		nav[i].color = 16;

	// Back
	if (touchnavigationstatus.customizingcontrols)
	{
		nav[KEY_ESCAPE].w = 16 * FRACUNIT;
		nav[KEY_ESCAPE].h = 16 * FRACUNIT;
		nav[KEY_ESCAPE].color = 35;
	}
	else
	{
		nav[KEY_ESCAPE].x = corneroffset;
		nav[KEY_ESCAPE].y = corneroffset;
		nav[KEY_ESCAPE].w = 24 * FRACUNIT;
		nav[KEY_ESCAPE].h = 24 * FRACUNIT;
		nav[KEY_ESCAPE].h = 24 * FRACUNIT;
	}

	// Confirm
	if (touchnavigationstatus.customizingcontrols)
		nav[KEY_ENTER].hidden = true;
	else
	{
		nav[KEY_ENTER].w = 24 * FRACUNIT;
		nav[KEY_ENTER].h = 24 * FRACUNIT;
		nav[KEY_ENTER].x = (((vid.width / vid.dupx) * FRACUNIT) - nav[KEY_ENTER].w - corneroffset);
		nav[KEY_ENTER].y = corneroffset;
	}

	// Console
	if (!touchnavigationstatus.canopenconsole)
		nav[KEY_CONSOLE].hidden = true;
	else
	{
		nav[KEY_CONSOLE].x = corneroffset;
		nav[KEY_CONSOLE].y = nav[KEY_ESCAPE].y + nav[KEY_ESCAPE].h + (8 * FRACUNIT);
		nav[KEY_CONSOLE].w = 24 * FRACUNIT;
		nav[KEY_CONSOLE].h = 24 * FRACUNIT;
		nav[KEY_CONSOLE].hidden = false;
	}

	// Normalize all buttons
	G_NormalizeTouchConfig(nav, NUMKEYS);
}

void G_DefineTouchButtons(void)
{
	static touchconfigstatus_t status, navstatus;
	size_t size = sizeof(touchconfigstatus_t);

	if (!TS_Ready())
		return;

	//
	// Touch controls
	//

	if (G_TouchPresetActive())
	{
		status.vidwidth = vid.width;
		status.vidheight = vid.height;
		status.guiscale = touch_gui_scale;

		status.preset = touch_preset;
		status.movementstyle = touch_movementstyle;

		status.ringslinger = G_RingSlingerGametype();
		status.ctfgametype = (gametyperules & GTR_TEAMFLAGS);
		status.canpause = ((netgame && (cv_pause.value || server || IsPlayerAdmin(consoleplayer))) || (modeattacking && demorecording));
		status.canviewpointswitch = G_CanViewpointSwitch(false);
		status.cantalk = (netgame && !CHAT_MUTE);
		status.canteamtalk = (G_GametypeHasTeams() && players[consoleplayer].ctfteam);
		status.promptblockcontrols = promptblockcontrols;

		if (memcmp(&status, &touchcontrolstatus, size))
		{
			M_Memcpy(&touchcontrolstatus, &status, size);

			G_PositionTouchButtons();

			if (!G_TouchPresetActive())
				G_TouchPresetChanged();
		}
	}
	else
	{
		status.vidwidth = vid.width;
		status.vidheight = vid.height;
		status.promptblockcontrols = promptblockcontrols;

		if (memcmp(&status, &touchcontrolstatus, size))
		{
			M_Memcpy(&touchcontrolstatus, &status, size);
			G_TouchPresetChanged();
		}
	}

	//
	// Touch navigation
	//

	navstatus.vidwidth = vid.width;
	navstatus.vidheight = vid.height;
	navstatus.customizingcontrols = TS_IsCustomizingControls();
	navstatus.canopenconsole = (!(modeattacking || metalrecording) && !navstatus.customizingcontrols);

	if (memcmp(&navstatus, &touchnavigationstatus, size))
	{
		M_Memcpy(&touchnavigationstatus, &navstatus, size);
		G_PositionTouchNavigation();
	}
}
#endif

// clear cmd building stuff
void G_ResetInputs(void)
{
	memset(gamekeydown, 0x00, sizeof(gamekeydown));

#ifdef TOUCHINPUTS
	memset(touchfingers, 0x00, sizeof(touchfingers));
	memset(touchcontroldown, 0x00, sizeof(touchcontroldown));

	touchxmove = touchymove = touchpressure = 0.0f;
#endif

	G_ResetJoysticks();
	G_ResetMice();
}

// clear joystick axes / accelerometer position
void G_ResetJoysticks(void)
{
	INT32 i;
	for (i = 0; i < JOYAXISSET; i++)
	{
		joyxmove[i] = joyymove[i] = 0;
		joy2xmove[i] = joy2ymove[i] = 0;
	}
}

// clear mice positions
void G_ResetMice(void)
{
	mousex = mousey = 0;
	mouse2x = mouse2y = 0;
}

boolean G_InGameInput(void)
{
	return (!(menuactive || CON_Ready() || chat_on));
}

INT32 G_GetControlScheme(INT32 (*fromcontrols)[2], const INT32 *gclist, INT32 gclen)
{
	INT32 i, j, gc;
	boolean skipscheme;

	for (i = 1; i < num_gamecontrolschemes; i++) // skip gcs_custom (0)
	{
		skipscheme = false;
		for (j = 0; j < (gclist && gclen ? gclen : num_gamecontrols); j++)
		{
			gc = (gclist && gclen) ? gclist[j] : j;
			if (((fromcontrols[gc][0] && gamecontroldefault[i][gc][0]) ? fromcontrols[gc][0] != gamecontroldefault[i][gc][0] : true) &&
				((fromcontrols[gc][0] && gamecontroldefault[i][gc][1]) ? fromcontrols[gc][0] != gamecontroldefault[i][gc][1] : true) &&
				((fromcontrols[gc][1] && gamecontroldefault[i][gc][0]) ? fromcontrols[gc][1] != gamecontroldefault[i][gc][0] : true) &&
				((fromcontrols[gc][1] && gamecontroldefault[i][gc][1]) ? fromcontrols[gc][1] != gamecontroldefault[i][gc][1] : true))
			{
				skipscheme = true;
				break;
			}
		}
		if (!skipscheme)
			return i;
	}

	return gcs_custom;
}

void G_CopyControls(INT32 (*setupcontrols)[2], INT32 (*fromcontrols)[2], const INT32 *gclist, INT32 gclen)
{
	INT32 i, gc;

	for (i = 0; i < (gclist && gclen ? gclen : num_gamecontrols); i++)
	{
		gc = (gclist && gclen) ? gclist[i] : i;
		setupcontrols[gc][0] = fromcontrols[gc][0];
		setupcontrols[gc][1] = fromcontrols[gc][1];
	}
}

void G_SaveKeySetting(FILE *f, INT32 (*fromcontrols)[2], INT32 (*fromcontrolsbis)[2])
{
	INT32 i;

	for (i = 1; i < num_gamecontrols; i++)
	{
		fprintf(f, "setcontrol \"%s\" \"%s\"", gamecontrolname[i],
			G_KeynumToString(fromcontrols[i][0]));

		if (fromcontrols[i][1])
			fprintf(f, " \"%s\"\n", G_KeynumToString(fromcontrols[i][1]));
		else
			fprintf(f, "\n");
	}

	for (i = 1; i < num_gamecontrols; i++)
	{
		fprintf(f, "setcontrol2 \"%s\" \"%s\"", gamecontrolname[i],
			G_KeynumToString(fromcontrolsbis[i][0]));

		if (fromcontrolsbis[i][1])
			fprintf(f, " \"%s\"\n", G_KeynumToString(fromcontrolsbis[i][1]));
		else
			fprintf(f, "\n");
	}
}

INT32 G_CheckDoubleUsage(INT32 keynum, boolean modify)
{
	INT32 result = gc_null;
	if (cv_controlperkey.value == 1)
	{
		INT32 i;
		for (i = 0; i < num_gamecontrols; i++)
		{
			if (gamecontrol[i][0] == keynum)
			{
				result = i;
				if (modify) gamecontrol[i][0] = KEY_NULL;
			}
			if (gamecontrol[i][1] == keynum)
			{
				result = i;
				if (modify) gamecontrol[i][1] = KEY_NULL;
			}
			if (gamecontrolbis[i][0] == keynum)
			{
				result = i;
				if (modify) gamecontrolbis[i][0] = KEY_NULL;
			}
			if (gamecontrolbis[i][1] == keynum)
			{
				result = i;
				if (modify) gamecontrolbis[i][1] = KEY_NULL;
			}
			if (result && !modify)
				return result;
		}
	}
	return result;
}

static INT32 G_FilterKeyByVersion(INT32 numctrl, INT32 keyidx, INT32 player, INT32 *keynum1, INT32 *keynum2, boolean *nestedoverride)
{
	// Special case: ignore KEY_PAUSE because it's hardcoded
	if (keyidx == 0 && *keynum1 == KEY_PAUSE)
	{
		if (*keynum2 != KEY_PAUSE)
		{
			*keynum1 = *keynum2; // shift down keynum2 and continue
			*keynum2 = 0;
		}
		else
			return -1; // skip setting control
	}
	else if (keyidx == 1 && *keynum2 == KEY_PAUSE)
		return -1; // skip setting control

	if (GETMAJOREXECVERSION(cv_execversion.value) < 27 && ( // v2.1.22
		numctrl == gc_weaponnext || numctrl == gc_weaponprev || numctrl == gc_tossflag ||
		numctrl == gc_use || numctrl == gc_camreset || numctrl == gc_jump ||
		numctrl == gc_pause || numctrl == gc_systemmenu || numctrl == gc_camtoggle ||
		numctrl == gc_screenshot || numctrl == gc_talkkey || numctrl == gc_scores ||
		numctrl == gc_centerview
	))
	{
		INT32 keynum = 0, existingctrl = 0;
		INT32 defaultkey;
		boolean defaultoverride = false;

		// get the default gamecontrol
		if (player == 0 && numctrl == gc_systemmenu)
			defaultkey = gamecontrol[numctrl][0];
		else
			defaultkey = (player == 1 ? gamecontrolbis[numctrl][0] : gamecontrol[numctrl][1]);

		// Assign joypad button defaults if there is an open slot.
		// At this point, gamecontrol/bis should have the default controls
		// (unless LOADCONFIG is being run)
		//
		// If the player runs SETCONTROL in-game, this block should not be reached
		// because EXECVERSION is locked onto the latest version.
		if (keyidx == 0 && !*keynum1)
		{
			if (*keynum2) // push keynum2 down; this is an edge case
			{
				*keynum1 = *keynum2;
				*keynum2 = 0;
				keynum = *keynum1;
			}
			else
			{
				keynum = defaultkey;
				defaultoverride = true;
			}
		}
		else if (keyidx == 1 && (!*keynum2 || (!*keynum1 && *keynum2))) // last one is the same edge case as above
		{
			keynum = defaultkey;
			defaultoverride = true;
		}
		else // default to the specified keynum
			keynum = (keyidx == 1 ? *keynum2 : *keynum1);

		// Did our last call override keynum2?
		if (*nestedoverride)
		{
			defaultoverride = true;
			*nestedoverride = false;
		}

		// Fill keynum2 with the default control
		if (keyidx == 0 && !*keynum2)
		{
			*keynum2 = defaultkey;
			// Tell the next call that this is an override
			*nestedoverride = true;

			// if keynum2 already matches keynum1, we probably recursed
			// so unset it
			if (*keynum1 == *keynum2)
			{
				*keynum2 = 0;
				*nestedoverride = false;
		}
		}

		// check if the key is being used somewhere else before passing it
		// pass it through if it's the same numctrl. This is an edge case -- when using
		// LOADCONFIG, gamecontrol is not reset with default.
		//
		// Also, only check if we're actually overriding, to preserve behavior where
		// config'd keys overwrite default keys.
		if (defaultoverride)
			existingctrl = G_CheckDoubleUsage(keynum, false);

		if (keynum && (!existingctrl || existingctrl == numctrl))
			return keynum;
		else if (keyidx == 0 && *keynum2)
		{
			// try it again and push down keynum2
			*keynum1 = *keynum2;
			*keynum2 = 0;
			return G_FilterKeyByVersion(numctrl, keyidx, player, keynum1, keynum2, nestedoverride);
			// recursion *should* be safe because we only assign keynum2 to a joy default
			// and then clear it if we find that keynum1 already has the joy default.
		}
		else
			return 0;
	}

	// All's good, so pass the keynum as-is
	if (keyidx == 1)
		return *keynum2;
	else //if (keyidx == 0)
		return *keynum1;
}

static void setcontrol(INT32 (*gc)[2])
{
	INT32 numctrl;
	const char *namectrl;
	INT32 keynum, keynum1, keynum2;
	INT32 player = ((void*)gc == (void*)&gamecontrolbis ? 1 : 0);
	boolean nestedoverride = false;

	namectrl = COM_Argv(1);
	for (numctrl = 0; numctrl < num_gamecontrols && stricmp(namectrl, gamecontrolname[numctrl]);
		numctrl++)
		;
	if (numctrl == num_gamecontrols)
	{
		CONS_Printf(M_GetText("Control '%s' unknown\n"), namectrl);
		return;
	}
	keynum1 = G_KeyStringtoNum(COM_Argv(2));
	keynum2 = G_KeyStringtoNum(COM_Argv(3));
	keynum = G_FilterKeyByVersion(numctrl, 0, player, &keynum1, &keynum2, &nestedoverride);

	if (keynum >= 0)
	{
		(void)G_CheckDoubleUsage(keynum, true);

		// if keynum was rejected, try it again with keynum2
		if (!keynum && keynum2)
		{
			keynum1 = keynum2; // push down keynum2
			keynum2 = 0;
			keynum = G_FilterKeyByVersion(numctrl, 0, player, &keynum1, &keynum2, &nestedoverride);
			if (keynum >= 0)
				(void)G_CheckDoubleUsage(keynum, true);
		}
	}

	if (keynum >= 0)
		gc[numctrl][0] = keynum;

	if (keynum2)
	{
		keynum = G_FilterKeyByVersion(numctrl, 1, player, &keynum1, &keynum2, &nestedoverride);
		if (keynum >= 0)
		{
			if (keynum != gc[numctrl][0])
				gc[numctrl][1] = keynum;
			else
				gc[numctrl][1] = 0;
		}
	}
	else
		gc[numctrl][1] = 0;
}

void Command_Setcontrol_f(void)
{
	INT32 na;

	na = (INT32)COM_Argc();

	if (na != 3 && na != 4)
	{
		CONS_Printf(M_GetText("setcontrol <controlname> <keyname> [<2nd keyname>]: set controls for player 1\n"));
		return;
	}

	setcontrol(gamecontrol);
}

void Command_Setcontrol2_f(void)
{
	INT32 na;

	na = (INT32)COM_Argc();

	if (na != 3 && na != 4)
	{
		CONS_Printf(M_GetText("setcontrol2 <controlname> <keyname> [<2nd keyname>]: set controls for player 2\n"));
		return;
	}

	setcontrol(gamecontrolbis);
}
