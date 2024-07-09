#
# Quake Makefile for Linux 2.0
#
# Aug '98 by Zoid <zoid@idsoftware.com>
#
# ELF only
#
# Missi: edited for BananaQuake (4/26/2023)
#

BASEVERSION=1.09
VERSION=$(BASEVERSION)$(LIBC++)

# RPM release number
RPM_RELEASE=5

# ifneq (,$(findstring libc6,$(shell if [ -e /lib/libc.so.6 ];then echo libc6;fi)))
# LIBC++=-glibc
# else
# LIBC++=
# endif

### Enable/Disable codecs for streaming music support
USE_CODEC_WAVE=1
USE_CODEC_FLAC=0
# vvvv Missi: was 1
USE_CODEC_MP3=1
USE_CODEC_VORBIS=1
USE_CODEC_OPUS=0
# either xmp or mikmod (or modplug)
USE_CODEC_MIKMOD=0
USE_CODEC_XMP=0
USE_CODEC_MODPLUG=0
USE_CODEC_UMX=0

ifneq (,$(findstring alpha,$(shell uname -m)))
ARCH=axp
else
#ARCH=i386
ARCH=x86_64
endif
NOARCH=noarch

MOUNT_DIR=/QuakeCPP/WinQuake

BUILD_DEBUG_DIR=debug_$(ARCH)$(LIBC++)
BUILD_RELEASE_DIR=release_$(ARCH)$(LIBC++)

# Missi: was gcc
EGCS=/bin/g++
CC=$(EGCS)

BASE_CPPFLAGS=-Dstricmp=strcasecmp
BASE_CPPFLAGS += $(call check_g++,-std=gnu11,)

RELEASE_CPPFLAGS=$(BASE_CPPFLAGS) -O6

#RELEASE_CPPFLAGS=$(BASE_CPPFLAGS) -O6 -ffast-math -funroll-loops \
#	-fomit-frame-pointer -fexpensive-optimizations
DEBUG_CPPFLAGS=$(BASE_CPPFLAGS) -g
LDFLAGS=-z noexecstack
SVGALDFLAGS=-lvga
XLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext -lXxf86dga
XCPPFLAGS=-DX11

CODECLIBS  :=
ifeq ($(USE_CODEC_WAVE),1)
BASE_CPPFLAGS+= -DUSE_CODEC_WAVE
endif
ifeq ($(USE_CODEC_FLAC),1)
BASE_CPPFLAGS+= -DUSE_CODEC_FLAC
CODECLIBS+= -lFLAC
endif
ifeq ($(USE_CODEC_OPUS),1)
# opus and opusfile put their *.h under <includedir>/opus,
# but they include the headers without the opus directory
# prefix and rely on pkg-config. ewww...
BASE_CPPFLAGS+= -DUSE_CODEC_OPUS
BASE_CPPFLAGS+= $(shell $(PKG_CONFIG) --cflags opusfile)
CODECLIBS+= $(shell $(PKG_CONFIG) --libs   opusfile)
endif
ifeq ($(USE_CODEC_VORBIS),1)
BASE_CPPFLAGS+= -DUSE_CODEC_VORBIS
CODECLIBS+= -lvorbis -lvorbisfile
endif
ifeq ($(USE_CODEC_MP3),1)
BASE_CPPFLAGS+= -DUSE_CODEC_MP3
CODECLIBS+= -lmad
endif
ifeq ($(USE_CODEC_MIKMOD),1)
BASE_CPPFLAGS+= -DUSE_CODEC_MIKMOD
CODECLIBS+= -lmikmod
endif
ifeq ($(USE_CODEC_XMP),1)
BASE_CPPFLAGS+= -DUSE_CODEC_XMP
CODECLIBS+= -lxmp
endif
ifeq ($(USE_CODEC_MODPLUG),1)
BASE_CPPFLAGS+= -DUSE_CODEC_MODPLUG
CODECLIBS+= -lmodplug
endif
ifeq ($(USE_CODEC_UMX),1)
BASE_CPPFLAGS+= -DUSE_CODEC_UMX
endif

# old linker stuff:
# -lGL -lGLU -lGLX

#	-I/usr/include/glide \
	-I/usr/src/glide/h5/glide3/src \
	-I/usr/src/glide/swlibs/fxmisc \
	-I/usr/src/glide/h5/incsrc \
	
#	-lX11 \
	-lXext \
	-ldl \
	-lXxf86dga \
	-lXxf86vm \

# -L/usr/X11/lib \
	
GLLIBINCLUDE=-L./sdl2/build -L./SDL_image -L./codecs/x64
	
GLLIBLINK=-lSDL2 -lSDL2_image -lm -lGL -lX11 -lXext -lXxf86dga -lXxf86vm -ldl $(CODECLIBS)

GLINCLUDES=-I./sdl2/include -I./SDL_image/include -I./codecs/include -I/QuakeCPP/WinQuake/mesa/3dfx/glide3/include/
	
GLOPTS:=-DGLQUAKE
	
GLCPPFLAGS=$(GLINCLUDES)

DO_CC=$(CC) $(BASE_CPPFLAGS) $(CPPFLAGS) -o $@ -c $<
DO_DEBUG_CC=$(CC) $(BASE_CPPFLAGS) $(DEBUG_CPPFLAGS) -o $@ -c $<
DO_GL_CC=$(CC) $(BASE_CPPFLAGS) $(CPPFLAGS) $(GLOPTS) $(GLCPPFLAGS) -o $@ -c $<
DO_GL_DEBUG_CC=$(CC) $(BASE_CPPFLAGS) $(DEBUG_CPPFLAGS) $(GLOPTS) $(GLCPPFLAGS) -o $@ -c $<
DO_O_CC=$(CC) $(BASE_CPPFLAGS) -O $(CPPFLAGS) -o $@ -c $<
DO_AS=$(CC) $(BASE_CPPFLAGS) $(CPPFLAGS) -DELF -x assembler-with-cpp -o $@ -c $<
DO_GL_AS=$(CC) $(BASE_CPPFLAGS) $(CPPFLAGS) $(GLCPPFLAGS) -DELF -x assembler-with-cpp -o $@ -c $<

