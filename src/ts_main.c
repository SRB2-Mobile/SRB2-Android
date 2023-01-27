// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2022 by Jaime Ita Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ts_main.c
/// \brief Touch screen

#include "doomdata.h"
#include "doomtype.h"
#include "doomdef.h"

#include "ts_main.h"
#include "ts_draw.h"
#include "ts_custom.h"

#include "g_game.h" // players[MAXPLAYERS], promptactive, promptblockcontrols

#include "m_menu.h" // M_IsCustomizingTouchControls
#include "m_misc.h" // M_ScreenShot
#include "m_random.h" // M_RandomKey
#include "console.h" // CON_Toggle

#include "st_stuff.h"
#include "hu_stuff.h"
#include "v_video.h"

#include "s_sound.h" // S_StartSound

#ifdef TOUCHINPUTS
boolean ts_ready = false;

boolean TS_Ready(void)
{
	return ts_ready;
}

float touchxmove, touchymove, touchpressure;

// Finger data
touchfinger_t touchfingers[NUMTOUCHFINGERS];
UINT8 touchcontroldown[NUM_GAMECONTROLS];

// Control buttons
touchconfig_t touchcontrols[NUM_GAMECONTROLS];
touchconfig_t *usertouchcontrols = NULL;

// Navigation buttons
touchnavbutton_t touchnavigation[NUMTOUCHNAV];

// Touch screen config. status
touchconfigstatus_t touchcontrolstatus;
touchnavstatus_t touchnavigationstatus;

// Input variables
INT32 touch_joystick_x, touch_joystick_y, touch_joystick_w, touch_joystick_h;
fixed_t touch_preset_scale;
boolean touch_scale_meta;

// Is the touch screen available for game inputs?
boolean touch_useinputs = true;
consvar_t cv_showfingers = CVAR_INIT ("showfingers", "Off", CV_SAVE, CV_OnOff, NULL);

// Finger event handler
void (*touch_fingerhandler)(touchfinger_t *, event_t *) = NULL;

// Touch screen settings
touchmovementstyle_e touch_movementstyle;
touchpreset_e touch_preset;
boolean touch_camera;
INT32 touch_corners;

// Options for the touch screen
static CV_PossibleValue_t touchstyle_cons_t[] = {
	{tms_joystick, "Joystick"},
	{tms_dpad, "D-Pad"},
	{0, NULL}};

static CV_PossibleValue_t touchpreset_cons_t[] = {
	{touchpreset_none, "None"},
	{touchpreset_normal, "Default"},
	{touchpreset_tiny, "Tiny"},
	{0, NULL}};

#define TOUCHCVARFLAGS (CV_SAVE | CV_CALL | CV_NOINIT)

consvar_t cv_touchinputs = CVAR_INIT ("touch_inputs", "On", TOUCHCVARFLAGS, CV_YesNo, TS_UpdateControls);
consvar_t cv_touchstyle = CVAR_INIT ("touch_movementstyle", "Joystick", TOUCHCVARFLAGS, touchstyle_cons_t, TS_UpdateControls);
consvar_t cv_touchpreset = CVAR_INIT ("touch_preset", "Default", TOUCHCVARFLAGS, touchpreset_cons_t, TS_PresetChanged);
consvar_t cv_touchlayout = CVAR_INIT ("touch_layout", "None", TOUCHCVARFLAGS, NULL, TS_LoadLayoutFromCVar);
consvar_t cv_touchcamera = CVAR_INIT ("touch_camera", "On", TOUCHCVARFLAGS, CV_OnOff, TS_UpdateControls);

static CV_PossibleValue_t touchpresetscale_cons_t[] = {{FRACUNIT/2, "MIN"}, {3 * FRACUNIT, "MAX"}, {0, NULL}};
consvar_t cv_touchpresetscale = CVAR_INIT ("touch_guiscale", "0.75", CV_FLOAT | TOUCHCVARFLAGS | CV_SLIDER_SAFE, touchpresetscale_cons_t, TS_UpdateControls);
consvar_t cv_touchscalemeta = CVAR_INIT ("touch_scalemeta", "Yes", TOUCHCVARFLAGS, CV_YesNo, TS_UpdateControls);

static CV_PossibleValue_t touchcorners_cons_t[] = {{0, "MIN"}, {64, "MAX"}, {0, NULL}};
consvar_t cv_touchcorners = CVAR_INIT ("touch_corners", "8", TOUCHCVARFLAGS | CV_SLIDER_SAFE, touchcorners_cons_t, TS_UpdateControls);

static CV_PossibleValue_t touchtrans_cons_t[] = {{0, "MIN"}, {10, "MAX"}, {0, NULL}};
consvar_t cv_touchtrans = CVAR_INIT ("touch_transinput", "10", CV_SAVE | CV_SLIDER_SAFE, touchtrans_cons_t, NULL);
consvar_t cv_touchmenutrans = CVAR_INIT ("touch_transmenu", "10", CV_SAVE | CV_SLIDER_SAFE, touchtrans_cons_t, NULL);

static CV_PossibleValue_t touchnavmethod_cons_t[] = {{0, "Default"}, {1, "Only highlight"}, {2, "Screen regions"}, {0, NULL}};
consvar_t cv_touchnavmethod = CVAR_INIT ("touch_navmethod", "Default", CV_SAVE, touchnavmethod_cons_t, NULL);

// Miscellaneous options
consvar_t cv_touchscreenshots = CVAR_INIT ("touch_screenshotinputs", "No", CV_SAVE, CV_YesNo, NULL);

// Touch layout options
#define TOUCHLAYOUTCVAR(name, def, func) CVAR_INIT (name, def, (CV_CALL | CV_NOINIT), CV_YesNo, func);

static void ClearLayoutAndKeepSettings(void)
{
	TS_ClearCurrentLayout(false);
	TS_SynchronizeCurrentLayout();
	TS_PositionExtraUserButtons();
	if (usertouchlayout)
		TS_MarkDPadButtons(usertouchlayout->config);
}

static void UseGrid_OnChange(void)
{
	if (TS_IsPresetActive() || usertouchlayout->widescreen)
	{
		if (TS_IsPresetActive())
		{
			CV_StealthSetValue(&cv_touchlayoutusegrid, 1);
			M_ShowESCMessage(TSC_MESSAGE_DISABLECONTROLPRESET);
		}
		else
		{
			CV_StealthSetValue(&cv_touchlayoutusegrid, 0);
			M_ShowESCMessage("You cannot change this option\nif the layout isn't GUI-scaled.\n\n");
		}

		return;
	}

	TS_SynchronizeCurrentLayout();
	userlayoutnew = false;
}

static void WideScreen_OnChangeResponse(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
	{
		CV_StealthSetValue(&cv_touchlayoutwidescreen, usertouchlayout->widescreen);
		return;
	}

	CV_StealthSetValue(&cv_touchlayoutusegrid, 0);
	ClearLayoutAndKeepSettings();
	S_StartSound(NULL, sfx_altdi1 + M_RandomKey(4));
}

static void WideScreen_OnChange(void)
{
	if (TS_IsPresetActive())
	{
		CV_StealthSetValue(&cv_touchlayoutwidescreen, 1);
		M_ShowESCMessage(TSC_MESSAGE_DISABLECONTROLPRESET);
		return;
	}

	if (userlayoutnew)
		ClearLayoutAndKeepSettings();
	else
		M_StartMessage(va("Changing this option will clear the current layout.\nProceed anyway?\n\n(%s)\n", M_GetUserActionString(CONFIRM_MESSAGE)), WideScreen_OnChangeResponse, MM_YESNO);
}

