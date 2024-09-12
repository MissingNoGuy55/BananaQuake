cmake_minimum_required(VERSION 3.6)

cmake_policy(SET CMP0076 NEW) # target_sources converting relative paths to absolute

# add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/sdl2 ${CMAKE_CURRENT_SOURCE_DIR}/sdl2/lib)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/sdl2/build ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(CMAKE_CURRENT_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build")
set(CMAKE_ASM_NASM_COMPILER "${CMAKE_CURRENT_SOURCE_DIR}/nasm.exe")

# Create an option to switch between a system sdl library and a vendored sdl library
option(BANANAQUAKE_VENDORED "Use vendored libraries" ON)

if(BANANAQUAKE_VENDORED)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/sdl2 EXCLUDE_FROM_ALL)
    #add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/sdl_image EXCLUDE_FROM_ALL)
else()
    # 1. Look for a SDL2 package, 2. look for the SDL2 component and 3. fail if none can be found
    find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)

		if (UNIX)
                # X11
                find_package(X11 REQUIRED CONFIG REQUIRED COMPONENTS X11)
                find_package(Xext REQUIRED CONFIG REQUIRED COMPONENTS X11)
        endif (UNIX)
                find_package(vorbis REQUIRED CONFIG REQUIRED COMPONENTS vorbis)

    # 1. Look for a SDL2 package, 2. Look for the SDL2maincomponent and 3. DO NOT fail when SDL2main is not available
    find_package(SDL2Main REQUIRED CONFIG COMPONENTS SDL2)

	# Missi: SDL_image is not required to build, however it is here for usefulness. Uncomment this and the related lines pertaining to this to use it
	#find_package(SDL_image REQUIRED CONFIG REQUIRED COMPONENTS SDL_image SDL2)
endif()

project(bananaquake LANGUAGES CXX ASM)

enable_language(CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED true)

SET(ASM_OPTIONS "-DELF -x assembler-with-cpp")
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

qe3.ico
quake.ico
winquake.rc

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
md4.cpp
menu.cpp
model.cpp
net_dgrm.cpp
net_loop.cpp
net_main.cpp
net_vcr.cpp
net_win.cpp
net_wins.cpp
net_wipx.cpp
pakstuff.cpp
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
unzip.cpp
vid_win.cpp
view.cpp
vpkfile.cpp
wad.cpp
world.cpp
zone.cpp
)

set(WINQUAKE_HEADERS anorm_dots.h
anorms.h
asm_draw.h
asm_i386.h
bgmusic.h
bspfile.h
bspfile_goldsrc.h
bspfile_source.h
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
pakstuff.h
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
str.h
sys.h
sys_win.h
utils.h
unzip.h
vid.h
vid_win.h
view.h
vpkfile.h
wad.h
winquake.h
world.h
zone.h
)

set(GLQUAKE_SOURCES
math.s
worlda.s
snd_mixa.s

qe3.ico
quake.ico
winquake.rc

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
gl_images.cpp
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
md4.cpp
menu.cpp
net_dgrm.cpp
net_loop.cpp
net_main.cpp
net_vcr.cpp
net_win.cpp
net_wins.cpp
net_wipx.cpp
pakstuff.cpp
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
unzip.cpp
view.cpp
vpkfile.cpp
wad.cpp
world.cpp
zone.cpp
)

set(GLQUAKE_HEADERS anorm_dots.h
anorms.h
asm_draw.h
asm_i386.h
bgmusic.h
bspfile.h
bspfile_goldsrc.h
bspfile_source.h
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
gl_draw.h
gl_images.h
gl_mesh.h
gl_model.h
gl_refrag.h
gl_rlight.h
gl_rmain.h
gl_rsurf.h
gl_warp.h
gl_warp_sin.h
gl_vidnt.h
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
pakstuff.h
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
str.h
sys.h
sys_win.h
utils.h
unzip.h
vid.h
vid_win.h
view.h
vpkfile.h
wad.h
winquake.h
world.h
zone.h
)
endif (WIN32)

if (UNIX)

#add_executable(bananaquakesoftware)
add_executable(bananaquakeopengl)

#[[
target_sources(bananaquakesoftware PRIVATE d_draw.s
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

qe3.ico
quake.ico
winquake.rc

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
md4.cpp
menu.cpp
model.cpp
net_dgrm.cpp
net_loop.cpp
net_main.cpp
net_vcr.cpp
net_bsd.cpp
net_udp.cpp
pakstuff.cpp
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
snd_linux.cpp
strlcat.cpp
strlcpy.cpp
sv_main.cpp
sv_move.cpp
sv_phys.cpp
sv_user.cpp
sys_linux.cpp
utils.cpp
unzip.cpp
view.cpp
vid_sdl_software.cpp
vpkfile.cpp
wad.cpp
world.cpp
zone.cpp
)

