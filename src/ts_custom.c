// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2022 by Jaime Ita Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ts_custom.c
/// \brief Touch controls customization
/// \todo  Allow customization from the keyboard.

#include "doomdata.h"
#include "doomtype.h"
#include "doomdef.h"

#include "ts_main.h"
#include "ts_draw.h"
#include "ts_custom.h"

#include "d_event.h"
#include "g_input.h"

#include "m_fixed.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_random.h"

#include "d_event.h"
#include "g_input.h"
#include "i_system.h" // I_mkdir

#include "f_finale.h" // curfadevalue
#include "v_video.h"
#include "st_stuff.h" // TS_DrawControls
#include "hu_stuff.h" // shiftxform
#include "s_sound.h" // S_StartSound
#include "z_zone.h"

#include "console.h" // CON_Ready()

#ifdef TOUCHINPUTS
static touchcust_buttonstatus_t touchcustbuttons[NUM_GAMECONTROLS];

static touchcust_submenu_e touchcust_submenu = touchcust_submenu_none;
static touchcust_submenu_button_t touchcust_submenu_buttons[TOUCHCUST_SUBMENU_MAXBUTTONS];
static INT32 touchcust_submenu_numbuttons = 0;

static INT32 touchcust_submenu_selection = 0;
static INT32 touchcust_submenu_highlight = -1;
static INT32 touchcust_submenu_listsize = 0;
static INT32 touchcust_submenu_list[TOUCHCUST_SUBMENU_MAXLISTSIZE];
static const char *touchcust_submenu_listnames[TOUCHCUST_SUBMENU_MAXLISTSIZE];

static INT32 touchcust_submenu_x;
static INT32 touchcust_submenu_y;
static INT32 touchcust_submenu_width;
static INT32 touchcust_submenu_height;

static touchcust_submenu_scrollbar_t touchcust_submenu_scrollbar[NUMTOUCHFINGERS];
static INT32 touchcust_submenu_scroll = 0;

static INT32 touchcust_addbutton_x = 0;
static INT32 touchcust_addbutton_y = 0;
static boolean touchcust_addbutton_finger = false;

static boolean touchcust_customizing = false;
static char *touchcust_deferredmessage = NULL;

static touchlayout_t *touchcust_layoutlist_renaming = NULL;

// ==========
// Prototypes
// ==========

static boolean LoadLayoutAtIndex(INT32 idx);
static boolean LoadLayoutFromName(const char *layoutname);
static boolean LoadLayoutOnList(void);

static boolean FingerTouchesRect(INT32 fx, INT32 fy, INT32 rx, INT32 ry, INT32 rw, INT32 rh);
static boolean FingerTouchesButton(INT32 x, INT32 y, touchconfig_t *btn, boolean inside);

static void GetButtonRect(touchconfig_t *btn, INT32 *x, INT32 *y, INT32 *w, INT32 *h, boolean inside);
static void GetButtonResizePoint(touchconfig_t *btn, vector2_t *point, INT32 *x, INT32 *y, INT32 *w, INT32 *h);
static boolean GetButtonOption(touchconfig_t *btn, touchcust_buttonstatus_t *btnstatus, touchcust_option_e opt, INT32 *x, INT32 *y, INT32 *w, INT32 *h, UINT8 *col, char *str);

static void MoveButtonTo(touchconfig_t *btn, INT32 x, INT32 y);
static void OffsetButtonBy(touchconfig_t *btn, float offsx, float offsy);
static void SnapButtonToGrid(touchconfig_t *btn);
static fixed_t RoundSnapCoord(fixed_t a, fixed_t b);

static void SetButtonSupposedLocation(touchconfig_t *btn);
static void MoveButtonToSupposedLocation(touchconfig_t *btn);

static void UpdateJoystickBase(touchconfig_t *btn);
static void UpdateJoystickSize(touchconfig_t *btn);
static void NormalizeDPad(void);

static INT32 AddButton(INT32 x, INT32 y, touchfinger_t *finger, event_t *event);
static void RemoveButton(touchconfig_t *btn);

static void ClearSelection(touchcust_buttonstatus_t *selection);
static void ClearAllSelections(void);

static void OpenSubmenu(touchcust_submenu_e submenu);
static void CloseSubmenu(void);

static void FocusSubmenuOnSelection(INT32 selection);
static boolean HandleSubmenuButtons(INT32 fx, INT32 fy, touchfinger_t *finger, event_t *event);

static void GetSubmenuListItems(size_t *t, size_t *i, size_t *b, size_t *height, boolean scrolled);
static void GetSubmenuScrollbar(INT32 basex, INT32 basey, INT32 *x, INT32 *y, INT32 *w, INT32 *h, boolean scrolled);

static boolean IsMovingSubmenuScrollbar(void);

static boolean HandleResizePointSelection(INT32 x, INT32 y, touchfinger_t *finger, touchconfig_t *btn, touchcust_buttonstatus_t *btnstatus);

static boolean HandleButtonOptions(INT32 x, INT32 y, touchfinger_t *finger, touchconfig_t *btn, touchcust_buttonstatus_t *btnstatus, boolean execute);
static boolean IsFingerTouchingButtonOptions(INT32 x, INT32 y, touchfinger_t *finger, touchconfig_t *btn, touchcust_buttonstatus_t *btnstatus);

// =====================================================

void TS_SetupCustomization(void)
{
	INT32 i;

	touchscreenavailable = true;
	touchcust_customizing = true;

	ClearAllSelections();
	CloseSubmenu();

	for (i = 0; i < NUM_GAMECONTROLS; i++)
	{
		touchcust_buttonstatus_t *btnstatus = &touchcustbuttons[i];
		btnstatus->snaptogrid = true;
	}
}

boolean TS_ExitCustomization(void)
{
	size_t layoutsize = (sizeof(touchconfig_t) * NUM_GAMECONTROLS);

	if (touchcust_submenu != touchcust_submenu_none)
	{
		touchcust_submenu_e lastmenu = touchcust_submenu;
		CloseSubmenu();
		return (lastmenu == touchcust_submenu_layouts);
	}

	ClearAllSelections();

	// Copy custom controls
	M_Memcpy(&touchcontrols, usertouchcontrols, layoutsize);

	return true;
}

#define MAXMESSAGELAYOUTNAME 24

static void DisplayMessage(const char *message)
{
	touchcust_deferredmessage = Z_StrDup(message);
}

static void StopRenamingLayout(touchlayout_t *layout)
{
	if (I_KeyboardOnScreen() && !CON_Ready())
		I_CloseScreenKeyboard();

	if (layout)
	{
		if (!strlen(layout->name))
			strlcpy(layout->name, "Unnamed layout", MAXTOUCHLAYOUTNAME+1);
		TS_SaveLayouts();
	}

	touchcust_layoutlist_renaming = NULL;

	if (touchcust_submenu == touchcust_submenu_layouts)
		TS_MakeLayoutList();
}

static boolean UseGridLimits(void)
{
	if (TS_IsPresetActive() || (usertouchlayout && usertouchlayout->widescreen))
		return false;

	return (cv_touchlayoutusegrid.value == 1);
}

// =======
// Layouts
// =======

touchlayout_t *touchlayouts = NULL;
INT32 numtouchlayouts = 0;

touchlayout_t *usertouchlayout = NULL;
INT32 usertouchlayoutnum = UNSAVEDTOUCHLAYOUT;
boolean userlayoutsaved = true;
boolean userlayoutnew = true;

char touchlayoutfolder[512] = "touchlayouts";

#define LAYOUTLISTSAVEFORMAT "%s: %s\n"

void TS_InitLayouts(void)
{
	if (ts_ready)
		return;

	I_mkdir(touchlayoutfolder, 0755);

	usertouchlayout = Z_Calloc(sizeof(touchlayout_t), PU_STATIC, NULL);

	TS_DefaultControlLayout(false);
	TS_SetDefaultLayoutSettings(usertouchlayout);
	TS_SynchronizeLayoutCvarsFromSettings(usertouchlayout);

	ts_ready = true;
}

void TS_LoadLayouts(void)
{
	FILE *f;
	char line[128];

	char *ch, *end;
	char name[MAXTOUCHLAYOUTNAME+1], filename[MAXTOUCHLAYOUTFILENAME+1];

	memset(line, 0x00, sizeof(line));

	f = fopen(va("%s"PATHSEP"%s", touchlayoutfolder, TOUCHLAYOUTSFILE), "rt");
	if (!f)
		return;

	numtouchlayouts = 0;

	while (fgets(line, sizeof(line), f) != NULL)
	{
		touchlayout_t *layout;
		size_t offs;

		end = strchr(line, ':');
		if (!end)
			continue;

		// copy filename
		*end = '\0';
		strlcpy(filename, line, MAXTOUCHLAYOUTFILENAME+1);

		if (!strlen(filename))
			continue;

		// copy layout name
		end++;
		if (*end == '\0')
			continue;

		ch = (end + strspn(end, "\t "));
		if (*ch == '\0')
			continue;

		strlcpy(name, ch, MAXTOUCHLAYOUTNAME+1);

		offs = strlen(name);
		if (!offs)
			continue;

		offs--;
		if (name[offs] == '\n' || name[offs] == '\r')
			name[offs] = '\0';

		// allocate layout
		TS_NewLayout();

		layout = touchlayouts + (numtouchlayouts - 1);
		layout->saved = true;

		strlcpy(layout->filename, filename, MAXTOUCHLAYOUTFILENAME+1);
		strlcpy(layout->name, name, MAXTOUCHLAYOUTNAME+1);
	}

	fclose(f);
}

boolean TS_SaveLayouts(void)
{
	FILE *f = NULL;
	touchlayout_t *layout = touchlayouts;
	INT32 i;

	if (!I_StoragePermission())
		return false;

	f = fopen(va("%s"PATHSEP"%s", touchlayoutfolder, TOUCHLAYOUTSFILE), "w");
	if (!f)
		return false;

	for (i = 0; i < numtouchlayouts; i++)
	{
		if (layout->saved)
		{
			const char *line = va(LAYOUTLISTSAVEFORMAT, layout->filename, layout->name);
			fwrite(line, strlen(line), 1, f);
		}
		layout++;
	}

	fclose(f);
	return true;
}

void TS_NewLayout(void)
{
	touchlayout_t *layout;

	numtouchlayouts++;
	touchlayouts = Z_Realloc(touchlayouts, (numtouchlayouts * sizeof(touchlayout_t)), PU_STATIC, NULL);

	layout = touchlayouts + (numtouchlayouts - 1);
	memset(layout, 0x00, sizeof(touchlayout_t));
}