MAKEFILE_SDL2 ?= $(shell find -O3 ./sdl2 -maxdepth 1 -name Makefile -print)
MAKEFILE_SDL_IMAGE ?= $(shell find -O3 ./SDL_image -maxdepth 1 -name Makefile -print)

SDL2_CONFIG=
SDL_IMAGE_CONFIG=

# Missi: check if any makefiles were found so we only have to configure once per installation (7/8/2024)
#
ifeq ($(MAKEFILE_SDL2),)	# Missi: check for empty string (7/8/2024)
	SDL2_CONFIG += (cd ./sdl2 && ./configure --prefix=${HOME}/sdl2 && cd ../)
endif
ifeq ($(MAKEFILE_SDL_IMAGE),)	# Missi: check for empty string (7/8/2024)
	SDL_IMAGE_CONFIG += (cd ./SDL_image && ./configure --prefix=${HOME}/SDL_image && cd ../)
endif

#############################################################################
# SETUP AND BUILD
#############################################################################

TARGETS=$(BUILDDIR)/bin/bananaquakeopengl
	# $(BUILDDIR)/bin/unixded

build_debug:
	@-mkdir $(BUILD_DEBUG_DIR) \
		$(BUILD_DEBUG_DIR)/bin \
		$(BUILD_DEBUG_DIR)/bananaquakeopengl \
		# $(BUILD_DEBUG_DIR)/squake \
		$(BUILD_DEBUG_DIR)/unixded \
		$(BUILD_DEBUG_DIR)/x11
	$(SDL2_CONFIG)
	$(MAKE) -C ./sdl2
	$(MAKE) -C ./sdl2 install
	$(SDL_IMAGE_CONFIG)
	$(MAKE) -C ./SDL_image
	$(MAKE) -C ./SDL_image install
	$(MAKE) targets BUILDDIR=$(BUILD_DEBUG_DIR) CPPFLAGS="$(DEBUG_CPPFLAGS)"
	$(shell cp "./$(BUILD_DEBUG_DIR)/bin/bananaquakeopengl" "./")

build_release:
	@-mkdir $(BUILD_RELEASE_DIR) \
		$(BUILD_RELEASE_DIR)/bin \
		$(BUILD_RELEASE_DIR)/bananaquakeopengl \
		# $(BUILD_RELEASE_DIR)/squake \
		$(BUILD_RELEASE_DIR)/unixded \
		$(BUILD_RELEASE_DIR)/x11
	$(SDL2_CONFIG)
	$(MAKE) -C ./sdl2
	$(MAKE) -C ./sdl2 install
	$(SDL_IMAGE_CONFIG)
	$(MAKE) -C ./SDL_image
	$(MAKE) -C ./SDL_image install
	$(MAKE) targets BUILDDIR=$(BUILD_RELEASE_DIR) CPPFLAGS="$(RELEASE_CPPFLAGS)"
	$(shell cp "./$(BUILD_RELEASE_DIR)/bin/bananaquakeopengl" "./")

all: build_debug build_release

targets: $(TARGETS)

#############################################################################
# SVGALIB Quake
#############################################################################

