cmake_minimum_required(VERSION 3.10)

# add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/sdl2 ${CMAKE_CURRENT_SOURCE_DIR}/sdl2/lib)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/sdl2/build ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(CMAKE_CURRENT_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build")

set (SDL2_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sdl2/include)

# Create an option to switch between a system sdl library and a vendored sdl library
option(MYGAME_VENDORED "Use vendored libraries" ON)

if(MYGAME_VENDORED)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/sdl2 EXCLUDE_FROM_ALL)
else()
    # 1. Look for a SDL2 package, 2. look for the SDL2 component and 3. fail if none can be found
    find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)

	if (UNIX)

		# X11
		find_package(X11 REQUIRED CONFIG REQUIRED COMPONENTS X11)
		find_package(Xext REQUIRED CONFIG REQUIRED COMPONENTS X11)

                find_package(vorbis REQUIRED CONFIG REQUIRED COMPONENTS vorbis)

	endif (UNIX)

    # 1. Look for a SDL2 package, 2. Look for the SDL2maincomponent and 3. DO NOT fail when SDL2main is not available
    find_package(SDL2Main REQUIRED CONFIG COMPONENTS SDL2)
endif()

enable_language(CXX ASM)

project(bananaquake LANGUAGES CXX ASM)

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED true)

message("${CPPFLAGS}")

SET(ASM_OPTIONS "-DELF -m32 -x assembler-with-cpp")
SET(CMAKE_ASM_FLAGS "${CFLAGS} ${ASM_OPTIONS}" )

#=================================================================

if (WIN32)
set(WINQUAKE_SOURCES d_copy.s
d_draw.s
d_draw16.s
d_parta.s
d_polysa.s
d_scana.s
d_spr8.s
d_varsa.s
math.s
r_aliasa.s
r_drawa.s
r_edgea.s
r_varsa.s
surf16.s
surf8.s
worlda.s
r_aclipa.s
snd_mixa.s
sys_dosa.s

bgmusic.cpp
cd_win.cpp
chase.cpp
cl_demo.cpp
cl_input.cpp
cl_main.cpp
cl_parse.cpp
cl_tent.cpp
cmd.cpp
common.cpp
conproc.cpp
console.cpp
crc.cpp
cvar.cpp
d_edge.cpp
d_fill.cpp
d_init.cpp
d_modech.cpp
d_part.cpp
d_polyse.cpp
d_scan.cpp
d_sky.cpp
d_sprite.cpp
d_surf.cpp
d_vars.cpp
d_zpoint.cpp
draw.cpp
host.cpp
host_cmd.cpp
in_win.cpp
keys.cpp
mathlib.cpp
menu.cpp
model.cpp
net_dgrm.cpp
net_loop.cpp
net_main.cpp
net_vcr.cpp
net_win.cpp
net_wins.cpp
net_wipx.cpp
pr_cmds.cpp
pr_edict.cpp
pr_exec.cpp
r_aclip.cpp
r_alias.cpp
r_bsp.cpp
r_draw.cpp
r_edge.cpp
r_efrag.cpp
r_light.cpp
r_main.cpp
r_misc.cpp
r_part.cpp
r_sky.cpp
r_sprite.cpp
r_surf.cpp
r_vars.cpp
sbar.cpp
screen.cpp
server.cpp
snd_codec.cpp
snd_dma.cpp
snd_mem.cpp
snd_mp3.cpp
snd_mp3tag.cpp
snd_mix.cpp
snd_vorbis.cpp
snd_wave.cpp
snd_win.cpp
strlcat.cpp
strlcpy.cpp
sv_main.cpp
sv_move.cpp
sv_phys.cpp
sv_user.cpp
sys_win.cpp
utils.cpp
vid_win.cpp
view.cpp
wad.cpp
world.cpp
zone.cpp
)

