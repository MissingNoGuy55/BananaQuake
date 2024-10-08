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

// draw.h -- these are the only functions outside the refresh allowed
// to touch the vid buffer

// Missi: no longer needed (4/24/2023)
// extern	CQuakePic *draw_disc;	// also used on sbar

class CSoftwareRenderer : public CCoreRenderer
{
public:

	CSoftwareRenderer();

	void Draw_Init();
	void Draw_Character(int x, int y, int num);
	void Draw_DebugChar(char num);
	void Draw_Pic(int x, int y, CQuakePic* tex);
	void Draw_TransPic(int x, int y, CQuakePic* tex);
	void Draw_TransPicTranslate(int x, int y, CQuakePic* tex, byte* translation);
	void Draw_CharToConback(int num, byte* dest);
	void Draw_ConsoleBackground(int lines);
	void R_DrawRect8(vrect_t* prect, int rowbytes, byte* psrc, int transparent);
	void R_DrawRect16(vrect_t* prect, int rowbytes, byte* psrc, int transparent);
	void Draw_BeginDisc();
	void Draw_EndDisc();
	void Draw_TileClear(int x, int y, int w, int h);
	void Draw_Fill(int x, int y, int w, int h, int c);
	void Draw_FadeScreen();
	void Draw_String(int x, int y, const char* str);
	CQuakePic* Draw_PicFromWad(const char* name);
	CQuakePic* Draw_CachePic(const char* path);

	void R_InitSky(struct texture_s* mt);
	void D_DrawSkyScans8(struct espan_s* pspan);
	void Turbulent8(struct espan_s* pspan);
	void D_DrawZSpans(struct espan_s* pspan);

	void R_MakeSky();

	byte* draw_chars;				// 8*8 graphic characters
	CQuakePic* draw_disc;
	CQuakePic* draw_backtile;

	CQuakePic* GetLoadingDisc() const { return draw_disc; }
	CQuakePic* GetBackTile() const { return draw_backtile; }

private:

	CSoftwareRenderer(const CSoftwareRenderer& src);

};

//byte* draw_chars;				// 8*8 graphic characters
//CQuakePic* draw_disc;
//CQuakePic* draw_backtile;

extern CSoftwareRenderer* g_SoftwareRenderer;
