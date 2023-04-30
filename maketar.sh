#!/bin/bash

find ./ -name "*.lib" -o -name "*.dll" -o -name "*.a" -o -name "*.q1" -o -name "*.q2" -o -name "*.cpp" -o -name "*.h" -o -name "*.H" -o -name "*.c" -o -name "*.s" -o -name "*.sln" -o -name "*.dsp" -o -name "*.dsw" -o -name "*.vcxproj" -o -name "*Makefile" -o -name "*Makefile.linuxi386" -o -name "*.Solaris" -o -name "*.sh" -o -name "*.bat" -o -name "*.gif" -o -name "*.ico" -o -name "*.txt" -o -name "*.rc" -o -name "*.cmake" -name "*.cmake.in" | tar cafv bananaquake_src.tar.gz -T -
