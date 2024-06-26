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
// vid_sdl_software.c -- general x video driver

#define _BSD


#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>

#include <X11/extensions/Xxf86dga.h>    // Missi (4/30/2023)
#include <X11/extensions/xf86vmode.h>

#include "quakedef.h"
#include "d_local.h"

cvar_t		_windowed_mouse = {"_windowed_mouse","0", true};
cvar_t		m_filter = {"m_filter","0", true};
float old_windowed_mouse;

bool        mouse_avail;
int             mouse_buttons=3;
int             mouse_oldbuttonstate;
int             mouse_buttonstate;
float   mouse_x, mouse_y;
float   old_mouse_x, old_mouse_y;
int p_mouse_x;
int p_mouse_y;
int ignorenext;
int bits_per_pixel;

SDL_Renderer* main_renderer;
SDL_Window* main_window;
SDL_Rect main_rect;
SDL_Surface* main_surface;
SDL_Texture* main_texture;

typedef struct
{
	int input;
	int output;
} keymap_t;

unsigned short d_8to16table[256];

int		num_shades=32;

int	d_con_indirect = 0;

int		vid_buffersize;

static bool			doShm;
static Display			*x_disp;
static Colormap			x_cmap;
static Window			x_win;
static GC				x_gc;
static Visual			*x_vis;
static XVisualInfo		*x_visinfo;
//static XImage			*x_image;

static int				x_shmeventtype;
//static XShmSegmentInfo	x_shminfo;

static bool			oktodraw = false;

int XShmQueryExtension(Display *);
int XShmGetEventBase(Display *);

int current_framebuffer;
static SDL_Surface             *framebuffer[2] = { 0, 0 };

static int verbose=0;

static byte current_palette[768];

static long X11_highhunkmark;
static long X11_buffersize;

int vid_surfcachesize;
void *vid_surfcache;

void (*vid_menudrawfn)(void);
void (*vid_menukeyfn)(int key);
void VID_MenuKey (int key);

typedef unsigned short PIXEL16;
typedef unsigned long PIXEL24;
static PIXEL16 st2d_8to16table[256];
static PIXEL24 st2d_8to24table[256];
static int shiftmask_fl=0;
static long r_shift,g_shift,b_shift;
static unsigned long r_mask,g_mask,b_mask;

void shiftmask_init()
{
    unsigned int x;
    r_mask=1;
    g_mask=1;
    b_mask=1;
    for(r_shift=-8,x=1;x<r_mask;x=x<<1)r_shift++;
    for(g_shift=-8,x=1;x<g_mask;x=x<<1)g_shift++;
    for(b_shift=-8,x=1;x<b_mask;x=x<<1)b_shift++;
    shiftmask_fl=1;
}

PIXEL16 xlib_rgb16(int r,int g,int b)
{
    PIXEL16 p;
    if(shiftmask_fl==0) shiftmask_init();
    p=0;

    if(r_shift>0) {
        p=(r<<(r_shift))&r_mask;
    } else if(r_shift<0) {
        p=(r>>(-r_shift))&r_mask;
    } else p|=(r&r_mask);

    if(g_shift>0) {
        p|=(g<<(g_shift))&g_mask;
    } else if(g_shift<0) {
        p|=(g>>(-g_shift))&g_mask;
    } else p|=(g&g_mask);

    if(b_shift>0) {
        p|=(b<<(b_shift))&b_mask;
    } else if(b_shift<0) {
        p|=(b>>(-b_shift))&b_mask;
    } else p|=(b&b_mask);

    return p;
}

PIXEL24 xlib_rgb24(int r,int g,int b)
{
    PIXEL24 p;
    if(shiftmask_fl==0) shiftmask_init();
    p=0;

    if(r_shift>0) {
        p=(r<<(r_shift))&r_mask;
    } else if(r_shift<0) {
        p=(r>>(-r_shift))&r_mask;
    } else p|=(r&r_mask);

    if(g_shift>0) {
        p|=(g<<(g_shift))&g_mask;
    } else if(g_shift<0) {
        p|=(g>>(-g_shift))&g_mask;
    } else p|=(g&g_mask);

    if(b_shift>0) {
        p|=(b<<(b_shift))&b_mask;
    } else if(b_shift<0) {
        p|=(b>>(-b_shift))&b_mask;
    } else p|=(b&b_mask);

    return p;
}

