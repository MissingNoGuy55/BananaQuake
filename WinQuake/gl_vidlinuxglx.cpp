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
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/vt.h>
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>

#include <dlfcn.h>

#include "quakedef.h"

extern XF86VidModeModeInfo** GetVideoModes();

#define WARP_WIDTH              320
#define WARP_HEIGHT             200

static Display *dpy = NULL;
static int      scrnum;
static Window   win;
static GLXContext ctx = NULL;
static bool     fullscreen = true;

static bool     mouse_override = false;

static cvar_t   x11_grab_mouse { "x11_grab_mouse", "1" };

#define KEY_MASK (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK (ButtonPressMask | ButtonReleaseMask | \
		    PointerMotionMask | ButtonMotionMask )
#define X_MASK (KEY_MASK | MOUSE_MASK | VisibilityChangeMask | StructureNotifyMask | LeaveWindowMask | EnterWindowMask )


unsigned short	d_8to16table[256];
unsigned int	d_8to24table[256];
unsigned char	d_15to8table[65536];
unsigned int	d_8to24table_wad[MAX_MAP_TEXTURES][256];

cvar_t          vid_mode = {"vid_mode","0",false};
 
static bool     mouse_avail;
static bool     mouse_active;
static int      mx, my;
static int      old_mouse_x, old_mouse_y;

static cvar_t   in_mouse = {"in_mouse", "1", false};
static cvar_t   in_dgamouse = {"in_dgamouse", "1", false};
static cvar_t   m_filter = {"m_filter", "0"};

bool            dgamouse = false;
bool            vidmode_ext = false;

static int      win_x, win_y;

static int      scr_width, scr_height;

static XF86VidModeModeInfo **vidmodes;
static int default_dotclock_vidmode;
static int num_vidmodes;
static bool vidmode_active = false;

/*-----------------------------------------------------------------------*/

//int		texture_mode = GL_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_NEAREST;
//int       texture_mode = GL_NEAREST_MIPMAP_LINEAR;
int         texture_mode = GL_LINEAR;
//int		texture_mode = GL_LINEAR_MIPMAP_NEAREST;
//int		texture_mode = GL_LINEAR_MIPMAP_LINEAR;

int		texture_extension_number = 1;

float		gldepthmin, gldepthmax;

cvar_t	gl_ztrick = {"gl_ztrick","1"};

const char      *gl_vendor;
const char      *gl_renderer;
const char      *gl_version;
const char      *gl_extensions;

void (*qglColorTableEXT) (int, int, int, int, int, const void*);
void (*qgl3DfxSetPaletteEXT) (GLuint *);

static float vid_gamma = 1.0;

bool        is8bit = false;
bool        isPermedia = false;
bool        gl_mtexable = false;

XF86VidModeModeInfo** GetVideoModes()
{
	return vidmodes;
}

