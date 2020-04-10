// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
// Copyright (C) 1997-2020 by Sam "Slouken" Lantinga.
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
static JNIEnv *jni_env = NULL;
static jobject activity_object;
static jclass activity_class;

void JNI_Startup(void)
{
	CONS_Printf("JNI_Startup()...\n");
	JNI_SetupActivity();
}

void JNI_SetupActivity(void)
{
	jni_env = (JNIEnv *)SDL_AndroidGetJNIEnv();
	(*jni_env)->GetJavaVM(jni_env, &jvm);
	activity_object = (jobject)SDL_AndroidGetActivity();
	activity_class = (*jni_env)->GetObjectClass(jni_env, activity_object);
}

static JNIEnv *JNI_GetEnv(void)
{
	JNIEnv *env;
	int status = (*jvm)->AttachCurrentThread(jvm, &env, NULL);
	if (status < 0)
		return jni_env;
	return env;
}

#define JNI_CONTEXT(returnarg) \
{ \
	env = JNI_GetEnv(); \
	if (!LocalReferenceHolder_Init(&refs, env)) \
	{ \
		LocalReferenceHolder_Cleanup(&refs); \
		return returnarg; \
	} \
	method = (*env)->GetStaticMethodID(env, activity_class, \
			"getContext", "()Landroid/content/Context;"); \
	context = (*env)->CallStaticObjectMethod(env, activity_class, method); \
}

// Lactozilla: I looked at (read: copypasted) SDL2's SDL_android.c for reference.
// https://hg.libsdl.org/SDL/file/tip/src/core/android/SDL_android.c

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
		I_Error("Failed to allocate enough JVM local references");
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

// Get the secondary external storage path
const char *JNI_ExternalStoragePath(void)
{
	static char *s_AndroidExternalFilesPath = NULL;

	if (!s_AndroidExternalFilesPath)
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

		JNI_CONTEXT(NULL);

		// fileObj = context.getExternalFilesDirs();
		method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context),
				"getExternalFilesDirs", "(Ljava/lang/String;)[Ljava/io/File;");
		fileObject = (*env)->CallObjectMethod(env, context, method, NULL);
		if (!fileObject)
		{
			CLEANREF
			return NULL;
		}

		// cast to array
		pathArray = (jobjectArray)fileObject;

		// first path string isn't external storage so that's not what I want
		arraySize = (jsize)(*env)->GetArrayLength(env, pathArray);
		if (arraySize < 2)
		{
			CLEANREF
			return NULL;
		}

		// get second file object
		fileObject = (jobject)(*env)->GetObjectArrayElement(env, pathArray, 1);

		// path = fileObject.getAbsolutePath();
		method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, fileObject),
				"getAbsolutePath", "()Ljava/lang/String;");
		pathString = (jstring)(*env)->CallObjectMethod(env, fileObject, method);

		path = (*env)->GetStringUTFChars(env, pathString, NULL);
		s_AndroidExternalFilesPath = malloc(strlen(path) + 1);
		strcpy(s_AndroidExternalFilesPath, path);
		(*env)->ReleaseStringUTFChars(env, pathString, path);

		CLEANREF
	}

	return s_AndroidExternalFilesPath;
}