// Sets default settings for a layout
void TS_SetDefaultLayoutSettings(touchlayout_t *layout)
{
	layout->usegridlimits = false;
	layout->widescreen = true;
}

// For backwards compatibility with old layouts
void TS_SetDefaultLayoutSettingsCompat(touchlayout_t *layout)
{
	layout->usegridlimits = true;
	layout->widescreen = false;
}

// Sets layout cvars from layout settings
void TS_SynchronizeLayoutCvarsFromSettings(touchlayout_t *layout)
{
	CV_StealthSetValue(&cv_touchlayoutusegrid, layout->usegridlimits);
	CV_StealthSetValue(&cv_touchlayoutwidescreen, layout->widescreen);
}

// Sets layout settings from layout cvars
void TS_SynchronizeLayoutSettingsFromCvars(touchlayout_t *layout)
{
	layout->usegridlimits = cv_touchlayoutusegrid.value;
	layout->widescreen = cv_touchlayoutwidescreen.value;
}

// Sets layout settings from cvars when they're changed
void TS_SynchronizeCurrentLayout(void)
{
	if (!ts_ready)
		return;

	if (usertouchlayoutnum != UNSAVEDTOUCHLAYOUT)
	{
		TS_SynchronizeLayoutSettingsFromCvars(touchlayouts + usertouchlayoutnum);
		userlayoutsaved = false;
	}
}

static void ResetAllConfigButtons(touchconfig_t *config)
{
	INT32 i;

	for (i = 0; i < NUM_GAMECONTROLS; i++, config++)
	{
		config->hidden = false; // reset hidden value so that all buttons are properly populated
		config->w = config->h = 0;
	}
}

void TS_ClearLayout(touchlayout_t *layout)
{
	ResetAllConfigButtons(layout->config);
	TS_BuildLayoutFromPreset(layout);
}

void TS_ClearCurrentLayout(boolean setdefaults)
{
	size_t layoutsize = sizeof(touchconfig_t) * NUM_GAMECONTROLS;

	if (setdefaults)
	{
		TS_SetDefaultLayoutSettings(usertouchlayout);
		TS_SynchronizeLayoutCvarsFromSettings(usertouchlayout);
	}
	else
		TS_SynchronizeLayoutSettingsFromCvars(usertouchlayout);

	TS_ClearLayout(usertouchlayout);

	M_Memcpy(usertouchcontrols, usertouchlayout->config, layoutsize);
	M_Memcpy(&touchcontrols, usertouchlayout->config, layoutsize);

	userlayoutsaved = false;
	userlayoutnew = true;
}

touchconfigstatus_t usertouchconfigstatus;

void TS_BuildLayoutFromPreset(touchlayout_t *layout)
{
	TS_BuildPreset(layout->config, &usertouchconfigstatus, tms_joystick, TS_GetDefaultScale(), false, layout->widescreen);

	// Position joystick, weapon buttons, etc.
	TS_PositionExtraUserButtons();

	// Mark movement controls as d-pad buttons
	TS_MarkDPadButtons(layout->config);
}

void TS_DefaultControlLayout(boolean makelayout)
{
	usertouchconfigstatus.altliveshud = true;
	usertouchconfigstatus.ringslinger = true;
	usertouchconfigstatus.ctfgametype = true;
	usertouchconfigstatus.canpause = true;
	usertouchconfigstatus.canviewpointswitch = true;
	usertouchconfigstatus.cantalk = true;
	usertouchconfigstatus.canteamtalk = true;

	if (!makelayout)
		return;

	if (usertouchcontrols == NULL)
	{
		usertouchcontrols = Z_Calloc(sizeof(touchconfig_t) * NUM_GAMECONTROLS, PU_STATIC, NULL);
		usertouchlayout->config = usertouchcontrols;
		TS_BuildLayoutFromPreset(usertouchlayout);
	}
}

void TS_DeleteLayout(INT32 layoutnum)
{
	touchlayout_t *todelete = (touchlayouts + layoutnum);

	if (usertouchlayout == todelete || (!(numtouchlayouts-1) && usertouchlayoutnum != UNSAVEDTOUCHLAYOUT))
	{
		usertouchlayout = Z_Calloc(sizeof(touchlayout_t), PU_STATIC, NULL);
		usertouchlayoutnum = UNSAVEDTOUCHLAYOUT;
		TS_SynchronizeLayoutSettingsFromCvars(usertouchlayout);
		TS_CopyLayoutTo(usertouchlayout, todelete);
		CV_StealthSet(&cv_touchlayout, "None");
		if (!todelete->saved)
			userlayoutsaved = userlayoutnew = false;
	}

	if (usertouchlayoutnum >= layoutnum)
	{
		usertouchlayoutnum--;
		touchcust_submenu_highlight = usertouchlayoutnum;
	}

	if (layoutnum < numtouchlayouts-1)
		memmove(todelete, (todelete + 1), (numtouchlayouts - (layoutnum + 1)) * sizeof(touchlayout_t));

	numtouchlayouts--;

	if (numtouchlayouts)
	{
		touchlayouts = Z_Realloc(touchlayouts, (numtouchlayouts * sizeof(touchlayout_t)), PU_STATIC, NULL);

		if (usertouchlayoutnum != UNSAVEDTOUCHLAYOUT)
			usertouchlayout = (touchlayouts + usertouchlayoutnum);
		else
			touchcust_submenu_highlight = -1;
	}
	else
	{
		Z_Free(touchlayouts);
		touchlayouts = NULL;
		if (usertouchlayoutnum == UNSAVEDTOUCHLAYOUT)
			touchcust_submenu_highlight = -1;
	}

	if (touchcust_submenu_selection >= numtouchlayouts)
		FocusSubmenuOnSelection(numtouchlayouts - 1);
}

void TS_CopyConfigTo(touchlayout_t *to, touchconfig_t *from)
{
	size_t layoutsize = sizeof(touchconfig_t) * NUM_GAMECONTROLS;

	if (!to->config)
		to->config = Z_Malloc(layoutsize, PU_STATIC, NULL);

	M_Memcpy(to->config, from, layoutsize);
}

#define sizeofmember(type, member) sizeof(((type *)NULL)->member)

void TS_CopyLayoutTo(touchlayout_t *to, touchlayout_t *from)
{
	size_t layoutsize = sizeof(touchconfig_t) * NUM_GAMECONTROLS;

	if (!to->config)
		to->config = Z_Calloc(layoutsize, PU_STATIC, NULL);

	if (from->config)
		M_Memcpy(to->config, from->config, layoutsize);
	else
		I_Error("TS_CopyLayoutTo: no layout to copy from!");

	to->usegridlimits = from->usegridlimits;
	to->widescreen = from->widescreen;
}

#undef sizeofmember

#define BUTTONLOADFORMAT "%64s %f %f %f %f"
#define BUTTONSAVEFORMAT "%s %f %f %f %f" "\n"

#define OPTIONLOADFORMAT "%64s %d"
#define OPTIONSAVEFORMAT "%s %d" "\n"

#define LAYOUTFILEDELIM "---\n"

struct {
	const char *name;
	consvar_t *cvar;
} const layoutoptions[] = {
	{"usegridlimits", &cv_touchlayoutusegrid},
	{"widescreen",    &cv_touchlayoutwidescreen},
	{NULL,            NULL},
};

boolean TS_LoadSingleLayout(INT32 ilayout)
{
	touchlayout_t *layout = touchlayouts + ilayout;

	FILE *f;
	char filename[MAXTOUCHLAYOUTFILENAME+5];
	size_t layoutsize = (sizeof(touchconfig_t) * NUM_GAMECONTROLS);

	float x, y, w, h;
	char gc[65];

	char option[65];
	INT32 optionvalue;

	INT32 igc;
	touchconfig_t *button = NULL;

	if (layout->loaded || !I_StoragePermission())
		return true;

	strcpy(filename, layout->filename);
	FIL_ForceExtension(filename, ".cfg");

	f = fopen(va("%s"PATHSEP"%s", touchlayoutfolder, filename), "rt");
	if (!f)
	{
		S_StartSound(NULL, sfx_lose);
		DisplayMessage(va(M_GetText(
			"\x82%s\n"
			"\x85""Failed to load layout!\n"
			"\n%s"),
			TS_GetShortLayoutName(layout, MAXMESSAGELAYOUTNAME),
			M_GetUserActionString(PRESS_A_KEY_MESSAGE)));
		return false;
	}

	if (layout->config == NULL)
		layout->config = Z_Calloc(layoutsize, PU_STATIC, NULL);

	memset(layout->config, 0x00, layoutsize);
	for (igc = 0; igc < NUM_GAMECONTROLS; igc++)
	{
		button = &(layout->config[igc]);
		button->hidden = true;
	}

	while (fscanf(f, BUTTONLOADFORMAT, gc, &x, &y, &w, &h) == 5)
	{
		for (igc = 0; igc < NUM_GAMECONTROLS; igc++)
		{
			if (!strcmp(gc, gamecontrolname[igc]))
				break;
		}

		if (igc == GC_NULL || (igc >= GC_WEPSLOT1 && igc <= GC_WEPSLOT10) || igc == NUM_GAMECONTROLS)
			continue;

		button = &(layout->config[igc]);
		button->x = FloatToFixed(x);
		button->y = FloatToFixed(y);
		button->w = FloatToFixed(w);
		button->h = FloatToFixed(h);
		button->hidden = false;
	}

	// Set defaults
	TS_SetDefaultLayoutSettingsCompat(layout);
	TS_SynchronizeLayoutCvarsFromSettings(layout);

	// Read options
	while (fscanf(f, OPTIONLOADFORMAT, option, &optionvalue) == 2)
	{
		for (igc = 0; layoutoptions[igc].cvar; igc++)
		{
			if (!strcmp(option, layoutoptions[igc].name))
			{
				CV_StealthSetValue(layoutoptions[igc].cvar, optionvalue);
				break;
			}
		}
	}

	// set cvars
	TS_SynchronizeLayoutSettingsFromCvars(layout);

	TS_SetButtonNames(layout->config, NULL);
	TS_MarkDPadButtons(layout->config);

	layout->loaded = true;

	fclose(f);
	return true;
}

