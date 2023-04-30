#!/bin/bash

# -name "*.lib" -o -name "*.dll"

find ./ -name "*.so" -o -name "*.a" -o -name "*.q1" -o -name "*.q2" -o -name "*.cpp" -o -name "*.h" -o -name "*.H" -o -name "*.c" -o -name "*.s" -o -name "*.sln" -o -name "*.dsp" -o -name "*.dsw" -o -name "*.vcxproj" -o -name "*Makefile" -o -name "*Makefile.linuxi386" -o -name "*.Solaris" -o -name "*.sh" -o -name "*.bat" -o -name "*.gif" -o -name "*.ico" -o -name "*.txt" -o -name "*.rc" -o -name "*.cmake" -name "*.cmake.in" | tar cfv bananaquake_src.tar -T -

find ./WinQuake/sdl2 -name "*" | tar ufv bananaquake_src.tar -T -
find ./WinQuake/vorbis-svn -name "*" | tar ufv bananaquake_src.tar -T -

gzip bananaquake_src.tar
