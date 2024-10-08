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
// sv_main.c -- server main program

#include "quakedef.h"

CQuakeServer*	sv = nullptr;
server_static_t	svs;

cvar_t		sv_cheats { "sv_cheats", "0", false, CVAR_SERVER | CVAR_NOTIFY };

char	localmodels[MAX_MODELS][5];			// inline model names for precache

//============================================================================

CQuakeServer::CQuakeServer()
{
	active = false;
	datagram = {};
	edicts = nullptr;
	lastcheck = 0;
	lastchecktime = 0.0;
	loadgame = false;
	max_edicts = 0;
	num_edicts = 0;
	paused = false;
	reliable_datagram = {};
	signon = {};
	state = {};
	time = 0.0;

	level_has_fog = false;

	memset(&worldmodel, 0, sizeof(worldmodel));
	memset(&sound_precache, 0, sizeof(sound_precache));
	memset(&signon_buf, 0, sizeof(signon_buf));
	memset(&model_precache, 0, sizeof(model_precache));
	memset(&modelname, 0, sizeof(modelname));
	memset(&models, 0, sizeof(models));
	memset(&name, 0, sizeof(name));
	memset(&reliable_datagram_buf, 0, sizeof(reliable_datagram_buf));
	memset(&lightstyles, 0, sizeof(lightstyles));
	memset(&datagram_buf, 0, sizeof(datagram_buf));
}

CQuakeServer::~CQuakeServer()
{
	active = false;
	datagram = {};
	lastcheck = 0;
	lastchecktime = 0.0;
	loadgame = false;
	max_edicts = 0;
	num_edicts = 0;
	paused = false;
	reliable_datagram = {};
	signon = {};
	state = {};
	time = 0.0;

	level_has_fog = false;

	memset(&worldmodel, 0, sizeof(worldmodel));
	memset(&sound_precache, 0, sizeof(sound_precache));
	memset(&signon_buf, 0, sizeof(signon_buf));
	memset(&model_precache, 0, sizeof(model_precache));
	memset(&modelname, 0, sizeof(modelname));
	memset(&models, 0, sizeof(models));
	memset(&name, 0, sizeof(name));
	memset(&reliable_datagram_buf, 0, sizeof(reliable_datagram_buf));
	memset(&lightstyles, 0, sizeof(lightstyles));
	memset(&datagram_buf, 0, sizeof(datagram_buf));
    free(edicts);
	edicts = nullptr;
}

/*
===============
SV_Init
===============
*/
void CQuakeServer::SV_Init ()
{
	int		i;
	extern	cvar_t	sv_maxvelocity;
	extern	cvar_t	sv_gravity;
	extern	cvar_t	sv_nostep;
	extern	cvar_t	sv_friction;
	extern	cvar_t	sv_edgefriction;
	extern	cvar_t	sv_stopspeed;
	extern	cvar_t	sv_maxspeed;
	extern	cvar_t	sv_accelerate;
	extern	cvar_t	sv_idealpitchscale;
	extern	cvar_t	sv_aim;

	Cvar_RegisterVariable (&sv_maxvelocity);
	Cvar_RegisterVariable (&sv_gravity);
	Cvar_RegisterVariable (&sv_friction);
	Cvar_RegisterVariable (&sv_edgefriction);
	Cvar_RegisterVariable (&sv_stopspeed);
	Cvar_RegisterVariable (&sv_maxspeed);
	Cvar_RegisterVariable (&sv_accelerate);
	Cvar_RegisterVariable (&sv_idealpitchscale);
	Cvar_RegisterVariable (&sv_aim);
    Cvar_RegisterVariable (&sv_nostep);
    Cvar_RegisterVariable (&sv_cheats);

	// Missi: set sv_cheats if developer is enabled (7/26/2024)
    if (Cvar_VariableValue("developer") != 0)
        Cvar_Set("sv_cheats", "1");

	for (i=0 ; i<MAX_MODELS ; i++)
		snprintf (localmodels[i], sizeof(localmodels[i]), "*%i", i);
}

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*  
==================
SV_StartParticle

Make sure the event gets sent to all clients
==================
*/
void CQuakeServer::SV_StartParticle (vec3_t org, vec3_t dir, int color, int count)
{
	int		i, v;

	if (datagram.cursize > MAX_DATAGRAM-16)
		return;	
	MSG_WriteByte (&datagram, svc_particle);
	MSG_WriteCoord (&datagram, org[0]);
	MSG_WriteCoord (&datagram, org[1]);
	MSG_WriteCoord (&datagram, org[2]);
	for (i=0 ; i<3 ; i++)
	{
		v = dir[i]*16;
		if (v > 127)
			v = 127;
		else if (v < -128)
			v = -128;
		MSG_WriteChar (&datagram, v);
	}
	MSG_WriteByte (&datagram, count);
	MSG_WriteByte (&datagram, color);
}           

