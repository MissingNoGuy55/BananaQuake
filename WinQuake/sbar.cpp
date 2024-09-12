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
// sbar.c -- status bar code

#include "quakedef.h"

int			sb_updates;		// if >= vid.numpages, no update needed

#define STAT_MINUS		10	// num frame for '-' stats digit
CQuakePic		*sb_nums[2][11];
CQuakePic		*sb_colon, *sb_slash;
CQuakePic		*sb_ibar;
CQuakePic		*sb_sbar;
CQuakePic		*sb_scorebar;

CQuakePic      *sb_weapons[7][8];   // 0 is active, 1 is owned, 2-5 are flashes
CQuakePic      *sb_ammo[4];
CQuakePic		*sb_sigil[4];
CQuakePic		*sb_armor[3];
CQuakePic		*sb_items[32];

CQuakePic	*sb_faces[7][2];		// 0 is gibbed, 1 is dead, 2-6 are alive
							// 0 is static, 1 is temporary animation
CQuakePic	*sb_face_invis;
CQuakePic	*sb_face_quad;
CQuakePic	*sb_face_invuln;
CQuakePic	*sb_face_invis_invuln;
CQuakePic	*sb_face_invis_quad;

bool	sb_showscores;

int			sb_lines;			// scan lines to draw

CQuakePic      *rsb_invbar[2];
CQuakePic      *rsb_weapons[5];
CQuakePic      *rsb_items[2];
CQuakePic      *rsb_ammo[3];
CQuakePic      *rsb_teambord;		// PGM 01/19/97 - team color border

//MED 01/04/97 added two more weapons + 3 alternates for grenade launcher
CQuakePic      *hsb_weapons[7][5];   // 0 is active, 1 is owned, 2-5 are flashes
//MED 01/04/97 added array to simplify weapon parsing
long long         hipweapons[4] = {HIT_LASER_CANNON_BIT,HIT_MJOLNIR_BIT,4,HIT_PROXIMITY_GUN_BIT};
//MED 01/04/97 added hipnotic items array
CQuakePic      *hsb_items[2];

void Sbar_MiniDeathmatchOverlay ();
void Sbar_DeathmatchOverlay ();
void M_DrawPic (int x, int y, CQuakePic *pic);

#ifdef __linux__
viddef_t vid;
#endif

/*
===============
Sbar_ShowScores

Tab key down
===============
*/
void Sbar_ShowScores ()
{
	if (sb_showscores)
		return;
	sb_showscores = true;
	sb_updates = 0;
}

/*
===============
Sbar_DontShowScores

Tab key up
===============
*/
void Sbar_DontShowScores ()
{
	sb_showscores = false;
	sb_updates = 0;
}

/*
===============
Sbar_Changed
===============
*/
void Sbar_Changed ()
{
	sb_updates = 0;	// update next frame
}