void st2_fixup( XImage *framebuf, int x, int y, int width, int height)
{
	int xi,yi;
    char *src;
	PIXEL16 *dest;
    int count, n;

	if( (x<0)||(y<0) )return;

	for (yi = y; yi < (y+height); yi++) {
		src = &framebuf->data [yi * framebuf->bytes_per_line];

		// Duff's Device
		count = width;
		n = (count + 7) / 8;
		dest = ((PIXEL16 *)src) + x+width - 1;
		src += x+width - 1;

		switch (count % 8) {
		case 0:	do {	*dest-- = st2d_8to16table[*src--];
		case 7:			*dest-- = st2d_8to16table[*src--];
		case 6:			*dest-- = st2d_8to16table[*src--];
		case 5:			*dest-- = st2d_8to16table[*src--];
		case 4:			*dest-- = st2d_8to16table[*src--];
		case 3:			*dest-- = st2d_8to16table[*src--];
		case 2:			*dest-- = st2d_8to16table[*src--];
		case 1:			*dest-- = st2d_8to16table[*src--];
				} while (--n > 0);
		}

//		for(xi = (x+width-1); xi >= x; xi--) {
//			dest[xi] = st2d_8to16table[src[xi]];
//		}
	}
}

void st3_fixup( XImage *framebuf, int x, int y, int width, int height)
{
	int xi,yi;
    char *src;
	PIXEL24 *dest;
    int count, n;

	if( (x<0)||(y<0) )return;

	for (yi = y; yi < (y+height); yi++) {
		src = &framebuf->data [yi * framebuf->bytes_per_line];

		// Duff's Device
		count = width;
		n = (count + 7) / 8;
		dest = ((PIXEL24 *)src) + x+width - 1;
		src += x+width - 1;

		switch (count % 8) {
		case 0:	do {	*dest-- = st2d_8to24table[*src--];
		case 7:			*dest-- = st2d_8to24table[*src--];
		case 6:			*dest-- = st2d_8to24table[*src--];
		case 5:			*dest-- = st2d_8to24table[*src--];
		case 4:			*dest-- = st2d_8to24table[*src--];
		case 3:			*dest-- = st2d_8to24table[*src--];
		case 2:			*dest-- = st2d_8to24table[*src--];
		case 1:			*dest-- = st2d_8to24table[*src--];
				} while (--n > 0);
		}

//		for(xi = (x+width-1); xi >= x; xi--) {
//			dest[xi] = st2d_8to16table[src[xi]];
//		}
	}
}


// ========================================================================
// Tragic death handler
// ========================================================================

void TragicDeath(int signal_num)
{
	Sys_Error("This death brought to you by the number %d\n", signal_num);
}

// ========================================================================
// makes a null cursor
// ========================================================================

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

void ResetFrameBuffer(void)
{
    int mem;
	int pwidth;
    byte* buf;

    if (framebuffer[0])
	{
        free(framebuffer[0]->pixels);
        free(framebuffer[0]);
	}

	if (d_pzbuffer)
	{
		D_FlushCaches ();
        g_MemCache->Hunk_FreeToHighMark (X11_highhunkmark);
		d_pzbuffer = NULL;
	}
    X11_highhunkmark = g_MemCache->Hunk_HighMark ();

// alloc an extra line in case we want to wrap, and allocate the z-buffer
	X11_buffersize = vid.width * vid.height * sizeof (*d_pzbuffer);

	vid_surfcachesize = D_SurfaceCacheForRes (vid.width, vid.height);

	X11_buffersize += vid_surfcachesize;

    d_pzbuffer = g_MemCache->Hunk_HighAllocName<short>(X11_buffersize, "video");
	if (d_pzbuffer == NULL)
		Sys_Error ("Not enough memory for video mode\n");

	vid_surfcache = (byte *) d_pzbuffer
		+ vid.width * vid.height * sizeof (*d_pzbuffer);

	D_InitCaches(vid_surfcache, vid_surfcachesize);

	pwidth = x_visinfo->depth / 8;
	if (pwidth == 3) pwidth = 4;
    mem = ((vid.width*pwidth+7)&~7) * vid.height;

    buf = (byte*)malloc(mem);

    /*x_framebuffer[0] = XCreateImage(	x_disp,
		x_vis,
		x_visinfo->depth,
		ZPixmap,
		0,
        buf,
		vid.width, vid.height,
		32,
		0);

	if (!x_framebuffer[0])
		Sys_Error("VID: XCreateImage failed\n");

    vid.buffer = (x_framebuffer[0]);
    vid.conbuffer = vid.buffer;*/

}

