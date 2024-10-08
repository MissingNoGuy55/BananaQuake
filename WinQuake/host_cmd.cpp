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

#include "quakedef.h"

extern cvar_t	pausable;

int	CQuakeHost::current_skill;

void Mod_Print ();

/*
==================
Host_Quit_f
==================
*/

extern void M_Menu_Quit_f ();

void CQuakeHost::Host_Quit_f ()
{
	if (key_dest != key_console && cls.state != ca_dedicated)
	{
		M_Menu_Quit_f ();
		return;
	}
	CL_Disconnect ();
	host->Host_ShutdownServer(false);		

	CloseAllVPKs();
	unzUnloadAllZips();

	Sys_Quit ();
}


/*
==================
Host_Status_f
==================
*/
void CQuakeHost::Host_Status_f ()
{
	client_t	*client;
	int			seconds;
	int			minutes;
	int			hours = 0;
	int			j;
	void		(*print) (const char *fmt, ...);
	
	if (cmd_source == src_command)
	{
		if (!sv->IsServerActive())
		{
			g_pCmds->Cmd_ForwardToServer();
			return;
		}
		print = Con_Printf;
	}
	else
		print = sv->SV_ClientPrintf;

	print ("host:    %s\n", Cvar_VariableString ("hostname"));
	print ("version: %4.2f\n", VERSION);
	if (tcpipAvailable)
		print ("tcp/ip:  %s\n", my_tcpip_address);
	if (ipxAvailable)
		print ("ipx:     %s\n", my_ipx_address);
	print ("map:     %s\n", sv->GetMapName());
	print ("players: %i active (%i max)\n\n", net_activeconnections, svs.maxclients);
	for (j=0, client = svs.clients ; j<svs.maxclients ; j++, client++)
	{
		if (!client->active)
			continue;
		seconds = (int)(net_time - client->netconnection->connecttime);
		minutes = seconds / 60;
		if (minutes)
		{
			seconds -= (minutes * 60);
			hours = minutes / 60;
			if (hours)
				minutes -= (hours * 60);
		}
		else
			hours = 0;
		print ("#%-2u %-16.16s  %3i  %2i:%02i:%02i\n", j+1, client->name, (int)client->edict->v.frags, hours, minutes, seconds);
		print ("   %s\n", client->netconnection->address);
	}
}


/*
==================
Host_God_f

Sets client to godmode
==================
*/
void CQuakeHost::Host_God_f ()
{
	if (cmd_source == src_command)
	{
		g_pCmds->Cmd_ForwardToServer ();
		return;
	}

	if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	sv_player->v.flags = (int)sv_player->v.flags ^ FL_GODMODE;
	if (!((int)sv_player->v.flags & FL_GODMODE) )
		sv->SV_ClientPrintf ("godmode OFF\n");
	else
		sv->SV_ClientPrintf ("godmode ON\n");
}

void CQuakeHost::Host_Notarget_f ()
{
	if (cmd_source == src_command)
	{
		g_pCmds->Cmd_ForwardToServer ();
		return;
	}

	if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	sv_player->v.flags = (int)sv_player->v.flags ^ FL_NOTARGET;
	if (!((int)sv_player->v.flags & FL_NOTARGET) )
		sv->SV_ClientPrintf ("notarget OFF\n");
	else
		sv->SV_ClientPrintf ("notarget ON\n");
}

bool noclip_anglehack;

void CQuakeHost::Host_Noclip_f ()
{
	if (cmd_source == src_command)
	{
		g_pCmds->Cmd_ForwardToServer ();
		return;
	}

	if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	if (sv_player->v.movetype != MOVETYPE_NOCLIP)
	{
		noclip_anglehack = true;
		sv_player->v.movetype = MOVETYPE_NOCLIP;
		sv->SV_ClientPrintf ("noclip ON\n");
	}
	else
	{
		noclip_anglehack = false;
		sv_player->v.movetype = MOVETYPE_WALK;
		sv->SV_ClientPrintf ("noclip OFF\n");
	}
}

/*
==================
Host_Fly_f

Sets client to flymode
==================
*/
void CQuakeHost::Host_Fly_f ()
{
	if (cmd_source == src_command)
	{
		g_pCmds->Cmd_ForwardToServer ();
		return;
	}

	if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	if (sv_player->v.movetype != MOVETYPE_FLY)
	{
		sv_player->v.movetype = MOVETYPE_FLY;
		sv->SV_ClientPrintf ("flymode ON\n");
	}
	else
	{
		sv_player->v.movetype = MOVETYPE_WALK;
		sv->SV_ClientPrintf ("flymode OFF\n");
	}
}


/*
==================
Host_Ping_f

==================
*/
void CQuakeHost::Host_Ping_f ()
{
	int		i, j;
	float	total;
	client_t	*client;
	
	if (cmd_source == src_command)
	{
		g_pCmds->Cmd_ForwardToServer ();
		return;
	}

	sv->SV_ClientPrintf ("Client ping times:\n");
	for (i=0, client = svs.clients ; i<svs.maxclients ; i++, client++)
	{
		if (!client->active)
			continue;
		total = 0;
		for (j=0 ; j<NUM_PING_TIMES ; j++)
			total+=client->ping_times[j];
		total /= NUM_PING_TIMES;
		sv->SV_ClientPrintf ("%4i %s\n", (int)(total*1000), client->name);
	}
}

/*
===============================================================================

SERVER TRANSITIONS

===============================================================================
*/


/*
======================
Host_Map_f

handle a 
map <servername>
command from the console.  Active clients are kicked off.
======================
*/
void CQuakeHost::Host_Map_f ()
{
	int		i;
	char	name[MAX_QPATH] = {};

	if (cmd_source != src_command)
		return;

	cls.demonum = -1;		// stop demo loop in case this fails

	CL_Disconnect ();
	host->Host_ShutdownServer(false);

	key_dest = key_game;			// remove console or menu
	SCR_BeginLoadingPlaque ();

	cls.mapstring[0] = 0;
	for (i=0 ; i< g_pCmds->Cmd_Argc() ; i++)
	{
		Q_strcat (cls.mapstring, g_pCmds->Cmd_Argv(i));
		Q_strcat (cls.mapstring, " ");
	}
	Q_strcat (cls.mapstring, "\n");

	svs.serverflags = 0;			// haven't completed an episode yet
	Q_strncpy (name, g_pCmds->Cmd_Argv(1), sizeof(name));
#ifdef QUAKE2
	SV_SpawnServer (name, NULL);
#else
	sv->SV_SpawnServer (name);
#endif
	if (!sv->IsServerActive())
		return;
	
	if (cls.state != ca_dedicated)
	{
		Q_strcpy (cls.spawnparms, "");

		for (i=2 ; i< g_pCmds->Cmd_Argc() ; i++)
		{
			Q_strcat (cls.spawnparms, g_pCmds->Cmd_Argv(i));
			Q_strcat (cls.spawnparms, " ");
		}
		
		g_pCmds->Cmd_ExecuteString("connect local", src_command);
	}	
}

