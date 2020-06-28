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
#include "i_system.h" // I_mkdir

#include "f_finale.h" // curfadevalue
#include "v_video.h"
#include "st_stuff.h" // ST_drawTouchGameInput
#include "hu_stuff.h" // shiftxform
#include "s_sound.h" // S_StartSound
#include "z_zone.h"

#include "console.h" // CON_Ready()

#ifdef TOUCHINPUTS
static touchcust_buttonstatus_t touchcustbuttons[num_gamecontrols];

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

static void FocusSubmenuOnSelection(INT32 selection);
static boolean HandleSubmenuButtons(INT32 fx, INT32 fy, touchfinger_t *finger, event_t *event);

static void GetSubmenuListItems(size_t *t, size_t *i, size_t *b, size_t *height, boolean scrolled);
static void GetSubmenuScrollbar(INT32 basex, INT32 basey, INT32 *x, INT32 *y, INT32 *w, INT32 *h, boolean scrolled);

static boolean IsMovingSubmenuScrollbar(void);

static boolean HandleResizePointSelection(INT32 x, INT32 y, touchfinger_t *finger, touchconfig_t *btn, touchcust_buttonstatus_t *btnstatus);

static boolean HandleButtonOptions(INT32 x, INT32 y, touchfinger_t *finger, touchconfig_t *btn, touchcust_buttonstatus_t *btnstatus, boolean execute);
static boolean IsFingerTouchingButtonOptions(INT32 x, INT32 y, touchfinger_t *finger, touchconfig_t *btn, touchcust_buttonstatus_t *btnstatus);

// =====================================================

static boolean ts_ready = false;

boolean TS_Ready(void)
{
	return ts_ready;
}

boolean TS_IsCustomizingControls(void)
{
	return M_IsCustomizingTouchControls();
}

void TS_SetupCustomization(void)
{
	INT32 i;

	touch_screenexists = true;
	touchcust_customizing = true;

	ClearAllSelections();
	CloseSubmenu();

	for (i = 0; i < num_gamecontrols; i++)
	{
		touchcust_buttonstatus_t *btnstatus = &touchcustbuttons[i];
		btnstatus->snaptogrid = true;
	}
}