SQUAKE_OBJS = \
	$(BUILDDIR)/squake/cfgfile.o \
	$(BUILDDIR)/squake/cl_demo.o \
	$(BUILDDIR)/squake/cl_input.o \
	$(BUILDDIR)/squake/cl_main.o \
	$(BUILDDIR)/squake/cl_parse.o \
	$(BUILDDIR)/squake/cl_tent.o \
	$(BUILDDIR)/squake/chase.o \
	$(BUILDDIR)/squake/cmd.o \
	$(BUILDDIR)/squake/common.o \
	$(BUILDDIR)/squake/console.o \
	$(BUILDDIR)/squake/crc.o \
	$(BUILDDIR)/squake/cvar.o \
	$(BUILDDIR)/squake/draw.o \
	$(BUILDDIR)/squake/d_edge.o \
	$(BUILDDIR)/squake/d_fill.o \
	$(BUILDDIR)/squake/d_init.o \
	$(BUILDDIR)/squake/d_modech.o \
	$(BUILDDIR)/squake/d_part.o \
	$(BUILDDIR)/squake/d_polyse.o \
	$(BUILDDIR)/squake/d_scan.o \
	$(BUILDDIR)/squake/d_sky.o \
	$(BUILDDIR)/squake/d_sprite.o \
	$(BUILDDIR)/squake/d_surf.o \
	$(BUILDDIR)/squake/d_vars.o \
	$(BUILDDIR)/squake/d_zpoint.o \
	$(BUILDDIR)/squake/host.o \
	$(BUILDDIR)/squake/host_cmd.o \
	$(BUILDDIR)/squake/keys.o \
	$(BUILDDIR)/squake/menu.o \
	$(BUILDDIR)/squake/mathlib.o \
	$(BUILDDIR)/squake/model.o \
	$(BUILDDIR)/squake/net_dgrm.o \
	$(BUILDDIR)/squake/net_loop.o \
	$(BUILDDIR)/squake/net_main.o \
	$(BUILDDIR)/squake/net_vcr.o \
	$(BUILDDIR)/squake/net_udp.o \
	$(BUILDDIR)/squake/net_bsd.o \
	$(BUILDDIR)/squake/nonintel.o \
	$(BUILDDIR)/squake/pr_cmds.o \
	$(BUILDDIR)/squake/pr_edict.o \
	$(BUILDDIR)/squake/pr_exec.o \
	$(BUILDDIR)/squake/r_aclip.o \
	$(BUILDDIR)/squake/r_alias.o \
	$(BUILDDIR)/squake/r_bsp.o \
	$(BUILDDIR)/squake/r_light.o \
	$(BUILDDIR)/squake/r_draw.o \
	$(BUILDDIR)/squake/r_efrag.o \
	$(BUILDDIR)/squake/r_edge.o \
	$(BUILDDIR)/squake/r_misc.o \
	$(BUILDDIR)/squake/r_main.o \
	$(BUILDDIR)/squake/r_sky.o \
	$(BUILDDIR)/squake/r_sprite.o \
	$(BUILDDIR)/squake/r_surf.o \
	$(BUILDDIR)/squake/r_part.o \
	$(BUILDDIR)/squake/r_vars.o \
	$(BUILDDIR)/squake/screen.o \
	$(BUILDDIR)/squake/sbar.o \
	$(BUILDDIR)/squake/sv_main.o \
	$(BUILDDIR)/squake/sv_phys.o \
	$(BUILDDIR)/squake/sv_move.o \
	$(BUILDDIR)/squake/sv_user.o \
	$(BUILDDIR)/squake/zone.o	\
	$(BUILDDIR)/squake/view.o	\
	$(BUILDDIR)/squake/wad.o \
	$(BUILDDIR)/squake/world.o \
	$(BUILDDIR)/squake/cd_linux.o \
	$(BUILDDIR)/squake/sys_linux.o \
	$(BUILDDIR)/squake/vid_svgalib.o \
	$(BUILDDIR)/squake/snd_dma.o \
	$(BUILDDIR)/squake/snd_mem.o \
	$(BUILDDIR)/squake/snd_mix.o \
	$(BUILDDIR)/squake/snd_linux.o \
	\
	$(BUILDDIR)/squake/d_copy.o \
	$(BUILDDIR)/squake/d_draw.o \
	$(BUILDDIR)/squake/d_draw16.o \
	$(BUILDDIR)/squake/d_parta.o \
	$(BUILDDIR)/squake/d_polysa.o \
	$(BUILDDIR)/squake/d_scana.o \
	$(BUILDDIR)/squake/d_spr8.o \
	$(BUILDDIR)/squake/d_varsa.o \
	$(BUILDDIR)/squake/math.o \
	$(BUILDDIR)/squake/r_aliasa.o \
	$(BUILDDIR)/squake/r_drawa.o \
	$(BUILDDIR)/squake/r_edgea.o \
	$(BUILDDIR)/squake/r_varsa.o \
	$(BUILDDIR)/squake/surf16.o \
	$(BUILDDIR)/squake/surf8.o \
	$(BUILDDIR)/squake/worlda.o \
	$(BUILDDIR)/squake/r_aclipa.o \
	$(BUILDDIR)/squake/snd_mixa.o \
	$(BUILDDIR)/squake/sys_dosa.o

$(BUILDDIR)/bin/squake : $(SQUAKE_OBJS)
	$(CC) $(CPPFLAGS) -o $@ $(SQUAKE_OBJS) $(SVGALDFLAGS) $(LDFLAGS)

####

$(BUILDDIR)/squake/cl_demo.o :  ./cl_demo.cpp
	$(DO_CC)

$(BUILDDIR)/squake/cl_input.o : ./cl_input.cpp
	$(DO_CC)

$(BUILDDIR)/squake/cl_main.o :  ./cl_main.cpp
	$(DO_CC)

$(BUILDDIR)/squake/cl_parse.o : ./cl_parse.cpp
	$(DO_CC)

$(BUILDDIR)/squake/cl_tent.o :  ./cl_tent.cpp
	$(DO_CC)

$(BUILDDIR)/squake/cfgfile.o :  ./cfgfile.cpp
	$(DO_CC)

$(BUILDDIR)/squake/chase.o :    ./chase.cpp
	$(DO_CC)

$(BUILDDIR)/squake/cmd.o :      ./cmd.cpp
	$(DO_CC)

$(BUILDDIR)/squake/common.o :   ./common.cpp
	$(DO_DEBUG_CC)

$(BUILDDIR)/squake/console.o :  ./console.cpp
	$(DO_CC)

$(BUILDDIR)/squake/crc.o :      ./crc.cpp
	$(DO_CC)

$(BUILDDIR)/squake/cvar.o :     ./cvar.cpp
	$(DO_CC)

$(BUILDDIR)/squake/draw.o :     ./draw.cpp
	$(DO_CC)

$(BUILDDIR)/squake/d_edge.o :   ./d_edge.cpp
	$(DO_CC)

$(BUILDDIR)/squake/d_fill.o :   ./d_fill.cpp
	$(DO_CC)

$(BUILDDIR)/squake/d_init.o :   ./d_init.cpp
	$(DO_CC)

$(BUILDDIR)/squake/d_modech.o : ./d_modech.cpp
	$(DO_CC)

$(BUILDDIR)/squake/d_part.o :   ./d_part.cpp
	$(DO_CC)

$(BUILDDIR)/squake/d_polyse.o : ./d_polyse.cpp
	$(DO_CC)

$(BUILDDIR)/squake/d_scan.o :   ./d_scan.cpp
	$(DO_CC)

$(BUILDDIR)/squake/d_sky.o :    ./d_sky.cpp
	$(DO_CC)

$(BUILDDIR)/squake/d_sprite.o : ./d_sprite.cpp
	$(DO_CC)

$(BUILDDIR)/squake/d_surf.o :   ./d_surf.cpp
	$(DO_CC)

$(BUILDDIR)/squake/d_vars.o :   ./d_vars.cpp
	$(DO_CC)

$(BUILDDIR)/squake/d_zpoint.o : ./d_zpoint.cpp
	$(DO_CC)

$(BUILDDIR)/squake/host.o :     ./host.cpp
	$(DO_CC)

$(BUILDDIR)/squake/host_cmd.o : ./host_cmd.cpp
	$(DO_CC)

