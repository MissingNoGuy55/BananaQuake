/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2021-2024 Stephen "Missi" Schmiedeberg

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sys_win.c -- Win32 system interface code

#define SDL_MAIN_HANDLED

#include "quakedef.h"
#include "host.h"
#include "winquake.h"
#include "errno.h"
#include "resource.h"
#include "conproc.h"
#include "sys_win.h"
#include <direct.h>	// Missi: for _mkdir
#include <time.h>
#include <VersionHelpers.h>

#define MINIMUM_WIN_MEMORY		0x0880000
#define MAXIMUM_WIN_MEMORY		0x1000000

#define CONSOLE_ERROR_TIMEOUT	60.0	// # of seconds to wait on Sys_Error running
										//  dedicated before exiting
#define PAUSE_SLEEP		50				// sleep time on pause or minimization
#define NOT_FOCUS_SLEEP	20				// sleep time when not focus


int			starttime;
bool	ActiveApp, Minimized;
bool	WinNT;


static double		pfreq;
static double		curtime = 0.0;
static double		lastcurtime = 0.0;
static int			lowshift;
static bool			isDedicated;
static bool			sc_return_on_enter = false;
HANDLE		hinput, houtput;

static const char	*tracking_tag = "Clams & Mooses";

static HANDLE	tevent;
static HANDLE	hFile;
static HANDLE	heventParent;
static HANDLE	heventChild;

static unsigned int ceil_cw, single_cw, full_cw, cw, pushed_cw;
static unsigned int fpenv[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

//void MaskExceptions (void);
void Sys_InitFloatTime (void);
//void Sys_PushFPCW_SetHigh (void);
//void Sys_PopFPCW (void);

volatile int					sys_checksum;


/*
================
Sys_PageIn
================
*/
#ifndef WIN64
void Sys_PageIn (void *ptr, int size)
#else
void Sys_PageIn	(void* ptr, size_t size)
#endif
{
	byte	*x;
	int		m, n;

// touch all the memory to make sure it's there. The 16-page skip is to
// keep Win 95 from thinking we're trying to page ourselves in (we are
// doing that, of course, but there's no reason we shouldn't)
	x = (byte *)ptr;

	for (n=0 ; n<4 ; n++)
	{
		for (m=0 ; m<(size - 16 * 0x1000) ; m += 4)
		{
			sys_checksum += *(int *)&x[m];
			sys_checksum += *(int *)&x[m + 16 * 0x1000];
		}
	}
}


/*
===============================================================================

FILE IO

===============================================================================
*/

FILE* sys_handles[MAX_HANDLES];

int		findhandle (void)
{
	int		i;
	
	for (i=1 ; i<MAX_HANDLES ; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error ("out of handles");
	return -1;
}

/*
================
filelength
================
*/

int filelength (FILE *f)
{
	int		pos;
	int		end;

	int		t;
	t = VID_ForceUnlockedAndReturnState ();

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);


	VID_ForceLockState (t);


	return end;
}

int Sys_FileOpenRead (const char *path, int *hndl)
{
	FILE	*f = NULL;
	errno_t err;
	int		i = 0, retval = 0;
	int		t = 0;


	t = VID_ForceUnlockedAndReturnState ();


	i = findhandle ();

	err = fopen_s(&f, path, "rb");

	if (err != 0)
	{
		*hndl = -1;
		retval = -1;
	}
	else
	{
		sys_handles[i] = f;
		*hndl = i;
		retval = filelength(f);
	}


	VID_ForceLockState (t);


	return retval;
}

int Sys_FileOpenWrite (const char *path)
{
	FILE	*f = NULL;
	int		i = 0;
	int		t = 0;
	char	p[MAX_OSPATH];
	errno_t err;

	t = VID_ForceUnlockedAndReturnState ();

	i = findhandle ();

	Q_strcpy(p, path);

	err = fopen_s(&f, path, "w+b");
	if (err)
		Sys_Error ("Error opening %s: %s", path,strerror_s(p,strlen(path),errno));
	sys_handles[i] = f;
	

	VID_ForceLockState (t);


	return i;
}

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES	((DWORD)-1)
#endif
int Sys_FileType(const char* path)
{
	DWORD result = GetFileAttributes(path);

	if (result == INVALID_FILE_ATTRIBUTES)
		return FS_ENT_NONE;
	if (result & FILE_ATTRIBUTE_DIRECTORY)
		return FS_ENT_DIRECTORY;

	return FS_ENT_FILE;
}

void Sys_FileClose (int handle)
{
	int		t;

	t = VID_ForceUnlockedAndReturnState ();

	fclose (sys_handles[handle]);
	sys_handles[handle] = NULL;


	VID_ForceLockState (t);

}

void Sys_FileSeek (int handle, int position)
{

	int		t;
	t = VID_ForceUnlockedAndReturnState ();

	fseek (sys_handles[handle], position, SEEK_SET);


	VID_ForceLockState (t);

}

int Sys_FileWrite (int handle, void *data, int count)
{
	int		x;

	int		t;
	t = VID_ForceUnlockedAndReturnState ();

	x = fwrite (data, 1, count, sys_handles[handle]);

	VID_ForceLockState (t);

	return x;
}

void Sys_mkdir (const char *path)
{
	int shut_up = 0;

	shut_up = _mkdir (path); // Missi: shut up compiler (11/28/2022)
}

/*
===============================================================================

SYSTEM IO

===============================================================================
*/

/*
================
Sys_MakeCodeWriteable
================
*/

#ifndef WIN64

#ifdef _WIN32

void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
	DWORD  flOldProtect;

	if (!VirtualProtect((LPVOID)startaddr, length, PAGE_READWRITE, &flOldProtect))
   		Sys_Error("Protection change failed\n");
}

