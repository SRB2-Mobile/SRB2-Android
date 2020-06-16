// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ts_custom.c
/// \brief Touch controls customization

#include "doomdata.h"
#include "doomtype.h"
#include "doomdef.h"

#include "ts_custom.h"

#include "d_event.h"
#include "g_input.h"

#include "m_fixed.h"
#include "m_misc.h"
#include "m_menu.h" // M_IsCustomizingTouchControls

#include "d_event.h"
#include "g_input.h"

#include "f_finale.h" // curfadevalue
#include "v_video.h"
#include "st_stuff.h" // ST_drawTouchGameInput

#include "console.h" // CON_Ready()

#ifdef TOUCHINPUTS
static touchcust_buttonstatus_t touchcustbuttons[num_gamecontrols];

static touchcust_submenu_e touchcust_submenu = touchcust_submenu_none;
static touchcust_submenu_button_t touchcust_submenu_buttons[TOUCHCUST_SUBMENU_MAXBUTTONS];
static INT32 touchcust_submenu_numbuttons = 0;

static INT32 touchcust_submenu_selection = 0;
static INT32 touchcust_submenu_listsize = 0;
static INT32 touchcust_submenu_list[TOUCHCUST_SUBMENU_MAXLISTSIZE];
static const char *touchcust_submenu_listnames[TOUCHCUST_SUBMENU_MAXLISTSIZE];

static touchcust_submenu_scrollbar_t touchcust_submenu_scrollbar[NUMTOUCHFINGERS];
static INT32 touchcust_submenu_scroll = 0;

static INT32 touchcust_addbutton_x = 0;
static INT32 touchcust_addbutton_y = 0;

// ==========
// Prototypes
// ==========

static boolean FingerTouchesRect(INT32 fx, INT32 fy, INT32 rx, INT32 ry, INT32 rw, INT32 rh);
static boolean FingerTouchesButton(INT32 x, INT32 y, touchconfig_t *btn, boolean inside);

static void GetButtonRect(touchconfig_t *btn, INT32 *x, INT32 *y, INT32 *w, INT32 *h, boolean inside);
static void GetButtonResizePoint(touchconfig_t *btn, vector2_t *point, INT32 *x, INT32 *y, INT32 *w, INT32 *h);
static boolean GetButtonOption(touchconfig_t *btn, touchcust_buttonstatus_t *btnstatus, touchcust_option_e opt, INT32 *x, INT32 *y, INT32 *w, INT32 *h, UINT8 *col, char *str);

static void MoveButtonTo(touchconfig_t *btn, INT32 x, INT32 y);
static void OffsetButtonBy(touchconfig_t *btn, fixed_t offsx, fixed_t offsy);
static void SnapButtonToGrid(touchconfig_t *btn);
static fixed_t RoundSnapCoord(fixed_t a, fixed_t b);

static void UpdateJoystickBase(touchconfig_t *btn);
static void UpdateJoystickSize(touchconfig_t *btn);
static void NormalizeDPad(void);

static INT32 AddButton(INT32 x, INT32 y, touchfinger_t *finger, event_t *event);
static void RemoveButton(touchconfig_t *btn);

static void ClearSelection(touchcust_buttonstatus_t *selection);
static void ClearAllSelections(void);

static void OpenSubmenu(touchcust_submenu_e submenu);
static void CloseSubmenu(void);

static boolean HandleSubmenuButtons(INT32 fx, INT32 fy, touchfinger_t *finger, event_t *event);

static void GetSubmenuListItems(size_t *t, size_t *i, size_t *b, size_t *height, boolean scrolled);
static void GetSubmenuScrollbar(INT32 basex, INT32 basey, INT32 *x, INT32 *y, INT32 *w, INT32 *h, boolean scrolled);

static boolean IsMovingSubmenuScrollbar(void);

static boolean HandleResizePointSelection(INT32 x, INT32 y, touchfinger_t *finger, touchconfig_t *btn, touchcust_buttonstatus_t *btnstatus);

static boolean HandleButtonOptions(INT32 x, INT32 y, touchfinger_t *finger, touchconfig_t *btn, touchcust_buttonstatus_t *btnstatus, boolean execute);
static boolean IsFingerTouchingButtonOptions(INT32 x, INT32 y, touchfinger_t *finger, touchconfig_t *btn, touchcust_buttonstatus_t *btnstatus);

// =====================================================

boolean TS_IsCustomizingControls(void)
{
	return M_IsCustomizingTouchControls();
}

void TS_SetupCustomization(void)
{
	INT32 i;

	touch_screenexists = true;
	ClearAllSelections();

	for (i = 0; i < num_gamecontrols; i++)
	{
		touchcust_buttonstatus_t *btnstatus = &touchcustbuttons[i];
		btnstatus->snaptogrid = true;
	}
}

boolean TS_ExitCustomization(void)
{
	if (touchcust_submenu != touchcust_submenu_none)
	{
		CloseSubmenu();
		return false;
	}

	ClearAllSelections();

	// Copy custom controls
	M_Memcpy(&touchcontrols, usertouchcontrols, sizeof(touchconfig_t) * num_gamecontrols);

	return true;
}

//
// Check if the finger (fx, fy) touches a rectangle
//
static boolean FingerTouchesRect(INT32 fx, INT32 fy, INT32 rx, INT32 ry, INT32 rw, INT32 rh)
{
	return (fx >= rx && fx <= rx + rw && fy >= ry && fy <= ry + rh);
}