/*
===============
Sbar_Init
===============
*/
void Sbar_Init ()
{
	int		i;

	for (i=0 ; i<10 ; i++)
	{
        sb_nums[0][i] = ResolveRenderer()->Draw_PicFromWad (g_Common->va("num_%i",i), GFX_WAD);
        sb_nums[1][i] = ResolveRenderer()->Draw_PicFromWad (g_Common->va("anum_%i",i), GFX_WAD);
	}

    sb_nums[0][10] = ResolveRenderer()->Draw_PicFromWad ("num_minus", GFX_WAD);
    sb_nums[1][10] = ResolveRenderer()->Draw_PicFromWad ("anum_minus", GFX_WAD);

    sb_colon = ResolveRenderer()->Draw_PicFromWad ("num_colon", GFX_WAD);
    sb_slash = ResolveRenderer()->Draw_PicFromWad ("num_slash", GFX_WAD);

    sb_weapons[0][0] = ResolveRenderer()->Draw_PicFromWad ("inv_shotgun", GFX_WAD);
    sb_weapons[0][1] = ResolveRenderer()->Draw_PicFromWad ("inv_sshotgun", GFX_WAD);
    sb_weapons[0][2] = ResolveRenderer()->Draw_PicFromWad ("inv_nailgun", GFX_WAD);
    sb_weapons[0][3] = ResolveRenderer()->Draw_PicFromWad ("inv_snailgun", GFX_WAD);
    sb_weapons[0][4] = ResolveRenderer()->Draw_PicFromWad ("inv_rlaunch", GFX_WAD);
    sb_weapons[0][5] = ResolveRenderer()->Draw_PicFromWad ("inv_srlaunch", GFX_WAD);
    sb_weapons[0][6] = ResolveRenderer()->Draw_PicFromWad ("inv_lightng", GFX_WAD);

    sb_weapons[1][0] = ResolveRenderer()->Draw_PicFromWad ("inv2_shotgun", GFX_WAD);
    sb_weapons[1][1] = ResolveRenderer()->Draw_PicFromWad ("inv2_sshotgun", GFX_WAD);
    sb_weapons[1][2] = ResolveRenderer()->Draw_PicFromWad ("inv2_nailgun", GFX_WAD);
    sb_weapons[1][3] = ResolveRenderer()->Draw_PicFromWad ("inv2_snailgun", GFX_WAD);
    sb_weapons[1][4] = ResolveRenderer()->Draw_PicFromWad ("inv2_rlaunch", GFX_WAD);
    sb_weapons[1][5] = ResolveRenderer()->Draw_PicFromWad ("inv2_srlaunch", GFX_WAD);
    sb_weapons[1][6] = ResolveRenderer()->Draw_PicFromWad ("inv2_lightng", GFX_WAD);

	for (i=0 ; i<5 ; i++)
	{
        sb_weapons[2+i][0] = ResolveRenderer()->Draw_PicFromWad (g_Common->va("inva%i_shotgun",i+1), GFX_WAD);
        sb_weapons[2+i][1] = ResolveRenderer()->Draw_PicFromWad (g_Common->va("inva%i_sshotgun",i+1), GFX_WAD);
        sb_weapons[2+i][2] = ResolveRenderer()->Draw_PicFromWad (g_Common->va("inva%i_nailgun",i+1), GFX_WAD);
        sb_weapons[2+i][3] = ResolveRenderer()->Draw_PicFromWad (g_Common->va("inva%i_snailgun",i+1), GFX_WAD);
        sb_weapons[2+i][4] = ResolveRenderer()->Draw_PicFromWad (g_Common->va("inva%i_rlaunch",i+1), GFX_WAD);
        sb_weapons[2+i][5] = ResolveRenderer()->Draw_PicFromWad (g_Common->va("inva%i_srlaunch",i+1), GFX_WAD);
        sb_weapons[2+i][6] = ResolveRenderer()->Draw_PicFromWad (g_Common->va("inva%i_lightng",i+1), GFX_WAD);
	}

    sb_ammo[0] = ResolveRenderer()->Draw_PicFromWad ("sb_shells", GFX_WAD);
    sb_ammo[1] = ResolveRenderer()->Draw_PicFromWad ("sb_nails", GFX_WAD);
    sb_ammo[2] = ResolveRenderer()->Draw_PicFromWad ("sb_rocket", GFX_WAD);
    sb_ammo[3] = ResolveRenderer()->Draw_PicFromWad ("sb_cells", GFX_WAD);

    sb_armor[0] = ResolveRenderer()->Draw_PicFromWad ("sb_armor1", GFX_WAD);
    sb_armor[1] = ResolveRenderer()->Draw_PicFromWad ("sb_armor2", GFX_WAD);
    sb_armor[2] = ResolveRenderer()->Draw_PicFromWad ("sb_armor3", GFX_WAD);

    sb_items[0] = ResolveRenderer()->Draw_PicFromWad ("sb_key1", GFX_WAD);
    sb_items[1] = ResolveRenderer()->Draw_PicFromWad ("sb_key2", GFX_WAD);
    sb_items[2] = ResolveRenderer()->Draw_PicFromWad ("sb_invis", GFX_WAD);
    sb_items[3] = ResolveRenderer()->Draw_PicFromWad ("sb_invuln", GFX_WAD);
    sb_items[4] = ResolveRenderer()->Draw_PicFromWad ("sb_suit", GFX_WAD);
    sb_items[5] = ResolveRenderer()->Draw_PicFromWad ("sb_quad", GFX_WAD);

    sb_sigil[0] = ResolveRenderer()->Draw_PicFromWad ("sb_sigil1", GFX_WAD);
    sb_sigil[1] = ResolveRenderer()->Draw_PicFromWad ("sb_sigil2", GFX_WAD);
    sb_sigil[2] = ResolveRenderer()->Draw_PicFromWad ("sb_sigil3", GFX_WAD);
    sb_sigil[3] = ResolveRenderer()->Draw_PicFromWad ("sb_sigil4", GFX_WAD);

    sb_faces[4][0] = ResolveRenderer()->Draw_PicFromWad ("face1", GFX_WAD);
    sb_faces[4][1] = ResolveRenderer()->Draw_PicFromWad ("face_p1", GFX_WAD);
    sb_faces[3][0] = ResolveRenderer()->Draw_PicFromWad ("face2", GFX_WAD);
    sb_faces[3][1] = ResolveRenderer()->Draw_PicFromWad ("face_p2", GFX_WAD);
    sb_faces[2][0] = ResolveRenderer()->Draw_PicFromWad ("face3", GFX_WAD);
    sb_faces[2][1] = ResolveRenderer()->Draw_PicFromWad ("face_p3", GFX_WAD);
    sb_faces[1][0] = ResolveRenderer()->Draw_PicFromWad ("face4", GFX_WAD);
    sb_faces[1][1] = ResolveRenderer()->Draw_PicFromWad ("face_p4", GFX_WAD);
    sb_faces[0][0] = ResolveRenderer()->Draw_PicFromWad ("face5", GFX_WAD);
    sb_faces[0][1] = ResolveRenderer()->Draw_PicFromWad ("face_p5", GFX_WAD);

    sb_face_invis = ResolveRenderer()->Draw_PicFromWad ("face_invis", GFX_WAD);
    sb_face_invuln = ResolveRenderer()->Draw_PicFromWad ("face_invul2", GFX_WAD);
    sb_face_invis_invuln = ResolveRenderer()->Draw_PicFromWad ("face_inv2", GFX_WAD);
	sb_face_invis_quad = ResolveRenderer()->Draw_PicFromWad("face_inv3", GFX_WAD);
    sb_face_quad = ResolveRenderer()->Draw_PicFromWad ("face_quad", GFX_WAD);

	g_pCmds->Cmd_AddCommand ("+showscores", Sbar_ShowScores);
	g_pCmds->Cmd_AddCommand ("-showscores", Sbar_DontShowScores);

    sb_sbar = ResolveRenderer()->Draw_PicFromWad ("sbar", GFX_WAD);
    sb_ibar = ResolveRenderer()->Draw_PicFromWad ("ibar", GFX_WAD);
    sb_scorebar = ResolveRenderer()->Draw_PicFromWad ("scorebar", GFX_WAD);

//MED 01/04/97 added new hipnotic weapons
	if (hipnotic)
	{
      hsb_weapons[0][0] = ResolveRenderer()->Draw_PicFromWad ("inv_laser", GFX_WAD);
      hsb_weapons[0][1] = ResolveRenderer()->Draw_PicFromWad ("inv_mjolnir", GFX_WAD);
      hsb_weapons[0][2] = ResolveRenderer()->Draw_PicFromWad ("inv_gren_prox", GFX_WAD);
      hsb_weapons[0][3] = ResolveRenderer()->Draw_PicFromWad ("inv_prox_gren", GFX_WAD);
      hsb_weapons[0][4] = ResolveRenderer()->Draw_PicFromWad ("inv_prox", GFX_WAD);

      hsb_weapons[1][0] = ResolveRenderer()->Draw_PicFromWad ("inv2_laser", GFX_WAD);
      hsb_weapons[1][1] = ResolveRenderer()->Draw_PicFromWad ("inv2_mjolnir", GFX_WAD);
      hsb_weapons[1][2] = ResolveRenderer()->Draw_PicFromWad ("inv2_gren_prox", GFX_WAD);
      hsb_weapons[1][3] = ResolveRenderer()->Draw_PicFromWad ("inv2_prox_gren", GFX_WAD);
      hsb_weapons[1][4] = ResolveRenderer()->Draw_PicFromWad ("inv2_prox", GFX_WAD);

	  for (i=0 ; i<5 ; i++)
	  {
         hsb_weapons[2+i][0] = ResolveRenderer()->Draw_PicFromWad (g_Common->va("inva%i_laser",i+1), GFX_WAD);
         hsb_weapons[2+i][1] = ResolveRenderer()->Draw_PicFromWad (g_Common->va("inva%i_mjolnir",i+1), GFX_WAD);
         hsb_weapons[2+i][2] = ResolveRenderer()->Draw_PicFromWad (g_Common->va("inva%i_gren_prox",i+1), GFX_WAD);
         hsb_weapons[2+i][3] = ResolveRenderer()->Draw_PicFromWad (g_Common->va("inva%i_prox_gren",i+1), GFX_WAD);
         hsb_weapons[2+i][4] = ResolveRenderer()->Draw_PicFromWad (g_Common->va("inva%i_prox",i+1), GFX_WAD);
	  }

      hsb_items[0] = ResolveRenderer()->Draw_PicFromWad ("sb_wsuit", GFX_WAD);
      hsb_items[1] = ResolveRenderer()->Draw_PicFromWad ("sb_eshld", GFX_WAD);
	}

	if (rogue)
	{
        rsb_invbar[0] = ResolveRenderer()->Draw_PicFromWad ("r_invbar1", GFX_WAD);
        rsb_invbar[1] = ResolveRenderer()->Draw_PicFromWad ("r_invbar2", GFX_WAD);

        rsb_weapons[0] = ResolveRenderer()->Draw_PicFromWad ("r_lava", GFX_WAD);
        rsb_weapons[1] = ResolveRenderer()->Draw_PicFromWad ("r_superlava", GFX_WAD);
        rsb_weapons[2] = ResolveRenderer()->Draw_PicFromWad ("r_gren", GFX_WAD);
        rsb_weapons[3] = ResolveRenderer()->Draw_PicFromWad ("r_multirock", GFX_WAD);
        rsb_weapons[4] = ResolveRenderer()->Draw_PicFromWad ("r_plasma", GFX_WAD);

        rsb_items[0] = ResolveRenderer()->Draw_PicFromWad ("r_shield1", GFX_WAD);
        rsb_items[1] = ResolveRenderer()->Draw_PicFromWad ("r_agrav1", GFX_WAD);

// PGM 01/19/97 - team color border
        rsb_teambord = ResolveRenderer()->Draw_PicFromWad ("r_teambord", GFX_WAD);
// PGM 01/19/97 - team color border

        rsb_ammo[0] = ResolveRenderer()->Draw_PicFromWad ("r_ammolava", GFX_WAD);
        rsb_ammo[1] = ResolveRenderer()->Draw_PicFromWad ("r_ammomulti", GFX_WAD);
        rsb_ammo[2] = ResolveRenderer()->Draw_PicFromWad ("r_ammoplasma", GFX_WAD);
	}
}


