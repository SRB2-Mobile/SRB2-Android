LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../../../../SDL

OBJDIR := ../../../../src/
JNIDIR := .

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include

SDL2_SOURCES := $(OBJDIR)/sdl/
LUA_SOURCES := $(OBJDIR)/blua/
HWR_SOURCES := $(OBJDIR)/hardware/

#
# SRB2 main source files
#

LOCAL_SRC_FILES := $(JNIDIR)/jni_android.c \
			$(JNIDIR)/ndk_strings.c \
			$(JNIDIR)/ndk_crash_handler.c \
			$(OBJDIR)/comptime.c \
			$(OBJDIR)/string.c   \
			$(OBJDIR)/d_main.c   \
			$(OBJDIR)/d_clisrv.c \
			$(OBJDIR)/d_net.c    \
			$(OBJDIR)/d_netfil.c \
			$(OBJDIR)/d_netcmd.c \
			$(OBJDIR)/dehacked.c \
			$(OBJDIR)/z_zone.c   \
			$(OBJDIR)/f_finale.c \
			$(OBJDIR)/f_wipe.c   \
			$(OBJDIR)/g_game.c   \
			$(OBJDIR)/g_demo.c   \
			$(OBJDIR)/g_input.c  \
			$(OBJDIR)/ts_main.c  \
			$(OBJDIR)/ts_custom.c\
			$(OBJDIR)/am_map.c   \
			$(OBJDIR)/command.c  \
			$(OBJDIR)/console.c  \
			$(OBJDIR)/hu_stuff.c \
			$(OBJDIR)/y_inter.c  \
			$(OBJDIR)/st_stuff.c \
			$(OBJDIR)/m_aatree.c \
			$(OBJDIR)/m_anigif.c \
			$(OBJDIR)/m_argv.c   \
			$(OBJDIR)/m_bbox.c   \
			$(OBJDIR)/m_cheat.c  \
			$(OBJDIR)/m_cond.c   \
			$(OBJDIR)/m_fixed.c  \
			$(OBJDIR)/m_menu.c   \
			$(OBJDIR)/m_misc.c   \
			$(OBJDIR)/m_random.c \
			$(OBJDIR)/m_queue.c  \
			$(OBJDIR)/info.c     \
			$(OBJDIR)/p_ceilng.c \
			$(OBJDIR)/p_enemy.c  \
			$(OBJDIR)/p_floor.c  \
			$(OBJDIR)/p_inter.c  \
			$(OBJDIR)/p_lights.c \
			$(OBJDIR)/p_map.c    \
			$(OBJDIR)/p_maputl.c \
			$(OBJDIR)/p_mobj.c   \
			$(OBJDIR)/p_polyobj.c\
			$(OBJDIR)/p_saveg.c  \
			$(OBJDIR)/p_setup.c  \
			$(OBJDIR)/p_sight.c  \
			$(OBJDIR)/p_spec.c   \
			$(OBJDIR)/p_telept.c \
			$(OBJDIR)/p_tick.c   \
			$(OBJDIR)/p_user.c   \
			$(OBJDIR)/p_slopes.c \
			$(OBJDIR)/tables.c   \
			$(OBJDIR)/r_bsp.c    \
			$(OBJDIR)/r_data.c   \
			$(OBJDIR)/r_draw.c   \
			$(OBJDIR)/r_main.c   \
			$(OBJDIR)/r_plane.c  \
			$(OBJDIR)/r_segs.c   \
			$(OBJDIR)/r_skins.c  \
			$(OBJDIR)/r_sky.c    \
			$(OBJDIR)/r_splats.c \
			$(OBJDIR)/r_things.c \
			$(OBJDIR)/r_patch.c  \
			$(OBJDIR)/r_portal.c \
			$(OBJDIR)/screen.c   \
			$(OBJDIR)/v_video.c  \
			$(OBJDIR)/s_sound.c  \
			$(OBJDIR)/sounds.c   \
			$(OBJDIR)/w_wad.c    \
			$(OBJDIR)/w_handle.c \
			$(OBJDIR)/filesrch.c \
			$(OBJDIR)/mserv.c    \
			$(OBJDIR)/i_tcp.c    \
			$(OBJDIR)/lzf.c      \
			$(OBJDIR)/b_bot.c    \
			$(OBJDIR)/md5.c

