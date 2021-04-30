// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime Ita Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ndk_crash_handler.c
/// \brief Android crash handler

#include "jni_android.h"
#include "ndk_crash_handler.h"

#include <stdio.h>
#include <unwind.h>
#include <dlfcn.h>

static FILE *errorlog = NULL;

typedef struct BacktraceState
{
	void **current;
	void **end;
} BacktraceState_t;

static _Unwind_Reason_Code NDKCrashHandler_Unwind(struct _Unwind_Context* context, void* arg)
{
	BacktraceState_t *state = (BacktraceState_t *)(arg);
	uintptr_t pc = _Unwind_GetIP(context);
	if (pc)
	{
		if (state->current == state->end)
			return _URC_END_OF_STACK;
		else
			*state->current++ = (void *)(pc);
	}
	return _URC_NO_REASON;
}

size_t NDKCrashHandler_CaptureBacktrace(void **buffer, size_t max)
{
	BacktraceState_t state = {buffer, buffer + max};
	_Unwind_Backtrace(NDKCrashHandler_Unwind, &state);
	return state.current - buffer;
}

static void NDKCrashHandler_PrintToLog(const char *fmt, ...)
{
	char txt[8192] = "";

	va_list argptr;
	va_start(argptr, fmt);
	Android_vsnprintf(txt, 8192, fmt, argptr);
	va_end(argptr);

	if (errorlog)
		fwrite(txt, strlen(txt), 1, errorlog);

	CON_LogMessage(txt);
}

static void NDKCrashHandler_StackTrace(void)
{
	const int max = 4096;
	void *buffer[max];
	size_t count = NDKCrashHandler_CaptureBacktrace(buffer, max);
	int i;

	if (!count)
		return;

	NDKCrashHandler_PrintToLog("Stack trace:\n");
	for (i = 0; i < count; i++)
	{
		Dl_info info;
		const void *addr = buffer[i];
		const char *symbol = "(mangled)";

		if (dladdr(addr, &info) && info.dli_sname)
			symbol = info.dli_sname;

		NDKCrashHandler_PrintToLog("%d: %p %s\n", i, addr, symbol);
	}
}

void NDKCrashHandler_ReportSignal(const char *sigmsg)
{
	INT32 i;

	// open errorlog.txt
	errorlog = fopen(va("%s/errorlog.txt", I_SharedStorageLocation()), "wt+");
	NDKCrashHandler_PrintToLog("Application killed by signal: %s\n\n", sigmsg);

	NDKCrashHandler_PrintToLog("Device info:\n", sigmsg);
	for (i = 0; JNI_DeviceInfoReference[i].info; i++)
	{
		JNI_DeviceInfoReference_t *ref = &JNI_DeviceInfoReference[i];
		JNI_DeviceInfo_t info_e = ref->info_enum;
		NDKCrashHandler_PrintToLog("   %s: %s\n", ref->display_info, JNI_DeviceInfo[info_e]);
	}

	if (JNI_ABICount)
	{
		NDKCrashHandler_PrintToLog("Supported ABIs: ");
		for (i = 0; i < JNI_ABICount; i++)
		{
			NDKCrashHandler_PrintToLog("%s", JNI_ABIList[i]);
			if (i == JNI_ABICount-1)
				NDKCrashHandler_PrintToLog("\n");
			else
				NDKCrashHandler_PrintToLog(", ");
		}
	}

	NDKCrashHandler_PrintToLog("\n");
	NDKCrashHandler_StackTrace();

	if (errorlog)
		fclose(errorlog);
}