/*-----------------------------------------------------------------------*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}

void D_EndDirectRect (int x, int y, int width, int height)
{
}

static int XLateKey(XKeyEvent *ev)
{

	int key;
	char buf[64];
	KeySym keysym;

	key = 0;

	XLookupString(ev, buf, sizeof buf, &keysym, 0);

	switch(keysym)
	{
		case XK_KP_Page_Up:	 
		case XK_Page_Up:	 key = K_PGUP; break;

		case XK_KP_Page_Down: 
		case XK_Page_Down:	 key = K_PGDN; break;

		case XK_KP_Home: 
		case XK_Home:	 key = K_HOME; break;

		case XK_KP_End:  
		case XK_End:	 key = K_END; break;

		case XK_KP_Left: 
		case XK_Left:	 key = K_LEFTARROW; break;

		case XK_KP_Right: 
		case XK_Right:	key = K_RIGHTARROW;		break;

		case XK_KP_Down: 
		case XK_Down:	 key = K_DOWNARROW; break;

		case XK_KP_Up:   
		case XK_Up:		 key = K_UPARROW;	 break;

		case XK_Escape: key = K_ESCAPE;		break;

		case XK_KP_Enter: 
		case XK_Return: key = K_ENTER;		 break;

		case XK_Tab:		key = K_TAB;			 break;

		case XK_F1:		 key = K_F1;				break;

		case XK_F2:		 key = K_F2;				break;

		case XK_F3:		 key = K_F3;				break;

		case XK_F4:		 key = K_F4;				break;

		case XK_F5:		 key = K_F5;				break;

		case XK_F6:		 key = K_F6;				break;

		case XK_F7:		 key = K_F7;				break;

		case XK_F8:		 key = K_F8;				break;

		case XK_F9:		 key = K_F9;				break;

		case XK_F10:		key = K_F10;			 break;

		case XK_F11:		key = K_F11;			 break;

		case XK_F12:		key = K_F12;			 break;

		case XK_BackSpace: key = K_BACKSPACE; break;

		case XK_KP_Delete: 
		case XK_Delete: key = K_DEL; break;

		case XK_Pause:	key = K_PAUSE;		 break;

		case XK_Shift_L:
		case XK_Shift_R:	key = K_SHIFT;		break;

		case XK_Execute: 
		case XK_Control_L: 
		case XK_Control_R:	key = K_CTRL;		 break;

		case XK_Alt_L:	
		case XK_Meta_L: 
		case XK_Alt_R:	
		case XK_Meta_R: key = K_ALT;			break;

		case XK_KP_Begin: key = '5';	break;

		case XK_KP_Insert: 
		case XK_Insert:key = K_INS; break;

		case XK_KP_Multiply: key = '*'; break;
		case XK_KP_Add:  key = '+'; break;
		case XK_KP_Subtract: key = '-'; break;
		case XK_KP_Divide: key = '/'; break;

#if 0
		case 0x021: key = '1';break;/* [!] */
		case 0x040: key = '2';break;/* [@] */
		case 0x023: key = '3';break;/* [#] */
		case 0x024: key = '4';break;/* [$] */
		case 0x025: key = '5';break;/* [%] */
		case 0x05e: key = '6';break;/* [^] */
		case 0x026: key = '7';break;/* [&] */
		case 0x02a: key = '8';break;/* [*] */
		case 0x028: key = '9';;break;/* [(] */
		case 0x029: key = '0';break;/* [)] */
		case 0x05f: key = '-';break;/* [_] */
		case 0x02b: key = '=';break;/* [+] */
		case 0x07c: key = '\'';break;/* [|] */
		case 0x07d: key = '[';break;/* [}] */
		case 0x07b: key = ']';break;/* [{] */
		case 0x022: key = '\'';break;/* ["] */
		case 0x03a: key = ';';break;/* [:] */
		case 0x03f: key = '/';break;/* [?] */
		case 0x03e: key = '.';break;/* [>] */
		case 0x03c: key = ',';break;/* [<] */
#endif

		default:
			key = *(unsigned char*)buf;
			if (key >= 'A' && key <= 'Z')
				key = key - 'A' + 'a';
			break;
	} 

	return key;
}

static Cursor CreateNullCursor(Display *display, Window root)
{
    Pixmap cursormask; 
    XGCValues xgc;
    GC gc;
    XColor dummycolour;
    Cursor cursor;

    cursormask = XCreatePixmap(display, root, 1, 1, 1/*depth*/);
    xgc.function = GXclear;
    gc =  XCreateGC(display, cursormask, GCFunction, &xgc);
    XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
    dummycolour.pixel = 0;
    dummycolour.red = 0;
    dummycolour.flags = 04;
    cursor = XCreatePixmapCursor(display, cursormask, cursormask,
          &dummycolour,&dummycolour, 0,0);
    XFreePixmap(display,cursormask);
    XFreeGC(display,gc);
    return cursor;
}

static void install_grabs(void)
{

    // inviso cursor
	XDefineCursor(dpy, win, CreateNullCursor(dpy, win));

	XGrabPointer(dpy, win,
				 True,
				 0,
				 GrabModeAsync, GrabModeAsync,
				 win,
				 None,
				 CurrentTime);

	if (in_dgamouse.value) {
		int MajorVersion, MinorVersion;

        if (!XF86DGAQueryVersion(dpy, &MajorVersion, &MinorVersion)) {
			// unable to query, probalby not supported
			Con_Printf( "Failed to detect XF86DGA Mouse\n" );
			in_dgamouse.value = 0;
		} else {
			dgamouse = true;
			XF86DGADirectVideo(dpy, DefaultScreen(dpy), XF86DGADirectMouse);
			XWarpPointer(dpy, None, win, 0, 0, 0, 0, 0, 0);
		}
	} else {
		XWarpPointer(dpy, None, win,
					 0, 0, 0, 0,
					 vid.width / 2, vid.height / 2);
	}

    if (fullscreen)
    {
        XGrabKeyboard(dpy, win,
                      False,
                      GrabModeAsync, GrabModeAsync,
                      CurrentTime);
    }

	mouse_active = true;

    //XSync(dpy, True);
}