void ResetSharedFrameBuffers(void)
{

	int size;
	int key;
	int minsize = getpagesize();
	int frm;

	if (d_pzbuffer)
	{
		D_FlushCaches ();
        g_MemCache->Hunk_FreeToHighMark (X11_highhunkmark);
		d_pzbuffer = NULL;
	}

    X11_highhunkmark = g_MemCache->Hunk_HighMark ();

// alloc an extra line in case we want to wrap, and allocate the z-buffer
	X11_buffersize = vid.width * vid.height * sizeof (*d_pzbuffer);

	vid_surfcachesize = D_SurfaceCacheForRes (vid.width, vid.height);

	X11_buffersize += vid_surfcachesize;

    d_pzbuffer = g_MemCache->Hunk_HighAllocName<short>(X11_buffersize, "video");
	if (d_pzbuffer == NULL)
		Sys_Error ("Not enough memory for video mode\n");

	vid_surfcache = (byte *) d_pzbuffer
		+ vid.width * vid.height * sizeof (*d_pzbuffer);

	D_InitCaches(vid_surfcache, vid_surfcachesize);

	for (frm=0 ; frm<2 ; frm++)
	{

	// free up old frame buffer memory

        if (framebuffer[frm])
		{
            free(framebuffer[frm]);
		}

	// create the image

        framebuffer[frm] = SDL_CreateRGBSurfaceFrom(framebuffer, vid.width, vid.height, 32, 0, 0, 0, 0, 0);

	}

}

// Called at startup to set up translation tables, takes 256 8 bit RGB values
// the palette data will go away after the call, so it must be copied off if
// the video driver will need it again

