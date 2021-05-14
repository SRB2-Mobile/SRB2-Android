// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime Ita Passos.
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
void JNI_SetupDeviceInfo(void);
void JNI_SetupABIList(void);

typedef enum JNI_DeviceInfo_e
{
	JNIDeviceInfo_Brand,
	JNIDeviceInfo_Device,
	JNIDeviceInfo_Manufacturer,
	JNIDeviceInfo_Model,

	JNIDeviceInfo_Size
} JNI_DeviceInfo_t;

typedef struct JNI_DeviceInfoReference_s
{
	const char *info;
	const char *display_info;
	JNI_DeviceInfo_t info_enum;
} JNI_DeviceInfoReference_t;

extern char *JNI_DeviceInfo[JNIDeviceInfo_Size];
extern JNI_DeviceInfoReference_t JNI_DeviceInfoReference[JNIDeviceInfo_Size + 1];

extern char **JNI_ABIList;
extern int JNI_ABICount;

extern const char *JNI_SharedStorage;
extern boolean JNI_StoragePermission;

char *JNI_GetStorageDirectory(void);
char *JNI_RemovableStoragePath(void);

boolean JNI_CheckPermission(const char *permission);
boolean JNI_CheckStoragePermission(void);
boolean JNI_StoragePermissionGranted(void);

const char *JNI_GetWriteExternalStoragePermission(void);

char *JNI_GetDeviceInfo(const char *info);
void JNI_DisplayToast(const char *text);

boolean JNI_IsInMultiWindowMode(void);

#endif