target_sources(bananaquakesoftware PUBLIC anorm_dots.h
anorms.h
asm_draw.h
asm_i386.h
bgmusic.h
bspfile.h
bspfile_goldsrc.h
bspfile_source.h
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
pakstuff.h
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
str.h
sys.h
utils.h
unzip.h
vid.h
view.h
vpkfile.h
wad.h
world.h
zone.h
)
]]
target_sources(bananaquakeopengl PRIVATE math.s
worlda.s
snd_mixa.s

qe3.ico
quake.ico
winquake.rc

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
gl_images.cpp
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
md4.cpp
menu.cpp
net_dgrm.cpp
net_loop.cpp
net_main.cpp
net_vcr.cpp
net_bsd.cpp
net_udp.cpp
pakstuff.cpp
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
snd_linux.cpp
strlcat.cpp
strlcpy.cpp
sv_main.cpp
sv_move.cpp
sv_phys.cpp
sv_user.cpp
sys_linux.cpp
utils.cpp
unzip.cpp
view.cpp
vpkfile.cpp
wad.cpp
world.cpp
zone.cpp
)

target_sources(bananaquakeopengl PUBLIC anorm_dots.h
anorms.h
asm_draw.h
asm_i386.h
bgmusic.h
bspfile.h
bspfile_goldsrc.h
bspfile_source.h
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
gl_draw.h
gl_images.h
gl_mesh.h
gl_model.h
gl_refrag.h
gl_rlight.h
gl_rmain.h
gl_rsurf.h
gl_warp.h
gl_warp_sin.h
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
pakstuff.h
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
str.h
sys.h
utils.h
unzip.h
vid.h
vid_win.h
view.h
vpkfile.h
wad.h
winquake.h
world.h
zone.h
)

set_property(SOURCE quakeasm.h PROPERTY LANGUAGE ASM)
set_property(SOURCE math.s PROPERTY LANGUAGE ASM)
set_property(SOURCE worlda.s PROPERTY LANGUAGE ASM)
set_property(SOURCE snd_mixa.s PROPERTY LANGUAGE ASM)

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
md4.cpp
menu.cpp
model.cpp
net_dgrm.cpp
net_loop.cpp
net_main.cpp
net_vcr.cpp
net_bsd.cpp
net_udp.cpp
pakstuff.cpp
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
unzip.cpp
utils.cpp
view.cpp
vid_macos.cpp
vpkfile.cpp
wad.cpp
world.cpp
zone.cpp
)

set(WINQUAKE_HEADERS anorm_dots.h
anorms.h
asm_draw.h
asm_i386.h
bgmusic.h
bspfile.h
bspfile_goldsrc.h
bspfile_source.h
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
pakstuff.h
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
str.h
sys.h
utils.h
unzip.h
vid.h
view.h
vpkfile.h
wad.h
world.h
zone.h
)

endif (APPLE)

#=================================================================

list(APPEND includepathWinQuake
    "${CMAKE_CURRENT_SOURCE_DIR}/sdl2/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/SDL_image/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/codecs/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/scitech/INCLUDE"
)

if (WIN32)

if (${CMAKE_SIZEOF_VOID_P} STREQUAL "4")

list(APPEND includepathLibraryWinQuakeSoftware
    "${CMAKE_CURRENT_SOURCE_DIR}/codecs/x86"
    "${CMAKE_CURRENT_SOURCE_DIR}/scitech/lib/win32/vc"
    "${CMAKE_CURRENT_BUILD_DIR}/sdl2"
)

list(APPEND includepathLibraryWinQuakeOpenGLx86
    "${CMAKE_CURRENT_SOURCE_DIR}/codecs/x86"
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_BUILD_DIR}/sdl2/Debug>
        $<$<CONFIG:RelWithDebInfo>:${CMAKE_CURRENT_BUILD_DIR}/sdl2/RelWithDebInfo>
        $<$<CONFIG:Release>:${CMAKE_CURRENT_BUILD_DIR}/sdl2/Release>
        $<$<CONFIG:MinSizeRel>:${CMAKE_CURRENT_BUILD_DIR}/sdl2/MinSizeRel>
    "${CMAKE_CURRENT_BUILD_DIR}/sdl2"
)

else()

list(APPEND includepathLibraryWinQuakeSoftware
    "${CMAKE_CURRENT_SOURCE_DIR}/codecs/x64"
    "${CMAKE_CURRENT_SOURCE_DIR}/scitech/lib/win32/vc"
    "${CMAKE_CURRENT_BUILD_DIR}/sdl2/Debug"
)

