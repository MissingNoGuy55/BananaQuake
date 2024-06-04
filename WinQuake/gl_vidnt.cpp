/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// gl_vidnt.c -- NT GL vid component

#include "quakedef.h"
#include "winquake.h"
#include "resource.h"
#include <commctrl.h>
#include <memory> // Missi: for std::shared_ptr (6/18/2022)
//#include <GL/glew.h>

#define MAX_MODE_LIST	30
#define VID_ROW_SIZE	3
#define WARP_WIDTH		320
#define WARP_HEIGHT		200
#define MAXWIDTH		10000
#define MAXHEIGHT		10000
#define BASEWIDTH		320
#define BASEHEIGHT		200

#define MODE_WINDOWED			0
#define NO_MODE					(MODE_WINDOWED - 1)
#define MODE_FULLSCREEN_DEFAULT	(MODE_WINDOWED + 1)

//typedef struct {
//	modestate_t	type;
//	int			width;
//	int			height;
//	int			modenum;
//	int			dib;
//	int			fullscreen;
//	int			bpp;
//	int			halfscreen;
//	char		modedesc[17];
//} vmode_t;

typedef struct {
	int			width;
	int			height;
} lmode_t;

lmode_t	lowresmodes[] = {
	{320, 200},
	{320, 240},
	{400, 300},
	{512, 384},
};



const char*		gl_vendor;
const char*		gl_renderer;
const char*		gl_version;
const char*		gl_extensions;

bool		DDActive;
bool		scr_skipupdate;

static vmode_t			modelist[MAX_MODE_LIST];
static int				nummodes;
static vmode_t			*pcurrentmode;
static vmode_t			badmode;

static DEVMODE			gdevmode;
static bool				vid_initialized = false;
static bool				windowed, leavecurrentmode;
static bool				vid_canalttab = false;
static bool				vid_wassuspended = false;
static int				windowed_mouse;
extern bool				mouseactive;  // from in_win.c
static HICON			hIcon;

int						DIBWidth, DIBHeight;
RECT					WindowRect;
static DWORD			WindowStyle, ExWindowStyle;

HWND					mainwindow, dibwindow;

static int				vid_modenum = NO_MODE;
static int				vid_realmode;
static int				vid_default = MODE_WINDOWED;
static int				windowed_default;
static unsigned char	vid_curpal[256*3];
static bool				fullsbardraw = false;

static float vid_gamma = 1.0;

static HGLRC	baseRC;
static HDC		maindc;

glvert_t glv;

cvar_t	gl_ztrick = {"gl_ztrick","1"};

HWND WINAPI InitializeWindow (HINSTANCE hInstance, int nCmdShow);

viddef_t	vid;				// global video state

unsigned short	d_8to16table[256];
unsigned int	d_8to24table[256];
unsigned char d_15to8table[65536];

float		gldepthmin, gldepthmax;

modestate_t	modestate = MS_UNINIT;

void VID_MenuDraw (void);
void VID_MenuKey (int key);

LONG WINAPI MainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void AppActivate(BOOL fActive, BOOL minimize);
char *VID_GetModeDescription (int mode);
void ClearAllStates (void);
void VID_UpdateWindowStatus (void);

PROC QglArrayElementEXT;
PROC QglColorPointerEXT;
PROC QglTexCoordPointerEXT;
PROC QglVertexPointerEXT;

typedef void (APIENTRY *lp3DFXFUNC) (int, int, int, int, int, const void*);
lp3DFXFUNC QglColorTableEXT;
bool is8bit = false;
bool gl_mtexable = false;

//====================================

cvar_t		vid_mode = {"vid_mode","0", false};
// Note that 0 is MODE_WINDOWED
cvar_t		_vid_default_mode = {"_vid_default_mode","0", true};
// Note that 3 is MODE_FULLSCREEN_DEFAULT
cvar_t		_vid_default_mode_win = {"_vid_default_mode_win","3", true};
cvar_t		vid_wait = {"vid_wait","0"};
cvar_t		vid_nopageflip = {"vid_nopageflip","0", true};
cvar_t		_vid_wait_override = {"_vid_wait_override", "0", true};
cvar_t		vid_config_x = {"vid_config_x","800", true};
cvar_t		vid_config_y = {"vid_config_y","600", true};
cvar_t		vid_stretch_by_2 = {"vid_stretch_by_2","1", true};
cvar_t		_windowed_mouse = {"_windowed_mouse","1", true};

int			window_center_x, window_center_y, window_x, window_y, window_width, window_height;
RECT		window_rect;

// direct draw software compatability stuff

vmode_t* GetVideoModes()
{
	return modelist;
}

void VID_HandlePause (bool pause)
{
}

void VID_ForceLockState (int lk)
{
}

void VID_LockBuffer (void)
{
}

void VID_UnlockBuffer (void)
{
}

int VID_ForceUnlockedAndReturnState (void)
{
	return 0;
}

void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}

void D_EndDirectRect (int x, int y, int width, int height)
{
}


void CenterWindow(HWND hWndCenter, int width, int height, BOOL lefttopjustify)
{
    int     CenterX, CenterY;

	CenterX = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
	CenterY = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
	if (CenterX > CenterY*2)
		CenterX >>= 1;	// dual screens
	CenterX = (CenterX < 0) ? 0: CenterX;
	CenterY = (CenterY < 0) ? 0: CenterY;
	SetWindowPos (hWndCenter, NULL, CenterX, CenterY, 0, 0,
			SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_DRAWFRAME);
}

bool VID_SetWindowedMode (int modenum)
{
	HDC				hdc;
	int				lastmodestate, width, height;
	RECT			rect;

	lastmodestate = modestate;

	WindowRect.top = WindowRect.left = 0;

	WindowRect.right = modelist[modenum].width;
	WindowRect.bottom = modelist[modenum].height;

	DIBWidth = modelist[modenum].width;
	DIBHeight = modelist[modenum].height;
	
	if (!g_Common->COM_CheckParm("-noborder"))
	{
		WindowStyle = WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU |
			WS_MINIMIZEBOX;
	}
	else
	{
		WindowStyle = WS_POPUP;
	}
	ExWindowStyle = 0;
	rect = WindowRect;
	AdjustWindowRectEx(&rect, WindowStyle, FALSE, 0);

	width = rect.right - rect.left;
	height = rect.bottom - rect.top;

	// Create the DIB window
	dibwindow = CreateWindowEx (
		 ExWindowStyle,
		 "BananaQuake",			// Missi: was WinQuake (12/1/2022)
		 "GLBananaQuake",		// Missi: was GLQuake (12/1/2022)
		 WindowStyle,
		 rect.left, rect.top,
		 width,
		 height,
		 NULL,
		 NULL,
		 global_hInstance,
		 NULL);

	if (!dibwindow)
		Sys_Error ("Couldn't create DIB window");

	// Center and show the DIB window
	CenterWindow(dibwindow, WindowRect.right - WindowRect.left,
				 WindowRect.bottom - WindowRect.top, false);

	ShowWindow (dibwindow, SW_SHOWDEFAULT);
	UpdateWindow (dibwindow);

	modestate = MS_WINDOWED;

// because we have set the background brush for the window to NULL
// (to avoid flickering when re-sizing the window on the desktop),
// we clear the window to black when created, otherwise it will be
// empty while Quake starts up.
	hdc = GetDC(dibwindow);
	PatBlt(hdc,0,0,WindowRect.right,WindowRect.bottom,BLACKNESS);
	ReleaseDC(dibwindow, hdc);

	if (vid.conheight > (unsigned int)modelist[modenum].height)
		vid.conheight = modelist[modenum].height;
	if (vid.conwidth > (unsigned int)modelist[modenum].width)
		vid.conwidth = modelist[modenum].width;
	vid.width = vid.conwidth;
	vid.height = vid.conheight;

	vid.numpages = 2;

	mainwindow = dibwindow;

	SendMessage (mainwindow, WM_SETICON, (WPARAM)TRUE, (LPARAM)hIcon);
	SendMessage (mainwindow, WM_SETICON, (WPARAM)FALSE, (LPARAM)hIcon);

	return true;
}