boolean TS_ExitCustomization(void)
{
	size_t layoutsize = (sizeof(touchconfig_t) * num_gamecontrols);

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

// =======
// Layouts
// =======

touchlayout_t *touchlayouts = NULL;
INT32 numtouchlayouts = 0;

touchlayout_t *usertouchlayout = NULL;
INT32 usertouchlayoutnum = -1;
boolean userlayoutsaved = true;

char touchlayoutfolder[512] = "touchlayouts";

#define LAYOUTLISTSAVEFORMAT "%s: %s\n"

void TS_InitLayouts(void)
{
	I_mkdir(touchlayoutfolder, 0755);

	if (usertouchlayout == NULL)
		usertouchlayout = Z_Calloc(sizeof(touchlayout_t), PU_STATIC, NULL);
	TS_DefaultLayout();

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

void TS_SaveLayouts(void)
{
	FILE *f = fopen(va("%s"PATHSEP"%s", touchlayoutfolder, TOUCHLAYOUTSFILE), "w");
	touchlayout_t *layout = touchlayouts;
	INT32 i;

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
}

void TS_NewLayout(void)
{
	numtouchlayouts++;
	touchlayouts = Z_Realloc(touchlayouts, (numtouchlayouts * sizeof(touchlayout_t)), PU_STATIC, NULL);
	memset(touchlayouts + (numtouchlayouts - 1), 0x00, sizeof(touchlayout_t));
}

void TS_ClearLayout(void)
{
	INT32 i;
	// reset hidden value so that all buttons are properly populated
	for (i = 0; i < num_gamecontrols; i++)
		usertouchcontrols[i].hidden = false;
	// AAAANNDD this is where I would put my
	// function that clears touchlayout_t...
	// IF I HAD ONE!!!!
	TS_BuildLayoutFromPreset(usertouchcontrols);
	M_Memcpy(&touchcontrols, usertouchcontrols, sizeof(touchconfig_t) * num_gamecontrols);
	userlayoutsaved = false;
}

touchconfigstatus_t usertouchconfigstatus;

void TS_BuildLayoutFromPreset(touchconfig_t *config)
{
	G_BuildTouchPreset(config, &usertouchconfigstatus, tms_joystick, TS_GetDefaultScale(), false);
}

void TS_DefaultLayout(void)
{
	usertouchconfigstatus.ringslinger = true;
	usertouchconfigstatus.ctfgametype = true;
	usertouchconfigstatus.canpause = true;
	usertouchconfigstatus.canviewpointswitch = true;
	usertouchconfigstatus.cantalk = true;
	usertouchconfigstatus.canteamtalk = true;

	if (usertouchcontrols == NULL)
	{
		usertouchcontrols = Z_Calloc(sizeof(touchconfig_t) * num_gamecontrols, PU_STATIC, NULL);
		TS_BuildLayoutFromPreset(usertouchcontrols);
		usertouchlayout->config = usertouchcontrols;
	}
}

void TS_DeleteLayout(INT32 layoutnum)
{
	touchlayout_t *todelete = (touchlayouts + layoutnum);
	usertouchlayout = Z_Calloc(sizeof(touchlayout_t), PU_STATIC, NULL);
	TS_CopyLayoutTo(usertouchlayout, todelete);

	if (layoutnum < numtouchlayouts-1)
		memmove(todelete, (todelete + 1), (numtouchlayouts - (layoutnum + 1)) * sizeof(touchlayout_t));

	numtouchlayouts--;
	touchlayouts = Z_Realloc(touchlayouts, (numtouchlayouts * sizeof(touchlayout_t)), PU_STATIC, NULL);

	if (touchcust_submenu_selection >= numtouchlayouts)
		FocusSubmenuOnSelection(numtouchlayouts - 1);
}

void TS_CopyConfigTo(touchlayout_t *to, touchconfig_t *from)
{
	size_t layoutsize = sizeof(touchconfig_t) * num_gamecontrols;

	if (!to->config)
		to->config = Z_Malloc(layoutsize, PU_STATIC, NULL);

	M_Memcpy(to->config, from, layoutsize);
}

void TS_CopyLayoutTo(touchlayout_t *to, touchlayout_t *from)
{
	size_t layoutsize = sizeof(touchconfig_t) * num_gamecontrols;

	if (!to->config)
		to->config = Z_Malloc(layoutsize, PU_STATIC, NULL);

	M_Memcpy(to->config, from->config, layoutsize);
}

#define BUTTONLOADFORMAT "%64s %f %f %f %f"
#define BUTTONSAVEFORMAT "%s %f %f %f %f" "\n"

boolean TS_LoadSingleLayout(INT32 ilayout)
{
	touchlayout_t *layout = touchlayouts + ilayout;

	FILE *f;
	char filename[MAXTOUCHLAYOUTFILENAME+5];
	size_t layoutsize = (sizeof(touchconfig_t) * num_gamecontrols);

	float x, y, w, h;
	char gc[65];
	INT32 igc;
	touchconfig_t *button = NULL;

	if (layout->loaded)
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
			"\n" PRESS_A_KEY_MESSAGE),
			TS_GetShortLayoutName(layout, MAXMESSAGELAYOUTNAME)));
		return false;
	}

	if (layout->config == NULL)
		layout->config = Z_Calloc(layoutsize, PU_STATIC, NULL);

	memset(layout->config, 0x00, layoutsize);
	for (igc = 0; igc < num_gamecontrols; igc++)
	{
		button = &(layout->config[igc]);
		button->hidden = true;
	}

	while (fscanf(f, BUTTONLOADFORMAT, gc, &x, &y, &w, &h) == 5)
	{
		for (igc = 0; igc < num_gamecontrols; igc++)
		{
			if (!strcmp(gc, gamecontrolname[igc]))
				break;
		}

		if (igc == gc_null || (igc >= gc_wepslot1 && igc <= gc_wepslot10) || igc == num_gamecontrols)
			continue;

		button = &(layout->config[igc]);
		button->x = FloatToFixed(x);
		button->y = FloatToFixed(y);
		button->w = FloatToFixed(w);
		button->h = FloatToFixed(h);
		button->hidden = false;
	}

	G_SetTouchButtonNames(layout->config);
	layout->loaded = true;

	fclose(f);
	return true;
}