/*  
==================
SV_StartSound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.  (max 4 attenuation)

==================
*/  
void CQuakeServer::SV_StartSound (edict_t *entity, int channel, const char *sample, int vol,
    float attenuation)
{       
    int         sound_num;
    int field_mask;
    int			i;
	int			ent;
	
	if (vol < 0 || vol > 255)
		Sys_Error ("SV_StartSound: vol = %i", vol);

	if (attenuation < 0 || attenuation > 4)
		Sys_Error ("SV_StartSound: attenuation = %f", attenuation);

	if (channel < 0 || channel > 7)
		Sys_Error ("SV_StartSound: channel = %i", channel);

	if (datagram.cursize > MAX_DATAGRAM-16)
		return;	

// find precache number for sound
    for (sound_num=1 ; sound_num<MAX_SOUNDS
        && sound_precache[sound_num] ; sound_num++)
        if (!strcmp(sample, sound_precache[sound_num]))
            break;
    
    if ( sound_num == MAX_SOUNDS || !sound_precache[sound_num] )
    {
        Con_Printf ("SV_StartSound: %s not precacheed\n", sample);
        return;
    }
    
	ent = NUM_FOR_EDICT(entity);

	channel = (ent<<3) | channel;

	field_mask = 0;
	if (vol != DEFAULT_SOUND_PACKET_VOLUME)
		field_mask |= SND_VOLUME;
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		field_mask |= SND_ATTENUATION;

// directed messages go only to the entity the are targeted on
	MSG_WriteByte (&datagram, svc_sound);
	MSG_WriteByte (&datagram, field_mask);
	if (field_mask & SND_VOLUME)
		MSG_WriteByte (&datagram, vol);
	if (field_mask & SND_ATTENUATION)
		MSG_WriteByte (&datagram, attenuation*64);
	MSG_WriteShort (&datagram, channel);
	MSG_WriteByte (&datagram, sound_num);
	for (i=0 ; i<3 ; i++)
		MSG_WriteCoord (&datagram, entity->v.origin[i] + 0.5 * (entity->v.mins[i] + entity->v.maxs[i]));
}           

/*
==============================================================================

CLIENT SPAWNING

==============================================================================
*/

/*
================
SV_SendServerinfo

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void CQuakeServer::SV_SendServerinfo (client_t *client)
{
	const char			**s;
	char			message[2048];

	MSG_WriteByte (&client->message, svc_print);
#ifndef BQUAKE
	snprintf(message, sizeof(message), "%c\nVERSION %4.2f SERVER (%i CRC)\n", 2, VERSION, pr_crc);
#else
	snprintf(message, sizeof(message), "%c\nBANANAQUAKE VERSION %4.2f SERVER (%i CRC)\n", 2, BANANAQUAKE_VERSION, pr_crc);
#endif
	MSG_WriteString (&client->message,message);

	MSG_WriteByte (&client->message, svc_serverinfo);
	MSG_WriteLong (&client->message, PROTOCOL_VERSION);
	MSG_WriteByte (&client->message, svs.maxclients);

	if (!host->coop.value && host->deathmatch.value)
		MSG_WriteByte (&client->message, GAME_DEATHMATCH);
	else
		MSG_WriteByte (&client->message, GAME_COOP);

	snprintf (message, sizeof(message), PR_GetString(edicts->v.message));

	MSG_WriteString (&client->message,message);

	for (s = model_precache+1 ; *s ; s++)
		MSG_WriteString (&client->message, *s);
	MSG_WriteByte (&client->message, 0);

	for (s = sound_precache+1 ; *s ; s++)
		MSG_WriteString (&client->message, *s);
	MSG_WriteByte (&client->message, 0);

// send music
	MSG_WriteByte (&client->message, svc_cdtrack);
	MSG_WriteByte (&client->message, edicts->v.sounds);
	MSG_WriteByte (&client->message, edicts->v.sounds);

// set view	
	MSG_WriteByte (&client->message, svc_setview);
	MSG_WriteShort (&client->message, NUM_FOR_EDICT(client->edict));

	MSG_WriteByte (&client->message, svc_signonnum);
	MSG_WriteByte (&client->message, 1);

	client->sendsignon = true;
	client->spawned = false;		// need prespawn, spawn, etc
}

static int s_botCount = 0;

/*
================
SV_ConnectClient

Initializes a client_t for a new net connection.  This will only be called
once for a player each game, not once for each level change.
================
*/
void CQuakeServer::SV_ConnectClient (int clientnum, bool bot)
{
	edict_t			*ent;
	client_t		*client;
	int				edictnum;
	struct qsocket_s *netconnection;
	int				i;
	float			spawn_parms[NUM_SPAWN_PARMS];

	client = svs.clients + clientnum;

	Con_DPrintf ("Client %s connected\n", client->netconnection->address);

	edictnum = clientnum+1;

	ent = EDICT_NUM(edictnum);
	
// set up the client_t
	netconnection = client->netconnection;
	
	if (loadgame)
		memcpy (spawn_parms, client->spawn_parms, sizeof(spawn_parms));
	memset (client, 0, sizeof(*client));
	client->netconnection = netconnection;

	Q_strcpy (client->name, "unconnected");
	client->active = true;
	client->spawned = false;
	client->edict = ent;
	client->message.data = client->msgbuf;
	client->message.maxsize = sizeof(client->msgbuf);
	client->message.allowoverflow = true;		// we can catch it

#ifdef IDGODS
	client->privileged = IsID(&client->netconnection->addr);
#else	
	client->privileged = false;				
#endif

	if (loadgame)
		memcpy (client->spawn_parms, spawn_parms, sizeof(spawn_parms));
	else
	{
	// call the progs to get default spawn parms for the new client
		PR_ExecuteProgram (pr_global_struct->SetNewParms);
		for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
			client->spawn_parms[i] = (&pr_global_struct->parm1)[i];
	}

	if (bot)
	{
		char botName[32];
		snprintf(botName, sizeof(botName), "Bot%02d", s_botCount);

		Q_strncpy(client->name, botName, sizeof(client->name));

		client->edict->v.netname = PR_SetEngineString(client->name);

		client->edict->v.flags = (int)client->edict->v.flags | FL_FAKECLIENT;

		pr_global_struct->time = GetServerTime();
		pr_global_struct->self = EDICT_TO_PROG(client->edict);

		PR_ExecuteProgram(pr_global_struct->ClientConnect);
		PR_ExecuteProgram(pr_global_struct->PutClientInServer);

		s_botCount++;
	}

	SV_SendServerinfo (client);
}