#endif

// #ifndef _M_IX86
/*
void Sys_LowFPPrecision(void)
{
	__asm fldcw single_cw
	__asm ret
}

void Sys_HighFPPrecision(void)
{
	__asm fldcw full_cw
	__asm ret
}

void Sys_SetFPCW (void)
{

	int testint = 0;

	__asm
	{

		fnstcw	cw
		mov 	eax, testint
#ifdef id386
		cmp 	testint, 0xF0
		cmp 	testint, 0x03	// round mode, 64-bit precision
#endif
		mov		eax, full_cw

#if	id386
		cmp		ah,  0xF0
		cmp		ah,  0x0C	// chop mode, single precision
#endif
		mov		eax, single_cw

#if	id386
		cmp		ah,  0xF0
		cmp		ah,  0x08	// ceil mode, single precision
#endif
		mov		eax, ceil_cw

	}

}

void Sys_PushFPCW_SetHigh (void)
{
	__asm
	{
		fnstcw	pushed_cw
		fldcw	full_cw
	}
}

void Sys_PopFPCW (void)
{
	__asm
	{
		fnstcw	pushed_cw
	}
}

void MaskExceptions (void)
{
	__asm
	{
		fnstenv	fpenv
		cmp		fpenv, not 0
		fldenv	fpenv
	}
}
*/
#if 0
void UnmaskExceptions(void)
{
	__asm fnstenv	fpenv
	__asm andl		$0xFFFFFFE0, fpenv
	__asm fldenv	fpenv

	__asm ret
}
#endif

#endif

// #endif

static void Sys_SetTimerResolution(void)
{
	/* Set OS timer resolution to 1ms.
	   Works around buffer underruns with directsound and SDL2, but also
	   will make Sleep()/SDL_Dleay() accurate to 1ms which should help framerate
	   stability.
	*/
	timeBeginPeriod(1);
}


/*
================
Sys_Init
================
*/
void Sys_Init (void)
{
	OSVERSIONINFO	vinfo = {};

	Sys_SetTimerResolution();

	Sys_InitFloatTime ();

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	if (!IsWindowsXPOrGreater())
		Sys_Error("Couldn't get OS info");
/*
	if ((vinfo.dwMajorVersion < 4) ||
		(vinfo.dwPlatformId == VER_PLATFORM_WIN32s))
	{
		Sys_Error ("WinQuake requires at least Win95 or NT 4.0");
	}

	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		WinNT = true;
	else
		WinNT = false;
*/
}