bool VID_SetFullDIBMode (int modenum)
{
	HDC				hdc;
	int				lastmodestate, width, height;
	RECT			rect;

	if (!leavecurrentmode)
	{
		gdevmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		gdevmode.dmBitsPerPel = modelist[modenum].bpp;
		gdevmode.dmPelsWidth = modelist[modenum].width <<
							   modelist[modenum].halfscreen;
		gdevmode.dmPelsHeight = modelist[modenum].height;
		gdevmode.dmSize = sizeof (gdevmode);

		if (ChangeDisplaySettings (&gdevmode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			Sys_Error ("Couldn't set fullscreen DIB mode");
	}

	lastmodestate = modestate;
	modestate = MS_FULLDIB;

	WindowRect.top = WindowRect.left = 0;

	WindowRect.right = modelist[modenum].width;
	WindowRect.bottom = modelist[modenum].height;

	DIBWidth = modelist[modenum].width;
	DIBHeight = modelist[modenum].height;

	WindowStyle = WS_POPUP;
	ExWindowStyle = 0;

	rect = WindowRect;
	AdjustWindowRectEx(&rect, WindowStyle, FALSE, 0);

	width = rect.right - rect.left;
	height = rect.bottom - rect.top;

	// Create the DIB window
	dibwindow = CreateWindowEx (
		 ExWindowStyle,
		 "BananaQuake",	// Missi: was WinQuake (12/1/2022)
		 "GLBananaQuake",		// Missi: was GLQuake (12/1/2022)
		 WindowStyle,
		 rect.left, rect.top,
		 width,
		 height,
		 NULL,
		 NULL,
		 global_hInstance,
		 NULL);

	if (!dibwindow)
		Sys_Error ("Couldn't create DIB window");

	ShowWindow (dibwindow, SW_SHOWDEFAULT);
	UpdateWindow (dibwindow);

	// Because we have set the background brush for the window to NULL
	// (to avoid flickering when re-sizing the window on the desktop), we
	// clear the window to black when created, otherwise it will be
	// empty while Quake starts up.
	hdc = GetDC(dibwindow);
	PatBlt(hdc,0,0,WindowRect.right,WindowRect.bottom,BLACKNESS);
	ReleaseDC(dibwindow, hdc);

	if (vid.conheight > (unsigned int)modelist[modenum].height)
		vid.conheight = modelist[modenum].height;
	if (vid.conwidth > (unsigned int)modelist[modenum].width)
		vid.conwidth = modelist[modenum].width;
	vid.width = vid.conwidth;
	vid.height = vid.conheight;

	vid.numpages = 2;

// needed because we're not getting WM_MOVE messages fullscreen on NT
	window_x = 0;
	window_y = 0;

	mainwindow = dibwindow;

	SendMessage (mainwindow, WM_SETICON, (WPARAM)TRUE, (LPARAM)hIcon);
	SendMessage (mainwindow, WM_SETICON, (WPARAM)FALSE, (LPARAM)hIcon);

	return true;
}


int VID_SetMode (int modenum, unsigned char *palette)
{
	int				original_mode, temp;
	bool		stat;
    MSG				msg;

	if ((windowed && (modenum != 0)) ||
		(!windowed && (modenum < 1)) ||
		(!windowed && (modenum >= nummodes)))
	{
		Sys_Error ("Bad video mode\n");
	}

// so Con_Printfs don't mess us up by forcing vid and snd updates
	temp = scr_disabled_for_loading;
	scr_disabled_for_loading = true;

	CDAudio_Pause ();

	if (vid_modenum == NO_MODE)
		original_mode = windowed_default;
	else
		original_mode = vid_modenum;

	// Set either the fullscreen or windowed mode
	if (modelist[modenum].type == MS_WINDOWED)
	{
		if (_windowed_mouse.value && key_dest == key_game)
		{
			stat = VID_SetWindowedMode(modenum);
			IN_ActivateMouse ();
			IN_HideMouse ();
		}
		else
		{
			IN_DeactivateMouse ();
			IN_ShowMouse ();
			stat = VID_SetWindowedMode(modenum);
		}
	}
	else if (modelist[modenum].type == MS_FULLDIB)
	{
		stat = VID_SetFullDIBMode(modenum);
		IN_ActivateMouse ();
		IN_HideMouse ();
	}
	else
	{
		Sys_Error ("VID_SetMode: Bad mode type in modelist");
	}

	window_width = DIBWidth;
	window_height = DIBHeight;
	VID_UpdateWindowStatus ();

	CDAudio_Resume ();
	scr_disabled_for_loading = temp;

	if (!stat)
	{
		Sys_Error ("Couldn't set video mode");
	}

// now we try to make sure we get the focus on the mode switch, because
// sometimes in some systems we don't.  We grab the foreground, then
// finish setting up, pump all our messages, and sleep for a little while
// to let messages finish bouncing around the system, then we put
// ourselves at the top of the z order, then grab the foreground again,
// Who knows if it helps, but it probably doesn't hurt
	SetForegroundWindow (mainwindow);
	VID_SetPalette (palette);
	vid_modenum = modenum;
	Cvar_SetValue ("vid_mode", (float)vid_modenum);

	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
	{
      	TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	Sleep (100);

	const bool bBorderless = g_Common->COM_CheckParm("-noborder");

	if (bBorderless)
	{
		LONG lStyle = GetWindowLong(mainwindow, GWL_STYLE);
		lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);

		SetWindowLong(mainwindow, GWL_STYLE, lStyle);
	}
	SetWindowPos (mainwindow, HWND_TOP, 0, 0, 0, 0,
				  SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW |
				  SWP_NOCOPYBITS);

	SetForegroundWindow (mainwindow);

// fix the leftover Alt from any Alt-Tab or the like that switched us away
	ClearAllStates ();

	if (!host->msg_suppress_1)
		Con_SafePrintf ("Video mode %s initialized.\n", VID_GetModeDescription (vid_modenum));

	VID_SetPalette (palette);

	vid.recalc_refdef = 1;

	return true;
}


/*
================
VID_UpdateWindowStatus
================
*/
void VID_UpdateWindowStatus (void)
{

	window_rect.left = window_x;
	window_rect.top = window_y;
	window_rect.right = window_x + window_width;
	window_rect.bottom = window_y + window_height;
	window_center_x = (window_rect.left + window_rect.right) / 2;
	window_center_y = (window_rect.top + window_rect.bottom) / 2;

	IN_UpdateClipCursor ();
}


//====================================

BINDTEXFUNCPTR bindTexFunc;

#define TEXTURE_EXT_STRING "GL_EXT_texture_object"

static bool GL_ParseExtensionList(const char* list, const char* name)
{
	const char* start;
	const char* where, * terminator;

	if (!list || !name || !*name)
		return false;
	if (strchr(name, ' ') != NULL)
		return false;	// extension names must not have spaces

	start = list;
	while (1) {
		where = strstr(start, name);
		if (!where)
			break;
		terminator = where + strlen(name);
		if (where == start || where[-1] == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return true;
		start = terminator;
	}
	return false;
}

/*
===============
GL_ParseExtensionList
===============
*/
void CheckTextureExtensions (void)
{
	char		*tmp;
	bool	texture_ext;
	HINSTANCE	hInstGL;

	texture_ext = FALSE;
	/* check for texture extension */
	tmp = (char *)glGetString(GL_EXTENSIONS);
	while (*tmp)
	{
		if (strncmp((const char*)tmp, TEXTURE_EXT_STRING, strlen(TEXTURE_EXT_STRING)) == 0)
			texture_ext = TRUE;
		tmp++;
	}

	if (!texture_ext || g_Common->COM_CheckParm ("-gl11") )
	{
		hInstGL = LoadLibrary("opengl32.dll");
		
		if (hInstGL == NULL)
			Sys_Error ("Couldn't load opengl32.dll\n");

		bindTexFunc = (BINDTEXFUNCPTR)GetProcAddress(hInstGL,"glBindTexture");

		if (!bindTexFunc)
			Sys_Error ("No texture objects!");
		return;
	}

/* load library and get procedure adresses for texture extension API */
	if ((bindTexFunc = (BINDTEXFUNCPTR)
		wglGetProcAddress((LPCSTR) "glBindTextureEXT")) == NULL)
	{
		Sys_Error ("GetProcAddress for BindTextureEXT failed");
		return;
	}

	// texture_non_power_of_two
	//
	if (g_Common->COM_CheckParm("-notexturenpot"))
		Con_DPrintf("texture_non_power_of_two disabled at command line\n");
	else if (GL_ParseExtensionList(gl_extensions, "GL_ARB_texture_non_power_of_two"))
	{
		Con_DPrintf("FOUND: ARB_texture_non_power_of_two\n");
		gl_texture_NPOT = true;
	}
	else
	{
		Con_DPrintf("texture_non_power_of_two not supported\n");
	}
}

void CheckArrayExtensions (void)
{

	/* check for texture extension */

	if (SDL_GL_ExtensionSupported("GL_EXT_vertex_array"))
	{
		if (
			((QglArrayElementEXT = wglGetProcAddress("glArrayElementEXT")) == NULL) ||
			((QglColorPointerEXT = wglGetProcAddress("glColorPointerEXT")) == NULL) ||
			((QglTexCoordPointerEXT = wglGetProcAddress("glTexCoordPointerEXT")) == NULL) ||
			((QglVertexPointerEXT = wglGetProcAddress("glVertexPointerEXT")) == NULL)
		)
		{
			Sys_Error ("GetProcAddress for vertex extension failed");
			return;
		}
		return;
	}


	Sys_Error ("Vertex array extension not present");
}

//int		texture_mode = GL_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_LINEAR;
int		texture_mode = GL_LINEAR;
//int		texture_mode = GL_LINEAR_MIPMAP_NEAREST;
//int		texture_mode = GL_LINEAR_MIPMAP_LINEAR;

int		texture_extension_number = 1;

#if defined(_WIN32) || defined(__CYGWIN__)
void CheckMultiTextureExtensions(void) 
{
	if (strstr(gl_extensions, "GL_SGIS_multitexture ") && !g_Common->COM_CheckParm("-nomtex")) {
		Con_Printf("Multitexture extensions found.\n");
		qglMTexCoord2fSGIS = (lpMTexFUNC) wglGetProcAddress("glMTexCoord2fSGIS");
		qglSelectTextureSGIS = (lpSelTexFUNC) wglGetProcAddress("glSelectTextureSGIS");
		gl_mtexable = true;
	}
}
#else
void CheckMultiTextureExtensions(void) 
{
		gl_mtexable = true;
}
#endif

/*
===============
GL_Init

Missi: modified. changed the printing functions to Con_SafePrintf (5/23/2022)
===============
*/
void CGLRenderer::GL_Init (void)
{
	gl_vendor = (const char*)glGetString(GL_VENDOR);
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = (const char*)glGetString (GL_RENDERER);
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

	gl_version = (const char*)glGetString (GL_VERSION);
	Con_Printf ("GL_VERSION: %s\n", gl_version);
	gl_extensions = (const char*)glGetString (GL_EXTENSIONS);
	Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);

//	Con_Printf ("%s %s\n", gl_renderer, gl_version);

    if (_strnicmp(gl_renderer,"PowerVR",7)==0)
		fullsbardraw = true;

	CheckTextureExtensions ();
	CheckMultiTextureExtensions ();

	glClearColor (1,0,0,0);
	glCullFace(GL_FRONT);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.666f);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel (GL_FLAT);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	// poll max size from hardware
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_hardware_maxsize);

#if 0
	CheckArrayExtensions ();

	glEnable (GL_VERTEX_ARRAY_EXT);
	glEnable (GL_TEXTURE_COORD_ARRAY_EXT);
	glVertexPointerEXT (3, GL_FLOAT, 0, 0, &glv.x);
	glTexCoordPointerEXT (2, GL_FLOAT, 0, 0, &glv.s);
	glColorPointerEXT (3, GL_FLOAT, 0, 0, &glv.r);
#endif
}