void	VID_Init (unsigned char *palette)
{

   int pnum, i;
   XVisualInfo templ;
   int num_visuals;
   int template_mask;

   if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
   {
       Sys_Error("Failed to init SDL_VIDEO\n");
       return;
   }
   
   ignorenext=0;
   vid.width = 320;
   vid.height = 200;
   vid.maxwarpwidth = WARP_WIDTH;
   vid.maxwarpheight = WARP_HEIGHT;
   vid.numpages = 2;
   vid.colormap = host->host_colormap;
   //	vid.cbits = VID_CBITS;
   //	vid.grades = VID_GRADES;
   vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
   
	srandom(getpid());

    verbose=g_Common->COM_CheckParm("-verbose");

// check for command-line window size
    if ((pnum=g_Common->COM_CheckParm("-winsize")))
	{
        if (pnum >= g_Common->com_argc-2)
			Sys_Error("VID: -winsize <width> <height>\n");
        vid.width = Q_atoi(g_Common->com_argv[pnum+1]);
        vid.height = Q_atoi(g_Common->com_argv[pnum+2]);
		if (!vid.width || !vid.height)
			Sys_Error("VID: Bad window width/height\n");
	}
    if ((pnum=g_Common->COM_CheckParm("-width"))) {
        if (pnum >= g_Common->com_argc-1)
			Sys_Error("VID: -width <width>\n");
        vid.width = Q_atoi(g_Common->com_argv[pnum+1]);
		if (!vid.width)
			Sys_Error("VID: Bad window width\n");
	}
    if ((pnum=g_Common->COM_CheckParm("-height"))) {
        if (pnum >= g_Common->com_argc-1)
			Sys_Error("VID: -height <height>\n");
        vid.height = Q_atoi(g_Common->com_argv[pnum+1]);
		if (!vid.height)
			Sys_Error("VID: Bad window height\n");
	}

    main_window = SDL_CreateWindow("BananaQuake", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, vid.width, vid.height, SDL_WINDOW_SHOWN);
    main_renderer = SDL_CreateRenderer(main_window, -1, SDL_RENDERER_SOFTWARE);

    int ret = -1;

    // Set render color to red ( background will be rendered in this color )
    ret = SDL_SetRenderDrawColor( main_renderer, 0, 0, 0, 255 );

    SDL_RenderClear( main_renderer );

    // Creat a rect at pos ( 50, 50 ) that's 50 pixels wide and 50 pixels high.
    main_rect.x = 0;
    main_rect.y = 0;
    main_rect.w = vid.width;
    main_rect.h = vid.height;

    // Set render color to blue ( rect will be rendered in this color )
    SDL_SetRenderDrawColor( main_renderer, 0, 0, 255, 255 );

    SDL_RenderClear( main_renderer );

    // Render rect
    //SDL_RenderFillRect( main_renderer, &main_rect );

    if (ret < 0)
        Sys_Error("Failed to draw SDL rectangle: %s\n", SDL_GetError());

	current_framebuffer = 0;

    framebuffer[current_framebuffer] = SDL_CreateRGBSurface(0, vid.width, vid.height, 8, 0, 0, 0, 0);

    vid.rowbytes = framebuffer[0]->refcount * vid.width;
    vid.buffer = (byte*)framebuffer[0]->pixels;
	vid.direct = 0;
    vid.conbuffer = (byte*)framebuffer[0]->pixels;
    vid.conrowbytes = vid.rowbytes;
	vid.conwidth = vid.width;
	vid.conheight = vid.height;
    vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);

    static uint32_t* test = new uint32_t[vid.width * vid.height];

    if (test)
    {
        delete[] test;
        test = new uint32_t[vid.width * vid.height];
    }

    byte* test2 = COM_LoadTempFile<byte>("krabs.bmp", NULL);

    Q_memcpy(test, test2, vid.width * vid.height);

    main_texture = SDL_CreateTexture(main_renderer, SDL_PIXELFORMAT_INDEX8, SDL_TEXTUREACCESS_STATIC, vid.width, vid.height);

    SDL_Rect dstrect;
    dstrect.x = 0;
    dstrect.y = 0;
    dstrect.w = 50;
    dstrect.h = 50;

    SDL_UpdateTexture(main_texture, NULL, test2, vid.width * sizeof (uint8_t));

    SDL_RenderCopy(main_renderer, main_texture, NULL, &dstrect);
}

void VID_ShiftPalette(unsigned char *p)
{
	VID_SetPalette(p);
}



void VID_SetPalette(unsigned char *palette)
{

	int i;
    SDL_Color colors[256];

	for(i=0;i<256;i++) {
		st2d_8to16table[i]= xlib_rgb16(palette[i*3], palette[i*3+1],palette[i*3+2]);
		st2d_8to24table[i]= xlib_rgb24(palette[i*3], palette[i*3+1],palette[i*3+2]);
	}

    if (palette != current_palette)
        memcpy(current_palette, palette, 768);
    for (i=0 ; i<256 ; i++)
    {
        colors[i].r = palette[i*3] * 257;
        colors[i].g = palette[i*3+1] * 257;
        colors[i].b = palette[i*3+2] * 257;
    }
}

// Called at shutdown

void	VID_Shutdown (void)
{
	Con_Printf("VID_Shutdown\n");
}

int XLateKey(XKeyEvent *ev)
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

		case XK_KP_Begin: key = K_AUX30;	break;

		case XK_Insert:
		case XK_KP_Insert: key = K_INS; break;

		case XK_KP_Multiply: key = '*'; break;
		case XK_KP_Add: key = '+'; break;
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
//			fprintf(stdout, "case 0x0%x: key = ___;break;/* [%c] */\n", keysym);
			break;
	} 

	return key;
}

struct
{
	int key;
	int down;
} keyq[64];
int keyq_head=0;
int keyq_tail=0;

int config_notify=0;
int config_notify_width;
int config_notify_height;
						      
