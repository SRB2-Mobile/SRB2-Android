LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := SDL2_mixer

LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libSDL2_mixer.so

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_SHARED_LIBRARIES := SDL2 mpg123

include $(PREBUILT_SHARED_LIBRARY)

###########################
#
# mpg123 library
#
###########################

include $(CLEAR_VARS)

LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libmpg123.so

LOCAL_MODULE := mpg123
LOCAL_LDLIBS := -llog

include $(PREBUILT_SHARED_LIBRARY)