/*
=================
CGLRenderer::GL_BeginRendering

=================
*/
void CGLRenderer::GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	extern cvar_t gl_clear;

	*x = *y = 0;
	*width = WindowRect.right - WindowRect.left;
	*height = WindowRect.bottom - WindowRect.top;

//    if (!wglMakeCurrent( maindc, baseRC ))
//		Sys_Error ("wglMakeCurrent failed");

//	glViewport (*x, *y, *width, *height);
}


void CGLRenderer::GL_EndRendering (void)
{
	if (!scr_skipupdate || block_drawing)
		SwapBuffers(maindc);

// handle the mouse state when windowed if that's changed
	if (modestate == MS_WINDOWED)
	{
		if (!_windowed_mouse.value) {
			if (windowed_mouse)	{
				IN_DeactivateMouse ();
				IN_ShowMouse ();
				windowed_mouse = false;
			}
		} else {
			windowed_mouse = true;
			if (key_dest == key_game && !mouseactive && ActiveApp) {
				IN_ActivateMouse ();
				IN_HideMouse ();
			} else if (mouseactive && key_dest != key_game) {
				IN_DeactivateMouse ();
				IN_ShowMouse ();
			}
		}
	}
	if (fullsbardraw)
		Sbar_Changed();
}

void	VID_SetPalette (unsigned char *palette)
{
	byte	*pal = nullptr, *src = nullptr, *dst = nullptr;
	int		mark = 0;
	unsigned short i = 0;
	FILE *f = nullptr;

	// Missi: don't load the palette twice. this also prevents the gross "darkening" effect seen in GLQuake (5/31/2024)
	/*g_Common->COM_FOpenFile("gfx/palette.lmp", &f, NULL);
	if (!f)
	{
		Sys_Error("Couldn't load gfx/palette.lmp");
		return;
	}*/

	mark = g_MemCache->Hunk_LowMark();
	pal = (byte*)g_MemCache->Hunk_Alloc<byte>(768);
	
	Q_memcpy(pal, host->host_basepal, 768);

	//fread(pal, sizeof(byte), 768, f);
	//fclose(f);

	//standard palette, 255 is transparent
	dst = (byte*)d_8to24table;
	src = pal;
	for (i = 0; i < 256; i++)
	{
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = 255;
	}
	((byte*)&d_8to24table[255])[3] = 0;

	//fullbright palette, 0-223 are black (for additive blending)
	src = pal + 224 * 3;
	dst = (byte*)&d_8to24table_fbright[224];
	for (i = 224; i < 256; i++)
	{
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = 255;
	}
	for (i = 0; i < 224; i++)
	{
		dst = (byte*)&d_8to24table_fbright[i];
		dst[3] = 255;
		dst[2] = dst[1] = dst[0] = 0;
	}

	//nobright palette, 224-255 are black (for additive blending)
	dst = (byte*)d_8to24table_nobright;
	src = pal;
	for (i = 0; i < 256; i++)
	{
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = 255;
	}
	for (i = 224; i < 256; i++)
	{
		dst = (byte*)&d_8to24table_nobright[i];
		dst[3] = 255;
		dst[2] = dst[1] = dst[0] = 0;
	}

	//fullbright palette, for fence textures
	memcpy(d_8to24table_fbright_fence, d_8to24table_fbright, 256 * 4);
	d_8to24table_fbright_fence[255] = 0; // Alpha of zero.

	//nobright palette, for fence textures
	memcpy(d_8to24table_nobright_fence, d_8to24table_nobright, 256 * 4);
	d_8to24table_nobright_fence[255] = 0; // Alpha of zero.

	//conchars palette, 0 and 255 are transparent
	// Missi: FIXME: does not even work and makes all the characters blank (9/13/2023)
	memcpy(d_8to24table_conchars, d_8to24table, 256 * 4);
	((byte*)&d_8to24table_conchars[0])[3] = 0;

	g_MemCache->Hunk_FreeToLowMark(mark);
}