set(WINQUAKE_HEADERS anorm_dots.h
anorms.h
asm_draw.h
asm_i386.h
bgmusic.h
BSPFILE.H
cdaudio.h
client.h
cmd.h
common.h
conproc.h
console.h
crc.h
cvar.h
d_iface.h
d_local.h
dosisms.h
draw.h
host.h
in_win.h
input.h
keys.h
mathlib.h
menu.h
model.h
modelgen.h
net.h
net_dgrm.h
net_loop.h
net_ser.h
net_vcr.h
net_wins.h
pr_cmds.h
pr_comp.h
progdefs.h
progs.h
protocol.h
q_ctype.h
quakeasm.h
quakedef.h
r_local.h
r_part.h
r_shared.h
render.h
screen.h
server.h
snd_codec.h
snd_codeci.h
snd_mp3.h
snd_vorbis.h
snd_wave.h
sound.h
spritegn.h
strl_fn.h
sys.h
sys_win.h
utils.h
vid.h
vid_win.h
view.h
wad.h
winquake.h
world.h
zone.h
)

set(GLQUAKE_SOURCES
math.s
worlda.s
snd_mixa.s

bgmusic.cpp
cd_win.cpp
chase.cpp
cl_demo.cpp
cl_input.cpp
cl_main.cpp
cl_parse.cpp
cl_tent.cpp
cmd.cpp
common.cpp
conproc.cpp
console.cpp
crc.cpp
cvar.cpp
gl_draw.cpp
gl_mesh.cpp
gl_model.cpp
gl_refrag.cpp
gl_rlight.cpp
gl_rmain.cpp
gl_rmisc.cpp
gl_rsurf.cpp
gl_screen.cpp
gl_test.cpp
gl_vidnt.cpp
gl_warp.cpp
host.cpp
host_cmd.cpp
in_win.cpp
keys.cpp
mathlib.cpp
menu.cpp
net_dgrm.cpp
net_loop.cpp
net_main.cpp
net_vcr.cpp
net_win.cpp
net_wins.cpp
net_wipx.cpp
pr_cmds.cpp
pr_edict.cpp
pr_exec.cpp
r_part.cpp
sbar.cpp
server.cpp
snd_codec.cpp
snd_dma.cpp
snd_mem.cpp
snd_mp3.cpp
snd_mp3tag.cpp
snd_mix.cpp
snd_vorbis.cpp
snd_wave.cpp
snd_win.cpp
strlcat.cpp
strlcpy.cpp
sv_main.cpp
sv_move.cpp
sv_phys.cpp
sv_user.cpp
sys_win.cpp
utils.cpp
view.cpp
wad.cpp
world.cpp
zone.cpp
)

set(GLQUAKE_HEADERS anorm_dots.h
anorms.h
asm_draw.h
asm_i386.h
bgmusic.h
BSPFILE.H
cdaudio.h
client.h
cmd.h
common.h
conproc.h
console.h
crc.h
cvar.h
d_iface.h
d_local.h
dosisms.h
draw.h
glquake.h
host.h
in_win.h
input.h
keys.h
mathlib.h
menu.h
model.h
modelgen.h
net.h
net_dgrm.h
net_loop.h
net_ser.h
net_vcr.h
net_wins.h
pr_cmds.h
pr_comp.h
progdefs.h
progs.h
protocol.h
q_ctype.h
quakeasm.h
quakedef.h
r_local.h
r_part.h
r_shared.h
render.h
screen.h
server.h
snd_codec.h
snd_codeci.h
snd_mp3.h
snd_vorbis.h
snd_wave.h
sound.h
spritegn.h
strl_fn.h
sys.h
sys_win.h
utils.h
vid.h
vid_win.h
view.h
wad.h
winquake.h
world.h
zone.h
)
endif (WIN32)