//
// Get the rectangular points of a button
//
static void GetButtonRect(touchconfig_t *btn, INT32 *x, INT32 *y, INT32 *w, INT32 *h, boolean inside)
{
	INT32 tx = btn->x, ty = btn->y, tw = btn->w, th = btn->h;

	G_ScaleTouchCoords(&tx, &ty, &tw, &th, true, (!btn->dontscale));

	*x = tx;
	*y = ty;
	*w = tw;
	*h = th;

	if (!inside)
	{
		*x -= BUTTONEXTENDH;
		*y -= BUTTONEXTENDV;
		*w += (BUTTONEXTENDH * 2);
		*h += (BUTTONEXTENDV * 2);
	}
}

// ===============
// Button resizing
// ===============

static vector2_t touchcust_resizepoints[touchcust_numresizepoints] = {
	// top
	{0, 0}, {FRACUNIT/2, 0}, {FRACUNIT, 0},

	// bottom
	{0, FRACUNIT}, {FRACUNIT/2, FRACUNIT},  {FRACUNIT, FRACUNIT},

	// left
	{0, FRACUNIT/2},

	// right
	{FRACUNIT, FRACUNIT/2}
};

//
// Get a resize point of a button
//
static void GetButtonResizePoint(touchconfig_t *btn, vector2_t *point, INT32 *x, INT32 *y, INT32 *w, INT32 *h)
{
	INT32 tx, ty, tw, th;

	INT32 psizew = 5 * vid.dupx;
	INT32 psizeh = 5 * vid.dupy;

	GetButtonRect(btn, &tx, &ty, &tw, &th, false);

#define pinterp(v0, t, v1) (FixedMul((FRACUNIT - t), v0) + FixedMul(t, v1))
	*x = pinterp(tx, point->x, tx + tw) - (psizew / 2);
	*y = pinterp(ty, point->y, ty + th) - (psizeh / 2);
#undef pinterp

	*w = psizew;
	*h = psizeh;
}

// ==============
// Button options
// ==============

//
// Get information of an option of a button
//
static boolean GetButtonOption(touchconfig_t *btn, touchcust_buttonstatus_t *btnstatus, touchcust_option_e opt, INT32 *x, INT32 *y, INT32 *w, INT32 *h, UINT8 *col, char *str)
{
	INT32 tx, ty, tw, th;

	INT32 offs = (4 * vid.dupx);
	INT32 yoff = (8 * vid.dupx);
	INT32 remw = (16 * vid.dupx);
	INT32 snpw = (16 * vid.dupx);
	INT32 btnh = (12 * vid.dupy);

	INT32 left, top;

	GetButtonRect(btn, &tx, &ty, &tw, &th, true);

	left = tx - offs;
	top = ty - btnh;

	if (snpw > (tw - remw))
		left = (tx + tw) - (remw + offs + snpw);

	if (top <= btnh)
		top = (ty + th + btnh);
	else
		top -= yoff;

	if (col)
		*col = 0;
	if (str)
		strcpy(str, "?");

	switch (opt)
	{
		case touchcust_option_snaptogrid:
			*w = snpw;
			*h = btnh;
			*x = left;
			*y = top;
			if (col)
				*col = (btnstatus->snaptogrid ? 112 : 15);
			if (str)
				strcpy(str, "\x18");
			break;
		case touchcust_option_remove:
			*w = remw;
			*h = btnh;
			*x = (tx + tw) - (remw - offs);
			*y = top;
			if (col)
				*col = 35;
			if (str)
				strcpy(str, "X");
			break;
		default:
			*x = 0;
			*y = 0;
			*w = 0;
			*h = 0;
			break;
	}

	return true;
}

// =======
// Buttons
// =======

//
// Check if the finger (x, y) touches a button
//
static boolean FingerTouchesButton(INT32 x, INT32 y, touchconfig_t *btn, boolean inside)
{
	INT32 tx, ty, tw, th;

	GetButtonRect(btn, &tx, &ty, &tw, &th, inside);

	return FingerTouchesRect(x, y, tx, ty, tw, th);
}

//
// Moves a button to (x, y)
//
static void MoveButtonTo(touchconfig_t *btn, INT32 x, INT32 y)
{
	btn->x = (x / vid.dupx) * FRACUNIT;
	btn->y = (y / vid.dupy) * FRACUNIT;

	if (btn == &usertouchcontrols[gc_joystick])
		UpdateJoystickBase(btn);

	G_NormalizeTouchButton(btn);
}

//
// Offsets a button by (offsx, offsy)
//
static void OffsetButtonBy(touchconfig_t *btn, fixed_t offsx, fixed_t offsy)
{
	btn->x *= BASEVIDWIDTH;
	btn->y *= BASEVIDHEIGHT;
	btn->x += offsx;
	btn->y += offsy;

	if (btn == &usertouchcontrols[gc_joystick])
		UpdateJoystickBase(btn);

	G_NormalizeTouchButton(btn);
}

//
// Snaps a coordinate to a grid
//
static fixed_t RoundSnapCoord(fixed_t a, fixed_t b)
{
	fixed_t div = FixedDiv(a, b);
	fixed_t frac = (div & 0xFFFF);
	fixed_t rounded;

	if (frac <= (FRACUNIT/2))
		rounded = FixedFloor(div);
	else
		rounded = FixedCeil(div);

	return FixedMul(rounded, b);
}

//
// Snaps a button to a grid
//
static void SnapButtonToGrid(touchconfig_t *btn)
{
	INT32 sgridx = TOUCHSMALLGRIDSIZE * FRACUNIT;
	INT32 sgridy = TOUCHSMALLGRIDSIZE * FRACUNIT;
#ifdef MTOUCH_SNAPTOSMALLGRID
	INT32 gridx = sgridx;
	INT32 gridy = sgridy;
#else
	INT32 gridx = TOUCHGRIDSIZE * FRACUNIT;
	INT32 gridy = TOUCHGRIDSIZE * FRACUNIT;
#endif

	btn->x *= BASEVIDWIDTH;
	btn->y *= BASEVIDHEIGHT;

	btn->x = RoundSnapCoord(btn->x, gridx);
	btn->y = RoundSnapCoord(btn->y, gridy);

	btn->w = RoundSnapCoord(btn->w, sgridx);
	btn->h = RoundSnapCoord(btn->h, sgridy);

	if (btn == &usertouchcontrols[gc_joystick])
		UpdateJoystickBase(btn);

	G_NormalizeTouchButton(btn);
}

