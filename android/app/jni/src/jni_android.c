// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2022 by Jaime Ita Passos.
// Copyright (C) 1997-2022 by Sam "Slouken" Lantinga.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  jni_android.c
/// \brief Android JNI functions

#include "jni_android.h"
#include "SDL.h"

static JavaVM *jvm = NULL;
static JNIEnv *jniEnv = NULL;

static jclass activityClass;

char *JNI_DeviceInfo[JNIDeviceInfo_Size];

JNI_DeviceInfoReference_t JNI_DeviceInfoReference[JNIDeviceInfo_Size + 1] =
{
	{"BRAND", "Brand", JNIDeviceInfo_Brand},
	{"DEVICE", "Device", JNIDeviceInfo_Device},
	{"MANUFACTURER", "Manufacturer", JNIDeviceInfo_Manufacturer},
	{"MODEL", "Model", JNIDeviceInfo_Model},
	{NULL, NULL, 0},
};

char **JNI_ABIList = NULL;
int JNI_ABICount = 0;

void JNI_Startup(void)
{
#ifdef HAVE_THREADS
	I_start_threads();
	I_AddExitFunc(I_stop_threads);
#endif
	CONS_Printf("%s()...\n", __FUNCTION__);
	JNI_SetupActivity();
	JNI_SetupDeviceInfo();
}

void JNI_SetupActivity(void)
{
	jobject activityObject;

	jniEnv = (JNIEnv *)SDL_AndroidGetJNIEnv();
	(*jniEnv)->GetJavaVM(jniEnv, &jvm);

	activityObject = (jobject)SDL_AndroidGetActivity();
	activityClass = (*jniEnv)->GetObjectClass(jniEnv, activityObject);
}

void JNI_SetupDeviceInfo(void)
{
	INT32 i;

	CONS_Printf("Device info:\n");

	for (i = 0; JNI_DeviceInfoReference[i].info; i++)
	{
		JNI_DeviceInfoReference_t *ref = &JNI_DeviceInfoReference[i];
		JNI_DeviceInfo_t info_e = ref->info_enum;
		JNI_DeviceInfo[info_e] = JNI_GetDeviceInfo(ref->info);
		CONS_Printf("%s: %s\n", ref->display_info, JNI_DeviceInfo[info_e]);
	}

	JNI_SetupABIList();

	if (JNI_ABICount)
	{
		CONS_Printf("Supported ABIs: ");
		for (i = 0; i < JNI_ABICount; i++)
		{
			CONS_Printf("%s", JNI_ABIList[i]);
			if (i == JNI_ABICount-1)
				CONS_Printf("\n");
			else
				CONS_Printf(", ");
		}
	}
}

static JNIEnv *JNI_GetEnv(void)
{
	JNIEnv *env;
	int status = (*jvm)->AttachCurrentThread(jvm, &env, NULL);
	if (status < 0)
		return jniEnv;
	return env;
}

#define JNI_ENV(returnarg) \
{ \
	env = JNI_GetEnv(); \
	if (!LocalReferenceHolder_Init(&refs, env)) \
	{ \
		LocalReferenceHolder_Cleanup(&refs); \
		return returnarg; \
	} \
}

#define JNI_CONTEXT \
{ \
	method = (*env)->GetStaticMethodID(env, activityClass, \
			"getContext", "()Landroid/content/Context;"); \
	context = (*env)->CallStaticObjectMethod(env, activityClass, method); \
}

// Lactozilla: Portions from SDL2
// https://github.com/libsdl-org/SDL/blob/main/src/core/android/SDL_android.c

static int s_active = 0;
struct LocalReferenceHolder
{
	JNIEnv *m_env;
	const char *m_func;
};

static struct LocalReferenceHolder LocalReferenceHolder_Setup(const char *func)
{
	struct LocalReferenceHolder refholder;
	refholder.m_env = NULL;
	refholder.m_func = func;
	return refholder;
}