static void uninstall_grabs(void)
{
	if (!dpy || !win)
		return;

	if (dgamouse) {
		dgamouse = false;
		XF86DGADirectVideo(dpy, DefaultScreen(dpy), 0);
	}

	XUngrabPointer(dpy, CurrentTime);

    if (!mouse_override)
        XUngrabKeyboard(dpy, CurrentTime);

// inviso cursor
	XUndefineCursor(dpy, win);

	mouse_active = false;
}

extern void KeyDown (kbutton_t *b);
extern void KeyUp (kbutton_t *b);

void IN_DeactivateMouse( void )
{
    if (!mouse_avail || !dpy || !win || x11_grab_mouse.value < 1)
        return;

    if (mouse_active) {
        uninstall_grabs();
        mouse_active = false;
    }
}

void IN_ActivateMouse( void )
{
    if (!mouse_avail || !dpy || !win || x11_grab_mouse.value < 1)
        return;

    if (!mouse_active) {
        mx = my = 0; // don't spazz
        install_grabs();
        mouse_active = true;
    }
}


static void HandleEvents(void)
{
	XEvent event;
	KeySym ks;
	int b;
	bool dowarp = false;
	int mwx = vid.width/2;
	int mwy = vid.height/2;

	if (!dpy)
		return;

	while (XPending(dpy)) {
		XNextEvent(dpy, &event);

        if (x11_grab_mouse.value == 0)
            uninstall_grabs();

		switch (event.type) {
		case KeyPress:
		case KeyRelease:
			Key_Event(XLateKey(&event.xkey), event.type == KeyPress);
			break;

		case MotionNotify:
			if (mouse_active) {
				if (dgamouse) {
					mx += (event.xmotion.x + win_x) * 2;
					my += (event.xmotion.y + win_y) * 2;
				} 
				else 
				{
					mx += ((int)event.xmotion.x - mwx) * 2;
					my += ((int)event.xmotion.y - mwy) * 2;
					mwx = event.xmotion.x;
					mwy = event.xmotion.y;

					if (mx || my)
						dowarp = true;
				}
			}
			break;

		case ButtonPress:
			b=-1;
			if (event.xbutton.button == 1)
				b = 0;
			else if (event.xbutton.button == 2)
				b = 2;
			else if (event.xbutton.button == 3)
				b = 1;
			if (b>=0)
				Key_Event(K_MOUSE1 + b, true);
			break;

		case ButtonRelease:
			b=-1;
			if (event.xbutton.button == 1)
				b = 0;
			else if (event.xbutton.button == 2)
				b = 2;
			else if (event.xbutton.button == 3)
				b = 1;
			if (b>=0)
				Key_Event(K_MOUSE1 + b, false);
			break;

		case CreateNotify :
			win_x = event.xcreatewindow.x;
			win_y = event.xcreatewindow.y;
			break;

        case EnterNotify:
            vidmode_active = true;
            if (g_Common->COM_CheckParm("-nomouse") == 0)
            {
                install_grabs();
                in_mlook.state |= 1;
            }
            break;

        case LeaveNotify:
            vidmode_active = false;
            if (g_Common->COM_CheckParm("-nomouse") == 0)
            {
                uninstall_grabs();
                in_mlook.state &= ~1;
                in_strafe.state &= ~1;  // Missi: prevents a bug on Linux where mlook is lost on alt+tabbing (6/27/2024)
            }
            break;

		case ConfigureNotify :
			win_x = event.xconfigure.x;
			win_y = event.xconfigure.y;
			break;
		}
	}

	if (dowarp) {
		/* move the mouse to the window center again */
		XWarpPointer(dpy, None, win, 0, 0, 0, 0, vid.width / 2, vid.height / 2);
	}

}

