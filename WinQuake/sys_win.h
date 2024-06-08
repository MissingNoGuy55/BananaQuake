#pragma once

#include <windows.h>

#define	MAX_HANDLES		64	// Missi: was 10 (1/10/2023)
extern FILE* sys_handles[MAX_HANDLES];

int VID_ForceUnlockedAndReturnState();
void VID_ForceLockState(int lk);

#ifndef QUAKE_GAME
#include "zone.h"

#define MINIMUM_WIN_MEMORY		0x0880000
#define MAXIMUM_WIN_MEMORY		0x1000000

#define CONSOLE_ERROR_TIMEOUT	60.0	// # of seconds to wait on Sys_Error running
//  dedicated before exiting
#define PAUSE_SLEEP		50				// sleep time on pause or minimization
#define NOT_FOCUS_SLEEP	20				// sleep time when not focus

int			starttime;
bool	ActiveApp, Minimized;
bool	WinNT;

double		pfreq;
double		curtime = 0.0;
double		lastcurtime = 0.0;
int			lowshift;
bool			isDedicated;
bool		sc_return_on_enter = false;
HANDLE				hinput, houtput;

const char* tracking_tag = "Clams & Mooses";

HANDLE	tevent;
HANDLE	hFile;
HANDLE	heventParent;
HANDLE	heventChild;

#endif

//double		pfreq;
//double		curtime = 0.0;
//double		lastcurtime = 0.0;
//int			lowshift;
//bool			isDedicated;
//bool		sc_return_on_enter = false;

template<typename T>
int Sys_FileRead(int handle, T* dest, size_t count)
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