consvar_t cv_touchlayoutusegrid = TOUCHLAYOUTCVAR("touch_layoutusegrid", "Yes", UseGrid_OnChange);
consvar_t cv_touchlayoutwidescreen = TOUCHLAYOUTCVAR("touch_layoutwidescreen", "Yes", WideScreen_OnChange);

// Touch screen sensitivity
static CV_PossibleValue_t touchcamsens_cons_t[] = {{1, "MIN"}, {100, "MAX"}, {0, NULL}};
consvar_t cv_touchcamhorzsens = CVAR_INIT ("touch_sens", "40", CV_SAVE, touchcamsens_cons_t, NULL);
consvar_t cv_touchcamvertsens = CVAR_INIT ("touch_vertsens", "45", CV_SAVE, touchcamsens_cons_t, NULL);

#define TOUCHJOYCVARFLAGS (CV_FLOAT | CV_SAVE)

static CV_PossibleValue_t touchjoysens_cons_t[] = {{FRACUNIT/100, "MIN"}, {4 * FRACUNIT, "MAX"}, {0, NULL}};
consvar_t cv_touchjoyhorzsens = CVAR_INIT ("touch_joyhorzsens", "2.0", TOUCHJOYCVARFLAGS, touchjoysens_cons_t, NULL);
consvar_t cv_touchjoyvertsens = CVAR_INIT ("touch_joyvertsens", "2.0", TOUCHJOYCVARFLAGS, touchjoysens_cons_t, NULL);
consvar_t cv_touchjoydeadzone = CVAR_INIT ("touch_joydeadzone", "0.125", TOUCHJOYCVARFLAGS, zerotoone_cons_t, NULL);

boolean TS_IsCustomizingControls(void)
{
	return M_IsCustomizingTouchControls(); // How redundant
}

void TS_RegisterVariables(void)
{
	// Display settings
	CV_RegisterVar(&cv_showfingers);
	CV_RegisterVar(&cv_touchscreenshots);
	CV_RegisterVar(&cv_touchmenutrans);
	CV_RegisterVar(&cv_touchtrans);

	// Layout settings
	CV_RegisterVar(&cv_touchlayoutusegrid);
	CV_RegisterVar(&cv_touchlayoutwidescreen);

	// Preset settings
	CV_RegisterVar(&cv_touchscalemeta);
	CV_RegisterVar(&cv_touchpresetscale);

	// Sensitivity settings
	CV_RegisterVar(&cv_touchcamhorzsens);
	CV_RegisterVar(&cv_touchcamvertsens);
	CV_RegisterVar(&cv_touchjoyvertsens);
	CV_RegisterVar(&cv_touchjoyhorzsens);
	CV_RegisterVar(&cv_touchjoydeadzone);

	// Main options
	CV_RegisterVar(&cv_touchnavmethod);
	CV_RegisterVar(&cv_touchcorners);
	CV_RegisterVar(&cv_touchcamera);
	CV_RegisterVar(&cv_touchpreset);
	CV_RegisterVar(&cv_touchlayout);
	CV_RegisterVar(&cv_touchstyle);
	CV_RegisterVar(&cv_touchinputs);
}

boolean TS_IsPresetActive(void)
{
	return (touch_preset != touchpreset_none);
}