//
// Updates the joystick
//
static void UpdateJoystickBase(touchconfig_t *btn)
{
	fixed_t scale, xscale, yscale;
	INT32 jw, jh;

	TS_GetJoystick(NULL, NULL, &jw, &jh, false);

	// Must not be normalized!
	touch_joystick_x = btn->x;
	touch_joystick_y = btn->y;

	// Update joystick size
	UpdateJoystickSize(btn);

	// Move d-pad
	scale = TS_GetDefaultScale();
	xscale = FixedMul(FixedDiv(btn->w, jw), scale);
	yscale = FixedMul(FixedDiv(btn->h, jh), scale);
	G_DPadPreset(usertouchcontrols, xscale, yscale, btn->w, false);

	// Normalize d-pad
	NormalizeDPad();
}

static void UpdateJoystickSize(touchconfig_t *btn)
{
	touch_joystick_w = btn->w;
	touch_joystick_h = btn->h;
}

static void NormalizeDPad(void)
{
	G_NormalizeTouchButton(&usertouchcontrols[gc_forward]);
	G_NormalizeTouchButton(&usertouchcontrols[gc_backward]);
	G_NormalizeTouchButton(&usertouchcontrols[gc_strafeleft]);
	G_NormalizeTouchButton(&usertouchcontrols[gc_straferight]);
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

//
// Adds a button
//
static INT32 AddButton(INT32 x, INT32 y, touchfinger_t *finger, event_t *event)
{
	INT32 gc = touchcust_submenu_list[touchcust_submenu_selection];
	touchconfig_t *btn = &usertouchcontrols[gc];

	(void)x;
	(void)y;
	(void)finger;
	(void)event;

	memset(btn, 0x00, sizeof(touchconfig_t));

	MoveButtonTo(btn, touchcust_addbutton_x, touchcust_addbutton_y);

	if (gc == gc_joystick)
	{
		fixed_t w, h;
		TS_GetJoystick(NULL, NULL, &w, &h, false);
		btn->w = w;
		btn->h = h;
	}
	else
	{
		btn->w = 32 * FRACUNIT;
		btn->h = 16 * FRACUNIT;
	}

	if (btn == &usertouchcontrols[gc_joystick])
	{
		btn->x *= BASEVIDWIDTH;
		btn->y *= BASEVIDHEIGHT;
		UpdateJoystickBase(btn);
		G_NormalizeTouchButton(btn);
	}

	btn->name = G_GetTouchButtonName(gc);
	btn->tinyname = G_GetTouchButtonShortName(gc);

	btn->hidden = false;

	return gc;
}

//
// Removes a button
//
static void RemoveButton(touchconfig_t *btn)
{
	memset(btn, 0x00, sizeof(touchconfig_t));
	btn->hidden = true;
}

//
// Clears a selection
//
static void ClearSelection(touchcust_buttonstatus_t *selection)
{
	selection->selected = false;
	selection->moving = false;
	selection->isselecting = false;
	selection->resizearea = false;
	selection->isresizing = touchcust_resizepoint_none;
	selection->optsel = touchcust_options_none;

	if (selection->finger)
		selection->finger->u.gamecontrol = gc_null;
	selection->finger = NULL;
}

//
// Clears all selections
//
static void ClearAllSelections(void)
{
	INT32 i;
	for (i = 0; i < num_gamecontrols; i++)
	{
		if (touchcustbuttons[i].selected)
			ClearSelection(&touchcustbuttons[i]);
	}
}

// ========
// Submenus
// ========

static void OpenSubmenu(touchcust_submenu_e submenu)
{
	touchcust_submenu = submenu;
	ClearAllSelections();
}

static void CloseSubmenu(void)
{
	touchcust_submenu = touchcust_submenu_none;
}

static boolean HandleSubmenuButtons(INT32 fx, INT32 fy, touchfinger_t *finger, event_t *event)
{
	INT32 dup = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
	INT32 bx, by, bw, bh;
	INT32 i;

	if (!touchcust_submenu_numbuttons)
		return false;

	switch (event->type)
	{
		case ev_touchdown:
			if (CON_Ready())
				break;

			for (i = 0; i < touchcust_submenu_numbuttons; i++)
			{
				touchcust_submenu_button_t *btn = &touchcust_submenu_buttons[i];

				bx = btn->x * vid.dupx;
				by = btn->y * vid.dupy;
				bw = btn->w * vid.dupx;
				bh = btn->h * vid.dupy;

				bx += (vid.width - (BASEVIDWIDTH * dup)) / 2;
				by += (vid.height - (BASEVIDHEIGHT * dup)) / 2;

				if (FingerTouchesRect(fx, fy, bx, by, bw, bh))
				{
					btn->isdown = finger;
					finger->u.selection = (i+1);
					return true;
				}
			}

			break;

		case ev_touchup:
			if (finger->u.selection)
			{
				touchcust_submenu_button_t *btn = &touchcust_submenu_buttons[finger->u.selection-1];

				bx = btn->x * vid.dupx;
				by = btn->y * vid.dupy;
				bw = btn->w * vid.dupx;
				bh = btn->h * vid.dupy;

				bx += (vid.width - (BASEVIDWIDTH * dup)) / 2;
				by += (vid.height - (BASEVIDHEIGHT * dup)) / 2;

				btn->isdown = NULL;

				if (FingerTouchesRect(fx, fy, bx, by, bw, bh))
					btn->action(fx, fy, finger, event);

				finger->u.selection = 0;
				return true;
			}
			break;

		default:
			break;
	}

	return false;
}

static void DrawSubmenuButtons(void)
{
	INT32 bx, by, bw, bh;
	INT32 i;

	if (!touchcust_submenu_numbuttons)
		return;

	for (i = 0; i < touchcust_submenu_numbuttons; i++)
	{
		touchcust_submenu_button_t *btn = &touchcust_submenu_buttons[i];
		UINT8 color = btn->color;
		INT32 sx, sy;
		INT32 strwidth, strheight;
		const char *str = btn->name;

		bx = btn->x;
		by = btn->y;
		bw = btn->w;
		bh = btn->h;

		V_DrawFill(bx, (btn->isdown ? by+1 : by), bw, (btn->isdown ? bh-1 : bh), (btn->isdown ? (color+3) : color));
		if (!btn->isdown)
			V_DrawFill(bx, by+bh, bw, 1, 31);

		strwidth = V_StringWidth(str, V_ALLOWLOWERCASE);
		strheight = 8;
		sx = (bx + (bw / 2)) - (strwidth / 2);
		sy = (by + (bh / 2)) - ((strheight) / 2);
		V_DrawString(sx, sy, V_ALLOWLOWERCASE, str);
	}
}

static void Submenu_Generic_ExitAction(INT32 x, INT32 y, touchfinger_t *finger, event_t *event)
{
	(void)x;
	(void)y;
	(void)finger;
	(void)event;
	CloseSubmenu();
}

static void GetSubmenuListItems(size_t *t, size_t *i, size_t *b, size_t *height, boolean scrolled)
{
	INT32 sel = 0;

	if (scrolled)
	{
		sel = touchcust_submenu_scroll;
		sel /= (TOUCHCUST_SUBMENU_DISPLAYITEMS * vid.dupy);
		sel = max(0, min(sel, touchcust_submenu_listsize));
	}

	// The list is too small
	if (touchcust_submenu_listsize <= TOUCHCUST_SUBMENU_DISPLAYITEMS)
	{
		*t = 0; // first item
		*b = touchcust_submenu_listsize - 1; // last item
		*i = 0; // "scrollbar" at "top" position
	}
	else
	{
		size_t q = *height;
		*height = (TOUCHCUST_SUBMENU_DISPLAYITEMS * *height)/touchcust_submenu_listsize; // height of scroll bar
		if (sel <= TOUCHCUST_SUBMENU_SCROLLITEMS) // all the way up
		{
			*t = 0; // first item
			*b = TOUCHCUST_SUBMENU_DISPLAYITEMS - 1; //9th item
			*i = 0; // scrollbar at top position
		}
		else if (sel >= touchcust_submenu_listsize - (TOUCHCUST_SUBMENU_SCROLLITEMS + 1)) // all the way down
		{
			*t = touchcust_submenu_listsize - TOUCHCUST_SUBMENU_DISPLAYITEMS; // # 9th last
			*b = touchcust_submenu_listsize - 1; // last item
			*i = q-(*height); // scrollbar at bottom position
		}
		else // somewhere in the middle
		{
			*t = sel - TOUCHCUST_SUBMENU_SCROLLITEMS; // 4 items above
			*b = sel + TOUCHCUST_SUBMENU_SCROLLITEMS; // 4 items below
			*i = (*t * (q-(*height)))/(touchcust_submenu_listsize - TOUCHCUST_SUBMENU_DISPLAYITEMS); // calculate position of scrollbar
		}
	}
}

static void GetSubmenuScrollbar(INT32 basex, INT32 basey, INT32 *x, INT32 *y, INT32 *w, INT32 *h, boolean scrolled)
{
	size_t i, m;
	size_t t, b;

	m = TOUCHCUST_SUBMENU_HEIGHT;
	GetSubmenuListItems(&t, &i, &b, &m, scrolled);

	// draw the scroll bar
	*x = basex + TOUCHCUST_SUBMENU_WIDTH-1 - TOUCHCUST_SUBMENU_SBARWIDTH;
	*y = (basey - 1) + i;
	*w = TOUCHCUST_SUBMENU_SBARWIDTH;
	*h = m;
}

static void DrawSubmenuList(INT32 x, INT32 y)
{
	size_t i, m;
	size_t t, b; // top and bottom item #s to draw in directory

	V_DrawFill(x, (y - 16) + (16 - 3), TOUCHCUST_SUBMENU_WIDTH, 1, 0);
	V_DrawFill(x, (y - 16) + (16 - 2), TOUCHCUST_SUBMENU_WIDTH, 1, 30);

	m = TOUCHCUST_SUBMENU_HEIGHT;
	V_DrawFill(x, y - 1, TOUCHCUST_SUBMENU_WIDTH, m, 159);

	GetSubmenuListItems(&t, &i, &b, &m, true);

	// draw the scroll bar
	{
		INT32 sx, sy, sw, sh;
		GetSubmenuScrollbar(x, y, &sx, &sy, &sw, &sh, true);
		V_DrawFill(sx, sy, sw, sh, 0);
	}
	V_DrawFill(x + TOUCHCUST_SUBMENU_WIDTH-1 - TOUCHCUST_SUBMENU_SBARWIDTH, (y - 1) + i, TOUCHCUST_SUBMENU_SBARWIDTH, m, 0);

	// draw list items
	for (i = t; i <= b; i++)
	{
		INT32 left = (x + 11);
		INT32 top = (y + 8);
		UINT32 flags = V_ALLOWLOWERCASE;

		if (y > BASEVIDHEIGHT)
			break;

		if ((INT32)i == touchcust_submenu_selection)
			V_DrawFill(left - 4, top - 4, TOUCHCUST_SUBMENU_WIDTH - 12 - TOUCHCUST_SUBMENU_SBARWIDTH, 16, 149);

		if (touchcust_submenu_list[i])
			V_DrawString(left, top, flags, touchcust_submenu_listnames[i]);

		y += 20;
	}
}

static boolean IsMovingSubmenuScrollbar(void)
{
	INT32 i;

	for (i = 0; i < NUMTOUCHFINGERS; i++)
	{
		if (touchcust_submenu_scrollbar[i].dragged)
			return true;
	}

	return false;
}

//
// New button submenu
//

static void Submenu_AddNewButton_OnFingerEvent(INT32 fx, INT32 fy, touchfinger_t *finger, event_t *event)
{
	INT32 fingernum = (finger - touchfingers);
	touchcust_submenu_scrollbar_t *scrollbar = &touchcust_submenu_scrollbar[fingernum];
	boolean foundbutton = false;

	INT32 mx = TOUCHCUST_SUBMENU_BASEX;
	INT32 my = TOUCHCUST_SUBMENU_BASEY;
	INT32 sx, sy, sw, sh;
	size_t i, m = TOUCHCUST_SUBMENU_HEIGHT;
	size_t t, b;

	INT32 dup = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);

	GetSubmenuListItems(&t, &i, &b, &m, true);
	GetSubmenuScrollbar(mx, my, &sx, &sy, &sw, &sh, (IsMovingSubmenuScrollbar() ? false : true));

	sx *= vid.dupx;
	sy *= vid.dupy;
	sw *= vid.dupx;
	sh *= vid.dupy;

	sx += (vid.width - (BASEVIDWIDTH * dup)) / 2;
	sy += (vid.height - (BASEVIDHEIGHT * dup)) / 2;

	if (HandleSubmenuButtons(fx, fy, finger, event))
		return;

	switch (event->type)
	{
		case ev_touchdown:
			if (CON_Ready())
				break;

			for (i = t; i <= b; i++)
			{
				INT32 left = (mx + 7);
				INT32 top = (my + 4);
				INT32 mw = (TOUCHCUST_SUBMENU_WIDTH - 12 - TOUCHCUST_SUBMENU_SBARWIDTH);
				INT32 mh = 16;

				if (my > BASEVIDHEIGHT)
					break;

				left *= vid.dupx;
				top *= vid.dupy;
				mw *= vid.dupx;
				mh *= vid.dupy;

				left += (vid.width - (BASEVIDWIDTH * dup)) / 2;
				top += (vid.height - (BASEVIDHEIGHT * dup)) / 2;

				if (FingerTouchesRect(fx, fy, left, top, mw, mh))
				{
					touchcust_submenu_selection = i;
					foundbutton = true;
					break;
				}

				my += 20;
			}

			finger->y = fy;

			if (!foundbutton && FingerTouchesRect(fx, fy, sx, sy, sw, sh))
			{
				scrollbar->dragged = true;
				scrollbar->y = fy;
			}

			break;

		case ev_touchmotion:
			if (CON_Ready())
				break;

			if (scrollbar->dragged)
			{
				touchcust_submenu_scroll += (fy - scrollbar->y);
				scrollbar->y = fy;
				break;
			}

			finger->y = fy;

			break;

		case ev_touchup:
			scrollbar->dragged = false;
			break;

		default:
			break;
	}
}

