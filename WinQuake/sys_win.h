#pragma once

#define	MAX_HANDLES		10
extern FILE* sys_handles[MAX_HANDLES];

int VID_ForceUnlockedAndReturnState();
void VID_ForceLockState(int lk);

#ifndef QUAKE_GAME
#include "zone.h"
#endif

//double		pfreq;
//double		curtime = 0.0;
//double		lastcurtime = 0.0;
//int			lowshift;
//bool			isDedicated;
//bool		sc_return_on_enter = false;

template<typename T>
#ifndef WIN64
int Sys_FileRead(int handle, T* dest, int count)
#else
int Sys_FileRead(int handle, T* dest, size_t count)
#endif
{
	int		t, x;

	t = VID_ForceUnlockedAndReturnState();
	x = fread(dest, 1, count, sys_handles[handle]);
	VID_ForceLockState(t);
	return x;
}

/*
void MaskExceptions(void);
void Sys_Init(void);
void Sys_InitFloatTime(void);
void Sys_PushFPCW_SetHigh(void);
void Sys_PopFPCW(void);
void Sys_AtExit(void);
*/