list(APPEND includepathLibraryWinQuakeOpenGLx64
    "${CMAKE_CURRENT_SOURCE_DIR}/codecs/x64"
    "${CMAKE_CURRENT_SOURCE_DIR}/SDL_image/include"
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_BUILD_DIR}/sdl2/Debug>
        $<$<CONFIG:RelWithDebInfo>:${CMAKE_CURRENT_BUILD_DIR}/sdl2/RelWithDebInfo>
        $<$<CONFIG:Release>:${CMAKE_CURRENT_BUILD_DIR}/sdl2/Release>
        $<$<CONFIG:MinSizeRel>:${CMAKE_CURRENT_BUILD_DIR}/sdl2/MinSizeRel>
)

endif()

endif (WIN32)

if (UNIX)
list(APPEND includepathLibraryWinQuake
    "${CMAKE_CURRENT_SOURCE_DIR}/codecs/x64"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdl2/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/SDL_image/include"
    "/usr/local/lib"
)
endif (UNIX)

if (APPLE)
list(APPEND includepathLibraryWinQuake
    "${CMAKE_CURRENT_SOURCE_DIR}/codecs/x64"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdl2/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/SDL_image/include"
)
endif (APPLE)

#=================================================================

if (WIN32)
message(PROJECT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")  
message(PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")  
#add_executable(bananaquakesoftware WIN32 ${WINQUAKE_SOURCES} ${WINQUAKE_HEADERS})
add_executable(bananaquakeopengl ${GLQUAKE_SOURCES} ${GLQUAKE_HEADERS})
add_custom_command(TARGET bananaquakeopengl 
                    POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:bananaquakeopengl> "${CMAKE_CURRENT_SOURCE_DIR}"
                    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BUILD_DIR}/sdl2/$<$<CONFIG:Debug>:Debug/SDL2d.dll>" "${CMAKE_CURRENT_SOURCE_DIR}"
                    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BUILD_DIR}/sdl2/$<$<CONFIG:Release>:Release/SDL2.dll>" "${CMAKE_CURRENT_SOURCE_DIR}"
                    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BUILD_DIR}/sdl2/$<$<CONFIG:MinSizeRel>:MinSizeRel/SDL2.dll>" "${CMAKE_CURRENT_SOURCE_DIR}"
                    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BUILD_DIR}/sdl2/$<$<CONFIG:Debug>:Debug/SDL2d.dll>" "${CMAKE_CURRENT_BUILD_DIR}/$<$<CONFIG:Debug>:Debug>"
                    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BUILD_DIR}/sdl2/$<$<CONFIG:Release>:Release/SDL2.dll>" "${CMAKE_CURRENT_BUILD_DIR}/$<$<CONFIG:Release>:Release>"
                    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BUILD_DIR}/sdl2/$<$<CONFIG:MinSizeRel>:MinSizeRel/SDL2.dll>" "${CMAKE_CURRENT_BUILD_DIR}/$<$<CONFIG:MinSizeRel>:MinSizeRel>"
                    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BUILD_DIR}/sdl2/$<$<CONFIG:RelWithDebInfo>:RelWithDebInfo/SDL2.dll>" "${CMAKE_CURRENT_BUILD_DIR}/$<$<CONFIG:RelWithDebInfo>:RelWithDebInfo>"
					#COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BUILD_DIR}/sdl_image/$<$<CONFIG:Debug>:Debug/SDL2_imaged.dll>" "${CMAKE_CURRENT_BUILD_DIR}/$<$<CONFIG:Debug>:Debug>"
                    #COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BUILD_DIR}/sdl_image/$<$<CONFIG:Release>:Release/SDL2_image.dll>" "${CMAKE_CURRENT_BUILD_DIR}/$<$<CONFIG:Release>:Release>"
                    #COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BUILD_DIR}/sdl_image/$<$<CONFIG:MinSizeRel>:MinSizeRel/SDL2_image.dll>" "${CMAKE_CURRENT_BUILD_DIR}/$<$<CONFIG:MinSizeRel>:MinSizeRel>"
                    #COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BUILD_DIR}/sdl_image/$<$<CONFIG:RelWithDebInfo>:RelWithDebInfo/SDL2_image.dll>" "${CMAKE_CURRENT_BUILD_DIR}/$<$<CONFIG:RelWithDebInfo>:RelWithDebInfo>"
)
endif (WIN32)

if (UNIX)
add_custom_command(TARGET bananaquakeopengl
                    POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:bananaquakeopengl> "${CMAKE_CURRENT_SOURCE_DIR}"
)
endif (UNIX)

if (APPLE)
# add_executable(bananaquakesoftware ${WINQUAKE_SOURCES} ${WINQUAKE_HEADERS})
add_executable(bananaquakeopengl ${GLQUAKE_SOURCES} ${GLQUAKE_HEADERS})
endif(APPLE)

