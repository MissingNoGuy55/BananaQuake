#
# Quake Makefile for Linux 2.0
#
# Aug '98 by Zoid <zoid@idsoftware.cppom>
#
# ELF only
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

ifneq (,$(findstring alpha,$(shell uname -m)))
ARCH=axp
else
ARCH=i386
endif
NOARCH=noarch

MOUNT_DIR=/QuakeCPP/WinQuake
MASTER_DIR=/QuakeCPP/QuakeMaster
MESA_DIR=/usr/local/src/Mesa-2.6
TDFXGL_DIR = /home/zoid/3dfxgl

BUILD_DEBUG_DIR=debug$(ARCH)$(LIBC++)
BUILD_RELEASE_DIR=release$(ARCH)$(LIBC++)

EGCS=/bin/gcc
CC=$(EGCS)

BASE_CPPFLAGS=-Dstricmp=strcasecmp
RELEASE_CPPFLAGS=$(BASE_CPPFLAGS) -g -mpentiumpro -O6 -ffast-math -funroll-loops \
	-fomit-frame-pointer -fexpensive-optimizations
DEBUG_CPPFLAGS=$(BASE_CPPFLAGS) -g
LDFLAGS=-lm
SVGALDFLAGS=-lvga
XLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext -lXxf86dga
XCPPFLAGS=-DX11

GLLDFLAGS=-L/usr/X11/lib -L/usr/local/lib -lGL -lX11 -lXext -ldl -lXxf86dga -lXxf86vm -lm
GLCPPFLAGS=-DGLQUAKE -I$(MESA_DIR)/include -I/usr/include/glide -I/usr/include/SDL2

DO_CC=$(CC) $(CPPFLAGS) -o $@ -c $<
DO_DEBUG_CC=$(CC) $(DEBUG_CPPFLAGS) -o $@ -c $<
DO_GL_CC=$(CC) $(CPPFLAGS) $(GLCPPFLAGS) -o $@ -c $<
DO_GL_DEBUG_CC=$(CC) $(DEBUG_CPPFLAGS) $(GLCPPFLAGS) -o $@ -c $<
DO_O_CC=$(CC) -O $(CPPFLAGS) -o $@ -c $<
DO_AS=$(CC) $(CPPFLAGS) -DELF -x assembler-with-cpp -o $@ -c $<
DO_GL_AS=$(CC) $(CPPFLAGS) $(GLCPPFLAGS) -DELF -x assembler-with-cpp -o $@ -c $<

#############################################################################
# SETUP AND BUILD
#############################################################################

TARGETS=$(BUILDDIR)/bin/glquake
	# $(BUILDDIR)/bin/unixded

build_debug:
	@-mkdir $(BUILD_DEBUG_DIR) \
		$(BUILD_DEBUG_DIR)/bin \
		$(BUILD_DEBUG_DIR)/glquake \
		# $(BUILD_DEBUG_DIR)/squake \
		$(BUILD_DEBUG_DIR)/unixded \
		$(BUILD_DEBUG_DIR)/x11
	$(MAKE) targets BUILDDIR=$(BUILD_DEBUG_DIR) CPPFLAGS="$(DEBUG_CPPFLAGS)"

build_release:
	@-mkdir $(BUILD_RELEASE_DIR) \
		$(BUILD_RELEASE_DIR)/bin \
		$(BUILD_RELEASE_DIR)/glquake \
		# $(BUILD_RELEASE_DIR)/squake \
		$(BUILD_RELEASE_DIR)/unixded \
		$(BUILD_RELEASE_DIR)/x11
	$(MAKE) targets BUILDDIR=$(BUILD_RELEASE_DIR) CPPFLAGS="$(RELEASE_CPPFLAGS)"

all: build_debug build_release

targets: $(TARGETS)

#############################################################################
# SVGALIB Quake
#############################################################################

