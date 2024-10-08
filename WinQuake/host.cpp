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
// host.c -- coordinates spawning and killing of local servers

#include "quakedef.h"

/*

A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

CQuakeHost* host;
CCoreRenderer* g_CoreRenderer;
client_t* host_client;
double		host_time;

#ifdef GLQUAKE
CGLRenderer* g_GLRenderer;
#else
CSoftwareRenderer* g_SoftwareRenderer;
#endif

//cvar_t	teamplay;
//cvar_t	skill;
//cvar_t	deathmatch;
//cvar_t	coop;
//cvar_t	fraglimit;
//cvar_t	timelimit;
//cvar_t	pausable;

/*
================
Host_EndGame
================
*/
void CQuakeHost::Host_EndGame (const char *message, ...)
{
	va_list		argptr;
	char		string[1024];
	
	va_start (argptr,message);
	vsnprintf (string, sizeof(string), message,argptr);
	va_end (argptr);
	Con_DPrintf ("Host_EndGame: %s\n",string);
	
	if (sv->IsServerActive())
		Host_ShutdownServer (false);

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_EndGame: %s\n",string);	// dedicated servers exit
	
	if (cls.demonum != -1)
		CL_NextDemo ();
	else
		CL_Disconnect ();

	longjmp(host_abortserver, 1);
}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void CQuakeHost::Host_Error (const char *error, ...)
{
	va_list		argptr;
	char		string[1024];
	static	bool inerror = false;
	
	if (inerror)
		Sys_Error ("Host_Error: recursively entered");
	inerror = true;
	
	SCR_EndLoadingPlaque ();		// reenable screen updates

	va_start (argptr,error);
	vsnprintf (string, sizeof(string), error,argptr);
	va_end (argptr);
	Con_Printf ("Host_Error: %s\n",string);
	
	if (sv->IsServerActive())
		Host_ShutdownServer (false);

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_Error: %s\n",string);	// dedicated servers exit

	CL_Disconnect ();
	cls.demonum = -1;

	inerror = false;

	longjmp(host_abortserver, 1);
}

/*
================
Host_FindMaxClients
================
*/
void CQuakeHost::Host_FindMaxClients ()
{
	int		i;

	svs.maxclients = 1;
		
	i = g_Common->COM_CheckParm ("-dedicated");
	if (i)
	{
		cls.state = ca_dedicated;
		if (i != (g_Common->com_argc - 1))
		{
			svs.maxclients = Q_atoi (g_Common->com_argv[i+1]);
		}
		else
			svs.maxclients = 8;
	}
	else
		cls.state = ca_disconnected;

	i = g_Common->COM_CheckParm ("-listen");
	if (i)
	{
		if (cls.state == ca_dedicated)
			Sys_Error ("Only one of -dedicated or -listen can be specified");
		if (i != (g_Common->com_argc - 1))
			svs.maxclients = Q_atoi (g_Common->com_argv[i+1]);
		else
			svs.maxclients = 8;
	}
	if (svs.maxclients < 1)
		svs.maxclients = 8;
	else if (svs.maxclients > MAX_SCOREBOARD)
		svs.maxclients = MAX_SCOREBOARD;

	svs.maxclientslimit = svs.maxclients;
	if (svs.maxclientslimit < 4)
		svs.maxclientslimit = 4;
	svs.clients = g_MemCache->Hunk_AllocName<struct client_s>(svs.maxclientslimit*sizeof(client_t), "clients");

	if (svs.maxclients > 1)
		Cvar_SetValue ("deathmatch", 1.0f);
	else
		Cvar_SetValue ("deathmatch", 0.0f);
}


/*
=======================
Host_InitLocal
======================
*/
void CQuakeHost::Host_InitLocal ()
{
	Host_InitCommands ();
	
	Cvar_RegisterVariable (&host_framerate);
	Cvar_RegisterVariable (&host_speeds);
	Cvar_RegisterVariable (&host_timescale);

	Cvar_RegisterVariable (&sys_ticrate);
	Cvar_RegisterVariable (&serverprofile);

	Cvar_RegisterVariable (&fraglimit);
	Cvar_RegisterVariable (&timelimit);
	Cvar_RegisterVariable (&teamplay);
	Cvar_RegisterVariable (&samelevel);
	Cvar_RegisterVariable (&noexit);
	Cvar_RegisterVariable (&skill);
	Cvar_RegisterVariable (&developer);
	Cvar_RegisterVariable (&deathmatch);
	Cvar_RegisterVariable (&coop);

	Cvar_RegisterVariable (&pausable);

	g_pCmds->Cmd_AddCommand("bot", CQuakeHost::Host_Bot_f, CVAR_CHEAT);

	Host_FindMaxClients ();
	
	host_time = 1.0;		// so a think at time 0 won't get called
}