//=============================================================================

// drawing routines are relative to the status bar location

/*
=============
Sbar_DrawPic
=============
*/
void Sbar_DrawPic (int x, int y, CQuakePic *pic)
{
	if (cl.gametype == GAME_DEATHMATCH)
		ResolveRenderer()->Draw_Pic(x, y + (vid.height - SBAR_HEIGHT), pic);
	else
		ResolveRenderer()->Draw_Pic (x + ((vid.width - 320)>>1), y + (vid.height-SBAR_HEIGHT), pic);
}

/*
=============
Sbar_DrawTransPic
=============
*/
void Sbar_DrawTransPic (int x, int y, CQuakePic *pic)
{
	if (cl.gametype == GAME_DEATHMATCH)
		ResolveRenderer()->Draw_TransPic (x, y + (vid.height-SBAR_HEIGHT), pic);
	else
		ResolveRenderer()->Draw_TransPic (x + ((vid.width - 320)>>1), y + (vid.height-SBAR_HEIGHT), pic);
}

/*
================
Sbar_DrawCharacter

Draws one solid graphics character
================
*/
void Sbar_DrawCharacter (int x, int y, int num)
{
	if (cl.gametype == GAME_DEATHMATCH)
		ResolveRenderer()->Draw_Character ( x /*+ ((vid.width - 320)>>1) */ + 4 , y + vid.height-SBAR_HEIGHT, num);
	else
		ResolveRenderer()->Draw_Character ( x + ((vid.width - 320)>>1) + 4 , y + vid.height-SBAR_HEIGHT, num);
}

/*
================
Sbar_DrawString
================
*/
void Sbar_DrawString (int x, int y, char *str)
{
	if (cl.gametype == GAME_DEATHMATCH)
		ResolveRenderer()->Draw_String (x /*+ ((vid.width - 320)>>1)*/, y+ vid.height-SBAR_HEIGHT, str);
	else
		ResolveRenderer()->Draw_String (x + ((vid.width - 320)>>1), y+ vid.height-SBAR_HEIGHT, str);
}

/*
=============
Sbar_itoa
=============
*/
int Sbar_itoa (int num, char *buf)
{
	char	*str;
	int		pow10;
	int		dig;

	str = buf;

	if (num < 0)
	{
		*str++ = '-';
		num = -num;
	}

	for (pow10 = 10 ; num >= pow10 ; pow10 *= 10)
	;

	do
	{
		pow10 /= 10;
		dig = num/pow10;
		*str++ = '0'+dig;
		num -= dig*pow10;
	} while (pow10 != 1);

	*str = 0;

	return str-buf;
}


/*
=============
Sbar_DrawNum
=============
*/
void Sbar_DrawNum (int x, int y, int num, int digits, int color)
{
	char			str[12];
	char			*ptr;
	int				l, frame;

	l = Sbar_itoa (num, str);
	ptr = str;
	if (l > digits)
		ptr += (l-digits);
	if (l < digits)
		x += (digits-l)*24;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Sbar_DrawTransPic (x,y,sb_nums[color][frame]);
		x += 24;
		ptr++;
	}
}

//=============================================================================

static int		fragsort[MAX_SCOREBOARD];