SQUAKE_OBJS = \
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
	$(BUILDDIR)/glquake/cl_demo.o \
	$(BUILDDIR)/glquake/cl_input.o \
	$(BUILDDIR)/glquake/cl_main.o \
	$(BUILDDIR)/glquake/cl_parse.o \
	$(BUILDDIR)/glquake/cl_tent.o \
	$(BUILDDIR)/glquake/chase.o \
	$(BUILDDIR)/glquake/cmd.o \
	$(BUILDDIR)/glquake/common.o \
	$(BUILDDIR)/glquake/console.o \
	$(BUILDDIR)/glquake/crc.o \
	$(BUILDDIR)/glquake/cvar.o \
	\
	$(BUILDDIR)/glquake/gl_draw.o \
	$(BUILDDIR)/glquake/gl_mesh.o \
	$(BUILDDIR)/glquake/gl_model.o \
	$(BUILDDIR)/glquake/gl_refrag.o \
	$(BUILDDIR)/glquake/gl_rlight.o \
	$(BUILDDIR)/glquake/gl_rmain.o \
	$(BUILDDIR)/glquake/gl_rmisc.o \
	$(BUILDDIR)/glquake/gl_rsurf.o \
	$(BUILDDIR)/glquake/gl_screen.o \
	$(BUILDDIR)/glquake/gl_test.o \
	$(BUILDDIR)/glquake/gl_warp.o \
	\
	$(BUILDDIR)/glquake/host.o \
	$(BUILDDIR)/glquake/host_cmd.o \
	$(BUILDDIR)/glquake/keys.o \
	$(BUILDDIR)/glquake/menu.o \
	$(BUILDDIR)/glquake/mathlib.o \
	$(BUILDDIR)/glquake/net_dgrm.o \
	$(BUILDDIR)/glquake/net_loop.o \
	$(BUILDDIR)/glquake/net_main.o \
	$(BUILDDIR)/glquake/net_vcr.o \
	$(BUILDDIR)/glquake/net_udp.o \
	$(BUILDDIR)/glquake/net_bsd.o \
	$(BUILDDIR)/glquake/pr_cmds.o \
	$(BUILDDIR)/glquake/pr_edict.o \
	$(BUILDDIR)/glquake/pr_exec.o \
	$(BUILDDIR)/glquake/r_part.o \
	$(BUILDDIR)/glquake/sbar.o \
	$(BUILDDIR)/glquake/sv_main.o \
	$(BUILDDIR)/glquake/sv_phys.o \
	$(BUILDDIR)/glquake/sv_move.o \
	$(BUILDDIR)/glquake/sv_user.o \
	$(BUILDDIR)/glquake/zone.o	\
	$(BUILDDIR)/glquake/view.o	\
	$(BUILDDIR)/glquake/wad.o \
	$(BUILDDIR)/glquake/world.o \
	$(BUILDDIR)/glquake/cd_linux.o \
	$(BUILDDIR)/glquake/sys_linux.o \
	$(BUILDDIR)/glquake/snd_dma.o \
	$(BUILDDIR)/glquake/snd_mem.o \
	$(BUILDDIR)/glquake/snd_mix.o \
	$(BUILDDIR)/glquake/snd_linux.o \
	\
	$(BUILDDIR)/glquake/math.o \
	$(BUILDDIR)/glquake/worlda.o \
	$(BUILDDIR)/glquake/snd_mixa.o \
	$(BUILDDIR)/glquake/sys_dosa.o \
	$(BUILDDIR)/glquake/utils.o

GLSVGA_OBJS=$(BUILDDIR)/glquake/gl_vidlinux.o

GLX_OBJS=$(BUILDDIR)/glquake/gl_vidlinuxglx.o

$(BUILDDIR)/bin/glquake : $(GLQUAKE_OBJS) $(GLSVGA_OBJS)
	$(CC) $(CPPFLAGS) -o $@ $(GLQUAKE_OBJS) $(GLSVGA_OBJS) $(SVGALDFLAGS) $(LDFLAGS)

$(BUILDDIR)/glquake/cl_demo.o :      ./cl_demo.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/cl_input.o :     ./cl_input.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/cl_main.o :      ./cl_main.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/cl_parse.o :     ./cl_parse.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/cl_tent.o :      ./cl_tent.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/chase.o :        ./chase.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/cmd.o :          ./cmd.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/common.o :       ./common.cpp
	$(DO_GL_DEBUG_CC)