boolean TS_SaveSingleLayout(INT32 ilayout)
{
	FILE *f = NULL;
	char filename[MAXTOUCHLAYOUTFILENAME+5];

	touchlayout_t *layout = touchlayouts + ilayout;
	INT32 gc;
	const char *line;

	strcpy(filename, layout->filename);
	FIL_ForceExtension(filename, ".cfg");

	if (I_StoragePermission())
		f = fopen(va("%s"PATHSEP"%s", touchlayoutfolder, filename), "w");

	if (!f)
	{
		S_StartSound(NULL, sfx_lose);
		DisplayMessage(va(M_GetText(
			"\x82%s\n"
			"\x85""Failed to save layout!\n"
			"\n\x80%s"),
			layout->name, M_GetUserActionString(PRESS_A_KEY_MESSAGE)));
		return false;
	}

	for (gc = (GC_NULL+1); gc < NUM_GAMECONTROLS; gc++)
	{
		touchconfig_t *button = &(layout->config[gc]);

		if (button->hidden || (gc >= GC_WEPSLOT1 && gc <= GC_WEPSLOT10))
			continue;

		line = va(BUTTONSAVEFORMAT,
				gamecontrolname[gc],
				FixedToFloat(button->x), FixedToFloat(button->y),
				FixedToFloat(button->w), FixedToFloat(button->h));
		fwrite(line, strlen(line), 1, f);
	}

	fwrite(LAYOUTFILEDELIM, strlen(LAYOUTFILEDELIM), 1, f);

	// Save options
	for (gc = 0; layoutoptions[gc].cvar; gc++)
	{
		consvar_t *cvar = layoutoptions[gc].cvar;
		line = va(OPTIONSAVEFORMAT, layoutoptions[gc].name, cvar->value);
		fwrite(line, strlen(line), 1, f);
	}

	fclose(f);
	return true;
}

static boolean NoLayoutInCVar(const char *layoutname)
{
	return (!layoutname || strlen(layoutname) < 1 || !strcmp(layoutname, "None"));
}

static boolean CanLoadLayoutFromCVar(const char *layoutname)
{
	if (!ts_ready || NoLayoutInCVar(layoutname))
		return false;
	return true;
}

static void ResetLayoutMenus(void)
{
	if (touchcust_customizing)
	{
		CloseSubmenu();
		ClearAllSelections();
	}
	else if (touchcust_submenu == touchcust_submenu_layouts)
	{
		TS_MakeLayoutList();
		touchcust_submenu_highlight = usertouchlayoutnum;
	}
}

void TS_LoadLayoutFromCVar(void)
{
	const char *layoutname = cv_touchlayout.string;

	if (!CanLoadLayoutFromCVar(layoutname))
	{
		if (NoLayoutInCVar(layoutname))
		{
			usertouchlayoutnum = UNSAVEDTOUCHLAYOUT;
			userlayoutsaved = true;
			userlayoutnew = false;
			ResetLayoutMenus();
		}
		return;
	}

	if (touchcust_submenu == touchcust_submenu_layouts)
		StopRenamingLayout(touchcust_layoutlist_renaming);

	if (!LoadLayoutFromName(layoutname))
		return;

	userlayoutsaved = true;
	userlayoutnew = false;
	ResetLayoutMenus();
}

void TS_LoadUserLayouts(void)
{
	const char *layoutname = cv_touchlayout.string;

	TS_LoadLayouts();
	userlayoutnew = false;

	if (CanLoadLayoutFromCVar(layoutname))
		userlayoutsaved = LoadLayoutFromName(layoutname);
}

char *TS_GetShortLayoutName(touchlayout_t *layout, size_t maxlen)
{
	if (strlen(layout->name) > maxlen)
	{
		static char name[MAXTOUCHLAYOUTNAME + 4];
		strlcpy(name, layout->name, min(maxlen, MAXTOUCHLAYOUTNAME));
		strcat(name, "...");
		return name;
	}

	return layout->name;
}

static void CreateAndSetupNewLayout(boolean setupnew)
{
	touchlayout_t *newlayout;
	INT32 newlayoutnum;

	TS_NewLayout();

	newlayoutnum = (numtouchlayouts - 1);
	newlayout = touchlayouts + newlayoutnum;

	FocusSubmenuOnSelection(numtouchlayouts - 1);
	touchcust_submenu_highlight = newlayoutnum;

	newlayout->loaded = true;

	strlcpy(newlayout->name, va("New layout %d", numtouchlayouts), MAXTOUCHLAYOUTNAME+1);
	strlcpy(newlayout->filename, va("layout%d", numtouchlayouts), MAXTOUCHLAYOUTFILENAME+1);

	// Copy or create config
	if (setupnew)
	{
		size_t layoutsize = sizeof(touchconfig_t) * NUM_GAMECONTROLS;

		newlayout->config = Z_Calloc(layoutsize, PU_STATIC, NULL);
		TS_BuildLayoutFromPreset(usertouchlayout);

		M_Memcpy(usertouchcontrols, newlayout->config, layoutsize);

		TS_SetDefaultLayoutSettings(newlayout);
		TS_SynchronizeLayoutCvarsFromSettings(newlayout);
	}
	else
	{
		TS_CopyLayoutTo(newlayout, usertouchlayout);
		TS_SynchronizeLayoutSettingsFromCvars(newlayout);
	}

	usertouchlayout = newlayout;
	usertouchlayoutnum = newlayoutnum;
	userlayoutsaved = true;
	userlayoutnew = true;

	TS_MakeLayoutList();
}

//
// Layout list
//

static void SubmenuMessageResponse_LayoutList_New(INT32 ch)
{
	if (ch == 'y' || ch == KEY_ENTER)
	{
		S_StartSound(NULL, sfx_strpst);
		CreateAndSetupNewLayout(false);
	}
}

static void Submenu_LayoutList_New(INT32 x, INT32 y, touchfinger_t *finger, event_t *event)
{
	(void)x;
	(void)y;
	(void)finger;
	(void)event;

	M_StartMessage(va(M_GetText(
		"Create a new layout?\n"
		"\n(%s)\n"), M_GetUserActionString(CONFIRM_MESSAGE)),
	SubmenuMessageResponse_LayoutList_New, MM_YESNO);
}

static boolean LoadLayoutAtIndex(INT32 idx)
{
	size_t layoutsize = (sizeof(touchconfig_t) * NUM_GAMECONTROLS);

	if (!TS_LoadSingleLayout(idx))
		return false;

	usertouchlayout = (touchlayouts + idx);
	usertouchlayoutnum = idx;

	CV_StealthSet(&cv_touchlayout, usertouchlayout->name);
	TS_SynchronizeLayoutCvarsFromSettings(usertouchlayout);

	if (usertouchcontrols == NULL)
		usertouchcontrols = Z_Calloc(sizeof(touchconfig_t) * NUM_GAMECONTROLS, PU_STATIC, NULL);

	M_Memcpy(usertouchcontrols, usertouchlayout->config, layoutsize);
	M_Memcpy(&touchcontrols, usertouchcontrols, layoutsize);

	// Position joystick, weapon buttons, etc.
	TS_PositionExtraUserButtons();

	// Mark movement controls as d-pad buttons
	TS_MarkDPadButtons(usertouchcontrols);

	return true;
}

static boolean LoadLayoutFromName(const char *layoutname)
{
	touchlayout_t *userlayout = touchlayouts;
	INT32 i;

	for (i = 0; i < numtouchlayouts; i++)
	{
		if (!strcmp(userlayout->name, layoutname))
			break;
		userlayout++;
	}

	if (i == numtouchlayouts)
	{
		CONS_Alert(CONS_ERROR, "Touch layout \"%s\" not found\n", layoutname);
		return false;
	}

	if (!LoadLayoutAtIndex(i))
	{
		CONS_Alert(CONS_ERROR, "Could not load touch layout \"%s\"\n", layoutname);
		return false;
	}

	return true;
}

static boolean LoadLayoutOnList(void)
{
	if (!LoadLayoutAtIndex(touchcust_submenu_selection))
		return false;

	userlayoutsaved = true;
	userlayoutnew = false;
	TS_MakeLayoutList();

	return true;
}

static void SubmenuMessageResponse_LayoutList_Load(INT32 ch)
{
	if (ch == 'y' || ch == KEY_ENTER)
	{
		if (LoadLayoutOnList())
		{
			S_StartSound(NULL, sfx_strpst);
			DisplayMessage(va(M_GetText(
				"\x82%s\n"
				"\x84Layout loaded!\n"
				"\n%s"),
				TS_GetShortLayoutName((touchlayouts + usertouchlayoutnum), MAXMESSAGELAYOUTNAME),
				M_GetUserActionString(PRESS_A_KEY_MESSAGE)));
			touchcust_submenu_highlight = usertouchlayoutnum;
		}
	}
}

static void Submenu_LayoutList_Load(INT32 x, INT32 y, touchfinger_t *finger, event_t *event)
{
	const char *layoutname;

	(void)x;
	(void)y;
	(void)finger;
	(void)event;

	if (touchlayouts == NULL || !numtouchlayouts)
	{
		S_StartSound(NULL, sfx_lose);
		return;
	}

	layoutname = TS_GetShortLayoutName((touchlayouts + touchcust_submenu_selection), MAXMESSAGELAYOUTNAME);

	if (userlayoutsaved && (usertouchlayoutnum == touchcust_submenu_selection))
	{
		S_StartSound(NULL, sfx_skid);
		DisplayMessage(va(M_GetText(
			"\x82%s\n"
			"This layout is already loaded!\n"
			"\n%s"),
		layoutname, M_GetUserActionString(PRESS_A_KEY_MESSAGE)));
		return;
	}

	if (!userlayoutsaved)
	{
		M_StartMessage(va(M_GetText(
			"\x82%s\n"
			"Load this layout?\n"
			"You will lose your unsaved changes."
			"\n\n(%s)\n"),
		layoutname, M_GetUserActionString(CONFIRM_MESSAGE)), SubmenuMessageResponse_LayoutList_Load, MM_YESNO);
	}
	else
	{
		M_StartMessage(va(M_GetText(
			"\x82%s\n"
			"Load this layout?\n"
			"\n(%s)\n"),
		layoutname, M_GetUserActionString(CONFIRM_MESSAGE)), SubmenuMessageResponse_LayoutList_Load, MM_YESNO);
	}
}