void TS_ScaleCoords(INT32 *x, INT32 *y, INT32 *w, INT32 *h, boolean normalized, boolean screenscale)
{
	fixed_t xs = FRACUNIT;
	fixed_t ys = FRACUNIT;

	if (normalized)
		TS_DenormalizeCoords(&xs, &ys);

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

static boolean IsFingerTouchingButton(INT32 x, INT32 y, touchconfig_t *btn)
{
	INT32 tx = btn->x, ty = btn->y, tw = btn->w, th = btn->h;
	TS_ScaleCoords(&tx, &ty, &tw, &th, true, (!btn->dontscale));
	if (!btn->dontscale)
		TS_CenterIntegerCoords(&tx, &ty);
	return (x >= tx && x <= tx + tw && y >= ty && y <= ty + th);
}

boolean TS_FingerTouchesButton(INT32 x, INT32 y, touchconfig_t *btn)
{
	return IsFingerTouchingButton(x, y, btn);
}

boolean TS_FingerTouchesNavigationButton(INT32 x, INT32 y, touchnavbutton_t *btn)
{
	if (btn->defined)
	{
		INT32 tx = btn->x, ty = btn->y, tw = btn->w, th = btn->h;
		TS_ScaleCoords(&tx, &ty, &tw, &th, true, true);
		return (x >= tx && x <= tx + tw && y >= ty && y <= ty + th);
	}
	return false;
}

boolean TS_FingerTouchesJoystickArea(INT32 x, INT32 y)
{
	INT32 tx = touch_joystick_x, ty = touch_joystick_y, tw = touch_joystick_w, th = touch_joystick_h;
	TS_ScaleCoords(&tx, &ty, &tw, &th, false, true);
	TS_CenterIntegerCoords(&tx, &ty);
	return (x >= tx && x <= tx + tw && y >= ty && y <= ty + th);
}

boolean TS_ButtonIsPlayerControl(INT32 gc)
{
	switch (gc)
	{
		case GC_TALKKEY:
		case GC_TEAMKEY:
		case GC_SCORES:
		case GC_CONSOLE:
		case GC_PAUSE:
		case GC_SYSTEMMENU:
		case GC_SCREENSHOT:
		case GC_RECORDGIF:
		case GC_VIEWPOINTNEXT:
		case GC_VIEWPOINTPREV:
		case GC_CAMTOGGLE:
		case GC_CAMRESET:
			return false;
		default:
			break;
	}
	return true;
}

static void HandleNonPlayerControlButton(INT32 gc)
{
	// Handle menu button
	if (gc == GC_SYSTEMMENU)
	{
		M_StartControlPanel();
		inputmethod = INPUTMETHOD_TOUCH;
	}
	// Handle console button
	else if (gc == GC_CONSOLE)
		CON_Toggle();
	// Handle pause button
	else if (gc == GC_PAUSE && !modeattacking)
		G_HandlePauseKey(true);
	// Handle spy mode
	else if (gc == GC_VIEWPOINTNEXT)
		G_DoViewpointSwitch(1);
	else if (gc == GC_VIEWPOINTPREV)
		G_DoViewpointSwitch(-1);
	// Handle screenshot
	else if (gc == GC_SCREENSHOT)
		M_ScreenShot();
	// Handle movie mode
	else if (gc == GC_RECORDGIF)
		((moviemode) ? M_StopMovie : M_StartMovie)();
	// Handle chasecam toggle
	else if (gc == GC_CAMTOGGLE)
		G_ToggleChaseCam();
	// Handle talk buttons
	else if ((gc == GC_TALKKEY || gc == GC_TEAMKEY) && netgame)
	{
		// Raise the screen keyboard if not muted
		boolean raise = (!CHAT_MUTE);

		// Only raise the screen keyboard in team games
		// if you're assigned to any team
		if (raise && (gc == GC_TEAMKEY))
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
void TS_HandleFingerEvent(event_t *ev)
{
	INT32 x = ev->x;
	INT32 y = ev->y;
	touchfinger_t *finger = &touchfingers[ev->key];
	boolean touchmotion = (ev->type == ev_touchmotion);
	boolean foundbutton = false;
	boolean movecamera = (!splitscreen);
	INT32 gc = finger->u.gamecontrol, i;

	if (TS_IsCustomizingControls())
		return;

	if (touch_fingerhandler)
		touch_fingerhandler(finger, ev);

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
			if (touchmotion && (finger->ignoremotion || (!TS_ButtonIsPlayerControl(gc))))
				break;

			// Lactozilla: Find every on-screen button and
			// check if they are below your finger.
			for (i = 0; i < NUM_GAMECONTROLS; i++)
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
				if (i == GC_JOYSTICK)
					continue;

				// Ignore d-pad if joystick is hidden
				if (btn->dpad && (touchcontrols[GC_JOYSTICK].hidden))
					continue;

				// In a touch motion event, simulate a key up event by clearing touchcontroldown.
				// This is done so that the buttons that are down don't 'stick'
				// if you move your finger from a button to another.
				if (touchmotion && (gc > GC_NULL))
				{
					// Let go of this button.
					touchcontroldown[gc] = 0;
					movecamera = false;
				}

				// Ignore disabled player controls
				if ((!touch_useinputs || promptblockcontrols) && TS_ButtonIsPlayerControl(i))
					continue;

				// Check if your finger touches this button.
				if (TS_FingerTouchesButton(x, y, btn) && (!touchcontroldown[i]))
				{
					finger->u.gamecontrol = i;
					touchcontroldown[i] = 1;
					controlmethod = INPUTMETHOD_TOUCH;
					foundbutton = true;
					break;
				}
			}

			// Check if your finger touches the joystick area.
			if (!foundbutton)
			{
				if (TS_FingerTouchesJoystickArea(x, y) && touch_useinputs && (!touchcontrols[GC_JOYSTICK].hidden))
				{
					// Joystick
					if (touch_movementstyle == tms_joystick && (!touchmotion))
					{
						finger->type.joystick = FINGERMOTION_JOYSTICK;
						finger->u.gamecontrol = -1;
						controlmethod = INPUTMETHOD_TOUCH;
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
				gc = GC_NULL;

				if (gamestate == GS_INTERMISSION || gamestate == GS_CUTSCENE)
					gc = GC_SPIN;
				else if (promptblockcontrols && F_GetPromptHideHud(y / vid.dupy))
					gc = GC_JUMP;

				if (gc != GC_NULL)
				{
					finger->ignoremotion = true;
					finger->u.gamecontrol = gc;
					touchcontroldown[gc] = 1;
					controlmethod = INPUTMETHOD_TOUCH;
					foundbutton = true;
				}
			}

			// The finger is moving either the joystick or the camera.
			if (!foundbutton)
			{
				INT32 dx = finger->dx;
				INT32 dy = finger->dy;

				controlmethod = INPUTMETHOD_TOUCH;

				if (touchmotion && finger->type.joystick) // Remember that this is an union!
				{
					INT32 movex = (INT32)(dx*((cv_touchcamhorzsens.value*cv_touchcamhorzsens.value)/110.0f + 0.1f));
					INT32 movey = (INT32)(dy*((cv_touchcamhorzsens.value*cv_touchcamhorzsens.value)/110.0f + 0.1f));

					// Joystick
					if (finger->type.joystick == FINGERMOTION_JOYSTICK)
					{
						float fx, fy;
						float xsens = FixedToFloat(cv_touchjoyhorzsens.value);
						float ysens = FixedToFloat(cv_touchjoyvertsens.value);
						INT32 padx = touch_joystick_x, pady = touch_joystick_y;
						INT32 padw = touch_joystick_w, padh = touch_joystick_h;

						padx *= vid.dupx;
						pady *= vid.dupy;
						padw *= vid.dupx;
						padh *= vid.dupy;
						TS_CenterCoords(&padx, &pady);

						fx = FixedToFloat((x * FRACUNIT) - (padx + (padw / 2)));
						fy = FixedToFloat((y * FRACUNIT) - (pady + (padh / 2)));

						touchxmove = (fx * xsens) / (FixedToFloat(touch_joystick_w) * (float)vid.dupx);
						touchymove = (fy * ysens) / (FixedToFloat(touch_joystick_h) * (float)vid.dupy);
						touchpressure = finger->pressure;
					}
					// Mouse
					else if (finger->type.mouse == FINGERMOTION_MOUSE && (touch_camera && movecamera))
					{
						mouse.rdx = movex;
						mouse.rdy = movey;
					}
				}
				else if (touch_camera && movecamera)
				{
					finger->type.mouse = FINGERMOTION_MOUSE;
					finger->u.gamecontrol = GC_NULL;
				}
			}
			break;

		case ev_touchup:
			// Let go of this finger.
			gc = finger->u.gamecontrol;
			if (gc > GC_NULL)
			{
				if (!TS_ButtonIsPlayerControl(gc) && TS_FingerTouchesButton(finger->x, finger->y, &touchcontrols[gc]))
					HandleNonPlayerControlButton(gc);
				touchcontroldown[gc] = 0;
			}

			finger->u.gamecontrol = GC_NULL;
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

INT32 TS_MapFingerEventToKey(event_t *event, INT32 *nav)
{
	INT32 i;

	// Check for any buttons
	if (event->type == ev_touchmotion) // Ignore motion events
		return KEY_NULL;

	for (i = 0; i < NUMTOUCHNAV; i++)
	{
		touchnavbutton_t *btn = &touchnavigation[i];

		// Ignore buttons that aren't defined
		if (!btn->defined)
			continue;

		// Check if your finger touches this button.
		if (TS_FingerTouchesNavigationButton(event->x, event->y, btn))
		{
			if (nav)
				(*nav) = i;
			return btn->key;
		}
	}

	return KEY_NULL;
}

void TS_UpdateFingers(INT32 realtics)
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
		// Mode Attack retry
		else if (modeattacking && finger->down && finger->u.gamecontrol == GC_PAUSE && G_InGameInput())
		{
			if (pausedelay < 0)
				finger->longpress = 0;
			else if (finger->longpress < TICRATE/2)
			{
				if (!finger->longpress && pausedelay < 1+(NEWTICRATE/2))
					pausedelay = 1+(NEWTICRATE/2);
				finger->longpress += realtics;
			}
			else if (G_CanRetryModeAttack())
				G_HandlePauseKey(false);
		}
	}
}

void TS_UpdateNavigation(INT32 realtics)
{
	INT32 i, tics;

	for (i = 0; i < NUMTOUCHNAV; i++)
	{
		touchnavbutton_t *btn = &touchnavigation[i];

		// Ignore buttons that aren't defined
		if (!btn->defined)
			continue;

		if (btn->down)
		{
			tics = btn->tics + realtics;
			btn->tics = min(tics, TS_NAVTICS);
		}
		else if (btn->tics)
		{
			tics = btn->tics - realtics;
			btn->tics = max(0, tics);
		}
	}
}

void TS_OnTouchEvent(INT32 id, evtype_t type, touchevent_t *event)
{
	touchfinger_t *finger = &touchfingers[id];

	finger->x = event->x;
	finger->y = event->y;
	finger->dx = event->dx;
	finger->dy = event->dy;
	finger->fx = event->fx;
	finger->fy = event->fy;
	finger->fdx = event->fdx;
	finger->fdy = event->fdy;
	finger->pressure = event->pressure;
	finger->longpress = 0;

	if (type == ev_touchdown)
		finger->down = true;
	else if (type == ev_touchup)
		finger->down = false;
}

void TS_ClearFingers(void)
{
	memset(touchfingers, 0x00, sizeof(touchfingers));
	memset(touchcontroldown, 0x00, sizeof(touchcontroldown));
	touchxmove = touchymove = touchpressure = 0.0f;
}

void TS_ClearNavigation(void)
{
	INT32 i;

	TS_NavigationFingersUp();

	for (i = 0; i < NUMTOUCHNAV; i++)
	{
		touchnavbutton_t *button = &touchnavigation[i];
		button->tics = 0;
	}
}

void TS_NavigationFingersUp(void)
{
	INT32 i;

	for (i = 0; i < NUMTOUCHNAV; i++)
	{
		touchnavbutton_t *button = &touchnavigation[i];
		button->down = false;
	}
}

void TS_GetSettings(void)
{
	if (!TS_Ready())
		return;

	touch_useinputs = cv_touchinputs.value && (!splitscreen);
	touch_movementstyle = cv_touchstyle.value;
	touch_camera = (cv_usemouse.value ? false : (!!cv_touchcamera.value));
	touch_preset = cv_touchpreset.value;
	touch_corners = cv_touchcorners.value;
	touch_preset_scale = cv_touchpresetscale.value;
	touch_scale_meta = cv_touchscalemeta.value;
}

void TS_UpdateControls(void)
{
	TS_GetSettings();
	TS_DefineButtons();
}

fixed_t TS_GetDefaultScale(void)
{
	return (fixed_t)(atof(cv_touchpresetscale.defaultvalue) * FRACUNIT);
}

void TS_GetJoystick(fixed_t *x, fixed_t *y, fixed_t *w, fixed_t *h, boolean tiny)
{
	if (tiny)
	{
		if (x)
			*x = 24 * FRACUNIT;
		if (y)
			*y = 128 * FRACUNIT;
		if (w)
			*w = 32 * FRACUNIT;
		if (h)
			*h = 32 * FRACUNIT;
	}
	else
	{
		if (x)
			*x = 24 * FRACUNIT;
		if (y)
			*y = 92 * FRACUNIT;
		if (w)
			*w = 64 * FRACUNIT;
		if (h)
			*h = 64 * FRACUNIT;
	}
}

void TS_DPadPreset(touchconfig_t *controls, fixed_t xscale, fixed_t yscale, fixed_t dw, boolean tiny)
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
	controls[GC_FORWARD].w = scaledw;
	controls[GC_FORWARD].h = scaledh;
	controls[GC_FORWARD].x = (touch_joystick_x + (dw / 2)) - FixedFloor(FixedMul(w, xscale) / 2) + middlealign;
	controls[GC_FORWARD].y = touch_joystick_y - scaledyoffs;

	// Down
	controls[GC_BACKWARD].w = scaledw;
	controls[GC_BACKWARD].h = scaledh;
	controls[GC_BACKWARD].x = (touch_joystick_x + (dw / 2)) - FixedFloor(FixedMul(w, xscale) / 2) + middlealign;
	controls[GC_BACKWARD].y = ((touch_joystick_y + touch_joystick_h) - controls[GC_BACKWARD].h) + scaledyoffs;

	// Left
	controls[GC_STRAFELEFT].w = scaledw;
	controls[GC_STRAFELEFT].h = scaledh;
	controls[GC_STRAFELEFT].x = touch_joystick_x - scaledxoffs;
	controls[GC_STRAFELEFT].y = (touch_joystick_y + (touch_joystick_h / 2)) - (controls[GC_STRAFELEFT].h / 2);

	// Right
	controls[GC_STRAFERIGHT].w = scaledw;
	controls[GC_STRAFERIGHT].h = scaledh;
	controls[GC_STRAFERIGHT].x = ((touch_joystick_x + touch_joystick_w) - controls[GC_STRAFERIGHT].w) + scaledxoffs;
	controls[GC_STRAFERIGHT].y = controls[GC_STRAFELEFT].y;

	// Up left
	controls[GC_DPADUL].w = scaledw;
	controls[GC_DPADUL].h = scaledh;
	controls[GC_DPADUL].x = controls[GC_FORWARD].x - diagxoffs;
	controls[GC_DPADUL].y = controls[GC_FORWARD].y + diagyoffs;

	// Up right
	controls[GC_DPADUR].w = scaledw;
	controls[GC_DPADUR].h = scaledh;
	controls[GC_DPADUR].x = controls[GC_FORWARD].x + diagxoffs;
	controls[GC_DPADUR].y = controls[GC_FORWARD].y + diagyoffs;

	// Down left
	controls[GC_DPADDL].w = scaledw;
	controls[GC_DPADDL].h = scaledh;
	controls[GC_DPADDL].x = controls[GC_BACKWARD].x - diagxoffs;
	controls[GC_DPADDL].y = controls[GC_BACKWARD].y - diagyoffs;

	// Down right
	controls[GC_DPADDR].w = scaledw;
	controls[GC_DPADDR].h = scaledh;
	controls[GC_DPADDR].x = controls[GC_BACKWARD].x + diagxoffs;
	controls[GC_DPADDR].y = controls[GC_BACKWARD].y - diagyoffs;
}

#define SCALECOORD(coord) FixedMul(coord, scale)

static void ScaleDPadBase(touchmovementstyle_e tms, touchconfigstatus_t *status, boolean tiny, fixed_t dx, fixed_t dy, fixed_t dw, fixed_t dh, fixed_t scale, fixed_t offs, fixed_t bottomalign)
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

		if (status)
		{
			if (status->altliveshud)
				touch_joystick_y += 8 * FRACUNIT;
			else if (status->ringslinger)
				touch_joystick_y -= 4 * FRACUNIT;

			if (status->specialstage && !status->nights)
				touch_joystick_y += 8 * FRACUNIT;
		}
	}
	else
	{
		if (tms == tms_joystick)
		{
			touch_joystick_x -= 12 * FRACUNIT;
			touch_joystick_y += 16 * FRACUNIT;
		}

		if (status)
		{
			if (status->altliveshud)
				touch_joystick_y += 16 * FRACUNIT;
			else if (status->ringslinger)
				touch_joystick_y -= 8 * FRACUNIT;

			if (status->specialstage && !status->nights)
				touch_joystick_y += 16 * FRACUNIT;
		}
	}
}