# Lua
LOCAL_SRC_FILES += $(OBJDIR)/lua_script.c \
	$(OBJDIR)/lua_baselib.c \
	$(OBJDIR)/lua_mathlib.c \
	$(OBJDIR)/lua_hooklib.c \
	$(OBJDIR)/lua_consolelib.c \
	$(OBJDIR)/lua_infolib.c \
	$(OBJDIR)/lua_mobjlib.c \
	$(OBJDIR)/lua_playerlib.c \
	$(OBJDIR)/lua_skinlib.c \
	$(OBJDIR)/lua_thinkerlib.c \
	$(OBJDIR)/lua_maplib.c \
	$(OBJDIR)/lua_blockmaplib.c \
	$(OBJDIR)/lua_hudlib.c \
	$(LUA_SOURCES)/lapi.c \
	$(LUA_SOURCES)/lbaselib.c \
	$(LUA_SOURCES)/ldo.c \
	$(LUA_SOURCES)/lfunc.c \
	$(LUA_SOURCES)/linit.c \
	$(LUA_SOURCES)/liolib.c \
	$(LUA_SOURCES)/llex.c \
	$(LUA_SOURCES)/lmem.c \
	$(LUA_SOURCES)/lobject.c \
	$(LUA_SOURCES)/lstate.c \
	$(LUA_SOURCES)/lstrlib.c \
	$(LUA_SOURCES)/ltablib.c \
	$(LUA_SOURCES)/lundump.c \
	$(LUA_SOURCES)/lzio.c \
	$(LUA_SOURCES)/lauxlib.c \
	$(LUA_SOURCES)/lcode.c \
	$(LUA_SOURCES)/ldebug.c \
	$(LUA_SOURCES)/ldump.c \
	$(LUA_SOURCES)/lgc.c \
	$(LUA_SOURCES)/lopcodes.c \
	$(LUA_SOURCES)/lparser.c \
	$(LUA_SOURCES)/lstring.c \
	$(LUA_SOURCES)/ltable.c \
	$(LUA_SOURCES)/ltm.c \
	$(LUA_SOURCES)/lvm.c \

# OpenGL
LOCAL_SRC_FILES += $(HWR_SOURCES)/r_gles/r_gles1.c $(SDL2_SOURCES)/ogl_es_sdl.c \
		$(HWR_SOURCES)/hw_bsp.c \
		$(HWR_SOURCES)/hw_draw.c \
		$(HWR_SOURCES)/hw_light.c \
		$(HWR_SOURCES)/hw_main.c \
		$(HWR_SOURCES)/hw_clip.c \
		$(HWR_SOURCES)/hw_md2.c \
		$(HWR_SOURCES)/hw_cache.c \
		$(HWR_SOURCES)/hw_trick.c \
		$(HWR_SOURCES)/hw_md2load.c \
		$(HWR_SOURCES)/hw_md3load.c \
		$(HWR_SOURCES)/hw_model.c \
		$(HWR_SOURCES)/u_list.c

#
# SDL2 interface
#
LOCAL_SRC_FILES += $(SDL2_SOURCES)/i_cdmus.c  \
			$(SDL2_SOURCES)/i_net.c    \
			$(SDL2_SOURCES)/i_video.c  \
			$(SDL2_SOURCES)/i_system.c \
			$(SDL2_SOURCES)/mixer_sound.c\
			$(SDL2_SOURCES)/dosstr.c   \
			$(SDL2_SOURCES)/endtxt.c   \
			$(SDL2_SOURCES)/hwsym_sdl.c\
			$(SDL2_SOURCES)/SDL_main/SDL_android_main.c

# Compile flags
LOCAL_CFLAGS += -DUNIXCOMMON -DLINUX \
				-DHAVE_SDL -DHAVE_MIXER \
				-DHWRENDER -DHAVE_GLES \
				-DTOUCHINPUTS -DDIRECTFULLSCREEN \
				-DHAVE_ZLIB -DHAVE_BLUA -DHAVE_PNG \
				-DHAVE_WHANDLE \
				-DNONX86 -DNOASM -DNOMUMBLE

# Libraries
LOCAL_SHARED_LIBRARIES := SDL2 hidapi \
	SDL2_mixer libmpg123 \
	libpng

LOCAL_LDLIBS := -lGLESv1_CM -lEGL -llog -lz

include $(BUILD_SHARED_LIBRARY)
