// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ts_draw.c
/// \brief Touch screen rendering

#include "ts_main.h"
#include "ts_draw.h"

#include "doomstat.h" // paused
#include "d_netcmd.h" // cv_playercolor
#include "f_finale.h" // F_GetPromptHideHud
#include "g_game.h" // promptblockcontrols
#include "hu_stuff.h" // HU_FONTSTART
#include "m_misc.h" // moviemode
#include "p_tick.h" // leveltime
#include "p_local.h" // stplyr
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "i_joy.h" // JOYAXISRANGE

#ifdef TOUCHINPUTS

#define SCALEBUTTONFIXED(touch, notonmenu) \
	if (touch->modifying) \
	{ \
		x = FloatToFixed(touch->supposed.x); \
		y = FloatToFixed(touch->supposed.y); \
		w = FloatToFixed(touch->supposed.w); \
		h = FloatToFixed(touch->supposed.h); \
		TS_CenterCoords(&x, &y); \
	} \
	else \
	{ \
		x = touch->x; \
		y = touch->y; \
		TS_DenormalizeCoords(&x, &y); \
		w = touch->w; \
		h = touch->h; \
		if (!touch->dontscale) \
		{ \
			x = FixedMul(x, dupx); \
			y = FixedMul(y, dupy); \
			w = FixedMul(w, dupx); \
			h = FixedMul(h, dupy); \
			if (notonmenu) \
				TS_CenterCoords(&x, &y); \
		} \
	}

#define SCALEBUTTON(touch) \
	SCALEBUTTONFIXED(touch, true) \
	x /= FRACUNIT; \
	y /= FRACUNIT; \
	w /= FRACUNIT; \
	h /= FRACUNIT;

#define SCALEMENUBUTTON(touch) \
	SCALEBUTTONFIXED(touch, false) \
	x /= FRACUNIT; \
	y /= FRACUNIT; \
	w /= FRACUNIT; \
	h /= FRACUNIT;

#define drawfill(dx, dy, dw, dh, dcol, dflags) \
	if (alphalevel < 10) \
		V_DrawFadeFill(dx, dy, dw, dh, dflags, dcol, alphalevel); \
	else \
		V_DrawFill(dx, dy, dw, dh, dcol|dflags);

void TS_DrawJoystickBacking(fixed_t padx, fixed_t pady, fixed_t padw, fixed_t padh, fixed_t scale, UINT8 color, INT32 flags)
{
	fixed_t x, y, w, h;
	fixed_t dupx = vid.dupx*FRACUNIT;
	fixed_t dupy = vid.dupy*FRACUNIT;
	fixed_t xscale, yscale;
	patch_t *backing = W_CachePatchLongName("JOY_BACKING", PU_PATCH);

	// generate colormap
	static UINT8 *colormap = NULL;
	static UINT8 lastcolor = 0;
	size_t colsize = 256 * sizeof(UINT8);

	if (colormap == NULL)
		colormap = Z_Calloc(colsize, PU_STATIC, NULL);
	if (color != lastcolor)
	{
		memset(colormap, color, colsize);
		lastcolor = color;
	}

	// scale coords
	x = FixedMul(padx, dupx);
	y = FixedMul(pady, dupy);
	w = FixedMul(padw, dupx);
	h = FixedMul(padh, dupy);

	TS_CenterCoords(&x, &y);

	xscale = FixedMul(FixedDiv(padw, SHORT(backing->width)*FRACUNIT), scale);
	yscale = FixedMul(FixedDiv(padh, SHORT(backing->height)*FRACUNIT), scale);

	V_DrawStretchyFixedPatch(
		((x + FixedDiv(w, 2 * FRACUNIT)) - (((SHORT(backing->width) * vid.dupx) / 2) * xscale)),
		((y + FixedDiv(h, 2 * FRACUNIT)) - (((SHORT(backing->height) * vid.dupy) / 2) * yscale)),
		xscale, yscale, flags, backing, colormap);
}

