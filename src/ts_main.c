// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
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
#include "ts_custom.h"

#include "g_game.h" // players[MAXPLAYERS], promptactive, promptblockcontrols

#include "m_menu.h" // M_IsCustomizingTouchControls
#include "m_misc.h" // M_ScreenShot
#include "console.h" // CON_Toggle

#include "st_stuff.h"
#include "hu_stuff.h" // chat (:LMFAOOO1:)

#ifdef TOUCHINPUTS
boolean ts_ready = false;

boolean TS_Ready(void)
{
	return ts_ready;
}

float touchxmove, touchymove, touchpressure;

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
consvar_t cv_showfingers = {"showfingers", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

// Finger event handler
void (*touch_fingerhandler)(touchfinger_t *, event_t *) = NULL;

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

consvar_t cv_touchstyle = {"touch_movementstyle", "Joystick", CV_SAVE|CV_CALL|CV_NOINIT, touchstyle_cons_t, TS_UpdateControls, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_touchpreset = {"touch_preset", "Default", CV_SAVE|CV_CALL|CV_NOINIT, touchpreset_cons_t, TS_PresetChanged, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_touchlayout = {"touch_layout", "None", CV_SAVE|CV_CALL|CV_NOINIT, NULL, TS_LoadLayoutFromCVar, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_touchcamera = {"touch_camera", "On", CV_SAVE|CV_CALL|CV_NOINIT, CV_OnOff, TS_UpdateControls, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t touchguiscale_cons_t[] = {{FRACUNIT/2, "MIN"}, {3 * FRACUNIT, "MAX"}, {0, NULL}};
consvar_t cv_touchguiscale = {"touch_guiscale", "0.75", CV_FLOAT|CV_SAVE|CV_CALL|CV_NOINIT, touchguiscale_cons_t, TS_UpdateControls, 0, NULL, NULL, 0, 0, NULL};

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

boolean TS_IsCustomizingControls(void)
{
	return M_IsCustomizingTouchControls();
}

void TS_RegisterVariables(void)
{
	// Display settings
	CV_RegisterVar(&cv_showfingers);
	CV_RegisterVar(&cv_touchmenutrans);
	CV_RegisterVar(&cv_touchtrans);

	// Layout settings
	CV_RegisterVar(&cv_touchlayoutusegrid);

	// Preset settings
	CV_RegisterVar(&cv_touchguiscale);

	// Sensitivity settings
	CV_RegisterVar(&cv_touchsens);
	CV_RegisterVar(&cv_touchvertsens);
	CV_RegisterVar(&cv_touchjoyvertsens);
	CV_RegisterVar(&cv_touchjoyhorzsens);

	// Main options
	CV_RegisterVar(&cv_touchcamera);
	CV_RegisterVar(&cv_touchpreset);
	CV_RegisterVar(&cv_touchlayout);
	CV_RegisterVar(&cv_touchstyle);
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

static boolean IsFingerTouchingButton(INT32 x, INT32 y, touchconfig_t *btn, boolean center)
{
	INT32 tx = btn->x, ty = btn->y, tw = btn->w, th = btn->h;
	TS_ScaleCoords(&tx, &ty, &tw, &th, true, (!btn->dontscale));
	if (center && (!btn->dontscale))
		TS_CenterIntegerCoords(&tx, &ty);
	return (x >= tx && x <= tx + tw && y >= ty && y <= ty + th);
}

boolean TS_FingerTouchesButton(INT32 x, INT32 y, touchconfig_t *btn)
{
	return IsFingerTouchingButton(x, y, btn, true);
}

boolean TS_FingerTouchesNavigationButton(INT32 x, INT32 y, touchconfig_t *btn)
{
	return IsFingerTouchingButton(x, y, btn, false);
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

static void HandleNonPlayerControlButton(INT32 gc)
{
	// Handle menu button
	if (gc == gc_systemmenu)
		M_StartControlPanel();
	// Handle console button
	else if (gc == gc_console)
		CON_Toggle();
	// Handle pause button
	else if (gc == gc_pause)
		G_HandlePauseKey(true);
	// Handle spy mode
	else if (gc == gc_viewpoint)
		G_DoViewpointSwitch();
	// Handle screenshot
	else if (gc == gc_screenshot)
		M_ScreenShot();
	// Handle movie mode
	else if (gc == gc_recordgif)
		((moviemode) ? M_StopMovie : M_StartMovie)();
	// Handle chasecam toggle
	else if (gc == gc_camtoggle)
		G_ToggleChaseCam();
	// Handle talk buttons
	else if ((gc == gc_talkkey || gc == gc_teamkey) && netgame)
	{
		// Raise the screen keyboard if not muted
		boolean raise = (!CHAT_MUTE);

		// Only raise the screen keyboard in team games
		// if you're assigned to any team
		if (raise && (gc == gc_teamkey))
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
	boolean movecamera = true;
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
				if (TS_FingerTouchesButton(x, y, btn) && (!touchcontroldown[i]))
				{
					finger->u.gamecontrol = i;
					touchcontroldown[i] = 1;
					foundbutton = true;
					break;
				}
			}

			// Check if your finger touches the joystick area.
			if (!foundbutton)
			{
				if (TS_FingerTouchesJoystickArea(x, y) && (!touchcontrols[gc_joystick].hidden))
				{
					// Joystick
					if (touch_movementstyle == tms_joystick && (!touchmotion))
					{
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

						TS_ScaleCoords(&padx, &pady, &padw, &padh, false, true);
						TS_CenterIntegerCoords(&padx, &pady);

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
				}
				else if (touch_camera && movecamera)
				{
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
				if (!TS_ButtonIsPlayerControl(gc) && TS_FingerTouchesButton(finger->x, finger->y, &touchcontrols[gc]))
					HandleNonPlayerControlButton(gc);
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

INT32 TS_MapFingerEventToKey(event_t *event)
{
	INT32 i;

	// Check for any buttons
	if (event->type == ev_touchmotion) // Ignore motion events
		return KEY_NULL;

	for (i = 0; i < NUMKEYS; i++)
	{
		touchconfig_t *btn = &touchnavigation[i];

		// Ignore hidden buttons
		if (btn->hidden)
			continue;

		// Check if your finger touches this button.
		if (TS_FingerTouchesNavigationButton(event->x, event->y, btn))
			return i;
	}

	return KEY_NULL;
}

void TS_UpdateFingers(INT32 realtics)
{
	INT32 i;

	for (i = 0; i < NUMTOUCHFINGERS; i++)
	{
		touchfinger_t *finger = &touchfingers[i];

		finger->lastx = finger->x;
		finger->lasty = finger->y;

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

void TS_PostFingerEvent(event_t *event)
{
	touchfinger_t *finger = &touchfingers[event->key];

	finger->x = event->x;
	finger->y = event->y;
	finger->pressure = event->pressure;

	if (event->type == ev_touchdown)
		finger->down = true;
	else if (event->type == ev_touchup)
		finger->down = false;
}

void TS_GetSettings(void)
{
	if (!TS_Ready())
		return;

	touch_movementstyle = cv_touchstyle.value;
	touch_camera = (cv_usemouse.value ? false : (!!cv_touchcamera.value));
	touch_preset = cv_touchpreset.value;
	touch_gui_scale = cv_touchguiscale.value;
}

#define SCALECOORD(coord) FixedMul(coord, scale)

void TS_UpdateControls(void)
{
	TS_GetSettings();
	TS_DefineButtons();
}

fixed_t TS_GetDefaultScale(void)
{
	return (fixed_t)(atof(cv_touchguiscale.defaultvalue) * FRACUNIT);
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

static void ScaleDPadBase(touchmovementstyle_e tms, boolean ringslinger, boolean tiny, fixed_t dx, fixed_t dy, fixed_t dw, fixed_t dh, fixed_t scale, fixed_t offs, fixed_t bottomalign)
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

void TS_NormalizeButton(touchconfig_t *button)
{
	button->x = FixedDiv(button->x, BASEVIDWIDTH * FRACUNIT);
	button->y = FixedDiv(button->y, BASEVIDHEIGHT * FRACUNIT);
}

void TS_NormalizeConfig(touchconfig_t *config, int configsize)
{
	INT32 i;
	for (i = 0; i < configsize; i++)
		TS_NormalizeButton(&config[i]);
}

void TS_DenormalizeCoords(fixed_t *x, fixed_t *y)
{
	*x *= BASEVIDWIDTH;
	*y *= BASEVIDHEIGHT;
}

void TS_CenterCoords(fixed_t *x, fixed_t *y)
{
	if (!TS_IsPresetActive())
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
	if (!TS_IsPresetActive())
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

const char *TS_GetButtonName(INT32 gc)
{
	return touchbuttonnames[gc].name;
}

const char *TS_GetButtonShortName(INT32 gc)
{
	return touchbuttonnames[gc].tinyname;
}

void TS_SetButtonNames(touchconfig_t *controls)
{
	INT32 i;

	for (i = 0; i < num_gamecontrols; i++)
	{
		controls[i].name = TS_GetButtonName(i);
		controls[i].tinyname = TS_GetButtonShortName(i);
	}
}

boolean TS_IsDPadButton(INT32 gc)
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

void TS_MarkDPadButtons(touchconfig_t *controls)
{
	INT32 i;

	for (i = 0; i < num_gamecontrols; i++)
	{
		if (TS_IsDPadButton(i))
			controls[i].dpad = true;
	}
}

void TS_BuildPreset(touchconfig_t *controls, touchconfigstatus_t *status, touchmovementstyle_e tms, fixed_t scale, boolean tiny)
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
	ScaleDPadBase(tms, status->ringslinger, tiny, dx, dy, dw, dh, scale, offs, bottomalign);
	TS_DPadPreset(
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
	TS_NormalizeConfig(controls, num_gamecontrols);

	// Mark movement controls as d-pad buttons
	TS_MarkDPadButtons(controls);

	// Set button names
	TS_SetButtonNames(controls);

	// Mark undefined buttons as hidden
	for (x = 0; x < num_gamecontrols; x++)
	{
		if (!controls[x].w)
			controls[x].hidden = true;
	}
}

static void BuildWeaponButtons(touchconfigstatus_t *status)
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

		TS_NormalizeButton(&touchcontrols[wep]);

		wep++;
	}
}

static void HidePlayerControlButtons(touchconfigstatus_t *status)
{
	if (status->promptblockcontrols)
	{
		INT32 i;
		for (i = 0; i < num_gamecontrols; i++)
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
		memset(touchcontrols, 0x00, sizeof(touchconfig_t) * num_gamecontrols);
		TS_BuildPreset(touchcontrols, status, touch_movementstyle, touch_gui_scale, (touch_preset == touchpreset_tiny));
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
	touch_joystick_x = touchcontrols[gc_joystick].x;
	touch_joystick_y = touchcontrols[gc_joystick].y;

	TS_DenormalizeCoords(&touch_joystick_x, &touch_joystick_y);

	touch_joystick_w = touchcontrols[gc_joystick].w;
	touch_joystick_h = touchcontrols[gc_joystick].h;

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
		TS_DefaultControlLayout();

		// Copy custom controls
		M_Memcpy(&touchcontrols, usertouchcontrols, sizeof(touchconfig_t) * num_gamecontrols);

		// Position joystick, weapon buttons, etc.
		TS_PositionExtraUserButtons();

		// Mark movement controls as d-pad buttons
		TS_MarkDPadButtons(touchcontrols);
	}
	else
		TS_DefineButtons();
}

#undef SCALECOORD

void TS_PositionNavigation(void)
{
	INT32 i;
	touchconfig_t *nav = touchnavigation;
	INT32 corneroffset = 4 * FRACUNIT;

	touchconfig_t *back = &nav[KEY_ESCAPE];
	touchconfig_t *confirm = &nav[KEY_ENTER];
	touchconfig_t *con = &nav[KEY_CONSOLE];

	// clear all
	memset(touchnavigation, 0x00, sizeof(touchconfig_t) * NUMKEYS);

	for (i = 0; i < NUMKEYS; i++)
		nav[i].color = 16;

	// Back
	if (touchnavigationstatus.customizingcontrols)
	{
		back->w = 16 * FRACUNIT;
		back->h = 16 * FRACUNIT;
		back->color = 35;
	}
	else
	{
		back->x = corneroffset;
		back->y = corneroffset;
		back->w = 24 * FRACUNIT;
		back->h = 24 * FRACUNIT;
		back->h = 24 * FRACUNIT;
	}

	back->name = "\x1C";

	// Confirm
	if (touchnavigationstatus.layoutsubmenuopen)
		confirm->hidden = true;
	else if (touchnavigationstatus.customizingcontrols)
	{
		confirm->w = 32 * FRACUNIT;
		confirm->h = 16 * FRACUNIT;
		confirm->x = ((BASEVIDWIDTH / 2) * FRACUNIT) - (TOUCHGRIDSIZE * FRACUNIT);
		confirm->color = 112;
		confirm->name = "+";
	}
	else
	{
		confirm->w = 24 * FRACUNIT;
		confirm->h = 24 * FRACUNIT;
		confirm->x = (((vid.width / vid.dupx) * FRACUNIT) - confirm->w - corneroffset);
		confirm->y = corneroffset;
		confirm->name = "\x1D";
	}

	// Console
	if (!touchnavigationstatus.canopenconsole)
		con->hidden = true;
	else
	{
		con->x = corneroffset;
		con->y = back->y + back->h + (8 * FRACUNIT);
		con->w = 24 * FRACUNIT;
		con->h = 24 * FRACUNIT;
		con->name = "$";
	}

	// Normalize all buttons
	TS_NormalizeConfig(nav, NUMKEYS);
}

void TS_DefineButtons(void)
{
	static touchconfigstatus_t status, navstatus;
	size_t size = sizeof(touchconfigstatus_t);

	if (!TS_Ready())
		return;

	//
	// Touch controls
	//

	if (TS_IsPresetActive())
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

			TS_PositionButtons();

			if (!TS_IsPresetActive())
				TS_PresetChanged();
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
			TS_PresetChanged();
		}
	}

	//
	// Touch navigation
	//

	navstatus.vidwidth = vid.width;
	navstatus.vidheight = vid.height;
	navstatus.customizingcontrols = TS_IsCustomizingControls();
	navstatus.layoutsubmenuopen = TS_IsCustomizationSubmenuOpen();
	navstatus.canopenconsole = (!(modeattacking || metalrecording) && !navstatus.customizingcontrols);

	if (memcmp(&navstatus, &touchnavigationstatus, size))
	{
		M_Memcpy(&touchnavigationstatus, &navstatus, size);
		TS_PositionNavigation();
	}
}

#endif // TOUCHINPUTS
