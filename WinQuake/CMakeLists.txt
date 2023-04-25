cmake_minimum_required(VERSION 3.10)

add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/SDL2 ${CMAKE_CURRENT_SOURCE_DIR}/SDL2/lib)

project(SDL2 C CXX)
project(BananaQuakeCMake LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CXX_STANDARD_REQUIRED true)

set(WINQUAKE_SOURCES bgmusic.cpp
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
host.cpp
host_cmd.cpp
in_win.cpp
keys.cpp
mathlib.cpp
menu.cpp
model.cpp
net_dgrm.cpp
net_loop.cpp
net_vcr.cpp
net_win.cpp
net_wins.cpp
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
BSPFILE.h
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

list(APPEND includepathWinQuake
    "${CMAKE_CURRENT_SOURCE_DIR}/sdl2/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/codecs/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/scitech/INCLUDE"
)

list(APPEND includepathLibraryWinQuake
	"${CMAKE_CURRENT_SOURCE_DIR}/codecs/x86"
)

add_executable(softwarebananaquake WIN32 ${WINQUAKE_SOURCES} ${WINQUAKE_HEADERS})

target_compile_definitions(softwarebananaquake PUBLIC LINUX)

target_include_directories(softwarebananaquake PUBLIC
    "${includepathWinQuake}"
)

target_link_directories(softwarebananaquake PUBLIC ${includepathLibraryWinQuake})

target_link_libraries(softwarebananaquake ${CMAKE_CURRENT_SOURCE_DIR}/scitech/lib/win32/vc/mgllt.lib
${CMAKE_CURRENT_SOURCE_DIR}/sdl2/lib/Debug/SDL2d.lib
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