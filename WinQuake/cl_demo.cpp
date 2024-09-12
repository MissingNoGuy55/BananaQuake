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

void CL_FinishTimeDemo ();

/*
==============================================================================

DEMO CODE

When a demo is playing back, all NET_SendMessages are skipped, and
NET_GetMessages are read from the demo file.

Whenever cl.time gets past the last received message, another message is
read from the demo file.
==============================================================================
*/

/*
==============
CL_StopPlayback

Called when a demo file runs out, or the user starts a game
==============
*/
void CL_StopPlayback ()
{
	if (!cls.demoplayback)
		return;

	cls.demofile_in.close();
	cls.demoplayback = false;
	cls.state = ca_disconnected;

	if (cls.timedemo)
		CL_FinishTimeDemo ();
}

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length and view angles
====================
*/
void CL_WriteDemoMessage ()
{
	int		len;
	int		i;
	float	f;

	len = LittleLong (net_message.cursize);
	
	//fwrite (&len, 4, 1, cls.demofile);

	char length[16];
	snprintf(length, sizeof(length), "%i", len);

	cls.demofile_out.write(length, sizeof(int));
	
	for (i=0 ; i<3 ; i++)
	{
		f = LittleFloat (cl.viewangles[i]);

		char viewang[16];
		snprintf(viewang, sizeof(viewang), "%f", f);

		cls.demofile_out.write(viewang, sizeof(float));

		//fwrite (&f, 4, 1, cls.demofile);
	}

	cls.demofile_out.write((char*)net_message.data, net_message.cursize);
	cls.demofile_out.flush();

	//fwrite (net_message.data, net_message.cursize, 1, cls.demofile);
	//fflush (cls.demofile);
}

/*
====================
CL_GetMessage

Handles recording and playback of demos, on top of NET_ code
====================
*/
int CL_GetMessage ()
{
	int		r, i;
	float	f;

	if	(cls.demoplayback)
	{
	// decide if it is time to grab the next message		
		if (cls.signon == SIGNONS)	// allways grab until fully connected
		{
			if (cls.timedemo)
			{
				if (host->host_framecount == cls.td_lastframe)
					return 0;		// allready read this frame's message
				cls.td_lastframe = host->host_framecount;
			// if this is the second frame, grab the real td_starttime
			// so the bogus time on the first frame doesn't count
				if (host->host_framecount == cls.td_startframe + 1)
					cls.td_starttime = host->realtime;
			}
			else if ( /* cl.time > 0 && */ cl.time <= cl.mtime[0])
			{
					return 0;		// don't need another message yet
			}
		}
		
	// get the next message
		//fread (&net_message.cursize, 4, 1, cls.demofile);
		
		char data[16];

		cls.demofile_in.read(data, sizeof(int));

		net_message.cursize = atoi(data);

		VectorCopy (cl.mviewangles[0], cl.mviewangles[1]);
		for (i=0 ; i<3 ; i++)
		{
			//r = fread (&f, 4, 1, cls.demofile);
			
			char read[16];

			cls.demofile_in.read(read, sizeof(int));
			
			f = atof(read);

			cl.mviewangles[0][i] = LittleFloat (f);
		}

		//fread(net_message.data, size, 1, cls.demofile);

		net_message.cursize = LittleLong(net_message.cursize);
		if (net_message.cursize > MAX_MSGLEN)
			Sys_Error("Demo message > MAX_MSGLEN");

		cls.demofile_in.read((char*)net_message.data, net_message.cursize);
		if (cls.demofile_in.bad())
		{
			CL_StopPlayback();
			return 0;
		}
	
		return 1;
	}

	while (1)
	{
		r = NET_GetMessage (cls.netcon);
		
		if (r != 1 && r != 2)
			return r;
	
	// discard nop keepalive message
		if (net_message.cursize == 1 && net_message.data[0] == svc_nop)
			Con_Printf ("<-- server to client keepalive\n");
		else
			break;
	}

	if (cls.demorecording)
		CL_WriteDemoMessage ();
	
	return r;
}


/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f ()
{
	if (cmd_source != src_command)
		return;

	if (!cls.demorecording)
	{
		Con_Printf ("Not recording a demo.\n");
		return;
	}

// write a disconnect message to the demo file
	SZ_Clear (&net_message);
	MSG_WriteByte (&net_message, svc_disconnect);
	CL_WriteDemoMessage ();

// finish up
	cls.demofile_out.close();
	cls.demorecording = false;
	Con_Printf ("Completed demo\n");
}