boolean TS_SaveSingleLayout(INT32 ilayout)
{
	FILE *f;
	char filename[MAXTOUCHLAYOUTFILENAME+5];

	touchlayout_t *layout = touchlayouts + ilayout;
	INT32 gc;

	strcpy(filename, layout->filename);
	FIL_ForceExtension(filename, ".cfg");

	f = fopen(va("%s"PATHSEP"%s", touchlayoutfolder, filename), "w");
	if (!f)
	{
		S_StartSound(NULL, sfx_lose);
		DisplayMessage(va(M_GetText(
			"\x82%s\n"
			"\x85""Failed to save layout!\n"
			"\n\x80" PRESS_A_KEY_MESSAGE),
			layout->name));
		return false;
	}

	for (gc = (gc_null+1); gc < num_gamecontrols; gc++)
	{
		touchconfig_t *button = &(layout->config[gc]);
		const char *line;

		if (button->hidden)
			continue;

		line = va(BUTTONSAVEFORMAT,
				gamecontrolname[gc],
				FixedToFloat(button->x), FixedToFloat(button->y),
				FixedToFloat(button->w), FixedToFloat(button->h));
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
			usertouchlayoutnum = -1;
			userlayoutsaved = true;
			ResetLayoutMenus();
		}
		return;
	}

	if (touchcust_submenu == touchcust_submenu_layouts)
		StopRenamingLayout(touchcust_layoutlist_renaming);

	if (!LoadLayoutFromName(layoutname))
		return;

	userlayoutsaved = true;
	ResetLayoutMenus();
}

void TS_LoadUserLayouts(void)
{
	const char *layoutname = cv_touchlayout.string;

	TS_LoadLayouts();

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
		size_t layoutsize = sizeof(touchconfig_t) * num_gamecontrols;

		newlayout->config = Z_Calloc(layoutsize, PU_STATIC, NULL);
		TS_BuildLayoutFromPreset(usertouchlayout->config);

		M_Memcpy(usertouchcontrols, newlayout->config, layoutsize);
	}
	else
		TS_CopyLayoutTo(newlayout, usertouchlayout);

	usertouchlayout = newlayout;
	usertouchlayoutnum = newlayoutnum;
	userlayoutsaved = true;

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

	M_StartMessage(M_GetText(
		"Create a new layout?\n"
		"\n("CONFIRM_MESSAGE")\n"),
	SubmenuMessageResponse_LayoutList_New, MM_YESNO);
}

static boolean LoadLayoutAtIndex(INT32 idx)
{
	size_t layoutsize = (sizeof(touchconfig_t) * num_gamecontrols);

	if (!TS_LoadSingleLayout(idx))
		return false;

	usertouchlayout = (touchlayouts + idx);
	usertouchlayoutnum = idx;

	M_Memcpy(usertouchcontrols, usertouchlayout->config, layoutsize);
	M_Memcpy(&touchcontrols, usertouchcontrols, layoutsize);

	G_PositionExtraUserTouchButtons();

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
				"\n" PRESS_A_KEY_MESSAGE),
				TS_GetShortLayoutName((touchlayouts + usertouchlayoutnum), MAXMESSAGELAYOUTNAME)));
			touchcust_submenu_highlight = usertouchlayoutnum;
		}
	}
}

