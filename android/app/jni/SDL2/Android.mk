LOCAL_PATH := $(call my-dir)

###########################
#
# SDL shared library
#
###########################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libSDL2.so

LOCAL_MODULE := SDL2
LOCAL_LDLIBS := -ldl -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid
LOCAL_SHARED_LIBRARIES := hidapi

ifeq ($(NDK_DEBUG),1)
    cmd-strip :=
endif

LOCAL_STATIC_LIBRARIES := cpufeatures

include $(PREBUILT_SHARED_LIBRARY)

###########################
#
# hidapi library
#
###########################

include $(CLEAR_VARS)

LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libhidapi.so

LOCAL_MODULE := libhidapi
LOCAL_LDLIBS := -llog

include $(PREBUILT_SHARED_LIBRARY)

$(call import-module,android/cpufeatures)