void VID_SetDefaultMode (void)
{
	IN_DeactivateMouse ();
}


void VID_Shutdown (void)
{
   	HGLRC hRC;
   	HDC	  hDC;

	if (vid_initialized)
	{
		vid_canalttab = false;
		hRC = wglGetCurrentContext();
    	hDC = wglGetCurrentDC();

    	wglMakeCurrent(NULL, NULL);

    	if (hRC)
    	    wglDeleteContext(hRC);

		if (hDC && dibwindow)
			ReleaseDC(dibwindow, hDC);

		if (modestate == MS_FULLDIB)
			ChangeDisplaySettings (NULL, 0);

		if (maindc && dibwindow)
			ReleaseDC (dibwindow, maindc);

		if (g_GLRenderer)
			delete g_GLRenderer;

		AppActivate(false, false);
	}
}


//==========================================================================


BOOL bSetupPixelFormat(HDC hDC)
{
    static PIXELFORMATDESCRIPTOR pfd = {
	sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
	1,								// version number
	PFD_DRAW_TO_WINDOW 				// support window
	|  PFD_SUPPORT_OPENGL 			// support OpenGL
	|  PFD_DOUBLEBUFFER ,			// double buffered
	PFD_TYPE_RGBA,					// RGBA type
	24,								// 24-bit color depth
	0, 0, 0, 0, 0, 0,				// color bits ignored
	0,								// no alpha buffer
	0,								// shift bit ignored
	0,								// no accumulation buffer
	0, 0, 0, 0, 					// accum bits ignored
	32,								// 32-bit z-buffer	
	0,								// no stencil buffer
	0,								// no auxiliary buffer
	PFD_MAIN_PLANE,					// main layer
	0,								// reserved
	0, 0, 0							// layer masks ignored
    };
    int pixelformat;

    if ( (pixelformat = ChoosePixelFormat(hDC, &pfd)) == 0 )
    {
        MessageBox(NULL, "ChoosePixelFormat failed", "Error", MB_OK);
        return FALSE;
    }

    if (SetPixelFormat(hDC, pixelformat, &pfd) == FALSE)
    {
        MessageBox(NULL, "SetPixelFormat failed", "Error", MB_OK);
        return FALSE;
    }

    return TRUE;
}



byte        scantokey[128] = 
					{ 
//  0           1       2       3       4       5       6       7 
//  8           9       A       B       C       D       E       F 
	0  ,    27,     '1',    '2',    '3',    '4',    '5',    '6', 
	'7',    '8',    '9',    '0',    '-',    '=',    K_BACKSPACE, 9, // 0 
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i', 
	'o',    'p',    '[',    ']',    13 ,    K_CTRL,'a',  's',      // 1 
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';', 
	'\'' ,    '`',    K_SHIFT,'\\',  'z',    'x',    'c',    'v',      // 2 
	'b',    'n',    'm',    ',',    '.',    '/',    K_SHIFT,'*', 
	K_ALT,' ',   0  ,    K_F1, K_F2, K_F3, K_F4, K_F5,   // 3 
	K_F6, K_F7, K_F8, K_F9, K_F10, K_PAUSE  ,    0  , K_HOME, 
	K_UPARROW,K_PGUP,'-',K_LEFTARROW,'5',K_RIGHTARROW,'+',K_END, //4 
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             0,              K_F11, 
	K_F12,0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 5 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7 
					}; 

byte        shiftscantokey[128] = 
					{ 
//  0           1       2       3       4       5       6       7 
//  8           9       A       B       C       D       E       F 
	0  ,    27,     '!',    '@',    '#',    '$',    '%',    '^', 
	'&',    '*',    '(',    ')',    '_',    '+',    K_BACKSPACE, 9, // 0 
	'Q',    'W',    'E',    'R',    'T',    'Y',    'U',    'I', 
	'O',    'P',    '{',    '}',    13 ,    K_CTRL,'A',  'S',      // 1 
	'D',    'F',    'G',    'H',    'J',    'K',    'L',    ':', 
	'"' ,    '~',    K_SHIFT,'|',  'Z',    'X',    'C',    'V',      // 2 
	'B',    'N',    'M',    '<',    '>',    '?',    K_SHIFT,'*', 
	K_ALT,' ',   0  ,    K_F1, K_F2, K_F3, K_F4, K_F5,   // 3 
	K_F6, K_F7, K_F8, K_F9, K_F10, K_PAUSE  ,    0  , K_HOME, 
	K_UPARROW,K_PGUP,'_',K_LEFTARROW,'%',K_RIGHTARROW,'+',K_END, //4 
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             0,              K_F11, 
	K_F12,0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 5 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7 
					}; 


/*
=======
MapKey

Map from windows to quake keynums
=======
*/
int MapKey (int key)
{
	key = (key>>16)&255;
	if (key > 127)
		return 0;
	if (scantokey[key] == 0)
		Con_DPrintf("key 0x%02x has no translation\n", key);
	return scantokey[key];
}

/*
===================================================================

MAIN WINDOW

===================================================================
*/

/*
================
ClearAllStates
================
*/
void ClearAllStates (void)
{
	int		i;
	
// send an up event for each key, to make sure the server clears them all
	for (i=0 ; i<256 ; i++)
	{
		Key_Event (i, false);
	}

	Key_ClearStates ();
	IN_ClearStates ();
}