void GetEvent(void)
{
	XEvent x_event;
	int b;
   
	XNextEvent(x_disp, &x_event);
	switch(x_event.type) {
	case KeyPress:
		keyq[keyq_head].key = XLateKey(&x_event.xkey);
		keyq[keyq_head].down = true;
		keyq_head = (keyq_head + 1) & 63;
		break;
	case KeyRelease:
		keyq[keyq_head].key = XLateKey(&x_event.xkey);
		keyq[keyq_head].down = false;
		keyq_head = (keyq_head + 1) & 63;
		break;

	case MotionNotify:
		if (_windowed_mouse.value) {
			mouse_x = (float) ((int)x_event.xmotion.x - (int)(vid.width/2));
			mouse_y = (float) ((int)x_event.xmotion.y - (int)(vid.height/2));
//printf("m: x=%d,y=%d, mx=%3.2f,my=%3.2f\n", 
//	x_event.xmotion.x, x_event.xmotion.y, mouse_x, mouse_y);

			/* move the mouse to the window center again */
			XSelectInput(x_disp,x_win,StructureNotifyMask|KeyPressMask
				|KeyReleaseMask|ExposureMask
				|ButtonPressMask
				|ButtonReleaseMask);
			XWarpPointer(x_disp,None,x_win,0,0,0,0, 
				(vid.width/2),(vid.height/2));
			XSelectInput(x_disp,x_win,StructureNotifyMask|KeyPressMask
				|KeyReleaseMask|ExposureMask
				|PointerMotionMask|ButtonPressMask
				|ButtonReleaseMask);
		} else {
			mouse_x = (float) (x_event.xmotion.x-p_mouse_x);
			mouse_y = (float) (x_event.xmotion.y-p_mouse_y);
			p_mouse_x=x_event.xmotion.x;
			p_mouse_y=x_event.xmotion.y;
		}
		break;

	case ButtonPress:
		b=-1;
		if (x_event.xbutton.button == 1)
			b = 0;
		else if (x_event.xbutton.button == 2)
			b = 2;
		else if (x_event.xbutton.button == 3)
			b = 1;
		if (b>=0)
			mouse_buttonstate |= 1<<b;
		break;

	case ButtonRelease:
		b=-1;
		if (x_event.xbutton.button == 1)
			b = 0;
		else if (x_event.xbutton.button == 2)
			b = 2;
		else if (x_event.xbutton.button == 3)
			b = 1;
		if (b>=0)
			mouse_buttonstate &= ~(1<<b);
		break;
	
	case ConfigureNotify:
//printf("config notify\n");
		config_notify_width = x_event.xconfigure.width;
		config_notify_height = x_event.xconfigure.height;
		config_notify = 1;
		break;

	default:
		if (doShm && x_event.type == x_shmeventtype)
			oktodraw = true;
	}
   
	if (old_windowed_mouse != _windowed_mouse.value) {
		old_windowed_mouse = _windowed_mouse.value;

		if (!_windowed_mouse.value) {
			/* ungrab the pointer */
			XUngrabPointer(x_disp,CurrentTime);
		} else {
			/* grab the pointer */
			XGrabPointer(x_disp,x_win,True,0,GrabModeAsync,
				GrabModeAsync,x_win,None,CurrentTime);
		}
	}
}

// flushes the given rectangles from the view buffer to the screen

void	VID_Update (vrect_t *rects)
{
    SDL_Rect full;

// if the window changes dimension, skip this frame

	if (config_notify)
	{
		fprintf(stderr, "config notify\n");
		config_notify = 0;
		vid.width = config_notify_width & ~7;
		vid.height = config_notify_height;
		if (doShm)
			ResetSharedFrameBuffers();
		else
			ResetFrameBuffer();
        vid.rowbytes = main_surface->refcount;
        vid.buffer = (byte*)main_surface->pixels;
		vid.conbuffer = vid.buffer;
		vid.conwidth = vid.width;
		vid.conheight = vid.height;
		vid.conrowbytes = vid.rowbytes;
		vid.recalc_refdef = 1;				// force a surface cache flush
		Con_CheckResize();
		Con_Clear_f();
		return;
	}

	if (doShm)
	{

		while (rects)
		{

			oktodraw = false;
			while (!oktodraw) GetEvent();
			rects = rects->pnext;
		}
		current_framebuffer = !current_framebuffer;
        vid.buffer = (byte*)framebuffer[current_framebuffer];
		vid.conbuffer = vid.buffer;
        SDL_RenderPresent(main_renderer);
	}
	else
	{
		while (rects)
        {
            SDL_RenderPresent(main_renderer);

			rects = rects->pnext;
		}
        SDL_RenderPresent(main_renderer);
	}

}