$(BUILDDIR)/squake/keys.o :     ./keys.cpp
	$(DO_CC)

$(BUILDDIR)/squake/menu.o :     ./menu.cpp
	$(DO_CC)

$(BUILDDIR)/squake/mathlib.o :  ./mathlib.cpp
	$(DO_CC)

$(BUILDDIR)/squake/model.o :    ./model.cpp
	$(DO_CC)

$(BUILDDIR)/squake/net_dgrm.o : ./net_dgrm.cpp
	$(DO_CC)

$(BUILDDIR)/squake/net_loop.o : ./net_loop.cpp
	$(DO_CC)

$(BUILDDIR)/squake/net_main.o : ./net_main.cpp
	$(DO_CC)

$(BUILDDIR)/squake/net_vcr.o :  ./net_vcr.cpp
	$(DO_CC)

$(BUILDDIR)/squake/net_udp.o :  ./net_udp.cpp
	$(DO_CC)

$(BUILDDIR)/squake/net_bsd.o :  ./net_bsd.cpp
	$(DO_CC)

$(BUILDDIR)/squake/nonintel.o : ./nonintel.cpp
	$(DO_CC)

$(BUILDDIR)/squake/pr_cmds.o :  ./pr_cmds.cpp
	$(DO_CC)

$(BUILDDIR)/squake/pr_edict.o : ./pr_edict.cpp
	$(DO_CC)

$(BUILDDIR)/squake/pr_exec.o :  ./pr_exec.cpp
	$(DO_CC)

$(BUILDDIR)/squake/r_aclip.o :  ./r_aclip.cpp
	$(DO_CC)

$(BUILDDIR)/squake/r_alias.o :  ./r_alias.cpp
	$(DO_CC)

$(BUILDDIR)/squake/r_bsp.o :    ./r_bsp.cpp
	$(DO_CC)

$(BUILDDIR)/squake/r_light.o :  ./r_light.cpp
	$(DO_CC)

$(BUILDDIR)/squake/r_draw.o :   ./r_draw.cpp
	$(DO_CC)

$(BUILDDIR)/squake/r_efrag.o :  ./r_efrag.cpp
	$(DO_CC)

$(BUILDDIR)/squake/r_edge.o :   ./r_edge.cpp
	$(DO_CC)

$(BUILDDIR)/squake/r_misc.o :   ./r_misc.cpp
	$(DO_CC)

$(BUILDDIR)/squake/r_main.o :   ./r_main.cpp
	$(DO_CC)

$(BUILDDIR)/squake/r_sky.o :    ./r_sky.cpp
	$(DO_CC)

$(BUILDDIR)/squake/r_sprite.o : ./r_sprite.cpp
	$(DO_CC)

$(BUILDDIR)/squake/r_surf.o :   ./r_surf.cpp
	$(DO_CC)

$(BUILDDIR)/squake/r_part.o :   ./r_part.cpp
	$(DO_CC)

$(BUILDDIR)/squake/r_vars.o :   ./r_vars.cpp
	$(DO_CC)

$(BUILDDIR)/squake/screen.o :   ./screen.cpp
	$(DO_CC)

$(BUILDDIR)/squake/sbar.o :     ./sbar.cpp
	$(DO_CC)

$(BUILDDIR)/squake/sv_main.o :  ./sv_main.cpp
	$(DO_CC)

$(BUILDDIR)/squake/sv_phys.o :  ./sv_phys.cpp
	$(DO_CC)

$(BUILDDIR)/squake/sv_move.o :  ./sv_move.cpp
	$(DO_CC)

$(BUILDDIR)/squake/sv_user.o :  ./sv_user.cpp
	$(DO_CC)

$(BUILDDIR)/squake/zone.o	:   ./zone.cpp
	$(DO_CC)

$(BUILDDIR)/squake/view.o	:   ./view.cpp
	$(DO_CC)

$(BUILDDIR)/squake/wad.o :      ./wad.cpp
	$(DO_CC)

$(BUILDDIR)/squake/world.o :    ./world.cpp
	$(DO_CC)

$(BUILDDIR)/squake/cd_linux.o : ./cd_linux.cpp
	$(DO_CC)

$(BUILDDIR)/squake/sys_linux.o :./sys_linux.cpp
	$(DO_CC)

$(BUILDDIR)/squake/vid_svgalib.o:./vid_svgalib.cpp
	$(DO_O_CC)

$(BUILDDIR)/squake/snd_dma.o :  ./snd_dma.cpp
	$(DO_CC)

$(BUILDDIR)/squake/snd_mem.o :  ./snd_mem.cpp
	$(DO_CC)

$(BUILDDIR)/squake/snd_mix.o :  ./snd_mix.cpp
	$(DO_CC)

$(BUILDDIR)/squake/snd_linux.o :./snd_linux.cpp
	$(DO_CC)

#####

$(BUILDDIR)/squake/d_copy.o :   ./d_copy.s
	$(DO_AS)

$(BUILDDIR)/squake/d_draw.o :   ./d_draw.s
	$(DO_AS)

$(BUILDDIR)/squake/d_draw16.o : ./d_draw16.s
	$(DO_AS)

$(BUILDDIR)/squake/d_parta.o :  ./d_parta.s
	$(DO_AS)

$(BUILDDIR)/squake/d_polysa.o : ./d_polysa.s
	$(DO_AS)

$(BUILDDIR)/squake/d_scana.o :  ./d_scana.s
	$(DO_AS)

$(BUILDDIR)/squake/d_spr8.o :   ./d_spr8.s
	$(DO_AS)

$(BUILDDIR)/squake/d_varsa.o :  ./d_varsa.s
	$(DO_AS)

$(BUILDDIR)/squake/math.o :     ./math.s
	$(DO_AS)

$(BUILDDIR)/squake/r_aliasa.o : ./r_aliasa.s
	$(DO_AS)