void AppActivate(BOOL fActive, BOOL minimize)
/****************************************************************************
*
* Function:     AppActivate
* Parameters:   fActive - True if app is activating
*
* Description:  If the application is activating, then swap the system
*               into SYSPAL_NOSTATIC mode so that our palettes will display
*               correctly.
*
****************************************************************************/
{
	static BOOL	sound_active;

	ActiveApp = fActive;
	Minimized = minimize;

// enable/disable sound on focus gain/loss
	if (!ActiveApp && sound_active)
	{
		g_SoundSystem->S_BlockSound ();
		sound_active = false;
	}
	else if (ActiveApp && !sound_active)
	{
		g_SoundSystem->S_UnblockSound ();
		sound_active = true;
	}

	if (fActive)
	{
		if (modestate == MS_FULLDIB)
		{
			IN_ActivateMouse ();
			IN_HideMouse ();
			if (vid_canalttab && vid_wassuspended) {
				vid_wassuspended = false;
				ChangeDisplaySettings (&gdevmode, CDS_FULLSCREEN);
				ShowWindow(mainwindow, SW_SHOWNORMAL);
			}
		}
		else if ((modestate == MS_WINDOWED) && _windowed_mouse.value && key_dest == key_game)
		{
			IN_ActivateMouse ();
			IN_HideMouse ();
		}
	}

	if (!fActive)
	{
		if (modestate == MS_FULLDIB)
		{
			IN_DeactivateMouse ();
			IN_ShowMouse ();
			if (vid_canalttab) { 
				ChangeDisplaySettings (NULL, 0);
				vid_wassuspended = true;
			}
		}
		else if ((modestate == MS_WINDOWED) && _windowed_mouse.value)
		{
			IN_DeactivateMouse ();
			IN_ShowMouse ();
		}
	}
}

/* main window procedure */
LONG WINAPI MainWndProc (
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    LONG    lRet = 1;
	int		fActive, fMinimized, temp;
	extern unsigned int uiWheelMessage;

	if ( uMsg == uiWheelMessage )
		uMsg = WM_MOUSEWHEEL;

    switch (uMsg)
    {
		case WM_KILLFOCUS:
			if (modestate == MS_FULLDIB)
				ShowWindow(mainwindow, SW_SHOWMINNOACTIVE);
			break;

		case WM_CREATE:
			break;

		case WM_MOVE:
			window_x = (int) LOWORD(lParam);
			window_y = (int) HIWORD(lParam);
			VID_UpdateWindowStatus ();
			break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			Key_Event (MapKey(lParam), true);
			break;
			
		case WM_KEYUP:
		case WM_SYSKEYUP:
			Key_Event (MapKey(lParam), false);
			break;

		case WM_SYSCHAR:
		// keep Alt-Space from happening
			break;

	// this is complicated because Win32 seems to pack multiple mouse events into
	// one update sometimes, so we always check all states and look for events
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEMOVE:
			temp = 0;

			if (wParam & MK_LBUTTON)
				temp |= 1;

			if (wParam & MK_RBUTTON)
				temp |= 2;

			if (wParam & MK_MBUTTON)
				temp |= 4;

			IN_MouseEvent (temp);

			break;

		// JACK: This is the mouse wheel with the Intellimouse
		// Its delta is either positive or neg, and we generate the proper
		// Event.
		case WM_MOUSEWHEEL: 
			if ((short) HIWORD(wParam) > 0) {
				Key_Event(K_MWHEELUP, true);
				Key_Event(K_MWHEELUP, false);
			} else {
				Key_Event(K_MWHEELDOWN, true);
				Key_Event(K_MWHEELDOWN, false);
			}
			break;

    	case WM_SIZE:
            break;

   	    case WM_CLOSE:
			if (MessageBox (mainwindow, "Are you sure you want to quit?", "Confirm Exit",
						MB_YESNO | MB_SETFOREGROUND | MB_ICONQUESTION) == IDYES)
			{
				Sys_Quit ();
			}

	        break;

		case WM_ACTIVATE:
			fActive = LOWORD(wParam);
			fMinimized = (BOOL) HIWORD(wParam);
			AppActivate(!(fActive == WA_INACTIVE), fMinimized);

		// fix the leftover Alt from any Alt-Tab or the like that switched us away
			ClearAllStates ();

			break;

   	    case WM_DESTROY:
        {
			if (dibwindow)
				DestroyWindow (dibwindow);

            PostQuitMessage (0);
        }
        break;

		//case MM_MCINOTIFY:
            //lRet = CDAudio_MessageHandler (hWnd, uMsg, wParam, lParam);
			//break;

    	default:
            /* pass all unhandled messages to DefWindowProc */
            lRet = DefWindowProc (hWnd, uMsg, wParam, lParam);
        break;
    }

    /* return 1 if handled message, 0 if not */
    return lRet;
}

/*
=================
VID_NumModes
=================
*/
int VID_NumModes (void)
{
	return nummodes;
}
	
/*
=================
VID_GetModePtr
=================
*/
vmode_t *VID_GetModePtr (int modenum)
{

	if ((modenum >= 0) && (modenum < nummodes))
		return &modelist[modenum];
	else
		return &badmode;
}

/*
=================
VID_GetModeDescription
=================
*/
char *VID_GetModeDescription (int mode)
{
	char		*pinfo;
	vmode_t		*pv;
	static char	temp[100];

	if ((mode < 0) || (mode >= nummodes))
		return NULL;

	if (!leavecurrentmode)
	{
		pv = VID_GetModePtr (mode);
		pinfo = pv->modedesc;
	}
	else
	{
		snprintf (temp, sizeof(temp), "Desktop resolution (%dx%d)",
				 modelist[MODE_FULLSCREEN_DEFAULT].width,
				 modelist[MODE_FULLSCREEN_DEFAULT].height);
		pinfo = temp;
	}

	return pinfo;
}

// KJB: Added this to return the mode driver name in description for console

char *VID_GetExtModeDescription (int mode)
{
	static char	pinfo[40];
	vmode_t		*pv;

	if ((mode < 0) || (mode >= nummodes))
		return NULL;

	pv = VID_GetModePtr (mode);
	if (modelist[mode].type == MS_FULLDIB)
	{
		if (!leavecurrentmode)
		{
			snprintf(pinfo, sizeof(pinfo), "%s fullscreen", pv->modedesc);
		}
		else
		{
			snprintf(pinfo, sizeof(pinfo), "Desktop resolution (%dx%d)",
					 modelist[MODE_FULLSCREEN_DEFAULT].width,
					 modelist[MODE_FULLSCREEN_DEFAULT].height);
		}
	}
	else
	{
		if (modestate == MS_WINDOWED)
			snprintf(pinfo, sizeof(pinfo), "%s windowed", pv->modedesc);
		else
			snprintf(pinfo, sizeof(pinfo), "windowed");
	}

	return pinfo;
}

/*
=================
VID_DescribeCurrentMode_f
=================
*/
void VID_DescribeCurrentMode_f (void)
{
	Con_Printf ("%s\n", VID_GetExtModeDescription (vid_modenum));
}

/*
=================
VID_NumModes_f
=================
*/
void VID_NumModes_f (void)
{

	if (nummodes == 1)
		Con_Printf ("%d video mode is available\n", nummodes);
	else
		Con_Printf ("%d video modes are available\n", nummodes);
}

