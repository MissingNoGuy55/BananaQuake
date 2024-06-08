include_guard(GLOBAL)

cmake_minimum_required(VERSION 3.6)

enable_language(CXX)

project(qcc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED true)

set(WINQUAKE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../WinQuake)

set(QCC_HEADERS builtin.h
cmdlib.h
pr_comp.h
qcc.h
)

set(QCC_SOURCES builtin.cpp
cmdlib.cpp
pr_comp.cpp
pr_lex.cpp
qcc.cpp
)

add_executable(qcc ${QCC_HEADERS} ${QCC_SOURCES})

set(QCC_INCLUDE_DIRS ${WINQUAKE_DIR})

target_include_directories(qcc PUBLIC ${QCC_INCLUDE_DIRS})
target_compile_definitions(qcc PUBLIC QUAKE_TOOLS)

if(MSVC)
target_compile_definitions(qcc PUBLIC _WIN32)
target_compile_definitions(qcc PUBLIC WIN32)
endif()