static void Submenu_AddNewButton_NewButtonAction(INT32 x, INT32 y, touchfinger_t *finger, event_t *event)
{
	INT32 gc = AddButton(x, y, finger, event);
	touchcust_buttonstatus_t *btnstatus = &touchcustbuttons[gc];

	CloseSubmenu();

	btnstatus->isselecting = true;
	btnstatus->selected = true;
	btnstatus->moving = false;
	btnstatus->finger = finger;
	btnstatus->isresizing = touchcust_resizepoint_none;
}

static void Submenu_AddNewButton_Drawer(void)
{
	if (curfadevalue)
		V_DrawFadeScreen(0xFF00, curfadevalue);

	DrawSubmenuList(TOUCHCUST_SUBMENU_BASEX, TOUCHCUST_SUBMENU_BASEY);
	DrawSubmenuButtons();
}

static void (*touchcust_submenufuncs[num_touchcust_submenus]) (INT32 x, INT32 y, touchfinger_t *finger, event_t *event) =
{
	NULL,
	Submenu_AddNewButton_OnFingerEvent,
};

static void (*touchcust_submenudrawfuncs[num_touchcust_submenus]) (void) =
{
	NULL,
	Submenu_AddNewButton_Drawer,
};

//
// Handles button resizing
//
static boolean HandleResizePointSelection(INT32 x, INT32 y, touchfinger_t *finger, touchconfig_t *btn, touchcust_buttonstatus_t *btnstatus)
{
	INT32 i;
	boolean resizing = false;
	INT32 px, py, pw, ph;

	if (btnstatus->isresizing == touchcust_resizepoint_none)
	{
		for (i = 0; i < touchcust_numresizepoints; i++)
		{
			vector2_t *point = &touchcust_resizepoints[i];
			GetButtonResizePoint(btn, point, &px, &py, &pw, &ph);
			if (FingerTouchesRect(x, y, px, py, pw, ph))
			{
				finger->x = x;
				finger->y = y;
				btnstatus->isresizing = i;
				resizing = true;
				break;
			}
		}
	}
	else
	{
		fixed_t dx = FixedDiv((x - finger->x) * FRACUNIT, vid.dupx * FRACUNIT);
		fixed_t dy = FixedDiv((y - finger->y) * FRACUNIT, vid.dupy * FRACUNIT);
		INT32 corner = btnstatus->isresizing;

		switch (corner)
		{
			case touchcust_resizepoint_topleft:
				OffsetButtonBy(btn, (btn->w > MINBTNWIDTH ? dx : 0), (btn->h > MINBTNHEIGHT ? dy : 0));
				btn->w -= dx;
				btn->h -= dy;
				break;
			case touchcust_resizepoint_topright:
				OffsetButtonBy(btn, 0, (btn->h > MINBTNHEIGHT ? dy : 0));
				btn->w += dx;
				btn->h -= dy;
				break;

			case touchcust_resizepoint_bottomleft:
				OffsetButtonBy(btn, (btn->w > MINBTNWIDTH ? dx : 0), 0);
				btn->w -= dx;
				btn->h += dy;
				break;
			case touchcust_resizepoint_bottomright:
				btn->w += dx;
				btn->h += dy;
				break;

			case touchcust_resizepoint_topmiddle:
				OffsetButtonBy(btn, 0, (btn->h > MINBTNHEIGHT ? dy : 0));
				btn->h -= dy;
				break;
			case touchcust_resizepoint_bottommiddle:
				btn->h += dy;
				break;

			case touchcust_resizepoint_leftside:
				OffsetButtonBy(btn, (btn->w > MINBTNWIDTH ? dx : 0), 0);
				btn->w -= dx;
				break;
			case touchcust_resizepoint_rightside:
				btn->w += dx;
				break;

			default:
				break;
		}

		if (btn->w < MINBTNWIDTH)
			btn->w = MINBTNWIDTH;
		if (btn->h < MINBTNHEIGHT)
			btn->h = MINBTNHEIGHT;

		if (btn == &usertouchcontrols[gc_joystick])
		{
			// Scale button
			btn->x *= BASEVIDWIDTH;
			btn->y *= BASEVIDHEIGHT;

			// Update joystick
			UpdateJoystickBase(btn);

			// Normalize again
			G_NormalizeTouchButton(btn);
		}

		finger->x = x;
		finger->y = y;
		return true;
	}

	return resizing;
}