/*
==================
Host_Changelevel_f

Goes to a new map, taking all clients along
==================
*/
void CQuakeHost::Host_Changelevel_f ()
{
#ifdef QUAKE2
	char	level[MAX_QPATH];
	char	_startspot[MAX_QPATH];
	char	*startspot;

	if (Cmd_Argc() < 2)
	{
		Con_Printf ("changelevel <levelname> : continue game on a new level\n");
		return;
	}
	if (!sv->IsServerActive() || cls.demoplayback)
	{
		Con_Printf ("Only the server may changelevel\n");
		return;
	}

	Q_strcpy (level, Cmd_Argv(1));
	if (Cmd_Argc() == 2)
		startspot = NULL;
	else
	{
		Q_strcpy (_startspot, Cmd_Argv(2));
		startspot = _startspot;
	}

	SV_SaveSpawnparms ();
	SV_SpawnServer (level, startspot);
#else
	char	level[MAX_QPATH] = {};

	if (g_pCmds->Cmd_Argc() != 2)
	{
		Con_Printf ("changelevel <levelname> : continue game on a new level\n");
		return;
	}
	if (!sv->IsServerActive() || cls.demoplayback)
	{
		Con_Printf ("Only the server may changelevel\n");
		return;
	}
    key_dest = key_game;
	sv->SV_SaveSpawnparms ();
    Q_strlcpy (level, g_pCmds->Cmd_Argv(1), sizeof(level));
	sv->SV_SpawnServer (level);
#endif
}

/*
==================
Host_Restart_f

Restarts the current server for a dead player
==================
*/
void CQuakeHost::Host_Restart_f ()
{
	char	mapname[MAX_QPATH] = {};
#ifdef QUAKE2
	char	startspot[MAX_QPATH] = {};
#endif

	if (cls.demoplayback || !sv->IsServerActive())
		return;

	if (cmd_source != src_command)
		return;
    Q_strlcpy (mapname, sv->GetMapName(), sizeof(mapname));	// must copy out, because it gets cleared
								// in sv_spawnserver
#ifdef QUAKE2
	Q_strcpy(startspot, sv->startspot);
	SV_SpawnServer (mapname, startspot);
#else
	sv->SV_SpawnServer (mapname);
#endif
}

/*
==================
Host_Reconnect_f

This command causes the client to wait for the signon messages again.
This is sent just before a server changes levels
==================
*/
void CQuakeHost::Host_Reconnect_f ()
{
	SCR_BeginLoadingPlaque ();
	cls.signon = 0;		// need new connection messages
}

/*
=====================
Host_Connect_f

User command to connect to server
=====================
*/
void CQuakeHost::Host_Connect_f ()
{
	char	name[MAX_QPATH] = {};
	
	cls.demonum = -1;		// stop demo loop in case this fails
	if (cls.demoplayback)
	{
		CL_StopPlayback ();
		CL_Disconnect ();
	}
	Q_strncpy (name, g_pCmds->Cmd_Argv(1), sizeof(name));
	CL_EstablishConnection (name);
	Host_Reconnect_f ();
}


/*
===============================================================================

LOAD / SAVE GAME

===============================================================================
*/

#define	SAVEGAME_VERSION	5

/*
===============
Host_SavegameComment

Writes a SAVEGAME_COMMENT_LENGTH character comment describing the current 
===============
*/
void CQuakeHost::Host_SavegameComment (char *text)
{
	int		i;
	char	kills[20];

	for (i=0 ; i<SAVEGAME_COMMENT_LENGTH ; i++)
		text[i] = ' ';
	memcpy (text, cl.levelname, strlen(cl.levelname));
	snprintf (kills,sizeof(kills), "kills:%3i/%3i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
	memcpy (text+22, kills, strlen(kills));
// convert space to _ to make stdio happy
	for (i=0 ; i<SAVEGAME_COMMENT_LENGTH ; i++)
		if (text[i] == ' ')
			text[i] = '_';
	text[SAVEGAME_COMMENT_LENGTH] = '\0';
}


/*
===============
Host_Savegame_f
===============
*/
void CQuakeHost::Host_Savegame_f ()
{
	char	name[256] = {};
	cxxofstream	f;
	int		i = 0;
	char	comment[SAVEGAME_COMMENT_LENGTH + 1] = {};

	if (cmd_source != src_command)
		return;

	if (!sv->IsServerActive())
	{
		Con_Printf ("Not playing a local game.\n");
		return;
	}

	if (cl.intermission)
	{
		Con_Printf ("Can't save in intermission.\n");
		return;
	}

	if (svs.maxclients != 1)
	{
		Con_Printf ("Can't save multiplayer games.\n");
		return;
	}

	if (g_pCmds->Cmd_Argc() != 2)
	{
		Con_Printf ("save <savename> : save a game\n");
		return;
	}

	if (strstr(g_pCmds->Cmd_Argv(1), ".."))
	{
		Con_Printf ("Relative pathnames are not allowed.\n");
		return;
	}
		
	for (i=0 ; i<svs.maxclients ; i++)
	{
		if (svs.clients[i].active && (svs.clients[i].edict->v.health <= 0) )
		{
			Con_Printf ("Can't savegame with a dead player\n");
			return;
		}
	}

    snprintf (name, sizeof(name), "%s/%s", g_Common->com_gamedir, g_pCmds->Cmd_Argv(1));
	g_Common->COM_DefaultExtension (name, ".sav");
	
	Con_Printf ("Saving game to %s...\n", name);
	f.open(name);
	if (!f.is_open())
	{
		Con_Printf ("ERROR: couldn't open.\n");
		return;
	}
	
	char write[512];
	snprintf(write, sizeof(write), "%i\n", SAVEGAME_VERSION);
	f.write(write, Q_strlen(write));

	Host_SavegameComment (comment);

	snprintf(write, sizeof(write), "%s\n", comment);
	f.write(write, Q_strlen(write));

	for (i = 0; i < NUM_SPAWN_PARMS; i++)
	{
		snprintf(write, sizeof(write), "%f\n", svs.clients->spawn_parms[i]);
		f.write(write, Q_strlen(write));
	}
	snprintf(write, sizeof(write), "%d\n", current_skill);
	f.write(write, Q_strlen(write));
	snprintf(write, sizeof(write), "%s\n", sv->GetMapName());
	f.write(write, Q_strlen(write));
	snprintf(write, sizeof(write), "%f\n", sv->GetServerTime());
	f.write(write, Q_strlen(write));

// write the light styles

	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		if (sv->GetLightStyles()[i])
		{
			snprintf(write, sizeof(write), "%s\n", sv->GetLightStyles()[i]);
			f.write(write, Q_strlen(write));
		}
		else
		{
			f.write("m\n", 2);
		}
	}


	ED_WriteGlobals (&f);
	for (i=0 ; i<sv->GetNumEdicts() ; i++)
	{
		ED_Write (&f, EDICT_NUM(i));
		f.flush();
	}
	f.close();
	Con_Printf ("done.\n");
}