if (UNIX)
set(WINQUAKE_SOURCES d_copy.s
d_draw.s
d_draw16.s
d_parta.s
d_polysa.s
d_scana.s
d_spr8.s
d_varsa.s
math.s
r_aliasa.s
r_drawa.s
r_edgea.s
r_varsa.s
surf16.s
surf8.s
worlda.s
r_aclipa.s
snd_mixa.s
sys_dosa.s

bgmusic.cpp
cd_linux.cpp
chase.cpp
cl_demo.cpp
cl_input.cpp
cl_main.cpp
cl_parse.cpp
cl_tent.cpp
cmd.cpp
common.cpp
console.cpp
crc.cpp
cvar.cpp
d_edge.cpp
d_fill.cpp
d_init.cpp
d_modech.cpp
d_part.cpp
d_polyse.cpp
d_scan.cpp
d_sky.cpp
d_sprite.cpp
d_surf.cpp
d_vars.cpp
d_zpoint.cpp
draw.cpp
host.cpp
host_cmd.cpp
keys.cpp
mathlib.cpp
menu.cpp
model.cpp
net_dgrm.cpp
net_loop.cpp
net_main.cpp
net_vcr.cpp
net_bsd.cpp
net_udp.cpp
pr_cmds.cpp
pr_edict.cpp
pr_exec.cpp
r_aclip.cpp
r_alias.cpp
r_bsp.cpp
r_draw.cpp
r_edge.cpp
r_efrag.cpp
r_light.cpp
r_main.cpp
r_misc.cpp
r_part.cpp
r_sky.cpp
r_sprite.cpp
r_surf.cpp
r_vars.cpp
sbar.cpp
screen.cpp
server.cpp
snd_codec.cpp
snd_dma.cpp
snd_mem.cpp
snd_mix.cpp
snd_vorbis.cpp
snd_wave.cpp
snd_linux.cpp
strlcat.cpp
strlcpy.cpp
sv_main.cpp
sv_move.cpp
sv_phys.cpp
sv_user.cpp
sys_linux.cpp
utils.cpp
view.cpp
vid_sunx.cpp
wad.cpp
world.cpp
zone.cpp
)

set(WINQUAKE_HEADERS anorm_dots.h
anorms.h
asm_draw.h
asm_i386.h
bgmusic.h
BSPFILE.H
cdaudio.h
client.h
cmd.h
common.h
conproc.h
console.h
crc.h
cvar.h
d_iface.h
d_local.h
dosisms.h
draw.h
host.h
input.h
keys.h
mathlib.h
menu.h
model.h
modelgen.h
net.h
net_dgrm.h
net_loop.h
net_ser.h
net_vcr.h
net_udp.h
pr_cmds.h
pr_comp.h
progdefs.h
progs.h
protocol.h
q_ctype.h
quakeasm.h
quakedef.h
r_local.h
r_part.h
r_shared.h
render.h
screen.h
server.h
snd_codec.h
snd_codeci.h
snd_mp3.h
snd_vorbis.h
snd_wave.h
sound.h
spritegn.h
strl_fn.h
sys.h
utils.h
vid.h
view.h
wad.h
world.h
zone.h
)

set(GLQUAKE_SOURCES math.s
worlda.s
snd_mixa.s

bgmusic.cpp
cd_linux.cpp
chase.cpp
cl_demo.cpp
cl_input.cpp
cl_main.cpp
cl_parse.cpp
cl_tent.cpp
cmd.cpp
common.cpp
console.cpp
crc.cpp
cvar.cpp
gl_draw.cpp
gl_mesh.cpp
gl_model.cpp
gl_refrag.cpp
gl_rlight.cpp
gl_rmain.cpp
gl_rmisc.cpp
gl_rsurf.cpp
gl_screen.cpp
gl_test.cpp
gl_vidlinuxglx.cpp
gl_warp.cpp
host.cpp
host_cmd.cpp
keys.cpp
mathlib.cpp
menu.cpp
net_dgrm.cpp
net_loop.cpp
net_main.cpp
net_vcr.cpp
net_bsd.cpp
net_udp.cpp
pr_cmds.cpp
pr_edict.cpp
pr_exec.cpp
r_part.cpp
sbar.cpp
server.cpp
snd_codec.cpp
snd_dma.cpp
snd_mem.cpp
snd_mix.cpp
snd_vorbis.cpp
snd_wave.cpp
snd_linux.cpp
strlcat.cpp
strlcpy.cpp
sv_main.cpp
sv_move.cpp
sv_phys.cpp
sv_user.cpp
sys_linux.cpp
utils.cpp
view.cpp
wad.cpp
world.cpp
zone.cpp
)