/*
====================
CL_Record_f

record <demoname> <map> [cd track]
====================
*/
void CL_Record_f ()
{
	int		c;
	char	name[MAX_OSPATH];
	int		track;

	if (cmd_source != src_command)
		return;

	c = g_pCmds->Cmd_Argc();
	if (c != 2 && c != 3 && c != 4)
	{
		Con_Printf ("record <demoname> [<map> [cd track]]\n");
		return;
	}

	if (strstr(g_pCmds->Cmd_Argv(1), ".."))
	{
		Con_Printf ("Relative pathnames are not allowed.\n");
		return;
	}

	if (c == 2 && cls.state == ca_connected)
	{
		Con_Printf("Can not record - already connected to server\nClient demo recording must be started before connecting\n");
		return;
	}

// write the forced cd track number, or -1
	if (c == 4)
	{
		track = atoi(g_pCmds->Cmd_Argv(3));
		Con_Printf ("Forcing CD track to %i\n", cls.forcetrack);
	}
	else
		track = -1;	

	snprintf (name, sizeof(name), "%s/%s", g_Common->com_gamedir, g_pCmds->Cmd_Argv(1));
	
//
// start the map up
//
	if (c > 2)
    {
        char buf[256];
        memset(buf, 0, sizeof(buf));
        snprintf(buf, sizeof(buf), "map %s", g_pCmds->Cmd_Argv(2));
		g_pCmds->Cmd_ExecuteString(buf, src_command);
	//	Cmd_ExecuteString (g_Common->va_unsafe("map %s", Cmd_Argv(2)), src_command);
    }
//
// open the demo file
//
	g_Common->COM_DefaultExtension (name, ".dem");

	Con_Printf ("recording to %s.\n", name);
	g_Common->COM_FOpenFile_OFStream(name, &cls.demofile_out, nullptr);
	if (cls.demofile_out.bad())
	{
		Con_Printf ("ERROR: couldn't open.\n");
		return;
	}

	cls.forcetrack = track;

	char write[64];

	snprintf(write, sizeof(write), "%i\n", cls.forcetrack);

	cls.demofile_out.write(write, sizeof(int));
	
	cls.demorecording = true;
}


/*
====================
CL_PlayDemo_f

play [demoname]
====================
*/
void CL_PlayDemo_f ()
{
	char	name[256];

	if (cmd_source != src_command)
		return;

	if (g_pCmds->Cmd_Argc() != 2)
	{
		Con_Printf ("play <demoname> : plays a demo\n");
		return;
	}

//
// disconnect from server
//
	CL_Disconnect ();
	
//
// open the demo file
//
	snprintf(name, sizeof(name), "%s", g_pCmds->Cmd_Argv(1));
	g_Common->COM_DefaultExtension (name, ".dem");

	Con_Printf ("Playing demo from %s.\n", name);

	int size = g_Common->COM_FOpenFile_IFStream (name, &cls.demofile_in, nullptr);
	if (cls.demofile_in.bad())
	{
		Con_Printf ("ERROR: couldn't open.\n");
		cls.demonum = -1;		// stop demo loop
		return;
	}

	char test[16];
	cls.demofile_in.read(test, sizeof(int));

	int val = atoi(test);

    cls.forcetrack = val;

    if (val == -1 || test[1] != '\n')
		//fscanf(cls.demofile, "%i", &cls.forcetrack) != 1 || fgetc(cls.demofile) != '\n')
	{
		cls.demofile_in.close();
		cls.demonum = -1;	// stop demo loop
		Con_Printf("ERROR: demo \"%s\" is invalid\n", name);
		return;
	}

	cls.demoplayback = true;
	cls.state = ca_connected;
	key_dest = key_game;

// ZOID, fscanf is evil
//	fscanf (cls.demofile, "%i\n", &cls.forcetrack);
}

/*
====================
CL_FinishTimeDemo

====================
*/
void CL_FinishTimeDemo ()
{
	int		frames;
	float	time;
	
	cls.timedemo = false;
	
// the first frame didn't count
	frames = (host->host_framecount - cls.td_startframe) - 1;
	time = host->realtime - cls.td_starttime;
	if (!time)
		time = 1;
	Con_Printf ("%i frames %5.1f seconds %5.1f fps\n", frames, time, frames/time);
}

/*
====================
CL_TimeDemo_f

timedemo [demoname]
====================
*/
void CL_TimeDemo_f ()
{
	if (cmd_source != src_command)
		return;

	if (g_pCmds->Cmd_Argc() != 2)
	{
		Con_Printf ("timedemo <demoname> : gets demo speeds\n");
		return;
	}

	CL_PlayDemo_f ();
	
// cls.td_starttime will be grabbed at the second frame of the demo, so
// all the loading time doesn't get counted
	
	cls.timedemo = true;
	cls.td_startframe = host->host_framecount;
	cls.td_lastframe = -1;		// get a new message this frame
}