/*
===================
SV_CheckForNewClients

===================
*/
void CQuakeServer::SV_CheckForNewClients ()
{
	struct qsocket_s	*ret;
	int				i;
		
//
// check for new connections
//
	while (1)
	{
		ret = NET_CheckNewConnections ();
		if (!ret)
			break;

	// 
	// init a new client structure
	//	
		for (i=0 ; i<svs.maxclients ; i++)
			if (!svs.clients[i].active)
				break;
		if (i == svs.maxclients)
			Sys_Error ("Host_CheckForNewClients: no free clients");
		
		svs.clients[i].netconnection = ret;
		SV_ConnectClient (i);	
	
		net_activeconnections++;
	}
}



/*
===============================================================================

FRAME UPDATES

===============================================================================
*/

/*
==================
SV_ClearDatagram

==================
*/
void CQuakeServer::SV_ClearDatagram ()
{
	SZ_Clear (&datagram);
}

/*
=============================================================================

The PVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.

=============================================================================
*/

int		fatbytes;
byte	fatpvs[MAX_MAP_LEAFS/8];

void CQuakeServer::SV_AddToFatPVS (vec3_t org, mnode_t *node)
{
	int		i;
	byte	*pvs;
	mplane_t	*plane;
	float	d;

	while (1)
	{
	// if this is a leaf, accumulate the pvs bits
		if (node->contents < 0)
		{
			if (node->contents != CONTENTS_SOLID)
			{
				pvs = Mod_LeafPVS ( (mleaf_t *)node, worldmodel);
				for (i=0 ; i<fatbytes ; i++)
					fatpvs[i] |= pvs[i];
			}
			return;
		}
	
		plane = node->plane;
		d = DotProduct (org, plane->normal) - plane->dist;
		if (d > 8)
			node = node->children[0];
		else if (d < -8)
			node = node->children[1];
		else
		{	// go down both
			SV_AddToFatPVS (org, node->children[0]);
			node = node->children[1];
		}
	}
}

/*
=============
SV_FatPVS

Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.
=============
*/
byte* CQuakeServer::SV_FatPVS (vec3_t org)
{
	fatbytes = (worldmodel->numleafs+31)>>3;
	Q_memset (fatpvs, 0, fatbytes);
	SV_AddToFatPVS (org, worldmodel->nodes);
	return fatpvs;
}

//=============================================================================


/*
=============
SV_WriteEntitiesToClient

=============
*/
void CQuakeServer::SV_WriteEntitiesToClient (edict_t	*clent, sizebuf_t *msg)
{
	int		e, i;
	int		bits;
	byte	*pvs;
	vec3_t	org;
	float	miss;
	edict_t	*ent;

// find the client's PVS
	VectorAdd (clent->v.origin, clent->v.view_ofs, org);
	pvs = SV_FatPVS (org);

// send over all entities (excpet the client) that touch the pvs
	ent = NEXT_EDICT(edicts);
	for (e=1 ; e<num_edicts ; e++, ent = NEXT_EDICT(ent))
	{
		// don't send if flagged for NODRAW and there are no lighting effects
		if (ent->v.effects == EF_NODRAW)
			continue;

        // ignore if not touching a PV leaf
		if (ent != clent)	// clent is ALLWAYS sent
		{
            // ignore ents without visible models
            if (!ent->v.modelindex || !PR_GetString(ent->v.model)[0])
				continue;

			for (i=0 ; i < ent->num_leafs ; i++)
				if (pvs[ent->leafnums[i] >> 3] & (1 << (ent->leafnums[i]&7) ))
					break;
				
            if (i == ent->num_leafs && ent->num_leafs < MAX_ENT_LEAFS)
                continue;		// not visible
		}

		if (msg->maxsize - msg->cursize < 16)
		{
			Con_Printf ("packet overflow\n");
			return;
		}

// send an update
		bits = 0;
		
		for (i=0 ; i<3 ; i++)
		{
			miss = ent->v.origin[i] - ent->baseline.origin[i];
			if ( miss < -0.1 || miss > 0.1 )
				bits |= U_ORIGIN1<<i;
		}

		if ( ent->v.angles[0] != ent->baseline.angles[0] )
			bits |= U_ANGLE1;
			
		if ( ent->v.angles[1] != ent->baseline.angles[1] )
			bits |= U_ANGLE2;
			
		if ( ent->v.angles[2] != ent->baseline.angles[2] )
			bits |= U_ANGLE3;
			
		if (ent->v.movetype == MOVETYPE_STEP)
			bits |= U_NOLERP;	// don't mess up the step animation
	
		if (ent->baseline.colormap != ent->v.colormap)
			bits |= U_COLORMAP;
			
		if (ent->baseline.skin != ent->v.skin)
			bits |= U_SKIN;
			
		if (ent->baseline.frame != ent->v.frame)
			bits |= U_FRAME;
		
		if (ent->baseline.effects != ent->v.effects)
			bits |= U_EFFECTS;
		
		if (ent->baseline.modelindex != ent->v.modelindex)
			bits |= U_MODEL;

		if (e >= 256)
			bits |= U_LONGENTITY;
			
		if (bits >= 256)
			bits |= U_MOREBITS;

	//
	// write the message
	//
		MSG_WriteByte (msg, bits | U_SIGNAL);
		
		if (bits & U_MOREBITS)
			MSG_WriteByte (msg, bits>>8);
		if (bits & U_LONGENTITY)
			MSG_WriteShort (msg,e);
		else
			MSG_WriteByte (msg,e);

		if (bits & U_MODEL)
			MSG_WriteByte (msg,	ent->v.modelindex);
		if (bits & U_FRAME)
			MSG_WriteByte (msg, ent->v.frame);
		if (bits & U_COLORMAP)
			MSG_WriteByte (msg, ent->v.colormap);
		if (bits & U_SKIN)
			MSG_WriteByte (msg, ent->v.skin);
		if (bits & U_EFFECTS)
			MSG_WriteByte (msg, ent->v.effects);
		if (bits & U_ORIGIN1)
			MSG_WriteCoord (msg, ent->v.origin[0]);		
		if (bits & U_ANGLE1)
			MSG_WriteAngle(msg, ent->v.angles[0]);
		if (bits & U_ORIGIN2)
			MSG_WriteCoord (msg, ent->v.origin[1]);
		if (bits & U_ANGLE2)
			MSG_WriteAngle(msg, ent->v.angles[1]);
		if (bits & U_ORIGIN3)
			MSG_WriteCoord (msg, ent->v.origin[2]);
		if (bits & U_ANGLE3)
			MSG_WriteAngle(msg, ent->v.angles[2]);
	}
}