$(BUILDDIR)/squake/r_drawa.o :  ./r_drawa.s
	$(DO_AS)

$(BUILDDIR)/squake/r_edgea.o :  ./r_edgea.s
	$(DO_AS)

$(BUILDDIR)/squake/r_varsa.o :  ./r_varsa.s
	$(DO_AS)

$(BUILDDIR)/squake/surf16.o :   ./surf16.s
	$(DO_AS)

$(BUILDDIR)/squake/surf8.o :    ./surf8.s
	$(DO_AS)

$(BUILDDIR)/squake/worlda.o :   ./worlda.s
	$(DO_AS)

$(BUILDDIR)/squake/r_aclipa.o : ./r_aclipa.s
	$(DO_AS)

$(BUILDDIR)/squake/snd_mixa.o : ./snd_mixa.s
	$(DO_AS)

$(BUILDDIR)/squake/sys_dosa.o : ./sys_dosa.s
	$(DO_AS)

#############################################################################
# GLQuake
#############################################################################

GLQUAKE_OBJS= \
	$(BUILDDIR)/bananaquakeopengl/bgmusic.o \
	$(BUILDDIR)/bananaquakeopengl/cl_demo.o \
	$(BUILDDIR)/bananaquakeopengl/cl_input.o \
	$(BUILDDIR)/bananaquakeopengl/cl_main.o \
	$(BUILDDIR)/bananaquakeopengl/cl_parse.o \
	$(BUILDDIR)/bananaquakeopengl/cl_tent.o \
	$(BUILDDIR)/bananaquakeopengl/cfgfile.o \
	$(BUILDDIR)/bananaquakeopengl/chase.o \
	$(BUILDDIR)/bananaquakeopengl/cmd.o \
	$(BUILDDIR)/bananaquakeopengl/common.o \
	$(BUILDDIR)/bananaquakeopengl/console.o \
	$(BUILDDIR)/bananaquakeopengl/crc.o \
	$(BUILDDIR)/bananaquakeopengl/cvar.o \
	\
	$(BUILDDIR)/bananaquakeopengl/gl_draw.o \
	$(BUILDDIR)/bananaquakeopengl/gl_images.o \
	$(BUILDDIR)/bananaquakeopengl/gl_mesh.o \
	$(BUILDDIR)/bananaquakeopengl/gl_model.o \
	$(BUILDDIR)/bananaquakeopengl/gl_refrag.o \
	$(BUILDDIR)/bananaquakeopengl/gl_rlight.o \
	$(BUILDDIR)/bananaquakeopengl/gl_rmain.o \
	$(BUILDDIR)/bananaquakeopengl/gl_rmisc.o \
	$(BUILDDIR)/bananaquakeopengl/gl_rsurf.o \
	$(BUILDDIR)/bananaquakeopengl/gl_screen.o \
	$(BUILDDIR)/bananaquakeopengl/gl_test.o \
	$(BUILDDIR)/bananaquakeopengl/gl_vidlinuxglx.o \
	$(BUILDDIR)/bananaquakeopengl/gl_warp.o \
	\
	$(BUILDDIR)/bananaquakeopengl/host.o \
	$(BUILDDIR)/bananaquakeopengl/host_cmd.o \
	$(BUILDDIR)/bananaquakeopengl/keys.o \
	$(BUILDDIR)/bananaquakeopengl/menu.o \
	$(BUILDDIR)/bananaquakeopengl/mathlib.o \
	$(BUILDDIR)/bananaquakeopengl/net_dgrm.o \
	$(BUILDDIR)/bananaquakeopengl/net_loop.o \
	$(BUILDDIR)/bananaquakeopengl/net_main.o \
	$(BUILDDIR)/bananaquakeopengl/net_vcr.o \
	$(BUILDDIR)/bananaquakeopengl/net_udp.o \
	$(BUILDDIR)/bananaquakeopengl/net_bsd.o \
	$(BUILDDIR)/bananaquakeopengl/pr_cmds.o \
	$(BUILDDIR)/bananaquakeopengl/pr_edict.o \
	$(BUILDDIR)/bananaquakeopengl/pr_exec.o \
	$(BUILDDIR)/bananaquakeopengl/r_part.o \
	$(BUILDDIR)/bananaquakeopengl/sbar.o \
	$(BUILDDIR)/bananaquakeopengl/strlcat.o \
	$(BUILDDIR)/bananaquakeopengl/strlcpy.o \
	$(BUILDDIR)/bananaquakeopengl/sv_main.o \
	$(BUILDDIR)/bananaquakeopengl/sv_phys.o \
	$(BUILDDIR)/bananaquakeopengl/sv_move.o \
	$(BUILDDIR)/bananaquakeopengl/sv_user.o \
	$(BUILDDIR)/bananaquakeopengl/zone.o	\
	$(BUILDDIR)/bananaquakeopengl/view.o	\
	$(BUILDDIR)/bananaquakeopengl/wad.o \
	$(BUILDDIR)/bananaquakeopengl/world.o \
	$(BUILDDIR)/bananaquakeopengl/cd_linux.o \
	$(BUILDDIR)/bananaquakeopengl/sys_linux.o \
	$(BUILDDIR)/bananaquakeopengl/snd_codec.o \
	$(BUILDDIR)/bananaquakeopengl/snd_dma.o \
	$(BUILDDIR)/bananaquakeopengl/snd_mem.o \
	$(BUILDDIR)/bananaquakeopengl/snd_mix.o \
	$(BUILDDIR)/bananaquakeopengl/snd_linux.o \
	$(BUILDDIR)/bananaquakeopengl/snd_mp3.o \
	$(BUILDDIR)/bananaquakeopengl/snd_mp3tag.o \
	$(BUILDDIR)/bananaquakeopengl/snd_wave.o \
	$(BUILDDIR)/bananaquakeopengl/snd_vorbis.o \
	\
	$(BUILDDIR)/bananaquakeopengl/math.o \
	$(BUILDDIR)/bananaquakeopengl/worlda.o \
	$(BUILDDIR)/bananaquakeopengl/snd_mixa.o \
	$(BUILDDIR)/bananaquakeopengl/utils.o

