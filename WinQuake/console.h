/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2021-2024 Stephen "Missi" Schimedeberg

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

#if defined(__linux__) || defined(__CYGWIN__)
#define TEXT_COLOR_DEFAULT "\x1B[0m"
#define TEXT_COLOR_RED "\x1B[31m"
#define TEXT_COLOR_GREEN "\x1B[32m"
#define TEXT_COLOR_YELLOW "\x1B[33m"
#define TEXT_COLOR_ORANGE "\x1B[38;5;208m"
#define TEXT_COLOR_CYAN "\x1B[38;5;51m"
#elif _WIN32
#define TEXT_COLOR_RED		"1"
#define TEXT_COLOR_GREEN	"2"
#define TEXT_COLOR_YELLOW	"3"
#define TEXT_COLOR_CYAN     "14"
#endif

//
// console
//
extern int con_totallines;
extern int con_backscroll;
extern	bool con_forcedup;	// because no entities to refresh
extern bool con_initialized;
extern byte *con_chars;
extern	int	con_notifylines;		// scan lines to clear for notify lines

void Con_DrawCharacter (int cx, int line, int num);

void Con_CheckResize ();
void Con_Init ();
void Con_DrawConsole (int lines, bool drawinput);
void Con_Print (const char *txt);
void Con_Printf (const char *fmt, ...);
void Con_Warning(const char *fmt, ...);

void Con_PrintColor(const char* color, const char* fmt, ...);

void Con_DPrintf (const char *fmt, ...);
void Con_SafePrintf (const char *fmt, ...);
void Con_Clear_f ();
void Con_DrawNotify ();
void Con_ClearNotify ();
void Con_ToggleConsole_f ();

void Con_NotifyBox (const char *text);	// during startup for sound / cd warnings