static int LocalReferenceHolder_Init(struct LocalReferenceHolder *refholder, JNIEnv *env)
{
	const int capacity = 16;
	if ((*env)->PushLocalFrame(env, capacity) < 0) {
		I_Error("%s: Failed to allocate enough JVM local references", __FUNCTION__);
		return 0;
	}
	++s_active;
	refholder->m_env = env;
	return 1;
}

static void LocalReferenceHolder_Cleanup(struct LocalReferenceHolder *refholder)
{
	if (refholder->m_env) {
		JNIEnv* env = refholder->m_env;
		(*env)->PopLocalFrame(env, NULL);
		--s_active;
	}
}

static int LocalReferenceHolder_IsActive(void)
{
	return (s_active > 0);
}

#define LOCALREF struct LocalReferenceHolder refs = LocalReferenceHolder_Setup(__FUNCTION__);
#define CLEANREF LocalReferenceHolder_Cleanup(&refs);

const char *JNI_SharedStorage = NULL;
boolean JNI_StoragePermission = false;

// Implementation of getExternalStorageDirectory.
char *JNI_GetStorageDirectory(void)
{
	static char *storageDir = NULL;

	if (!storageDir)
	{
		LOCALREF
		JNIEnv *env;
		jmethodID method;
		jclass envClass;
		jobject fileObject;
		jstring pathString;
		const char *path;

		JNI_ENV(NULL);

		envClass = (*env)->FindClass(env, "android/os/Environment");
		if (!envClass)
		{
			CLEANREF
			return NULL;
		}

		method = (*env)->GetStaticMethodID(env, envClass, "getExternalStorageDirectory", "()Ljava/io/File;");
		if (!method)
		{
			CLEANREF
			return NULL;
		}

		// fileObject = environment.getExternalStorageDirectory();
		fileObject = (*env)->CallStaticObjectMethod(env, envClass, method);
		if (!fileObject)
		{
			CLEANREF
			return NULL;
		}

		// pathString = fileObject.getAbsolutePath();
		method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, fileObject), "getAbsolutePath", "()Ljava/lang/String;");
		pathString = (jstring)(*env)->CallObjectMethod(env, fileObject, method);

		path = (*env)->GetStringUTFChars(env, pathString, NULL);
		storageDir = malloc(strlen(path) + 1);
		strcpy(storageDir, path);
		(*env)->ReleaseStringUTFChars(env, pathString, path);

		CLEANREF
	}

	return storageDir;
}

// Get path to removable storage
static char *JNI_RemovableStorage = NULL;
char *JNI_RemovableStoragePath(void)
{
	LOCALREF
	JNIEnv *env;
	jmethodID method;
	jobject context;
	jobject fileObject;
	jobjectArray pathArray;
	jsize arraySize;
	jstring pathString;
	const char *path;

	JNI_ENV(NULL);
	JNI_CONTEXT;

#define NULLREMSTORAGE \
	if (JNI_RemovableStorage) \
	{ \
		free(JNI_RemovableStorage); \
		JNI_RemovableStorage = NULL; \
	}

	method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getExternalFilesDirs", "(Ljava/lang/String;)[Ljava/io/File;");
	if (!method)
	{
		CLEANREF
		NULLREMSTORAGE
		return NULL;
	}

	// fileObject = context.getExternalFilesDirs();
	fileObject = (*env)->CallObjectMethod(env, context, method, NULL);
	if (!fileObject)
	{
		CLEANREF
		NULLREMSTORAGE
		return NULL;
	}

	pathArray = (jobjectArray)fileObject; // Cast to array.
	arraySize = (jsize)(*env)->GetArrayLength(env, pathArray);
	if (arraySize < 2)
	{
		CLEANREF
		NULLREMSTORAGE
		return NULL;
	}

	// Get second file object.
	fileObject = (jobject)(*env)->GetObjectArrayElement(env, pathArray, 1);
	if (fileObject == NULL)
	{
		CLEANREF
		NULLREMSTORAGE
		return NULL;
	}

	// pathString = fileObject.getAbsolutePath();
	method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, fileObject), "getAbsolutePath", "()Ljava/lang/String;");
	if (!method)
	{
		CLEANREF
		NULLREMSTORAGE
		return NULL;
	}

	pathString = (jstring)(*env)->CallObjectMethod(env, fileObject, method);
	path = (*env)->GetStringUTFChars(env, pathString, NULL);

	JNI_RemovableStorage = realloc(JNI_RemovableStorage, strlen(path) + 1);
	strcpy(JNI_RemovableStorage, path);

	(*env)->ReleaseStringUTFChars(env, pathString, path);

	CLEANREF

