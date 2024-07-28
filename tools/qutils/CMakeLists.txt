cmake_minimum_required(VERSION 3.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/sdl2/build ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(CMAKE_CURRENT_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build")

enable_language(CXX ASM)

project(qutils LANGUAGES CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED true)

SET(ASM_OPTIONS "-DELF -x assembler-with-cpp")
SET(CMAKE_ASM_FLAGS "${CFLAGS} ${ASM_OPTIONS}" )

#===================================================================
# COMMON
#===================================================================

set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/COMMON)

file(GLOB_RECURSE CMN_SOURCES
${COMMON_DIR}/CMDLIB.cpp
${COMMON_DIR}/LBMLIB.cpp
${COMMON_DIR}/MATHLIB.cpp
${COMMON_DIR}/TRILIB.cpp
)

file(GLOB_RECURSE CMN_HEADERS
${COMMON_DIR}/*.h
)

set(CMN_FILES ${CMN_SOURCES} ${CMN_HEADERS})

#===================================================================
# QBSP
#===================================================================

set(QBSP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/QBSP)

file(GLOB_RECURSE QBSP_SOURCES
${COMMON_DIR}/BSPFILE.cpp
${QBSP_DIR}/*.cpp
)

file(GLOB_RECURSE QBSP_HEADERS
${QBSP_DIR}/*.h
)

set(QBSP_FILES ${QBSP_SOURCES} 
${QBSP_HEADERS} 
${CMN_SOURCES}
${CMN_HEADERS})

list(APPEND includepathQBSP
	"${CMAKE_CURRENT_SOURCE_DIR}"
)

#===================================================================
# QVIS
#===================================================================

set(QVIS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/VIS)

file(GLOB_RECURSE QVIS_SOURCES
${COMMON_DIR}/BSPFILE.cpp
${QVIS_DIR}/*.cpp
)

file(GLOB_RECURSE QVIS_HEADERS
${QVIS_DIR}/*.h
)

set(QVIS_FILES
${QVIS_SOURCES}
${QVIS_HEADERS}
${CMN_SOURCES}
${CMN_HEADERS}
)


list(APPEND includepathQVIS
	"${CMAKE_CURRENT_SOURCE_DIR}"
)

#===================================================================
# QRAD
#===================================================================

set(QRAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/LIGHT)

file(GLOB_RECURSE QRAD_SOURCES
${COMMON_DIR}/BSPFILE.cpp
${QRAD_DIR}/*.cpp
)

file(GLOB_RECURSE QRAD_HEADERS
${QRAD_DIR}/*.h
)

set(QRAD_FILES
${QRAD_SOURCES}
${QRAD_HEADERS}
${CMN_SOURCES}
${CMN_HEADERS}
)


list(APPEND includepathQRAD
	"${CMAKE_CURRENT_SOURCE_DIR}"
)

#===================================================================
# QCC
#===================================================================

set(QCC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/QCC)

file(GLOB_RECURSE QCC_SOURCES
${COMMON_DIR}/BSPFILE.cpp
${QCC_DIR}/*.cpp
)

file(GLOB_RECURSE QCC_HEADERS
${QCC_DIR}/*.h
)

set(QCC_FILES
${QCC_SOURCES}
${QCC_HEADERS}
${CMN_SOURCES}
${CMN_HEADERS}
)


list(APPEND includepathQCC
	"${CMAKE_CURRENT_SOURCE_DIR}"
)

#===================================================================
# QFILES
#===================================================================

set(QFILES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/QFILES)

file(GLOB_RECURSE QFILES_SOURCES
${COMMON_DIR}/BSPFILE.cpp
${QFILES_DIR}/*.cpp
)

file(GLOB_RECURSE QFILES_HEADERS
${QFILES_DIR}/*.h
)

set(QFILES_FILES
${QFILES_SOURCES}
${QFILES_HEADERS}
${CMN_SOURCES}
${CMN_HEADERS}
)


list(APPEND includepathQFILES
	"${CMAKE_CURRENT_SOURCE_DIR}"
)

#===================================================================
# MODELGEN
#===================================================================

set(MODELGEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/MODELGEN)

file(GLOB_RECURSE MODELGEN_SOURCES
${COMMON_DIR}/SCRIPLIB.cpp
${MODELGEN_DIR}/*.cpp
)

file(GLOB_RECURSE MODELGEN_HEADERS
${MODELGEN_DIR}/*.h
)

set(MODELGEN_FILES
${MODELGEN_SOURCES}
${MODELGEN_HEADERS}
${CMN_SOURCES}
${CMN_HEADERS}
)

list(APPEND includepathMODELGEN
	"${CMAKE_CURRENT_SOURCE_DIR}"
)

#===================================================================
# QLUMPY
#===================================================================

set(QLUMPY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/QLUMPY)

file(GLOB_RECURSE QLUMPY_SOURCES
${COMMON_DIR}/WADFILE.cpp
${COMMON_DIR}/WADLIB.cpp
${COMMON_DIR}/SCRIPLIB.cpp
${QLUMPY_DIR}/*.cpp
)

file(GLOB_RECURSE QLUMPY_HEADERS
${COMMON_DIR}/WADFILE.h
${COMMON_DIR}/WADLIB.h
${COMMON_DIR}/SCRIPLIB.h
${QLUMPY_DIR}/*.h
)

set(QLUMPY_FILES
${QLUMPY_SOURCES}
${QLUMPY_HEADERS}
${CMN_SOURCES}
${CMN_HEADERS}
)

list(APPEND includepathQLUMPY
	"${CMAKE_CURRENT_SOURCE_DIR}"
)

#===================================================================
# SPRGEN
#===================================================================

set(SPRGEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SPRGEN)

file(GLOB_RECURSE SPRGEN_SOURCES
${COMMON_DIR}/LBMLIB.cpp
${COMMON_DIR}/SCRIPLIB.cpp
${SPRGEN_DIR}/*.cpp
)

file(GLOB_RECURSE SPRGEN_HEADERS
${COMMON_DIR}/LBMLIB.h
${COMMON_DIR}/SCRIPLIB.h
${SPRGEN_DIR}/*.h
)

set(SPRGEN_FILES
${SPRGEN_SOURCES}
${SPRGEN_HEADERS}
${CMN_SOURCES}
${CMN_HEADERS}
)

list(APPEND includepathSPRGEN
	"${CMAKE_CURRENT_SOURCE_DIR}"
)

#===================================================================
# BSPINFO
#===================================================================

set(BSPINFO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/BSPINFO)

file(GLOB_RECURSE BSPINFO_SOURCES
${COMMON_DIR}/BSPFILE.cpp
${BSPINFO_DIR}/*.cpp
)

file(GLOB_RECURSE BSPINFO_HEADERS
${BSPINFO_DIR}/*.h
)

set(BSPINFO_FILES
${BSPINFO_SOURCES}
${BSPINFO_HEADERS}
${CMN_SOURCES}
${CMN_HEADERS}
)

list(APPEND includepathBSPINFO
	"${CMAKE_CURRENT_SOURCE_DIR}"
)

#===================================================================
# TEXMAKE
#===================================================================

set(TEXMAKE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/TEXMAKE)

file(GLOB_RECURSE TEXMAKE_SOURCES
${TEXMAKE_DIR}/*.cpp
)

file(GLOB_RECURSE TEXMAKE_HEADERS
${TEXMAKE_DIR}/*.h
)

set(TEXMAKE_FILES
${TEXMAKE_SOURCES}
${TEXMAKE_HEADERS}
${CMN_SOURCES}
${CMN_HEADERS}
)

list(APPEND includepathTEXMAKE
	"${CMAKE_CURRENT_SOURCE_DIR}"
)

#===================================================================
# SOURCE GROUPS
#===================================================================

add_executable(qbsp ${QBSP_FILES})
add_executable(qvis ${QVIS_FILES})
add_executable(qrad ${QRAD_FILES})
add_executable(qcc ${QCC_FILES})
add_executable(qfiles ${QFILES_FILES})
add_executable(qlumpy ${QLUMPY_FILES})
add_executable(modelgen ${MODELGEN_FILES})
add_executable(bspinfo ${BSPINFO_FILES})
add_executable(texmake ${TEXMAKE_FILES})
add_executable(sprgen ${SPRGEN_FILES})

target_include_directories(qbsp PUBLIC
    "${includepathQBSP}"
)

target_include_directories(qvis PUBLIC
    "${includepathQVIS}"
)

target_include_directories(qrad PUBLIC
    "${includepathQRAD}"
)

target_include_directories(qcc PUBLIC
    "${includepathQCC}"
)

target_include_directories(qfiles PUBLIC
    "${includepathQFILES}"
)

target_include_directories(qlumpy PUBLIC
    "${includepathQLUMPY}"
)

target_include_directories(modelgen PUBLIC
    "${includepathMODELGEN}"
)

target_include_directories(bspinfo PUBLIC
    "${includepathBSPINFO}"
)

target_include_directories(texmake PUBLIC
    "${includepathTEXMAKE}"
)

target_include_directories(sprgen PUBLIC
    "${includepathSPRGEN}"
)

add_custom_command(TARGET qcc 
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:qcc> "${CMAKE_CURRENT_SOURCE_DIR}/../../../WinQuake/id1"
)

add_custom_command(TARGET qbsp 
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:qbsp> "${CMAKE_CURRENT_SOURCE_DIR}/../../../WinQuake/id1"
)

add_custom_command(TARGET qvis 
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:qvis> "${CMAKE_CURRENT_SOURCE_DIR}/../../../WinQuake/id1"
)

add_custom_command(TARGET qrad 
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:qrad> "${CMAKE_CURRENT_SOURCE_DIR}/../../../WinQuake/id1"
)

add_custom_command(TARGET qfiles 
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:qfiles> "${CMAKE_CURRENT_SOURCE_DIR}/../../../WinQuake/id1"
)

add_custom_command(TARGET qlumpy 
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:qlumpy> "${CMAKE_CURRENT_SOURCE_DIR}/../../../WinQuake/id1"
)

add_custom_command(TARGET modelgen 
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:modelgen> "${CMAKE_CURRENT_SOURCE_DIR}/../../../WinQuake/id1"
)

add_custom_command(TARGET bspinfo 
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:bspinfo> "${CMAKE_CURRENT_SOURCE_DIR}/../../../WinQuake/id1"
)

add_custom_command(TARGET texmake 
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:texmake> "${CMAKE_CURRENT_SOURCE_DIR}/../../../WinQuake/id1"
)

add_custom_command(TARGET sprgen 
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:sprgen> "${CMAKE_CURRENT_SOURCE_DIR}/../../../WinQuake/id1"
)