void VID_Shutdown(void)
{
	if (!ctx || !dpy)
		return;

    mouse_override = false;

	IN_DeactivateMouse();
	if (dpy) {
		if (ctx)
			glXDestroyContext(dpy, ctx);
		if (win)
			XDestroyWindow(dpy, win);
		if (vidmode_active)
			XF86VidModeSwitchToMode(dpy, scrnum, vidmodes[0]);
		XCloseDisplay(dpy);
	}
	vidmode_active = false;
	dpy = NULL;
	win = 0;
	ctx = NULL;
}

void signal_handler(int sig)
{
	printf("Received signal %d, exiting...\n", sig);
	Sys_Quit();
	exit(0);
}

void InitSig(void)
{
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGTRAP, signal_handler);
	signal(SIGIOT, signal_handler);
	signal(SIGBUS, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
}

void VID_ShiftPalette(unsigned char *p)
{
//	VID_SetPalette(p);
}

void	VID_SetPalette (unsigned char *palette)
{
#if 0
    byte	*pal;
    unsigned r,g,b;
    unsigned v;
    int     r1,g1,b1;
    int		j,k,l,m;
    unsigned short i;
    unsigned	*table;
    FILE *f;
    char s[255];
    int dist, bestdist;

//
// 8 8 8 encoding
//
	pal = palette;
	table = d_8to24table;
	for (i=0 ; i<256 ; i++)
	{
		r = pal[0];
		g = pal[1];
		b = pal[2];
		pal += 3;
		
		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		*table++ = v;
	}
	d_8to24table[255] &= 0xffffff;	// 255 is transparent

	for (i=0; i < (1<<15); i++) {
		/* Maps
		000000000000000
		000000000011111 = Red  = 0x1F
		000001111100000 = Blue = 0x03E0
		111110000000000 = Grn  = 0x7C00
		*/
		r = ((i & 0x1F) << 3)+4;
		g = ((i & 0x03E0) >> 2)+4;
		b = ((i & 0x7C00) >> 7)+4;
		pal = (unsigned char *)d_8to24table;
		for (v=0,k=0,bestdist=10000*10000; v<256; v++,pal+=4) {
			r1 = (int)r - (int)pal[0];
			g1 = (int)g - (int)pal[1];
			b1 = (int)b - (int)pal[2];
			dist = (r1*r1)+(g1*g1)+(b1*b1);
			if (dist < bestdist) {
				k=v;
				bestdist = dist;
			}
		}
		d_15to8table[i]=k;
	}
#else
    int i = 0;
    byte* pal = nullptr;
    byte* dst = nullptr;
    byte* src = nullptr;
    size_t mark = 0;

    mark = g_MemCache->Hunk_LowMark();
    pal = (byte*)g_MemCache->Hunk_Alloc<byte>(768);

    if (host->host_basepal)
        Q_memcpy(pal, host->host_basepal, 768);

	//standard palette, 255 is transparent
	dst = (byte*)d_8to24table;
    src = host->host_basepal;
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
#endif
}

void CheckMultiTextureExtensions(void) 
{
	void *prjobj;

	if (strstr(gl_extensions, "GL_SGIS_multitexture ") && !g_Common->COM_CheckParm("-nomtex")) {
		Con_Printf("Found GL_SGIS_multitexture...\n");

		if ((prjobj = dlopen(NULL, RTLD_LAZY)) == NULL) {
			Con_Printf("Unable to open symbol list for main program.\n");
			return;
		}

		qglMTexCoord2fSGIS = (lpMTexFUNC) dlsym(prjobj, "glMTexCoord2fSGIS");
		qglSelectTextureSGIS = (lpSelTexFUNC) dlsym(prjobj, "glSelectTextureSGIS");

		if (qglMTexCoord2fSGIS && qglSelectTextureSGIS) {
			Con_Printf("Multitexture extensions found.\n");
			gl_mtexable = true;
		} else
			Con_Printf("Symbol not found, disabled.\n");

		dlclose(prjobj);
	}
}

static const char* GL_MakeNiceExtensionsList()
{
    const char* list = (const char*)glGetString (GL_EXTENSIONS);
    static char silly[65535];
    int i = 0;
    
    do
    {
        if (*list == ' ')
            silly[i] = '\n';
        
        silly[i] = *list++;
        i++;
    }
    while (*list);
        
    char* real_list = silly;
    
    return real_list;
}