/*
=================
VID_DescribeMode_f
=================
*/
void VID_DescribeMode_f (void)
{
	int		t, modenum;
	
	modenum = Q_atoi (g_pCmds->Cmd_Argv(1));

	t = leavecurrentmode;
	leavecurrentmode = 0;

	Con_Printf ("%s\n", VID_GetExtModeDescription (modenum));

	leavecurrentmode = t;
}

/*
=================
VID_DescribeModes_f
=================
*/
void VID_DescribeModes_f (void)
{
	int			i, lnummodes, t;
	char		*pinfo;
	vmode_t		*pv;

	lnummodes = VID_NumModes ();

	t = leavecurrentmode;
	leavecurrentmode = 0;

	for (i=1 ; i<lnummodes ; i++)
	{
		pv = VID_GetModePtr (i);
		pinfo = VID_GetExtModeDescription (i);
		Con_Printf ("%2d: %s\n", i, pinfo);
	}

	leavecurrentmode = t;
}

void VID_InitDIB (HINSTANCE hInstance)
{
	WNDCLASS		wc;

	memset(&wc, 0, sizeof(wc));

	/* Register the frame class */
    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC)MainWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = 0;
    wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wc.hbrBackground = NULL;
    wc.lpszMenuName  = 0;
    wc.lpszClassName = "BananaQuake"; // Missi: was WinQuake (12/1/2022)

    if (!RegisterClass (&wc) )
		Sys_Error ("Couldn't register window class");

	modelist[0].type = MS_WINDOWED;

	if (g_Common->COM_CheckParm("-width"))
		modelist[0].width = Q_atoi(g_Common->com_argv[g_Common->COM_CheckParm("-width")+1]);
	else if (g_Common->COM_CheckParm("-w"))
		modelist[0].width = Q_atoi(g_Common->com_argv[g_Common->COM_CheckParm("-w") + 1]);
	else
		modelist[0].width = 640;

	if (modelist[0].width < 320)
		modelist[0].width = 320;

	if (g_Common->COM_CheckParm("-height"))
		modelist[0].height= Q_atoi(g_Common->com_argv[g_Common->COM_CheckParm("-height")+1]);
	else if (g_Common->COM_CheckParm("-h"))
		modelist[0].height = Q_atoi(g_Common->com_argv[g_Common->COM_CheckParm("-h") + 1]);
	else
		modelist[0].height = modelist[0].width * 240/320;

	if (modelist[0].height < 240)
		modelist[0].height = 240;

	snprintf (modelist[0].modedesc, sizeof(modelist[0].modedesc), "%dx%d",
			 modelist[0].width, modelist[0].height);

	modelist[0].modenum = MODE_WINDOWED;
	modelist[0].dib = 1;
	modelist[0].fullscreen = 0;
	modelist[0].halfscreen = 0;
	modelist[0].bpp = 0;

	nummodes = 1;
}

/*
=================
VID_InitFullDIB
=================
*/
void VID_InitFullDIB (HINSTANCE hInstance)
{
	DEVMODE	devmode;
	int		i, modenum, originalnummodes, existingmode, numlowresmodes;
	int		j, bpp, done;
	BOOL	stat;

// enumerate >8 bpp modes
	originalnummodes = nummodes;
	modenum = 0;

	do
	{
		stat = EnumDisplaySettings (NULL, modenum, &devmode);

		if ((devmode.dmBitsPerPel >= 15) &&
			(devmode.dmPelsWidth <= MAXWIDTH) &&
			(devmode.dmPelsHeight <= MAXHEIGHT) &&
			(nummodes < MAX_MODE_LIST))
		{
			devmode.dmFields = DM_BITSPERPEL |
							   DM_PELSWIDTH |
							   DM_PELSHEIGHT;

			if (ChangeDisplaySettings (&devmode, CDS_TEST | CDS_FULLSCREEN) ==
					DISP_CHANGE_SUCCESSFUL)
			{
				modelist[nummodes].type = MS_FULLDIB;
				modelist[nummodes].width = devmode.dmPelsWidth;
				modelist[nummodes].height = devmode.dmPelsHeight;
				modelist[nummodes].modenum = 0;
				modelist[nummodes].halfscreen = 0;
				modelist[nummodes].dib = 1;
				modelist[nummodes].fullscreen = 1;
				modelist[nummodes].bpp = devmode.dmBitsPerPel;
				snprintf (modelist[nummodes].modedesc, sizeof(modelist[nummodes].modedesc), "%dx%dx%d",
						 devmode.dmPelsWidth, devmode.dmPelsHeight,
						 devmode.dmBitsPerPel);

			// if the width is more than twice the height, reduce it by half because this
			// is probably a dual-screen monitor
				if (!g_Common->COM_CheckParm("-noadjustaspect"))
				{
					if (modelist[nummodes].width > (modelist[nummodes].height << 1))
					{
						modelist[nummodes].width >>= 1;
						modelist[nummodes].halfscreen = 1;
						snprintf (modelist[nummodes].modedesc, sizeof(modelist[nummodes].modedesc), "%dx%dx%d",
								 modelist[nummodes].width,
								 modelist[nummodes].height,
								 modelist[nummodes].bpp);
					}
				}

				for (i=originalnummodes, existingmode = 0 ; i<nummodes ; i++)
				{
					if ((modelist[nummodes].width == modelist[i].width)   &&
						(modelist[nummodes].height == modelist[i].height) &&
						(modelist[nummodes].bpp == modelist[i].bpp))
					{
						existingmode = 1;
						break;
					}
				}

				if (!existingmode)
				{
					nummodes++;
				}
			}
		}

		modenum++;
	} while (stat);

// see if there are any low-res modes that aren't being reported
	numlowresmodes = sizeof(lowresmodes) / sizeof(lowresmodes[0]);
	bpp = 16;
	done = 0;

	do
	{
		for (j=0 ; (j<numlowresmodes) && (nummodes < MAX_MODE_LIST) ; j++)
		{
			devmode.dmBitsPerPel = bpp;
			devmode.dmPelsWidth = lowresmodes[j].width;
			devmode.dmPelsHeight = lowresmodes[j].height;
			devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

			if (ChangeDisplaySettings (&devmode, CDS_TEST | CDS_FULLSCREEN) ==
					DISP_CHANGE_SUCCESSFUL)
			{
				modelist[nummodes].type = MS_FULLDIB;
				modelist[nummodes].width = devmode.dmPelsWidth;
				modelist[nummodes].height = devmode.dmPelsHeight;
				modelist[nummodes].modenum = 0;
				modelist[nummodes].halfscreen = 0;
				modelist[nummodes].dib = 1;
				modelist[nummodes].fullscreen = 1;
				modelist[nummodes].bpp = devmode.dmBitsPerPel;
				snprintf (modelist[nummodes].modedesc, sizeof(modelist[nummodes].modedesc), "%dx%dx%d",
						 devmode.dmPelsWidth, devmode.dmPelsHeight,
						 devmode.dmBitsPerPel);

				for (i=originalnummodes, existingmode = 0 ; i<nummodes ; i++)
				{
					if ((modelist[nummodes].width == modelist[i].width)   &&
						(modelist[nummodes].height == modelist[i].height) &&
						(modelist[nummodes].bpp == modelist[i].bpp))
					{
						existingmode = 1;
						break;
					}
				}

				if (!existingmode)
				{
					nummodes++;
				}
			}
		}
		switch (bpp)
		{
			case 16:
				bpp = 32;
				break;

			case 32:
				bpp = 24;
				break;

			case 24:
				done = 1;
				break;
		}
	} while (!done);

	if (nummodes == originalnummodes)
		Con_SafePrintf ("No fullscreen DIB modes found\n");
}