void Sys_Error (const char *error, ...)
{
	va_list		argptr;
	char		text[1024], text2[1024];
	const char		*text3 = "Press Enter to exit\n";
	const char		*text4 = "***********************************\n";
	const char		*text5 = "\n";
	DWORD		dummy;
	double		start_time;
	static int	in_sys_error0 = 0;
	static int	in_sys_error1 = 0;
	static int	in_sys_error2 = 0;
	static int	in_sys_error3 = 0;

	if (!in_sys_error3)
	{
		in_sys_error3 = 1;
		VID_ForceUnlockedAndReturnState ();
	}

	va_start (argptr, error);
	vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	if (isDedicated)
	{
		va_start (argptr, error);
		vsnprintf (text, sizeof(text), error, argptr);
		va_end (argptr);

		snprintf (text2, sizeof(text2), "ERROR: %s\n", text);
		WriteFile (houtput, text5, strlen (text5), &dummy, NULL);
		WriteFile (houtput, text4, strlen (text4), &dummy, NULL);
		WriteFile (houtput, text2, strlen (text2), &dummy, NULL);
		WriteFile (houtput, text3, strlen (text3), &dummy, NULL);
		WriteFile (houtput, text4, strlen (text4), &dummy, NULL);


		start_time = Sys_DoubleTime ();
		sc_return_on_enter = true;	// so Enter will get us out of here

		while (!Sys_ConsoleInput () &&
				((Sys_DoubleTime () - start_time) < CONSOLE_ERROR_TIMEOUT))
		{
		}
	}
	else
	{
	// switch to windowed so the message box is visible, unless we already
	// tried that and failed
		if (!in_sys_error0)
		{
			in_sys_error0 = 1;
			VID_SetDefaultMode ();
			MessageBox(NULL, text, "Quake Error",
					   MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
		}
		else
		{
			MessageBox(NULL, text, "Double Quake Error",
					   MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
		}
	}

	if (!in_sys_error1)
	{
		in_sys_error1 = 1;
		host->Host_Shutdown ();
	}

// shut down QHOST hooks if necessary
	if (!in_sys_error2)
	{
		in_sys_error2 = 1;
		g_ConProc->DeinitConProc ();
	}

	exit (1);
}


void Sys_Printf (const char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];
	DWORD		dummy;
	
	if (isDedicated)
	{
		va_start (argptr,fmt);
		Q_vsnprintf_s(text, sizeof(text), 1024, fmt, argptr);
		va_end (argptr);

		WriteFile(houtput, text, strlen (text), &dummy, NULL);	
	}
}

void Sys_Warning(const char* fmt, ...)
{
	va_list		argptr;
	char		text[1024];
	DWORD		dummy;

	if (isDedicated)
	{
		va_start(argptr, fmt);
		Q_vsnprintf_s(text, sizeof(text), 1024, fmt, argptr);
		va_end(argptr);

		WriteFile(houtput, text, strlen(text), &dummy, NULL);
	}
}

void Sys_Quit (void)
{

	VID_ForceUnlockedAndReturnState ();

	host->Host_Shutdown();

	if (tevent)
		CloseHandle (tevent);

	if (isDedicated)
		FreeConsole ();

// shut down QHOST hooks if necessary
	g_ConProc->DeinitConProc ();

	exit (0);
}

int	Sys_FileTime(const char* path)
{
	FILE* f;
	errno_t err;
	int		t, retval;

	t = VID_ForceUnlockedAndReturnState();

	err = fopen_s(&f, path, "r+");

	if (err == 0)
	{
		fclose(f);
		retval = 1;
	}
	else
	{
		retval = -1;
	}

	VID_ForceLockState(t);

	return retval;
}

/*
================
Sys_FloatTime
================
double Sys_FloatTime (void)
{
	static int			sametimecount;
	static unsigned int	oldtime;
	static int			first = 1;
	LARGE_INTEGER		PerformanceCount;
	unsigned int		temp, t2;
	double				time;
	SYSTEMTIME			st;

	GetSystemTime(&st);


	//Sys_PushFPCW_SetHigh ();

	QueryPerformanceCounter (&PerformanceCount);

	temp = ((unsigned int)PerformanceCount.LowPart >> lowshift) |
		   ((unsigned int)PerformanceCount.HighPart << (32 - lowshift));

	if (first)
	{
		oldtime = temp;
		first = 0;
	}
	else
	{
	// check for turnover or backward time
		if ((temp <= oldtime) && ((oldtime - temp) < 0x10000000))
		{
			oldtime = temp;	// so we can't get stuck
		}
		else
		{
			t2 = temp - oldtime;

			time = (double)t2 * pfreq;
			oldtime = temp;

			curtime += time;

			if (curtime == lastcurtime)
			{
				sametimecount++;

				if (sametimecount > 100000)
				{
					curtime += 1.0;
					sametimecount = 0;
				}
			}
			else
			{
				sametimecount = 0;
			}

			lastcurtime = curtime;
		}
	}

	//Sys_PopFPCW ();

    return curtime;
}
*/

double Sys_DoubleTime()
{
#if (__x86_64__) || (WIN64)
	return SDL_GetTicks64() / 1000.0f;
#else
	return SDL_GetTicks() / 1000.0f;
#endif
}


/*
================
Sys_InitFloatTime
================
*/
void Sys_InitFloatTime (void)
{
	int		j;

	Sys_DoubleTime ();

	j = g_Common->COM_CheckParm("-starttime");

	if (j)
	{
		curtime = (double) (Q_atof(g_Common->com_argv[j+1]));
	}
	else
	{
		curtime = 0.0;
	}

	lastcurtime = curtime;
}


char *Sys_ConsoleInput (void)
{
	static char	text[256];
	static int		len = 0;
	INPUT_RECORD	recs[1024];
	int		ch = 0; 
	DWORD	dummy, numread, numevents;

	if (!isDedicated)
		return NULL;


	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
			Sys_Error ("Error getting # of console events");

		if (!numevents)
			break;

		if (!ReadConsoleInput(hinput, recs, 1, &numread))
			Sys_Error ("Error reading console input");

		if (numread != 1)
			Sys_Error ("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT)
		{
			if (!recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
					case '\r':
						WriteFile(houtput, "\r\n", 2, &dummy, NULL);	

						if (len)
						{
							text[len] = 0;
							len = 0;
							return text;
						}
						/*
						else if (sc_return_on_enter)
						{
						// special case to allow exiting from the error handler on Enter
							text[0] = '\r';
							len = 0;
							return text;
						}
						*/
						break;

					case '\b':
						WriteFile(houtput, "\b \b", 3, &dummy, NULL);	
						if (len)
						{
							len--;
						}
						break;

					default:
						if (ch >= ' ')
						{
							WriteFile(houtput, &ch, 1, &dummy, NULL);	
							text[len] = ch;
							len = (len + 1) & 0xff;
						}

						break;

				}
			}
		}
	}

	return NULL;
}