/*
===============
GL_Init
===============
*/
void CGLRenderer::GL_Init (void)
{
	gl_vendor = (const char*)glGetString (GL_VENDOR);
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = (const char*)glGetString (GL_RENDERER);
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

	gl_version = (const char*)glGetString (GL_VERSION);
	Con_Printf ("GL_VERSION: %s\n", gl_version);
    
    gl_extensions = GL_MakeNiceExtensionsList();
    
	Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);

//	Con_Printf ("%s %s\n", gl_renderer, gl_version);

	CheckMultiTextureExtensions ();

	glClearColor (1,0,0,0);
	glCullFace(GL_FRONT);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.666);

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
}

/*
=================
GL_BeginRendering

=================
*/
void CGLRenderer::GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	extern cvar_t gl_clear;

	*x = *y = 0;
	*width = scr_width;
	*height = scr_height;

//    if (!wglMakeCurrent( maindc, baseRC ))
//		Sys_Error ("wglMakeCurrent failed");

//	glViewport (*x, *y, *width, *height);
}


void CGLRenderer::GL_EndRendering (void)
{
	glFlush();
	glXSwapBuffers(dpy, win);
}

bool VID_Is8bit(void)
{
	return is8bit;
}

struct export_vtable {
   void (*helloworld)(GLuint*);
};

struct export_vtable2 {
   void (*helloworld)(int, int, int, int, int, const void*);
};

void VID_Init8bitPalette(void) 
{
	// Check for 8bit Extensions and initialize them.
	int i;
	void *prjobj;
    
    export_vtable* vtable1 = NULL;
    export_vtable2* vtable2 = NULL;
    
	if ((prjobj = dlopen(NULL, RTLD_LAZY)) == NULL) {
		Con_Printf("Unable to open symbol list for main program.\n");
		return;
	}

	vtable1 = (export_vtable*)dlsym(prjobj, "glColorTableEXT");
	
    if (vtable1)
        qgl3DfxSetPaletteEXT = vtable1->helloworld;
    
    vtable2 = (export_vtable2*)dlsym(prjobj, "glSetPaletteEXT");
	
    if (vtable2)
        qglColorTableEXT = vtable2->helloworld;
    
	/* if (strstr(gl_extensions, "3DFX_set_global_palette") && qgl3DfxSetPaletteEXT) {
		GLubyte table[256][4];
		char *oldpal;

		Con_SafePrintf("8-bit GL extensions enabled.\n");
		glEnable( GL_SHARED_TEXTURE_PALETTE_EXT );
		oldpal = (char *) d_8to24table; //d_8to24table3dfx;
		for (i=0;i<256;i++) {
			table[i][2] = *oldpal++;
			table[i][1] = *oldpal++;
			table[i][0] = *oldpal++;
			table[i][3] = 255;
			oldpal++;
		}
		qgl3DfxSetPaletteEXT((GLuint *)table);
		is8bit = true;

	} else */ if (strstr(gl_extensions, "GL_EXT_shared_texture_palette") &&
		qglColorTableEXT) {
		char thePalette[256*3];
		char *oldPalette, *newPalette;

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
		qglColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGB, 256, GL_RGB, GL_UNSIGNED_BYTE, (void *) thePalette);
		is8bit = true;
	}
	
	dlclose(prjobj);
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
			vid_gamma = 0.7; // default to 0.7 on non-3dfx hardware
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