static char	savegame_string[32768] = {};

/*
===============
Host_Loadgame_f
===============
*/
void CQuakeHost::Host_Loadgame_f ()
{
    static char	*start;

    char	name[MAX_OSPATH];
    char	mapname[MAX_QPATH];
    float	time, tfloat;
    const char	*data;
    int	i;
    edict_t	*ent;
    int	entnum;
    int	version;
    float	spawn_parms[NUM_SPAWN_PARMS];

    if (cmd_source != src_command)
        return;

    if (g_pCmds->Cmd_Argc() != 2)
    {
        Con_Printf ("load <savename> : load a game\n");
        return;
    }

    if (strstr(g_pCmds->Cmd_Argv(1), ".."))
    {
        Con_Printf ("Relative pathnames are not allowed.\n");
        return;
    }

    cls.demonum = -1;		// stop demo loop in case this fails

    snprintf (name, sizeof(name), "%s/%s", g_Common->com_gamedir, g_pCmds->Cmd_Argv(1));
    g_Common->COM_AddExtension (name, ".sav", sizeof(name));

// we can't call SCR_BeginLoadingPlaque, because too much stack space has
// been used.  The menu calls it before stuffing loadgame command
//	SCR_BeginLoadingPlaque ();

    Con_Printf ("Loading game from %s...\n", name);

// avoid leaking if the previous Host_Loadgame_f failed with a Host_Error
    if (start != NULL)
        free (start);

    start = (char *) g_Common->COM_LoadMallocFile_TextMode_OSPath(name, NULL);
    if (start == NULL)
    {
        Con_Printf ("ERROR: couldn't open.\n");
        return;
    }

    data = start;
    data = g_Common->COM_ParseIntNewline (data, &version);
    if (version != SAVEGAME_VERSION)
    {
        free (start);
        start = NULL;
        host->Host_Error ("Savegame is version %i, not %i", version, SAVEGAME_VERSION);
        return;
    }
    data = g_Common->COM_ParseStringNewline (data);
    for (i = 0; i < NUM_SPAWN_PARMS; i++)
        data = g_Common->COM_ParseFloatNewline (data, &spawn_parms[i]);
// this silliness is so we can load 1.06 save files, which have float skill values
    data = g_Common->COM_ParseFloatNewline(data, &tfloat);
    current_skill = (int)(tfloat + 0.1);
    Cvar_SetValue ("skill", (float)current_skill);

    data = g_Common->COM_ParseStringNewline (data);
    Q_strlcpy (mapname, g_Common->com_token, sizeof(mapname));
    data = g_Common->COM_ParseFloatNewline (data, &time);

    CL_Disconnect_f ();

    sv->SV_SpawnServer (mapname);

    if (!sv->IsServerActive())
    {
        free (start);
        start = NULL;
        SCR_EndLoadingPlaque ();
        Con_Printf ("Couldn't load map\n");
        return;
    }
    sv->SetServerPaused(true);		// pause until all clients connect
    sv->SetLoadGame(true);

// load the light styles
    for (i = 0; i < MAX_LIGHTSTYLES; i++)
    {
        data = g_Common->COM_ParseStringNewline (data);
        sv->SetLightStyle(i, (const char *)g_MemCache->Hunk_Strdup (g_Common->com_token, "lightstyles"));
    }

// load the edicts out of the savegame file
    entnum = -1;		// -1 is the globals
    while (*data)
    {
        data = g_Common->COM_Parse (data);
        if (!g_Common->com_token[0])
            break;		// end of file
        if (strcmp(g_Common->com_token,"{"))
        {
            host->Host_Error ("First token isn't a brace");
        }

        if (entnum == -1)
        {	// parse the global vars
            data = ED_ParseGlobals (data);
        }
        else
        {	// parse an edict
            ent = EDICT_NUM(entnum);
            if (entnum < sv->GetNumEdicts()) {
                ent->free = false;
                memset (&ent->v, 0, progs->entityfields * 4);
            }
            data = ED_ParseEdict (data, ent);

        // link it into the bsp tree
            if (!ent->free)
                sv->SV_LinkEdict (ent, false);
        }

        entnum++;
    }

    // Free edicts allocated during map loading but no longer used after restoring saved game state
    for (i = entnum; i < sv->GetNumEdicts(); i++)
        ED_Free(EDICT_NUM(i));

    sv->SetNumEdicts(entnum);
    sv->SetServerTime(time);

    free (start);
    start = NULL;

    for (i = 0; i < NUM_SPAWN_PARMS; i++)
        svs.clients->spawn_parms[i] = spawn_parms[i];

    if (cls.state != ca_dedicated)
    {
        CL_EstablishConnection ("local");
        Host_Reconnect_f ();
    }
}

#ifdef QUAKE2
void CQuakeHost::SaveGamestate()
{
	char	name[256];
	FILE	*f;
	int		i;
	char	comment[SAVEGAME_COMMENT_LENGTH+1];
	edict_t	*ent;

	sprintf (name, "%s/%s.gip", com_gamedir, sv->name);
	
	Con_Printf ("Saving game to %s...\n", name);
	f = fopen (name, "w");
	if (!f)
	{
		Con_Printf ("ERROR: couldn't open.\n");
		return;
	}
	
	fprintf (f, "%i\n", SAVEGAME_VERSION);
	Host_SavegameComment (comment);
	fprintf (f, "%s\n", comment);
//	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
//		fprintf (f, "%f\n", svs.clients->spawn_parms[i]);
	fprintf (f, "%f\n", host->skill.value);
	fprintf (f, "%s\n", sv->name);
	fprintf (f, "%f\n", sv->time);

// write the light styles

	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		if (sv->lightstyles[i])
			fprintf (f, "%s\n", sv->lightstyles[i]);
		else
			fprintf (f,"m\n");
	}


	for (i=svs.maxclients+1 ; i<sv->num_edicts ; i++)
	{
		ent = EDICT_NUM(i);
		if ((int)ent->v.flags & FL_ARCHIVE_OVERRIDE)
			continue;
		fprintf (f, "%i\n",i);
		ED_Write (f, ent);
		fflush (f);
	}
	fclose (f);
	Con_Printf ("done.\n");
}

