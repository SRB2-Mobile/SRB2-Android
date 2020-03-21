LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../../../../SDL

OBJDIR := ../../../../src/

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
			$(OBJDIR)/md5.c      \
			$(OBJDIR)/sdl/i_cdmus.c  \
			$(OBJDIR)/sdl/i_net.c    \
			$(OBJDIR)/sdl/i_video.c  \
			$(OBJDIR)/sdl/i_system.c \
			$(OBJDIR)/sdl/mixer_sound.c\
			$(OBJDIR)/sdl/dosstr.c   \
			$(OBJDIR)/sdl/endtxt.c   \
			$(OBJDIR)/sdl/hwsym_sdl.c\
			$(OBJDIR)/sdl/SDL_main/SDL_android_main.c\

# Lactozilla: I added md5.c in this list because the Makefile did it differently,
# and removed vid_copy.s because there is no ASM compiling here. At least for now.
# Also, all of the SDL interface files are there.

LOCAL_CFLAGS += -DANDROID \
				-DHAVE_SDL -DHAVE_MIXER \
				-DUNIXCOMMON -DLINUX \
				-DDEBUGMODE -DDIRECTFULLSCREEN \
				-DNONX86 -DNOASM -DNOHW -DNOMUMBLE \
				-DHAVE_ZLIB

LOCAL_SHARED_LIBRARIES := SDL2 \
	SDL2_mixer

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog -lz

include $(BUILD_SHARED_LIBRARY)
