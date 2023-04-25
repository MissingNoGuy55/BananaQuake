#pragma once

#include <unistd.h>

template<typename T>
int Sys_FileRead (int handle, T *dest, size_t count)
{
    return read (handle, dest, count);
}