int CQuakeHost::LoadGamestate(char *level, char *startspot)
{
	char	name[MAX_OSPATH];
	FILE	*f;
	char	mapname[MAX_QPATH];
	float	time, sk;
	char	str[32768], *start;
	int		i, r;
	edict_t	*ent;
	int		entnum;
	int		version;
//	float	spawn_parms[NUM_SPAWN_PARMS];

	sprintf (name, "%s/%s.gip", com_gamedir, level);
	
	Con_Printf ("Loading game from %s...\n", name);
	f = fopen (name, "r");
	if (!f)
	{
		Con_Printf ("ERROR: couldn't open.\n");
		return -1;
	}

	fscanf (f, "%i\n", &version);
	if (version != SAVEGAME_VERSION)
	{
		fclose (f);
		Con_Printf ("Savegame is version %i, not %i\n", version, SAVEGAME_VERSION);
		return -1;
	}
	fscanf (f, "%s\n", str);
//	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
//		fscanf (f, "%f\n", &spawn_parms[i]);
	fscanf (f, "%f\n", &sk);
	Cvar_SetValue ("skill", sk);

	fscanf (f, "%s\n",mapname);
	fscanf (f, "%f\n",&time);

	SV_SpawnServer (mapname, startspot);

	if (!sv->IsServerActive())
	{
		Con_Printf ("Couldn't load map\n");
		return -1;
	}

// load the light styles
	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		fscanf (f, "%s\n", str);
		sv->lightstyles[i] = Hunk_Alloc (strlen(str)+1);
		Q_strcpy (sv->lightstyles[i], str);
	}

// load the edicts out of the savegame file
	while (!feof(f))
	{
		fscanf (f, "%i\n",&entnum);
		for (i=0 ; i<sizeof(str)-1 ; i++)
		{
			r = fgetc (f);
			if (r == EOF || !r)
				break;
			str[i] = r;
			if (r == '}')
			{
				i++;
				break;
			}
		}
		if (i == sizeof(str)-1)
			Sys_Error ("Loadgame buffer overflow");
		str[i] = 0;
		start = str;
		start = COM_Parse(str);
		if (!com_token[0])
			break;		// end of file
		if (strcmp(com_token,"{"))
			Sys_Error ("First token isn't a brace");
			
		// parse an edict

		ent = EDICT_NUM(entnum);
		memset (&ent->v, 0, progs->entityfields * 4);
		ent->free = false;
		ED_ParseEdict (start, ent);
	
		// link it into the bsp tree
		if (!ent->free)
			SV_LinkEdict (ent, false);
	}
	
//	sv->num_edicts = entnum;
	sv->time = time;
	fclose (f);

//	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
//		svs.clients->spawn_parms[i] = spawn_parms[i];

	return 0;
}

// changing levels within a unit
void CQuakeHost::Host_Changelevel2_f ()
{
	char	level[MAX_QPATH];
	char	_startspot[MAX_QPATH];
	char	*startspot;

	if (Cmd_Argc() < 2)
	{
		Con_Printf ("changelevel2 <levelname> : continue game on a new level in the unit\n");
		return;
	}
	if (!sv->IsServerActive() || cls.demoplayback)
	{
		Con_Printf ("Only the server may changelevel\n");
		return;
	}

	Q_strcpy (level, Cmd_Argv(1));
	if (Cmd_Argc() == 2)
		startspot = NULL;
	else
	{
		Q_strcpy (_startspot, Cmd_Argv(2));
		startspot = _startspot;
	}

	SV_SaveSpawnparms ();

	// save the current level's state
	SaveGamestate ();

	// try to restore the new level
	if (LoadGamestate (level, startspot))
		SV_SpawnServer (level, startspot);
}
#endif


//============================================================================

/*
======================
Host_Name_f
======================
*/
void CQuakeHost::Host_Name_f ()
{
	char	newName[16];
	const char* pszNewName = NULL;
	memset(newName, 0, sizeof(newName));

	if (g_pCmds->Cmd_Argc () == 1)
	{
		Con_Printf ("\"name\" is \"%s\"\n", cl_name.string);
		return;
	}

	if (g_pCmds->Cmd_Argc() == 2)
	{
		snprintf(newName, sizeof(newName), "%s", g_pCmds->Cmd_Argv(1));
		pszNewName = newName;
	}
	else
	{
		Q_strcat(newName, g_pCmds->Cmd_Args());
	}
	newName[15] = 0;

	if (cmd_source == src_command)
	{
		if (Q_strcmp(cl_name.string, newName) == 0)
			return;
		Cvar_Set ("_cl_name", newName);
		if (cls.state == ca_connected)
			g_pCmds->Cmd_ForwardToServer ();
		return;
	}

	if (host_client->name[0] && strcmp(host_client->name, "unconnected") )
		if (Q_strcmp(host_client->name, newName) != 0)
			Con_Printf ("%s renamed to %s\n", host_client->name, newName);
	Q_strcpy (host_client->name, newName);
	host_client->edict->v.netname = PR_SetEngineString(host_client->name);
	
// send notification to all clients
	
	MSG_WriteByte (&sv->GetReliableDatagramBuffer(), svc_updatename);
	MSG_WriteByte (&sv->GetReliableDatagramBuffer(), host_client - svs.clients);
	MSG_WriteString (&sv->GetReliableDatagramBuffer(), host_client->name);
}

	
void CQuakeHost::Host_Version_f ()
{
	Con_Printf ("Version %4.2f\n", BANANAQUAKE_VERSION);
	Con_Printf ("Exe: %s %s\n", __TIME__, __DATE__);
}