/*
=============
SV_CleanupEnts

=============
*/
void CQuakeServer::SV_CleanupEnts ()
{
	int		e;
	edict_t	*ent;
	
	ent = NEXT_EDICT(edicts);
	for (e=1 ; e<num_edicts ; e++, ent = NEXT_EDICT(ent))
	{
		ent->v.effects = (int)ent->v.effects & ~EF_MUZZLEFLASH;
	}

}

/*
==================
SV_WriteClientdataToMessage

==================
*/
void CQuakeServer::SV_WriteClientdataToMessage (edict_t *ent, sizebuf_t *msg)
{
	int		bits;
	int		i;
	edict_t	*other;
	long long	items;
#ifndef QUAKE2
	eval_t	*val;
#endif

	extern bool standard_quake;

//
// send a damage message
//
	if (ent->v.dmg_take || ent->v.dmg_save)
	{
		other = PROG_TO_EDICT(ent->v.dmg_inflictor);
		MSG_WriteByte (msg, svc_damage);
		MSG_WriteByte (msg, ent->v.dmg_save);
		MSG_WriteByte (msg, ent->v.dmg_take);
		for (i=0 ; i<3 ; i++)
			MSG_WriteCoord (msg, other->v.origin[i] + 0.5f*(other->v.mins[i] + other->v.maxs[i]));
	
		ent->v.dmg_take = 0;
		ent->v.dmg_save = 0;
	}

//
// send the current viewpos offset from the view entity
//
	SV_SetIdealPitch ();		// how much to look up / down ideally

// a fixangle might get lost in a dropped packet.  Oh well.
	if ( ent->v.fixangle )
	{
		MSG_WriteByte (msg, svc_setangle);
		for (i=0 ; i < 3 ; i++)
			MSG_WriteAngle (msg, ent->v.angles[i] );
		ent->v.fixangle = 0;
	}

	bits = 0;
	
	if (ent->v.view_ofs[2] != DEFAULT_VIEWHEIGHT)
		bits |= SU_VIEWHEIGHT;
		
	if (ent->v.idealpitch)
		bits |= SU_IDEALPITCH;

// stuff the sigil bits into the high bits of items for sbar, or else
// mix in items2
#ifdef QUAKE2
	items = (long long)ent->v.items | ((long long)ent->v.items2 << 23);
#else
	val = GetEdictFieldValue(ent, "items2");

	if (val)
		items = (long long)ent->v.items | ((long long)val->_float << 23);
	else
		items = (long long)ent->v.items | ((long long)pr_global_struct->serverflags << 28);
#endif

	bits |= SU_ITEMS;
	
	if ( (int)ent->v.flags & FL_ONGROUND)
		bits |= SU_ONGROUND;
	
	if ( ent->v.waterlevel >= 2)
		bits |= SU_INWATER;
	
	for (i=0 ; i<3 ; i++)
	{
		if (ent->v.punchangle[i])
			bits |= (SU_PUNCH1<<i);
		if (ent->v.velocity[i])
			bits |= (SU_VELOCITY1<<i);
	}
	
	if (ent->v.weaponframe)
		bits |= SU_WEAPONFRAME;

	if (ent->v.armorvalue)
		bits |= SU_ARMOR;

//	if (ent->v.weapon)
		bits |= SU_WEAPON;

// send the data

	MSG_WriteByte (msg, svc_clientdata);
	MSG_WriteShort (msg, bits);

	if (bits & SU_VIEWHEIGHT)
		MSG_WriteChar (msg, ent->v.view_ofs[2]);

	if (bits & SU_IDEALPITCH)
		MSG_WriteChar (msg, ent->v.idealpitch);

	for (i=0 ; i<3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i))
			MSG_WriteChar (msg, ent->v.punchangle[i]);
		if (bits & (SU_VELOCITY1<<i))
			MSG_WriteChar (msg, ent->v.velocity[i]/16);
	}