# GLSVGA_OBJS=$(BUILDDIR)/bananaquakeopengl/gl_vidlinux.o

# GLX_OBJS=$(BUILDDIR)/bananaquakeopengl/gl_vidlinuxglx.o

# $(GLSVGA_OBJS)

$(BUILDDIR)/bin/bananaquakeopengl : $(GLQUAKE_OBJS)
	$(CC) $(GLCPPFLAGS) -o $@ $(GLQUAKE_OBJS) $(LDFLAGS) $(CODECLIBS) $(GLLIBINCLUDE) $(GLLIBLINK)

$(BUILDDIR)/bananaquakeopengl/bgmusic.o :  	./bgmusic.cpp
	$(DO_GL_CC)	
	
$(BUILDDIR)/bananaquakeopengl/cfgfile.o :  	./cfgfile.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/cl_demo.o :      ./cl_demo.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/cl_input.o :     ./cl_input.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/cl_main.o :      ./cl_main.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/cl_parse.o :     ./cl_parse.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/cl_tent.o :      ./cl_tent.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/chase.o :        ./chase.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/cmd.o :          ./cmd.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/common.o :       ./common.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/console.o :      ./console.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/crc.o :          ./crc.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/cvar.o :         ./cvar.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/gl_draw.o :      ./gl_draw.cpp
	$(DO_GL_CC)
	
$(BUILDDIR)/bananaquakeopengl/gl_images.o :      ./gl_images.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/gl_mesh.o :      ./gl_mesh.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/gl_model.o :     ./gl_model.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/gl_refrag.o :    ./gl_refrag.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/gl_rlight.o :    ./gl_rlight.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/gl_rmain.o :     ./gl_rmain.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/gl_rmisc.o :     ./gl_rmisc.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/gl_rsurf.o :     ./gl_rsurf.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/gl_screen.o :    ./gl_screen.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/gl_test.o :      ./gl_test.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/gl_vidlinuxglx.o :  ./gl_vidlinuxglx.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/gl_warp.o :      ./gl_warp.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/host.o :         ./host.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/host_cmd.o :     ./host_cmd.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/keys.o :         ./keys.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/menu.o :         ./menu.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/mathlib.o :      ./mathlib.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/net_dgrm.o :     ./net_dgrm.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/net_loop.o :     ./net_loop.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/net_main.o :     ./net_main.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/net_vcr.o :      ./net_vcr.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/net_udp.o :      ./net_udp.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/net_bsd.o :      ./net_bsd.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/pr_cmds.o :      ./pr_cmds.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/pr_edict.o :     ./pr_edict.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/pr_exec.o :      ./pr_exec.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/r_part.o :       ./r_part.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/sbar.o :         ./sbar.cpp
	$(DO_GL_CC)
	
$(BUILDDIR)/bananaquakeopengl/snd_codec.o :         ./snd_codec.cpp
	$(DO_GL_CC)
	
$(BUILDDIR)/bananaquakeopengl/snd_mp3.o :         ./snd_mp3.cpp
	$(DO_GL_CC)
	
$(BUILDDIR)/bananaquakeopengl/snd_mp3tag.o :         ./snd_mp3tag.cpp
	$(DO_GL_CC)
	
$(BUILDDIR)/bananaquakeopengl/snd_wave.o :         ./snd_wave.cpp
	$(DO_GL_CC)
	
	
$(BUILDDIR)/bananaquakeopengl/snd_vorbis.o :         ./snd_vorbis.cpp
	$(DO_GL_CC)
	
$(BUILDDIR)/bananaquakeopengl/strlcat.o :         ./strlcat.cpp
	$(DO_GL_CC)
	
$(BUILDDIR)/bananaquakeopengl/strlcpy.o :         ./strlcpy.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/sv_main.o :      ./sv_main.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/sv_phys.o :      ./sv_phys.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/sv_move.o :      ./sv_move.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/sv_user.o :      ./sv_user.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/zone.o	:        ./zone.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/view.o	:        ./view.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/wad.o :          ./wad.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/world.o :        ./world.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/cd_linux.o :     ./cd_linux.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/sys_linux.o :    ./sys_linux.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/snd_dma.o :      ./snd_dma.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/snd_mem.o :      ./snd_mem.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/snd_mix.o :      ./snd_mix.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/snd_linux.o :    ./snd_linux.cpp
	$(DO_GL_CC)
	
$(BUILDDIR)/bananaquakeopengl/utils.o :    ./utils.cpp
	$(DO_GL_CC)

$(BUILDDIR)/bananaquakeopengl/math.o :         ./math.s
	$(DO_GL_AS)

$(BUILDDIR)/bananaquakeopengl/worlda.o :       ./worlda.s
	$(DO_GL_AS)

$(BUILDDIR)/bananaquakeopengl/snd_mixa.o :     ./snd_mixa.s
	$(DO_GL_AS)

#############################################################################
# RPM
#############################################################################

# Make RPMs.  You need to be root to make this work
RPMROOT=/usr/src/redhat
RPM = rpm
RPMFLAGS = -bb
INSTALLDIR = /usr/local/games/quake
TMPDIR = /var/tmp
RPMDIR = $(TMPDIR)/quake-$(VERSION)
BASERPMDIR = $(TMPDIR)/quake-$(BASEVERSION)

rpm: rpm-quake rpm-quake-data rpm-hipnotic rpm-rogue