static char		scoreboardtext[MAX_SCOREBOARD][20];
static int		scoreboardtop[MAX_SCOREBOARD];
static int		scoreboardbottom[MAX_SCOREBOARD];
static int		scoreboardcount[MAX_SCOREBOARD];
static int		scoreboardlines;

/*
===============
Sbar_SortFrags
===============
*/
void Sbar_SortFrags ()
{
	int		i, j, k;

// sort by frags
	scoreboardlines = 0;
	for (i=0 ; i<cl.maxclients ; i++)
	{
		if (cl.scores[i].name[0])
		{
			fragsort[scoreboardlines] = i;
			scoreboardlines++;
		}
	}

	for (i=0 ; i<scoreboardlines ; i++)
		for (j=0 ; j<scoreboardlines-1-i ; j++)
			if (cl.scores[fragsort[j]].frags < cl.scores[fragsort[j+1]].frags)
			{
				k = fragsort[j];
				fragsort[j] = fragsort[j+1];
				fragsort[j+1] = k;
			}
}

int	Sbar_ColorForMap (int m)
{
	return m < 128 ? m + 8 : m + 8;
}

/*
===============
Sbar_UpdateScoreboard
===============
*/
void Sbar_UpdateScoreboard ()
{
	int		i, k;
	int		top, bottom;
	scoreboard_t	*s;

	Sbar_SortFrags ();

// draw the text
	memset (scoreboardtext, 0, sizeof(scoreboardtext));

	for (i=0 ; i<scoreboardlines; i++)
	{
		k = fragsort[i];
		s = &cl.scores[k];
		snprintf (&scoreboardtext[i][1], sizeof(scoreboardtext[i]), "%3i %s", s->frags, s->name);

		top = s->colors & 0xf0;
		bottom = (s->colors & 15) <<4;
		scoreboardtop[i] = Sbar_ColorForMap (top);
		scoreboardbottom[i] = Sbar_ColorForMap (bottom);
	}
}



/*
===============
Sbar_SoloScoreboard
===============
*/
void Sbar_SoloScoreboard ()
{
	char	str[80];
	int		minutes, seconds, tens, units;
	int		l;

	snprintf (str, sizeof(str), "Monsters:%3i /%3i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
	Sbar_DrawString (8, 4, str);

	snprintf(str, sizeof(str), "Secrets :%3i /%3i", cl.stats[STAT_SECRETS], cl.stats[STAT_TOTALSECRETS]);
	Sbar_DrawString (8, 12, str);

// time
	minutes = cl.time / 60;
	seconds = cl.time - (double)60*minutes; // Missi (11/19/2022)
	tens = seconds / 10;
	units = seconds - 10*tens;
	snprintf (str, sizeof(str), "Time :%3i:%i%i", minutes, tens, units);
	Sbar_DrawString (184, 4, str);

// draw level name
	l = strlen (cl.levelname);
	Sbar_DrawString (232 - l*4, 12, cl.levelname);
}

/*
===============
Sbar_DrawScoreboard
===============
*/
void Sbar_DrawScoreboard ()
{
	Sbar_SoloScoreboard ();
	if (cl.gametype == GAME_DEATHMATCH)
		Sbar_DeathmatchOverlay ();
#if 0
	int		i, j, c;
	int		x, y;
	int		l;
	int		top, bottom;
	scoreboard_t	*s;

	if (cl.gametype != GAME_DEATHMATCH)
	{
		Sbar_SoloScoreboard ();
		return;
	}

	Sbar_UpdateScoreboard ();

	l = scoreboardlines <= 6 ? scoreboardlines : 6;

	for (i=0 ; i<l ; i++)
	{
		x = 20*(i&1);
		y = i/2 * 8;

		s = &cl.scores[fragsort[i]];
		if (!s->name[0])
			continue;

	// draw background
		top = s->colors & 0xf0;
		bottom = (s->colors & 15)<<4;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);

		Draw_Fill ( x*8+10 + ((vid.width - 320)>>1), y + vid.height - SBAR_HEIGHT, 28, 4, top);
		Draw_Fill ( x*8+10 + ((vid.width - 320)>>1), y+4 + vid.height - SBAR_HEIGHT, 28, 4, bottom);

	// draw text
		for (j=0 ; j<20 ; j++)
		{
			c = scoreboardtext[i][j];
			if (c == 0 || c == ' ')
				continue;
			Sbar_DrawCharacter ( (x+j)*8, y, c);
		}
	}
#endif
}

//=============================================================================

