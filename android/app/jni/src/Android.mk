LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

# Paths

SRB2_PATH := ../../../..
SRC_MAIN := $(SRB2_PATH)/src
SRC_JNI := .

ifeq ($(OS),Windows_NT)
WINDOWSHELL=1
endif

MAKE_DIR := $(LOCAL_PATH)/$(SRC_MAIN)/Makefile.d

ANDROID := 1

include $(MAKE_DIR)/platform.mk
include $(MAKE_DIR)/util.mk

# Source files

SRC_HWR := $(SRC_MAIN)/hardware/
SRC_SDL := $(SRC_MAIN)/sdl/

LOCAL_SRC_FILES := $(call List,$(LOCAL_PATH)/$(SRC_JNI)/Sourcefile)
LOCAL_SRC_FILES += $(call List,$(LOCAL_PATH)/$(SRC_MAIN)/Sourcefile)
LOCAL_SRC_FILES += $(call List,$(LOCAL_PATH)/$(SRC_MAIN)/blua/Sourcefile)
LOCAL_SRC_FILES += $(call List,$(LOCAL_PATH)/$(SRC_HWR)/Sourcefile)
LOCAL_SRC_FILES += $(call List,$(LOCAL_PATH)/$(SRC_SDL)/Sourcefile)

LOCAL_SRC_FILES += $(SRC_SDL)/SDL_main/SDL_android_main.c $(SRC_SDL)/mixer_sound.c $(SRC_SDL)/i_threads.c
LOCAL_SRC_FILES += $(SRC_HWR)/r_gles/r_gles2.c $(SRC_SDL)/ogl_es_sdl.c
LOCAL_SRC_FILES += $(SRC_MAIN)/w_handle.c $(SRC_MAIN)/comptime.c $(SRC_MAIN)/md5.c

# Compile flags
LOCAL_CFLAGS += -DUNIXCOMMON -DLINUX \
				-DHAVE_SDL -DHAVE_MIXER -DHAVE_MIXERX -DHAVE_LIBGME \
				-DHWRENDER -DHAVE_GLES -DHAVE_GLES2 \
				-DTOUCHINPUTS -DNATIVESCREENRES -DDIRECTFULLSCREEN \
				-DHAVE_ZLIB -DHAVE_PNG -DHAVE_CURL \
				-DHAVE_WHANDLE -DHAVE_THREADS -DLOGCAT -DCOMPVERSION \
				-DNONX86 -DNOASM -DNOMUMBLE

# Libraries
LOCAL_SHARED_LIBRARIES := SDL2 hidapi \
	SDL2_mixer libmpg123 \
	libpng libgme

LOCAL_STATIC_LIBRARIES := libcurl
LOCAL_LDLIBS := -lGLESv2 -lEGL -llog -lz

include $(BUILD_SHARED_LIBRARY)