void TS_NormalizeButton(touchconfig_t *button)
{
	button->x = FixedDiv(button->x, BASEVIDWIDTH * FRACUNIT);
	button->y = FixedDiv(button->y, BASEVIDHEIGHT * FRACUNIT);
}

void TS_NormalizeConfig(touchconfig_t *config, int configsize)
{
	INT32 i;

	for (i = 0; i < configsize; i++)
	{
		touchconfig_t *button = &config[i];
		if (button->x || button->y)
			TS_NormalizeButton(button);
	}
}

static void TS_NormalizeNavigation(void)
{
	INT32 i;

	for (i = 0; i < NUMTOUCHNAV; i++)
	{
		touchnavbutton_t *button = &touchnavigation[i];
		if (button->defined)
		{
			button->x = FixedDiv(button->x, BASEVIDWIDTH * FRACUNIT);
			button->y = FixedDiv(button->y, BASEVIDHEIGHT * FRACUNIT);
		}
	}
}

void TS_DenormalizeCoords(fixed_t *x, fixed_t *y)
{
	*x *= BASEVIDWIDTH;
	*y *= BASEVIDHEIGHT;
}

static boolean LayoutIsWidescreen(void)
{
	if (TS_IsPresetActive())
		return true;

	return usertouchlayout->widescreen;
}

void TS_CenterCoords(fixed_t *x, fixed_t *y)
{
	if (!LayoutIsWidescreen())
	{
		INT32 dup = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
		if (vid.width != BASEVIDWIDTH * dup)
			*x += ((vid.width - (BASEVIDWIDTH * dup)) / 2) * FRACUNIT;
		if (vid.height != BASEVIDHEIGHT * dup)
			*y += ((vid.height - (BASEVIDHEIGHT * dup)) / 2) * FRACUNIT;
	}
}