$(BUILDDIR)/glquake/console.o :      ./console.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/crc.o :          ./crc.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/cvar.o :         ./cvar.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/gl_draw.o :      ./gl_draw.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/gl_mesh.o :      ./gl_mesh.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/gl_model.o :     ./gl_model.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/gl_refrag.o :    ./gl_refrag.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/gl_rlight.o :    ./gl_rlight.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/gl_rmain.o :     ./gl_rmain.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/gl_rmisc.o :     ./gl_rmisc.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/gl_rsurf.o :     ./gl_rsurf.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/gl_screen.o :    ./gl_screen.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/gl_test.o :      ./gl_test.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/gl_vidlinux.o :  ./gl_vidlinux.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/gl_vidlinuxglx.o :  ./gl_vidlinuxglx.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/gl_warp.o :      ./gl_warp.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/host.o :         ./host.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/host_cmd.o :     ./host_cmd.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/keys.o :         ./keys.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/menu.o :         ./menu.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/mathlib.o :      ./mathlib.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/net_dgrm.o :     ./net_dgrm.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/net_loop.o :     ./net_loop.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/net_main.o :     ./net_main.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/net_vcr.o :      ./net_vcr.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/net_udp.o :      ./net_udp.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/net_bsd.o :      ./net_bsd.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/pr_cmds.o :      ./pr_cmds.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/pr_edict.o :     ./pr_edict.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/pr_exec.o :      ./pr_exec.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/r_part.o :       ./r_part.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/sbar.o :         ./sbar.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/sv_main.o :      ./sv_main.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/sv_phys.o :      ./sv_phys.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/sv_move.o :      ./sv_move.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/sv_user.o :      ./sv_user.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/zone.o	:        ./zone.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/view.o	:        ./view.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/wad.o :          ./wad.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/world.o :        ./world.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/cd_linux.o :     ./cd_linux.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/sys_linux.o :    ./sys_linux.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/snd_dma.o :      ./snd_dma.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/snd_mem.o :      ./snd_mem.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/snd_mix.o :      ./snd_mix.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/snd_linux.o :    ./snd_linux.cpp
	$(DO_GL_CC)
	
$(BUILDDIR)/glquake/utils.o :    ./utils.cpp
	$(DO_GL_CC)

$(BUILDDIR)/glquake/math.o :         ./math.s
	$(DO_GL_AS)

$(BUILDDIR)/glquake/worlda.o :       ./worlda.s
	$(DO_GL_AS)

$(BUILDDIR)/glquake/snd_mixa.o :     ./snd_mixa.s
	$(DO_GL_AS)

$(BUILDDIR)/glquake/sys_dosa.o :     ./sys_dosa.s
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
		$(BUILD_RELEASE_DIR)/bin/glquake \
		$(BUILD_RELEASE_DIR)/bin/glquake.glx \
		$(BUILD_RELEASE_DIR)/bin/glquake.3dfxgl
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
	cp $(BUILD_RELEASE_DIR)/bin/glquake $(RPMDIR)/$(INSTALLDIR)/glquake
	strip $(RPMDIR)/$(INSTALLDIR)/glquake
	cp $(BUILD_RELEASE_DIR)/bin/glquake.glx $(RPMDIR)/$(INSTALLDIR)/glquake.glx
	strip $(RPMDIR)/$(INSTALLDIR)/glquake.glx
	cp $(BUILD_RELEASE_DIR)/bin/glquake.3dfxgl $(RPMDIR)/$(INSTALLDIR)/glquake.3dfxgl
	strip $(RPMDIR)/$(INSTALLDIR)/glquake.3dfxgl
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
	rm -f squake.spec glquake.spec quake.x11.spec

clean-debug:
	$(MAKE) clean2 BUILDDIR=$(BUILD_DEBUG_DIR) CPPFLAGS="$(DEBUG_CPPFLAGS)"

clean-release:
	$(MAKE) clean2 BUILDDIR=$(BUILD_RELEASE_DIR) CPPFLAGS="$(DEBUG_CPPFLAGS)"

clean2:
	-rm -f $(SQUAKE_OBJS) $(X11_OBJS) $(GLQUAKE_OBJS) $(GLSVGA_OBJS) \
		$(GLX_OBJS)

