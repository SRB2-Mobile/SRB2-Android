// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
// Copyright (C) 1997-2020 by Sam "Slouken" Lantinga.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  jni_android.h
/// \brief Android JNI functions

#ifndef __JNI_ANDROID_H__
#define __JNI_ANDROID_H__

#include <jni.h>

#include "../../../../src/doomdata.h"
#include "../../../../src/doomtype.h"
#include "../../../../src/doomdef.h"

#include "../../../../src/i_system.h"
#include "../../../../src/console.h"

void JNI_Startup(void);
void JNI_SetupActivity(void);

const char *JNI_ExternalStoragePath(void);

#endif