bool VID_Is8bit() {
	return is8bit;
}

void VID_Init8bitPalette() 
{
	// Check for 8bit Extensions and initialize them.
	int i;
	char thePalette[256*3];
	char *oldPalette, *newPalette;

	QglColorTableEXT = (lp3DFXFUNC)wglGetProcAddress("glColorTableEXT");
    if (!QglColorTableEXT || strstr(gl_extensions, "GL_EXT_shared_texture_palette") ||
		g_Common->COM_CheckParm("-no8bit"))
		return;

	Con_SafePrintf("8-bit GL extensions enabled.\n");
    glEnable( GL_SHARED_TEXTURE_PALETTE_EXT );
	oldPalette = (char *) d_8to24table; //d_8to24table3dfx;
	newPalette = thePalette;
	for (i=0;i<256;i++) {
		*newPalette++ = *oldPalette++;
		*newPalette++ = *oldPalette++;
		*newPalette++ = *oldPalette++;
		oldPalette++;
	}
	QglColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGB, 256, GL_RGB, GL_UNSIGNED_BYTE,
		(void *) thePalette);
	is8bit = TRUE;
}

static void Check_Gamma (unsigned char *pal)
{
	float	f, inf;
	unsigned char	palette[768];
	int		i;

	if ((i = g_Common->COM_CheckParm("-gamma")) == 0) {
		if ((gl_renderer && strstr(gl_renderer, "Voodoo")) ||
			(gl_vendor && strstr(gl_vendor, "3Dfx")))
			vid_gamma = 1;
		else
			vid_gamma = 0.7f; // default to 0.7 on non-3dfx hardware
	} else
		vid_gamma = Q_atof(g_Common->com_argv[i+1]);

	for (i=0 ; i<768 ; i++)
	{
		f = pow ( (pal[i]+1)/256.0 , vid_gamma );
		inf = f*255 + 0.5;
		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		palette[i] = inf;
	}

	memcpy (pal, palette, sizeof(palette));
}

/*
===================
VID_Init
===================
*/
void	VID_Init (unsigned char *palette)
{
	int		i, existingmode;
	int		basenummodes = 0, width = 0, height = 0, bpp = 0, findbpp = 0, done = 0;
	char	gldir[MAX_OSPATH];
	HDC		hdc;
	DEVMODE	devmode;

	memset(&devmode, 0, sizeof(devmode));

	Cvar_RegisterVariable (&vid_mode);
	Cvar_RegisterVariable (&vid_wait);
	Cvar_RegisterVariable (&vid_nopageflip);
	Cvar_RegisterVariable (&_vid_wait_override);
	Cvar_RegisterVariable (&_vid_default_mode);
	Cvar_RegisterVariable (&_vid_default_mode_win);
	Cvar_RegisterVariable (&vid_config_x);
	Cvar_RegisterVariable (&vid_config_y);
	Cvar_RegisterVariable (&vid_stretch_by_2);
	Cvar_RegisterVariable (&_windowed_mouse);
	Cvar_RegisterVariable (&gl_ztrick);

	g_pCmds->Cmd_AddCommand ("vid_nummodes", VID_NumModes_f);
	g_pCmds->Cmd_AddCommand ("vid_describecurrentmode", VID_DescribeCurrentMode_f);
	g_pCmds->Cmd_AddCommand ("vid_describemode", VID_DescribeMode_f);
	g_pCmds->Cmd_AddCommand ("vid_describemodes", VID_DescribeModes_f);

	hIcon = LoadIcon (global_hInstance, MAKEINTRESOURCE (IDI_ICON2));

	InitCommonControls();

	VID_InitDIB (global_hInstance);
	basenummodes = nummodes = 1;

	VID_InitFullDIB (global_hInstance);

	// Missi: Source engine styled "-sw", "-w" and "-h" command support (5/1/2023)
	if (g_Common->COM_CheckParm("-window") || g_Common->COM_CheckParm("-sw") || g_Common->COM_CheckParm("-windowed"))
	{
		hdc = GetDC (NULL);

		if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
		{
			Sys_Error ("Can't run in non-RGB mode");
		}

		if (g_Common->COM_CheckParm("-width"))
			width = Q_atoi(g_Common->com_argv[g_Common->COM_CheckParm("-width") + 1]);
		else if (g_Common->COM_CheckParm("-w"))
			width = Q_atoi(g_Common->com_argv[g_Common->COM_CheckParm("-w") + 1]);
		else
			width = 640;

		if (g_Common->COM_CheckParm("-height"))
			height = Q_atoi(g_Common->com_argv[g_Common->COM_CheckParm("-height") + 1]);
		else if (g_Common->COM_CheckParm("-h"))
			height = Q_atoi(g_Common->com_argv[g_Common->COM_CheckParm("-h") + 1]);

		ReleaseDC (NULL, hdc);

		windowed = true;

		vid_default = MODE_WINDOWED;
	}
	else
	{
		if (nummodes == 1)
			Sys_Error ("No RGB fullscreen modes available");

		windowed = false;

		if (g_Common->COM_CheckParm("-mode"))
		{
			vid_default = Q_atoi(g_Common->com_argv[g_Common->COM_CheckParm("-mode")+1]);
		}
		else
		{
			if (g_Common->COM_CheckParm("-current"))
			{
				modelist[MODE_FULLSCREEN_DEFAULT].width =
						GetSystemMetrics (SM_CXSCREEN);
				modelist[MODE_FULLSCREEN_DEFAULT].height =
						GetSystemMetrics (SM_CYSCREEN);
				vid_default = MODE_FULLSCREEN_DEFAULT;
				leavecurrentmode = 1;
			}
			else
			{
				if (g_Common->COM_CheckParm("-width"))
					width = Q_atoi(g_Common->com_argv[g_Common->COM_CheckParm("-width")+1]);
				else if (g_Common->COM_CheckParm("-w"))
					width = Q_atoi(g_Common->com_argv[g_Common->COM_CheckParm("-w") + 1]);
				else
					width = 640;

				if (g_Common->COM_CheckParm("-bpp"))
				{
					bpp = Q_atoi(g_Common->com_argv[g_Common->COM_CheckParm("-bpp")+1]);
					findbpp = 0;
				}
				else
				{
					bpp = 15;
					findbpp = 1;
				}

				if (g_Common->COM_CheckParm("-height"))
					height = Q_atoi(g_Common->com_argv[g_Common->COM_CheckParm("-height")+1]);
				else if (g_Common->COM_CheckParm("-h"))
					height = Q_atoi(g_Common->com_argv[g_Common->COM_CheckParm("-h") + 1]);

				// if they want to force it, add the specified mode to the list
				if (g_Common->COM_CheckParm("-force") && (nummodes < MAX_MODE_LIST))
				{
					modelist[nummodes].type = MS_FULLDIB;
					modelist[nummodes].width = width;
					modelist[nummodes].height = height;
					modelist[nummodes].modenum = 0;
					modelist[nummodes].halfscreen = 0;
					modelist[nummodes].dib = 1;
					modelist[nummodes].fullscreen = 1;
					modelist[nummodes].bpp = bpp;
					snprintf (modelist[nummodes].modedesc, sizeof(modelist[nummodes].modedesc), "%dx%dx%d",
							 devmode.dmPelsWidth, devmode.dmPelsHeight,
							 devmode.dmBitsPerPel);

					for (i=nummodes, existingmode = 0 ; i<nummodes ; i++)
					{
						if ((modelist[nummodes].width == modelist[i].width)   &&
							(modelist[nummodes].height == modelist[i].height) &&
							(modelist[nummodes].bpp == modelist[i].bpp))
						{
							existingmode = 1;
							break;
						}
					}

					if (!existingmode)
					{
						nummodes++;
					}
				}

				done = 0;

				do
				{
					if (g_Common->COM_CheckParm("-height"))
					{
						height = Q_atoi(g_Common->com_argv[g_Common->COM_CheckParm("-height")+1]);

						for (i=1, vid_default=0 ; i<nummodes ; i++)
						{
							if ((modelist[i].width == width) &&
								(modelist[i].height == height) &&
								(modelist[i].bpp == bpp))
							{
								vid_default = i;
								done = 1;
								break;
							}
						}
					}
					else if (g_Common->COM_CheckParm("-h"))
					{
						height = Q_atoi(g_Common->com_argv[g_Common->COM_CheckParm("-h") + 1]);

						for (i = 1, vid_default = 0; i < nummodes; i++)
						{
							if ((modelist[i].width == width) &&
								(modelist[i].height == height) &&
								(modelist[i].bpp == bpp))
							{
								vid_default = i;
								done = 1;
								break;
							}
						}
					}
					else
					{
						for (i=1, vid_default=0 ; i<nummodes ; i++)
						{
							if ((modelist[i].width == width) && (modelist[i].bpp == bpp))
							{
								vid_default = i;
								done = 1;
								break;
							}
						}
					}

					if (!done)
					{
						if (findbpp)
						{
							switch (bpp)
							{
							case 15:
								bpp = 16;
								break;
							case 16:
								bpp = 32;
								break;
							case 32:
								bpp = 24;
								break;
							case 24:
								done = 1;
								break;
							}
						}
						else
						{
							done = 1;
						}
					}
				} while (!done);

				if (!vid_default)
				{
					Sys_Error ("Specified video mode not available");
				}
			}
		}
	}

	vid_initialized = true;

	if ((i = g_Common->COM_CheckParm("-conwidth")) != 0)
		vid.conwidth = Q_atoi(g_Common->com_argv[i+1]);
	else
		vid.conwidth = 640;
	
	if (g_Common->COM_CheckParm("-w"))
        vid.conwidth = width;
    if (g_Common->COM_CheckParm("-h"))
        vid.conheight = height;

	vid.conwidth &= 0xfff8; // make it a multiple of eight

	if (vid.conwidth < 320)
		vid.conwidth = 320;

	// pick a conheight that matches with correct aspect
	vid.conheight = vid.conwidth*3 / 4;

	if ((i = g_Common->COM_CheckParm("-conheight")) != 0)
		vid.conheight = Q_atoi(g_Common->com_argv[i+1]);
	if (vid.conheight < 200)
		vid.conheight = 200;

	vid.maxwarpwidth = WARP_WIDTH;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.colormap = host->host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

	DestroyWindow (hwnd_dialog);

	if (!palette)
		return;

	Check_Gamma(palette);
	VID_SetPalette (palette);

	VID_SetMode (vid_default, palette);

    maindc = GetDC(mainwindow);
	bSetupPixelFormat(maindc);

    baseRC = wglCreateContext( maindc );
	if (!baseRC)
		Sys_Error ("Could not initialize GL (wglCreateContext failed).\n\nMake sure you in are 65535 color mode, and try running -window.");
    if (!wglMakeCurrent( maindc, baseRC ))
		Sys_Error ("wglMakeCurrent failed");

	g_GLRenderer->GL_Init ();

	snprintf (gldir, sizeof(gldir), "%s/glquake", g_Common->com_gamedir);
	Sys_mkdir (gldir);

	vid_realmode = vid_modenum;

	// Check for 3DFX Extensions and initialize them.
	//VID_Init8bitPalette();

	vid_menudrawfn = VID_MenuDraw;
	vid_menukeyfn = VID_MenuKey;

	Q_strcpy (badmode.modedesc, "Bad mode");
	vid_canalttab = true;

	if (g_Common->COM_CheckParm("-fullsbar"))
		fullsbardraw = true;
}