void TS_CenterIntegerCoords(INT32 *x, INT32 *y)
{
	if (!LayoutIsWidescreen())
	{
		INT32 dup = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
		if (vid.width != BASEVIDWIDTH * dup)
			*x += (vid.width - (BASEVIDWIDTH * dup)) / 2;
		if (vid.height != BASEVIDHEIGHT * dup)
			*y += (vid.height - (BASEVIDHEIGHT * dup)) / 2;
	}
}

typedef struct
{
	INT32 gc;
	const char *name;
	const char *tinyname;
} touchbuttonname_t;

static touchbuttonname_t touchbuttonnames[] = {
	{GC_WEAPONNEXT, "WEP.NEXT", "WNX"},
	{GC_WEAPONPREV, "WEP.PREV", "WPV"},
	{GC_FIRE, "FIRE", "FRE"},
	{GC_FIRENORMAL, "F.NORMAL", "FRN"},
	{GC_TOSSFLAG, "TOSSFLAG", "FLG"},
	{GC_SPIN, "SPIN", "SPN"},
	{GC_CAMTOGGLE, "CHASECAM", "CHASE"},
	{GC_CAMRESET, "RESET CAM", "R.CAM"},
	{GC_LOOKUP, "LOOK UP", "L.UP"},
	{GC_LOOKDOWN, "LOOK DOWN", "L.DW"},
	{GC_CENTERVIEW, "CENTER VIEW", "CVW"},
	{GC_MOUSEAIMING, "MOUSEAIM", "AIM"},
	{GC_TALKKEY, "TALK", "TLK"},
	{GC_TEAMKEY, "TEAM", "TTK"},
	{GC_SCORES, "SCORES", "TAB"},
	{GC_JUMP, "JUMP", "JMP"},
	{GC_CONSOLE, "CONSOLE", "CON"},
	{GC_SYSTEMMENU, "MENU", "MNU"},
	{GC_SCREENSHOT, "SCRCAP", "SCR"},
	{GC_RECORDGIF, "REC", NULL},
	{GC_VIEWPOINTNEXT, "NEXT VIEW", "V.NTX"},
	{GC_VIEWPOINTPREV, "PREV VIEW", "V.PRV"},
	{GC_CUSTOM1, "CUSTOM1", "C1"},
	{GC_CUSTOM2, "CUSTOM2", "C2"},
	{GC_CUSTOM3, "CUSTOM3", "C3"},
	{GC_NULL, NULL, NULL}
};

static touchbuttonname_t replaytouchbuttonnames[] = {
	{GC_SYSTEMMENU, "EXIT", "ESC"},
	{GC_NULL, NULL, NULL}
};

static touchbuttonname_t nightstouchbuttonnames[] = {
	{GC_SPIN, "BRAKE", "BRK"},
	{GC_JUMP, "DRILL", "DRL"},
	{GC_NULL, NULL, NULL}
};

const char *TS_GetButtonName(INT32 gc, touchconfigstatus_t *status)
{
	INT32 i;

	if (modeattacking && demoplayback)
	{
		for (i = 0; (replaytouchbuttonnames[i].gc != GC_NULL); i++)
		{
			if (replaytouchbuttonnames[i].gc == gc)
				return replaytouchbuttonnames[i].name;
		}
	}

	if (status && status->nights)
	{
		for (i = 0; (nightstouchbuttonnames[i].gc != GC_NULL); i++)
		{
			if (nightstouchbuttonnames[i].gc == gc)
				return nightstouchbuttonnames[i].name;
		}
	}

	for (i = 0; (touchbuttonnames[i].gc != GC_NULL); i++)
	{
		if (touchbuttonnames[i].gc == gc)
			return touchbuttonnames[i].name;
	}

	return NULL;
}

const char *TS_GetButtonShortName(INT32 gc, touchconfigstatus_t *status)
{
	INT32 i;

	if (status && status->nights)
	{
		for (i = 0; (nightstouchbuttonnames[i].gc != GC_NULL); i++)
		{
			if (nightstouchbuttonnames[i].gc == gc)
				return nightstouchbuttonnames[i].tinyname;
		}
	}

	for (i = 0; (touchbuttonnames[i].gc != GC_NULL); i++)
	{
		if (touchbuttonnames[i].gc == gc)
			return touchbuttonnames[i].tinyname;
	}

	return NULL;
}

void TS_SetButtonNames(touchconfig_t *controls, touchconfigstatus_t *status)
{
	INT32 i;

	for (i = 0; i < NUM_GAMECONTROLS; i++)
	{
		controls[i].name = TS_GetButtonName(i, status);
		controls[i].tinyname = TS_GetButtonShortName(i, status);
	}
}

boolean TS_IsDPadButton(INT32 gc)
{
	switch (gc)
	{
		case GC_FORWARD:
		case GC_BACKWARD:
		case GC_STRAFELEFT:
		case GC_STRAFERIGHT:
		case GC_DPADUL:
		case GC_DPADUR:
		case GC_DPADDL:
		case GC_DPADDR:
			return true;
		default:
			break;
	}

	return false;
}

void TS_MarkDPadButtons(touchconfig_t *controls)
{
	INT32 i;

	for (i = 0; i < NUM_GAMECONTROLS; i++)
	{
		if (TS_IsDPadButton(i))
			controls[i].dpad = true;
	}
}