// [always sent]	if (bits & SU_ITEMS)
	MSG_WriteLong (msg, items);

	if (bits & SU_WEAPONFRAME)
		MSG_WriteByte (msg, ent->v.weaponframe);
	if (bits & SU_ARMOR)
		MSG_WriteByte (msg, ent->v.armorvalue);
	if (bits & SU_WEAPON)
		MSG_WriteByte (msg, SV_ModelIndex(PR_GetString(ent->v.weaponmodel)));
	
	MSG_WriteShort (msg, ent->v.health);
	MSG_WriteByte (msg, ent->v.currentammo);
	MSG_WriteByte (msg, ent->v.ammo_shells);
	MSG_WriteByte (msg, ent->v.ammo_nails);
	MSG_WriteByte (msg, ent->v.ammo_rockets);
	MSG_WriteByte (msg, ent->v.ammo_cells);

	if (standard_quake)
	{
		MSG_WriteByte (msg, ent->v.weapon);
	}
	else
	{
		for(i=0;i<32;i++)
		{
			if ( ((int)ent->v.weapon) & (1<<i) )
			{
				MSG_WriteByte (msg, i);
				break;
			}
		}
	}
}

/*
=======================
SV_SendClientDatagram
=======================
*/
bool CQuakeServer::SV_SendClientDatagram (client_t *client)
{
	static byte		buf[MAX_DATAGRAM];
	sizebuf_t	msg;
	
	msg.data = buf;
	msg.maxsize = sizeof(buf);
	msg.cursize = 0;

	MSG_WriteByte (&msg, svc_time);
	MSG_WriteFloat (&msg, time);

// add the client specific data to the datagram
	SV_WriteClientdataToMessage (client->edict, &msg);

	SV_WriteEntitiesToClient (client->edict, &msg);

// copy the server datagram if there is space
	if (msg.cursize + datagram.cursize < msg.maxsize)
		SZ_Write (&msg, datagram.data, datagram.cursize);

// send the datagram
	if (NET_SendUnreliableMessage (client->netconnection, &msg) == -1)
	{
		SV_DropClient (true);// if the message couldn't send, kick off
		return false;
	}
	
	return true;
}

/*
=======================
SV_UpdateToReliableMessages
=======================
*/
void CQuakeServer::SV_UpdateToReliableMessages ()
{
	int			i, j;
	client_t *client;

// check for changes to be sent over the reliable streams
	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		if (host_client->old_frags != host_client->edict->v.frags)
		{
			for (j=0, client = svs.clients ; j<svs.maxclients ; j++, client++)
			{
				if (!client->active)
					continue;
				MSG_WriteByte (&client->message, svc_updatefrags);
				MSG_WriteByte (&client->message, i);
				MSG_WriteShort (&client->message, host_client->edict->v.frags);
			}

			host_client->old_frags = host_client->edict->v.frags;
		}
	}
	
	for (j=0, client = svs.clients ; j<svs.maxclients ; j++, client++)
	{
		if (!client->active)
			continue;
		SZ_Write (&client->message, reliable_datagram.data, reliable_datagram.cursize);
	}

	SZ_Clear (&reliable_datagram);
}


/*
=======================
SV_SendNop

Send a nop message without trashing or sending the accumulated client
message buffer
=======================
*/
void CQuakeServer::SV_SendNop (client_t *client)
{
	sizebuf_t	msg;
	byte		buf[4];
	
	msg.data = buf;
	msg.maxsize = sizeof(buf);
	msg.cursize = 0;

	MSG_WriteChar (&msg, svc_nop);

	if (NET_SendUnreliableMessage (client->netconnection, &msg) == -1)
		SV_DropClient (true);	// if the message couldn't send, kick off
	client->last_message = host->realtime;
}

/*
=======================
SV_SendClientMessages
=======================
*/
void CQuakeServer::SV_SendClientMessages ()
{
	int			i;
	
// update frags, names, etc
	SV_UpdateToReliableMessages ();

// build individual updates
	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		if (!host_client->active)
			continue;

		if (host_client->spawned)
		{
			if (!SV_SendClientDatagram (host_client))
				continue;
		}
		else
		{
		// the player isn't totally in the game yet
		// send small keepalive messages if too much time has passed
		// send a full message when the next signon stage has been requested
		// some other message data (name changes, etc) may accumulate 
		// between signon stages
			if (!host_client->sendsignon)
			{
				if (host->realtime - host_client->last_message > 5)
					SV_SendNop (host_client);
				continue;	// don't send out non-signon messages
			}
		}

		// check for an overflowed message.  Should only happen
		// on a very fucked up connection that backs up a lot, then
		// changes level
		if (host_client->message.overflowed)
		{
			SV_DropClient (true);
			host_client->message.overflowed = false;
			continue;
		}
			
		if (host_client->message.cursize || host_client->dropasap)
		{
			if (!NET_CanSendMessage (host_client->netconnection))
			{
//				I_Printf ("can't write\n");
				continue;
			}

			if (host_client->dropasap)
				SV_DropClient (false);	// went to another level
			else
			{
				if (NET_SendMessage (host_client->netconnection
				, &host_client->message) == -1)
					SV_DropClient (true);	// if the message couldn't send, kick off
				SZ_Clear (&host_client->message);
				host_client->last_message = host->realtime;
				host_client->sendsignon = false;
			}
		}
	}
	
	