void VID_Init(unsigned char *palette)
{
	int i;
	int attrib[] = {
		GLX_RGBA,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DOUBLEBUFFER,
		GLX_DEPTH_SIZE, 1,
		None
	};
	char	gldir[MAX_OSPATH];
	int width = 640, height = 480;
	XSetWindowAttributes attr;
	unsigned long mask;
	Window root;
	XVisualInfo *visinfo;
	int MajorVersion, MinorVersion;
	int actualWidth, actualHeight;

	Cvar_RegisterVariable (&vid_mode);
	Cvar_RegisterVariable (&in_mouse);
	Cvar_RegisterVariable (&in_dgamouse);
	Cvar_RegisterVariable (&m_filter);
	Cvar_RegisterVariable (&gl_ztrick);
	
	vid.maxwarpwidth = WARP_WIDTH;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.colormap = host->host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

// interpret command-line params

// set vid parameters

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Error couldn't open the X display\n");
        exit(1);
    }

    // Missi: Source engine styled "-sw", "-w" and "-h" command support (5/1/2023)
	if ((i = g_Common->COM_CheckParm("-window")) != 0)
		fullscreen = false;
	if ((i = g_Common->COM_CheckParm("-sw")) != 0)
		fullscreen = false;

	if ((i = g_Common->COM_CheckParm("-width")) != 0)
		width = atoi(g_Common->com_argv[i+1]);
	else if (g_Common->COM_CheckParm("-w"))
        width = atoi(g_Common->com_argv[g_Common->COM_CheckParm("-w") + 1]);
    else
    {
        // Missi: grab the current monitor's display resolution to use in cases
        // like fullscreen, because the default 640x480 will thrash the display
        // settings in modern Linux distros (6/14/2024)
        int screenCount = ScreenCount(dpy);
        Screen* test = nullptr;

        for (int i = 0; i < screenCount; i++)
        {
            test = ScreenOfDisplay(dpy, i);

            if (!test)
                break;

            width = WidthOfScreen(test);
        }
    }

	if ((i = g_Common->COM_CheckParm("-height")) != 0)
		height = atoi(g_Common->com_argv[i+1]);
	else if (g_Common->COM_CheckParm("-h"))
		height = Q_atoi(g_Common->com_argv[g_Common->COM_CheckParm("-h") + 1]);
    else
    {
        // Missi: grab the current monitor's display resolution to use in cases
        // like fullscreen, because the default 640x480 will thrash the display
        // settings in modern Linux distros (6/14/2024)
        int screenCount = ScreenCount(dpy);
        Screen* test = nullptr;

        for (int i = 0; i < screenCount; i++)
        {
            test = ScreenOfDisplay(dpy, i);

            if (!test)
                break;

            height = HeightOfScreen(test);
        }
    }

	if ((i = g_Common->COM_CheckParm("-conwidth")) != 0)
		vid.conwidth = Q_atoi(g_Common->com_argv[i+1]);
	else
		vid.conwidth = 640;

    if (g_Common->COM_CheckParm("-w"))
    {
        vid.width = width;
        //vid.conwidth = width;
    }
    if (g_Common->COM_CheckParm("-h"))
    {
        vid.height = height;
        //vid.conheight = height;
    }
	vid.conwidth &= 0xfff8; // make it a multiple of eight

	if (vid.conwidth < 320)
		vid.conwidth = 320;

	// pick a conheight that matches with correct aspect
    vid.conheight = vid.conwidth*3 / 4;

	if ((i = g_Common->COM_CheckParm("-conheight")) != 0)
		vid.conheight = Q_atoi(g_Common->com_argv[i+1]);
	if (vid.conheight < 200)
		vid.conheight = 200;

	scrnum = DefaultScreen(dpy);
	root = RootWindow(dpy, scrnum);

	// Get video mode list
	MajorVersion = MinorVersion = 0;
	if (!XF86VidModeQueryVersion(dpy, &MajorVersion, &MinorVersion)) { 
		vidmode_ext = false;
	} else {
		Con_Printf("Using XFree86-VidModeExtension Version %d.%d\n", MajorVersion, MinorVersion);
		vidmode_ext = true;
	}

	visinfo = glXChooseVisual(dpy, scrnum, attrib);
	if (!visinfo) {
		fprintf(stderr, "qkHack: Error couldn't get an RGB, Double-buffered, Depth visual\n");
		exit(1);
	}

	if (vidmode_ext) {
		int best_fit, best_dist, dist, x, y;
		
		XF86VidModeGetAllModeLines(dpy, scrnum, &num_vidmodes, &vidmodes);

		// Are we going fullscreen?  If so, let's change video mode
		if (fullscreen) {
			best_dist = 9999999;
			best_fit = -1;

			for (i = 0; i < num_vidmodes; i++) {
				if (width > vidmodes[i]->hdisplay ||
					height > vidmodes[i]->vdisplay)
					continue;

				x = width - vidmodes[i]->hdisplay;
				y = height - vidmodes[i]->vdisplay;
				dist = (x * x) + (y * y);
				if (dist < best_dist) {
					best_dist = dist;
					best_fit = i;
				}
			}

			if (best_fit != -1) {
				actualWidth = vidmodes[best_fit]->hdisplay;
				actualHeight = vidmodes[best_fit]->vdisplay;

				// change to the mode
				XF86VidModeSwitchToMode(dpy, scrnum, vidmodes[best_fit]);
				vidmode_active = true;

				// Move the viewport to top left
				XF86VidModeSetViewPort(dpy, scrnum, 0, 0);
			} else
				fullscreen = 0;
		}
	}

	/* window attributes */
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
	attr.event_mask = X_MASK;
	if (vidmode_active) {
		mask = CWBackPixel | CWColormap | CWSaveUnder | CWBackingStore | 
			CWEventMask | CWOverrideRedirect;
		attr.override_redirect = True;
		attr.backing_store = NotUseful;
		attr.save_under = False;
	} else
		mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	win = XCreateWindow(dpy, root, 0, 0, width, height,
						0, visinfo->depth, InputOutput,
						visinfo->visual, mask, &attr);
	XMapWindow(dpy, win);

    if (vidmode_active) {
		XMoveWindow(dpy, win, 0, 0);
		XRaiseWindow(dpy, win);
		XWarpPointer(dpy, None, win, 0, 0, 0, 0, 0, 0);
		XFlush(dpy);
		// Move the viewport to top left
		XF86VidModeSetViewPort(dpy, scrnum, 0, 0);
	}

	XFlush(dpy);

	ctx = glXCreateContext(dpy, visinfo, NULL, True);

	glXMakeCurrent(dpy, win, ctx);

	scr_width = width;
	scr_height = height;

	if (vid.conheight > height)
		vid.conheight = height;
	if (vid.conwidth > width)
		vid.conwidth = width;
	vid.width = vid.conwidth;
	vid.height = vid.conheight;

    vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
	vid.numpages = 2;

	InitSig(); // trap evil signals

    g_GLRenderer->GL_Init();

	sprintf (gldir, "%s/glquake", g_Common->com_gamedir);
	Sys_mkdir (gldir);

    VID_SetPalette(palette);

	// Check for 3DFX Extensions and initialize them.
    //VID_Init8bitPalette();

	Con_SafePrintf ("Video mode %dx%d initialized.\n", width, height);

    vid.recalc_refdef = 1;				// force a surface cache flush
}