/*
===============
Sbar_DrawInventory

Missi: modified to take raw item values as opposed to bit-shifting, for supporting
64-bit item identifiers (9/5/2024)
===============
*/
void Sbar_DrawInventory ()
{
	unsigned long long	i;
	char	num[6];
	float	time;
	int		flashon;

	if (rogue)
	{
		if ( cl.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN )
			Sbar_DrawTransPic (0, -24, rsb_invbar[0]);
		else
			Sbar_DrawTransPic(0, -24, rsb_invbar[1]);
	}
	else
	{
		Sbar_DrawTransPic(0, -24, sb_ibar);
	}

// weapons
	for (i = 0; i < 7; i++)
	{
		if (cl.items & ((unsigned long long)1 << i))
		{
			time = cl.item_gettime[i];
			flashon = (int)((cl.time - time) * 10);
			if (flashon >= 10)
			{
				if (cl.stats[STAT_ACTIVEWEAPON] == (IT_SHOTGUN << i))
					flashon = 1;
				else
					flashon = 0;
			}
			else
				flashon = (flashon % 5) + 2;

			Sbar_DrawTransPic(i * 24, -16, sb_weapons[flashon][i]);

			if (flashon > 1)
				sb_updates = 0;		// force update to remove flash
		}
	}

// MED 01/04/97
// hipnotic weapons
    if (hipnotic)
    {
      int grenadeflashing=0;
      for (i=0 ; i<4 ; i++)
      {
         if (cl.items & (static_cast<long long>(1) << hipweapons[i]) )
         {
            time = cl.item_gettime[hipweapons[i]];
            flashon = (int)((cl.time - time)*10);
            if (flashon >= 10)
            {
               if ( cl.stats[STAT_ACTIVEWEAPON] == (static_cast<long long>(1) << hipweapons[i])  )
                  flashon = 1;
               else
                  flashon = 0;
            }
            else
               flashon = (flashon%5) + 2;

            // check grenade launcher
            if (i==2)
            {
               if (cl.items & HIT_PROXIMITY_GUN)
               {
                  if (flashon)
                  {
                     grenadeflashing = 1;
                     Sbar_DrawPic (96, -16, hsb_weapons[flashon][2]);
                  }
               }
            }
            else if (i==3)
            {
               if (cl.items & (IT_SHOTGUN<<4))
               {
                  if (flashon && !grenadeflashing)
                  {
                     Sbar_DrawPic (96, -16, hsb_weapons[flashon][3]);
                  }
                  else if (!grenadeflashing)
                  {
                     Sbar_DrawPic (96, -16, hsb_weapons[0][3]);
                  }
               }
               else
                  Sbar_DrawPic (96, -16, hsb_weapons[flashon][4]);
            }
            else
               Sbar_DrawPic (176 + (i*24), -16, hsb_weapons[flashon][i]);
            if (flashon > 1)
               sb_updates = 0;      // force update to remove flash
         }
      }
    }

	if (rogue)
	{
    // check for powered up weapon.
		if ( cl.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN )
		{
			for (i=0;i<5;i++)
			{
				if (cl.stats[STAT_ACTIVEWEAPON] == (RIT_LAVA_NAILGUN << i))
				{
					Sbar_DrawPic ((i+2)*24, -16, rsb_weapons[i]);
				}
			}
		}
	}

// ammo counts
	for (i=0 ; i<4 ; i++)
	{
		snprintf (num, sizeof(num), "%3i",cl.stats[STAT_SHELLS+i] );
		if (num[0] != ' ')
			Sbar_DrawCharacter ( (6*i+1)*8 - 2, -24, 18 + num[0] - '0');
		if (num[1] != ' ')
			Sbar_DrawCharacter ( (6*i+2)*8 - 2, -24, 18 + num[1] - '0');
		if (num[2] != ' ')
			Sbar_DrawCharacter ( (6*i+3)*8 - 2, -24, 18 + num[2] - '0');
	}

	flashon = 0;
   // items
   for (i=0 ; i<6 ; i++)
      if (cl.items & (static_cast<long long>(1) << (17+i)))
      {
         time = cl.item_gettime[17+i];
         if (time && time > cl.time - 2 && flashon )
         {  // flash frame
            sb_updates = 0;
         }
         else
         {
         //MED 01/04/97 changed keys
            if (!hipnotic || (i>1))
            {
               Sbar_DrawPic (192 + i*16, -16, sb_items[i]);
            }
         }
         if (time && time > cl.time - 2)
            sb_updates = 0;
      }
   //MED 01/04/97 added hipnotic items
   // hipnotic items
   if (hipnotic)
   {
      for (i=0 ; i<2 ; i++)
         if (cl.items & (static_cast<long long>(1) << (24+i)))
         {
            time = cl.item_gettime[24+i];
            if (time && time > cl.time - 2 && flashon )
            {  // flash frame
               sb_updates = 0;
            }
            else
            {
               Sbar_DrawPic (288 + i*16, -16, hsb_items[i]);
            }
            if (time && time > cl.time - 2)
               sb_updates = 0;
         }
   }

	if (rogue)
	{
	// new rogue items
		for (i=0 ; i<2 ; i++)
		{
			if (cl.items & (static_cast<long long>(1) << (29+i)))
			{
				time = cl.item_gettime[29+i];

				if (time &&	time > cl.time - 2 && flashon )
				{	// flash frame
					sb_updates = 0;
				}
				else
				{
					Sbar_DrawPic (288 + i*16, -16, rsb_items[i]);
				}

				if (time &&	time > cl.time - 2)
					sb_updates = 0;
			}
		}
	}
	else
	{
	// sigils
		for (i=0 ; i<4 ; i++)
		{
			if (cl.items >= IT_SIGIL1 && cl.items <= IT_SIGIL4)		// Missi: used to be 1 << (28+i) (9/5/2024)
			{
				time = cl.item_gettime[28+i];
				if (time &&	time > cl.time - 2 && flashon )
				{	// flash frame
					sb_updates = 0;
				}
				else
					Sbar_DrawPic (320-32 + i*8, -16, sb_sigil[i]);
				if (time &&	time > cl.time - 2)
					sb_updates = 0;
			}
		}
	}
}

//=============================================================================

/*
===============
Sbar_DrawFrags
===============
*/
void Sbar_DrawFrags ()
{
	int				i, k, l;
	int				top, bottom;
	int				x, y, f;
	int				xofs;
	char			num[12];
	scoreboard_t	*s;

	Sbar_SortFrags ();

// draw the text
	l = scoreboardlines <= 4 ? scoreboardlines : 4;

	x = 23;
	if (cl.gametype == GAME_DEATHMATCH)
		xofs = 0;
	else
		xofs = (vid.width - 320)>>1;
	y = vid.height - SBAR_HEIGHT - 23;

	for (i=0 ; i<l ; i++)
	{
		k = fragsort[i];
		s = &cl.scores[k];
		if (!s->name[0])
			continue;

	// draw background
		top = s->colors & 0xf0;
		bottom = (s->colors & 15)<<4;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);

#ifdef GLQUAKE
		g_GLRenderer->Draw_Fill (xofs + x*8 + 10, y, 28, 4, top);
		g_GLRenderer->Draw_Fill (xofs + x*8 + 10, y+4, 28, 3, bottom);
#else
		g_SoftwareRenderer->Draw_Fill(xofs + x * 8 + 10, y, 28, 4, top);
		g_SoftwareRenderer->Draw_Fill(xofs + x * 8 + 10, y + 4, 28, 3, bottom);
#endif
	// draw number
		f = s->frags;
		snprintf (num, sizeof(num), "%3i",f);

		Sbar_DrawCharacter ( (x+1)*8 , -24, num[0]);
		Sbar_DrawCharacter ( (x+2)*8 , -24, num[1]);
		Sbar_DrawCharacter ( (x+3)*8 , -24, num[2]);

		if (k == cl.viewentity - 1)
		{
			Sbar_DrawCharacter (x*8+2, -24, 16);
			Sbar_DrawCharacter ( (x+4)*8-4, -24, 17);
		}
		x+=4;
	}
}

