// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ts_custom.h
/// \brief Touch controls customization

#include "doomdata.h"
#include "doomtype.h"
#include "doomdef.h"

#include "d_event.h"
#include "g_input.h"

#ifdef TOUCHINPUTS
boolean TS_Ready(void);

void TS_SetupCustomization(void);
boolean TS_ExitCustomization(void);
boolean TS_IsCustomizingControls(void);

void TS_OpenLayoutList(void);
void TS_MakeLayoutList(void);

#define MAXTOUCHLAYOUTNAME     40
#define MAXTOUCHLAYOUTFILENAME 10

#define TOUCHLAYOUTSFILE "layouts.cfg"

typedef struct
{
	touchconfig_t *config;
	char name[MAXTOUCHLAYOUTNAME+1];
	char filename[MAXTOUCHLAYOUTFILENAME+1];
	boolean saved, loaded;
} touchlayout_t;

extern touchlayout_t *touchlayouts;
extern INT32 numtouchlayouts;

extern touchlayout_t *usertouchlayout;
extern INT32 usertouchlayoutnum;
extern boolean userlayoutsaved;

extern char touchlayoutfolder[512];

extern boolean ts_init;

void TS_InitLayouts(void);
void TS_LoadLayouts(void);
void TS_SaveLayouts(void);

void TS_NewLayout(void);
void TS_ClearLayout(void);
void TS_DeleteLayout(INT32 layoutnum);

void TS_CopyConfigTo(touchlayout_t *to, touchconfig_t *from);
void TS_CopyLayoutTo(touchlayout_t *to, touchlayout_t *from);

boolean TS_LoadSingleLayout(INT32 ilayout);
boolean TS_SaveSingleLayout(INT32 ilayout);

char *TS_GetShortLayoutName(touchlayout_t *layout, size_t maxlen);

boolean TS_HandleCustomization(INT32 x, INT32 y, touchfinger_t *finger, event_t *event);
boolean TS_HandleKeyEvent(INT32 key, event_t *event);

void TS_UpdateCustomization(void);
void TS_DrawCustomization(void);

fixed_t TS_GetDefaultScale(void);
void TS_GetJoystick(INT32 *x, INT32 *y, INT32 *w, INT32 *h, boolean tiny);

#define MTOUCH_SNAPTOSMALLGRID

typedef enum
{
	touchcust_submenu_none = 0,

	touchcust_submenu_newbtn,
	touchcust_submenu_layouts,

	num_touchcust_submenus,
} touchcust_submenu_e;

#define TOUCHCUST_SUBMENU_MAXBUTTONS 16

#define TOUCHCUST_NEXTSUBMENUBUTTON \
	lastbtn = btn; \
	btn++; \
	touchcust_submenu_numbuttons++;

#define TOUCHCUST_LASTSUBMENUBUTTON touchcust_submenu_numbuttons++;

typedef struct
{
	touchfinger_t *finger;

	// movement
	boolean selected;
	boolean isselecting;

	boolean moving;
	boolean resizearea;
	INT32 isresizing;
	INT32 optsel;

	// options
	boolean snaptogrid;
} touchcust_buttonstatus_t;

typedef enum
{
	touchcust_resizepoint_none = -1,

	touchcust_resizepoint_topleft,
	touchcust_resizepoint_topmiddle,
	touchcust_resizepoint_topright,

	touchcust_resizepoint_bottomleft,
	touchcust_resizepoint_bottommiddle,
	touchcust_resizepoint_bottomright,

	touchcust_resizepoint_leftside,
	touchcust_resizepoint_rightside,

	touchcust_numresizepoints
} touchcust_resizepoint_e;

typedef enum
{
	touchcust_options_none = 0,
	touchcust_options_first = 1,

	touchcust_option_snaptogrid = touchcust_options_first,
	touchcust_option_remove,

	num_touchcust_options
} touchcust_option_e;

#define MINBTNWIDTH 8*FRACUNIT
#define MINBTNHEIGHT 8*FRACUNIT

#define TOUCHGRIDSIZE      16
#define TOUCHSMALLGRIDSIZE (TOUCHGRIDSIZE / 2)

#define BUTTONEXTENDH (4 * vid.dupx)
#define BUTTONEXTENDV (4 * vid.dupy)

typedef struct
{
	INT32 x, y, w, h;
	UINT8 color;
	const char *name;

	touchfinger_t *isdown;
	void (*action)(INT32 x, INT32 y, touchfinger_t *finger, event_t *event);
} touchcust_submenu_button_t;

#define TOUCHCUST_SUBMENU_MAXLISTSIZE  256
#define TOUCHCUST_SUBMENU_DISPLAYITEMS 7
#define TOUCHCUST_SUBMENU_SCROLLITEMS  3

#define TOUCHCUST_SUBMENU_BASEX 60
#define TOUCHCUST_SUBMENU_BASEY 20

#define TOUCHCUST_SUBMENU_WIDTH     (24*8+6)
#define TOUCHCUST_SUBMENU_HEIGHT    145
#define TOUCHCUST_SUBMENU_SBARWIDTH 12

typedef struct
{
	boolean dragged;
	INT32 y;
} touchcust_submenu_scrollbar_t;

#endif // TOUCHINPUTS
