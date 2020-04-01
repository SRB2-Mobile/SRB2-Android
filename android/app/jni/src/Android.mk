LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../../../../SDL

OBJDIR := ../../../../src/
JNIDIR := .

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include

# Add your application source files here...
LOCAL_SRC_FILES := $(OBJDIR)/comptime.c \
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
			$(OBJDIR)/g_input.c  \
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
			$(OBJDIR)/filesrch.c \
			$(OBJDIR)/mserv.c    \
			$(OBJDIR)/i_tcp.c    \
			$(OBJDIR)/lzf.c      \
			$(OBJDIR)/b_bot.c    \
			$(OBJDIR)/md5.c

# Include the SDL2 interface files.
SDL2_SOURCE := ../../../../src/sdl/
LOCAL_SRC_FILES += $(SDL2_SOURCE)/i_cdmus.c  \
			$(SDL2_SOURCE)/i_net.c    \
			$(SDL2_SOURCE)/i_video.c  \
			$(SDL2_SOURCE)/i_system.c \
			$(SDL2_SOURCE)/mixer_sound.c\
			$(SDL2_SOURCE)/dosstr.c   \
			$(SDL2_SOURCE)/endtxt.c   \
			$(SDL2_SOURCE)/hwsym_sdl.c\
			$(SDL2_SOURCE)/SDL_main/SDL_android_main.c

# Include Lua.
LUA_SOURCE := ../../../../src/blua/
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
	$(LUA_SOURCE)/lapi.c \
	$(LUA_SOURCE)/lbaselib.c \
	$(LUA_SOURCE)/ldo.c \
	$(LUA_SOURCE)/lfunc.c \
	$(LUA_SOURCE)/linit.c \
	$(LUA_SOURCE)/llex.c \
	$(LUA_SOURCE)/lmem.c \
	$(LUA_SOURCE)/lobject.c \
	$(LUA_SOURCE)/lstate.c \
	$(LUA_SOURCE)/lstrlib.c \
	$(LUA_SOURCE)/ltablib.c \
	$(LUA_SOURCE)/lundump.c \
	$(LUA_SOURCE)/lzio.c \
	$(LUA_SOURCE)/lauxlib.c \
	$(LUA_SOURCE)/lcode.c \
	$(LUA_SOURCE)/ldebug.c \
	$(LUA_SOURCE)/ldump.c \
	$(LUA_SOURCE)/lgc.c \
	$(LUA_SOURCE)/lopcodes.c \
	$(LUA_SOURCE)/lparser.c \
	$(LUA_SOURCE)/lstring.c \
	$(LUA_SOURCE)/ltable.c \
	$(LUA_SOURCE)/ltm.c \
	$(LUA_SOURCE)/lvm.c \
	$(JNIDIR)/localeconv.c

LOCAL_CFLAGS += -DHAVE_SDL -DHAVE_MIXER \
				-DTOUCHINPUTS \
				-DUNIXCOMMON -DLINUX \
				-DDEBUGMODE -DLOGCAT -DDIRECTFULLSCREEN \
				-DHAVE_ZLIB -DHAVE_BLUA \
				-DNONX86 -DNOASM -DNOHW -DNOMUMBLE

LOCAL_SHARED_LIBRARIES := SDL2 hidapi \
	SDL2_mixer libmpg123

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog -lz

include $(BUILD_SHARED_LIBRARY)
