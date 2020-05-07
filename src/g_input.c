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
#include "console.h"
#include "i_system.h"

#ifdef HAVE_BLUA
#include "lua_hook.h"
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

// Input variables
INT32 touch_dpad_x, touch_dpad_y, touch_dpad_w, touch_dpad_h;
fixed_t touch_gui_scale;

// Is the touch screen available?
boolean touch_screenexists = false;

// Touch screen settings
touchmovementstyle_e touch_movementstyle;
boolean touch_tinycontrols;
boolean touch_camera;

// Console variables for the touch screen
static CV_PossibleValue_t touchstyle_cons_t[] = {{tms_joystick, "Joystick"}, {tms_dpad, "D-Pad"}, {0, NULL}};

consvar_t cv_touchstyle = {"touch_movementstyle", "Joystick", CV_SAVE|CV_CALL|CV_NOINIT, touchstyle_cons_t, G_UpdateTouchControls, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_touchtiny = {"touch_tinycontrols", "Off", CV_SAVE|CV_CALL|CV_NOINIT, CV_OnOff, G_UpdateTouchControls, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_touchcamera = {"touch_camera", "On", CV_SAVE|CV_CALL|CV_NOINIT, CV_OnOff, G_UpdateTouchControls, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t touchguiscale_cons_t[] = {{FRACUNIT/2, "MIN"}, {3 * FRACUNIT, "MAX"}, {0, NULL}};
consvar_t cv_touchguiscale = {"touch_guiscale", "0.75", CV_FLOAT|CV_SAVE, touchguiscale_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t touchtrans_cons_t[] = {{0, "MIN"}, {10, "MAX"}, {0, NULL}};
consvar_t cv_touchtrans = {"touch_transinput", "10", CV_SAVE, touchtrans_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_touchmenutrans = {"touch_transmenu", "10", CV_SAVE, touchtrans_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

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
void G_ScaleTouchCoords(INT32 *x, INT32 *y, INT32 *w, INT32 *h)
{
	fixed_t dupx = vid.dupx * FRACUNIT;
	fixed_t dupy = vid.dupy * FRACUNIT;
	*x = FixedMul((*x), dupx) / FRACUNIT;
	*y = FixedMul((*y), dupy) / FRACUNIT;
	*w = FixedMul((*w), dupx) / FRACUNIT;
	*h = FixedMul((*h), dupy) / FRACUNIT;
}

boolean G_FingerTouchesButton(INT32 x, INT32 y, touchconfig_t *butt)
{
	INT32 tx = butt->x, ty = butt->y, tw = butt->w, th = butt->h;
	G_ScaleTouchCoords(&tx, &ty, &tw, &th);
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
	// Handle talk buttons
	else if (gamecontrol == gc_talkkey || gamecontrol == gc_teamkey)
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

	if (gamestate != GS_LEVEL)
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
				touchconfig_t *butt = &touchcontrols[i];

				// Ignore camera and joystick movement
				if (finger->type.mouse)
					break;

				// Ignore undefined buttons
				if (!butt->w)
					continue;

				// Ignore hidden buttons
				if (butt->hidden)
					continue;

				// Ignore mismatching movement styles
				if ((touch_movementstyle != tms_dpad) && butt->dpad)
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
				if (G_FingerTouchesButton(x, y, butt) && (!touchcontroldown[i]))
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

			// Check if your finger touches the d-pad area.
			if (!foundbutton)
			{
				touchconfig_t dpad;
				dpad.x = touch_dpad_x;
				dpad.y = touch_dpad_y;
				dpad.w = touch_dpad_w;
				dpad.h = touch_dpad_h;
				if (G_FingerTouchesButton(x, y, &dpad))
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
						INT32 padx = touch_dpad_x, pady = touch_dpad_y;
						INT32 padw = touch_dpad_w, padh = touch_dpad_h;

						G_ScaleTouchCoords(&padx, &pady, &padw, &padh);

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

// Returns true if you can switch your viewpoint to this player.
boolean G_CanViewpointSwitchToPlayer(player_t *player)
{
	player_t *myself = &players[consoleplayer];

	if (player->spectator)
		return false;

	if (G_GametypeHasTeams())
	{
		if (myself->ctfteam && player->ctfteam != myself->ctfteam)
			return false;
	}
	else if (gametype == GT_HIDEANDSEEK)
	{
		if (myself->pflags & PF_TAGIT)
			return false;
	}
	// Other Tag-based gametypes?
	else if (G_TagGametype())
	{
		if (!myself->spectator && (myself->pflags & PF_TAGIT) != (player->pflags & PF_TAGIT))
			return false;
	}
	else if (G_GametypeHasSpectators() && G_RingSlingerGametype())
	{
		if (!myself->spectator)
			return false;
	}

	return true;
}

// Returns true if you can switch your viewpoint at all.
boolean G_CanViewpointSwitch(void)
{
	// ViewpointSwitch Lua hook.
#ifdef HAVE_BLUA
	UINT8 canSwitchView = 0;
#endif

	INT32 checkdisplayplayer = displayplayer;

	if (splitscreen || !netgame)
		return false;

	if (D_NumPlayers() <= 1)
		return false;

	do
	{
		checkdisplayplayer++;
		if (checkdisplayplayer == MAXPLAYERS)
			checkdisplayplayer = 0;

		if (!playeringame[checkdisplayplayer])
			continue;

#ifdef HAVE_BLUA
		// Call ViewpointSwitch hooks here.
		canSwitchView = LUAh_ViewpointSwitch(&players[consoleplayer], &players[checkdisplayplayer], false);
		if (canSwitchView == 1) // Set viewpoint to this player
			break;
		else if (canSwitchView == 2) // Skip this player
			continue;
#endif

		if (!G_CanViewpointSwitchToPlayer(&players[checkdisplayplayer]))
			continue;

		break;
	} while (checkdisplayplayer != consoleplayer);

	// had any change??
	return (checkdisplayplayer != displayplayer);
}

// Handles the viewpoint switch key being pressed.
boolean G_DoViewpointSwitch(void)
{
	// ViewpointSwitch Lua hook.
#ifdef HAVE_BLUA
	UINT8 canSwitchView = 0;
#endif

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

#ifdef HAVE_BLUA
			// Call ViewpointSwitch hooks here.
			canSwitchView = LUAh_ViewpointSwitch(&players[consoleplayer], &players[displayplayer], false);
			if (canSwitchView == 1) // Set viewpoint to this player
				break;
			else if (canSwitchView == 2) // Skip this player
				continue;
#endif

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

static const char *gamecontrolname[num_gamecontrols] =
{
	"nothing", // a key/button mapped to gc_null has no effect
	"forward",
	"backward",
	"strafeleft",
	"straferight",
	"turnleft",
	"turnright",
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
	CV_RegisterVar(&cv_touchmenutrans);
	CV_RegisterVar(&cv_touchtrans);

	CV_RegisterVar(&cv_touchcamera);
	CV_RegisterVar(&cv_touchtiny);
	CV_RegisterVar(&cv_touchstyle);

	CV_RegisterVar(&cv_touchguiscale);

	G_UpdateTouchControls();
#endif
}

// Lactozilla: Touch input
#ifdef TOUCHINPUTS
void G_SetupTouchSettings(void)
{
	touch_movementstyle = cv_touchstyle.value;
	touch_camera = (cv_usemouse.value ? false : (!!cv_touchcamera.value));
	touch_tinycontrols = !!cv_touchtiny.value;
	touch_gui_scale = cv_touchguiscale.value;
}

#define SCALECOORD(coord) FixedMul(coord, scale)

void G_UpdateTouchControls(void)
{
	G_SetupTouchSettings();
	G_DefineTouchButtons();
}

static void G_DPadPreset(fixed_t scale, fixed_t dw)
{
	fixed_t w, h;

	if (touch_tinycontrols)
	{
		// Up
		w = 20 * FRACUNIT;
		h = 16 * FRACUNIT;
		touchcontrols[gc_forward].w = SCALECOORD(w);
		touchcontrols[gc_forward].h = SCALECOORD(h);
		touchcontrols[gc_forward].x = (touch_dpad_x + SCALECOORD(FixedDiv(dw, 2 * FRACUNIT))) - FixedCeil(FixedDiv(FixedMul(w, scale), 3 * FRACUNIT));
		touchcontrols[gc_forward].y = touch_dpad_y - SCALECOORD(8 * FRACUNIT);

		// Down
		touchcontrols[gc_backward].w = SCALECOORD(w);
		touchcontrols[gc_backward].h = SCALECOORD(h);
		touchcontrols[gc_backward].x = (touch_dpad_x + SCALECOORD(FixedDiv(dw, 2 * FRACUNIT))) - FixedCeil(FixedDiv(FixedMul(w, scale), 3 * FRACUNIT));
		touchcontrols[gc_backward].y = ((touch_dpad_y + touch_dpad_h) - touchcontrols[gc_backward].h) + SCALECOORD(8 * FRACUNIT);

		// Left
		w = 16 * FRACUNIT;
		h = 14 * FRACUNIT;
		touchcontrols[gc_strafeleft].w = SCALECOORD(w);
		touchcontrols[gc_strafeleft].h = SCALECOORD(h);
		touchcontrols[gc_strafeleft].x = touch_dpad_x - SCALECOORD(8 * FRACUNIT);
		touchcontrols[gc_strafeleft].y = (touch_dpad_y + FixedDiv(touch_dpad_h, 2 * FRACUNIT)) - FixedDiv(touchcontrols[gc_strafeleft].h, 2 * FRACUNIT);

		// Right
		touchcontrols[gc_straferight].w = SCALECOORD(w);
		touchcontrols[gc_straferight].h = SCALECOORD(h);
		touchcontrols[gc_straferight].x = ((touch_dpad_x + touch_dpad_w) - touchcontrols[gc_straferight].w) + SCALECOORD(8 * FRACUNIT);
		touchcontrols[gc_straferight].y = (touch_dpad_y + FixedDiv(touch_dpad_h, 2 * FRACUNIT)) - FixedDiv(touchcontrols[gc_straferight].h, 2 * FRACUNIT);
	}
	else
	{
		// Up
		w = 40 * FRACUNIT;
		h = 32 * FRACUNIT;
		touchcontrols[gc_forward].w = SCALECOORD(w);
		touchcontrols[gc_forward].h = SCALECOORD(h);
		touchcontrols[gc_forward].x = (touch_dpad_x + SCALECOORD(FixedDiv(dw, 2 * FRACUNIT))) - SCALECOORD(FixedDiv(w, 3 * FRACUNIT));
		touchcontrols[gc_forward].y = touch_dpad_y - SCALECOORD(16 * FRACUNIT);

		// Down
		touchcontrols[gc_backward].w = SCALECOORD(w);
		touchcontrols[gc_backward].h = SCALECOORD(h);
		touchcontrols[gc_backward].x = (touch_dpad_x + SCALECOORD(FixedDiv(dw, 2 * FRACUNIT))) - SCALECOORD(FixedDiv(w, 3 * FRACUNIT));
		touchcontrols[gc_backward].y = ((touch_dpad_y + touch_dpad_h) - touchcontrols[gc_backward].h) + SCALECOORD(16 * FRACUNIT);

		// Left
		w = 32 * FRACUNIT;
		h = 28 * FRACUNIT;
		touchcontrols[gc_strafeleft].w = SCALECOORD(w);
		touchcontrols[gc_strafeleft].h = SCALECOORD(h);
		touchcontrols[gc_strafeleft].x = touch_dpad_x - SCALECOORD(16 * FRACUNIT);
		touchcontrols[gc_strafeleft].y = (touch_dpad_y + FixedDiv(touch_dpad_h, 2 * FRACUNIT)) - FixedDiv(touchcontrols[gc_strafeleft].h, 2 * FRACUNIT);

		// Right
		touchcontrols[gc_straferight].w = SCALECOORD(w);
		touchcontrols[gc_straferight].h = SCALECOORD(h);
		touchcontrols[gc_straferight].x = ((touch_dpad_x + touch_dpad_w) - touchcontrols[gc_straferight].w) + SCALECOORD(16 * FRACUNIT);
		touchcontrols[gc_straferight].y = (touch_dpad_y + FixedDiv(touch_dpad_h, 2 * FRACUNIT)) - FixedDiv(touchcontrols[gc_straferight].h, 2 * FRACUNIT);
	}
}

static void G_ScaleDPadBase(fixed_t dx, fixed_t dy, fixed_t dw, fixed_t dh, fixed_t scale, fixed_t offs, fixed_t bottomalign)
{
	touch_dpad_w = SCALECOORD(dw);
	touch_dpad_h = SCALECOORD(dh);
	touch_dpad_x = max(dx, ((dx + FixedDiv(dw, FRACUNIT * 2)) - FixedDiv(touch_dpad_w, FRACUNIT * 2)));
	if (scale < FRACUNIT)
		touch_dpad_y = ((dy + FixedDiv(dh, FRACUNIT * 2)) - FixedDiv(touch_dpad_h, FRACUNIT * 2));
	else
		touch_dpad_y = ((dy + dh) - touch_dpad_h);
	touch_dpad_y += (offs + bottomalign);

	// Offset d-pad base
	if (touch_tinycontrols)
	{
		if (touch_movementstyle == tms_joystick)
		{
			touch_dpad_x -= 4 * FRACUNIT;
			touch_dpad_y += 8 * FRACUNIT;
		}

		if (G_RingSlingerGametype())
			touch_dpad_y -= 4 * FRACUNIT;
	}
	else
	{
		if (touch_movementstyle == tms_joystick)
		{
			touch_dpad_x -= 12 * FRACUNIT;
			touch_dpad_y += 16 * FRACUNIT;
		}

		if (G_RingSlingerGametype())
			touch_dpad_y -= 8 * FRACUNIT;
	}
}

void G_TouchControlPreset(void)
{
	fixed_t x, y, w, h;
	fixed_t dx, dy, dw, dh;
	fixed_t corneroffset = 4 * FRACUNIT;
	fixed_t rightcorner = ((vid.width / vid.dupx) * FRACUNIT);
	fixed_t bottomcorner = ((vid.height / vid.dupy) * FRACUNIT);
	fixed_t jsoffs = G_RingSlingerGametype() ? (-4 * FRACUNIT) : 0, jumph;
	fixed_t offs = (promptactive ? -16 : 0);
	fixed_t nonjoyoffs = -12 * FRACUNIT;
	fixed_t bottomalign = 0;
	fixed_t scale = touch_gui_scale;

	touchconfig_t *ref;
	boolean bothvisible;
	boolean eithervisible;

	if (vid.height != BASEVIDHEIGHT * vid.dupy)
		bottomalign = ((vid.height - (BASEVIDHEIGHT * vid.dupy)) / vid.dupy) * FRACUNIT;

	// clear all
	memset(touchcontrols, 0x00, sizeof(touchconfig_t) * num_gamecontrols);

	if (touch_tinycontrols)
	{
		dx = 24 * FRACUNIT;
		dy = 128 * FRACUNIT;
		dw = 32 * FRACUNIT;
		dh = 32 * FRACUNIT;
	}
	else
	{
		dx = 24 * FRACUNIT;
		dy = 92 * FRACUNIT;
		dw = 64 * FRACUNIT;
		dh = 64 * FRACUNIT;
	}

	// D-Pad
	G_ScaleDPadBase(dx, dy, dw, dh, scale, offs, bottomalign);
	G_DPadPreset(scale, dw);

	// Jump and spin
	if (touch_tinycontrols)
	{
		// Jump
		w = 40 * FRACUNIT;
		h = jumph = 32 * FRACUNIT;
		touchcontrols[gc_jump].w = SCALECOORD(w);
		touchcontrols[gc_jump].h = SCALECOORD(h);
		touchcontrols[gc_jump].x = (rightcorner - touchcontrols[gc_jump].w - corneroffset - (12 * FRACUNIT));
		touchcontrols[gc_jump].y = (bottomcorner - touchcontrols[gc_jump].h - corneroffset - (12 * FRACUNIT)) + jsoffs + offs + nonjoyoffs;

		// Spin
		w = 32 * FRACUNIT;
		h = 24 * FRACUNIT;
		touchcontrols[gc_use].w = SCALECOORD(w);
		touchcontrols[gc_use].h = SCALECOORD(h);
		touchcontrols[gc_use].x = (touchcontrols[gc_jump].x - touchcontrols[gc_use].w - (12 * FRACUNIT));
		touchcontrols[gc_use].y = touchcontrols[gc_jump].y + (8 * FRACUNIT);
	}
	else
	{
		// Jump
		w = 48 * FRACUNIT;
		h = jumph = 48 * FRACUNIT;
		touchcontrols[gc_jump].w = SCALECOORD(w);
		touchcontrols[gc_jump].h = SCALECOORD(h);
		touchcontrols[gc_jump].x = (rightcorner - touchcontrols[gc_jump].w - corneroffset - (12 * FRACUNIT));
		touchcontrols[gc_jump].y = (bottomcorner - touchcontrols[gc_jump].h - corneroffset - (12 * FRACUNIT)) + jsoffs + offs + nonjoyoffs;

		// Spin
		w = 40 * FRACUNIT;
		h = 32 * FRACUNIT;
		touchcontrols[gc_use].w = SCALECOORD(w);
		touchcontrols[gc_use].h = SCALECOORD(h);
		touchcontrols[gc_use].x = (touchcontrols[gc_jump].x - touchcontrols[gc_use].w - (12 * FRACUNIT));
		touchcontrols[gc_use].y = touchcontrols[gc_jump].y + (12 * FRACUNIT);
	}

	// Fire, fire normal, and toss flag
	if (G_RingSlingerGametype())
	{
		offs = SCALECOORD(8 * FRACUNIT);
		h = SCALECOORD(FixedDiv(jumph, 2 * FRACUNIT)) + SCALECOORD(4 * FRACUNIT);
		touchcontrols[gc_use].h = h;
		touchcontrols[gc_use].y = ((touchcontrols[gc_jump].y + touchcontrols[gc_jump].h) - h) + offs;

		touchcontrols[gc_fire].w = touchcontrols[gc_use].w;
		touchcontrols[gc_fire].h = h;
		touchcontrols[gc_fire].x = touchcontrols[gc_use].x;
		touchcontrols[gc_fire].y = touchcontrols[gc_jump].y - offs;

		if (gametyperules & GTR_TEAMFLAGS)
		{
			ref = &touchcontrols[gc_tossflag];
			touchcontrols[gc_firenormal].hidden = true;
		}
		else
		{
			ref = &touchcontrols[gc_firenormal];
			touchcontrols[gc_tossflag].hidden = true;
		}

		ref->w = touchcontrols[gc_jump].w;
		ref->h = touchcontrols[gc_fire].h;
		if (!touch_tinycontrols)
			ref->h = FixedDiv(ref->h, 2 * FRACUNIT);

		ref->x = touchcontrols[gc_jump].x;
		ref->y = touchcontrols[gc_jump].y - ref->h - SCALECOORD(4 * FRACUNIT);
	}
	else
	{
		touchcontrols[gc_fire].hidden = true;
		touchcontrols[gc_firenormal].hidden = true;
		touchcontrols[gc_tossflag].hidden = true;
	}

	offs = SCALECOORD(8 * FRACUNIT);

	// Menu
	touchcontrols[gc_systemmenu].w = SCALECOORD(32 * FRACUNIT);
	touchcontrols[gc_systemmenu].h = SCALECOORD(32 * FRACUNIT);
	touchcontrols[gc_systemmenu].x = (rightcorner - touchcontrols[gc_systemmenu].w - corneroffset);
	touchcontrols[gc_systemmenu].y = corneroffset;

	// Pause
	touchcontrols[gc_pause].x = touchcontrols[gc_systemmenu].x;
	touchcontrols[gc_pause].y = touchcontrols[gc_systemmenu].y;
	touchcontrols[gc_pause].w = SCALECOORD(24 * FRACUNIT);
	touchcontrols[gc_pause].h = SCALECOORD(24 * FRACUNIT);
	if ((netgame && (cv_pause.value || server || IsPlayerAdmin(consoleplayer)))
	|| (modeattacking && demorecording))
		touchcontrols[gc_pause].x -= (touchcontrols[gc_pause].w + SCALECOORD(4 * FRACUNIT));
	else
		touchcontrols[gc_pause].hidden = true;

	// Spy mode
	touchcontrols[gc_viewpoint].hidden = true;
	touchcontrols[gc_viewpoint].x = touchcontrols[gc_pause].x;
	touchcontrols[gc_viewpoint].y = touchcontrols[gc_pause].y;
	if (G_CanViewpointSwitch())
	{
		touchcontrols[gc_viewpoint].w = SCALECOORD(32 * FRACUNIT);
		touchcontrols[gc_viewpoint].h = SCALECOORD(24 * FRACUNIT);
		touchcontrols[gc_viewpoint].x -= (touchcontrols[gc_viewpoint].w + SCALECOORD(4 * FRACUNIT));
		touchcontrols[gc_viewpoint].hidden = false;
	}

	// Align screenshot and movie mode buttons
	w = SCALECOORD(40 * FRACUNIT);
	h = SCALECOORD(24 * FRACUNIT);

	bothvisible = ((!touchcontrols[gc_viewpoint].hidden) && (!touchcontrols[gc_pause].hidden));
	eithervisible = (touchcontrols[gc_viewpoint].hidden ^ touchcontrols[gc_pause].hidden);

	if (bothvisible || eithervisible)
	{
		if (bothvisible)
			ref = &touchcontrols[gc_pause];
		else // only if one of either are visible, but not both
			ref = (touchcontrols[gc_viewpoint].hidden ? &touchcontrols[gc_pause] : &touchcontrols[gc_viewpoint]);

		x = ref->x - (w - ref->w);
		y = ref->y + ref->h + offs;
	}
	else
	{
		x = (touchcontrols[gc_viewpoint].x - w - SCALECOORD(4 * FRACUNIT));
		y = touchcontrols[gc_viewpoint].y;
	}

	touchcontrols[gc_screenshot].w = touchcontrols[gc_recordgif].w = w;
	touchcontrols[gc_screenshot].h = touchcontrols[gc_recordgif].h = h;

	// Screenshot
	touchcontrols[gc_screenshot].x = x;
	touchcontrols[gc_screenshot].y = y;

	// Movie mode
	touchcontrols[gc_recordgif].x = x;
	touchcontrols[gc_recordgif].y = (touchcontrols[gc_screenshot].y + touchcontrols[gc_screenshot].h + offs);

	// Talk key and team talk key
	touchcontrols[gc_talkkey].hidden = true; // hidden by default
	touchcontrols[gc_teamkey].hidden = true; // hidden outside of team games

	// if netgame + chat not muted
	if (netgame && !CHAT_MUTE)
	{
		touchcontrols[gc_talkkey].w = SCALECOORD(32 * FRACUNIT);
		touchcontrols[gc_talkkey].h = SCALECOORD(24 * FRACUNIT);
		touchcontrols[gc_talkkey].x = (rightcorner - touchcontrols[gc_talkkey].w - corneroffset);
		touchcontrols[gc_talkkey].y = (touchcontrols[gc_systemmenu].y + touchcontrols[gc_systemmenu].h + offs);
		touchcontrols[gc_talkkey].hidden = false;

		if (G_GametypeHasTeams() && players[consoleplayer].ctfteam)
		{
			touchcontrols[gc_teamkey].w = SCALECOORD(32 * FRACUNIT);
			touchcontrols[gc_teamkey].h = SCALECOORD(24 * FRACUNIT);
			touchcontrols[gc_teamkey].x = touchcontrols[gc_talkkey].x;
			touchcontrols[gc_teamkey].y = touchcontrols[gc_talkkey].y + touchcontrols[gc_talkkey].h + offs;
			touchcontrols[gc_teamkey].hidden = false;
		}
	}

	// Mark movement controls as d-pad buttons
	touchcontrols[gc_forward].dpad = true;
	touchcontrols[gc_backward].dpad = true;
	touchcontrols[gc_strafeleft].dpad = true;
	touchcontrols[gc_straferight].dpad = true;

	// Set button names
	touchcontrols[gc_jump].name = "JUMP";
	touchcontrols[gc_use].name = "SPIN";
	touchcontrols[gc_fire].name = "FIRE";
	touchcontrols[gc_firenormal].name = "F.NORMAL";
	touchcontrols[gc_tossflag].name = "TOSSFLAG";

	touchcontrols[gc_systemmenu].name = "MENU";
	touchcontrols[gc_viewpoint].name = "F12";
	touchcontrols[gc_screenshot].name = "SCRCAP";
	touchcontrols[gc_recordgif].name = "REC";
	touchcontrols[gc_talkkey].name = "TALK";
	touchcontrols[gc_teamkey].name = "TEAM";

	// Set abbreviated button names
	touchcontrols[gc_jump].tinyname = "JMP";
	touchcontrols[gc_use].tinyname = "SPN";
	touchcontrols[gc_fire].tinyname = "FRE";
	touchcontrols[gc_firenormal].tinyname = "FRN";
	touchcontrols[gc_tossflag].tinyname = "TSS";

	touchcontrols[gc_systemmenu].tinyname = "MNU";
	touchcontrols[gc_screenshot].tinyname = "SCR";
	touchcontrols[gc_talkkey].tinyname = "TLK";
	touchcontrols[gc_teamkey].tinyname = "TTK";

	// Hide movement controls in prompts that block controls
	if (promptblockcontrols)
	{
		INT32 i;
		for (i = 0; i < num_gamecontrols; i++)
		{
			if (G_TouchButtonIsPlayerControl(i))
				touchcontrols[i].hidden = true;
		}
	}
}

#undef SCALECOORD

void G_TouchNavigationPreset(void)
{
	INT32 corneroffset = 4 * FRACUNIT;

	// clear all
	memset(touchnavigation, 0x00, sizeof(touchconfig_t) * NUMKEYS);

	// Back
	touchnavigation[KEY_ESCAPE].x = corneroffset;
	touchnavigation[KEY_ESCAPE].y = corneroffset;
	touchnavigation[KEY_ESCAPE].w = 24 * FRACUNIT;
	touchnavigation[KEY_ESCAPE].h = 24 * FRACUNIT;

	// Confirm
	touchnavigation[KEY_ENTER].w = 24 * FRACUNIT;
	touchnavigation[KEY_ENTER].h = 24 * FRACUNIT;
	touchnavigation[KEY_ENTER].x = (((vid.width / vid.dupx) * FRACUNIT) - touchnavigation[KEY_ENTER].w - corneroffset);
	touchnavigation[KEY_ENTER].y = corneroffset;

	// Console
	if (modeattacking || metalrecording)
		touchnavigation[KEY_CONSOLE].hidden = true;
	else
	{
		touchnavigation[KEY_CONSOLE].x = corneroffset;
		touchnavigation[KEY_CONSOLE].y = touchnavigation[KEY_ENTER].y + touchnavigation[KEY_ENTER].h + (8 * FRACUNIT);
		touchnavigation[KEY_CONSOLE].w = 24 * FRACUNIT;
		touchnavigation[KEY_CONSOLE].h = 24 * FRACUNIT;
		touchnavigation[KEY_CONSOLE].hidden = false;
	}
}

void G_DefineTouchButtons(void)
{
	G_TouchControlPreset();
	G_TouchNavigationPreset();
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