//
// Handles button options
//
static boolean IsFingerTouchingButtonOptions(INT32 x, INT32 y, touchfinger_t *finger, touchconfig_t *btn, touchcust_buttonstatus_t *btnstatus)
{
	touchcust_option_e opt;

	(void)finger;

	for (opt = touchcust_options_first; opt < num_touchcust_options; opt++)
	{
		INT32 px, py, pw, ph;
		GetButtonOption(btn, btnstatus, opt, &px, &py, &pw, &ph, NULL, NULL);
		if (FingerTouchesRect(x, y, px, py, pw, ph))
			return true;
	}

	return false;
}

static boolean HandleButtonOptions(INT32 x, INT32 y, touchfinger_t *finger, touchconfig_t *btn, touchcust_buttonstatus_t *btnstatus, boolean execute)
{
	INT32 px, py, pw, ph;
	touchcust_option_e opt = btnstatus->optsel;

	(void)finger;

	if (!execute)
	{
		for (opt = touchcust_options_first; opt < num_touchcust_options; opt++)
		{
			if (!GetButtonOption(btn, btnstatus, opt, &px, &py, &pw, &ph, NULL, NULL))
				continue;

			if (FingerTouchesRect(x, y, px, py, pw, ph))
			{
				btnstatus->optsel = opt;
				return true;
			}
		}
	}
	else if (opt != touchcust_options_none)
	{
		if (!GetButtonOption(btn, btnstatus, opt, &px, &py, &pw, &ph, NULL, NULL))
			return false;

		if (FingerTouchesRect(x, y, px, py, pw, ph))
		{
			if (!execute)
				return true;

			switch (opt)
			{
				case touchcust_option_snaptogrid:
					btnstatus->snaptogrid = !btnstatus->snaptogrid;
					SnapButtonToGrid(btn);
					break;
				case touchcust_option_remove:
					RemoveButton(btn);
					ClearSelection(btnstatus);
					break;
				default:
					break;
			}

			return true;
		}
	}

	return false;
}