void Sys_SendKeyEvents(void)
{
	HandleEvents();
}

void Force_CenterView_f (void)
{
	cl.viewangles[PITCH] = 0;
}

void IN_Init(void)
{
    Cvar_RegisterVariable(&x11_grab_mouse);

    if (g_Common->COM_CheckParm("-nomouse") == 0)
    {
        mouse_avail = true;
        IN_ActivateMouse();
    }
}

void IN_Shutdown(void)
{
}

/*
===========
IN_Commands
===========
*/
void IN_Commands (void)
{
	if (!dpy || !win)
		return;

    if (vidmode_active && key_dest == key_game && g_Common->COM_CheckParm("-nomouse") == 0) // Missi: used to be an || statement, but we need this to have full control if we want to use the mouse (6/23/2024)
		IN_ActivateMouse();
	else
		IN_DeactivateMouse ();
}

/*
===========
IN_Move
===========
*/
void IN_MouseMove (usercmd_t *cmd)
{
	if (!mouse_avail)
		return;
   
	if (m_filter.value)
	{
		mx = (mx + old_mouse_x) * 0.5;
		my = (my + old_mouse_y) * 0.5;
	}
	old_mouse_x = mx;
	old_mouse_y = my;

	mx *= sensitivity.value;
	my *= sensitivity.value;

// add mouse X/Y movement to cmd
    if ( (in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1) ))
		cmd->sidemove += m_side.value * mx;
	else
		cl.viewangles[YAW] -= m_yaw.value * mx;
	
	if (in_mlook.state & 1)
		V_StopPitchDrift ();
		
	if ( (in_mlook.state & 1) && !(in_strafe.state & 1))
	{
		cl.viewangles[PITCH] += m_pitch.value * my;
		if (cl.viewangles[PITCH] > 80)
			cl.viewangles[PITCH] = 80;
		if (cl.viewangles[PITCH] < -70)
			cl.viewangles[PITCH] = -70;
	}
	else
	{
		if ((in_strafe.state & 1) && noclip_anglehack)
			cmd->upmove -= m_forward.value * my;
		else
			cmd->forwardmove -= m_forward.value * my;
	}
	mx = my = 0;
}

void IN_Move (usercmd_t *cmd)
{
	IN_MouseMove(cmd);
}