static void TS_DrawDPadButton(
	fixed_t x, fixed_t y, fixed_t w, fixed_t h,
	INT32 shx, INT32 shy,
	patch_t *patch, INT32 flags, boolean isdown,
	UINT8 *colormap)
{
	fixed_t offs = (3*FRACUNIT);

	x *= BASEVIDWIDTH * vid.dupx;
	y *= BASEVIDHEIGHT * vid.dupy;

	TS_CenterCoords(&x, &y);

	// draw shadow
	if (!isdown)
	{
		static UINT8 *shadow = NULL;
		size_t colsize = 256 * sizeof(UINT8);

		if (shadow == NULL)
			shadow = Z_Calloc(colsize, PU_STATIC, NULL);
		memset(shadow, 31, colsize);
		V_DrawStretchyFixedPatch(x + (offs * shx), y + (offs * shy), w, h, flags, patch, shadow);
	}

	// draw button
	V_DrawStretchyFixedPatch(x, (isdown ? y+offs : y), w, h, flags, patch, (isdown ? colormap : NULL));
}

void TS_DrawDPad(fixed_t dpadx, fixed_t dpady, fixed_t dpadw, fixed_t dpadh, INT32 accent, INT32 flags, touchconfig_t *config, boolean backing)
{
	INT32 x, y, w, h;
	fixed_t dupx = vid.dupx * FRACUNIT;
	fixed_t dupy = vid.dupy * FRACUNIT;
	fixed_t xscale, yscale;

	touchconfig_t *tleft = &config[gc_strafeleft];
	touchconfig_t *tright = &config[gc_straferight];
	touchconfig_t *tup = &config[gc_forward];
	touchconfig_t *tdown = &config[gc_backward];

	touchconfig_t *tul = &config[gc_dpadul];
	touchconfig_t *tur = &config[gc_dpadur];
	touchconfig_t *tdl = &config[gc_dpaddl];
	touchconfig_t *tdr = &config[gc_dpaddr];

	patch_t *up = W_CachePatchLongName("DPAD_UP", PU_PATCH);
	patch_t *down = W_CachePatchLongName("DPAD_DOWN", PU_PATCH);
	patch_t *left = W_CachePatchLongName("DPAD_LEFT", PU_PATCH);
	patch_t *right = W_CachePatchLongName("DPAD_RIGHT", PU_PATCH);

	patch_t *ul = W_CachePatchLongName("DPAD_UL", PU_PATCH);
	patch_t *ur = W_CachePatchLongName("DPAD_UR", PU_PATCH);
	patch_t *dl = W_CachePatchLongName("DPAD_DL", PU_PATCH);
	patch_t *dr = W_CachePatchLongName("DPAD_DR", PU_PATCH);

	boolean moveleft = touchcontroldown[gc_strafeleft];
	boolean moveright = touchcontroldown[gc_straferight];
	boolean moveup = touchcontroldown[gc_forward];
	boolean movedown = touchcontroldown[gc_backward];

	boolean moveul = touchcontroldown[gc_dpadul];
	boolean moveur = touchcontroldown[gc_dpadur];
	boolean movedl = touchcontroldown[gc_dpaddl];
	boolean movedr = touchcontroldown[gc_dpaddr];

	// generate colormap
	static UINT8 *colormap = NULL;
	static UINT8 lastcolor = 0;
	size_t colsize = 256 * sizeof(UINT8);

	if (colormap == NULL)
		colormap = Z_Calloc(colsize, PU_STATIC, NULL);
	if (accent != lastcolor)
	{
		memset(colormap, accent, colsize);
		lastcolor = accent;
	}

	// O backing
	if (backing)
		TS_DrawJoystickBacking(dpadx, dpady, dpadw, dpadh, 3*FRACUNIT/2, 20, flags);

	SCALEBUTTONFIXED(tup, true);
	xscale = FixedDiv(tup->w, SHORT(up->width)*FRACUNIT);
	yscale = FixedDiv(tup->h, SHORT(up->height)*FRACUNIT);
	TS_DrawDPadButton(tup->x, tup->y, xscale, yscale, 0, 1, up, flags, moveup, colormap);

	SCALEBUTTONFIXED(tdown, true);
	xscale = FixedDiv(tdown->w, SHORT(down->width)*FRACUNIT);
	yscale = FixedDiv(tdown->h, SHORT(down->height)*FRACUNIT);
	TS_DrawDPadButton(tdown->x, tdown->y, xscale, yscale, 0, 1, down, flags, movedown, colormap);

	SCALEBUTTONFIXED(tleft, true);
	xscale = FixedDiv(tleft->w, SHORT(left->width)*FRACUNIT);
	yscale = FixedDiv(tleft->h, SHORT(left->height)*FRACUNIT);
	TS_DrawDPadButton(tleft->x, tleft->y, xscale, yscale, -1, 1, left, flags, moveleft, colormap);

	SCALEBUTTONFIXED(tright, true);
	xscale = FixedDiv(tright->w, SHORT(right->width)*FRACUNIT);
	yscale = FixedDiv(tright->h, SHORT(right->height)*FRACUNIT);
	TS_DrawDPadButton(tright->x, tright->y, xscale, yscale, 1, 1, right, flags, moveright, colormap);

	// diagonals
	SCALEBUTTONFIXED(tul, true);
	xscale = FixedDiv(tul->w, SHORT(ul->width)*FRACUNIT);
	yscale = FixedDiv(tul->h, SHORT(ul->height)*FRACUNIT);
	TS_DrawDPadButton(tul->x, tul->y, xscale, yscale, 1, 1, ul, flags, moveul, colormap);

	SCALEBUTTONFIXED(tur, true);
	xscale = FixedDiv(tur->w, SHORT(ur->width)*FRACUNIT);
	yscale = FixedDiv(tur->h, SHORT(ur->height)*FRACUNIT);
	TS_DrawDPadButton(tur->x, tur->y, xscale, yscale, -1, 1, ur, flags, moveur, colormap);

	SCALEBUTTONFIXED(tdl, true);
	xscale = FixedDiv(tdl->w, SHORT(dl->width)*FRACUNIT);
	yscale = FixedDiv(tdl->h, SHORT(dl->height)*FRACUNIT);
	TS_DrawDPadButton(tdl->x, tdl->y, xscale, yscale, 1, 1, dl, flags, movedl, colormap);

	SCALEBUTTONFIXED(tdr, true);
	xscale = FixedDiv(tdr->w, SHORT(dr->width)*FRACUNIT);
	yscale = FixedDiv(tdr->h, SHORT(dr->height)*FRACUNIT);
	TS_DrawDPadButton(tdr->x, tdr->y, xscale, yscale, -1, 1, dr, flags, movedr, colormap);
}