#ifdef IDGODS
void CQuakeHost::Host_Please_f ()
{
	client_t *cl;
	int			j;
	
	if (cmd_source != src_command)
		return;

	if ((Cmd_Argc () == 3) && Q_strcmp(Cmd_Argv(1), "#") == 0)
	{
		j = Q_atof(Cmd_Argv(2)) - 1;
		if (j < 0 || j >= svs.maxclients)
			return;
		if (!svs.clients[j].active)
			return;
		cl = &svs.clients[j];
		if (cl.privileged)
		{
			cl.privileged = false;
			cl.edict->v.flags = (int)cl.edict->v.flags & ~(FL_GODMODE|FL_NOTARGET);
			cl.edict->v.movetype = MOVETYPE_WALK;
			noclip_anglehack = false;
		}
		else
			cl.privileged = true;
	}

	if (Cmd_Argc () != 2)
		return;

	for (j=0, cl = svs.clients ; j<svs.maxclients ; j++, cl++)
	{
		if (!cl.active)
			continue;
		if (Q_strcasecmp(cl.name, Cmd_Argv(1)) == 0)
		{
			if (cl.privileged)
			{
				cl.privileged = false;
				cl.edict->v.flags = (int)cl.edict->v.flags & ~(FL_GODMODE|FL_NOTARGET);
				cl.edict->v.movetype = MOVETYPE_WALK;
				noclip_anglehack = false;
			}
			else
				cl.privileged = true;
			break;
		}
	}
}
#endif


void CQuakeHost::Host_Say(bool teamonly)
{
	client_t *client = NULL;
	client_t *save = NULL;
	int		j = 0;
	char*	p = NULL;
	char	text[64] = {};
	bool	fromServer = false;


	if (cmd_source == src_command)
	{
		if (cls.state == ca_dedicated)
		{
			fromServer = true;
			teamonly = false;
		}
		else
		{
			g_pCmds->Cmd_ForwardToServer ();
			return;
		}
	}

	if (g_pCmds->Cmd_Argc () < 2)
		return;
	
	p = mainzone->Z_Malloc<char>(strlen(g_pCmds->Cmd_Args())+1);	// Missi (12/7/2022)

	save = host_client;

	Q_strcat(p, g_pCmds->Cmd_Args());
// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[Q_strlen(p)-1] = 0;
	}

// turn on color set 1
	if (!fromServer)
		snprintf (text, sizeof(text), "%c%s: ", 1, save->name);
	else
		snprintf (text, sizeof(text), "%c<%s> ", 1, hostname.string);

	j = sizeof(text) - 2 - Q_strlen(text);  // -2 for /n and null terminator
	if (Q_strlen(p) > j)
		p[j] = 0;

	Q_strcat (text, p);
	Q_strcat (text, "\n");

	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client || !client->active || !client->spawned)
			continue;
		if (host->teamplay.value && teamonly && client->edict->v.team != save->edict->v.team)
			continue;
		
		client;
		sv->SV_ClientPrintf("%s", text);
	}
	host_client = save;

	Sys_Printf("%s", &text[1]);
}


void CQuakeHost::Host_Say_f()
{
	Host_Say(false);
}


void CQuakeHost::Host_Say_Team_f()
{
	Host_Say(true);
}

void CQuakeHost::Host_Bot_f()
{
	sv->SV_ConnectClient(1, true);
}

void CQuakeHost::Host_Tell_f()
{
	client_t *client;
	client_t *save;
	int		j;
	char*	p = {};
	char	text[64] = {};

	if (cmd_source == src_command)
	{
		g_pCmds->Cmd_ForwardToServer ();
		return;
	}

	if (g_pCmds->Cmd_Argc () < 3)
		return;

	Q_strcpy(text, host_client->name);
	Q_strcat(text, ": ");

	Q_strcat(p, g_pCmds->Cmd_Args());

// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[Q_strlen(p)-1] = 0;
	}

// check length & truncate if necessary
	j = sizeof(text) - 2 - Q_strlen(text);  // -2 for /n and null terminator
	if (Q_strlen(p) > j)
		p[j] = 0;

	Q_strcat (text, p);
	Q_strcat (text, "\n");

	save = host_client;
	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client->active || !client->spawned)
			continue;
		if (Q_strcasecmp(client->name, g_pCmds->Cmd_Argv(1)))
			continue;
		host_client = client;
		sv->SV_ClientPrintf("%s", text);
		break;
	}
	host_client = save;
}


/*
==================
Host_Color_f
==================
*/
void CQuakeHost::Host_Color_f()
{
	int		top, bottom;
	int		playercolor;
	
	if (g_pCmds->Cmd_Argc() == 1)
	{
		Con_Printf ("\"color\" is \"%i %i\"\n", ((int)cl_color.value) >> 4, ((int)cl_color.value) & 0x0f);
		Con_Printf ("color <0-13> [0-13]\n");
		return;
	}

	if (g_pCmds->Cmd_Argc() == 2)
		top = bottom = atoi(g_pCmds->Cmd_Argv(1));
	else
	{
		top = atoi(g_pCmds->Cmd_Argv(1));
		bottom = atoi(g_pCmds->Cmd_Argv(2));
	}
	
	top &= 15;
	if (top > 13)
		top = 13;
	bottom &= 15;
	if (bottom > 13)
		bottom = 13;
	
	playercolor = top*16 + bottom;

	if (cmd_source == src_command)
	{
		Cvar_SetValue ("_cl_color", playercolor);
		if (cls.state == ca_connected)
			g_pCmds->Cmd_ForwardToServer ();
		return;
	}

	host_client->colors = playercolor;
	host_client->edict->v.team = bottom + 1;

// send notification to all clients
	MSG_WriteByte (&sv->GetReliableDatagramBuffer(), svc_updatecolors);
	MSG_WriteByte (&sv->GetReliableDatagramBuffer(), host_client - svs.clients);
	MSG_WriteByte (&sv->GetReliableDatagramBuffer(), host_client->colors);
}

/*
==================
Host_Kill_f
==================
*/
void CQuakeHost::Host_Kill_f ()
{
	if (cmd_source == src_command)
	{
		g_pCmds->Cmd_ForwardToServer ();
		return;
	}

	if (sv_player->v.health <= 0)
	{
		sv->SV_ClientPrintf ("Can't suicide -- allready dead!\n");
		return;
	}
	
	pr_global_struct->time = sv->GetServerTime();
	pr_global_struct->self = EDICT_TO_PROG(sv_player);
	PR_ExecuteProgram (pr_global_struct->ClientKill);
}


/*
==================
Host_Pause_f
==================
*/
void CQuakeHost::Host_Pause_f ()
{
	
	if (cmd_source == src_command)
	{
		g_pCmds->Cmd_ForwardToServer ();
		return;
	}
	if (!host->pausable.value)
		sv->SV_ClientPrintf ("Pause not allowed.\n");
	else
	{
		int pause = sv->IsServerPaused();

		sv->SetServerPaused(pause ^= 1);

		if (sv->IsServerPaused())
		{
			sv->SV_BroadcastPrintf ("%s paused the game\n", PR_GetString(sv_player->v.netname));
		}
		else
		{
			sv->SV_BroadcastPrintf ("%s unpaused the game\n",PR_GetString(sv_player->v.netname));
		}

	// send notification to all clients
		MSG_WriteByte (&sv->GetReliableDatagramBuffer(), svc_setpause);
		MSG_WriteByte (&sv->GetReliableDatagramBuffer(), sv->IsServerPaused());
	}
}

