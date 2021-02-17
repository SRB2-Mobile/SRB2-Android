// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ts_draw.h
/// \brief Touch screen drawing

#ifndef __TS_DRAW_H__
#define __TS_DRAW_H__

#include "ts_main.h"

#define TS_BUTTONUPCOLOR 16
#define TS_BUTTONDOWNCOLOR 24

#ifdef TOUCHINPUTS
void TS_DrawControls(touchconfig_t *config, boolean drawgamecontrols, INT32 alphalevel);
void TS_DrawNavigation(void);
#endif

#endif // __TS_DRAW_H__