void TS_DrawJoystick(fixed_t dpadx, fixed_t dpady, fixed_t dpadw, fixed_t dpadh, UINT8 color, INT32 flags)
{
	patch_t *cursor = W_CachePatchLongName("JOY_CURSOR", PU_PATCH);
	fixed_t dupx = vid.dupx*FRACUNIT;
	fixed_t dupy = vid.dupy*FRACUNIT;
	fixed_t pressure = max(FRACUNIT/2, FRACUNIT - FloatToFixed(touchpressure));

	// scale coords
	fixed_t x = FixedMul(dpadx, dupx);
	fixed_t y = FixedMul(dpady, dupy);
	fixed_t w = FixedMul(dpadw, dupx);
	fixed_t h = FixedMul(dpadh, dupy);

	float xmove = 0.0f, ymove = 0.0f;
	fixed_t stickx, sticky;
	joystickvector2_t *joy = &movejoystickvectors[0];

	fixed_t basexscale = FixedDiv(dpadw, SHORT(cursor->width)*FRACUNIT);
	fixed_t baseyscale = FixedDiv(dpadh, SHORT(cursor->height)*FRACUNIT);
	fixed_t xscale = FixedMul(pressure, (basexscale / 2));
	fixed_t yscale = FixedMul(pressure, (baseyscale / 2));
	fixed_t xextend = TOUCHJOYEXTENDX;
	fixed_t yextend = TOUCHJOYEXTENDY;

	// generate colormap
	static UINT8 *colormap = NULL;
	static UINT8 lastcolor = 0;
	size_t colsize = 256 * sizeof(UINT8);

	if (colormap == NULL)
		colormap = Z_Calloc(colsize, PU_STATIC, NULL);
	if (color != lastcolor)
	{
		memset(colormap, color, colsize);
		lastcolor = color;
	}

	TS_CenterCoords(&x, &y);

	// set stick position
	if (joy->xaxis)
		xmove = ((float)joy->xaxis / JOYAXISRANGE);
	if (joy->yaxis)
		ymove = ((float)joy->yaxis / JOYAXISRANGE);

	stickx = max(-xextend, min(FixedMul(FloatToFixed(xmove), xextend), xextend));
	sticky = max(-yextend, min(FixedMul(FloatToFixed(ymove), yextend), yextend));

	// O backing
	TS_DrawJoystickBacking(dpadx, dpady, dpadw, dpadh, FRACUNIT, 20, flags);

	// Hole
	V_DrawStretchyFixedPatch(
		((x + FixedDiv(w, 2 * FRACUNIT)) - (((SHORT(cursor->width) * vid.dupx) / 2) * (basexscale / 4))),
		((y + FixedDiv(h, 2 * FRACUNIT)) - (((SHORT(cursor->height) * vid.dupy) / 2) * (baseyscale / 4))),
		(basexscale / 4), (baseyscale / 4), flags, cursor, NULL);

	// Stick
	V_DrawStretchyFixedPatch(
		((x + FixedDiv(w, 2 * FRACUNIT)) - (((SHORT(cursor->width) * vid.dupx) / 2) * xscale)) + (stickx * vid.dupx),
		((y + FixedDiv(h, 2 * FRACUNIT)) - (((SHORT(cursor->height) * vid.dupy) / 2) * yscale)) + (sticky * vid.dupy),
		xscale, yscale, flags, cursor, colormap);
}