//
// Setup the "add new button" submenu
//

struct {
	const char *name;
	gamecontrols_e gc;
} const touchcust_buttonlist[] = {
	{"Joystick / D-Pad",     gc_joystick},
	{"Jump",                 gc_jump},
	{"Spin",                 gc_use},
	{"Look Up",              gc_lookup},
	{"Look Down",            gc_lookdown},
	{"Center View",          gc_centerview},
	{"Toggle Mouselook",     gc_mouseaiming},
	{"Toggle Third-Person",  gc_camtoggle},
	{"Reset Camera",         gc_camreset},
	{"Game Status",          gc_scores},
	{"Pause / Run Retry",    gc_pause},
	{"Screenshot",           gc_screenshot},
	{"Toggle GIF Recording", gc_recordgif},
	{"Open/Close Menu",      gc_systemmenu},
	{"Change Viewpoint",     gc_viewpoint},
	{"Talk",                 gc_talkkey},
	{"Talk (Team only)",     gc_teamkey},
	{"Fire",                 gc_fire},
	{"Fire Normal",          gc_firenormal},
	{"Toss Flag",            gc_tossflag},
	{"Next Weapon",          gc_weaponnext},
	{"Prev Weapon",          gc_weaponprev},
	{"Custom Action 1",      gc_custom1},
	{"Custom Action 2",      gc_custom2},
	{"Custom Action 3",      gc_custom3},
	{NULL,                   gc_null},
};