static void Submenu_LayoutList_Load(INT32 x, INT32 y, touchfinger_t *finger, event_t *event)
{
	const char *layoutname = TS_GetShortLayoutName((touchlayouts + touchcust_submenu_selection), MAXMESSAGELAYOUTNAME);

	(void)x;
	(void)y;
	(void)finger;
	(void)event;

	if (userlayoutsaved && (usertouchlayoutnum == touchcust_submenu_selection))
	{
		S_StartSound(NULL, sfx_skid);
		DisplayMessage(va(M_GetText(
			"\x82%s\n"
			"This layout is already loaded!\n"
			"\n" PRESS_A_KEY_MESSAGE),
		layoutname));
		return;
	}

	if (!userlayoutsaved)
	{
		M_StartMessage(va(M_GetText(
			"\x82%s\n"
			"Load this layout?\n"
			"You will lose your unsaved changes."
			"\n\n("CONFIRM_MESSAGE")\n"),
		layoutname), SubmenuMessageResponse_LayoutList_Load, MM_YESNO);
	}
	else
	{
		M_StartMessage(va(M_GetText(
			"\x82%s\n"
			"Load this layout?\n"
			"\n("CONFIRM_MESSAGE")\n"),
		layoutname), SubmenuMessageResponse_LayoutList_Load, MM_YESNO);
	}
}

static void SaveLayoutOnList(void)
{
	touchlayout_t *savelayout = NULL;

#if 0
	boolean savelayout = true;

	if (usertouchlayoutnum == -1)
	{
		savelayout = false;
		CreateAndSetupNewLayout(false);
	}
	else
#endif
	if (touchcust_submenu_selection != usertouchlayoutnum)
	{
		savelayout = touchlayouts + touchcust_submenu_selection;

		TS_CopyConfigTo(usertouchlayout, usertouchcontrols);
		TS_CopyLayoutTo(savelayout, usertouchlayout);

		usertouchlayout->saved = true;
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

	userlayoutsaved = true;

#if 0
	if (savelayout)
#endif
	{
		savelayout->saved = true;
		TS_SaveSingleLayout(usertouchlayoutnum);
		TS_SaveLayouts();
	}

	S_StartSound(NULL, sfx_strpst);
	DisplayMessage(va(M_GetText(
		"\x82%s\n"
		"\x83Layout saved!\n"
		"\n" PRESS_A_KEY_MESSAGE),
	TS_GetShortLayoutName((touchlayouts + usertouchlayoutnum), MAXMESSAGELAYOUTNAME)));

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

	if (usertouchlayoutnum == -1)
	{
		SaveLayoutOnList();
		return;
	}

	if (touchcust_submenu_selection != usertouchlayoutnum)
	{
		M_StartMessage(va(M_GetText(
			"\x82%s\n"
			"\x80Save over the \x82%s \x80layout?\n"
			"\n("PRESS_Y_MESSAGE" to save"
			"\nor "PRESS_N_MESSAGE_L" to return)\n"),
			TS_GetShortLayoutName((touchlayouts + usertouchlayoutnum), MAXMESSAGELAYOUTNAME),
			TS_GetShortLayoutName((touchlayouts + touchcust_submenu_selection), MAXMESSAGELAYOUTNAME)),
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
			userlayoutsaved = false;

		TS_SaveLayouts();

		DisplayMessage(va(M_GetText(
			"\x82%s\n"
			"\x85""Layout deleted.\n"
			"\n" PRESS_A_KEY_MESSAGE),
			TS_GetShortLayoutName(layout, MAXMESSAGELAYOUTNAME)));
	}
	else
	{
		TS_DeleteLayout(layoutnum);
		DisplayMessage(M_GetText("Layout deleted.\n" PRESS_A_KEY_MESSAGE));
	}

	TS_MakeLayoutList();
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

	layout = (touchlayouts + touchcust_submenu_selection);

	M_StartMessage(va(M_GetText(
		"\x82%s\n"
		"\x80""%s this layout %s?\n"
		"You'll still be able to edit it.\n"
		"\n("PRESS_Y_MESSAGE" to delete)\n"),
		TS_GetShortLayoutName(layout, MAXMESSAGELAYOUTNAME),
		(layout->saved ? "Delete" : "Remove"),
		(layout->saved ? "from your device" : "from the list")),
	SubmenuMessageResponse_LayoutList_Delete, MM_YESNO);
}

static void Submenu_LayoutList_Rename(INT32 x, INT32 y, touchfinger_t *finger, event_t *event)
{
	touchlayout_t *layout = (touchlayouts + touchcust_submenu_selection);

	(void)x;
	(void)y;
	(void)finger;
	(void)event;

	touchcust_layoutlist_renaming = layout;
#if defined(__ANDROID__)
	I_RaiseScreenKeyboard(layout->name, MAXTOUCHLAYOUTNAME);
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

	touch_screenexists = true;
	touchcust_customizing = false;

	if (usertouchlayoutnum >= 0)
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
				size_t len, offs;

				append++;
				attach = va("...%s", append);
				len = strlen(attach) + 1;

				strlcpy(layoutnames[i], string, maxlen+1);

				offs = (maxlen + 1) - len;
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
	INT32 tx = btn->x, ty = btn->y, tw = btn->w, th = btn->h;

	G_ScaleTouchCoords(&tx, &ty, &tw, &th, true, (!btn->dontscale));
	G_CenterIntegerCoords(&tx, &ty);

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
	fixed_t w = (BASEVIDWIDTH * FRACUNIT);
	fixed_t h = (BASEVIDHEIGHT * FRACUNIT);

	G_DenormalizeCoords(&btn->x, &btn->y);

	btn->x += offsx;
	btn->y += offsy;

	if (btn->x < 0)
		btn->x = 0;
	if (btn->x + btn->w > w)
		btn->x = (w - btn->w);

	if (btn->y < 0)
		btn->y = 0;
	if (btn->y + btn->h > h)
		btn->y = (h - btn->h);

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

	G_DenormalizeCoords(&btn->x, &btn->y);

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
		G_DenormalizeCoords(&btn->x, &btn->y);
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
					finger->extra.selection = (i+1);
					return true;
				}
			}

			break;

		case ev_touchup:
			if (finger->extra.selection)
			{
				touchcust_submenu_button_t *btn = &touchcust_submenu_buttons[finger->extra.selection-1];

				bx = btn->x * vid.dupx;
				by = btn->y * vid.dupy;
				bw = btn->w * vid.dupx;
				bh = btn->h * vid.dupy;

				bx += (vid.width - (BASEVIDWIDTH * dup)) / 2;
				by += (vid.height - (BASEVIDHEIGHT * dup)) / 2;

				btn->isdown = NULL;

				if (FingerTouchesRect(fx, fy, bx, by, bw, bh))
					btn->action(fx, fy, finger, event);

				finger->extra.selection = 0;
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

	m = touchcust_submenu_height;
	GetSubmenuListItems(&t, &i, &b, &m, scrolled);

	// draw the scroll bar
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
	finger->x = x;
	finger->y = y;
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
			// Denormalize button
			G_DenormalizeCoords(&btn->x, &btn->y);

			// Update joystick
			UpdateJoystickBase(btn);

			// Normalize button
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
			CloseSubmenu();
		return true;
	}

	return false;
}