rpm-quake: quake.spec \
		$(BUILD_RELEASE_DIR)/bin/squake \
		$(BUILD_RELEASE_DIR)/bin/quake.x11 \
		$(BUILD_RELEASE_DIR)/bin/bananaquakeopengl \
		$(BUILD_RELEASE_DIR)/bin/bananaquakeopengl.glx \
		$(BUILD_RELEASE_DIR)/bin/bananaquakeopengl.3dfxgl
	touch $(RPMROOT)/SOURCES/quake-$(VERSION).tar.gz
	if [ ! -d RPMS ];then mkdir RPMS;fi
	cp $(MOUNT_DIR)/quake.gif $(RPMROOT)/SOURCES/quake.gif

	# basic binaries rpm
	-mkdirhier $(RPMDIR)/$(INSTALLDIR)
	cp $(MOUNT_DIR)/docs/README $(RPMDIR)/$(INSTALLDIR)/.
	cp $(BUILD_RELEASE_DIR)/bin/squake $(RPMDIR)/$(INSTALLDIR)/squake
	strip $(RPMDIR)/$(INSTALLDIR)/squake
	cp $(BUILD_RELEASE_DIR)/bin/quake.x11 $(RPMDIR)/$(INSTALLDIR)/quake.x11
	strip $(RPMDIR)/$(INSTALLDIR)/quake.x11
	cp $(BUILD_RELEASE_DIR)/bin/bananaquakeopengl $(RPMDIR)/$(INSTALLDIR)/bananaquakeopengl
	strip $(RPMDIR)/$(INSTALLDIR)/bananaquakeopengl
	cp $(BUILD_RELEASE_DIR)/bin/bananaquakeopengl.glx $(RPMDIR)/$(INSTALLDIR)/bananaquakeopengl.glx
	strip $(RPMDIR)/$(INSTALLDIR)/bananaquakeopengl.glx
	cp $(BUILD_RELEASE_DIR)/bin/bananaquakeopengl.3dfxgl $(RPMDIR)/$(INSTALLDIR)/bananaquakeopengl.3dfxgl
	strip $(RPMDIR)/$(INSTALLDIR)/bananaquakeopengl.3dfxgl
	-mkdirhier $(RPMDIR)/usr/lib
	cp $(TDFXGL_DIR)/release$(ARCH)$(LIBC++)/lib3dfxgl.so $(RPMDIR)/usr/lib/lib3dfxgl.so
	cp $(MESA_DIR)/lib/libMesaGL.so.2.6 $(RPMDIR)/usr/lib/libMesaGL.so.2.6

	cp quake.spec $(RPMROOT)/SPECS/.
	cd $(RPMROOT)/SPECS; $(RPM) $(RPMFLAGS) quake.spec
	rm -rf $(RPMDIR)
	rm -f $(RPMROOT)/SOURCES/quake-$(VERSION).tar.gz

	mv $(RPMROOT)/RPMS/$(ARCH)/quake-$(VERSION)-$(RPM_RELEASE).$(ARCH).rpm RPMS/.

QUAKEDATADIR=$(TMPDIR)/quake-data-$(BASEVERSION)
rpm-quake-data: quake-data.spec
	# data rpm
	touch $(RPMROOT)/SOURCES/quake-$(BASEVERSION)-data.tar.gz

	-mkdirhier $(QUAKEDATADIR)/$(INSTALLDIR)/id1
	cp $(MASTER_DIR)/id1/pak0.pak $(QUAKEDATADIR)/$(INSTALLDIR)/id1/.
	cp $(MASTER_DIR)/id1/pak1.pak $(QUAKEDATADIR)/$(INSTALLDIR)/id1/.
	cp $(MOUNT_DIR)/docs/README $(QUAKEDATADIR)/$(INSTALLDIR)/.
	cp $(MOUNT_DIR)/data/comexp.txt $(QUAKEDATADIR)/$(INSTALLDIR)/.
	cp $(MOUNT_DIR)/data/help.txt $(QUAKEDATADIR)/$(INSTALLDIR)/.
	cp $(MOUNT_DIR)/data/licinfo.txt $(QUAKEDATADIR)/$(INSTALLDIR)/.
	cp $(MOUNT_DIR)/data/manual.txt $(QUAKEDATADIR)/$(INSTALLDIR)/.
	cp $(MOUNT_DIR)/data/readme.txt $(QUAKEDATADIR)/$(INSTALLDIR)/.
	cp $(MOUNT_DIR)/data/rlicnse.txt $(QUAKEDATADIR)/$(INSTALLDIR)/.
	cp $(MOUNT_DIR)/data/slicnse.txt $(QUAKEDATADIR)/$(INSTALLDIR)/.
	cp $(MOUNT_DIR)/data/techinfo.txt $(QUAKEDATADIR)/$(INSTALLDIR)/.
	cp quake-data.spec $(RPMROOT)/SPECS/.
	cd $(RPMROOT)/SPECS; $(RPM) $(RPMFLAGS) quake-data.spec
	rm -rf $(QUAKEDATADIR)
	rm -f $(RPMROOT)/SOURCES/quake-$(BASEVERSION)-data.tar.gz

	mv $(RPMROOT)/RPMS/$(NOARCH)/quake-data-$(BASEVERSION)-$(RPM_RELEASE).$(NOARCH).rpm RPMS/.