#=================================================================
#[[
target_compile_definitions(bananaquakesoftware PUBLIC BQUAKE)
target_compile_definitions(bananaquakesoftware PUBLIC USE_CODEC_VORBIS)
target_compile_definitions(bananaquakesoftware PUBLIC USE_CODEC_WAVE)
target_compile_definitions(bananaquakesoftware PUBLIC USE_CODEC_MP3)
target_include_directories(bananaquakesoftware PUBLIC
    "${includepathWinQuake}"
)
]]

target_compile_definitions(bananaquakeopengl PUBLIC BQUAKE)
target_compile_definitions(bananaquakeopengl PUBLIC $<$<CONFIG:Debug>:DEBUG>)
#target_compile_definitions(bananaquakeopengl PUBLIC $<$<CONFIG:Debug>:_DEBUG>)
target_compile_definitions(bananaquakeopengl PUBLIC GLQUAKE)
target_compile_definitions(bananaquakeopengl PUBLIC USE_CODEC_VORBIS)
target_compile_definitions(bananaquakeopengl PUBLIC USE_CODEC_WAVE)
target_compile_definitions(bananaquakeopengl PUBLIC USE_CODEC_MP3)

target_include_directories(bananaquakeopengl PUBLIC
    "${includepathWinQuake}"
)

if (WIN32)

if (${CMAKE_SIZEOF_VOID_P} STREQUAL "8")
target_compile_definitions(bananaquakeopengl PUBLIC WIN64)
endif()

endif(WIN32)
#=================================================================

#target_link_directories(bananaquakesoftware PUBLIC ${includepathLibraryWinQuakeSoftware})
if(WIN32)
if (${CMAKE_SIZEOF_VOID_P} STREQUAL "4")
target_link_directories(bananaquakeopengl PUBLIC ${includepathLibraryWinQuakeOpenGLx86})
else()
target_link_directories(bananaquakeopengl PUBLIC ${includepathLibraryWinQuakeOpenGLx64})
endif()
endif(WIN32)
#=================================================================

if (MSVC)

#[[

$<$<CONFIG:Debug>:SDL2d.lib>
$<$<CONFIG:RelWithDebInfo>:SDL2.lib>
$<$<CONFIG:Release>:SDL2.lib>
$<$<CONFIG:MinSizeRel>:SDL2.lib>
$<$<CONFIG:Debug>:SDL2_imaged.lib>
$<$<CONFIG:RelWithDebInfo>:SDL2_imaged.lib>
$<$<CONFIG:Release>:SDL2_imaged.lib>
$<$<CONFIG:MinSizeRel>:SDL2_imaged.lib>

]]

target_link_libraries(bananaquakeopengl SDL2::SDL2
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

endif (MSVC)

#target_link_libraries(bananaquakesoftware SDL2::SDL2)

if (MSVC)
target_compile_options(bananaquakeopengl PRIVATE "/MP")
target_compile_options(bananaquakeopengl PRIVATE "/WX")
#target_compile_options(bananaquakeopengl PRIVATE "/fsanitize=address")
target_link_options(bananaquakeopengl PRIVATE "/SUBSYSTEM:WINDOWS")
endif (MSVC)

if (UNIX)
target_link_libraries(bananaquakeopengl X11)
target_link_libraries(bananaquakeopengl Xext)
target_link_libraries(bananaquakeopengl Xxf86dga)
target_link_libraries(bananaquakeopengl Xxf86vm)

target_link_libraries(bananaquakeopengl m)
target_link_libraries(bananaquakeopengl mad)
target_link_libraries(bananaquakeopengl dl)
target_link_libraries(bananaquakeopengl vorbis)
target_link_libraries(bananaquakeopengl vorbisfile)

target_link_libraries(bananaquakeopengl GL)

target_link_libraries(bananaquakeopengl SDL2)
target_link_libraries(bananaquakeopengl SDL2main)
#[[
target_link_libraries(bananaquakesoftware X11)
target_link_libraries(bananaquakesoftware Xext)
target_link_libraries(bananaquakesoftware Xxf86dga)
target_link_libraries(bananaquakesoftware Xxf86vm)

target_link_libraries(bananaquakesoftware m)
target_link_libraries(bananaquakesoftware mad)
target_link_libraries(bananaquakesoftware dl)
target_link_libraries(bananaquakesoftware vorbis)
target_link_libraries(bananaquakesoftware vorbisfile)

target_link_libraries(bananaquakesoftware SDL2)
target_link_libraries(bananaquakesoftware SDL2main)
target_link_libraries(bananaquakesoftware SDL2_image)
]]
# Missi: prevents "missing .note.GNU-stack section implies executable stack" warning in recent binutils on Linux
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -z noexecstack")

target_compile_options(bananaquakeopengl PRIVATE "-Dstricmp=strcasecmp -g")

#target_compile_options(bananaquakesoftware PUBLIC "-Dstricmp=strcasecmp -g")

endif (UNIX)

if (APPLE)

#target_link_libraries(bananaquakesoftware GL)

endif (APPLE)