static boolean SetupNewButtonSubmenu(touchfinger_t *finger)
{
	INT32 i;

	touchcust_submenu_button_t *btn = &touchcust_submenu_buttons[0], *lastbtn;
	touchcust_submenu_numbuttons = 0;

	// "Add" button
	btn->w = 32;
	btn->h = 24;
	btn->x = ((TOUCHCUST_SUBMENU_BASEX + TOUCHCUST_SUBMENU_WIDTH) - btn->w) - 4;
	btn->y = (TOUCHCUST_SUBMENU_BASEY + TOUCHCUST_SUBMENU_HEIGHT) + 4;

	btn->color = 112;
	btn->name = "ADD";
	btn->action = Submenu_AddNewButton_NewButtonAction;

	lastbtn = btn;
	btn++;
	touchcust_submenu_numbuttons++;

	// "Exit" button
	btn->w = 42;
	btn->h = 24;
	btn->x = (lastbtn->x - btn->w) - 4;
	btn->y = lastbtn->y;

	btn->color = 35;
	btn->name = "EXIT";
	btn->action = Submenu_Generic_ExitAction;

	touchcust_submenu_numbuttons++;

	// Create button list
	touchcust_submenu_selection = 0;
	touchcust_submenu_listsize = 0;

	touchcust_submenu_scroll = 0;
	memset(&touchcust_submenu_scrollbar, 0x00, sizeof(touchcust_submenu_scrollbar_t) * NUMTOUCHFINGERS);

	for (i = 0; (touchcust_buttonlist[i].gc != gc_null); i++)
	{
		INT32 gc = touchcust_buttonlist[i].gc;
		touchconfig_t *btn = &usertouchcontrols[gc];

		if (touchcust_submenu_listsize >= TOUCHCUST_SUBMENU_MAXLISTSIZE)
			break;

		// Button does not exist, so add it to the list.
		if (btn->hidden)
		{
			touchcust_submenu_list[touchcust_submenu_listsize] = gc;
			touchcust_submenu_listnames[touchcust_submenu_listsize] = touchcust_buttonlist[i].name;
			touchcust_submenu_listsize++;
		}
	}

	// Set last finger position
	touchcust_addbutton_x = finger->x;
	touchcust_addbutton_y = finger->y;

	// Returns true if any item was added to the list.
	return (touchcust_submenu_listsize > 0);
}

//
// Handles finger long press
//
static boolean HandleLongPress(void *f)
{
	touchfinger_t *finger = (touchfinger_t *)f;

	if (finger->longpress >= (TICRATE/2))
	{
		if (!SetupNewButtonSubmenu(finger))
			return true;

		OpenSubmenu(touchcust_submenu_newbtn);
		return true;
	}

	return false;
}

boolean TS_HandleCustomization(INT32 x, INT32 y, touchfinger_t *finger, event_t *event)
{
	boolean touchmotion = (event->type == ev_touchmotion);
	boolean foundbutton = false;
	boolean optionsarea = false;
	INT32 gc = finger->u.gamecontrol, i;

	touchconfig_t *btn = NULL;
	touchcust_buttonstatus_t *btnstatus = NULL;

	if (touchcust_submenufuncs[touchcust_submenu])
	{
		(touchcust_submenufuncs[touchcust_submenu])(x, y, finger, event);
		return true;
	}

	switch (event->type)
	{
		case ev_touchdown:
		case ev_touchmotion:
			// Ignore when the console is open
			if (CON_Ready())
				break;

			if (finger->u.gamecontrol != gc_null)
			{
				btn = &usertouchcontrols[finger->u.gamecontrol];
				btnstatus = &touchcustbuttons[finger->u.gamecontrol];
				optionsarea = IsFingerTouchingButtonOptions(x, y, finger, btn, btnstatus);
			}

			for (i = (num_gamecontrols - 1); i >= 0; i--)
			{
				btn = &usertouchcontrols[i];
				btnstatus = &touchcustbuttons[i];

				// Ignore hidden buttons
				if (btn->hidden)
					continue;

				// Ignore D-Pad buttons
				if (btn->dpad)
					continue;

				// Only move selected button
				if (touchmotion)
				{
					if (finger->u.gamecontrol != i)
						continue;

					if (btnstatus->optsel)
						continue;
				}

				// Select options
				if (HandleButtonOptions(x, y, finger, btn, btnstatus, false) && (i == finger->u.gamecontrol))
				{
					finger->u.gamecontrol = i;
					foundbutton = true;
					break;
				}
				// Move selected button
				else if (btnstatus->selected && touchmotion && (i == finger->u.gamecontrol))
				{
					if (btnstatus->resizearea && (!btnstatus->moving))
					{
						HandleResizePointSelection(x, y, finger, btn, btnstatus);
						foundbutton = true;
						break;
					}

					if (HandleResizePointSelection(x, y, finger, btn, btnstatus) && (!btnstatus->moving))
						btnstatus->resizearea = true;
					else
					{
						fixed_t dx = FixedDiv((x - finger->x) * FRACUNIT, vid.dupx * FRACUNIT);
						fixed_t dy = FixedDiv((y - finger->y) * FRACUNIT, vid.dupy * FRACUNIT);
						OffsetButtonBy(btn, dx, dy);
						btnstatus->moving = true;
					}

					btnstatus->isselecting = false;

					finger->x = x;
					finger->y = y;

					foundbutton = true;
					break;
				}
				// Check if your finger touches this button.
				else if (FingerTouchesButton(x, y, btn, false) && (!optionsarea))
				{
					// Let go of other fingers
					ClearAllSelections();

					finger->u.gamecontrol = i;
					btnstatus->isselecting = true;
					btnstatus->selected = true;
					btnstatus->moving = false;
					btnstatus->finger = finger;
					btnstatus->isresizing = touchcust_resizepoint_none;

					finger->x = x;
					finger->y = y;

					foundbutton = true;
					break;
				}
			}

			finger->x = x;
			finger->y = y;

			// long press
			if (finger->longpressaction && touchmotion)
			{
				finger->longpressaction = NULL;
				finger->longpress = 0;
			}
			else if (!foundbutton && (!touchmotion))
			{
				for (i = 0; i < NUMKEYS; i++)
				{
					touchconfig_t *btn = &touchnavigation[i];

					// Ignore hidden buttons
					if (btn->hidden)
						continue;

					// Check if your finger touches this button.
					if (G_FingerTouchesButton(x, y, btn))
						return false;
				}

				finger->longpressaction = HandleLongPress;
				return true;
			}

			if (touchmotion || foundbutton)
				return true;

			break;
		case ev_touchup:
			// Let go of this finger.
			finger->x = x;
			finger->y = y;

			if (finger->longpressaction)
			{
				finger->longpressaction = NULL;
				finger->longpress = 0;
			}

			gc = finger->u.gamecontrol;

			if (gc > gc_null)
			{
				btn = &usertouchcontrols[gc];
				btnstatus = &touchcustbuttons[gc];

				// Select options
				if (HandleButtonOptions(x, y, finger, btn, btnstatus, true))
				{
					finger->u.gamecontrol = gc_null;
					btnstatus->optsel = touchcust_options_none;
					break;
				}

				// Deselect button
				if (!btnstatus->moving && !btnstatus->isselecting && (btnstatus->isresizing < 0))
				{
					btnstatus->finger = NULL;
					ClearSelection(btnstatus);
					finger->u.gamecontrol = gc_null;
				}
				// Stop moving button
				else
				{
					// Snap to the grid
					if (btnstatus->snaptogrid && (btnstatus->moving || btnstatus->isresizing >= 0))
						SnapButtonToGrid(btn);

					// Clear movement
					btnstatus->isselecting = false;
					btnstatus->moving = false;
					btnstatus->resizearea = false;
					btnstatus->isresizing = touchcust_resizepoint_none;
				}
			}

			return true;
		default:
			break;
	}

	return false;
}