//=============================================================================


/*
===============
Sbar_DrawFace
===============
*/
void Sbar_DrawFace ()
{
	int		f, anim;

// PGM 01/19/97 - team color drawing
// PGM 03/02/97 - fixed so color swatch only appears in CTF modes
	if (rogue &&
        (cl.maxclients != 1) &&
        (host->teamplay.value>3) &&
        (host->teamplay.value<7))
	{
		int				top, bottom;
		int				xofs;
		char			num[12];
		scoreboard_t	*s;
		
		s = &cl.scores[cl.viewentity - 1];
		// draw background
		top = s->colors & 0xf0;
		bottom = (s->colors & 15)<<4;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);

		if (cl.gametype == GAME_DEATHMATCH)
			xofs = 113;
		else
			xofs = ((vid.width - 320)>>1) + 113;

		Sbar_DrawPic (112, 0, rsb_teambord);

#ifdef GLQUAKE
		g_GLRenderer->Draw_Fill (xofs, vid.height-SBAR_HEIGHT+3, 22, 9, top);
		g_GLRenderer->Draw_Fill (xofs, vid.height-SBAR_HEIGHT+12, 22, 9, bottom);
#else
		g_SoftwareRenderer->Draw_Fill(xofs, vid.height - SBAR_HEIGHT + 3, 22, 9, top);
		g_SoftwareRenderer->Draw_Fill(xofs, vid.height - SBAR_HEIGHT + 12, 22, 9, bottom);
#endif

		// draw number
		f = s->frags;
		snprintf (num, sizeof(num), "%3i",f);

		if (top==8)
		{
			if (num[0] != ' ')
				Sbar_DrawCharacter(109, 3, 18 + num[0] - '0');
			if (num[1] != ' ')
				Sbar_DrawCharacter(116, 3, 18 + num[1] - '0');
			if (num[2] != ' ')
				Sbar_DrawCharacter(123, 3, 18 + num[2] - '0');
		}
		else
		{
			Sbar_DrawCharacter ( 109, 3, num[0]);
			Sbar_DrawCharacter ( 116, 3, num[1]);
			Sbar_DrawCharacter ( 123, 3, num[2]);
		}
		
		return;
	}
// PGM 01/19/97 - team color drawing

	if ( (cl.items & (IT_INVISIBILITY | IT_INVULNERABILITY) )
	== (IT_INVISIBILITY | IT_INVULNERABILITY) )
	{
		Sbar_DrawPic (112, 0, sb_face_invis_invuln);
		return;
	}
	if ((cl.items & (IT_INVISIBILITY | IT_QUAD))
		== (IT_INVISIBILITY | IT_QUAD))
	{
		Sbar_DrawPic(112, 0, sb_face_invis_quad);
		return;
	}
	if (cl.items & IT_QUAD)
	{
		Sbar_DrawPic (112, 0, sb_face_quad );
		return;
	}
	if (cl.items & IT_INVISIBILITY)
	{
		Sbar_DrawPic (112, 0, sb_face_invis );
		return;
	}
	if (cl.items & IT_INVULNERABILITY)
	{
		Sbar_DrawPic (112, 0, sb_face_invuln);
		return;
	}

	if (cl.stats[STAT_HEALTH] >= 100)
		f = 4;
	else
		f = cl.stats[STAT_HEALTH] / 20;

	if (cl.time <= cl.faceanimtime)
	{
		anim = 1;
		sb_updates = 0;		// make sure the anim gets drawn over
	}
	else
		anim = 0;
	Sbar_DrawPic (112, 0, sb_faces[f][anim]);
}

/*
===============
Sbar_Draw
===============
*/
void Sbar_Draw ()
{
	if (scr_con_current == vid.height)
		return;		// console is full screen

	if (sb_updates >= vid.numpages)
		return;

	scr_copyeverything = 1;

	sb_updates++;

	if (sb_lines && vid.width > 320) 
#ifdef GLQUAKE
		g_GLRenderer->Draw_TileClear (0, vid.height - sb_lines, vid.width, sb_lines);
#else
		g_SoftwareRenderer->Draw_TileClear (0, vid.height - sb_lines, vid.width, sb_lines);
#endif

	if (sb_lines > 24)
	{
		Sbar_DrawInventory ();
		if (cl.maxclients != 1)
			Sbar_DrawFrags ();
	}

	if (sb_showscores || cl.stats[STAT_HEALTH] <= 0)
	{
		Sbar_DrawPic (0, 0, sb_scorebar);
		Sbar_DrawScoreboard ();
		sb_updates = 0;
	}
	else if (sb_lines)
	{
		Sbar_DrawPic (0, 0, sb_sbar);

   // keys (hipnotic only)
      //MED 01/04/97 moved keys here so they would not be overwritten
      if (hipnotic)
      {
         if (cl.items & IT_KEY1)
            Sbar_DrawPic (209, 3, sb_items[0]);
         if (cl.items & IT_KEY2)
            Sbar_DrawPic (209, 12, sb_items[1]);
      }
   // armor
		if (cl.items & IT_INVULNERABILITY)
		{
			Sbar_DrawNum (24, 0, 666, 3, 1);
			Sbar_DrawPic (0, 0, ResolveRenderer()->GetLoadingDisc());
		}
		else
		{
			if (rogue)
			{
				Sbar_DrawNum (24, 0, cl.stats[STAT_ARMOR], 3,
								cl.stats[STAT_ARMOR] <= 25);
				if (cl.items & RIT_ARMOR3)
					Sbar_DrawPic (0, 0, sb_armor[2]);
				else if (cl.items & RIT_ARMOR2)
					Sbar_DrawPic (0, 0, sb_armor[1]);
				else if (cl.items & RIT_ARMOR1)
					Sbar_DrawPic (0, 0, sb_armor[0]);
			}
			else
			{
				Sbar_DrawNum (24, 0, cl.stats[STAT_ARMOR], 3
				, cl.stats[STAT_ARMOR] <= 25);
				if (cl.items & IT_ARMOR3)
					Sbar_DrawPic (0, 0, sb_armor[2]);
				else if (cl.items & IT_ARMOR2)
					Sbar_DrawPic (0, 0, sb_armor[1]);
				else if (cl.items & IT_ARMOR1)
					Sbar_DrawPic (0, 0, sb_armor[0]);
			}
		}

	// face
		Sbar_DrawFace ();

	// health
		Sbar_DrawNum (136, 0, cl.stats[STAT_HEALTH], 3
		, cl.stats[STAT_HEALTH] <= 25);

	// ammo icon
		if (rogue)
		{
			if (cl.items & RIT_SHELLS)
				Sbar_DrawPic (224, 0, sb_ammo[0]);
			else if (cl.items & RIT_NAILS)
				Sbar_DrawPic (224, 0, sb_ammo[1]);
			else if (cl.items & RIT_ROCKETS)
				Sbar_DrawPic (224, 0, sb_ammo[2]);
			else if (cl.items & RIT_CELLS)
				Sbar_DrawPic (224, 0, sb_ammo[3]);
			else if (cl.items & RIT_LAVA_NAILS)
				Sbar_DrawPic (224, 0, rsb_ammo[0]);
			else if (cl.items & RIT_PLASMA_AMMO)
				Sbar_DrawPic (224, 0, rsb_ammo[1]);
			else if (cl.items & RIT_MULTI_ROCKETS)
				Sbar_DrawPic (224, 0, rsb_ammo[2]);
		}
		else
		{
			if (cl.items & IT_SHELLS)
				Sbar_DrawPic (224, 0, sb_ammo[0]);
			else if (cl.items & IT_NAILS)
				Sbar_DrawPic (224, 0, sb_ammo[1]);
			else if (cl.items & IT_ROCKETS)
				Sbar_DrawPic (224, 0, sb_ammo[2]);
			else if (cl.items & IT_CELLS)
				Sbar_DrawPic (224, 0, sb_ammo[3]);
		}

		Sbar_DrawNum (248, 0, cl.stats[STAT_AMMO], 3,
					  cl.stats[STAT_AMMO] <= 10);
	}

	if (vid.width > 320) {
		if (cl.gametype == GAME_DEATHMATCH)
			Sbar_MiniDeathmatchOverlay ();
	}
}