void Sys_Sleep (void)
{
	Sleep (1);
}


void Sys_SendKeyEvents (void)
{
    MSG        msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
	{
	// we always update if there are any event, even if we're paused
		scr_skipupdate = 0;

		if (!GetMessage (&msg, NULL, 0, 0))
			Sys_Quit ();

      	TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}
}


/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/


/*
==================
WinMain
==================
*/
void SleepUntilInput (int time)
{

	MsgWaitForMultipleObjects(1, &tevent, FALSE, time, QS_ALLINPUT);
}


/*
==================
WinMain
==================
*/
HINSTANCE	global_hInstance;
int			global_nCmdShow;
char		*argv[MAX_NUM_ARGVS];
static const char	*empty_string = "";
HWND		hwnd_dialog = {};


int WINAPI WinMain (_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	quakeparms_t<byte*>	parms;
	double			time, oldtime, newtime;
#ifndef WIN64
	MEMORYSTATUS	lpBuffer;
#else
	MEMORYSTATUSEX	lpBuffer;
#endif
	static	char	cwd[1024];
	int				t;
	RECT			rect;

	g_Common = new CCommon;
	g_FileSystem = new CFileSystem;
	host = new CQuakeHost;

    /* previous instances do not exist in Win32 */
    if (hPrevInstance)
        return 0;

	global_hInstance = hInstance;
	global_nCmdShow = nCmdShow;
#ifndef WIN64
	lpBuffer.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&lpBuffer);
#else
	lpBuffer.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&lpBuffer);
#endif
	if (!GetCurrentDirectory (sizeof(cwd), cwd))
		Sys_Error ("Couldn't determine current directory");

	if (cwd[Q_strlen(cwd)-1] == '/')
		cwd[Q_strlen(cwd)-1] = 0;

	parms.basedir = cwd;
	parms.cachedir = NULL;

	parms.argc = 1;
	argv[0] = new char[1];
	Q_strncpy(argv[0], empty_string, sizeof(*empty_string));

	int pos = 0;
	bool hitquote = false;

	while (*lpCmdLine && (parms.argc < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv[parms.argc] = lpCmdLine;

			// Missi: changed this to account for quoted strings e.g. for the "-game" parameter
			// In the original code, spaces would terminate an argv regardless of quotes (6/2/2024)
			if (*lpCmdLine == '\"' && !hitquote)
			{
				char testStr[MAX_PATH] = {};
				int pos = 0;

				lpCmdLine++;

				testStr[pos] = '\"';

				while (*lpCmdLine && *lpCmdLine != '\"')
				{
					pos++;
					testStr[pos] = *lpCmdLine;
					lpCmdLine++;
				}

				testStr[pos+1] = '\"';

				argv[parms.argc] = testStr;
				hitquote = true;
			}

			parms.argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}
			
		}
	}

	for (int pos = 0; pos < parms.argc; pos++)
		Q_FixQuotes(argv[pos], argv[pos], strlen(argv[pos])+1);

	parms.argv = argv;

	g_Common->COM_InitArgv (parms.argc, parms.argv);

	parms.argc = g_Common->com_argc;
	parms.argv = g_Common->com_argv;

	isDedicated = (g_Common->COM_CheckParm ("-dedicated") != 0);

	if (!isDedicated)
	{

		hwnd_dialog = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, NULL);

		if (hwnd_dialog)
		{
			if (GetWindowRect (hwnd_dialog, &rect))
			{
				if (rect.left > (rect.top * 2))
				{
					SetWindowPos (hwnd_dialog, 0,
						(rect.left / 2) - ((rect.right - rect.left) / 2),
						rect.top, 0, 0,
						SWP_NOZORDER | SWP_NOSIZE);
				}
			}

			ShowWindow (hwnd_dialog, SW_SHOWDEFAULT);
			UpdateWindow (hwnd_dialog);
			SetForegroundWindow (hwnd_dialog);
		}
	}