static void DrawGrid(void)
{
	INT32 i;
	INT32 col = 0;
	INT32 alpha = 8;
	INT32 scol = 15;
	INT32 salpha = 2;

	for (i = 0; i < BASEVIDWIDTH; i += TOUCHSMALLGRIDSIZE)
		V_DrawFadeFill(i-1, 0, 1, BASEVIDHEIGHT, 0, scol, salpha);
	for (i = 0; i < BASEVIDHEIGHT; i += TOUCHSMALLGRIDSIZE)
		V_DrawFadeFill(0, i-1, BASEVIDWIDTH, 1, 0, scol, salpha);

	for (i = 0; i < BASEVIDWIDTH; i += TOUCHGRIDSIZE)
		V_DrawFadeFill(i-1, 0, 1, BASEVIDHEIGHT, 0, col, alpha);
	for (i = 0; i < BASEVIDHEIGHT; i += TOUCHGRIDSIZE)
		V_DrawFadeFill(0, i-1, BASEVIDWIDTH, 1, 0, col, alpha);
}

static void DrawButtonOption(touchcust_option_e opt, touchconfig_t *btn, touchcust_buttonstatus_t *btnstatus)
{
	INT32 px, py, pw, ph;
	INT32 sx, sy;
	INT32 strwidth, strheight;
	UINT8 col;
	char str[5];

	if (!GetButtonOption(btn, btnstatus, opt, &px, &py, &pw, &ph, &col, str))
		return;

	V_DrawFill(px, py, pw, ph, col|V_NOSCALESTART);

	strwidth = V_StringWidth(str, V_NOSCALESTART|V_ALLOWLOWERCASE);
	strheight = 8;
	sx = (px + (pw / 2)) - (strwidth / 2);
	sy = (py + (ph / 2)) - ((strheight * vid.dupy) / 2);
	V_DrawString(sx, sy, V_NOSCALESTART|V_ALLOWLOWERCASE, str);
}

void TS_DrawCustomization(void)
{
	static INT32 flash = 0;
	INT32 i, j;

	INT32 red = 35;
	INT32 green = 112;
	INT32 yellow = 72;
	INT32 orange = 54;

	DrawGrid();
	ST_drawTouchGameInput(usertouchcontrols, true, 10);

	for (i = 0; i < num_gamecontrols; i++)
	{
		touchconfig_t *btn = &usertouchcontrols[i];
		touchcust_buttonstatus_t *btnstatus = &touchcustbuttons[i];
		INT32 tx, ty, tw, th;
		INT32 col, blinkcol;
		boolean blink = ((flash % TICRATE) <= TICRATE/2);

		// Not selected
		if (!btnstatus->selected)
			continue;

		// Ignore hidden buttons
		if (btn->hidden)
			continue;

		// Ignore D-Pad buttons
		if (btn->dpad)
			continue;

		GetButtonRect(btn, &tx, &ty, &tw, &th, false);

		blinkcol = (blink ? red : yellow);
		col = (btnstatus->moving ? green : blinkcol);

		V_DrawFill(tx, ty, tw, vid.dupy, col|V_NOSCALESTART);
		V_DrawFill(tx, ty + th, tw, vid.dupy, col|V_NOSCALESTART);

		V_DrawFill(tx, ty, vid.dupx, th, col|V_NOSCALESTART);
		V_DrawFill(tx + tw, ty, vid.dupx, th, col|V_NOSCALESTART);

		// Draw options and resize points
		if (!btnstatus->moving)
		{
			// Draw options
			for (j = (INT32)touchcust_options_first; j < (INT32)num_touchcust_options; j++)
				DrawButtonOption(j, btn, btnstatus);

			// Draw resize points
			for (j = 0; j < touchcust_numresizepoints; j++)
			{
				INT32 px, py, pw, ph;
				vector2_t *point = &touchcust_resizepoints[j];
				GetButtonResizePoint(btn, point, &px, &py, &pw, &ph);
				V_DrawFill(px, py, pw, ph, orange|V_NOSCALESTART);
			}
		}
	}

	if (touchcust_submenudrawfuncs[touchcust_submenu])
		(touchcust_submenudrawfuncs[touchcust_submenu])();

	flash++;
}

#endif // TOUCHINPUTS