void TS_BuildPreset(touchconfig_t *controls, touchconfigstatus_t *status,
					touchmovementstyle_e tms, fixed_t scale,
					boolean tiny, boolean widescreen)
{
	fixed_t x, y, w, h;
	fixed_t dx, dy, dw, dh;
	fixed_t corneroffset = 4 * FRACUNIT;
	fixed_t topcorner, bottomcorner, rightcorner;
	fixed_t jsoffs = status->ringslinger ? (-4 * FRACUNIT) : 0, jumph;
	fixed_t offs = 0;
	fixed_t promptoffs = 0;
	fixed_t nonjoyoffs = -12 * FRACUNIT;
	fixed_t bottomalign = 0;

	touchconfig_t *ref;
	boolean bothvisible;
	boolean eithervisible;
	boolean specialstage = (status->nights || status->specialstage);

	if (status->tutorialmode && status->promptactive)
	{
		y = F_GetPromptHideHudBound();
		if (y < 0)
			promptoffs = -(y / vid.dupy);
	}

	if (widescreen)
	{
		rightcorner = (((vid.width / vid.dupx) - status->corners) * FRACUNIT);
		bottomcorner = (((vid.height / vid.dupy) - promptoffs) * FRACUNIT);
	}
	else
	{
		rightcorner = (BASEVIDWIDTH - status->corners) * FRACUNIT;
		bottomcorner = (BASEVIDHEIGHT - promptoffs) * FRACUNIT;
	}

	if (status->altliveshud)
	{
		y = (16 * FRACUNIT);
		if (status->splitscreen)
			y /= 2;
		topcorner = (ST_GetLivesHUDInfo()->y * FRACUNIT) + y + corneroffset;
	}
	else
		topcorner = corneroffset;

	// For the D-Pad
	if (widescreen && (vid.height != BASEVIDHEIGHT * vid.dupy))
		bottomalign = ((vid.height - (BASEVIDHEIGHT * vid.dupy)) / vid.dupy) * FRACUNIT;

	TS_GetJoystick(&dx, &dy, &dw, &dh, tiny);
	dx += (status->corners * FRACUNIT);
	dy -= (promptoffs * FRACUNIT);

	// D-Pad
	ScaleDPadBase(tms, status, tiny, dx, dy, dw, dh, scale, offs, bottomalign);
	TS_DPadPreset(
		controls,
		FixedMul(FixedDiv(touch_joystick_w, dw), scale),
		FixedMul(FixedDiv(touch_joystick_h, dh), scale),
		touch_joystick_w, tiny);

	controls[GC_JOYSTICK].x = touch_joystick_x;
	controls[GC_JOYSTICK].y = touch_joystick_y;
	controls[GC_JOYSTICK].w = touch_joystick_w;
	controls[GC_JOYSTICK].h = touch_joystick_h;

	if (specialstage)
		jsoffs += (16 * FRACUNIT);

	if (status->nights && status->modeattacking)
		jsoffs -= (24 * FRACUNIT);

	// Jump and spin
	if (tiny)
	{
		// Jump
		w = 40 * FRACUNIT;
		h = jumph = 32 * FRACUNIT;
		controls[GC_JUMP].w = SCALECOORD(w);
		controls[GC_JUMP].h = SCALECOORD(h);
		controls[GC_JUMP].x = (rightcorner - controls[GC_JUMP].w - corneroffset - (12 * FRACUNIT));
		controls[GC_JUMP].y = (bottomcorner - controls[GC_JUMP].h - corneroffset - (12 * FRACUNIT)) + jsoffs + offs + nonjoyoffs;

		// Spin
		w = 32 * FRACUNIT;
		h = 24 * FRACUNIT;
		controls[GC_SPIN].w = SCALECOORD(w);
		controls[GC_SPIN].h = SCALECOORD(h);
		controls[GC_SPIN].x = (controls[GC_JUMP].x - controls[GC_SPIN].w - (12 * FRACUNIT));
		controls[GC_SPIN].y = controls[GC_JUMP].y + (8 * FRACUNIT);
	}
	else
	{
		// Jump
		w = 48 * FRACUNIT;
		h = jumph = 48 * FRACUNIT;
		controls[GC_JUMP].w = SCALECOORD(w);
		controls[GC_JUMP].h = SCALECOORD(h);
		controls[GC_JUMP].x = (rightcorner - controls[GC_JUMP].w - corneroffset - (12 * FRACUNIT));
		controls[GC_JUMP].y = (bottomcorner - controls[GC_JUMP].h - corneroffset - (12 * FRACUNIT)) + jsoffs + offs + nonjoyoffs;

		// Spin
		w = 40 * FRACUNIT;
		h = 32 * FRACUNIT;
		controls[GC_SPIN].w = SCALECOORD(w);
		controls[GC_SPIN].h = SCALECOORD(h);
		controls[GC_SPIN].x = (controls[GC_JUMP].x - controls[GC_SPIN].w - (12 * FRACUNIT));
		controls[GC_SPIN].y = controls[GC_JUMP].y + (12 * FRACUNIT);
	}

	// Fire, fire normal, and toss flag
	if (status->ringslinger)
	{
		offs = SCALECOORD(8 * FRACUNIT);
		h = SCALECOORD(jumph / 2) + SCALECOORD(4 * FRACUNIT);
		controls[GC_SPIN].h = h;
		controls[GC_SPIN].y = ((controls[GC_JUMP].y + controls[GC_JUMP].h) - h) + offs;

		controls[GC_FIRE].w = controls[GC_SPIN].w;
		controls[GC_FIRE].h = h;
		controls[GC_FIRE].x = controls[GC_SPIN].x;
		controls[GC_FIRE].y = controls[GC_JUMP].y - offs;

		if (status->ctfgametype)
		{
			ref = &controls[GC_TOSSFLAG];
			controls[GC_FIRENORMAL].hidden = true;
		}
		else
		{
			ref = &controls[GC_FIRENORMAL];
			controls[GC_TOSSFLAG].hidden = true;
		}

		ref->w = controls[GC_JUMP].w;
		ref->h = controls[GC_FIRE].h;
		if (!tiny)
			ref->h = (ref->h / 2);

		ref->x = controls[GC_JUMP].x;
		ref->y = controls[GC_JUMP].y - ref->h - SCALECOORD(4 * FRACUNIT);
	}
	else
	{
		controls[GC_FIRE].hidden = true;
		controls[GC_FIRENORMAL].hidden = true;
		controls[GC_TOSSFLAG].hidden = true;
	}

	//
	// Non-control buttons
	//

	if (!status->scalemeta)
		scale = TS_GetDefaultScale();

	offs = SCALECOORD(8 * FRACUNIT);

	// Menu
	controls[GC_SYSTEMMENU].w = SCALECOORD(32 * FRACUNIT);
	controls[GC_SYSTEMMENU].h = SCALECOORD(32 * FRACUNIT);
	controls[GC_SYSTEMMENU].x = (rightcorner - controls[GC_SYSTEMMENU].w - corneroffset);
	controls[GC_SYSTEMMENU].y = (topcorner + (specialstage ? 40 * FRACUNIT : 0));

	// Pause
	controls[GC_PAUSE].x = controls[GC_SYSTEMMENU].x;
	controls[GC_PAUSE].y = controls[GC_SYSTEMMENU].y;
	controls[GC_PAUSE].w = SCALECOORD(24 * FRACUNIT);
	controls[GC_PAUSE].h = SCALECOORD(24 * FRACUNIT);
	if (status->canpause)
		controls[GC_PAUSE].x -= (controls[GC_PAUSE].w + SCALECOORD(4 * FRACUNIT));
	else
		controls[GC_PAUSE].hidden = true;

	// Spy mode
	controls[GC_VIEWPOINTNEXT].hidden = true;
	controls[GC_VIEWPOINTNEXT].x = controls[GC_PAUSE].x;
	controls[GC_VIEWPOINTNEXT].y = controls[GC_PAUSE].y;
	if (status->canviewpointswitch)
	{
		controls[GC_VIEWPOINTNEXT].w = SCALECOORD(32 * FRACUNIT);
		controls[GC_VIEWPOINTNEXT].h = SCALECOORD(24 * FRACUNIT);
		controls[GC_VIEWPOINTNEXT].x -= (controls[GC_VIEWPOINTNEXT].w + SCALECOORD(4 * FRACUNIT));
		controls[GC_VIEWPOINTNEXT].hidden = false;
	}

	// Align screenshot and movie mode buttons
	w = SCALECOORD(40 * FRACUNIT);
	h = SCALECOORD(24 * FRACUNIT);

	bothvisible = ((!controls[GC_VIEWPOINTNEXT].hidden) && (!controls[GC_PAUSE].hidden));
	eithervisible = (controls[GC_VIEWPOINTNEXT].hidden ^ controls[GC_PAUSE].hidden);

	if (bothvisible || eithervisible)
	{
		fixed_t left, top;

		if (!status->cantalk)
			ref = &controls[GC_SYSTEMMENU];
		else if (bothvisible)
			ref = &controls[GC_PAUSE];
		else // only if one of either are visible, but not both
			ref = (controls[GC_VIEWPOINTNEXT].hidden ? &controls[GC_PAUSE] : &controls[GC_VIEWPOINTNEXT]);

		if (specialstage && !status->modeattacking
		&& !(!controls[GC_VIEWPOINTNEXT].hidden || !controls[GC_PAUSE].hidden))
		{
			left = rightcorner;
			top = ref->y + ref->h + SCALECOORD(4 * FRACUNIT);
		}
		else
		{
			left = ref->x;
			top = ref->y;
		}

		x = left - (w - ref->w);
		y = top + ref->h + offs;
	}
	else
	{
		boolean bothhidden = (controls[GC_PAUSE].hidden && controls[GC_VIEWPOINTNEXT].hidden);
		boolean usemenupos = (specialstage && (!status->cantalk && bothhidden));
		boolean usespecialstagepos = (!usemenupos) && (specialstage && !(status->cantalk && bothhidden));
		fixed_t left, top;

		ref = &controls[usemenupos ? GC_SYSTEMMENU : GC_VIEWPOINTNEXT];
		left = usespecialstagepos ? rightcorner : ref->x;
		top = usespecialstagepos ? ref->h + SCALECOORD(4 * FRACUNIT) : 0;

		x = (left - w - SCALECOORD(4 * FRACUNIT));
		y = ref->y + top;
	}

	controls[GC_SCREENSHOT].w = controls[GC_RECORDGIF].w = w;
	controls[GC_SCREENSHOT].h = controls[GC_RECORDGIF].h = h;

	// Screenshot
	controls[GC_SCREENSHOT].x = x;
	controls[GC_SCREENSHOT].y = y;

	// Movie mode
	controls[GC_RECORDGIF].x = x;
	controls[GC_RECORDGIF].y = (controls[GC_SCREENSHOT].y + controls[GC_SCREENSHOT].h + offs);

	// Talk key and team talk key
	controls[GC_TALKKEY].hidden = true; // hidden by default
	controls[GC_TEAMKEY].hidden = true; // hidden outside of team games

	// if netgame + chat not muted
	if (status->cantalk)
	{
		controls[GC_TALKKEY].w = SCALECOORD(32 * FRACUNIT);
		controls[GC_TALKKEY].h = SCALECOORD(24 * FRACUNIT);
		controls[GC_TALKKEY].x = (rightcorner - controls[GC_TALKKEY].w - corneroffset);
		controls[GC_TALKKEY].y = (controls[GC_SYSTEMMENU].y + controls[GC_SYSTEMMENU].h + offs);
		controls[GC_TALKKEY].hidden = false;

		if (status->canteamtalk)
		{
			controls[GC_TEAMKEY].w = SCALECOORD(32 * FRACUNIT);
			controls[GC_TEAMKEY].h = SCALECOORD(24 * FRACUNIT);
			controls[GC_TEAMKEY].x = controls[GC_TALKKEY].x;
			controls[GC_TEAMKEY].y = controls[GC_TALKKEY].y + controls[GC_TALKKEY].h + offs;
			controls[GC_TEAMKEY].hidden = false;
		}
	}

	// Normalize all buttons
	TS_NormalizeConfig(controls, NUM_GAMECONTROLS);

	// Mark movement controls as d-pad buttons
	TS_MarkDPadButtons(controls);

	// Set button names
	TS_SetButtonNames(controls, status);

	// Mark undefined buttons as hidden
	for (x = 0; x < NUM_GAMECONTROLS; x++)
	{
		if (!controls[x].w)
			controls[x].hidden = true;
	}
}