//===========================================================================


/*
==================
Host_PreSpawn_f
==================
*/
void CQuakeHost::Host_PreSpawn_f ()
{
	if (cmd_source == src_command)
	{
		Con_Printf ("prespawn is not valid from the console\n");
		return;
	}

	if (host_client->spawned)
	{
		Con_Printf ("prespawn not valid -- allready spawned\n");
		return;
	}
	
	SZ_Write (&host_client->message, sv->GetSignOnBufferData(), sv->GetSignOnBufferCursize());
	MSG_WriteByte (&host_client->message, svc_signonnum);
	MSG_WriteByte (&host_client->message, 2);
	host_client->sendsignon = true;
}

/*
==================
Host_Spawn_f
==================
*/
void CQuakeHost::Host_Spawn_f ()
{
	int		i;
	client_t	*client;
	edict_t	*ent;

	if (cmd_source == src_command)
	{
		Con_Printf ("spawn is not valid from the console\n");
		return;
	}

	if (host_client->spawned)
	{
		Con_Printf ("Spawn not valid -- allready spawned\n");
		return;
	}

// run the entrance script
	if (sv->IsLoadGame())
	{	// loaded games are fully inited allready
		// if this is the last client to be connected, unpause
		sv->SetServerPaused(false);
	}
	else
	{
		// set up the edict
		ent = host_client->edict;

		memset (&ent->v, 0, progs->entityfields * 4);
		ent->v.colormap = NUM_FOR_EDICT(ent);
		ent->v.team = (host_client->colors & 15) + 1;
		ent->v.netname = PR_SetEngineString(host_client->name);

		// copy spawn parms out of the client_t

		for (i=0 ; i< NUM_SPAWN_PARMS ; i++)
			(&pr_global_struct->parm1)[i] = host_client->spawn_parms[i];

		// call the spawn function

		pr_global_struct->time = sv->GetServerTime();
		pr_global_struct->self = EDICT_TO_PROG(sv_player);
		PR_ExecuteProgram (pr_global_struct->ClientConnect);

		if ((Sys_DoubleTime() - host_client->netconnection->connecttime) <= sv->GetServerTime())
			Sys_Printf ("%s entered the game\n", host_client->name);

		PR_ExecuteProgram (pr_global_struct->PutClientInServer);	
	}


// send all current names, colors, and frag counts
	SZ_Clear (&host_client->message);

// send time of update
	MSG_WriteByte (&host_client->message, svc_time);
	MSG_WriteFloat (&host_client->message, sv->GetServerTime());

	for (i=0, client = svs.clients ; i<svs.maxclients ; i++, client++)
	{
		MSG_WriteByte (&host_client->message, svc_updatename);
		MSG_WriteByte (&host_client->message, i);
		MSG_WriteString (&host_client->message, client->name);
		MSG_WriteByte (&host_client->message, svc_updatefrags);
		MSG_WriteByte (&host_client->message, i);
		MSG_WriteShort (&host_client->message, client->old_frags);
		MSG_WriteByte (&host_client->message, svc_updatecolors);
		MSG_WriteByte (&host_client->message, i);
		MSG_WriteByte (&host_client->message, client->colors);
	}
	
// send all current light styles
	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		MSG_WriteByte (&host_client->message, svc_lightstyle);
		MSG_WriteByte (&host_client->message, (char)i);
		MSG_WriteString (&host_client->message, sv->GetLightStyles()[i]);
	}

//
// send some stats
//
	MSG_WriteByte (&host_client->message, svc_updatestat);
	MSG_WriteByte (&host_client->message, STAT_TOTALSECRETS);
	MSG_WriteLong (&host_client->message, pr_global_struct->total_secrets);

	MSG_WriteByte (&host_client->message, svc_updatestat);
	MSG_WriteByte (&host_client->message, STAT_TOTALMONSTERS);
	MSG_WriteLong (&host_client->message, pr_global_struct->total_monsters);

	MSG_WriteByte (&host_client->message, svc_updatestat);
	MSG_WriteByte (&host_client->message, STAT_SECRETS);
	MSG_WriteLong (&host_client->message, pr_global_struct->found_secrets);

	MSG_WriteByte (&host_client->message, svc_updatestat);
	MSG_WriteByte (&host_client->message, STAT_MONSTERS);
	MSG_WriteLong (&host_client->message, pr_global_struct->killed_monsters);

	
//
// send a fixangle
// Never send a roll angle, because savegames can catch the server
// in a state where it is expecting the client to correct the angle
// and it won't happen if the game was just loaded, so you wind up
// with a permanent head tilt
	ent = EDICT_NUM( 1 + (host_client - svs.clients) );
	MSG_WriteByte (&host_client->message, svc_setangle);
	for (i=0 ; i < 2 ; i++)
		MSG_WriteAngle (&host_client->message, ent->v.angles[i] );
	MSG_WriteAngle (&host_client->message, 0 );

	sv->SV_WriteClientdataToMessage (sv_player, &host_client->message);

	MSG_WriteByte (&host_client->message, svc_signonnum);
	MSG_WriteByte (&host_client->message, 3);
	host_client->sendsignon = true;
}

/*
==================
Host_Begin_f
==================
*/
void CQuakeHost::Host_Begin_f ()
{
	if (cmd_source == src_command)
	{
		Con_Printf ("begin is not valid from the console\n");
		return;
	}

	host_client->spawned = true;
}

//===========================================================================