#undef NULLREMSTORAGE

	return JNI_RemovableStorage;
}

// Get device info
// https://developer.android.com/reference/android/os/Build.html
char *JNI_GetDeviceInfo(const char *info)
{
	LOCALREF
	JNIEnv *env;

	jclass build;
	jfieldID id;
	jstring str;
	const char *deviceInfo_ref = NULL;
	char *deviceInfo = NULL;

	JNI_ENV(NULL);

	build = (*env)->FindClass(env, "android/os/Build");
	id = (*env)->GetStaticFieldID(env, build, info, "Ljava/lang/String;");
	str = (jstring)(*env)->GetStaticObjectField(env, build, id);
	deviceInfo_ref = (*env)->GetStringUTFChars(env, str, NULL);

	deviceInfo = malloc(strlen(deviceInfo_ref) + 1);
	strcpy(deviceInfo, deviceInfo_ref);
	(*env)->ReleaseStringUTFChars(env, str, deviceInfo_ref);

	CLEANREF

	return deviceInfo;
}

// https://developer.android.com/ndk/guides/abis
void JNI_SetupABIList(void)
{
	LOCALREF
	JNIEnv *env;
	jmethodID method;
	jobject context;

	jclass build;
	jfieldID id;
	jobjectArray array;
	jstring str;
	const char *ABI;
	int i;

	JNI_ENV();
	JNI_CONTEXT;

	build = (*env)->FindClass(env, "android/os/Build");
	id = (*env)->GetStaticFieldID(env, build, "SUPPORTED_ABIS", "[Ljava/lang/String;");
	array = (jobjectArray)(*env)->GetStaticObjectField(env, build, id);

	JNI_ABICount = (*env)->GetArrayLength(env, array);
	if (JNI_ABICount < 1)
		return;

	JNI_ABIList = malloc(sizeof(char **) * JNI_ABICount);
	if (!JNI_ABIList)
		return;

	for (i = 0; i < JNI_ABICount; i++)
	{
		str = (jstring)((*env)->GetObjectArrayElement(env, array, i));
		ABI = (*env)->GetStringUTFChars(env, str, NULL);
		JNI_ABIList[i] = malloc(strlen(ABI) + 1);
		strcpy(JNI_ABIList[i], ABI);
		(*env)->ReleaseStringUTFChars(env, str, ABI);
	}

	CLEANREF
}

boolean JNI_CheckPermission(const char *permission)
{
	JNIEnv *env = JNI_GetEnv();
	jstring permissionString = (*env)->NewStringUTF(env, permission);
	jmethodID method = (*env)->GetStaticMethodID(env, activityClass, "checkPermission", "(Ljava/lang/String;)Z");
	jboolean granted = (*env)->CallStaticBooleanMethod(env, activityClass, method, permissionString);
	return (granted == JNI_TRUE);
}

boolean JNI_CheckStoragePermission(void)
{
	return JNI_CheckPermission(JNI_GetWriteExternalStoragePermission());
}

boolean JNI_StoragePermissionGranted(void)
{
	return JNI_StoragePermission;
}

const char *JNI_GetWriteExternalStoragePermission(void)
{
	return "android.permission.WRITE_EXTERNAL_STORAGE";
}

void JNI_DisplayToast(const char *text)
{
	SDL_AndroidShowToast(text, 1, -1, 0, 0);
}

boolean JNI_IsInMultiWindowMode(void)
{
	JNIEnv *env = JNI_GetEnv();
	jmethodID method = (*env)->GetStaticMethodID(env, activityClass, "inMultiWindowMode", "()Z");
	jboolean multiwindow = (*env)->CallStaticBooleanMethod(env, activityClass, method);
	return (multiwindow == JNI_TRUE);
}