//=============================================================================

/*
==================
Sbar_IntermissionNumber

==================
*/
void Sbar_IntermissionNumber (int x, int y, int num, int digits, int color)
{
	char			str[12];
	char			*ptr;
	int				l, frame;

	l = Sbar_itoa (num, str);
	ptr = str;
	if (l > digits)
		ptr += (l-digits);
	if (l < digits)
		x += (digits-l)*24;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

#ifdef GLQUAKE
		g_GLRenderer->Draw_TransPic (x,y,sb_nums[color][frame]);
#else
		g_SoftwareRenderer->Draw_TransPic (x,y,sb_nums[color][frame]);
#endif

		x += 24;
		ptr++;
	}
}

/*
==================
Sbar_DeathmatchOverlay

==================
*/
void Sbar_DeathmatchOverlay ()
{
	CQuakePic			*pic;
	int				i, k, l;
	int				top, bottom;
	int				x, y, f;
	char			num[12];
	scoreboard_t	*s;

	scr_copyeverything = 1;
	scr_fullupdate = 0;
#ifdef GLQUAKE
	pic = g_GLRenderer->Draw_CachePic ("gfx/ranking.lmp");
#else
	pic = g_SoftwareRenderer->Draw_CachePic("gfx/ranking.lmp");
#endif
	M_DrawPic ((320-pic->width)/2, 8, pic);

// scores
	Sbar_SortFrags ();

// draw the text
	l = scoreboardlines;

	x = 80 + ((vid.width - 320)>>1);
	y = 40;
	for (i=0 ; i<l ; i++)
	{
		k = fragsort[i];
		s = &cl.scores[k];
		if (!s->name[0])
			continue;

	// draw background
		top = s->colors & 0xf0;
		bottom = (s->colors & 15)<<4;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);
#ifdef GLQUAKE
		g_GLRenderer->Draw_Fill ( x, y, 40, 4, top);
		g_GLRenderer->Draw_Fill ( x, y+4, 40, 4, bottom);
#else
		g_SoftwareRenderer->Draw_Fill(x, y, 40, 4, top);
		g_SoftwareRenderer->Draw_Fill(x, y + 4, 40, 4, bottom);
#endif
	// draw number
		f = s->frags;
		snprintf (num, sizeof(num), "%3i",f);

#ifdef GLQUAKE
		g_GLRenderer->Draw_Character ( x+8 , y, num[0]);
		g_GLRenderer->Draw_Character ( x+16 , y, num[1]);
		g_GLRenderer->Draw_Character ( x+24 , y, num[2]);
#else
		g_SoftwareRenderer->Draw_Character(x + 8, y, num[0]);
		g_SoftwareRenderer->Draw_Character(x + 16, y, num[1]);
		g_SoftwareRenderer->Draw_Character(x + 24, y, num[2]);
#endif

		if (k == cl.viewentity - 1)
#ifdef GLQUAKE
			g_GLRenderer->Draw_Character ( x - 8, y, 12);
#else
			g_SoftwareRenderer->Draw_Character ( x - 8, y, 12);
#endif


#if 0
{
	int				total;
	int				n, minutes, tens, units;

	// draw time
		total = cl.completed_time - s->entertime;
		minutes = (int)total/60;
		n = total - minutes*60;
		tens = n/10;
		units = n%10;

		sprintf (num, "%3i:%i%i", minutes, tens, units);

		Draw_String ( x+48 , y, num);
}
#endif

	// draw name
#ifdef GLQUAKE
		g_GLRenderer->Draw_String (x+64, y, s->name);
#else
		g_SoftwareRenderer->Draw_String (x+64, y, s->name);
#endif
		y += 10;
	}
}