/*
==================
Host_Kick_f

Kicks a user off of the server
==================
*/
void CQuakeHost::Host_Kick_f ()
{
	const char		*who = nullptr;
	const char		*message = nullptr;
	client_t	*save = nullptr;
	int			i;
	bool	byNumber = false;

	if (cmd_source == src_command)
	{
		if (!sv->IsServerActive())
		{
			g_pCmds->Cmd_ForwardToServer ();
			return;
		}
	}
	else if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	save = host_client;

	if (g_pCmds->Cmd_Argc() > 2 && Q_strcmp(g_pCmds->Cmd_Argv(1), "#") == 0)
	{
		i = Q_atof(g_pCmds->Cmd_Argv(2)) - 1;
		if (i < 0 || i >= svs.maxclients)
			return;
		if (!svs.clients[i].active)
			return;
		host_client = &svs.clients[i];
		byNumber = true;
	}
	else
	{
		for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
		{
			if (!host_client->active)
				continue;
			if (Q_strcasecmp(host_client->name, g_pCmds->Cmd_Argv(1)) == 0)
				break;
		}
	}

	if (i < svs.maxclients)
	{
		if (cmd_source == src_command)
			if (cls.state == ca_dedicated)
				who = "Console";
			else
				who = cl_name.string;
		else
			who = save->name;

		// can't kick yourself!
		if (host_client == save)
			return;

		if (g_pCmds->Cmd_Argc() > 2)
		{
			message = g_Common->COM_Parse(g_pCmds->Cmd_Args());
			if (byNumber)
			{
				message++;							// skip the #
				while (*message == ' ')				// skip white space
					message++;
				message += Q_strlen(g_pCmds->Cmd_Argv(2));	// skip the number
			}
			while (*message && *message == ' ')
				message++;
		}
		if (message)
			sv->SV_ClientPrintf ("Kicked by %s: %s\n", who, message);
		else
			sv->SV_ClientPrintf ("Kicked by %s\n", who);
		sv->SV_DropClient (false);
	}

	host_client = save;
}

/*
===============================================================================

DEBUGGING TOOLS

===============================================================================
*/

/*
==================
Host_Give_f
==================
*/
void CQuakeHost::Host_Give_f ()
{
	char	t[256];
	int		v;
	eval_t	*val;

	if (cmd_source == src_command)
	{
		g_pCmds->Cmd_ForwardToServer ();
		return;
	}

	if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	Q_strncpy(t, g_pCmds->Cmd_Argv(1), sizeof(t));
	v = atoi (g_pCmds->Cmd_Argv(2));
	
	switch (t[0])
	{
   case '0':
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
   case '9':
      // MED 01/04/97 added hipnotic give stuff
      if (hipnotic)
      {
         if (t[0] == '6')
         {
            if (t[1] == 'a')
               sv_player->v.items = (int)sv_player->v.items | HIT_PROXIMITY_GUN;
            else
               sv_player->v.items = (int)sv_player->v.items | IT_GRENADE_LAUNCHER;
         }
         else if (t[0] == '9')
            sv_player->v.items = (int)sv_player->v.items | HIT_LASER_CANNON;
         else if (t[0] == '0')
            sv_player->v.items = (int)sv_player->v.items | HIT_MJOLNIR;
         else if (t[0] >= '2')
            sv_player->v.items = (int)sv_player->v.items | (IT_SHOTGUN << (t[0] - '2'));
      }
      else
      {
         if (t[0] >= '2')
            sv_player->v.items = (int)sv_player->v.items | (IT_SHOTGUN << (t[0] - '2'));
      }
		break;
	
    case 's':
		if (rogue)
		{
	        val = GetEdictFieldValue(sv_player, "ammo_shells1");
		    if (val)
			    val->_float = v;
		}

        sv_player->v.ammo_shells = v;
        break;		
    case 'n':
		if (rogue)
		{
			val = GetEdictFieldValue(sv_player, "ammo_nails1");
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon <= IT_LIGHTNING)
					sv_player->v.ammo_nails = v;
			}
		}
		else
		{
			sv_player->v.ammo_nails = v;
		}
        break;		
    case 'l':
		if (rogue)
		{
			val = GetEdictFieldValue(sv_player, "ammo_lava_nails");
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon > IT_LIGHTNING)
					sv_player->v.ammo_nails = v;
			}
		}
        break;
    case 'r':
		if (rogue)
		{
			val = GetEdictFieldValue(sv_player, "ammo_rockets1");
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon <= IT_LIGHTNING)
					sv_player->v.ammo_rockets = v;
			}
		}
		else
		{
			sv_player->v.ammo_rockets = v;
		}
        break;		
    case 'm':
		if (rogue)
		{
			val = GetEdictFieldValue(sv_player, "ammo_multi_rockets");
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon > IT_LIGHTNING)
					sv_player->v.ammo_rockets = v;
			}
		}
        break;		
    case 'h':
        sv_player->v.health = v;
        break;		
    case 'c':
		if (rogue)
		{
			val = GetEdictFieldValue(sv_player, "ammo_cells1");
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon <= IT_LIGHTNING)
					sv_player->v.ammo_cells = v;
			}
		}
		else
		{
			sv_player->v.ammo_cells = v;
		}
        break;		
    case 'p':
		if (rogue)
		{
			val = GetEdictFieldValue(sv_player, "ammo_plasma");
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon > IT_LIGHTNING)
					sv_player->v.ammo_cells = v;
			}
		}
        break;		
    }
}

edict_t	*FindViewthing ()
{
	int		i;
	edict_t	*e;
	
	for (i=0 ; i<sv->GetNumEdicts() ; i++)
	{
		e = EDICT_NUM(i);
		if ( !strcmp (PR_GetString(e->v.classname), "viewthing") )
			return e;
	}
	Con_Printf ("No viewthing on map\n");
	return NULL;
}

/*
==================
Host_Viewmodel_f
==================
*/
void CQuakeHost::Host_Viewmodel_f ()
{
	edict_t	*e;
	model_t	*m;

	e = FindViewthing ();
	if (!e)
		return;

	m = Mod_ForName (g_pCmds->Cmd_Argv(1), false);
	if (!m)
	{
		Con_Printf ("Can't load %s\n", g_pCmds->Cmd_Argv(1));
		return;
	}
	
	e->v.frame = 0;
	cl.model_precache[(int)e->v.modelindex] = m;
}

/*
==================
Host_Viewframe_f
==================
*/
void CQuakeHost::Host_Viewframe_f ()
{
	edict_t	*e;
	int		f;
	model_t	*m;

	e = FindViewthing ();
	if (!e)
		return;
	m = cl.model_precache[(int)e->v.modelindex];

	f = atoi(g_pCmds->Cmd_Argv(1));
	if (f >= m->numframes)
		f = m->numframes-1;

	e->v.frame = f;		
}


void PrintFrameName (model_t *m, int frame)
{
	aliashdr_t 			*hdr;
	maliasframedesc_t	*pframedesc;
#ifndef GLQUAKE
	hdr = (aliashdr_t *)Mod_Extradata(m);
#else
	hdr = Mod_Extradata<aliashdr_t>(m);
#endif
	if (!hdr)
		return;
	pframedesc = &hdr->frames[frame];
	
	Con_Printf ("frame %i: %s\n", frame, pframedesc->name);
}