static int dither;

void VID_DitherOn(void)
{
    if (dither == 0)
    {
		vid.recalc_refdef = 1;
        dither = 1;
    }
}

void VID_DitherOff(void)
{
    if (dither)
    {
		vid.recalc_refdef = 1;
        dither = 0;
    }
}

int Sys_OpenWindow(void)
{
	return 0;
}

void Sys_EraseWindow(int window)
{
}

void Sys_DrawCircle(int window, int x, int y, int r)
{
}

void Sys_DisplayWindow(int window)
{
}

void Sys_SendKeyEvents(void)
{
// get events from x server
	if (x_disp)
	{
		while (XPending(x_disp)) GetEvent();
		while (keyq_head != keyq_tail)
		{
			Key_Event(keyq[keyq_tail].key, keyq[keyq_tail].down);
			keyq_tail = (keyq_tail + 1) & 63;
		}
	}
}

#if 0
char *Sys_ConsoleInput (void)
{

	static char	text[256];
	int		len;
	fd_set  readfds;
	int		ready;
	struct timeval timeout;

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	FD_ZERO(&readfds);
	FD_SET(0, &readfds);
	ready = select(1, &readfds, 0, 0, &timeout);

	if (ready>0)
	{
		len = read (0, text, sizeof(text));
		if (len >= 1)
		{
			text[len-1] = 0;	// rip off the /n and terminate
			return text;
		}
	}

	return 0;
	
}
#endif

void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
// direct drawing of the "accessing disk" icon isn't supported under Linux
}

void D_EndDirectRect (int x, int y, int width, int height)
{
// direct drawing of the "accessing disk" icon isn't supported under Linux
}

void IN_Init (void)
{
	Cvar_RegisterVariable (&_windowed_mouse);
	Cvar_RegisterVariable (&m_filter);
   if ( g_Common->COM_CheckParm ("-nomouse") )
     return;
   mouse_x = mouse_y = 0.0;
   mouse_avail = 1;
}

void IN_Shutdown (void)
{
   mouse_avail = 0;
}

void IN_Commands (void)
{
	int i;
   
	if (!mouse_avail) return;
   
	for (i=0 ; i<mouse_buttons ; i++) {
		if ( (mouse_buttonstate & (1<<i)) && !(mouse_oldbuttonstate & (1<<i)) )
			Key_Event (K_MOUSE1 + i, true);

		if ( !(mouse_buttonstate & (1<<i)) && (mouse_oldbuttonstate & (1<<i)) )
			Key_Event (K_MOUSE1 + i, false);
	}
	mouse_oldbuttonstate = mouse_buttonstate;
}

void IN_Move (usercmd_t *cmd)
{
	if (!mouse_avail)
		return;
   
	if (m_filter.value) {
		mouse_x = (mouse_x + old_mouse_x) * 0.5;
		mouse_y = (mouse_y + old_mouse_y) * 0.5;
	}

	old_mouse_x = mouse_x;
	old_mouse_y = mouse_y;
   
	mouse_x *= sensitivity.value;
	mouse_y *= sensitivity.value;
   
	if ( (in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1) ))
		cmd->sidemove += m_side.value * mouse_x;
	else
		cl.viewangles[YAW] -= m_yaw.value * mouse_x;
	if (in_mlook.state & 1)
		V_StopPitchDrift ();
   
	if ( (in_mlook.state & 1) && !(in_strafe.state & 1)) {
		cl.viewangles[PITCH] += m_pitch.value * mouse_y;
		if (cl.viewangles[PITCH] > 80)
			cl.viewangles[PITCH] = 80;
		if (cl.viewangles[PITCH] < -70)
			cl.viewangles[PITCH] = -70;
	} else {
		if ((in_strafe.state & 1) && noclip_anglehack)
			cmd->upmove -= m_forward.value * mouse_y;
		else
			cmd->forwardmove -= m_forward.value * mouse_y;
	}
	mouse_x = mouse_y = 0.0;
}