/*
==================
Sbar_DeathmatchOverlay

==================
*/
void Sbar_MiniDeathmatchOverlay ()
{
	int				i, k, l;
	int				top, bottom;
	int				x, y, f;
	char			num[12];
	scoreboard_t	*s;
	int				numlines;

	if (vid.width < 512 || !sb_lines)
		return;

	scr_copyeverything = 1;
	scr_fullupdate = 0;

// scores
	Sbar_SortFrags ();

// draw the text
	l = scoreboardlines;
	y = vid.height - sb_lines;
	numlines = sb_lines/8;
	if (numlines < 3)
		return;

	//find us
	for (i = 0; i < scoreboardlines; i++)
		if (fragsort[i] == cl.viewentity - 1)
			break;

    if (i == scoreboardlines) // we're not there
            i = 0;
    else // figure out start
            i = i - numlines/2;

    if (i > scoreboardlines - numlines)
            i = scoreboardlines - numlines;
    if (i < 0)
            i = 0;

	x = 324;
	for (; i < scoreboardlines && (unsigned int)y < vid.height - 8 ; i++)
	{
		k = fragsort[i];
		s = &cl.scores[k];
		if (!s->name[0])
			continue;

	// draw background
		top = s->colors & 0xf0;
		bottom = (s->colors & 15)<<4;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);

#ifdef GLQUAKE
		g_GLRenderer->Draw_Fill ( x, y+1, 40, 3, top);
		g_GLRenderer->Draw_Fill ( x, y+4, 40, 4, bottom);
#else
		g_SoftwareRenderer->Draw_Fill(x, y + 1, 40, 3, top);
		g_SoftwareRenderer->Draw_Fill(x, y + 4, 40, 4, bottom);
#endif
	// draw number
		f = s->frags;
		snprintf (num, sizeof(num), "%3i",f);

#ifdef GLQUAKE
		g_GLRenderer->Draw_Character ( x+8 , y, num[0]);
		g_GLRenderer->Draw_Character ( x+16 , y, num[1]);
		g_GLRenderer->Draw_Character ( x+24 , y, num[2]);
#else
		g_SoftwareRenderer->Draw_Character(x + 8, y, num[0]);
		g_SoftwareRenderer->Draw_Character(x + 16, y, num[1]);
		g_SoftwareRenderer->Draw_Character(x + 24, y, num[2]);
#endif

#ifdef GLQUAKE
		if (k == cl.viewentity - 1) {
			g_GLRenderer->Draw_Character ( x, y, 16);
			g_GLRenderer->Draw_Character ( x + 32, y, 17);
		}
#else
		if (k == cl.viewentity - 1) {
			g_SoftwareRenderer->Draw_Character(x, y, 16);
			g_SoftwareRenderer->Draw_Character(x + 32, y, 17);
		}
#endif

#if 0
{
	int				total;
	int				n, minutes, tens, units;

	// draw time
		total = cl.completed_time - s->entertime;
		minutes = (int)total/60;
		n = total - minutes*60;
		tens = n/10;
		units = n%10;

		sprintf (num, "%3i:%i%i", minutes, tens, units);

		Draw_String ( x+48 , y, num);
}
#endif

	// draw name
#ifdef GLQUAKE
		g_GLRenderer->Draw_String (x+48, y, s->name);
#else
		g_SoftwareRenderer->Draw_String (x+48, y, s->name);
#endif
		y += 8;
	}
}

/*
==================
Sbar_IntermissionOverlay

==================
*/
void Sbar_IntermissionOverlay ()
{
	CQuakePic	*pic;
	int		dig;
	int		num;

	scr_copyeverything = 1;
	scr_fullupdate = 0;

	if (cl.gametype == GAME_DEATHMATCH)
	{
		Sbar_DeathmatchOverlay ();
		return;
	}
#ifdef GLQUAKE
	pic = g_GLRenderer->Draw_CachePic ("gfx/complete.lmp");
	g_GLRenderer->Draw_Pic (64, 24, pic);

	pic = g_GLRenderer->Draw_CachePic ("gfx/inter.lmp");
	g_GLRenderer->Draw_TransPic (0, 56, pic);
#else
	pic = g_SoftwareRenderer->Draw_CachePic("gfx/complete.lmp");
	g_SoftwareRenderer->Draw_Pic(64, 24, pic);

	pic = g_SoftwareRenderer->Draw_CachePic("gfx/inter.lmp");
	g_SoftwareRenderer->Draw_TransPic(0, 56, pic);
#endif

// time
	dig = cl.completed_time/60;
	Sbar_IntermissionNumber (160, 64, dig, 3, 0);
	num = cl.completed_time - dig*60;

#ifdef GLQUAKE
	g_GLRenderer->Draw_TransPic (234,64,sb_colon);
	g_GLRenderer->Draw_TransPic (246,64,sb_nums[0][num/10]);
	g_GLRenderer->Draw_TransPic (266,64,sb_nums[0][num%10]);
#else
	g_SoftwareRenderer->Draw_TransPic(234, 64, sb_colon);
	g_SoftwareRenderer->Draw_TransPic(246, 64, sb_nums[0][num / 10]);
	g_SoftwareRenderer->Draw_TransPic(266, 64, sb_nums[0][num % 10]);
#endif

	Sbar_IntermissionNumber (160, 104, cl.stats[STAT_SECRETS], 3, 0);
#ifdef GLQUAKE
	g_GLRenderer->Draw_TransPic (232,104,sb_slash);
#else
	g_SoftwareRenderer->Draw_TransPic (232,104,sb_slash);
#endif
	Sbar_IntermissionNumber (240, 104, cl.stats[STAT_TOTALSECRETS], 3, 0);

	Sbar_IntermissionNumber (160, 144, cl.stats[STAT_MONSTERS], 3, 0);
#ifdef GLQUAKE
	g_GLRenderer->Draw_TransPic (232,144,sb_slash);
#else
	g_SoftwareRenderer->Draw_TransPic (232,144,sb_slash);
#endif
	Sbar_IntermissionNumber (240, 144, cl.stats[STAT_TOTALMONSTERS], 3, 0);

}


/*
==================
Sbar_FinaleOverlay

==================
*/
void Sbar_FinaleOverlay ()
{
	CQuakePic	*pic;

	scr_copyeverything = 1;
#ifdef GLQUAKE
	pic = g_GLRenderer->Draw_CachePic ("gfx/finale.lmp");
	g_GLRenderer->Draw_TransPic ( (vid.width-pic->width)/2, 16, pic);
#else
	pic = g_SoftwareRenderer->Draw_CachePic("gfx/finale.lmp");
	g_SoftwareRenderer->Draw_TransPic((vid.width - pic->width) / 2, 16, pic);
#endif
}