static void SaveLayoutOnList(void)
{
	touchlayout_t *savelayout = NULL;

	if (touchcust_submenu_selection != usertouchlayoutnum)
	{
		savelayout = touchlayouts + touchcust_submenu_selection;

		TS_CopyConfigTo(usertouchlayout, usertouchcontrols);
		TS_CopyLayoutTo(savelayout, usertouchlayout);

		usertouchlayout = savelayout;
		usertouchlayoutnum = touchcust_submenu_selection;

		touchcust_submenu_highlight = touchcust_submenu_selection;
	}
	else
	{
		savelayout = touchlayouts + usertouchlayoutnum;
		TS_CopyConfigTo(usertouchlayout, usertouchcontrols);
		TS_CopyLayoutTo(savelayout, usertouchlayout);
	}

	TS_SaveSingleLayout(usertouchlayoutnum);

	if (TS_SaveLayouts())
	{
		userlayoutsaved = true;
		userlayoutnew = false;
		savelayout->saved = true;

		S_StartSound(NULL, sfx_strpst);
		DisplayMessage(va(M_GetText(
			"\x82%s\n"
			"\x83Layout saved!\n"
			"\n%s"),
		TS_GetShortLayoutName((touchlayouts + usertouchlayoutnum), MAXMESSAGELAYOUTNAME),
		M_GetUserActionString(PRESS_A_KEY_MESSAGE)));
	}
	else
	{
		S_StartSound(NULL, sfx_lose);
		DisplayMessage(va(M_GetText(
			"\x82%s\n"
			"\x85""Failed to save layout!\n"
			"\n%s"),
		TS_GetShortLayoutName((touchlayouts + usertouchlayoutnum), MAXMESSAGELAYOUTNAME),
		M_GetUserActionString(PRESS_A_KEY_MESSAGE)));
	}

	TS_MakeLayoutList();
}

static void SubmenuMessageResponse_LayoutList_Save(INT32 ch)
{
	if (ch == 'y' || ch == KEY_ENTER)
		SaveLayoutOnList();
}

static void Submenu_LayoutList_Save(INT32 x, INT32 y, touchfinger_t *finger, event_t *event)
{
	(void)x;
	(void)y;
	(void)finger;
	(void)event;

	if (touchlayouts == NULL || !numtouchlayouts)
	{
		S_StartSound(NULL, sfx_lose);
		return;
	}
	else if (usertouchlayoutnum == UNSAVEDTOUCHLAYOUT)
	{
		SaveLayoutOnList();
		return;
	}

	if (touchcust_submenu_selection != usertouchlayoutnum)
	{
		M_StartMessage(va(M_GetText(
			"\x82%s\n"
			"\x80Save over the \x82%s \x80layout?\n"
			"\n(%s to save"
			"\nor %s to return)\n"),
			TS_GetShortLayoutName((touchlayouts + usertouchlayoutnum), MAXMESSAGELAYOUTNAME),
			TS_GetShortLayoutName((touchlayouts + touchcust_submenu_selection), MAXMESSAGELAYOUTNAME),
			M_GetUserActionString(PRESS_Y_MESSAGE), M_GetUserActionString(PRESS_N_MESSAGE_L)),
		SubmenuMessageResponse_LayoutList_Save, MM_YESNO);
		return;
	}

	SaveLayoutOnList();
}

static void DeleteLayoutOnList(INT32 layoutnum)
{
	touchlayout_t *layout = touchlayouts + layoutnum;

	if (layout->saved)
	{
		char filename[MAXTOUCHLAYOUTFILENAME+5];

		TS_LoadSingleLayout(layoutnum);

		strcpy(filename, layout->filename);
		FIL_ForceExtension(filename, ".cfg");

		remove(va("%s"PATHSEP"%s", touchlayoutfolder, filename));
		layout->saved = false;
		if (usertouchlayoutnum == layoutnum)
			userlayoutsaved = userlayoutnew = false;

		TS_SaveLayouts();

		DisplayMessage(va(M_GetText(
			"\x82%s\n"
			"\x85""Layout deleted.\n"
			"\n%s"),
			TS_GetShortLayoutName(layout, MAXMESSAGELAYOUTNAME),
			M_GetUserActionString(PRESS_A_KEY_MESSAGE)));
	}
	else
	{
		TS_DeleteLayout(layoutnum);
		DisplayMessage(va(M_GetText("Layout deleted.\n%s"), M_GetUserActionString(PRESS_A_KEY_MESSAGE)));
	}

	TS_MakeLayoutList();
	S_StartSound(NULL, sfx_altdi1 + M_RandomKey(4));
}

static void SubmenuMessageResponse_LayoutList_Delete(INT32 ch)
{
	(void)ch;
	if (ch == 'y' || ch == KEY_ENTER)
		DeleteLayoutOnList(touchcust_submenu_selection);
}

static void Submenu_LayoutList_Delete(INT32 x, INT32 y, touchfinger_t *finger, event_t *event)
{
	touchlayout_t *layout;

	(void)x;
	(void)y;
	(void)finger;
	(void)event;

	if (touchlayouts == NULL || !numtouchlayouts)
	{
		S_StartSound(NULL, sfx_lose);
		return;
	}

	layout = (touchlayouts + touchcust_submenu_selection);

	M_StartMessage(va(M_GetText(
		"\x82%s\n"
		"\x80%s\n"
		"%s\n"
		"\n(%s to delete)\n"),

		TS_GetShortLayoutName(layout, MAXMESSAGELAYOUTNAME),

		(layout->saved
			? "Delete this layout from your device?"
			: "Remove this layout from the list?"),

		((layout->saved || (!layout->saved && usertouchlayout == layout))
			? "You will still be able to edit the layout."
			: "You will not be able to edit the layout."),

		M_GetUserActionString(PRESS_Y_MESSAGE)),

	SubmenuMessageResponse_LayoutList_Delete, MM_YESNO);
}

static void Submenu_LayoutList_Rename(INT32 x, INT32 y, touchfinger_t *finger, event_t *event)
{
	touchlayout_t *layout;

	(void)x;
	(void)y;
	(void)finger;
	(void)event;

	if (touchlayouts == NULL || !numtouchlayouts)
	{
		S_StartSound(NULL, sfx_lose);
		return;
	}

	layout = (touchlayouts + touchcust_submenu_selection);
	touchcust_layoutlist_renaming = layout;

#ifdef VIRTUAL_KEYBOARD
	I_ShowVirtualKeyboard(layout->name, MAXTOUCHLAYOUTNAME);
#endif
}

static void Submenu_LayoutList_Exit(INT32 x, INT32 y, touchfinger_t *finger, event_t *event)
{
	(void)x;
	(void)y;
	(void)finger;
	(void)event;
	CloseSubmenu();
	M_SetupNextMenu(currentMenu->prevMenu);
}

void TS_OpenLayoutList(void)
{
	touchcust_submenu_button_t *btn = &touchcust_submenu_buttons[0], *lastbtn;
	touchcust_submenu_numbuttons = 0;

	OpenSubmenu(touchcust_submenu_layouts);

	touchcust_submenu_selection = 0;
	touchcust_submenu_highlight = -1;
	TS_MakeLayoutList();

	touchcust_submenu_scroll = 0;
	memset(&touchcust_submenu_scrollbar, 0x00, sizeof(touchcust_submenu_scrollbar_t) * NUMTOUCHFINGERS);

	// "Load" button
	btn->w = 40;
	btn->h = 24;
	btn->x = ((touchcust_submenu_x + touchcust_submenu_width) - btn->w);
	btn->y = (touchcust_submenu_y + touchcust_submenu_height) + 4;

	btn->color = 151;
	btn->name = "LOAD";
	btn->action = Submenu_LayoutList_Load;

	TOUCHCUST_NEXTSUBMENUBUTTON

	// "New" button
	btn->w = 32;
	btn->h = 24;
	btn->x = (lastbtn->x - btn->w) - 4;
	btn->y = lastbtn->y;

	btn->color = 113;
	btn->name = "NEW";
	btn->action = Submenu_LayoutList_New;

	TOUCHCUST_NEXTSUBMENUBUTTON

	// "Save" button
	btn->w = 40;
	btn->h = 24;
	btn->x = (lastbtn->x - btn->w) - 4;
	btn->y = lastbtn->y;

	btn->color = 54;
	btn->name = "SAVE";
	btn->action = Submenu_LayoutList_Save;

	TOUCHCUST_NEXTSUBMENUBUTTON

	// "Rename" button
	btn->w = 32;
	btn->h = 24;
	btn->x = (lastbtn->x - btn->w) - 4;
	btn->y = lastbtn->y;

	btn->color = 212;
	btn->name = "REN";
	btn->action = Submenu_LayoutList_Rename;

	TOUCHCUST_NEXTSUBMENUBUTTON

	// "Delete" button
	btn->w = 32;
	btn->h = 24;
	btn->x = (lastbtn->x - btn->w) - 4;
	btn->y = lastbtn->y;

	btn->color = 35;
	btn->name = "DEL";
	btn->action = Submenu_LayoutList_Delete;

	TOUCHCUST_NEXTSUBMENUBUTTON

	// "Exit" button
	btn->w = 42;
	btn->h = 24;
	btn->x = touchcust_submenu_x;
	btn->y = lastbtn->y;

	btn->color = 16;
	btn->name = "EXIT";
	btn->action = Submenu_LayoutList_Exit;

	TOUCHCUST_LASTSUBMENUBUTTON

	touchscreenavailable = true;
	touchcust_customizing = false;

	if (usertouchlayoutnum != UNSAVEDTOUCHLAYOUT)
	{
		FocusSubmenuOnSelection(usertouchlayoutnum);
		touchcust_submenu_highlight = touchcust_submenu_selection;
	}
}

