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
${COMMON_DIR}/BSPFILE.cpp
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
# SOURCE GROUPS
#===================================================================

add_executable(qbsp ${QBSP_FILES})
add_executable(qvis ${QVIS_FILES})
add_executable(qrad ${QRAD_FILES})
add_executable(qcc ${QCC_FILES})
add_executable(qfiles ${QFILES_FILES})

target_include_directories(qbsp PUBLIC
    "${includepathQBSP}"
)

target_include_directories(qvis PUBLIC
    "${includepathQVIS}"
)

target_include_directories(qrad PUBLIC
    "${includepathQVIS}"
)

target_include_directories(qcc PUBLIC
    "${includepathQVIS}"
)

target_include_directories(qfiles PUBLIC
    "${includepathQFILES}"
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