//
// Touch input in-game
//

static void TS_DrawTouchGameInputButton(touchconfig_t *config, INT32 gctype, const char *str, INT32 keycol, const INT32 accent, INT32 alphalevel, INT32 flags)
{
	touchconfig_t *control = &config[gctype];
	INT32 x, y, w, h;
	INT32 col, offs;
	INT32 shadow = vid.dupy;
	fixed_t dupx = vid.dupx * FRACUNIT;
	fixed_t dupy = vid.dupy * FRACUNIT;

	if (!control->hidden && !F_GetPromptHideHud(control->y / vid.dupy))
	{
		fixed_t strx, stry;
		fixed_t strwidth, strheight;
		INT32 strflags = (flags | V_ALLOWLOWERCASE);
		boolean drawthin = false;
		const char *defaultkeystr = control->name;
		const char *optkeystr = ((str != NULL) ? str : NULL);
		const char *keystr = ((optkeystr != NULL) ? optkeystr : defaultkeystr);

		if (!keystr)
			return;

		SCALEBUTTON(control);

		// Draw the button
		if (touchcontroldown[gctype])
		{
			col = accent;
			offs = shadow;
		}
		else
		{
			col = keycol;
			offs = 0;
			drawfill(x, y + h, w, shadow, 29, flags);
		}
		drawfill(x, y + offs, w, h, col, flags);

		// Draw the button name
		SCALEBUTTONFIXED(control, true);

		// String width
		strwidth = V_StringWidth(keystr, strflags) * FRACUNIT;
		drawthin = ((strwidth + (2 * FRACUNIT)) >= w);

		// Too long? Draw thinner string
		if (drawthin)
		{
			fixed_t thinoffs = (FRACUNIT / 2);
			strwidth = ((V_ThinStringWidth(keystr, strflags) * vid.dupx) * FRACUNIT) + thinoffs;

			// Still too long? Draw abbreviated name
			if (((strwidth+2) >= w) && (!optkeystr))
			{
				keystr = control->tinyname;
				strwidth = V_StringWidth(keystr, strflags) * FRACUNIT;
				drawthin = ((strwidth + (2 * FRACUNIT)) >= w);
				if (drawthin)
					strwidth = ((V_ThinStringWidth(keystr, strflags) * vid.dupx) * FRACUNIT) + thinoffs;
			}
		}

		// String height
		strheight = (8 * FRACUNIT);
		if (drawthin)
			strheight -= FRACUNIT;

		strx = (x + (w / 2)) - (strwidth / 2);
		stry = ((y + (h / 2)) - ((strheight * vid.dupy) / 2) + (offs * FRACUNIT));

		if (drawthin)
			V_DrawThinStringAtFixed(strx, stry, strflags, keystr);
		else
			V_DrawStringAtFixed(strx, stry, strflags, keystr);
	}
}

static INT32 GetInputAccent(void)
{
	if (stplyr && stplyr->skincolor)
		return skincolors[stplyr->skincolor].ramp[4];
	return TS_BUTTONDOWNCOLOR;
}