/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void CQuakeHost::Host_WriteConfiguration ()
{
// dedicated servers initialize the host but don't parse and set the
// config.cfg cvars
	if (host_initialized && !isDedicated)
    {
		cxxofstream	f(g_Common->va("%s/config.cfg", g_Common->com_gamedir), std::ios_base::binary);

        if (f.bad())
		{
            Con_Printf ("Couldn't write config.cfg.\n");
            f.close();
			return;
		}
		
		Key_WriteBindings (f);
		Cvar_WriteVariables (f);

        f.close();
	}
}


/*
=================
SV_ClientPrintf

Sends text across to be displayed 
FIXME: make this just a stuffed echo?
=================
*/
void CQuakeServer::SV_ClientPrintf (const char *fmt, ...)
{
	va_list		argptr = {};
	char		string[1024] = {};
	
	va_start (argptr,fmt);
	vsnprintf (string, sizeof(string), fmt,argptr);
	va_end (argptr);
	
	MSG_WriteByte (&host_client->message, svc_print);
	MSG_WriteString (&host_client->message, string);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void CQuakeServer::SV_BroadcastPrintf (const char *fmt, ...)
{
	va_list		argptr = {};
	char		string[1024] = {};
	int			i = 0;
	
	va_start (argptr,fmt);
	vsnprintf (string, sizeof(string), fmt,argptr);
	va_end (argptr);
	
	for (i=0 ; i<svs.maxclients ; i++)
		if (svs.clients[i].active && svs.clients[i].spawned)
		{
			MSG_WriteByte (&svs.clients[i].message, svc_print);
			MSG_WriteString (&svs.clients[i].message, string);
		}
}

/*
=================
Host_ClientCommands

Send text over to the client to be executed
=================
*/
void CQuakeHost::Host_ClientCommands (const char *fmt, ...)
{
	va_list		argptr = {};
	char		string[1024] = {};
	
	va_start (argptr,fmt);
	vsnprintf (string, sizeof(string), fmt, argptr);
	va_end (argptr);
	
	MSG_WriteByte (&host_client->message, svc_stufftext);
	MSG_WriteString (&host_client->message, string);
}

/*
=====================
SV_DropClient

Called when the player is getting totally kicked off the host
if (crash = true), don't bother sending signofs
=====================
*/
void CQuakeServer::SV_DropClient (bool crash)
{
	int		saveSelf = 0;
	int		i = 0;
	client_t *client = nullptr;

	if (!crash)
	{
		// send any final messages (don't check for errors)
		if (NET_CanSendMessage (host_client->netconnection))
		{
			MSG_WriteByte (&host_client->message, svc_disconnect);
			NET_SendMessage (host_client->netconnection, &host_client->message);
		}
	
		if (host_client->edict && host_client->spawned)
		{
		// call the prog function for removing a client
		// this will set the body to a dead frame, among other things
			saveSelf = pr_global_struct->self;
			pr_global_struct->self = EDICT_TO_PROG(host_client->edict);
			PR_ExecuteProgram (pr_global_struct->ClientDisconnect);
			pr_global_struct->self = saveSelf;
		}

		Sys_Printf ("Client %s removed\n",host_client->name);
	}

// break the net connection
	NET_Close (host_client->netconnection);
	host_client->netconnection = NULL;

// free the client (the body stays around)
	host_client->active = false;
	host_client->name[0] = 0;
	host_client->old_frags = -999999;
	net_activeconnections--;

// send notification to all clients
	for (i=0, client = svs.clients ; i<svs.maxclients ; i++, client++)
	{
		if (!client->active)
			continue;
		MSG_WriteByte (&client->message, svc_updatename);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteString (&client->message, "");
		MSG_WriteByte (&client->message, svc_updatefrags);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteShort (&client->message, 0);
		MSG_WriteByte (&client->message, svc_updatecolors);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteByte (&client->message, 0);
	}
}

/*
==================
Host_ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
void CQuakeHost::Host_ShutdownServer(bool crash)
{
	int			i = 0;
	int			count = 0;
	sizebuf_t	buf = {};
	char		message[4] = {};
	double		start = 0.0;

	if (!sv->IsServerActive())
		return;

	sv->SetServerActive(false);

// stop all client sounds immediately
	if (cls.state == ca_connected)
		CL_Disconnect ();

// flush any pending messages - like the score!!!
	start = Sys_DoubleTime();
	do
	{
		count = 0;
		for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
		{
			if (host_client->active && host_client->message.cursize)
			{
				if (NET_CanSendMessage (host_client->netconnection))
				{
					NET_SendMessage(host_client->netconnection, &host_client->message);
					SZ_Clear (&host_client->message);
				}
				else
				{
					NET_GetMessage(host_client->netconnection);
					count++;
				}
			}
		}
		if ((Sys_DoubleTime() - start) > 3.0)
			break;
	}
	while (count);

// make sure all the clients know we're disconnecting
	buf.data = (byte*)message;
	buf.maxsize = 4;
	buf.cursize = 0;
	MSG_WriteByte(&buf, svc_disconnect);
	count = NET_SendToAll(&buf, 5);
	if (count)
		Con_Printf("Host_ShutdownServer: NET_SendToAll failed for %u clients\n", count);

	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
		if (host_client->active)
			sv->SV_DropClient(crash);

//
// clear structures
//

	if (pr_vectors && progs->numvectors > 0)
	{
		for (int i = 0; i < progs->numvectors; i++)
		{
			pr_vectors[i].data->Clear();
		}

		memset(&pr_vectors, 0, sizeof(pr_vectors));
	}

	memset (svs.clients, 0, svs.maxclientslimit*sizeof(client_t));
}

CQuakeHost::CQuakeHost() :
	host_initialized(false),
	host_frametime(0),
	host_basepal(NULL),
	host_colormap(NULL),
	host_framecount(0),
	realtime(0),
	msg_suppress_1(false),
	isDedicated(false),
	host_died(false),
    minimum_memory(0),
	oldrealtime(0),
    host_hunklevel(0),
	sys_nostdout()
{
    memset(&host_parms, 0, sizeof(host_parms));
}

CQuakeHost::CQuakeHost(quakeparms_t<byte*> parms) :
	host_parms(parms),
	host_initialized(false),
	host_frametime(0),
	host_basepal(NULL),
	host_colormap(NULL),
	host_framecount(0),
	realtime(0),
	msg_suppress_1(false),
	isDedicated(false),
	host_died(false),
    minimum_memory(0),
	oldrealtime(0),
    host_hunklevel(0),
	sys_nostdout()
{
	host_parms.basedir = parms.basedir ? parms.basedir : strdup("");
	host_parms.cachedir = parms.cachedir ? parms.cachedir : strdup("");
}

/*
================
Host_ClearMemory

This clears all the memory used by both the client and server, but does
not reinitialize anything.
================
*/
void CQuakeHost::Host_ClearMemory ()
{
	Con_DPrintf ("Clearing memory\n");
	D_FlushCaches ();
	Mod_ClearAll ();
	if (host_hunklevel)
		g_MemCache->Hunk_FreeToLowMark(host_hunklevel);

    cls.signon = 0;

    memset(sv, 0, sizeof(*sv));
	memset(&cl, 0, sizeof(cl));
}


//============================================================================


/*
===================
Host_FilterTime

Returns false if the time is too short to run a frame
===================
*/
bool CQuakeHost::Host_FilterTime (float time)
{
	realtime += time;

	if (!cls.timedemo && realtime - oldrealtime < 1.0/72.0)
		return false;		// framerate is too high

	host_frametime = (realtime - oldrealtime) * host_timescale.value;
	oldrealtime = realtime;

	if (host_framerate.value > 0)
		host_frametime = host_framerate.value;
	else
	{	// don't allow really long or short frames
		if (host_frametime > 0.1)
			host_frametime = 0.1;
		if (host_frametime < 0.001)
			host_frametime = 0.001;
	}
	
	return true;
}


/*
===================
Host_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void CQuakeHost::Host_GetConsoleCommands ()
{
	char	*cmd = nullptr;

	if (!isDedicated)
		return;

	while (1)
	{
		cmd = Sys_ConsoleInput ();
		if (!cmd)
			break;
		g_pCmdBuf->Cbuf_AddText (cmd);
	}
}


/*
==================
Host_ServerFrame

==================
*/
#ifdef FPS_20

void _Host_ServerFrame ()
{
// run the world state	
	pr_global_struct->frametime = host_frametime;

// read client messages
	SV_RunClients ();
	
// move things around and think
// always pause in single player if in console or menus
	if (!sv->paused && (svs.maxclients > 1 || key_dest == key_game) )
		SV_Physics ();
}

void Host_ServerFrame ()
{
	float	save_host_frametime;
	float	temp_host_frametime;

// run the world state	
	pr_global_struct->frametime = host_frametime;

// set the time and clear the general datagram
	SV_ClearDatagram ();
	
// check for new clients
	SV_CheckForNewClients ();

	temp_host_frametime = save_host_frametime = host_frametime;
	while(temp_host_frametime > (1.0/72.0))
	{
		if (temp_host_frametime > 0.05)
			host_frametime = 0.05;
		else
			host_frametime = temp_host_frametime;
		temp_host_frametime -= host_frametime;
		_Host_ServerFrame ();
	}
	host_frametime = save_host_frametime;

// send all messages to the clients
	SV_SendClientMessages ();
}

#else

void CQuakeHost::Host_ServerFrame ()
{
// run the world state	
	pr_global_struct->frametime = host_frametime;

// set the time and clear the general datagram
	sv->SV_ClearDatagram ();
	
// check for new clients
	sv->SV_CheckForNewClients ();

// read client messages
	sv->SV_RunClients ();
	
// move things around and think
// always pause in single player if in console or menus
	if (!sv->IsServerPaused() && (svs.maxclients > 1 || key_dest == key_game) )
		sv->SV_Physics ();

// send all messages to the clients
	sv->SV_SendClientMessages ();
}

#endif


/*
==================
Host_Frame

Runs all active servers
==================
*/
void CQuakeHost::_Host_Frame (float time)
{
	static double		time1 = 0.0;
	static double		time2 = 0.0;
	static double		time3 = 0.0;
	int			pass1 = 0, pass2 = 0, pass3 = 0;

	if ( setjmp(host_abortserver) )
		return;			// something bad happened, or the server disconnected

// keep the random time dependent
	rand ();
	
// decide the simulation time
	if (!Host_FilterTime (time))
		return;			// don't run too fast, or packets will flood out
		
// get new key events
	Sys_SendKeyEvents ();

// allow mice or other external controllers to add commands
	IN_Commands ();

// process console commands
	g_pCmdBuf->Cbuf_Execute ();

	NET_Poll();

// if running the server locally, make intentions now
	if (sv->IsServerActive())
		CL_SendCmd ();
	
//-------------------
//
// server operations
//
//-------------------

// check for commands typed to the host
	Host_GetConsoleCommands ();
	
	if (sv->IsServerActive())
		Host_ServerFrame ();

//-------------------
//
// client operations
//
//-------------------

// if running the server remotely, send intentions now after
// the incoming messages have been read
	if (!sv->IsServerActive())
		CL_SendCmd ();

	host_time += host_frametime;

// fetch results from server
	if (cls.state == ca_connected)
	{
		CL_ReadFromServer ();
	}

// update video
	if (host_speeds.value)
		time1 = Sys_DoubleTime ();
		
	SCR_UpdateScreen ();

	if (host_speeds.value)
		time2 = Sys_DoubleTime ();
		
// update audio
	if (cls.state != ca_dedicated)
	{
		g_pBGM->BGM_Update();

		if (cls.signon == SIGNONS)
		{
			g_SoundSystem->S_Update(r_origin, vpn, vright, vup);
			CL_DecayLights();
		}
		else
			g_SoundSystem->S_Update(vec3_origin, vec3_origin, vec3_origin, vec3_origin);

		CDAudio_Update();
	}

	if (host_speeds.value)
	{
		pass1 = (time1 - time3)*1000;
		time3 = Sys_DoubleTime ();
		pass2 = (time2 - time1)*1000;
		pass3 = (time3 - time2)*1000;
		Con_Printf ("%3i tot %3i server %3i gfx %3i snd\n",
					pass1+pass2+pass3, pass1, pass2, pass3);
	}

	host_framecount++;

	for (int i = 0; i < g_pTimers.GetNumAllocated(); i++)
	{
		g_pTimers[i].UpdateTimer();
	}
}

void CQuakeHost::Host_Frame (float time)
{
	double	time1 = 0.0, time2 = 0.0;
	static double	timetotal = 0.0;
	static int		timecount = 0;
	int		i = 0, c = 0, m = 0;

	if (!serverprofile.value)
	{
		_Host_Frame (time);
		return;
	}
	
	time1 = Sys_DoubleTime ();
	_Host_Frame (time);
	time2 = Sys_DoubleTime ();	
	
	timetotal += time2 - time1;
	timecount++;
	
	if (timecount < 1000)
		return;

	m = timetotal*1000/timecount;
	timecount = 0;
	timetotal = 0;
	c = 0;
	for (i=0 ; i<svs.maxclients ; i++)
	{
		if (svs.clients[i].active)
			c++;
	}

	Con_Printf ("serverprofile: %2i clients %2i msec\n",  c,  m);
}

//============================================================================


//extern int vcrFile;
//#define	VCR_SIGNATURE	0x56435231
//// "VCR1"
//
//void CQuakeHost::Host_InitVCR (quakeparms_t<byte*> parms)
//{
//	int		i, len, n;
//	char	*p;
//	
//	if (g_Common->COM_CheckParm("-playback"))
//	{
//		if (com_argc != 2)
//			Sys_Error("No other parameters allowed with -playback\n");
//
//		Sys_FileOpenRead("quake.vcr", &vcrFile);
//		if (vcrFile == -1)
//			Sys_Error("playback file not found\n");
//
//		Sys_FileRead (vcrFile, &i, sizeof(int));
//		if (i != VCR_SIGNATURE)
//			Sys_Error("Invalid signature in vcr file\n");
//
//		Sys_FileRead (vcrFile, &com_argc, sizeof(int));
//		com_argv = static_cast<char**>(malloc(com_argc * sizeof(char *)));
//		com_argv[0] = parms.argv[0];
//		for (i = 0; i < com_argc; i++)
//		{
//			Sys_FileRead (vcrFile, &len, sizeof(int));
//			p = static_cast<char*>(malloc(len));
//			Sys_FileRead (vcrFile, p, len);
//			com_argv[i+1] = p;
//		}
//		com_argc++; /* add one for arg[0] */
//		parms.argc = com_argc;
//		parms.argv = com_argv;
//	}
//
//	if ( (n = COM_CheckParm("-record")) != 0)
//	{
//		vcrFile = Sys_FileOpenWrite("quake.vcr");
//
//		i = VCR_SIGNATURE;
//		Sys_FileWrite(vcrFile, &i, sizeof(int));
//		i = com_argc - 1;
//		Sys_FileWrite(vcrFile, &i, sizeof(int));
//		for (i = 1; i < com_argc; i++)
//		{
//			if (i == n)
//			{
//				len = 10;
//				Sys_FileWrite(vcrFile, &len, sizeof(int));
//				Sys_FileWrite(vcrFile, "-playback", len);
//				continue;
//			}
//			len = Q_strlen(com_argv[i]) + 1;
//			Sys_FileWrite(vcrFile, &len, sizeof(int));
//			Sys_FileWrite(vcrFile, com_argv[i], len);
//		}
//	}
//	
//}

/*
====================
Host_Init
====================
*/
void CQuakeHost::Host_Init (quakeparms_t<byte*> parms)
{

	if (SDL_Init(0) < 0)
		Sys_Error("Couldn't initialize SDL");

    if (standard_quake)
        minimum_memory = MINIMUM_MEMORY;
	else
        minimum_memory = MINIMUM_MEMORY_LEVELPAK;

	if (g_Common->COM_CheckParm ("-minmemory"))
		parms.memsize = minimum_memory;

	host_parms = parms;

	if (parms.memsize < minimum_memory)
		Sys_Error ("Only %4.1f megs of memory available, can't execute game", parms.memsize / (float)0x100000);

	memcpy(&g_Common->com_argc, &parms.argc, sizeof(parms.argc));
	memcpy(&g_Common->com_argv, &parms.argv, sizeof(parms.argv));

	g_MemCache = new CMemCache;
	g_MemCache->Memory_Init (host_parms.membase, host_parms.memsize);
	g_pCmds = new CCommand;
	g_pCmdBuf = new CCommandBuffer;
	g_CRCManager = new CCRCManager;
	sv = new CQuakeServer;
	V_Init ();
	Chase_Init ();
	//Host_InitVCR (parms);
	g_Common->COM_Init (parms.basedir);
	Host_InitLocal ();
	W_LoadWadFile (GFX_WAD);
    W_LoadGoldSrcWadFiles();
	LoadAllVPKs();
	if (cls.state != ca_dedicated)
	{
		Key_Init ();
		Con_Init ();	
	}
	M_Init ();	
	PR_Init ();
	Mod_Init ();
	NET_Init ();
	sv->SV_Init();

	Con_Printf("Exe: %s %s\n", __TIME__, __DATE__);
	Con_Printf ("%4.1f megabyte heap\n",parms.memsize/ (1024*1024.0));

	if (cls.state != ca_dedicated)
	{
        host_basepal = COM_LoadHunkFile<byte> ("gfx/palette.lmp", NULL);
		if (!host_basepal)
			Con_Warning ("Couldn't load gfx/palette.lmp");
		host_colormap = COM_LoadHunkFile<byte> ("gfx/colormap.lmp", NULL);
		if (!host_colormap)
            Con_Warning ("Couldn't load gfx/colormap.lmp");

#if (_WIN32) && !(GLQUAKE)
		SDL_setenv("SDL_AudioDriver", "directsound", 1);
		g_SoundSystem = new CSoundSystemWin;
#elif (__linux__) && !(GLQUAKE)
        SDL_setenv("SDL_AudioDriver", "alsa", 1);
        g_SoundSystem = new CSoundDMA;
#endif

		IN_Init ();

		g_CoreRenderer = new CCoreRenderer;		// needed even for dedicated servers
#ifndef GLQUAKE
		g_CoreRenderer->R_Init();
		g_SoftwareRenderer = new CSoftwareRenderer;
		g_SoftwareRenderer->Draw_Init ();
#else
		g_GLRenderer = new CGLRenderer;
		VID_Init (host_basepal);
		g_GLRenderer->Draw_Init();
        g_GLRenderer->R_Init();
#endif
		SCR_Init();
#if (_WIN32) &&	(GLQUAKE)
		SDL_setenv("SDL_AudioDriver", "directsound", 1);
		g_SoundSystem = new CSoundDMA;
		g_SoundSystem->S_Init();
#elif (__linux__) && (GLQUAKE)
        SDL_setenv("SDL_AudioDriver", "pulseaudio", 1);
		g_SoundSystem = new CSoundDMA;
		g_SoundSystem->S_Init();
#endif
#if 0
    // on Win32, sound initialization has to come before video initialization, so we
    // can put up a popup if the sound hardware is in use
        g_SoundSystem->S_Init ();
#endif

		CDAudio_Init ();
		g_pBGM = new CBackgroundMusic;

		Sbar_Init ();
		CL_Init ();
	}

	g_MemCache->Hunk_AllocName<char>(0, "-HOST_HUNKLEVEL-");
	host_hunklevel = g_MemCache->Hunk_LowMark();

	if (cls.state != ca_dedicated)
		g_pCmdBuf->Cbuf_InsertText("exec quake.rc\n");
	else
	{
        g_pCmdBuf->Cbuf_InsertText("exec server.cfg\n");
        g_pCmdBuf->Cbuf_AddText("exec autoexec.cfg\n");
		g_pCmdBuf->Cbuf_AddText("stuffcmds");
        g_pCmdBuf->Cbuf_Execute();
    }

	host_initialized = true;
}


/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void CQuakeHost::Host_Shutdown()
{
	static bool isdown = false;
	
	if (isdown)
	{
		printf ("recursive shutdown\n");
		return;
	}
	isdown = true;

// keep Con_Printf from trying to update the screen
	scr_disabled_for_loading = true;

	Host_WriteConfiguration (); 
	CDAudio_Shutdown ();

	g_pBGM->BGM_Shutdown();
	delete g_pBGM;

	NET_Shutdown ();

	if (g_SoundSystem)
		g_SoundSystem->S_Shutdown();

	delete g_SoundSystem;

	IN_Shutdown ();

	if (cls.state != ca_dedicated)
	{
		VID_Shutdown();
	}

	if (pr_vectors && progs->numvectors > 0)
	{
		for (int i = 0; i < progs->numvectors; i++)
		{
			pr_vectors[i].data->Clear();
		}
	}

	for (int i = 1; i < MAX_LOADED_WADS; i++)
	{
		if (!wad_names[i])
			break;

		delete[] wad_names[i];
	}

	delete this;
}