set(GLQUAKE_HEADERS anorm_dots.h
anorms.h
asm_draw.h
asm_i386.h
bgmusic.h
BSPFILE.H
cdaudio.h
client.h
cmd.h
common.h
conproc.h
console.h
crc.h
cvar.h
d_iface.h
d_local.h
dosisms.h
draw.h
glquake.h
host.h
in_win.h
input.h
keys.h
mathlib.h
menu.h
model.h
modelgen.h
net.h
net_dgrm.h
net_loop.h
net_ser.h
net_vcr.h
net_wins.h
pr_cmds.h
pr_comp.h
progdefs.h
progs.h
protocol.h
q_ctype.h
quakeasm.h
quakedef.h
r_local.h
r_part.h
r_shared.h
render.h
screen.h
server.h
snd_codec.h
snd_codeci.h
snd_mp3.h
snd_vorbis.h
snd_wave.h
sound.h
spritegn.h
strl_fn.h
sys.h
utils.h
vid.h
vid_win.h
view.h
wad.h
winquake.h
world.h
zone.h
)

endif (UNIX)

if (APPLE)
set(WINQUAKE_SOURCES d_copy.s
d_draw.s
d_draw16.s
d_parta.s
d_polysa.s
d_scana.s
d_spr8.s
d_varsa.s
math.s
r_aliasa.s
r_drawa.s
r_edgea.s
r_varsa.s
surf16.s
surf8.s
worlda.s
r_aclipa.s
snd_mixa.s
sys_dosa.s

bgmusic.cpp
cd_macos.cpp
chase.cpp
cl_demo.cpp
cl_input.cpp
cl_main.cpp
cl_parse.cpp
cl_tent.cpp
cmd.cpp
common.cpp
console.cpp
crc.cpp
cvar.cpp
d_edge.cpp
d_fill.cpp
d_init.cpp
d_modech.cpp
d_part.cpp
d_polyse.cpp
d_scan.cpp
d_sky.cpp
d_sprite.cpp
d_surf.cpp
d_vars.cpp
d_zpoint.cpp
draw.cpp
host.cpp
host_cmd.cpp
keys.cpp
mathlib.cpp
menu.cpp
model.cpp
net_dgrm.cpp
net_loop.cpp
net_main.cpp
net_vcr.cpp
net_bsd.cpp
net_udp.cpp
pr_cmds.cpp
pr_edict.cpp
pr_exec.cpp
r_aclip.cpp
r_alias.cpp
r_bsp.cpp
r_draw.cpp
r_edge.cpp
r_efrag.cpp
r_light.cpp
r_main.cpp
r_misc.cpp
r_part.cpp
r_sky.cpp
r_sprite.cpp
r_surf.cpp
r_vars.cpp
sbar.cpp
screen.cpp
server.cpp
snd_codec.cpp
snd_dma.cpp
snd_mem.cpp
snd_mix.cpp
snd_mp3.cpp
snd_mp3tag.cpp
snd_vorbis.cpp
snd_wave.cpp
snd_macos.cpp
strlcat.cpp
strlcpy.cpp
sv_main.cpp
sv_move.cpp
sv_phys.cpp
sv_user.cpp
sys_linux.cpp
utils.cpp
view.cpp
vid_macos.cpp
wad.cpp
world.cpp
zone.cpp
)