void TS_DrawControls(touchconfig_t *config, boolean drawgamecontrols, INT32 alphalevel)
{
	const INT32 transflag = ((10-alphalevel)<<V_ALPHASHIFT);
	const INT32 flags = (transflag | V_NOSCALESTART);
	const INT32 accent = GetInputAccent();

	INT32 i;
	INT32 noncontrolbtns[] = {gc_systemmenu, gc_viewpoint, gc_screenshot, gc_talkkey, gc_scores, gc_camtoggle, gc_camreset};
	INT32 numnoncontrolbtns = (INT32)(sizeof(noncontrolbtns) / sizeof(INT32));

	if (!alphalevel)
		return;

	// Draw movement control
	if (!promptblockcontrols && drawgamecontrols && (!config[gc_joystick].hidden))
	{
		// Draw the d-pad
		if (touch_movementstyle == tms_dpad)
			TS_DrawDPad(touch_joystick_x, touch_joystick_y, touch_joystick_w, touch_joystick_h, accent, flags, config, true);
		else // Draw the joystick
			TS_DrawJoystick(touch_joystick_x, touch_joystick_y, touch_joystick_w, touch_joystick_h, accent, flags);
	}

#define drawbutton(gctype, str, keycol) TS_DrawTouchGameInputButton(config, gctype, str, keycol, accent, alphalevel, flags)
#define drawbtn(gctype) drawbutton(gctype, NULL, TS_BUTTONUPCOLOR)
#define drawbtnname(gctype, str) drawbutton(gctype, str, TS_BUTTONUPCOLOR)
#define drawcolbtn(gctype, col) drawbutton(gctype, NULL, col)

	if (drawgamecontrols)
	{
		for (i = 0; i < num_gamecontrols; i++)
		{
			if (TS_ButtonIsPlayerControl(i) && (!config[i].hidden))
				drawbtn(i);
		}
	}

	//
	// Non-control buttons
	//
	for (i = 0; i < numnoncontrolbtns; i++)
	{
		INT32 gc = noncontrolbtns[i];
		if (!config[gc].hidden)
			drawbtn(gc);
	}

	// Pause
	drawbtnname(gc_pause, (paused ? "\x1D" : "II"));

	// Movie mode
	drawcolbtn(gc_recordgif, (moviemode ? ((leveltime & 16) ? 36 : 43) : 36));

	// Team talk key
	drawcolbtn(gc_teamkey, accent);

#undef drawcolbtn
#undef drawbtnname
#undef drawbtn
#undef drawbutton
}

//
// Menu touch input
//

void TS_DrawMenuNavigation(void)
{
	fixed_t dupx = vid.dupx*FRACUNIT;
	fixed_t dupy = vid.dupy*FRACUNIT;
	const INT32 alphalevel = cv_touchmenutrans.value;
	const INT32 transflag = ((10-alphalevel)<<V_ALPHASHIFT);
	const INT32 flags = (transflag | V_NOSCALESTART);
	const INT32 accent = TS_BUTTONDOWNCOLOR;
	const INT32 shadow = vid.dupy;
	touchconfig_t *control;
	INT32 col, offs;
	INT32 x, y, w, h;
	patch_t *font;

	if (!touchscreenavailable || inputmethod != INPUTMETHOD_TOUCH || !alphalevel)
		return;

#define drawbtn(keyname) \
	control = &touchnavigation[keyname]; \
	if (!control->hidden) \
	{ \
		char symb = control->name[0]; \
		SCALEMENUBUTTON(control); \
		if (control->down) \
		{ \
			col = accent; \
			offs = shadow; \
		} \
		else \
		{ \
			col = control->color; \
			offs = 0; \
			if (alphalevel >= 10) \
				V_DrawFill(x, y + h, w, shadow, 29|flags); \
		} \
		font = hu_font[toupper(symb) - HU_FONTSTART]; \
		drawfill(x, y + offs, w, h, col, flags); \
		V_DrawCharacter((x + (w / 2)) - ((SHORT(font->width)*vid.dupx) / 2), \
						(y + (h / 2)) - ((SHORT(font->height)*vid.dupx) / 2) + offs, \
						symb|flags, false); \
	}

	drawbtn(KEY_ESCAPE); // left arrow
	drawbtn(KEY_ENTER); // right arrow
	drawbtn(KEY_CONSOLE);

#undef drawbtn
}

#undef drawfill
#undef SCALEBUTTON

#endif // TOUCHINPUTS
