# game-music-emu shared library

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CFLAGS := -DLIBGME_VISIBILITY -DVGM_YM2612_GENS -DGME_SPC_ISOLATED_ECHO_BUFFER -DHAVE_ZLIB_H
LOCAL_CPPFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden -Wno-inconsistent-missing-override

 # Try to protect against undefined behavior from signed integer overflow
    # This has caused miscompilation of code already and there are other
    # potential uses; see https://bitbucket.org/mpyne/game-music-emu/issues/18/

LOCAL_CPPFLAGS += -fwrapv
LOCAL_CPP_FEATURES += exceptions

LOCAL_MODULE := libgme
LOCAL_SRC_FILES :=\
	$(LOCAL_PATH)/gme/Ay_Apu.cpp \
	$(LOCAL_PATH)/gme/Ay_Cpu.cpp \
	$(LOCAL_PATH)/gme/Ay_Emu.cpp \
	$(LOCAL_PATH)/gme/Blip_Buffer.cpp \
	$(LOCAL_PATH)/gme/Classic_Emu.cpp \
	$(LOCAL_PATH)/gme/Data_Reader.cpp \
	$(LOCAL_PATH)/gme/Dual_Resampler.cpp \
	$(LOCAL_PATH)/gme/Effects_Buffer.cpp \
	$(LOCAL_PATH)/gme/Fir_Resampler.cpp \
	$(LOCAL_PATH)/gme/Gbs_Emu.cpp \
	$(LOCAL_PATH)/gme/Gb_Apu.cpp \
	$(LOCAL_PATH)/gme/Gb_Cpu.cpp \
	$(LOCAL_PATH)/gme/Gb_Oscs.cpp \
	$(LOCAL_PATH)/gme/gme.cpp \
	$(LOCAL_PATH)/gme/Gme_File.cpp \
	$(LOCAL_PATH)/gme/Gym_Emu.cpp \
	$(LOCAL_PATH)/gme/Hes_Apu.cpp \
	$(LOCAL_PATH)/gme/Hes_Cpu.cpp \
	$(LOCAL_PATH)/gme/Hes_Emu.cpp \
	$(LOCAL_PATH)/gme/Kss_Cpu.cpp \
	$(LOCAL_PATH)/gme/Kss_Emu.cpp \
	$(LOCAL_PATH)/gme/Kss_Scc_Apu.cpp \
	$(LOCAL_PATH)/gme/M3u_Playlist.cpp \
	$(LOCAL_PATH)/gme/Multi_Buffer.cpp \
	$(LOCAL_PATH)/gme/Music_Emu.cpp \
	$(LOCAL_PATH)/gme/Nes_Apu.cpp \
	$(LOCAL_PATH)/gme/Nes_Cpu.cpp \
	$(LOCAL_PATH)/gme/Nes_Fme7_Apu.cpp \
	$(LOCAL_PATH)/gme/Nes_Namco_Apu.cpp \
	$(LOCAL_PATH)/gme/Nes_Oscs.cpp \
	$(LOCAL_PATH)/gme/Nes_Vrc6_Apu.cpp \
	$(LOCAL_PATH)/gme/Nsfe_Emu.cpp \
	$(LOCAL_PATH)/gme/Nsf_Emu.cpp \
	$(LOCAL_PATH)/gme/Sap_Apu.cpp \
	$(LOCAL_PATH)/gme/Sap_Cpu.cpp \
	$(LOCAL_PATH)/gme/Sap_Emu.cpp \
	$(LOCAL_PATH)/gme/Sms_Apu.cpp \
	$(LOCAL_PATH)/gme/Snes_Spc.cpp \
	$(LOCAL_PATH)/gme/Spc_Cpu.cpp \
	$(LOCAL_PATH)/gme/Spc_Dsp.cpp \
	$(LOCAL_PATH)/gme/Spc_Emu.cpp \
	$(LOCAL_PATH)/gme/Spc_Filter.cpp \
	$(LOCAL_PATH)/gme/Vgm_Emu.cpp \
	$(LOCAL_PATH)/gme/Vgm_Emu_Impl.cpp \
	$(LOCAL_PATH)/gme/Ym2413_Emu.cpp \
	$(LOCAL_PATH)/gme/Ym2612_GENS.cpp

LOCAL_LDLIBS := -lz
LOCAL_STATIC_LIBRARIES := c++_static
include $(BUILD_SHARED_LIBRARY)

$(call import-module,cxx-stl/llvm-libc++)