static boolean CheckNavigation(INT32 x, INT32 y)
{
	INT32 i;

	for (i = 0; i < NUMKEYS; i++)
	{
		touchconfig_t *btn = &touchnavigation[i];

		// Ignore hidden buttons
		if (btn->hidden)
			continue;

		// Check if your finger touches this button.
		if (G_FingerTouchesButton(x, y, btn))
			return true;
	}

	return false;
}

boolean TS_HandleKeyEvent(INT32 key, event_t *event)
{
	touchlayout_t *layout = NULL;

	if (touchcust_submenu != touchcust_submenu_layouts)
		return false;

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
					userlayoutsaved = false;
					break;
				}
				// Move selected button
				else if (btnstatus->selected && touchmotion && (i == finger->u.gamecontrol))
				{
					if (btnstatus->resizearea && (!btnstatus->moving))
					{
						HandleResizePointSelection(x, y, finger, btn, btnstatus);
						foundbutton = true;
						userlayoutsaved = false;
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
						userlayoutsaved = false;
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
				if (CheckNavigation(x, y))
					return false;

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

void TS_UpdateCustomization(void)
{
#if 0
	if (touchcust_layoutlist_renaming && !I_KeyboardOnScreen())
		StopRenamingLayout(touchcust_layoutlist_renaming);
#endif

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

	if (!touchcust_customizing)
	{
		if (touchcust_submenudrawfuncs[touchcust_submenu])
			(touchcust_submenudrawfuncs[touchcust_submenu])();
		return;
	}

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