RPMHIPNOTICDIR=$(TMPDIR)/quake-hipnotic-$(BASEVERSION)
rpm-hipnotic: quake-hipnotic.spec
	touch $(RPMROOT)/SOURCES/quake-hipnotic-$(BASEVERSION).tar.gz
	if [ ! -d RPMS ];then mkdir RPMS;fi
	cp $(MOUNT_DIR)/quake.gif $(RPMROOT)/SOURCES/quake.gif
	-mkdirhier $(RPMHIPNOTICDIR)/$(INSTALLDIR)/hipnotic/docs
	cp $(MASTER_DIR)/hipnotic/pak0.pak $(RPMHIPNOTICDIR)/$(INSTALLDIR)/hipnotic/.
	cp $(MASTER_DIR)/hipnotic/config.cppfg $(RPMHIPNOTICDIR)/$(INSTALLDIR)/hipnotic/.
	cp $(MASTER_DIR)/hipnotic/docs/manual.doc $(RPMHIPNOTICDIR)/$(INSTALLDIR)/hipnotic/docs/.
	cp $(MASTER_DIR)/hipnotic/docs/manual.htm $(RPMHIPNOTICDIR)/$(INSTALLDIR)/hipnotic/docs/.
	cp $(MASTER_DIR)/hipnotic/docs/manual.txt $(RPMHIPNOTICDIR)/$(INSTALLDIR)/hipnotic/docs/.
	cp $(MASTER_DIR)/hipnotic/docs/readme.doc $(RPMHIPNOTICDIR)/$(INSTALLDIR)/hipnotic/docs/.
	cp $(MASTER_DIR)/hipnotic/docs/readme.htm $(RPMHIPNOTICDIR)/$(INSTALLDIR)/hipnotic/docs/.
	cp $(MASTER_DIR)/hipnotic/docs/readme.txt $(RPMHIPNOTICDIR)/$(INSTALLDIR)/hipnotic/docs/.
	cp quake-hipnotic.spec $(RPMROOT)/SPECS/.
	cd $(RPMROOT)/SPECS; $(RPM) $(RPMFLAGS) quake-hipnotic.spec
	rm -rf $(RPMHIPNOTICDIR)
	rm -f $(RPMROOT)/SOURCES/quake-hipnotic-$(BASEVERSION).tar.gz

	mv $(RPMROOT)/RPMS/$(NOARCH)/quake-hipnotic-$(BASEVERSION)-$(RPM_RELEASE).$(NOARCH).rpm RPMS/.

RPMROGUEDIR=$(TMPDIR)/quake-rogue-$(BASEVERSION)
rpm-rogue: quake-rogue.spec
	touch $(RPMROOT)/SOURCES/quake-rogue-$(BASEVERSION).tar.gz
	if [ ! -d RPMS ];then mkdir RPMS;fi
	cp $(MOUNT_DIR)/quake.gif $(RPMROOT)/SOURCES/quake.gif
	-mkdirhier $(RPMROGUEDIR)/$(INSTALLDIR)/rogue/docs
	cp $(MASTER_DIR)/rogue/pak0.pak $(RPMROGUEDIR)/$(INSTALLDIR)/rogue/.
	cp $(MASTER_DIR)/rogue/docs/manual.doc $(RPMROGUEDIR)/$(INSTALLDIR)/rogue/docs/.
	cp $(MASTER_DIR)/rogue/docs/manual.htm $(RPMROGUEDIR)/$(INSTALLDIR)/rogue/docs/.
	cp $(MASTER_DIR)/rogue/docs/manual.txt $(RPMROGUEDIR)/$(INSTALLDIR)/rogue/docs/.
	cp $(MASTER_DIR)/rogue/docs/readme.doc $(RPMROGUEDIR)/$(INSTALLDIR)/rogue/docs/.
	cp $(MASTER_DIR)/rogue/docs/readme.htm $(RPMROGUEDIR)/$(INSTALLDIR)/rogue/docs/.
	cp $(MASTER_DIR)/rogue/docs/readme.txt $(RPMROGUEDIR)/$(INSTALLDIR)/rogue/docs/.
	cp $(MASTER_DIR)/rogue/docs/ctf.doc $(RPMROGUEDIR)/$(INSTALLDIR)/rogue/docs/.
	cp $(MASTER_DIR)/rogue/docs/ctf.htm $(RPMROGUEDIR)/$(INSTALLDIR)/rogue/docs/.
	cp $(MASTER_DIR)/rogue/docs/ctf.txt $(RPMROGUEDIR)/$(INSTALLDIR)/rogue/docs/.
	cp quake-rogue.spec $(RPMROOT)/SPECS/.
	cd $(RPMROOT)/SPECS; $(RPM) $(RPMFLAGS) quake-rogue.spec
	rm -rf $(RPMROGUEDIR)
	rm -f $(RPMROOT)/SOURCES/quake-rogue-$(BASEVERSION).tar.gz

	mv $(RPMROOT)/RPMS/$(NOARCH)/quake-rogue-$(BASEVERSION)-$(RPM_RELEASE).$(NOARCH).rpm RPMS/.

quake.spec : $(MOUNT_DIR)/quake.spec.sh
	sh $< $(VERSION) $(RPM_RELEASE) $(INSTALLDIR) > $@

quake-data.spec : $(MOUNT_DIR)/quake-data.spec.sh
	sh $< $(BASEVERSION) $(RPM_RELEASE) $(INSTALLDIR) > $@

quake-hipnotic.spec : $(MOUNT_DIR)/quake-hipnotic.spec.sh
	sh $< $(BASEVERSION) $(RPM_RELEASE) $(INSTALLDIR) > $@

quake-rogue.spec : $(MOUNT_DIR)/quake-rogue.spec.sh
	sh $< $(BASEVERSION) $(RPM_RELEASE) $(INSTALLDIR) > $@

#############################################################################
# MISC
#############################################################################

clean: clean-debug clean-release
	rm -f squake.spec bananaquakeopengl.spec quake.x11.spec

clean-debug:
	$(MAKE) clean2 BUILDDIR=$(BUILD_DEBUG_DIR) CPPFLAGS="$(DEBUG_CPPFLAGS)"
	$(MAKE) clean -C ./sdl2
	$(MAKE) clean -C ./SDL_image

clean-release:
	$(MAKE) clean2 BUILDDIR=$(BUILD_RELEASE_DIR) CPPFLAGS="$(DEBUG_CPPFLAGS)"
	$(MAKE) clean -C ./sdl2
	$(MAKE) clean -C ./SDL_image

clean2:
	-rm -f $(SQUAKE_OBJS) $(X11_OBJS) $(GLQUAKE_OBJS) $(GLSVGA_OBJS) \
		$(GLX_OBJS)

