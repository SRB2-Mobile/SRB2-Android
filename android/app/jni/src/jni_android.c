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

#if defined(__ANDROID__)

#include "SDL.h"
#include "jni_android.h"

static JNIEnv *mEnv = NULL;
static jobject mActivityObject;
static jclass mActivityClass;

void JNI_Startup(void)
{
	CONS_Printf("JNI_Startup()...\n");

	mEnv = (JNIEnv *)SDL_AndroidGetJNIEnv();
	mActivityObject = (jobject)SDL_AndroidGetActivity();
	mActivityClass = (*mEnv)->GetObjectClass(mEnv, mActivityObject);
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

const char *JNI_ExternalStoragePath(void)
{
    static char *s_AndroidExternalFilesPath = NULL;

    if (!s_AndroidExternalFilesPath) {
        struct LocalReferenceHolder refs = LocalReferenceHolder_Setup(__FUNCTION__);
        jmethodID mid;
        jobject context;
        jobject fileObject;
        jobjectArray pathArray;
        jsize arraySize;
        jstring pathString;
        const char *path;

        JNIEnv *env = mEnv;
        if (!LocalReferenceHolder_Init(&refs, env)) {
            LocalReferenceHolder_Cleanup(&refs);
            return NULL;
        }

        // context = SDLActivity.getContext();
        mid = (*env)->GetStaticMethodID(env, mActivityClass,
                "getContext","()Landroid/content/Context;");
        context = (*env)->CallStaticObjectMethod(env, mActivityClass, mid);

        // fileObj = context.getExternalFilesDirs();
        mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context),
                "getExternalFilesDirs", "(Ljava/lang/String;)[Ljava/io/File;");
        fileObject = (*env)->CallObjectMethod(env, context, mid, NULL);
        if (!fileObject) {
            LocalReferenceHolder_Cleanup(&refs);
            return NULL;
        }

        // cast to array
        pathArray = (jobjectArray)fileObject;

        // first path string isn't external storage so that's not what I want
        arraySize = (jsize)(*env)->GetArrayLength(env, pathArray);
        if (arraySize < 2)
		{
			LocalReferenceHolder_Cleanup(&refs);
			return NULL;
		}

        // get second file object
        fileObject = (jobject)(*env)->GetObjectArrayElement(env, pathArray, 1);

        // path = fileObject.getAbsolutePath();
        mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, fileObject),
                "getAbsolutePath", "()Ljava/lang/String;");
        pathString = (jstring)(*env)->CallObjectMethod(env, fileObject, mid);

        path = (*env)->GetStringUTFChars(env, pathString, NULL);
        s_AndroidExternalFilesPath = malloc(strlen(path) + 1);
        strcpy(s_AndroidExternalFilesPath, path);
        (*env)->ReleaseStringUTFChars(env, pathString, path);

        LocalReferenceHolder_Cleanup(&refs);
    }
    return s_AndroidExternalFilesPath;
}

#endif