/*
==================
Host_Viewnext_f
==================
*/
void CQuakeHost::Host_Viewnext_f ()
{
	edict_t	*e;
	model_t	*m;
	
	e = FindViewthing ();
	if (!e)
		return;
	m = cl.model_precache[(int)e->v.modelindex];

	e->v.frame = e->v.frame + 1;
	if (e->v.frame >= m->numframes)
		e->v.frame = m->numframes - 1;

	PrintFrameName (m, e->v.frame);		
}

/*
==================
Host_Viewprev_f
==================
*/
void CQuakeHost::Host_Viewprev_f ()
{
	edict_t	*e;
	model_t	*m;

	e = FindViewthing ();
	if (!e)
		return;

	m = cl.model_precache[(int)e->v.modelindex];

	e->v.frame = e->v.frame - 1;
	if (e->v.frame < 0)
		e->v.frame = 0;

	PrintFrameName (m, e->v.frame);		
}

/*
==================
Host_Viewprev_f

Missi: modified. removed the need for a second argument and added help text (5/1/2023)
==================
*/
void Mod_GetPos()
{
	edict_t* e = NULL;
	model_t* m = NULL;
	int v = 0;

	v = atoi(g_pCmds->Cmd_Argv(1));

	if (!v)
	{
		Con_Printf("Usage: getpos (edict num)\n");
		return;
	}

	e = EDICT_NUM(v);
	if (!e)
		return;

	Con_Printf("%f %f %f", e->v.origin[0], e->v.origin[1], e->v.origin[2]);
}

/*
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


/*
==================
Host_Startdemos_f
==================
*/
void CQuakeHost::Host_Startdemos_f ()
{
	int		i, c;

	if (cls.state == ca_dedicated)
	{
		if (!sv->IsServerActive())
			g_pCmdBuf->Cbuf_AddText ("map start\n");
		return;
	}

	c = g_pCmds->Cmd_Argc() - 1;
	if (c > MAX_DEMOS)
	{
		Con_Printf ("Max %i demos in demoloop\n", MAX_DEMOS);
		c = MAX_DEMOS;
	}
	Con_Printf ("%i demo(s) in loop\n", c);

	for (i=1 ; i<c+1 ; i++)
		Q_strncpy (cls.demos[i-1], g_pCmds->Cmd_Argv(i), sizeof(cls.demos[0])-1);

	if (!sv->IsServerActive() && cls.demonum != -1 && !cls.demoplayback)
	{
		cls.demonum = 0;
		CL_NextDemo ();
	}
	else
		cls.demonum = -1;
}


/*
==================
Host_Demos_f

Return to looping demos
==================
*/
void CQuakeHost::Host_Demos_f ()
{
	if (cls.state == ca_dedicated)
		return;
	if (cls.demonum == -1)
		cls.demonum = 1;
	CL_Disconnect_f ();
	CL_NextDemo ();
}

/*
==================
Host_Stopdemo_f

Return to looping demos
==================
*/
void CQuakeHost::Host_Stopdemo_f ()
{
	if (cls.state == ca_dedicated)
		return;
	if (!cls.demoplayback)
		return;
	CL_StopPlayback ();
	CL_Disconnect ();
}

//=============================================================================

/*
==================
Host_InitCommands
==================
*/
void CQuakeHost::Host_InitCommands ()
{
	g_pCmds->Cmd_AddCommand ("status", Host_Status_f);
	g_pCmds->Cmd_AddCommand ("quit", Host_Quit_f);
    g_pCmds->Cmd_AddCommand ("god", Host_God_f, CVAR_CHEAT);
    g_pCmds->Cmd_AddCommand ("notarget", Host_Notarget_f, CVAR_CHEAT);
    g_pCmds->Cmd_AddCommand ("fly", Host_Fly_f, CVAR_CHEAT);
	g_pCmds->Cmd_AddCommand ("map", Host_Map_f);
	g_pCmds->Cmd_AddCommand ("restart", Host_Restart_f);
	g_pCmds->Cmd_AddCommand ("changelevel", Host_Changelevel_f);
#ifdef QUAKE2
	g_pCmds->Cmd_AddCommand ("changelevel2", Host_Changelevel2_f);
#endif
	g_pCmds->Cmd_AddCommand ("connect", Host_Connect_f);
	g_pCmds->Cmd_AddCommand ("reconnect", Host_Reconnect_f);
	g_pCmds->Cmd_AddCommand ("name", Host_Name_f);
    g_pCmds->Cmd_AddCommand ("noclip", Host_Noclip_f, CVAR_CHEAT);
	g_pCmds->Cmd_AddCommand ("version", Host_Version_f);
#ifdef IDGODS
	g_pCmds->Cmd_AddCommand ("please", Host_Please_f);
#endif
	g_pCmds->Cmd_AddCommand ("say", Host_Say_f);
	g_pCmds->Cmd_AddCommand ("say_team", Host_Say_Team_f);
	g_pCmds->Cmd_AddCommand ("tell", Host_Tell_f);
	g_pCmds->Cmd_AddCommand ("color", Host_Color_f);
	g_pCmds->Cmd_AddCommand ("kill", Host_Kill_f);
	g_pCmds->Cmd_AddCommand ("pause", Host_Pause_f);
	g_pCmds->Cmd_AddCommand ("spawn", Host_Spawn_f);
	g_pCmds->Cmd_AddCommand ("begin", Host_Begin_f);
	g_pCmds->Cmd_AddCommand ("prespawn", Host_PreSpawn_f);
	g_pCmds->Cmd_AddCommand ("kick", Host_Kick_f);
	g_pCmds->Cmd_AddCommand ("ping", Host_Ping_f);
	g_pCmds->Cmd_AddCommand ("load", Host_Loadgame_f);
	g_pCmds->Cmd_AddCommand ("save", Host_Savegame_f);
	g_pCmds->Cmd_AddCommand ("give", Host_Give_f);

	g_pCmds->Cmd_AddCommand ("startdemos", Host_Startdemos_f);
	g_pCmds->Cmd_AddCommand ("demos", Host_Demos_f);
	g_pCmds->Cmd_AddCommand ("stopdemo", Host_Stopdemo_f);

	g_pCmds->Cmd_AddCommand ("viewmodel", Host_Viewmodel_f);
	g_pCmds->Cmd_AddCommand ("viewframe", Host_Viewframe_f);
	g_pCmds->Cmd_AddCommand ("viewnext", Host_Viewnext_f);
	g_pCmds->Cmd_AddCommand ("viewprev", Host_Viewprev_f);

	g_pCmds->Cmd_AddCommand ("mcache", Mod_Print);

	g_pCmds->Cmd_AddCommand("getpos", Mod_GetPos);
}