#undef SCALECOORD

static void BuildWeaponButtons(touchconfigstatus_t *status)
{
	INT32 i, wep, dup = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
	fixed_t x, y, w, h;

	x = (ST_WEAPONS_X * FRACUNIT) + (6 * FRACUNIT);
	y = (ST_WEAPONS_Y * FRACUNIT) - (2 * FRACUNIT);
	w = ST_WEAPONS_W * FRACUNIT;
	h = ST_WEAPONS_H * FRACUNIT;

	wep = GC_WEPSLOT1;

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

		TS_NormalizeButton(&touchcontrols[wep]);

		wep++;
	}
}

static void HidePlayerControlButtons(touchconfigstatus_t *status)
{
	if (status->promptblockcontrols)
	{
		INT32 i;
		for (i = 0; i < NUM_GAMECONTROLS; i++)
		{
			if (TS_ButtonIsPlayerControl(i))
				touchcontrols[i].hidden = true;
		}
	}
}

void TS_PositionButtons(void)
{
	touchconfigstatus_t *status = &touchcontrolstatus;

	// Build preset
	if (TS_IsPresetActive())
	{
		memset(touchcontrols, 0x00, sizeof(touchconfig_t) * NUM_GAMECONTROLS);
		TS_BuildPreset(touchcontrols, status, touch_movementstyle, touch_preset_scale, (touch_preset == touchpreset_tiny), true);
	}

	// Weapon select slots
	BuildWeaponButtons(status);

	// Hide movement controls in prompts that block controls
	HidePlayerControlButtons(status);

	// Mark movement controls as d-pad buttons
	TS_MarkDPadButtons(touchcontrols);
}

void TS_PositionExtraUserButtons(void)
{
	// Position joystick
	touch_joystick_x = touchcontrols[GC_JOYSTICK].x;
	touch_joystick_y = touchcontrols[GC_JOYSTICK].y;

	TS_DenormalizeCoords(&touch_joystick_x, &touch_joystick_y);

	touch_joystick_w = touchcontrols[GC_JOYSTICK].w;
	touch_joystick_h = touchcontrols[GC_JOYSTICK].h;

	// Weapon select slots
	BuildWeaponButtons(&usertouchconfigstatus);

	// Hide movement controls in prompts that block controls
	HidePlayerControlButtons(&usertouchconfigstatus);

	// Mark movement controls as d-pad buttons
	TS_MarkDPadButtons(usertouchcontrols);
}

void TS_PresetChanged(void)
{
	if (!TS_Ready())
		return;

	// set touch_preset
	TS_GetSettings();

	if (!TS_IsPresetActive())
	{
		// Make a default custom controls set
		TS_DefaultControlLayout(true);

		// Copy custom controls
		M_Memcpy(&touchcontrols, usertouchcontrols, sizeof(touchconfig_t) * NUM_GAMECONTROLS);

		// Position joystick, weapon buttons, etc.
		TS_PositionExtraUserButtons();

		// Mark movement controls as d-pad buttons
		TS_MarkDPadButtons(touchcontrols);
	}
	else
		TS_DefineButtons();
}