// clear muzzle flashes
	SV_CleanupEnts ();
}


/*
==============================================================================

SERVER SPAWNING

==============================================================================
*/

/*
================
SV_ModelIndex

================
*/
int CQuakeServer::SV_ModelIndex (const char *modname)
{
	int		i;
	
	if (!modname || !modname[0])
		return 0;

	for (i=0 ; i<MAX_MODELS && model_precache[i] ; i++)
		if (!strcmp(model_precache[i], modname))
			return i;
	if (i==MAX_MODELS || !model_precache[i])
		Sys_Error ("SV_ModelIndex: model %s not precached", modname);
	return i;
}

/*
================
SV_CreateBaseline

================
*/
void CQuakeServer::SV_CreateBaseline ()
{
	int			i;
	edict_t			*svent;
	int				entnum;	
		
	for (entnum = 0; entnum < num_edicts ; entnum++)
	{
	// get the current server version
		svent = EDICT_NUM(entnum);
		if (svent->free)
			continue;
		if (entnum > svs.maxclients && !svent->v.modelindex)
			continue;

	//
	// create entity baseline
	//
		VectorCopy (svent->v.origin, svent->baseline.origin);
		VectorCopy (svent->v.angles, svent->baseline.angles);
		svent->baseline.frame = svent->v.frame;
		svent->baseline.skin = svent->v.skin;
		if (entnum > 0 && entnum <= svs.maxclients)
		{
			svent->baseline.colormap = entnum;
			svent->baseline.modelindex = SV_ModelIndex("progs/player.mdl");
		}
		else
		{
			svent->baseline.colormap = 0;
			svent->baseline.modelindex =
				SV_ModelIndex(PR_GetString(svent->v.model));
		}
		
	//
	// add to the message
	//
		MSG_WriteByte (&signon,svc_spawnbaseline);		
		MSG_WriteShort (&signon,entnum);

		MSG_WriteByte (&signon, svent->baseline.modelindex);
		MSG_WriteByte (&signon, svent->baseline.frame);
		MSG_WriteByte (&signon, svent->baseline.colormap);
		MSG_WriteByte (&signon, svent->baseline.skin);
		for (i=0 ; i<3 ; i++)
		{
			MSG_WriteCoord(&signon, svent->baseline.origin[i]);
			MSG_WriteAngle(&signon, svent->baseline.angles[i]);
		}
	}
}


/*
================
SV_SendReconnect

Tell all the clients that the server is changing levels
================
*/
void CQuakeServer::SV_SendReconnect ()
{
	char	data[128];
	sizebuf_t	msg;

	msg.data = reinterpret_cast<byte*>(data);
	msg.cursize = 0;
	msg.maxsize = sizeof(data);

	MSG_WriteChar (&msg, svc_stufftext);
	MSG_WriteString (&msg, "reconnect\n");
	NET_SendToAll (&msg, 5);
	
	if (cls.state != ca_dedicated)
#ifdef QUAKE2
		Cbuf_InsertText ("reconnect\n");
#else
		g_pCmds->Cmd_ExecuteString ("reconnect\n", src_command);
#endif
}


/*
================
SV_SaveSpawnparms

Grabs the current state of each client for saving across the
transition to another level
================
*/
void CQuakeServer::SV_SaveSpawnparms ()
{
	int		i, j;

	svs.serverflags = pr_global_struct->serverflags;

	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		if (!host_client->active)
			continue;

	// call the progs to get default spawn parms for the new client
		pr_global_struct->self = EDICT_TO_PROG(host_client->edict);
		PR_ExecuteProgram (pr_global_struct->SetChangeParms);
		for (j=0 ; j<NUM_SPAWN_PARMS ; j++)
			host_client->spawn_parms[j] = (&pr_global_struct->parm1)[j];
	}
}


/*
================
SV_SpawnServer

This is called at the start of each level
================
*/
extern float		scr_centertime_off;