//========================================================
// Video menu stuff
//========================================================

extern void M_Menu_Options_f (void);
extern void M_Print (int cx, int cy, const char *str);
extern void M_PrintWhite (int cx, int cy, const char *str);
extern void M_DrawCharacter (int cx, int line, int num);
extern void M_DrawTransPic (int x, int y, CQuakePic *pic);
extern void M_DrawPic (int x, int y, CQuakePic *pic);

static int	vid_line, vid_wmodes;

typedef struct
{
	int		modenum;
	char	*desc;
	int		iscur;
} modedesc_t;

#define MAX_COLUMN_SIZE		9
#define MODE_AREA_HEIGHT	(MAX_COLUMN_SIZE + 2)
#define MAX_MODEDESCS		(MAX_COLUMN_SIZE*3)

static modedesc_t	modedescs[MAX_MODEDESCS];

/*
================
VID_MenuDraw
================
*/
void VID_MenuDraw (void)
{
	CQuakePic		*p;
	char		*ptr;
	int			lnummodes, i, k, column, row;
	vmode_t		*pv;

	p = g_GLRenderer->Draw_CachePic ("gfx/vidmodes.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	vid_wmodes = 0;
	lnummodes = VID_NumModes ();
	
	for (i=1 ; (i<lnummodes) && (vid_wmodes < MAX_MODEDESCS) ; i++)
	{
		ptr = VID_GetModeDescription (i);
		pv = VID_GetModePtr (i);

		k = vid_wmodes;

		modedescs[k].modenum = i;
		modedescs[k].desc = ptr;
		modedescs[k].iscur = 0;

		if (i == vid_modenum)
			modedescs[k].iscur = 1;

		vid_wmodes++;

	}

	if (vid_wmodes > 0)
	{
		M_Print (2*8, 36+0*8, "Fullscreen Modes (WIDTHxHEIGHTxBPP)");

		column = 8;
		row = 36+2*8;

		for (i=0 ; i<vid_wmodes ; i++)
		{
			if (modedescs[i].iscur)
				M_PrintWhite (column, row, modedescs[i].desc);
			else
				M_Print (column, row, modedescs[i].desc);

			column += 13*8;

			if ((i % VID_ROW_SIZE) == (VID_ROW_SIZE - 1))
			{
				column = 8;
				row += 8;
			}
		}
	}

	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*2,
			 "Video modes must be set from the");
	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*3,
			 "command line with -width <width>");
	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*4,
			 "and -bpp <bits-per-pixel>");
	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*6,
			 "Select windowed mode with -window");
}


/*
================
VID_MenuKey
================
*/
void VID_MenuKey (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		g_SoundSystem->S_LocalSound ("misc/menu1.wav");
		M_Menu_Options_f ();
		break;

	default:
		break;
	}
}