void TS_PositionNavigation(void)
{
	touchnavstatus_t *status = &touchnavigationstatus;
	touchnavbutton_t *nav = touchnavigation;
	fixed_t hcorner = 4 * FRACUNIT, vcorner = hcorner;
	fixed_t basebtnsize = 24 * FRACUNIT;
	fixed_t btnsize = basebtnsize;
	INT32 dupz = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);

	touchnavbutton_t *back = &nav[TOUCHNAV_BACK];
	touchnavbutton_t *confirm = &nav[TOUCHNAV_CONFIRM];
	touchnavbutton_t *con = &nav[TOUCHNAV_CONSOLE];
	touchnavbutton_t *del = &nav[TOUCHNAV_DELETE];

	if (vid.width != BASEVIDWIDTH * vid.dupx)
	{
		INT32 left = (vid.width - (BASEVIDWIDTH * dupz)) / 2;
		INT32 top = (vid.height - (BASEVIDHEIGHT * dupz)) / 2;
		fixed_t size;

		if (top > left)
			size = (top * FRACUNIT) - ((vcorner + basebtnsize) * dupz);
		else
			size = (left * FRACUNIT) - ((hcorner * 2 + basebtnsize) * dupz);

		size /= dupz;

		if (size > 0)
			btnsize += size;
	}

	hcorner += (status->corners * FRACUNIT);

	// Back
	back->key = KEY_ESCAPE;
	back->shadow = false;
	back->dontscaletext = (btnsize == basebtnsize);
	back->defined = true;
	back->patch = NULL;

	if (!status->canreturn)
		back->defined = false;
	else if (status->customizingcontrols)
	{
		back->name = "\x1C";
		back->w = 16 * FRACUNIT;
		back->h = 16 * FRACUNIT;
		back->color = 35;
		back->pressedcolor = back->color + 3;
		back->dontscaletext = true;
	}
	else
	{
		fixed_t y = vcorner;
		fixed_t test = 0;

		back->patch = "NAV_BACK";
		back->shadow = true;
		back->w = back->h = btnsize;

		if (status->returncorner & TSNAV_CORNER_RIGHT)
			back->x = (((vid.width / vid.dupx) * FRACUNIT) - confirm->w) - hcorner;
		else
			back->x = hcorner;

		if (vid.height != BASEVIDHEIGHT * dupz)
			test = (BASEVIDHEIGHT * dupz) * FRACUNIT;

		if ((status->returncorner & TSNAV_CORNER_BOTTOM)
		|| (y > test && status->returncorner & TSNAV_CORNER_TOP_TEST))
			back->y = (((vid.height / vid.dupy) * FRACUNIT) - back->h) - vcorner;
		else
			back->y = y;
	}

	// Confirm
	confirm->key = KEY_ENTER;
	confirm->shadow = false;
	confirm->dontscaletext = (btnsize == basebtnsize);
	confirm->defined = true;
	confirm->patch = NULL;

	if (status->layoutsubmenuopen || !status->canconfirm)
		confirm->defined = false;
	else if (status->customizingcontrols)
	{
		confirm->name = "+";
		confirm->w = 32 * FRACUNIT;
		confirm->h = 16 * FRACUNIT;
		confirm->x = (((vid.width / (vid.dupx * 2)) * FRACUNIT) - (confirm->w / 2));
		confirm->color = 112;
		confirm->pressedcolor = confirm->color + 3;
		confirm->dontscaletext = true;
	}
	else
	{
		confirm->patch = "NAV_CONFIRM";
		confirm->shadow = true;
		confirm->w = confirm->h = btnsize;
		confirm->x = (((vid.width / vid.dupx) * FRACUNIT) - confirm->w) - hcorner;
		confirm->y = vcorner;
	}

	// Console
	con->key = KEY_CONSOLE;
	con->defined = status->canopenconsole;

	if (con->defined)
	{
		fixed_t y = vcorner;

		con->patch = "NAV_CONSOLE";
		con->x = hcorner;

		con->h = max(24 * FRACUNIT, btnsize / 2);

		if (back->defined)
			y = back->y + back->h + (8 * FRACUNIT);

		if (vid.height > vid.width)
		{
			fixed_t test = 0;

			if (vid.height != BASEVIDHEIGHT * dupz)
				test = (BASEVIDHEIGHT * dupz) * FRACUNIT;

			if (test > 0 && con->h < test)
				con->y = (((vid.height / vid.dupy) * FRACUNIT) - con->h) - (vcorner * 4);
			else
				con->y = y;
		}
		else
			con->y = y;

		if (M_TSNav_OnMainMenu())
			con->w = FixedMul(con->h, FixedDiv(32 * FRACUNIT, 24 * FRACUNIT));
		else
		{
			con->patch = "NAV_CONSOLE_SHORT";
			con->w = con->h;
		}
	}

	// Delete
	del->key = status->showdelete;
	del->defined = (del->key != KEY_NULL);
	del->snapflags = 0;

	if (del->defined)
	{
		del->w = 32 * FRACUNIT;
		del->h = 24 * FRACUNIT;

		if (currentMenu == &SP_LoadDef)
		{
			del->name = "DELETE";
			del->w += 24 * FRACUNIT;
		}
		else
		{
			del->name = "DEL";
			del->snapflags |= V_SNAPTOBOTTOM;
		}

		del->x = hcorner;
		del->y = (((vid.height / vid.dupy) * FRACUNIT) - del->h) - vcorner;

		del->color = 35;
		del->pressedcolor = del->color + 3;
	}

	// Normalize all buttons
	TS_NormalizeNavigation();
}

void TS_HideNavigationButtons(void)
{
	INT32 i = 0;
	for (; i < NUMTOUCHNAV; i++)
		touchnavigation[i].defined = false;
}

void TS_DefineButtons(void)
{
	static touchconfigstatus_t status;
	size_t size = sizeof(touchconfigstatus_t);

	if (!TS_Ready())
		return;

	if (TS_IsPresetActive())
	{
		status.vid.width = vid.width;
		status.vid.height = vid.height;
		status.vid.dupx = vid.dupx;
		status.vid.dupy = vid.dupy;
		status.presetscale = touch_preset_scale;
		status.scalemeta = touch_scale_meta;
		status.corners = touch_corners;

		status.preset = touch_preset;
		status.movementstyle = touch_movementstyle;

		status.ringslinger = G_RingSlingerGametype();
		status.ctfgametype = (gametyperules & GTR_TEAMFLAGS);
		status.nights = (maptol & TOL_NIGHTS);
		status.specialstage = G_IsSpecialStage(gamemap);
		status.tutorialmode = tutorialmode;
		status.splitscreen = splitscreen;
		status.modeattacking = modeattacking;
		status.canpause = ((netgame && (cv_pause.value || server || IsPlayerAdmin(consoleplayer))) || (modeattacking && demorecording));
		status.canviewpointswitch = G_CanViewpointSwitch(false);
		status.cantalk = (netgame && !CHAT_MUTE);
		status.canteamtalk = (G_GametypeHasTeams() && players[consoleplayer].ctfteam);
		status.promptactive = promptactive;
		status.promptblockcontrols = promptblockcontrols;

		if (F_GetPromptHideHud(ST_GetLivesHUDInfo()->y))
			status.altliveshud = false;
		else
			status.altliveshud = ST_AltLivesHUDEnabled() && (!(status.nights || status.specialstage)) && (gamestate == GS_LEVEL);

		if (memcmp(&status, &touchcontrolstatus, size))
		{
			M_Memcpy(&touchcontrolstatus, &status, size);

			TS_PositionButtons();

			if (!TS_IsPresetActive())
				TS_PresetChanged();
		}
	}
	else
	{
		status.vid.width = vid.width;
		status.vid.height = vid.height;
		status.vid.dupx = vid.dupx;
		status.vid.dupy = vid.dupy;
		status.preset = touch_preset;
		status.promptblockcontrols = promptblockcontrols;

		if (memcmp(&status, &touchcontrolstatus, size))
		{
			M_Memcpy(&touchcontrolstatus, &status, size);
			TS_PresetChanged();
		}
	}

	TS_DefineNavigationButtons();
}

void TS_DefineNavigationButtons(void)
{
	static touchnavstatus_t navstatus;
	size_t size = sizeof(touchnavstatus_t);

	if (!TS_Ready())
		return;

	navstatus.vid.width = vid.width;
	navstatus.vid.height = vid.height;
	navstatus.vid.dupx = vid.dupx;
	navstatus.vid.dupy = vid.dupy;
	navstatus.corners = cv_touchcorners.value;

	navstatus.customizingcontrols = TS_IsCustomizingControls();
	navstatus.layoutsubmenuopen = TS_IsCustomizationSubmenuOpen();
	navstatus.canreturn = M_TSNav_CanShowBack();
	navstatus.canconfirm = M_TSNav_CanShowConfirm();
	navstatus.showdelete = M_TSNav_DeleteButtonAction();
	navstatus.returncorner = M_TSNav_BackCorner();

	if (modeattacking || metalrecording || marathonmode || navstatus.customizingcontrols)
		navstatus.canopenconsole = false;
	else
		navstatus.canopenconsole = M_TSNav_CanShowConsole();

	if (memcmp(&navstatus, &touchnavigationstatus, size))
	{
		M_Memcpy(&touchnavigationstatus, &navstatus, size);
		TS_PositionNavigation();
	}
}

#endif // TOUCHINPUTS