// take the greater of all the available memory or half the total memory,
// but at least 8 Mb and no more than 16 Mb, unless they explicitly
// request otherwise

#ifndef WIN64
	parms.memsize = lpBuffer.dwAvailPhys;
#else
	parms.memsize = lpBuffer.ullAvailPhys;
#endif

	if (parms.memsize < MINIMUM_WIN_MEMORY)
		parms.memsize = MINIMUM_WIN_MEMORY;

#ifndef WIN64
	if (parms.memsize < (lpBuffer.dwTotalPhys >> 1))
		parms.memsize = lpBuffer.dwTotalPhys >> 1;
#else
	if (parms.memsize < (lpBuffer.ullTotalPhys >> 1))
		parms.memsize = lpBuffer.ullTotalPhys >> 1;
#endif

	if (parms.memsize > MAXIMUM_WIN_MEMORY)
		parms.memsize = MAXIMUM_WIN_MEMORY;

	if (g_Common->COM_CheckParm ("-heapsize"))
	{
		t = g_Common->COM_CheckParm("-heapsize") + 1;

		if (t < g_Common->com_argc)
			parms.memsize = Q_atoi (g_Common->com_argv[t]) * 1024;
	}

	parms.membase = (byte*)malloc (parms.memsize);

	if (!parms.membase)
		Sys_Error ("Not enough memory free; check disk space\n");

	// Missi: old Win9x code. not really necessary any more (6/12/2024)
	//Sys_PageIn ((void*)parms.membase, parms.memsize);

	tevent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (!tevent)
		Sys_Error ("Couldn't create event");

	if (isDedicated)
	{
		if (!AllocConsole ())
		{
			Sys_Error ("Couldn't create dedicated server console");
		}

		hinput = GetStdHandle (STD_INPUT_HANDLE);
		houtput = GetStdHandle (STD_OUTPUT_HANDLE);
#ifndef WIN64
	// give QHOST a chance to hook into the console
		if ((t = g_Common->COM_CheckParm ("-HFILE")) > 0)
		{
			if (t < g_Common->com_argc)
				hFile = (HANDLE)Q_atoi (g_Common->com_argv[t+1]);
		}
			
		if ((t = g_Common->COM_CheckParm ("-HPARENT")) > 0)
		{
			if (t < g_Common->com_argc)
				heventParent = (HANDLE)Q_atoi (g_Common->com_argv[t+1]);
		}
			
		if ((t = g_Common->COM_CheckParm ("-HCHILD")) > 0)
		{
			if (t < g_Common->com_argc)
				heventChild = (HANDLE)Q_atoi (g_Common->com_argv[t+1]);
		}
#endif
		g_ConProc->InitConProc (hFile, heventParent, heventChild);
	}

	Sys_Init ();

// because sound is off until we become active
	g_SoundSystem->S_BlockSound ();

	Sys_Printf ("Host_Init\n");
	host->Host_Init(parms);

	oldtime = Sys_DoubleTime();

    /* main window message loop */
	while (1)
	{
		if (isDedicated)
		{
			newtime = Sys_DoubleTime ();
			time = newtime - oldtime;

			while (time < host->sys_ticrate.value )
			{
				Sys_Sleep();
				newtime = Sys_DoubleTime ();
				time = newtime - oldtime;
			}
		}
		else
		{
		// yield the CPU for a little while when paused, minimized, or not the focus
			if ((cl.paused && (!ActiveApp && !DDActive)) || Minimized || block_drawing)
			{
				SleepUntilInput (PAUSE_SLEEP);
				scr_skipupdate = 1;		// no point in bothering to draw
			}
			else if (!ActiveApp && !DDActive)
			{
				SleepUntilInput (NOT_FOCUS_SLEEP);
			}
			newtime = Sys_DoubleTime ();
			time = newtime - oldtime;
		}

		host->Host_Frame (time);
		oldtime = newtime;
	}

    /* return success of application */
    return TRUE;
}

