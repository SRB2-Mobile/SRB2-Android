// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ts_draw.h
/// \brief Touch screen rendering

#ifndef __TS_DRAW_H__
#define __TS_DRAW_H__

#include "ts_main.h"

#define TS_BUTTONUPCOLOR 16
#define TS_BUTTONDOWNCOLOR 24

#ifdef TOUCHINPUTS
void TS_DrawControls(touchconfig_t *config, boolean drawgamecontrols, INT32 alphalevel);
void TS_DrawMenuNavigation(void);

void TS_DrawDPad(fixed_t dpadx, fixed_t dpady, fixed_t dpadw, fixed_t dpadh, INT32 accent, INT32 flags, touchconfig_t *config, boolean backing);
void TS_DrawJoystickBacking(fixed_t padx, fixed_t pady, fixed_t padw, fixed_t padh, fixed_t scale, UINT8 color, INT32 flags);
void TS_DrawJoystick(fixed_t dpadx, fixed_t dpady, fixed_t dpadw, fixed_t dpadh, UINT8 color, INT32 flags);
#endif // TOUCHINPUTS

#endif // __TS_DRAW_H__