#ifdef QUAKE2
void CQuakeServer::SV_SpawnServer (char *server, char *startspot)
#else
void CQuakeServer::SV_SpawnServer (char *server)
#endif
{
	edict_t		*ent;
	int			i;
	static char	dummy[8] = { 0,0,0,0,0,0,0,0 };

	// Missi: exec 'listenserver.cfg' here as we need to set the vars after the host is created (6/9/2024)
	if (svs.maxclients > 1 && cls.state != ca_dedicated)
		g_pCmdBuf->Cbuf_InsertText("exec listenserver.cfg\n");

	// Missi: set coop and deathmatch now so we don't get any mismatches with what server hosts have set (6/9/2024)
	host->coop.value = Cvar_VariableValue("coop");
	host->deathmatch.value = Cvar_VariableValue("deathmatch");

	// let's not have any servers with no name
	if (hostname.string[0] == 0)
		Cvar_Set ("hostname", "UNNAMED");
	scr_centertime_off = 0;

	Con_DPrintf ("SpawnServer: %s\n",server);
	svs.changelevel_issued = false;		// now safe to issue another

//
// tell all connected clients that we are going to a new level
//
	if (active)
	{
		SV_SendReconnect ();
	}

//
// make cvars consistant
//
	if (host->coop.value)
		Cvar_SetValue ("deathmatch", 0);
	host->current_skill = (int)(host->skill.value + 0.5);
	if (host->current_skill < 0)
		host->current_skill = 0;
	if (host->current_skill > 3)
		host->current_skill = 3;

	Cvar_SetValue ("skill", (float)host->current_skill);
	
	if (pr_vectors && progs->numvectors > 0)
	{
		for (i = 0; i < progs->numvectors; i++)
		{
			pr_vectors[i].data->Clear();
		}
	}

	//
	// set up the new server
	//

	// Missi: should be cleared in CQuakeHost::Host_ClearMemory, but
	// clearing it here is faster and costs one less ASM instruction.
	// This also prevents a memory leak (9/4/2024)
	free(edicts);

	host->Host_ClearMemory ();

	Q_strlcpy(name, server, sizeof(name));
#ifdef QUAKE2
	if (startspot)
		Q_strcpy(startspot, startspot);
#endif

// load progs to get entity field count
	PR_LoadProgs ();

// allocate server memory
	max_edicts = MAX_EDICTS;

	edicts = (edict_t*)malloc(max_edicts * pr_edict_size);

	datagram.maxsize = sizeof(datagram_buf);
	datagram.cursize = 0;
	datagram.data = datagram_buf;
	
	reliable_datagram.maxsize = sizeof(reliable_datagram_buf);
	reliable_datagram.cursize = 0;
	reliable_datagram.data = reliable_datagram_buf;
	
	signon.maxsize = sizeof(signon_buf);
	signon.cursize = 0;
	signon.data = signon_buf;
	
// leave slots at start for clients only
	num_edicts = svs.maxclients+1;
	for (i=0 ; i<svs.maxclients ; i++)
	{
		ent = EDICT_NUM(i+1);
		svs.clients[i].edict = ent;
	}
	
	state = ss_loading;
	paused = false;

	time = 1.0;
	
	Q_strlcpy (name, server, sizeof(name));
	snprintf(modelname, sizeof(modelname), "maps/%s.bsp", server);
	worldmodel = Mod_ForName (modelname, false);
	if (!worldmodel)
	{
		Con_Printf ("Couldn't spawn server %s\n", modelname);
		active = false;
		return;
	}
	models[1] = worldmodel;
	
//
// clear world interaction links
//
	SV_ClearWorld ();
	
	sound_precache[0] = dummy;
	model_precache[0] = dummy;
	model_precache[1] = modelname;
	for (i = 1; i < worldmodel->numsubmodels; i++)
	{
		model_precache[1+i] = localmodels[i];
		models[i+1] = Mod_ForName (localmodels[i], false);
	}

//
// load the rest of the entities
//	
	ent = EDICT_NUM(0);
	memset (&ent->v, 0, sizeof(progs->entityfields * 4));
	ent->free = false;
	ent->v.model = PR_SetEngineString(worldmodel->name);
	ent->v.modelindex = 1;		// world model
	ent->v.solid = SOLID_BSP;
	ent->v.movetype = MOVETYPE_PUSH;

	if (host->coop.value)
		pr_global_struct->coop = host->coop.value;
	else
		pr_global_struct->deathmatch = host->deathmatch.value;

	pr_global_struct->mapname = PR_SetEngineString(name);
#ifdef QUAKE2
	pr_global_struct->startspot = startspot - pr_strings;
#endif

// serverflags are for cross level information (sigils)
	pr_global_struct->serverflags = svs.serverflags;

	ED_LoadFromFile (worldmodel->entities);

	// Missi: set up fog
	const char* lump = worldmodel->entities;
	int pos = 0;

	level_has_fog = false;
#ifdef GLQUAKE

    const char* parse = lump;
	edict_t* parse2 = nullptr;
	bool fog_in_progs = false;

    eval_t* fog_eval = GetEdictFieldValue(ent, "fog");
    eval_t* fog_c_eval = GetEdictFieldValue(ent, "fog_colour");
    eval_t* fog_density_eval = GetEdictFieldValue(ent, "fog_density");
    eval_t* fog_lerp_time_eval = GetEdictFieldValue(ent, "fog_lerp_time");
    eval_t* fog_start_eval = GetEdictFieldValue(ent, "fog_start");
    eval_t* fog_end_eval = GetEdictFieldValue(ent, "fog_end");

	if (!fog_eval)
	{
		for (const char* parsed = g_Common->COM_ParseStringNewline(parse); parsed[0] != '}'; parsed = g_Common->COM_ParseStringNewline(parsed))
		{
			if (!Q_strncmp(parsed, "\"fog\"", 5))
			{
				parsed = g_Common->COM_ParseStringNewline(parsed);
				Q_snscanf(parsed, sizeof(parsed), "\"%f %f %f %f\"", &fog_color_vec[0], &fog_color_vec[1], &fog_color_vec[2], &fog_color_vec[3]);
				fog_in_progs = true;
			}
			if (!Q_strncmp(parsed, "\"fog_start\"", 11))
			{
				parsed = g_Common->COM_ParseStringNewline(parsed);
				Q_snscanf(parsed, sizeof(parsed), "\"%f\"", &s_dCurFogStart);
				fog_in_progs = true;
			}
			if (!Q_strncmp(parsed, "\"fog_end\"", 10))
			{
				parsed = g_Common->COM_ParseStringNewline(parsed);
				Q_snscanf(parsed, sizeof(parsed), "\"%f\"", &s_dCurFogEnd);
				fog_in_progs = true;
			}
		}
	}

	if (!fog_in_progs)
	{
		if (fog_eval)
		{
			const char* fog_value = PR_UglyValueString(ev_string, fog_eval);
			if (fog_value[0])
			{
				Con_DPrintf("Found fog value: %s\n", fog_value);

				Q_snscanf(fog_value, sizeof(fog_value), "%f %f %f %f", &fog_color_vec[0], &fog_color_vec[1], &fog_color_vec[2], &fog_color_vec[3]);

				Cvar_SetValue("fog_r", fog_color_vec[0]);
				Cvar_SetValue("fog_g", fog_color_vec[1]);
				Cvar_SetValue("fog_b", fog_color_vec[2]);
				Cvar_SetValue("fog_density", fog_color_vec[3]);

				s_dWorldFogColor[0] = fog_color_vec[0];
				s_dWorldFogColor[1] = fog_color_vec[1];
				s_dWorldFogColor[2] = fog_color_vec[2];

				level_has_fog = true;
			}
		}
		if (fog_c_eval)
		{
			const char* fog_value = PR_UglyValueString(ev_string, fog_c_eval);
			if (fog_value[0])
			{
				Con_DPrintf("Found fog value: %s\n", fog_value);

				Q_snscanf(fog_value, sizeof(fog_value), "%f %f %f", &fog_color_vec[0], &fog_color_vec[1], &fog_color_vec[2]);

				Cvar_SetValue("fog_r", fog_color_vec[0]);
				Cvar_SetValue("fog_g", fog_color_vec[1]);
				Cvar_SetValue("fog_b", fog_color_vec[2]);

				s_dWorldFogColor[0] = fog_color_vec[0];
				s_dWorldFogColor[1] = fog_color_vec[1];
				s_dWorldFogColor[2] = fog_color_vec[2];

				level_has_fog = true;
			}
		}
		if (fog_density_eval)
		{
			const char* fog_density_value = PR_UglyValueString(ev_string, fog_density_eval);
			if (fog_density_value[0])
			{
				Con_DPrintf("Found fog_density value: %s\n", fog_density_value);

				float fog_density = 0.0f;

				Q_snscanf(fog_density_value, sizeof(fog_density_value), "%f", &fog_density);

				Cvar_SetValue("fog_density", fog_density);
				fog_color_vec[3] = fog_density;
			}
		}
		if (fog_lerp_time_eval)
		{
			const char* fog_lerp_time_value = PR_UglyValueString(ev_string, fog_lerp_time_eval);
			if (fog_lerp_time_value[0])
			{
				Con_DPrintf("Found fog_lerp_time value: %s\n", fog_lerp_time_value);

				float fog_lerp_time = 0.0f;

				Q_snscanf(fog_lerp_time_value, sizeof(fog_lerp_time_value), "%f", &fog_lerp_time);

				Cvar_SetValue("fog_lerp_time", fog_lerp_time);
				s_dFogLerpTime = Sys_DoubleTime() + fog_lerp_time;
			}
		}
		if (fog_start_eval)
		{
			const char* fog_start_value = PR_UglyValueString(ev_string, fog_start_eval);

			if (fog_start_value[0])
			{
				Con_DPrintf("Found fog_start value: %s\n", fog_start_value);

				float fog_start = 0.0f;

				Q_snscanf(fog_start_value, sizeof(fog_start_value), "%f", &fog_start);

				Cvar_SetValue("fog_start", fog_start);
				s_dCurFogStart = fog_start;
			}
		}
		if (fog_end_eval)
		{
			const char* fog_end_value = PR_UglyValueString(ev_string, fog_end_eval);
			if (fog_end_value[0])
			{
				Con_DPrintf("Found fog_end value: %s\n", fog_end_value);

				float fog_end = 0.0f;

				Q_snscanf(fog_end_value, sizeof(fog_end_value), "%f", &fog_end);

				Cvar_SetValue("fog_end", fog_end);
				s_dCurFogEnd = fog_end;
			}
		}
	}

#endif
	active = true;

// all setup is completed, any further precache statements are errors
	state = ss_active;
	
// run two frames to allow everything to settle
	host->host_frametime = 0.1;
	SV_Physics ();
	SV_Physics ();

// create a baseline for more efficient communications
	SV_CreateBaseline ();

// send serverinfo to all connected clients
	for (i=0,host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
		if (host_client->active)
			SV_SendServerinfo (host_client);

    for (int j = 0; j < num_edicts; j++)
    {
        edict_t* ed = EDICT_NUM(j);
        const char* classname = PR_GetString(ed->v.classname);

        if (!Q_strncmp(classname, "func_fog_volume", 15))
        {
            Con_Printf("Level has Quake 3-styled fog volumes\n");
            break;
        }
    }

	Con_DPrintf ("Server spawned.\n");
}