void TS_MakeLayoutList(void)
{
	static char **layoutnames = NULL;
	static INT32 numlayoutnames = 0;

	touchlayout_t *layout = touchlayouts;
	INT32 i;

	touchcust_submenu_listsize = 0;

	if (layoutnames)
	{
		for (i = 0; i < numlayoutnames; i++)
		{
			if (layoutnames[i])
				Z_Free(layoutnames[i]);
		}
		Z_Free(layoutnames);
	}

	layoutnames = Z_Calloc(numtouchlayouts * sizeof(char *), PU_STATIC, NULL);
	numlayoutnames = numtouchlayouts;

	for (i = 0; i < numtouchlayouts; i++)
	{
		char *string, *append;
		const char *unsavedstr = " \x85(unsaved)";
		const char *modifiedstr = " \x87(modified)";
		size_t len = strlen(layout->name);
		size_t extlen = 0;
		size_t maxlen = 30;

		boolean unsaved = (!layout->saved);
		boolean modified = ((layout == usertouchlayout) && (!userlayoutsaved));

		if (unsaved)
		{
			size_t ln = strlen(unsavedstr);
			len += ln;
			extlen += ln;
		}
		if (modified)
		{
			size_t ln = strlen(modifiedstr);
			len += ln;
			extlen += ln;
		}

		string = Z_Malloc(len+1, PU_STATIC, NULL);
		strcpy(string, layout->name);

		append = string + strlen(string);

		if (unsaved)
			strcat(string, unsavedstr);
		if (modified)
			strcat(string, modifiedstr);

		if (len > maxlen)
		{
			layoutnames[i] = Z_Malloc(maxlen+1, PU_STATIC, NULL);

			if (extlen)
			{
				const char *attach;
				size_t len2, offs;

				append++;
				attach = va("...%s", append);
				len2 = strlen(attach) + 1;

				strlcpy(layoutnames[i], string, maxlen+1);

				offs = (maxlen + 1) - len2;
				strlcpy(layoutnames[i] + offs, attach, (maxlen + 1) - offs);
			}
			else
			{
				string[maxlen - 3] = '\0';
				snprintf(layoutnames[i], (maxlen+1), "%s...", string);
			}
		}
		else
			layoutnames[i] = Z_StrDup(string);

		Z_Free(string);

		layout++;
	}

	layout = touchlayouts;

	for (i = 0; i < numtouchlayouts; i++)
	{
		if (touchcust_submenu_listsize >= TOUCHCUST_SUBMENU_MAXLISTSIZE)
			break;

		touchcust_submenu_list[touchcust_submenu_listsize] = i;
		touchcust_submenu_listnames[touchcust_submenu_listsize] = layoutnames[i];
		touchcust_submenu_listsize++;

		layout++;
	}
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
	INT32 tx, ty, tw, th;

	if (btn->modifying)
	{
		tx = FixedInt(btn->supposed.fx);
		ty = FixedInt(btn->supposed.fy);
		tw = FixedInt(btn->supposed.fw);
		th = FixedInt(btn->supposed.fh);
	}
	else
	{
		tx = btn->x;
		ty = btn->y;
		tw = btn->w;
		th = btn->h;
		TS_ScaleCoords(&tx, &ty, &tw, &th, true, (!btn->dontscale));
	}

	TS_CenterIntegerCoords(&tx, &ty);

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

	INT32 psizew = 12 * vid.dupx;
	INT32 psizeh = 12 * vid.dupy;

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
// Calculates the fixed-point supposed positions of a button
//
static void CalcButtonFixedSupposedPos(touchconfig_t *btn)
{
	btn->supposed.fx = FloatToFixed(btn->supposed.x);
	btn->supposed.fy = FloatToFixed(btn->supposed.y);

#ifdef TSC_SNAPMOVE
	if (btn->supposed.snap)
	{
		btn->supposed.fx = TS_RoundSnapXCoord(btn->supposed.fx);
		btn->supposed.fy = TS_RoundSnapYCoord(btn->supposed.fy);
	}
#endif
}

//
// Calculates the fixed-point supposed dimensions of a button
//
static void CalcButtonFixedSupposedSize(touchconfig_t *btn)
{
	btn->supposed.fw = max(FloatToFixed(MINBTNWIDTH), FloatToFixed(btn->supposed.w));
	btn->supposed.fh = max(FloatToFixed(MINBTNHEIGHT), FloatToFixed(btn->supposed.h));

#ifdef TSC_SNAPMOVE
	if (btn->supposed.snap)
	{
		btn->supposed.fw = TS_RoundSnapWCoord(btn->supposed.fw);
		btn->supposed.fh = TS_RoundSnapHCoord(btn->supposed.fh);
	}
#endif
}

//
// Moves a button to (x, y)
//
static void MoveButtonTo(touchconfig_t *btn, INT32 x, INT32 y)
{
	SetButtonSupposedLocation(btn);

	btn->supposed.x = (float)x;
	btn->supposed.y = (float)y;

	CalcButtonFixedSupposedPos(btn);

	if (btn == &usertouchcontrols[GC_JOYSTICK])
		UpdateJoystickBase(btn);
}

//
// Offsets a button by (offsx, offsy)
//
static void OffsetButtonBy(touchconfig_t *btn, float offsx, float offsy)
{
	float w = (BASEVIDWIDTH * vid.dupx);
	float h = (BASEVIDHEIGHT * vid.dupy);

	SetButtonSupposedLocation(btn);

	btn->supposed.x += offsx;
	btn->supposed.y += offsy;

	if (UseGridLimits())
	{
		if (btn->supposed.x < 0.0f)
			btn->supposed.x = 0.0f;
		if (btn->supposed.x + btn->supposed.w > w)
			btn->supposed.x = (w - btn->supposed.w);

		if (btn->supposed.y < 0.0f)
			btn->supposed.y = 0.0f;
		if (btn->supposed.y + btn->supposed.h > h)
			btn->supposed.y = (h - btn->supposed.h);
	}

	CalcButtonFixedSupposedPos(btn);

	if (btn == &usertouchcontrols[GC_JOYSTICK])
		UpdateJoystickBase(btn);
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

fixed_t TS_RoundSnapXCoord(fixed_t x)
{
#ifdef TSC_SNAPTOSMALLGRID
	INT32 gridx = TOUCHSMALLGRIDSIZE;
#else
	INT32 gridx = TOUCHGRIDSIZE;
#endif

	return RoundSnapCoord(x, gridx * FRACUNIT);
}

fixed_t TS_RoundSnapYCoord(fixed_t y)
{
#ifdef TSC_SNAPTOSMALLGRID
	INT32 gridy = TOUCHSMALLGRIDSIZE;
#else
	INT32 gridy = TOUCHGRIDSIZE;
#endif

	return RoundSnapCoord(y, gridy * FRACUNIT);
}

fixed_t TS_RoundSnapWCoord(fixed_t w)
{
	return RoundSnapCoord(w, TOUCHSMALLGRIDSIZE * FRACUNIT);
}

fixed_t TS_RoundSnapHCoord(fixed_t h)
{
	return RoundSnapCoord(h, TOUCHSMALLGRIDSIZE * FRACUNIT);
}

//
// Snaps a button to a grid
//
static void SnapButtonToGrid(touchconfig_t *btn)
{
	TS_DenormalizeCoords(&btn->x, &btn->y);

	btn->x = TS_RoundSnapXCoord(btn->x);
	btn->y = TS_RoundSnapYCoord(btn->y);

	btn->w = TS_RoundSnapWCoord(btn->w);
	btn->h = TS_RoundSnapHCoord(btn->h);

	if (btn == &usertouchcontrols[GC_JOYSTICK])
		UpdateJoystickBase(btn);

	TS_NormalizeButton(btn);
}

//
// Set the "supposed" location of a button
//
static void SetButtonSupposedLocation(touchconfig_t *btn)
{
	if (!btn->modifying)
	{
		fixed_t x = btn->x;
		fixed_t y = btn->y;

		TS_DenormalizeCoords(&x, &y);

		btn->supposed.x = FixedToFloat(FixedMul(x, vid.dupx * FRACUNIT));
		btn->supposed.y = FixedToFloat(FixedMul(y, vid.dupy * FRACUNIT));
		btn->supposed.w = FixedToFloat(FixedMul(btn->w, vid.dupx * FRACUNIT));
		btn->supposed.h = FixedToFloat(FixedMul(btn->h, vid.dupy * FRACUNIT));

		CalcButtonFixedSupposedPos(btn);
		CalcButtonFixedSupposedSize(btn);

		btn->modifying = true;
	}
}

//
// Moves a button to its "supposed" location
//
static void MoveButtonToSupposedLocation(touchconfig_t *btn)
{
	if (btn->modifying)
	{
		btn->x = FixedDiv(btn->supposed.fx, vid.dupx * FRACUNIT);
		btn->y = FixedDiv(btn->supposed.fy, vid.dupy * FRACUNIT);
		btn->w = FixedDiv(btn->supposed.fw, vid.dupx * FRACUNIT);
		btn->h = FixedDiv(btn->supposed.fh, vid.dupy * FRACUNIT);
		btn->modifying = false;
		btn->supposed.snap = false;
		TS_NormalizeButton(btn);
	}
}

//
// Updates the joystick
//
static void UpdateJoystickBase(touchconfig_t *btn)
{
	fixed_t scale, xscale, yscale;
	fixed_t btnw, btnh;
	INT32 jw, jh;

	TS_GetJoystick(NULL, NULL, &jw, &jh, false);

	// Must not be normalized!
	if (btn->modifying)
	{
		touch_joystick_x = btn->supposed.fx / vid.dupx;
		touch_joystick_y = btn->supposed.fy / vid.dupy;

#ifdef TSC_SNAPMOVE
		if (btn->supposed.snap)
		{
			touch_joystick_x = TS_RoundSnapXCoord(touch_joystick_x);
			touch_joystick_y = TS_RoundSnapYCoord(touch_joystick_y);
		}
#endif
	}
	else
	{
		touch_joystick_x = btn->x;
		touch_joystick_y = btn->y;
	}

	// Update joystick size
	UpdateJoystickSize(btn);

	if (btn->modifying)
	{
		btnw = btn->supposed.fw / vid.dupx;
		btnh = btn->supposed.fh / vid.dupy;

#ifdef TSC_SNAPMOVE
		if (btn->supposed.snap)
		{
			btnw = TS_RoundSnapWCoord(btnw);
			btnh = TS_RoundSnapHCoord(btnh);
		}
#endif
	}
	else
	{
		btnw = btn->w;
		btnh = btn->h;
	}

	// Move d-pad
	scale = TS_GetDefaultScale();
	xscale = FixedMul(FixedDiv(btnw, jw), scale);
	yscale = FixedMul(FixedDiv(btnh, jh), scale);
	TS_DPadPreset(usertouchcontrols, xscale, yscale, btnw, false);

	// Normalize d-pad
	NormalizeDPad();
}

static void UpdateJoystickSize(touchconfig_t *btn)
{
	if (btn->modifying)
	{
		touch_joystick_w = btn->supposed.fw / vid.dupx;
		touch_joystick_h = btn->supposed.fh / vid.dupy;

#ifdef TSC_SNAPMOVE
		if (btn->supposed.snap)
		{
			touch_joystick_w = TS_RoundSnapWCoord(touch_joystick_w);
			touch_joystick_h = TS_RoundSnapHCoord(touch_joystick_h);
		}
#endif
	}
	else
	{
		touch_joystick_w = btn->w;
		touch_joystick_h = btn->h;
	}
}

static void NormalizeDPad(void)
{
	INT32 i;

	for (i = 0; i < NUM_GAMECONTROLS; i++)
	{
		if (TS_IsDPadButton(i))
			TS_NormalizeButton(&usertouchcontrols[i]);
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

	if (gc == GC_JOYSTICK)
	{
		fixed_t w, h;
		TS_GetJoystick(NULL, NULL, &w, &h, false);
		btn->w = w;
		btn->h = h;
	}
	else
	{
		btn->w = TOUCHCUST_DEFAULTBTNWIDTH * FRACUNIT;
		btn->h = TOUCHCUST_DEFAULTBTNHEIGHT * FRACUNIT;
	}

	if (!touchcust_addbutton_finger)
	{
		touchcust_addbutton_x = (vid.width / 2) - (btn->w / FRACUNIT);
		touchcust_addbutton_y = (vid.height / 2) - (btn->h / FRACUNIT);
	}

	MoveButtonTo(btn, touchcust_addbutton_x, touchcust_addbutton_y);

	if (btn == &usertouchcontrols[GC_JOYSTICK])
	{
		TS_DenormalizeCoords(&btn->x, &btn->y);
		UpdateJoystickBase(btn);
		TS_NormalizeButton(btn);
	}

	btn->name = TS_GetButtonName(gc, NULL);
	btn->tinyname = TS_GetButtonShortName(gc, NULL);

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
	{
		touchconfig_t *btn = &usertouchcontrols[selection->finger->u.gamecontrol];
		MoveButtonToSupposedLocation(btn);
		selection->finger->u.gamecontrol = GC_NULL;
	}

	selection->finger = NULL;
}

//
// Clears all selections
//
static void ClearAllSelections(void)
{
	INT32 i;
	for (i = 0; i < NUM_GAMECONTROLS; i++)
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

	touchcust_submenu_x = TOUCHCUST_SUBMENU_BASEX;
	touchcust_submenu_y = TOUCHCUST_SUBMENU_BASEY;
	touchcust_submenu_width = TOUCHCUST_SUBMENU_WIDTH;
	touchcust_submenu_height = TOUCHCUST_SUBMENU_HEIGHT;

	if (submenu == touchcust_submenu_layouts)
	{
		INT32 offs = 72;

		touchcust_submenu_x -= (offs / 2);
		touchcust_submenu_width += offs;

		touchcust_submenu_x++;
	}
}

static void CloseSubmenu(void)
{
	if (I_KeyboardOnScreen())
		I_CloseScreenKeyboard();

	if (touchcust_submenu == touchcust_submenu_layouts)
		StopRenamingLayout(touchcust_layoutlist_renaming);

	touchcust_submenu = touchcust_submenu_none;
}

boolean TS_IsCustomizationSubmenuOpen(void)
{
	if (!TS_IsCustomizingControls())
		return false;
	return (touchcust_submenu != touchcust_submenu_none);
}

static void FocusSubmenuOnSelection(INT32 selection)
{
	touchcust_submenu_selection = selection;
	touchcust_submenu_scroll = (touchcust_submenu_selection * vid.dupy * TOUCHCUST_SUBMENU_DISPLAYITEMS);
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
					finger->selection = (i+1);
					return true;
				}
			}

			break;

		case ev_touchup:
			if (finger->selection)
			{
				touchcust_submenu_button_t *btn = &touchcust_submenu_buttons[finger->selection-1];

				bx = btn->x * vid.dupx;
				by = btn->y * vid.dupy;
				bw = btn->w * vid.dupx;
				bh = btn->h * vid.dupy;

				bx += (vid.width - (BASEVIDWIDTH * dup)) / 2;
				by += (vid.height - (BASEVIDHEIGHT * dup)) / 2;

				btn->isdown = NULL;

				if (FingerTouchesRect(fx, fy, bx, by, bw, bh))
					btn->action(fx, fy, finger, event);

				finger->selection = 0;
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

		if (btn->isdown)
			sy++;

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
		*height = (TOUCHCUST_SUBMENU_DISPLAYITEMS * (*height))/touchcust_submenu_listsize; // height of scroll bar
		if (sel <= TOUCHCUST_SUBMENU_SCROLLITEMS) // all the way up
		{
			*t = 0; // first item
			*b = TOUCHCUST_SUBMENU_DISPLAYITEMS - 1; // 7th item
			*i = 0; // scrollbar at top position
		}
		else if (sel >= touchcust_submenu_listsize - (TOUCHCUST_SUBMENU_SCROLLITEMS + 1)) // all the way down
		{
			*t = touchcust_submenu_listsize - TOUCHCUST_SUBMENU_DISPLAYITEMS; // # 7th last
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

	m = touchcust_submenu_height;
	GetSubmenuListItems(&t, &i, &b, &m, scrolled);

	*x = basex + touchcust_submenu_width-1 - TOUCHCUST_SUBMENU_SBARWIDTH;
	*y = (basey - 1) + i;
	*w = TOUCHCUST_SUBMENU_SBARWIDTH;
	*h = m;
}

static void DrawSubmenuBox(INT32 x, INT32 y)
{
	V_DrawFill(x, (y - 16) + (16 - 3), touchcust_submenu_width, 1, 0);
	V_DrawFill(x, (y - 16) + (16 - 2), touchcust_submenu_width, 1, 30);
	V_DrawFill(x, y - 1, touchcust_submenu_width, touchcust_submenu_height, 159);
}

static void DrawSubmenuScrollbar(INT32 x, INT32 y)
{
	INT32 sx, sy, sw, sh;
	GetSubmenuScrollbar(x, y, &sx, &sy, &sw, &sh, true);
	V_DrawFill(sx, sy, sw, sh, 0);
}

static void DrawSubmenuList(INT32 x, INT32 y)
{
	size_t i, m;
	size_t t, b;

	DrawSubmenuBox(x, y);

	m = (size_t)touchcust_submenu_height;
	GetSubmenuListItems(&t, &i, &b, &m, true);

	// draw the scroll bar
	DrawSubmenuScrollbar(x, y);

	if (!touchcust_submenu_listsize)
		return;

	// draw list items
	for (i = t; i <= b; i++)
	{
		INT32 left = (x + 11);
		INT32 top = (y + 8);
		UINT32 flags = V_ALLOWLOWERCASE;
		const char *str;

		if (i >= (size_t)touchcust_submenu_listsize)
			break;

		if (y > BASEVIDHEIGHT)
			break;

		if ((INT32)i == touchcust_submenu_selection)
			V_DrawFill(left - 4, top - 4, touchcust_submenu_width - 12 - TOUCHCUST_SUBMENU_SBARWIDTH, 16, 149);

		str = touchcust_submenu_listnames[i];
		if ((INT32)i == touchcust_submenu_highlight)
			str = va("\x82%s", touchcust_submenu_listnames[i]);
		V_DrawString(left, top, flags, str);

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

static void Submenu_GenericList_OnFingerEvent(INT32 fx, INT32 fy, touchfinger_t *finger, event_t *event)
{
	INT32 fingernum = (finger - touchfingers);
	touchcust_submenu_scrollbar_t *scrollbar = &touchcust_submenu_scrollbar[fingernum];
	boolean foundbutton = false;

	INT32 mx = touchcust_submenu_x;
	INT32 my = touchcust_submenu_y;
	INT32 sx, sy, sw, sh;
	size_t i, m = touchcust_submenu_height;
	size_t t, b;

	INT32 dup = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);

	GetSubmenuListItems(&t, &i, &b, &m, true);
	GetSubmenuScrollbar(mx, my, &sx, &sy, &sw, &sh, !IsMovingSubmenuScrollbar());

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

			if (!touchcust_submenu_listsize)
				break;

			for (i = t; i <= b; i++)
			{
				INT32 left = (mx + 7);
				INT32 top = (my + 4);
				INT32 mw = (touchcust_submenu_width - 12 - TOUCHCUST_SUBMENU_SBARWIDTH);
				INT32 mh = 16;

				if (i >= (size_t)touchcust_submenu_listsize)
					break;

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

static void Submenu_Generic_DrawListAndButtons(void)
{
	DrawSubmenuList(touchcust_submenu_x, touchcust_submenu_y);
	DrawSubmenuButtons();
}

static void Submenu_Generic_Drawer(void)
{
	if (curfadevalue)
		V_DrawFadeScreen(0xFF00, curfadevalue);

	Submenu_Generic_DrawListAndButtons();
}

#if 0
static void Submenu_GenericNoFade_Drawer(void)
{
	Submenu_Generic_DrawListAndButtons();
}
#endif

//
// New button submenu
//

static void Submenu_AddNewButton_NewButtonAction(INT32 x, INT32 y, touchfinger_t *finger, event_t *event)
{
	INT32 gc = AddButton(x, y, finger, event);
	touchcust_buttonstatus_t *btnstatus = &touchcustbuttons[gc];

	CloseSubmenu();

	btnstatus->isselecting = false;
	btnstatus->selected = true;
	btnstatus->moving = false;
	btnstatus->finger = finger;
	btnstatus->isresizing = touchcust_resizepoint_none;

	finger->u.gamecontrol = gc;
}

//
// Layout list submenu
//

static void Submenu_LayoutList_Drawer(void)
{
	INT32 x = touchcust_submenu_x;
	INT32 y = touchcust_submenu_y;

	size_t i, m;
	size_t t, b;
	size_t renaming = 0;

	static INT32 blink;
	if (--blink <= 0)
		blink = 8;

	DrawSubmenuButtons();
	DrawSubmenuBox(x, y);

	m = (size_t)touchcust_submenu_height;
	GetSubmenuListItems(&t, &i, &b, &m, true);

	// draw the scroll bar
	DrawSubmenuScrollbar(x, y);

	if (!touchcust_submenu_listsize)
		return;

	if (touchcust_layoutlist_renaming)
		renaming = (touchcust_layoutlist_renaming - touchlayouts);

	// draw list items
	for (i = t; i <= b; i++)
	{
		INT32 left = (x + 11);
		INT32 top = (y + 8);
		UINT32 flags = V_ALLOWLOWERCASE;
		const char *str;

		if (i >= (size_t)touchcust_submenu_listsize)
			break;

		if (y > BASEVIDHEIGHT)
			break;

		if ((INT32)i == touchcust_submenu_selection)
			V_DrawFill(left - 4, top - 4, touchcust_submenu_width - 12 - TOUCHCUST_SUBMENU_SBARWIDTH, 16, 149);

		str = touchcust_submenu_listnames[i];
		if ((INT32)i == touchcust_submenu_highlight)
			str = va("\x82%s", touchcust_submenu_listnames[i]);

		if (touchcust_layoutlist_renaming)
		{
			if (i == renaming)
			{
				char *layoutname = touchcust_layoutlist_renaming->name;
				INT32 nlen = strlen(layoutname) + 1;
				static char lname[MAXTOUCHLAYOUTNAME+1];
				size_t maxlen = 30;

				if (nlen > (INT32)maxlen)
					strlcpy(lname, layoutname + (nlen - maxlen), min(maxlen, MAXTOUCHLAYOUTNAME));
				else
					strlcpy(lname, layoutname, min(maxlen, MAXTOUCHLAYOUTNAME));

				if (blink < 4)
					str = va("%s_", lname);
				else
					str = lname;
			}
			else
				flags |= V_TRANSLUCENT;
		}

		V_DrawString(left, top, flags, str);

		y += 20;
	}
}

//
// Submenu info
//

static void (*touchcust_submenufuncs[num_touchcust_submenus]) (INT32 x, INT32 y, touchfinger_t *finger, event_t *event) =
{
	NULL,
	Submenu_GenericList_OnFingerEvent,
	Submenu_GenericList_OnFingerEvent,
};

static void (*touchcust_submenudrawfuncs[num_touchcust_submenus]) (void) =
{
	NULL,
	Submenu_Generic_Drawer,
	Submenu_LayoutList_Drawer,
};

//
// Handles button resizing
//
static boolean HandleResizePointSelection(INT32 x, INT32 y, touchfinger_t *finger, touchconfig_t *btn, touchcust_buttonstatus_t *btnstatus)
{
	INT32 i;
	boolean resizing = false;
	INT32 px, py, pw, ph;

	btn->supposed.snap = btnstatus->snaptogrid;

	if (btnstatus->isresizing == touchcust_resizepoint_none)
	{
		for (i = 0; i < touchcust_numresizepoints; i++)
		{
			vector2_t *point = &touchcust_resizepoints[i];
			GetButtonResizePoint(btn, point, &px, &py, &pw, &ph);
			if (FingerTouchesRect(x, y, px, py, pw, ph))
			{
				btnstatus->isresizing = i;
				resizing = true;
				break;
			}
		}
	}
	else
	{
		float dx = finger->fdx;
		float dy = -finger->fdy;
		float offx, offy;

		INT32 corner = btnstatus->isresizing;
		boolean chk_w, chk_h;

		SetButtonSupposedLocation(btn);

#define SUPPOSEDWIDTHCHECKLEFT  (UseGridLimits() && (btn->supposed.x + dx < 0.0f))
#define SUPPOSEDHEIGHTCHECKTOP  (UseGridLimits() && (btn->supposed.y + dy < 0.0f))

#define SUPPOSEDWIDTHCHECKRIGHT (UseGridLimits() && (btn->supposed.x + btn->supposed.w + dx > (BASEVIDWIDTH * vid.dupx)))
#define SUPPOSEDHEIGHTCHECKBOT  (UseGridLimits() && (btn->supposed.y + btn->supposed.h + dy > (BASEVIDHEIGHT * vid.dupy)))

		switch (corner)
		{
			// top
			case touchcust_resizepoint_topleft:
				offx = dx;
				offy = dy;

				if (btn->supposed.w - offx < MINBTNWIDTH)
					offx = max(0, offx - MINBTNWIDTH);
				if (btn->supposed.h - offy < MINBTNHEIGHT)
					offy = max(0, offy - MINBTNHEIGHT);

				if (UseGridLimits())
				{
					if ((offx < 0.0f && btn->supposed.x - offx < 0.0f)
					|| (offy < 0.0f && btn->supposed.y - offy < 0.0f))
						break;
				}

				OffsetButtonBy(btn, offx, offy);

				chk_w = (dx > 0.0f) || (dx < 0.0f && !SUPPOSEDWIDTHCHECKLEFT);
				chk_h = (dy > 0.0f) || (dy < 0.0f && !SUPPOSEDHEIGHTCHECKTOP);

				if (chk_w)
					btn->supposed.w -= dx;
				if (chk_h)
					btn->supposed.h -= dy;
				if (chk_w || chk_h)
					CalcButtonFixedSupposedSize(btn);
				break;

			case touchcust_resizepoint_topright:
				offy = dy;
				if (btn->supposed.h - offy < MINBTNHEIGHT)
					offy = max(0, offy - MINBTNHEIGHT);

				if (UseGridLimits())
				{
					if (offy < 0.0f && btn->supposed.y - offy < 0.0f)
						break;
				}

				chk_w = SUPPOSEDWIDTHCHECKRIGHT;

				if ((dy > 0.0f) || (dy < 0.0f && !SUPPOSEDHEIGHTCHECKTOP))
				{
					OffsetButtonBy(btn, 0.0f, offy);
					btn->supposed.h -= dy;
					if (chk_w)
						CalcButtonFixedSupposedSize(btn);
				}

				if (chk_w)
					break;
				btn->supposed.w += dx;
				CalcButtonFixedSupposedSize(btn);
				break;

			// bottom
			case touchcust_resizepoint_bottomleft:
				offx = dx;
				if (btn->supposed.w - offx < MINBTNWIDTH)
					offx = max(0, offx - MINBTNWIDTH);

				if (UseGridLimits())
				{
					if (offx < 0.0f && btn->supposed.x - offx < 0.0f)
						break;
				}

				chk_h = SUPPOSEDWIDTHCHECKRIGHT;

				if ((dx > 0.0f) || (dx < 0.0f && !SUPPOSEDWIDTHCHECKLEFT))
				{
					OffsetButtonBy(btn, offx, 0.0f);
					btn->supposed.w -= dx;
					if (chk_h)
						CalcButtonFixedSupposedSize(btn);
				}

				if (chk_h)
					break;
				btn->supposed.h += dy;
				CalcButtonFixedSupposedSize(btn);
				break;

			case touchcust_resizepoint_bottomright:
				chk_w = SUPPOSEDWIDTHCHECKRIGHT;
				chk_h = SUPPOSEDHEIGHTCHECKBOT;
				if (!chk_w)
					btn->supposed.w += dx;
				if (!chk_h)
					btn->supposed.h += dy;
				if (!(chk_w || chk_h))
					CalcButtonFixedSupposedSize(btn);
				break;

			// middle
			case touchcust_resizepoint_topmiddle:
				offy = dy;
				if (btn->supposed.h - offy < MINBTNHEIGHT)
					offy = max(0, offy - MINBTNHEIGHT);

				if (UseGridLimits())
				{
					if (offy < 0.0f && btn->supposed.y - offy < 0.0f)
						break;
				}

				if ((dy > 0.0f) || (dy < 0.0f && !SUPPOSEDHEIGHTCHECKTOP))
				{
					OffsetButtonBy(btn, 0.0f, offy);
					btn->supposed.h -= dy;
					CalcButtonFixedSupposedSize(btn);
				}
				break;

			case touchcust_resizepoint_bottommiddle:
				if (SUPPOSEDHEIGHTCHECKBOT)
					break;
				btn->supposed.h += dy;
				CalcButtonFixedSupposedSize(btn);
				break;

			// sides
			case touchcust_resizepoint_leftside:
				offx = dx;
				if (btn->supposed.w - offx < MINBTNWIDTH)
					offx = max(0, offx - MINBTNWIDTH);

				if (UseGridLimits())
				{
					if (offx < 0.0f && btn->supposed.x - offx < 0.0f)
						break;
				}

				if ((dx > 0.0f) || (dx < 0.0f && !SUPPOSEDWIDTHCHECKLEFT))
				{
					OffsetButtonBy(btn, offx, 0.0f);
					btn->supposed.w -= dx;
					CalcButtonFixedSupposedSize(btn);
				}
				break;

			case touchcust_resizepoint_rightside:
				if (SUPPOSEDWIDTHCHECKRIGHT)
					break;
				btn->supposed.w += dx;
				CalcButtonFixedSupposedSize(btn);
				break;

#undef SUPPOSEDHEIGHTCHECKTOP
#undef SUPPOSEDHEIGHTCHECKBOT

#undef SUPPOSEDWIDTHCHECKLEFT
#undef SUPPOSEDWIDTHCHECKRIGHT

			default:
				break;
		}

		if (btn == &usertouchcontrols[GC_JOYSTICK])
			UpdateJoystickBase(btn);

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
	{"Joystick / D-Pad",     GC_JOYSTICK},
	{"Jump",                 GC_JUMP},
	{"Spin",                 GC_SPIN},
	{"Look Up",              GC_LOOKUP},
	{"Look Down",            GC_LOOKDOWN},
	{"Center View",          GC_CENTERVIEW},
	{"Toggle Mouselook",     GC_MOUSEAIMING},
	{"Toggle Third-Person",  GC_CAMTOGGLE},
	{"Reset Camera",         GC_CAMRESET},
	{"Game Status",          GC_SCORES},
	{"Pause / Run Retry",    GC_PAUSE},
	{"Screenshot",           GC_SCREENSHOT},
	{"Toggle GIF Recording", GC_RECORDGIF},
	{"Open/Close Menu",      GC_SYSTEMMENU},
	{"Next Viewpoint",       GC_VIEWPOINTNEXT},
	{"Prev Viewpoint",       GC_VIEWPOINTPREV},
	{"Talk",                 GC_TALKKEY},
	{"Talk (Team only)",     GC_TEAMKEY},
	{"Fire",                 GC_FIRE},
	{"Fire Normal",          GC_FIRENORMAL},
	{"Toss Flag",            GC_TOSSFLAG},
	{"Next Weapon",          GC_WEAPONNEXT},
	{"Prev Weapon",          GC_WEAPONPREV},
	{"Custom Action 1",      GC_CUSTOM1},
	{"Custom Action 2",      GC_CUSTOM2},
	{"Custom Action 3",      GC_CUSTOM3},
	{NULL,                   GC_NULL},
};

static boolean SetupNewButtonSubmenu(touchfinger_t *finger)
{
	INT32 i;

	touchcust_submenu_button_t *btn = &touchcust_submenu_buttons[0], *lastbtn;
	touchcust_submenu_numbuttons = 0;

	OpenSubmenu(touchcust_submenu_newbtn);

	// "Add" button
	btn->w = 32;
	btn->h = 24;
	btn->x = ((touchcust_submenu_x + touchcust_submenu_width) - btn->w) - 4;
	btn->y = (touchcust_submenu_y + touchcust_submenu_height) + 4;

	btn->color = 113;
	btn->name = "ADD";
	btn->action = Submenu_AddNewButton_NewButtonAction;

	TOUCHCUST_NEXTSUBMENUBUTTON

	// "Exit" button
	btn->w = 42;
	btn->h = 24;
	btn->x = (lastbtn->x - btn->w) - 4;
	btn->y = lastbtn->y;

	btn->color = 35;
	btn->name = "EXIT";
	btn->action = Submenu_Generic_ExitAction;

	TOUCHCUST_LASTSUBMENUBUTTON

	// Create button list
	touchcust_submenu_selection = 0;
	touchcust_submenu_highlight = -1;
	touchcust_submenu_listsize = 0;

	touchcust_submenu_scroll = 0;
	memset(&touchcust_submenu_scrollbar, 0x00, sizeof(touchcust_submenu_scrollbar_t) * NUMTOUCHFINGERS);

	for (i = 0; (touchcust_buttonlist[i].gc != GC_NULL); i++)
	{
		INT32 gc = touchcust_buttonlist[i].gc;
		touchconfig_t *ubtn = &usertouchcontrols[gc];

		if (touchcust_submenu_listsize >= TOUCHCUST_SUBMENU_MAXLISTSIZE)
			break;

		// Button does not exist, so add it to the list.
		if (ubtn->hidden)
		{
			touchcust_submenu_list[touchcust_submenu_listsize] = gc;
			touchcust_submenu_listnames[touchcust_submenu_listsize] = touchcust_buttonlist[i].name;
			touchcust_submenu_listsize++;
		}
	}

	// Set last finger position
	if (finger)
	{
		touchcust_addbutton_x = finger->x;
		touchcust_addbutton_y = finger->y;
		touchcust_addbutton_finger = true;
	}
	else
		touchcust_addbutton_finger = false;

	// Returns true if any item was added to the list.
	return (touchcust_submenu_listsize > 0);
}

static boolean CheckNavigation(INT32 x, INT32 y)
{
	INT32 i;

	for (i = 0; i < NUMTOUCHNAV; i++)
	{
		touchnavbutton_t *btn = &touchnavigation[i];

		// Ignore buttons that aren't defined
		if (!btn->defined)
			continue;

		// Check if your finger touches this button.
		if (TS_FingerTouchesNavigationButton(x, y, btn))
			return true;
	}

	return false;
}

boolean TS_HandleKeyEvent(INT32 key, event_t *event)
{
	touchlayout_t *layout = NULL;

	if (touchcust_submenu != touchcust_submenu_layouts)
	{
		if (touchcust_submenu == touchcust_submenu_none && key == KEY_ENTER)
		{
			SetupNewButtonSubmenu(NULL);
			return true;
		}
		else
			return false;
	}

	(void)event;

	if (touchcust_layoutlist_renaming)
	{
		size_t l;

		layout = touchcust_layoutlist_renaming;
		l = strlen(layout->name);

		switch (key)
		{
			case KEY_ENTER:
				StopRenamingLayout(layout);
				return true;

			case KEY_ESCAPE:
				StopRenamingLayout(layout);
				return true;

			case KEY_BACKSPACE:
				if (l > 0)
					layout->name[l-1] = '\0';
				return true;

			case KEY_DEL:
				if (l > 0)
					layout->name[0] = '\0';
				return true;

			default:
				if (I_KeyboardOnScreen())
					return true;

				if (key < 32 || key > 127)
					return true;

				if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z'))
				{
					if (shiftdown ^ capslock)
						key = shiftxform[key];
				}
				else if (shiftdown)
					key = shiftxform[key];

				if (l < MAXTOUCHLAYOUTNAME)
				{
					layout->name[l] = (char)(key);
					layout->name[l+1] = '\0';
				}

				return true;
		}
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

	if (touchcust_layoutlist_renaming)
	{
		StopRenamingLayout(touchcust_layoutlist_renaming);
		return true;
	}

	if (touchcust_submenufuncs[touchcust_submenu])
	{
		if (touchcust_submenu == touchcust_submenu_layouts && CheckNavigation(x, y))
			return false;
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

			if (finger->u.gamecontrol != GC_NULL)
			{
				btn = &usertouchcontrols[finger->u.gamecontrol];
				btnstatus = &touchcustbuttons[finger->u.gamecontrol];
				optionsarea = IsFingerTouchingButtonOptions(x, y, finger, btn, btnstatus);
			}

			for (i = (NUM_GAMECONTROLS - 1); i >= 0; i--)
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

				// Move selected button
				if (btnstatus->selected && touchmotion && (i == finger->u.gamecontrol))
				{
					boolean resized = false;

					if (btnstatus->resizearea && (!btnstatus->moving))
					{
						HandleResizePointSelection(x, y, finger, btn, btnstatus);
						foundbutton = true;
						userlayoutsaved = userlayoutnew = false;
						break;
					}

					if (!btnstatus->moving)
					{
						resized = HandleResizePointSelection(x, y, finger, btn, btnstatus);
						if (resized)
							btnstatus->resizearea = true;
						else if (!FingerTouchesButton(x, y, btn, true))
							break;
					}

					if (!btnstatus->resizearea)
					{
						OffsetButtonBy(btn, finger->fdx, -finger->fdy);
						btnstatus->moving = true;
						userlayoutsaved = userlayoutnew = false;
					}

					btnstatus->isselecting = false;
					foundbutton = true;
					break;
				}
				// Select options
				else if (HandleButtonOptions(x, y, finger, btn, btnstatus, false) && (i == finger->u.gamecontrol))
				{
					finger->u.gamecontrol = i;
					foundbutton = true;
					userlayoutsaved = userlayoutnew = false;
					break;
				}
				// Check if your finger touches this button.
				else if (FingerTouchesButton(x, y, btn, true) && (!optionsarea))
				{
					// Let go of other fingers
					ClearAllSelections();

					finger->u.gamecontrol = i;

					btnstatus->isselecting = true;
					btnstatus->selected = true;
					btnstatus->moving = false;
					btnstatus->finger = finger;
					btnstatus->isresizing = touchcust_resizepoint_none;

					foundbutton = true;
					break;
				}
			}

			if (!foundbutton && (!touchmotion))
			{
				if (CheckNavigation(x, y))
					return false;
				return true;
			}

			if (touchmotion || foundbutton)
				return true;

			break;
		case ev_touchup:
			// Let go of this finger.
			gc = finger->u.gamecontrol;

			if (gc > GC_NULL)
			{
				btn = &usertouchcontrols[gc];
				btnstatus = &touchcustbuttons[gc];

				// Select options
				if (HandleButtonOptions(x, y, finger, btn, btnstatus, true))
				{
					btnstatus->optsel = touchcust_options_none;
					break;
				}

				// Deselect button
				if (!btnstatus->moving && !btnstatus->isselecting && (btnstatus->isresizing < 0))
				{
					btnstatus->finger = NULL;
					ClearSelection(btnstatus);
					finger->u.gamecontrol = GC_NULL;
				}
				// Stop moving button
				else
				{
					// Snap to the grid
					if (btnstatus->moving || btnstatus->isresizing >= 0)
					{
						MoveButtonToSupposedLocation(btn);
						if (btnstatus->snaptogrid)
							SnapButtonToGrid(btn);
					}

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

void TS_UpdateCustomization(void)
{
	if (touchcust_deferredmessage)
	{
		M_StartMessage(touchcust_deferredmessage, NULL, MM_NOTHING);
		Z_Free(touchcust_deferredmessage);
		touchcust_deferredmessage = NULL;
	}
}

static void DrawGrid(void)
{
	INT32 i;
	INT32 col = 0;
	INT32 alpha = 8;
	INT32 scol = 15;
	INT32 salpha = 2;

	boolean widescreen = (usertouchlayout->widescreen);
	INT32 w = (widescreen ? vid.width : BASEVIDWIDTH);
	INT32 h = (widescreen ? vid.height : BASEVIDHEIGHT);
	INT32 flags = (widescreen ? (V_SNAPTOTOP|V_SNAPTOLEFT) : 0);

	for (i = 0; i < w; i += TOUCHSMALLGRIDSIZE)
		V_DrawFadeFill(i-1, 0, 1, h, flags, scol, salpha);
	for (i = 0; i < h; i += TOUCHSMALLGRIDSIZE)
		V_DrawFadeFill(-1, i-1, w, 1, flags, scol, salpha);

	for (i = 0; i < w; i += TOUCHGRIDSIZE)
		V_DrawFadeFill(i-1, 0, 1, h, flags, col, alpha);
	for (i = 0; i < h; i += TOUCHGRIDSIZE)
		V_DrawFadeFill(-1, i-1, w, 1, flags, col, alpha);
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
	INT32 alpha = 8;

	if (!touchcust_customizing)
	{
		if (touchcust_submenudrawfuncs[touchcust_submenu])
			(touchcust_submenudrawfuncs[touchcust_submenu])();
		return;
	}

	DrawGrid();
	TS_DrawControls(usertouchcontrols, true, 10);

	for (i = 0; i < NUM_GAMECONTROLS; i++)
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
		V_DrawFill(tx, ty + th, (tw + vid.dupx), vid.dupy, col|V_NOSCALESTART);

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

				V_DrawFadeFill(px + vid.dupx, py + vid.dupy, pw - vid.dupx, ph - vid.dupy, V_NOSCALESTART, orange, alpha);
				V_DrawFadeFill(px, py, pw, vid.dupy, V_NOSCALESTART, (orange + 3), alpha);
				V_DrawFadeFill(px, py + ph, (pw + vid.dupx), vid.dupy, V_NOSCALESTART, (orange + 3), alpha);
				V_DrawFadeFill(px, py, vid.dupx, ph, V_NOSCALESTART, (orange + 3), alpha);
				V_DrawFadeFill(px + pw, py, vid.dupx, ph, V_NOSCALESTART, (orange + 3), alpha);
			}
		}
	}

	if (touchcust_submenudrawfuncs[touchcust_submenu])
		(touchcust_submenudrawfuncs[touchcust_submenu])();

	flash++;
}

#endif // TOUCHINPUTS