set(WINQUAKE_HEADERS anorm_dots.h
anorms.h
asm_draw.h
asm_i386.h
bgmusic.h
BSPFILE.H
cdaudio.h
client.h
cmd.h
common.h
conproc.h
console.h
crc.h
cvar.h
d_iface.h
d_local.h
dosisms.h
draw.h
host.h
input.h
keys.h
mathlib.h
menu.h
model.h
modelgen.h
net.h
net_dgrm.h
net_loop.h
net_ser.h
net_vcr.h
net_udp.h
pr_cmds.h
pr_comp.h
progdefs.h
progs.h
protocol.h
q_ctype.h
quakeasm.h
quakedef.h
r_local.h
r_part.h
r_shared.h
render.h
screen.h
server.h
snd_codec.h
snd_codeci.h
snd_mp3.h
snd_vorbis.h
snd_wave.h
sound.h
spritegn.h
strl_fn.h
sys.h
utils.h
vid.h
view.h
wad.h
world.h
zone.h
)

endif (APPLE)

#=================================================================

list(APPEND includepathWinQuake
    "${CMAKE_CURRENT_SOURCE_DIR}/sdl2/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/codecs/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/scitech/INCLUDE"
)

if (WIN32)
list(APPEND includepathLibraryWinQuake
    "${CMAKE_CURRENT_SOURCE_DIR}/codecs/x86"
    "${CMAKE_CURRENT_SOURCE_DIR}/scitech/lib/win32/vc"
    "${CMAKE_CURRENT_BUILD_DIR}/sdl2/Debug"
)
endif (WIN32)

if (UNIX)
list(APPEND includepathLibraryWinQuake
    "${CMAKE_CURRENT_SOURCE_DIR}/codecs/x86"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdl2/include"
    "/usr/local/lib"
)
endif (UNIX)

if (APPLE)
list(APPEND includepathLibraryWinQuake
    "${CMAKE_CURRENT_SOURCE_DIR}/codecs/x86"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdl2/include"
)
endif (APPLE)

#=================================================================

if (WIN32)
add_executable(bananaquakesoftware WIN32 ${WINQUAKE_SOURCES} ${WINQUAKE_HEADERS})
add_executable(bananaquakeopengl WIN32 ${GLQUAKE_SOURCES} ${GLQUAKE_HEADERS})
endif (WIN32)

if (UNIX)
add_executable(bananaquakesoftware ${WINQUAKE_SOURCES} ${WINQUAKE_HEADERS})
add_executable(bananaquakeopengl ${GLQUAKE_SOURCES} ${GLQUAKE_HEADERS})
endif(UNIX)

if (APPLE)
add_executable(bananaquakesoftware ${WINQUAKE_SOURCES} ${WINQUAKE_HEADERS})
add_executable(bananaquakeopengl ${GLQUAKE_SOURCES} ${GLQUAKE_HEADERS})
endif(APPLE)

#=================================================================

if (UNIX)
target_compile_definitions(bananaquakesoftware PUBLIC LINUX)
endif (UNIX)
target_compile_definitions(bananaquakesoftware PUBLIC MGL40_COMPAT)
target_compile_definitions(bananaquakesoftware PUBLIC USE_CODEC_VORBIS)
target_compile_definitions(bananaquakesoftware PUBLIC USE_CODEC_WAVE)
if (WIN32)
target_compile_definitions(bananaquakesoftware PUBLIC USE_CODEC_MP3)
endif (WIN32)
target_compile_definitions(bananaquakesoftware PUBLIC DEBUG)
target_compile_definitions(bananaquakesoftware PUBLIC _DEBUG)

target_include_directories(bananaquakesoftware PUBLIC
    "${includepathWinQuake}"
)

target_compile_definitions(bananaquakeopengl PUBLIC GLQUAKE)
target_compile_definitions(bananaquakeopengl PUBLIC USE_CODEC_VORBIS)
target_compile_definitions(bananaquakeopengl PUBLIC USE_CODEC_WAVE)
if (WIN32)
target_compile_definitions(bananaquakeopengl PUBLIC USE_CODEC_MP3)
endif (WIN32)
target_compile_definitions(bananaquakeopengl PUBLIC LINUX)
target_compile_definitions(bananaquakeopengl PUBLIC DEBUG)
target_compile_definitions(bananaquakeopengl PUBLIC _DEBUG)

