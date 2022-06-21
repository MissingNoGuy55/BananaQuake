
#include "quakedef.h"
#include "conproc.h"

#ifdef SDL_VERSION
#undef main
#endif

HINSTANCE	global_hInstance;
int			global_nCmdShow;
char* argv[MAX_NUM_ARGVS];
static char* empty_string = "";
HWND		hwnd_dialog;
HANDLE	hinput, houtput;

#define MINIMUM_WIN_MEMORY		0x0880000
#define MAXIMUM_WIN_MEMORY		0x1000000

#define CONSOLE_ERROR_TIMEOUT	60.0	// # of seconds to wait on Sys_Error running
//  dedicated before exiting
#define PAUSE_SLEEP		50				// sleep time on pause or minimization
#define NOT_FOCUS_SLEEP	20				// sleep time when not focus

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
bool			isDedicated;
static bool		sc_return_on_enter = false;

static char* tracking_tag = "Clams & Mooses";

static HANDLE	tevent;
static HANDLE	hFile;
static HANDLE	heventParent;
static HANDLE	heventChild;

unsigned int ceil_cw, single_cw, full_cw, cw, pushed_cw;
unsigned int fpenv[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

bool		DDActive;
bool		scr_skipupdate;

/*
==================
WinMain
==================
*/
void SleepUntilInput(int time)
{

	MsgWaitForMultipleObjects(1, &tevent, FALSE, time, QS_ALLINPUT);
}

int main(int argc, char* argv[])
{
	MSG				msg;
	quakeparms_t<byte*>	parms;
	double			time, oldtime, newtime;
	MEMORYSTATUS	lpBuffer;
	static	char	cwd[1024];
	int				t;
	RECT			rect;
	SDL_Window*		window;
	bool			isDedicated;

	host = new CQuakeHost;

	lpBuffer.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&lpBuffer);

	if (!GetCurrentDirectory(sizeof(cwd), cwd))
		Sys_Error("Couldn't determine current directory");

	if (cwd[Q_strlen(cwd) - 1] == '/')
		cwd[Q_strlen(cwd) - 1] = 0;

	parms.basedir = cwd;
	parms.cachedir = NULL;
	parms.argc = argc;
	parms.argv = argv;

	parms.argc = 1;
	argv[0] = empty_string;

	parms.argv = argv;

	COM_InitArgv(parms.argc, parms.argv);

	parms.argc = com_argc;
	parms.argv = com_argv;

	isDedicated = (COM_CheckParm("-dedicated") != 0);

	// take the greater of all the available memory or half the total memory,
	// but at least 8 Mb and no more than 16 Mb, unless they explicitly
	// request otherwise
	parms.memsize = lpBuffer.dwAvailPhys;

	if (parms.memsize < MINIMUM_MEMORY)
		parms.memsize = MINIMUM_MEMORY;

	if (parms.memsize < (lpBuffer.dwTotalPhys >> 1))
		parms.memsize = lpBuffer.dwTotalPhys >> 1;

	if (parms.memsize > MINIMUM_MEMORY)
		parms.memsize = MINIMUM_MEMORY;

	if (COM_CheckParm("-heapsize"))
	{
		t = COM_CheckParm("-heapsize") + 1;

		if (t < com_argc)
			parms.memsize = Q_atoi(com_argv[t]) * 1024;
	}

	parms.membase = (byte*)malloc(parms.memsize);

	if (!parms.membase)
		Sys_Error("Not enough memory free; check disk space\n");

	//Sys_PageIn((void*)parms.membase, parms.memsize);

	// tevent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// if (!tevent)
	// 	Sys_Error ("Couldn't create event");

	if (isDedicated)
	{
		if (!AllocConsole())
		{
			Sys_Error("Couldn't create dedicated server console");
		}

		g_ConProc = new CConProc;

		hinput = GetStdHandle(STD_INPUT_HANDLE);
		houtput = GetStdHandle(STD_OUTPUT_HANDLE);

		// give QHOST a chance to hook into the console
		if ((t = COM_CheckParm("-HFILE")) > 0)
		{
			if (t < com_argc)
				hFile = (HANDLE)Q_atoi(com_argv[t + 1]);
		}

		if ((t = COM_CheckParm("-HPARENT")) > 0)
		{
			if (t < com_argc)
				heventParent = (HANDLE)Q_atoi(com_argv[t + 1]);
		}

		if ((t = COM_CheckParm("-HCHILD")) > 0)
		{
			if (t < com_argc)
				heventChild = (HANDLE)Q_atoi(com_argv[t + 1]);
		}

		g_ConProc->InitConProc(hFile, heventParent, heventChild);
	}

	// Sys_Init ();

	// because sound is off until we become active
		// g_SoundSystem->S_BlockSound ();

	Sys_Printf("Host_Init\n");
	host->Host_Init(parms);

	oldtime = Sys_DoubleTime();

	/* main window message loop */
	while (1)
	{
		if (isDedicated)
		{
			newtime = Sys_DoubleTime();
			time = newtime - oldtime;

			while (time < host->sys_ticrate.value)
			{
				Sys_Sleep();
				newtime = Sys_DoubleTime();
				time = newtime - oldtime;
			}
		}
		else
		{
			// yield the CPU for a little while when paused, minimized, or not the focus
			if ((cl.paused && (!ActiveApp && !DDActive)) || Minimized || block_drawing)
			{
				SleepUntilInput(PAUSE_SLEEP);
				scr_skipupdate = 1;		// no point in bothering to draw
			}
			else if (!ActiveApp && !DDActive)
			{
				SleepUntilInput(NOT_FOCUS_SLEEP);
			}

			newtime = Sys_DoubleTime();
			time = newtime - oldtime;
		}

		host->Host_Frame(time);
		oldtime = newtime;
	}

	/* return success of application */
	return TRUE;

}