target_include_directories(bananaquakeopengl PUBLIC
    "${includepathWinQuake}"
)

#=================================================================

target_link_directories(bananaquakesoftware PUBLIC ${includepathLibraryWinQuake})
target_link_directories(bananaquakeopengl PUBLIC ${includepathLibraryWinQuake})

#=================================================================

if (WIN32)

target_link_libraries(bananaquakesoftware mgllt.lib
SDL2d.lib
libvorbisfile.lib
libvorbis.lib
libopusfile.lib
libopus.lib
libFLAC.lib
libogg.lib
libmad.lib
libxmp.lib
winmm.lib
wsock32.lib
opengl32.lib
glu32.lib
odbc32.lib
odbccp32.lib)

target_link_libraries(bananaquakeopengl SDL2d.lib
libvorbisfile.lib
libvorbis.lib
libopusfile.lib
libopus.lib
libFLAC.lib
libogg.lib
libmad.lib
libxmp.lib
comctl32.lib
winmm.lib
wsock32.lib
opengl32.lib
glu32.lib
odbc32.lib
odbccp32.lib)

endif (WIN32)

if (WIN32)
target_link_options(bananaquakesoftware PUBLIC "$<$<CXX_COMPILER_ID:MSVC>:-SAFESEH:NO>")
endif (WIN32)

if (MSVC)
SET_TARGET_PROPERTIES(bananaquakesoftware PROPERTIES
LINK_FLAGS_DEBUG
"/NODEFAULTLIB:libc.lib")

SET_TARGET_PROPERTIES(bananaquakeopengl PROPERTIES
LINK_FLAGS_DEBUG
"/NODEFAULTLIB:libc.lib")
endif (MSVC)

# SDL2::SDL2main may or may not be available. It is e.g. required by Windows GUI applications
if(TARGET SDL2::SDL2main)
    # It has an implicit dependency on SDL2 functions, so it MUST be added before SDL2::SDL2 (or SDL2::SDL2-static)
    target_link_libraries(bananaquakesoftware SDL2::SDL2main)
endif()

# Link to the actual SDL2 library. SDL2::SDL2 is the shared SDL library, SDL2::SDL2-static is the static SDL libarary.
target_link_libraries(bananaquakesoftware SDL2::SDL2)

if(TARGET SDL2::SDL2main)
    target_link_libraries(bananaquakeopengl SDL2::SDL2main)
endif()

target_link_libraries(bananaquakeopengl SDL2::SDL2)

if (UNIX)
target_link_libraries(bananaquakesoftware m)
target_link_libraries(bananaquakesoftware X11)
target_link_libraries(bananaquakesoftware Xext)
target_link_libraries(bananaquakesoftware Xxf86dga)

target_link_libraries(bananaquakeopengl X11)
target_link_libraries(bananaquakeopengl Xext)
target_link_libraries(bananaquakeopengl Xxf86dga)
target_link_libraries(bananaquakeopengl Xxf86vm)

target_link_libraries(bananaquakeopengl m)
target_link_libraries(bananaquakeopengl dl)
target_link_libraries(bananaquakeopengl vorbis)
target_link_libraries(bananaquakeopengl vorbisfile)

target_link_libraries(bananaquakeopengl GL)

SET(ASM_OPTIONS "-DELF -x assembler-with-cpp")
SET(CMAKE_ASM_FLAGS "${CFLAGS} ${ASM_OPTIONS}" )

target_compile_options(bananaquakesoftware PUBLIC "-Dstricmp=strcasecmp -g")
target_compile_options(bananaquakeopengl PUBLIC "-Dstricmp=strcasecmp -g")

# target_compile_options(bananaquakesoftware PUBLIC ${BQUAKE_COMPILE_FLAGS})
# target_link_options(bananaquakesoftware PUBLIC ${BQUAKE_LINKER_FLAGS})

endif (UNIX)

if (APPLE)

target_link_libraries(bananaquakesoftware GL)

endif (APPLE)
