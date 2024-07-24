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

#include "quakedef.h"

#define	RETURN_EDICT(e) (((int *)pr_globals)[OFS_RETURN] = EDICT_TO_PROG(e))

/*
===============================================================================

						BUILT-IN FUNCTIONS

===============================================================================
*/

char *PF_VarString (int	first)
{
	int		i;
	static char out[256];
	
	out[0] = 0;
	for (i=first ; i<pr_argc ; i++)
	{
		Q_strcat (out, G_STRING((OFS_PARM0+i*3)));
	}
	return out;
}


/*
=================
PF_errror

This is a TERMINAL error, which will kill off the entire server.
Dumps self.

error(value)
=================
*/
void PF_error (void)
{
	char	*s;
	edict_t	*ed;
	
	s = PF_VarString(0);
	Con_Printf ("======SERVER ERROR in %s:\n%s\n"
	,PR_GetString(pr_xfunction->s_name),s);
	ed = PROG_TO_EDICT(pr_global_struct->self);
	ED_Print (ed);

	host->Host_Error ("Program error");
}

/*
=================
PF_objerror

Dumps out self, then an error message.  The program is aborted and self is
removed, but the level can continue.

objerror(value)
=================
*/
void PF_objerror (void)
{
	char	*s;
	edict_t	*ed;
	
	s = PF_VarString(0);
	Con_Printf ("======OBJECT ERROR in %s:\n%s\n"
	, PR_GetString(pr_xfunction->s_name),s);
	ed = PROG_TO_EDICT(pr_global_struct->self);
	ED_Print (ed);
	ED_Free (ed);
	
	host->Host_Error ("Program error");
}



/*
==============
PF_makevectors

Writes new values for v_forward, v_up, and v_right based on angles
makevectors(vector)
==============
*/
void PF_makevectors (void)
{
	AngleVectors (G_VECTOR(OFS_PARM0), pr_global_struct->v_forward, pr_global_struct->v_right, pr_global_struct->v_up);
}

/*
=================
PF_setorigin

This is the only valid way to move an object without using the physics of the world (setting velocity and waiting).  Directly changing origin will not set internal links correctly, so clipping would be messed up.  This should be called when an object is spawned, and then only if it is teleported.

setorigin (entity, origin)
=================
*/
void PF_setorigin (void)
{
	edict_t	*e;
	float	*org;
	
	e = G_EDICT(OFS_PARM0);
	org = G_VECTOR(OFS_PARM1);
	VectorCopy (org, e->v.origin);
	sv->SV_LinkEdict (e, false);
}


void SetMinMaxSize (edict_t *e, float *min, float *max, bool rotate)
{
	float	*angles;
	vec3_t	rmin, rmax;
	float	bounds[2][3];
	float	xvector[2], yvector[2];
	float	a;
	vec3_t	base, transformed;
	int		i, j, k, l;
	
	for (i=0 ; i<3 ; i++)
		if (min[i] > max[i])
			PR_RunError ("backwards mins/maxs");

	rotate = false;		// FIXME: implement rotation properly again

	if (!rotate)
	{
		VectorCopy (min, rmin);
		VectorCopy (max, rmax);
	}
	else
	{
	// find min / max for rotations
		angles = e->v.angles;
		
		a = angles[1]/180 * M_PI;
		
		xvector[0] = cos(a);
		xvector[1] = sin(a);
		yvector[0] = -sin(a);
		yvector[1] = cos(a);
		
		VectorCopy (min, bounds[0]);
		VectorCopy (max, bounds[1]);
		
		rmin[0] = rmin[1] = rmin[2] = 9999;
		rmax[0] = rmax[1] = rmax[2] = -9999;
		
		for (i=0 ; i<= 1 ; i++)
		{
			base[0] = bounds[i][0];
			for (j=0 ; j<= 1 ; j++)
			{
				base[1] = bounds[j][1];
				for (k=0 ; k<= 1 ; k++)
				{
					base[2] = bounds[k][2];
					
				// transform the point
					transformed[0] = xvector[0]*base[0] + yvector[0]*base[1];
					transformed[1] = xvector[1]*base[0] + yvector[1]*base[1];
					transformed[2] = base[2];
					
					for (l=0 ; l<3 ; l++)
					{
						if (transformed[l] < rmin[l])
							rmin[l] = transformed[l];
						if (transformed[l] > rmax[l])
							rmax[l] = transformed[l];
					}
				}
			}
		}
	}
	
// set derived values
	VectorCopy (rmin, e->v.mins);
	VectorCopy (rmax, e->v.maxs);
	VectorSubtract (max, min, e->v.size);
	
	sv->SV_LinkEdict (e, false);
}

/*
=================
PF_setsize

the size box is rotated by the current angle

setsize (entity, minvector, maxvector)
=================
*/
void PF_setsize (void)
{
	edict_t	*e;
	float	*min, *max;
	
	e = G_EDICT(OFS_PARM0);
	min = G_VECTOR(OFS_PARM1);
	max = G_VECTOR(OFS_PARM2);
	SetMinMaxSize (e, min, max, false);
}


/*
=================
PF_setmodel

setmodel(entity, model)
=================
*/
static void PF_setmodel (void)
{
	edict_t	*e;
	const char* m;
	const char** check;
	model_t	*mod;
	int		i;

	e = G_EDICT(OFS_PARM0);
	m = G_STRING(OFS_PARM1);

// check to see if model was properly precached
	for (i = 0, check = sv->GetModelPrecache(); *check; i++, check++)
	{
		if (!strcmp(*check, m))
			break;
	}
			
	if (!*check)
		PR_RunError ("no precache: %s\n", m);
		

	e->v.model = PR_SetEngineString(*check);
	e->v.modelindex = i; //SV_ModelIndex (m);

	mod = sv->GetModels()[ (int)e->v.modelindex];  // Mod_ForName (m, true);
	
	if (mod)
		SetMinMaxSize (e, mod->mins, mod->maxs, true);
	else
		SetMinMaxSize (e, vec3_origin, vec3_origin, true);
}

/*
=================
PF_bprint

broadcast print to everyone on server

bprint(value)
=================
*/
void PF_bprint (void)
{
	char		*s;

	s = PF_VarString(0);
	sv->SV_BroadcastPrintf ("%s", s);
}

/*
=================
PF_sprint

single print to a specific client

sprint(clientent, value)
=================
*/
void PF_sprint (void)
{
	char		*s;
	client_t	*client;
	int			entnum;
	
	entnum = G_EDICTNUM(OFS_PARM0);
	s = PF_VarString(1);
	
	if (entnum < 1 || entnum > svs.maxclients)
	{
		Con_Printf ("tried to sprint to a non-client\n");
		return;
	}
		
	client = &svs.clients[entnum-1];
		
	MSG_WriteChar (&client->message,svc_print);
	MSG_WriteString (&client->message, s );
}


/*
=================
PF_centerprint

single print to a specific client

centerprint(clientent, value)
=================
*/
void PF_centerprint (void)
{
	char		*s;
	client_t	*client;
	int			entnum;
	
	entnum = G_EDICTNUM(OFS_PARM0);
	s = PF_VarString(1);
	
	if (entnum < 1 || entnum > svs.maxclients)
	{
		Con_Printf ("tried to sprint to a non-client\n");
		return;
	}
		
	client = &svs.clients[entnum-1];
		
	MSG_WriteChar (&client->message,svc_centerprint);
	MSG_WriteString (&client->message, s );
}

void PF_drawdebugline()
{
	float* origin = G_VECTOR(OFS_PARM0);
	float* end = G_VECTOR(OFS_PARM1);
	float* color = G_VECTOR(OFS_PARM2);

	glBegin(GL_POLYGON);
	glColor3fv(color);
	glVertex3fv(origin);
	glVertex3fv(end);
	glEnd();
}

/*
=================
PF_normalize

vector normalize(vector)
=================
*/
void PF_normalize (void)
{
	float	*value1;
	vec3_t	newvalue;
	float	fnew;
	
	value1 = G_VECTOR(OFS_PARM0);

	fnew = value1[0] * value1[0] + value1[1] * value1[1] + value1[2]*value1[2];
	fnew = sqrt(fnew);
	
	if (fnew == 0)
		newvalue[0] = newvalue[1] = newvalue[2] = 0;
	else
	{
		fnew = 1/fnew;
		newvalue[0] = value1[0] * fnew;
		newvalue[1] = value1[1] * fnew;
		newvalue[2] = value1[2] * fnew;
	}
	
	VectorCopy (newvalue, G_VECTOR(OFS_RETURN));	
}

/*
=================
PF_vlen

scalar vlen(vector)
=================
*/
void PF_vlen (void)
{
	float	*value1;
	float	fnew;
	
	value1 = G_VECTOR(OFS_PARM0);

	fnew = value1[0] * value1[0] + value1[1] * value1[1] + value1[2]*value1[2];
	fnew = sqrt(fnew);
	
	G_FLOAT(OFS_RETURN) = fnew;
}

/*
=================
PF_vectoyaw

float vectoyaw(vector)
=================
*/
void PF_vectoyaw (void)
{
	float	*value1;
	float	yaw;
	
	value1 = G_VECTOR(OFS_PARM0);

	if (value1[1] == 0 && value1[0] == 0)
		yaw = 0;
	else
	{
		yaw = (int) (atan2(value1[1], value1[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;
	}

	G_FLOAT(OFS_RETURN) = yaw;
}


/*
=================
PF_vectoangles

vector vectoangles(vector)
=================
*/
void PF_vectoangles (void)
{
	float	*value1;
	float	forward;
	float	yaw, pitch;
	
	value1 = G_VECTOR(OFS_PARM0);

	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (int) (atan2(value1[1], value1[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;

		forward = sqrt (value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = (int) (atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	G_FLOAT(OFS_RETURN+0) = pitch;
	G_FLOAT(OFS_RETURN+1) = yaw;
	G_FLOAT(OFS_RETURN+2) = 0;
}

/*
=================
PF_Random

Returns a number from 0<= num < 1

random()
=================
*/
void PF_random (void)
{
	float		num;
		
	num = (rand ()&0x7fff) / ((float)0x7fff);
	
	G_FLOAT(OFS_RETURN) = num;
}

/*
=================
PF_particle

particle(origin, color, count)
=================
*/
void PF_particle (void)
{
	float		*org, *dir;
	float		color;
	float		count;
			
	org = G_VECTOR(OFS_PARM0);
	dir = G_VECTOR(OFS_PARM1);
	color = G_FLOAT(OFS_PARM2);
	count = G_FLOAT(OFS_PARM3);
	sv->SV_StartParticle (org, dir, color, count);
}


/*
=================
PF_ambientsound

=================
*/
void PF_ambientsound (void)
{
	const char		**check;
	const char		*samp;
	float		*pos;
	float 		vol, attenuation;
	int			i, soundnum;

	pos = G_VECTOR (OFS_PARM0);			
	samp = G_STRING(OFS_PARM1);
	vol = G_FLOAT(OFS_PARM2);
	attenuation = G_FLOAT(OFS_PARM3);

    const char** precache = sv->GetSoundPrecache();
	
// check to see if samp was properly precached
	for (soundnum=0, check = precache ; *check ; check++, soundnum++)
		if (!strcmp(*check,samp))
			break;
			
	if (!*check)
	{
		Con_Printf ("no precache: %s\n", samp);
		return;
	}

// add an svc_spawnambient command to the level signon packet

	MSG_WriteByte (&sv->GetSignOnBuffer(),svc_spawnstaticsound);
	for (i=0 ; i<3 ; i++)
		MSG_WriteCoord(&sv->GetSignOnBuffer(), pos[i]);

	MSG_WriteByte (&sv->GetSignOnBuffer(), soundnum);

	MSG_WriteByte (&sv->GetSignOnBuffer(), vol*255);
	MSG_WriteByte (&sv->GetSignOnBuffer(), attenuation*64);

}

/*
=================
PF_sound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.

=================
*/
void PF_sound (void)
{
	const char		*sample;
	int			channel;
	edict_t		*entity;
	int 		vol;
	float attenuation;
	
#ifdef _DEBUG
	edict_t* ent = G_EDICT(OFS_PARM0);
#endif

	entity = G_EDICT(OFS_PARM0);
	channel = G_FLOAT(OFS_PARM1);
	sample = G_STRING(OFS_PARM2);
	vol = G_FLOAT(OFS_PARM3) * 255;
	attenuation = G_FLOAT(OFS_PARM4);
	
	if (vol < 0 || vol > 255)
		Sys_Error ("SV_StartSound: volume = %i", vol);

	if (attenuation < 0 || attenuation > 4)
		Sys_Error ("SV_StartSound: attenuation = %f", attenuation);

	if (channel < 0 || channel > 7)
		Sys_Error ("SV_StartSound: channel = %i", channel);

	sv->SV_StartSound (entity, channel, sample, vol, attenuation);
}

/*
=================
PF_break

break()
=================
*/
void PF_break (void)
{
Con_Printf ("break statement\n");
*(int *)-4 = 0;	// dump to debugger
//	PR_RunError ("break statement");
}

/*
=================
PF_traceline

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

traceline (vector1, vector2, tryents)
=================
*/
void PF_traceline (void)
{
	float	*v1, *v2;
	trace_t	trace;
	int		nomonsters;
	edict_t	*ent;

	v1 = G_VECTOR(OFS_PARM0);
	v2 = G_VECTOR(OFS_PARM1);
	nomonsters = G_FLOAT(OFS_PARM2);
	ent = G_EDICT(OFS_PARM3);

	if (IS_NAN(v1[0]) || IS_NAN(v1[2]) || IS_NAN(v1[1]))
		Con_Warning("PF_traceline: NaN in v1 variable!\n");

	if (IS_NAN(v2[0]) || IS_NAN(v2[2]) || IS_NAN(v2[1]))
		Con_Warning("PF_traceline: NaN in v1 variable!\n");

	trace = sv->SV_Move (v1, vec3_origin, vec3_origin, v2, nomonsters, EDICT_NUM(0));

	pr_global_struct->trace_allsolid = trace.allsolid;
	pr_global_struct->trace_startsolid = trace.startsolid;
	pr_global_struct->trace_fraction = trace.fraction;
	pr_global_struct->trace_inwater = trace.inwater;
	pr_global_struct->trace_inopen = trace.inopen;
	VectorCopy (trace.endpos, pr_global_struct->trace_endpos);
	VectorCopy (trace.plane.normal, pr_global_struct->trace_plane_normal);
	pr_global_struct->trace_plane_dist =  trace.plane.dist;	
	if (trace.ent)
		pr_global_struct->trace_ent = EDICT_TO_PROG(trace.ent);
	else
		pr_global_struct->trace_ent = EDICT_TO_PROG(sv->GetServerEdicts());
}

/*extern trace_t SV_Trace_Toss(edict_t* ent, edict_t* ignore);

void PF_TraceToss (void)
{
	trace_t	trace;
	edict_t	*ent;
	edict_t	*ignore;

	ent = G_EDICT(OFS_PARM0);
	ignore = G_EDICT(OFS_PARM1);

	trace = SV_Trace_Toss (ent, ignore);

	pr_global_struct->trace_allsolid = trace.allsolid;
	pr_global_struct->trace_startsolid = trace.startsolid;
	pr_global_struct->trace_fraction = trace.fraction;
	pr_global_struct->trace_inwater = trace.inwater;
	pr_global_struct->trace_inopen = trace.inopen;
	VectorCopy (trace.endpos, pr_global_struct->trace_endpos);
	VectorCopy (trace.plane.normal, pr_global_struct->trace_plane_normal);
	pr_global_struct->trace_plane_dist =  trace.plane.dist;	
	if (trace.ent)
		pr_global_struct->trace_ent = EDICT_TO_PROG(trace.ent);
	else
		pr_global_struct->trace_ent = EDICT_TO_PROG(sv->GetServerEdicts());
}*/

/*
=================
PF_checkpos

Returns true if the given entity can move to the given position from it's
current position by walking or rolling.
FIXME: make work...
scalar checkpos (entity, vector)
=================
*/
void PF_checkpos (void)
{
}

//============================================================================

byte	checkpvs[MAX_MAP_LEAFS/8];

int PF_newcheckclient (int check)
{
	int		i;
	byte	*pvs;
	edict_t	*ent;
	mleaf_t	*leaf;
	vec3_t	org;

// cycle to the next one

	if (check < 1)
		check = 1;
	if (check > svs.maxclients)
		check = svs.maxclients;

	if (check == svs.maxclients)
		i = 1;
	else
		i = check + 1;

	for ( ;  ; i++)
	{
		if (i == svs.maxclients+1)
			i = 1;

		ent = EDICT_NUM(i);

		if (i == check)
			break;	// didn't find anything else

		if (ent->free)
			continue;
		if (ent->v.health <= 0)
			continue;
		if ((int)ent->v.flags & FL_NOTARGET)
			continue;

	// anything that is a client, or has a client as an enemy
		break;
	}

// get the PVS for the entity
	VectorAdd (ent->v.origin, ent->v.view_ofs, org);
	leaf = Mod_PointInLeaf (org, sv->GetWorldModel());
	pvs = Mod_LeafPVS (leaf, sv->GetWorldModel());
	memcpy (checkpvs, pvs, (sv->GetWorldModel()->numleafs+7)>>3 );

	return i;
}

/*
=================
PF_checkclient

Returns a client (or object that has a client enemy) that would be a
valid target.

If there are more than one valid options, they are cycled each frame

If (self.origin + self.viewofs) is not in the PVS of the current target,
it is not returned at all.

name checkclient ()
=================
*/
#define	MAX_CHECK	16
int c_invis, c_notvis;
void PF_checkclient (void)
{
	edict_t	*ent, *self;
	mleaf_t	*leaf;
	int		l;
	vec3_t	view;
	
// find a new check if on a new frame
	if (sv->GetServerTime() - sv->GetLastCheckTime() >= 0.1)
	{
		sv->SetLastCheck(PF_newcheckclient (sv->GetLastCheck()));
		sv->SetLastCheckTime(sv->GetServerTime());
	}

// return check if it might be visible	
	ent = EDICT_NUM(sv->GetLastCheck());
	if (ent->free || ent->v.health <= 0)
	{
		RETURN_EDICT(sv->GetServerEdicts());
		return;
	}

// if current entity can't possibly see the check entity, return 0
	self = PROG_TO_EDICT(pr_global_struct->self);
	VectorAdd (self->v.origin, self->v.view_ofs, view);
	leaf = Mod_PointInLeaf (view, sv->GetWorldModel());
	l = (leaf - sv->GetWorldModel()->leafs) - 1;
	if ( (l<0) || !(checkpvs[l>>3] & (1<<(l&7)) ) )
	{
c_notvis++;
		RETURN_EDICT(sv->GetServerEdicts());
		return;
	}

// might be able to see it
c_invis++;
	RETURN_EDICT(ent);
}

//============================================================================


/*
=================
PF_stuffcmd

Sends text over to the client's execution buffer

stuffcmd (clientent, value)
=================
*/
void PF_stuffcmd (void)
{
	int		entnum;
	const char	*str = "";
	client_t	*old = NULL;
	
	entnum = G_EDICTNUM(OFS_PARM0);
	if (entnum < 1 || entnum > svs.maxclients)
		PR_RunError ("Parm 0 not a client");
	str = G_STRING(OFS_PARM1);	
	
	old = host_client;
	host_client = &svs.clients[entnum-1];
	host->Host_ClientCommands ("%s", str);
	host_client = old;
}

/*
=================
PF_localcmd

Sends text over to the client's execution buffer

localcmd (string)
=================
*/
void PF_localcmd (void)
{
	const char	*str;
	
	str = G_STRING(OFS_PARM0);	
	g_pCmdBuf->Cbuf_AddText (str);
}

/*
=================
PF_cvar

float cvar (string)
=================
*/
void PF_cvar (void)
{
	const char	*str;
	
	str = G_STRING(OFS_PARM0);
	
	G_FLOAT(OFS_RETURN) = Cvar_VariableValue (str);
}

/*
=================
PF_cvar_set

float cvar (string)
=================
*/
void PF_cvar_set (void)
{
	const char	*var, *val;
	
	var = G_STRING(OFS_PARM0);
	val = G_STRING(OFS_PARM1);
	
	Cvar_Set (var, val);
}

/*
=================
PF_findradius

Returns a chain of entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
void PF_findradius (void)
{
	edict_t	*ent, *chain;
	float	rad;
	float	*org;
	vec3_t	eorg;
	int		i, j;

	chain = (edict_t *)sv->GetServerEdicts();
	
	org = G_VECTOR(OFS_PARM0);
	rad = G_FLOAT(OFS_PARM1);

	ent = NEXT_EDICT(sv->GetServerEdicts());
	for (i=1 ; i<sv->GetNumEdicts() ; i++, ent = NEXT_EDICT(ent))
	{
		if (ent->free)
			continue;
		if (ent->v.solid == SOLID_NOT)
			continue;
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (ent->v.origin[j] + (ent->v.mins[j] + ent->v.maxs[j])*0.5);			
		if (Length(eorg) > rad)
			continue;
			
		ent->v.chain = EDICT_TO_PROG(chain);
		chain = ent;
	}

	RETURN_EDICT(chain);
}


/*
=========
PF_dprint
=========
*/
void PF_dprint (void)
{
	Con_DPrintf ("%s",PF_VarString(0));
}

char	pr_string_temp[128];

void PF_ftos (void)
{
	float	v;
	v = G_FLOAT(OFS_PARM0);
	
	if (v == (int)v)
		snprintf (pr_string_temp, sizeof(pr_string_temp), "%d",(int)v);
	else
		snprintf (pr_string_temp, sizeof(pr_string_temp), "%f",v);
	G_INT(OFS_RETURN) = PR_SetEngineString(pr_string_temp);
}

void PF_fabs (void)
{
	float	v;
	v = G_FLOAT(OFS_PARM0);
	G_FLOAT(OFS_RETURN) = fabs(v);
}

void PF_vtos (void)
{
	snprintf (pr_string_temp, sizeof(pr_string_temp), "'%5.1f %5.1f %5.1f'", G_VECTOR(OFS_PARM0)[0], G_VECTOR(OFS_PARM0)[1], G_VECTOR(OFS_PARM0)[2]);
	G_INT(OFS_RETURN) = PR_SetEngineString(pr_string_temp);
}

void PF_etos (void)
{
	sprintf (pr_string_temp, "entity %i", G_EDICTNUM(OFS_PARM0));
	G_INT(OFS_RETURN) = pr_string_temp - pr_strings;
}

void PF_Spawn (void)
{
	edict_t	*ed;
	ed = ED_Alloc();
	RETURN_EDICT(ed);
}

void PF_Remove (void)
{
	edict_t	*ed;
	
	ed = G_EDICT(OFS_PARM0);
	ED_Free (ed);
}


// entity (entity start, .string field, string match) find = #5;
void PF_Find (void)
#ifdef QUAKE2
{
	int		e;	
	int		f;
	char	*s, *t;
	edict_t	*ed;
	edict_t	*first;
	edict_t	*second;
	edict_t	*last;

	first = second = last = (edict_t *)sv->GetServerEdicts();
	e = G_EDICTNUM(OFS_PARM0);
	f = G_INT(OFS_PARM1);
	s = G_STRING(OFS_PARM2);
	if (!s)
		PR_RunError ("PF_Find: bad search string");
		
	for (e++ ; e < sv->num_edicts ; e++)
	{
		ed = EDICT_NUM(e);
		if (ed->free)
			continue;
		t = E_STRING(ed,f);
		if (!t)
			continue;
		if (!strcmp(t,s))
		{
			if (first == (edict_t *)sv->GetServerEdicts())
				first = ed;
			else if (second == (edict_t *)sv->GetServerEdicts())
				second = ed;
			ed->v.chain = EDICT_TO_PROG(last);
			last = ed;
		}
	}

	if (first != last)
	{
		if (last != second)
			first->v.chain = last->v.chain;
		else
			first->v.chain = EDICT_TO_PROG(last);
		last->v.chain = EDICT_TO_PROG((edict_t *)sv->GetServerEdicts());
		if (second && second != last)
			second->v.chain = EDICT_TO_PROG(last);
	}
	RETURN_EDICT(first);
}
#else
{
	int		e;	
	int		f;
	const char	*s, *t;
	edict_t	*ed;

	e = G_EDICTNUM(OFS_PARM0);
	f = G_INT(OFS_PARM1);
	s = G_STRING(OFS_PARM2);
	if (!s)
		PR_RunError ("PF_Find: bad search string");
		
	for (e++ ; e < sv->GetNumEdicts() ; e++)
	{
		ed = EDICT_NUM(e);
		if (ed->free)
			continue;
		t = E_STRING(ed,f);
		if (!t)
			continue;
		if (!strcmp(t,s))
		{
			RETURN_EDICT(ed);
			return;
		}
	}

	RETURN_EDICT(sv->GetServerEdicts());
}
#endif

void PR_CheckEmptyString (const char *s)
{
	if (s[0] <= ' ')
		PR_RunError ("Bad string");
}

void PF_precache_file (void)
{	// precache_file is only used to copy files with qcc, it does nothing
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
}

void PF_MoveToGoal(void)
{	// precache_file is only used to copy files with qcc, it does nothing
	sv->SV_MoveToGoal();
}

void PF_precache_sound (void)
{
	const char	*s;
	int		i;
	
	if (sv->GetServerState() != ss_loading)
		PR_RunError ("PF_Precache_*: Precache can only be done in spawn functions");
		
	s = G_STRING(OFS_PARM0);
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
	PR_CheckEmptyString (s);
	
	for (i=0 ; i<MAX_SOUNDS ; i++)
	{
        if (!sv->GetSoundPrecacheEntry(i))
		{
			sv->SetSoundPrecacheEntry(i, s);
			return;
		}
		if (!strcmp(sv->GetSoundPrecacheEntry(i), s))
			return;
	}
	PR_RunError ("PF_precache_sound: overflow");
}

static void PF_precache_model (void)
{
	const char* s;
	int		i, m;

	if (sv->GetServerState() != ss_loading)
		PR_RunError("PF_Precache_*: Precache can only be done in spawn functions");

	s = G_STRING(OFS_PARM0);
	m = G_INT(OFS_PARM0);
	PR_CheckEmptyString(s);

	for (i = 0; i < MAX_MODELS; i++)
	{
        if (!sv->GetModelPrecacheEntry(i))
		{
            sv->SetModelPrecacheEntry(i, s);
            sv->SetModelEntry(i, Mod_ForName(s, true));
			return;
		}
        if (!strcmp(sv->GetModelPrecacheEntry(i), s))
			return;
	}
	PR_RunError("PF_precache_model: overflow");
}


void PF_coredump (void)
{
	ED_PrintEdicts ();
}

void PF_traceon (void)
{
	pr_trace = true;
}

void PF_traceoff (void)
{
	pr_trace = false;
}

void PF_eprint (void)
{
	ED_PrintNum (G_EDICTNUM(OFS_PARM0));
}

/*
===============
PF_walkmove

float(float yaw, float dist) walkmove
===============
*/
void PF_walkmove (void)
{
	edict_t	*ent;
	float	yaw, dist;
	vec3_t	move;
	dfunction_t	*oldf;
	int 	oldself;
	
	ent = PROG_TO_EDICT(pr_global_struct->self);
	yaw = G_FLOAT(OFS_PARM0);
	dist = G_FLOAT(OFS_PARM1);
	
	if ( !( (int)ent->v.flags & (FL_ONGROUND|FL_FLY|FL_SWIM) ) )
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	yaw = yaw*M_PI*2 / 360;
	
	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

// save program state, because SV_movestep may call other progs
	oldf = pr_xfunction;
	oldself = pr_global_struct->self;
	
	G_FLOAT(OFS_RETURN) = sv->SV_movestep(ent, move, true);
	
	
// restore program state
	pr_xfunction = oldf;
	pr_global_struct->self = oldself;
}

/*
===============
PF_droptofloor

void() droptofloor
===============
*/
void PF_droptofloor (void)
{
	edict_t		*ent;
	vec3_t		end;
	trace_t		trace;
	
	ent = PROG_TO_EDICT(pr_global_struct->self);

	VectorCopy (ent->v.origin, end);
	end[2] -= 256;
	
	trace = sv->SV_Move (ent->v.origin, ent->v.mins, ent->v.maxs, end, false, ent);

	if (trace.fraction == 1 || trace.allsolid)
		G_FLOAT(OFS_RETURN) = 0;
	else
	{
		VectorCopy (trace.endpos, ent->v.origin);
		sv->SV_LinkEdict (ent, false);
		ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
		ent->v.groundentity = EDICT_TO_PROG(trace.ent);
		G_FLOAT(OFS_RETURN) = 1;
	}
}

/*
===============
PF_lightstyle

void(float style, string value) lightstyle
===============
*/
void PF_lightstyle (void)
{
	int		style;
	const char	*val;
	client_t	*client;
	int			j;
	
	style = G_FLOAT(OFS_PARM0);
	val = G_STRING(OFS_PARM1);

	// bounds check to avoid clobbering sv struct
	if (style < 0 || style >= MAX_LIGHTSTYLES)
	{
		Con_DPrintf("PF_lightstyle: invalid style %d\n", style);
		return;
	}

// change the string in sv
	sv->SetLightStyle(style, val);
	
// send message to all clients on this server
	if (sv->GetServerState() != ss_active)
		return;
	
	for (j=0, client = svs.clients ; j<svs.maxclients ; j++, client++)
		if (client->active || client->spawned)
		{
			MSG_WriteChar (&client->message, svc_lightstyle);
			MSG_WriteChar (&client->message,style);
			MSG_WriteString (&client->message, val);
		}
}

void PF_rint (void)
{
	float	f;
	f = G_FLOAT(OFS_PARM0);
	if (f > 0)
		G_FLOAT(OFS_RETURN) = (int)(f + 0.5);
	else
		G_FLOAT(OFS_RETURN) = (int)(f - 0.5);
}
void PF_floor (void)
{
	G_FLOAT(OFS_RETURN) = floor(G_FLOAT(OFS_PARM0));
}
void PF_ceil (void)
{
	G_FLOAT(OFS_RETURN) = ceil(G_FLOAT(OFS_PARM0));
}


/*
=============
PF_checkbottom
=============
*/
void PF_checkbottom (void)
{
	edict_t	*ent;
	
	ent = G_EDICT(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = sv->SV_CheckBottom (ent);
}

/*
=============
PF_pointcontents
=============
*/
void PF_pointcontents (void)
{
	float	*v;
	
	v = G_VECTOR(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = sv->SV_PointContents (v);
}

/*
=============
PF_nextent

entity nextent(entity)
=============
*/
void PF_nextent (void)
{
	int		i;
	edict_t	*ent;
	
	i = G_EDICTNUM(OFS_PARM0);
	while (1)
	{
		i++;
		if (i == sv->GetNumEdicts())
		{
			RETURN_EDICT(sv->GetServerEdicts());
			return;
		}
		ent = EDICT_NUM(i);
		if (!ent->free)
		{
			RETURN_EDICT(ent);
			return;
		}
	}
}

/*
=============
PF_aim

Pick a vector for the player to shoot along
vector aim(entity, missilespeed)
=============
*/
cvar_t	sv_aim = {"sv_aim", "0.93"};
void PF_aim (void)
{
	edict_t	*ent, *check, *bestent;
	vec3_t	start, dir, end, bestdir;
	int		i, j;
	trace_t	tr;
	float	dist, bestdist;
	float	speed;
	
	ent = G_EDICT(OFS_PARM0);
	speed = G_FLOAT(OFS_PARM1);

	VectorCopy (ent->v.origin, start);
	start[2] += 20;

// try sending a trace straight
	VectorCopy (pr_global_struct->v_forward, dir);
	VectorMA (start, 2048, dir, end);
	tr = sv->SV_Move (start, vec3_origin, vec3_origin, end, false, ent);
	if (tr.ent && tr.ent->v.takedamage == DAMAGE_AIM
	&& (!host->teamplay.value || ent->v.team <=0 || ent->v.team != tr.ent->v.team) )
	{
		VectorCopy (pr_global_struct->v_forward, G_VECTOR(OFS_RETURN));
		return;
	}


// try all possible entities
	VectorCopy (dir, bestdir);
	bestdist = sv_aim.value;
	bestent = NULL;
	
	check = NEXT_EDICT(sv->GetServerEdicts());
	for (i=1 ; i<sv->GetNumEdicts() ; i++, check = NEXT_EDICT(check) )
	{
		if (check->v.takedamage != DAMAGE_AIM)
			continue;
		if (check == ent)
			continue;
		if (host->teamplay.value && ent->v.team > 0 && ent->v.team == check->v.team)
			continue;	// don't aim at teammate
		for (j=0 ; j<3 ; j++)
			end[j] = check->v.origin[j]
			+ 0.5*(check->v.mins[j] + check->v.maxs[j]);
		VectorSubtract (end, start, dir);
		VectorNormalize (dir);
		dist = DotProduct (dir, pr_global_struct->v_forward);
		if (dist < bestdist)
			continue;	// to far to turn
		tr = sv->SV_Move (start, vec3_origin, vec3_origin, end, false, ent);
		if (tr.ent == check)
		{	// can shoot at this one
			bestdist = dist;
			bestent = check;
		}
	}
	
	if (bestent)
	{
		VectorSubtract (bestent->v.origin, ent->v.origin, dir);
		dist = DotProduct (dir, pr_global_struct->v_forward);
		VectorScale (pr_global_struct->v_forward, dist, end);
		end[2] = dir[2];
		VectorNormalize (end);
		VectorCopy (end, G_VECTOR(OFS_RETURN));	
	}
	else
	{
		VectorCopy (bestdir, G_VECTOR(OFS_RETURN));
	}
}

/*
==============
PF_changeyaw

This was a major timewaster in progs, so it was converted to C
==============
*/
void PF_changeyaw (void)
{
	edict_t		*ent;
	float		ideal, current, move, speed;
	
	ent = PROG_TO_EDICT(pr_global_struct->self);
	current = anglemod( ent->v.angles[1] );
	ideal = ent->v.ideal_yaw;
	speed = ent->v.yaw_speed;
	
	if (current == ideal)
		return;
	move = ideal - current;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}
	
	ent->v.angles[1] = anglemod (current + move);
}

/*
==============
PF_changepitch
==============
*/
void PF_changepitch (void)
{
	edict_t		*ent;
	float		ideal, current, move, speed;
	
	ent = G_EDICT(OFS_PARM0);
	current = anglemod( ent->v.angles[0] );
	ideal = ent->v.idealpitch;
	speed = ent->v.pitch_speed;
	
	if (current == ideal)
		return;
	move = ideal - current;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}
	
	ent->v.angles[0] = anglemod (current + move);
}

/*
===============================================================================

MESSAGE WRITING

===============================================================================
*/

#define	MSG_BROADCAST	0		// unreliable to all
#define	MSG_ONE			1		// reliable to one (msg_entity)
#define	MSG_ALL			2		// reliable to all
#define	MSG_INIT		3		// write to the init string

sizebuf_t *WriteDest (void)
{
	int		entnum;
	int		dest;
	edict_t	*ent;

	dest = G_FLOAT(OFS_PARM0);
	switch (dest)
	{
	case MSG_BROADCAST:
		return &sv->GetDatagramBuffer();
	
	case MSG_ONE:
		ent = PROG_TO_EDICT(pr_global_struct->msg_entity);
		entnum = NUM_FOR_EDICT(ent);
		if (entnum < 1 || entnum > svs.maxclients)
			PR_RunError ("WriteDest: not a client");
		return &svs.clients[entnum-1].message;
		
	case MSG_ALL:
		return &sv->GetReliableDatagramBuffer();
	
	case MSG_INIT:
		return &sv->GetSignOnBuffer();

	default:
		PR_RunError ("WriteDest: bad destination");
		break;
	}
	
	return NULL;
}

void PF_WriteByte (void)
{
	MSG_WriteByte (WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteChar (void)
{
	MSG_WriteChar (WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteShort (void)
{
	MSG_WriteShort (WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteLong (void)
{
	MSG_WriteLong (WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteAngle (void)
{
	MSG_WriteAngle (WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteCoord (void)
{
	MSG_WriteCoord (WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteString (void)
{
	MSG_WriteString (WriteDest(), G_STRING(OFS_PARM1));
}


void PF_WriteEntity (void)
{
	MSG_WriteShort (WriteDest(), G_EDICTNUM(OFS_PARM1));
}

//=============================================================================

void PF_makestatic (void)
{
	edict_t	*ent;
	int		i;
	
	ent = G_EDICT(OFS_PARM0);

	if (sv->SV_ModelIndex(PR_GetString(ent->v.model)) & 0xFF00 || (int)(ent->v.frame) & 0xFF00)
	{
		ED_Free(ent);
		return; //can't display the correct model & frame, so don't show it at all
	}

	MSG_WriteByte (&sv->GetSignOnBuffer(),svc_spawnstatic);

	MSG_WriteByte (&sv->GetSignOnBuffer(), sv->SV_ModelIndex(PR_GetString(ent->v.model)));

	MSG_WriteByte (&sv->GetSignOnBuffer(), ent->v.frame);
	MSG_WriteByte (&sv->GetSignOnBuffer(), ent->v.colormap);
	MSG_WriteByte (&sv->GetSignOnBuffer(), ent->v.skin);
	for (i=0 ; i<3 ; i++)
	{
		MSG_WriteCoord(&sv->GetSignOnBuffer(), ent->v.origin[i]);
		MSG_WriteAngle(&sv->GetSignOnBuffer(), ent->v.angles[i]);
	}

// throw the entity away now
	ED_Free (ent);
}

//=============================================================================

/*
==============
PF_setspawnparms
==============
*/
void PF_setspawnparms (void)
{
	edict_t	*ent;
	int		i;
	client_t	*client;

	ent = G_EDICT(OFS_PARM0);
	i = NUM_FOR_EDICT(ent);
	if (i < 1 || i > svs.maxclients)
		PR_RunError ("Entity is not a client");

	// copy spawn parms out of the client_t
	client = svs.clients + (i-1);

	for (i=0 ; i< NUM_SPAWN_PARMS ; i++)
		(&pr_global_struct->parm1)[i] = client->spawn_parms[i];
}

/*
==============
PF_changelevel
==============
*/
void PF_changelevel (void)
{
#ifdef QUAKE2
	char	*s1, *s2;

	if (svs.changelevel_issued)
		return;
	svs.changelevel_issued = true;

	s1 = G_STRING(OFS_PARM0);
	s2 = G_STRING(OFS_PARM1);

	if ((int)pr_global_struct->serverflags & (SFL_NEW_UNIT | SFL_NEW_EPISODE))
		Cbuf_AddText (va("changelevel %s %s\n",s1, s2));
	else
		Cbuf_AddText (va("changelevel2 %s %s\n",s1, s2));
#else
	const char	*s;

// make sure we don't issue two changelevels
	if (svs.changelevel_issued)
		return;
	svs.changelevel_issued = true;
	
	s = G_STRING(OFS_PARM0);
	g_pCmdBuf->Cbuf_AddText (g_Common->va("changelevel %s\n",s));
#endif
}

#define	CONTENT_WATER	-3
#define CONTENT_SLIME	-4
#define CONTENT_LAVA	-5

#define FL_IMMUNE_WATER	131072
#define	FL_IMMUNE_SLIME	262144
#define FL_IMMUNE_LAVA	524288

#define	CHAN_VOICE	2
#define	CHAN_BODY	4

#define	ATTN_NORM	1

void PF_WaterMove (void)
{
	edict_t		*self;
	int			flags;
	int			waterlevel;
	int			watertype;
	float		drownlevel;
	float		damage = 0.0;

	self = PROG_TO_EDICT(pr_global_struct->self);

	if (self->v.movetype == MOVETYPE_NOCLIP)
	{
		self->v.air_finished = sv->GetServerTime() + 12;
		G_FLOAT(OFS_RETURN) = damage;
		return;
	}

	if (self->v.health < 0)
	{
		G_FLOAT(OFS_RETURN) = damage;
		return;
	}

	if (self->v.deadflag == DEAD_NO)
		drownlevel = 3;
	else
		drownlevel = 1;

	flags = (int)self->v.flags;
	waterlevel = (int)self->v.waterlevel;
	watertype = (int)self->v.watertype;

	if (!(flags & (FL_IMMUNE_WATER + FL_GODMODE)))
		if (((flags & FL_SWIM) && (waterlevel < drownlevel)) || (waterlevel >= drownlevel))
		{
			if (self->v.air_finished < sv->GetServerTime())
				if (self->v.pain_finished < sv->GetServerTime())
				{
					self->v.dmg = self->v.dmg + 2;
					if (self->v.dmg > 15)
						self->v.dmg = 10;
//					T_Damage (self, world, world, self.dmg, 0, FALSE);
					damage = self->v.dmg;
					self->v.pain_finished = sv->GetServerTime() + 1.0;
				}
		}
		else
		{
			if (self->v.air_finished < sv->GetServerTime())
//				sound (self, CHAN_VOICE, "player/gasp2.wav", 1, ATTN_NORM);
				sv->SV_StartSound (self, CHAN_VOICE, "player/gasp2.wav", 255, ATTN_NORM);
			else if (self->v.air_finished < sv->GetServerTime() + 9)
//				sound (self, CHAN_VOICE, "player/gasp1.wav", 1, ATTN_NORM);
				sv->SV_StartSound (self, CHAN_VOICE, "player/gasp1.wav", 255, ATTN_NORM);
			self->v.air_finished = sv->GetServerTime() + 12.0;
			self->v.dmg = 2;
		}
	
	if (!waterlevel)
	{
		if (flags & FL_INWATER)
		{	
			// play leave water sound
//			sound (self, CHAN_BODY, "misc/outwater.wav", 1, ATTN_NORM);
			sv->SV_StartSound (self, CHAN_BODY, "misc/outwater.wav", 255, ATTN_NORM);
			self->v.flags = (float)(flags &~FL_INWATER);
		}
		self->v.air_finished = sv->GetServerTime() + 12.0;
		G_FLOAT(OFS_RETURN) = damage;
		return;
	}

	if (watertype == CONTENT_LAVA)
	{	// do damage
		if (!(flags & (FL_IMMUNE_LAVA + FL_GODMODE)))
			if (self->v.dmgtime < sv->GetServerTime())
			{
				if (self->v.radsuit_finished < sv->GetServerTime())
					self->v.dmgtime = sv->GetServerTime() + 0.2;
				else
					self->v.dmgtime = sv->GetServerTime() + 1.0;
//				T_Damage (self, world, world, 10*self.waterlevel, 0, TRUE);
				damage = (float)(10*waterlevel);
			}
	}
	else if (watertype == CONTENT_SLIME)
	{	// do damage
		if (!(flags & (FL_IMMUNE_SLIME + FL_GODMODE)))
			if (self->v.dmgtime < sv->GetServerTime() && self->v.radsuit_finished < sv->GetServerTime())
			{
				self->v.dmgtime = sv->GetServerTime() + 1.0;
//				T_Damage (self, world, world, 4*self.waterlevel, 0, TRUE);
				damage = (float)(4*waterlevel);
			}
	}
	
	if ( !(flags & FL_INWATER) )
	{	

// player enter water sound
		if (watertype == CONTENT_LAVA)
//			sound (self, CHAN_BODY, "player/inlava.wav", 1, ATTN_NORM);
			sv->SV_StartSound (self, CHAN_BODY, "player/inlava.wav", 255, ATTN_NORM);
		if (watertype == CONTENT_WATER)
//			sound (self, CHAN_BODY, "player/inh2o.wav", 1, ATTN_NORM);
			sv->SV_StartSound (self, CHAN_BODY, "player/inh2o.wav", 255, ATTN_NORM);
		if (watertype == CONTENT_SLIME)
//			sound (self, CHAN_BODY, "player/slimbrn2.wav", 1, ATTN_NORM);
			sv->SV_StartSound (self, CHAN_BODY, "player/slimbrn2.wav", 255, ATTN_NORM);

		self->v.flags = (float)(flags | FL_INWATER);
		self->v.dmgtime = 0;
	}
	
	if (! (flags & FL_WATERJUMP) )
	{
//		self.velocity = self.velocity - 0.8*self.waterlevel*frametime*self.velocity;
		VectorMA (self->v.velocity, -0.8 * self->v.waterlevel * host->host_frametime, self->v.velocity, self->v.velocity);
	}

	G_FLOAT(OFS_RETURN) = damage;
}

void PF_sin (void)
{
	G_FLOAT(OFS_RETURN) = sin(G_FLOAT(OFS_PARM0));
}

void PF_cos (void)
{
	G_FLOAT(OFS_RETURN) = cos(G_FLOAT(OFS_PARM0));
}

void PF_sqrt (void)
{
	G_FLOAT(OFS_RETURN) = sqrt(G_FLOAT(OFS_PARM0));
}

/*
=================
Missi: PF_cxxvectoradd_flt

adds to a C++-like vector in QC

cxxvectoradd_flt (vecname, data) (6/12/2024)
=================
*/
static void PF_cxxvectoradd_flt(void)
{
	progvector_t* vec = G_CPPVECTOR(G_STRING(OFS_PARM0));
	float* val = new float(G_FLOAT(OFS_PARM1));

	vec->data->AddToEnd(val);
}

/*
=================
Missi: PF_cxxvectoradd_int

adds to a C++-like vector in QC

cxxvectoradd_int (vecname, data) (6/12/2024)
=================
*/
static void PF_cxxvectoradd_int(void)
{
	progvector_t* vec = G_CPPVECTOR(G_STRING(OFS_PARM0));
	int* val = new int((int)G_FLOAT(OFS_PARM1));

	vec->data->AddToEnd(val);
}

/*
=================
Missi: PF_cxxvectoradd_ent

adds to a C++-like vector in QC

cxxvectoradd_ent (vecname, data) (6/12/2024)
=================
*/
static void PF_cxxvectoradd_ent(void)
{
	progvector_t* vec = G_CPPVECTOR(G_STRING(OFS_PARM0));
	edict_t* val = new edict_t(*G_EDICT(OFS_PARM1));

	vec->data->AddToEnd(val);
}

/*
=================
Missi: PF_cxxvectoradd_str

adds to a C++-like vector in QC

cxxvectoradd_str (vecname, data) (6/12/2024)
=================
*/
static void PF_cxxvectoradd_str(void)
{
	progvector_t* vec = G_CPPVECTOR(G_STRING(OFS_PARM0));
	string_t* val = new string_t(*G_STRING(OFS_PARM1));

	vec->data->AddToEnd(val);
}

/*
=================
Missi: PF_vec_access_flt

pulls a value from a C++-like vector in QC

PF_vec_access_flt (vecname, element) (6/12/2024)
=================
*/
static void PF_vec_access_flt(void)
{
	progvector_t* vec = G_CPPVECTOR(G_STRING(OFS_PARM0));
	int pos = (int)G_FLOAT(OFS_PARM1);

	if (!vec || vec->data->GetBase()[0] == nullptr || vec->data->GetNumAllocated() == 0)
		return Con_Warning("WARNING: attempted to access empty vector!\n");

	if (pos > vec->data->GetNumAllocated())
		return Con_Warning("WARNING: vector \"%s\" attempted to access an element beyond its allocated size!\n", PR_GetString(vec->name));

	if (vec->data->GetBase()[pos] == nullptr)
		return Con_Warning("WARNING: vector \"%s\" attempted to access a null element!\n", PR_GetString(vec->name));

	float* test2 = (float*)vec->data->GetBase()[pos];

	G_FLOAT(OFS_RETURN) = *test2;
}

/*
=================
Missi: PF_vec_access_int

pulls a value from a C++-like vector in QC

PF_vec_access_int (vecname, element) (6/12/2024)
=================
*/
static void PF_vec_access_int(void)
{
	progvector_t* vec = G_CPPVECTOR(G_STRING(OFS_PARM0));
	int pos = (int)G_FLOAT(OFS_PARM1);

	if (!vec || vec->data->GetBase()[0] == nullptr || vec->data->GetNumAllocated() == 0)
		return Con_Warning("WARNING: attempted to access empty vector!\n");

	if (pos > vec->data->GetNumAllocated())
		return Con_Warning("WARNING: vector \"%s\" attempted to access an element beyond its allocated size!\n", PR_GetString(vec->name));

	if (vec->data->GetBase()[pos] == nullptr)
		return Con_Warning("WARNING: vector \"%s\" attempted to access a null element!\n", PR_GetString(vec->name));

	int* test2 = (int*)vec->data->GetBase()[pos];

	G_INT(OFS_RETURN) = *test2;
}

/*
=================
Missi: PF_vec_access_ent

pulls a value from a C++-like vector in QC

PF_vec_access_ent (vecname, element) (6/12/2024)
=================
*/
static void PF_vec_access_ent(void)
{
	progvector_t* vec = G_CPPVECTOR(G_STRING(OFS_PARM0));
	int pos = (int)G_FLOAT(OFS_PARM1);

	if (!vec || vec->data->GetBase()[0] == nullptr || vec->data->GetNumAllocated() == 0)
		return Con_Warning("WARNING: attempted to access empty vector!\n");

	if (pos > vec->data->GetNumAllocated())
		return Con_Warning("WARNING: vector \"%s\" attempted to access an element beyond its allocated size!\n", PR_GetString(vec->name));

	if (vec->data->GetBase()[pos] == nullptr)
		return Con_Warning("WARNING: vector \"%s\" attempted to access a null element!\n", PR_GetString(vec->name));

	edict_t* test2 = (edict_t*)vec->data->GetBase()[pos];

	*G_EDICT(OFS_RETURN) = *test2;
}

/*
=================
Missi: PF_vec_access_str

pulls a value from a C++-like vector in QC

PF_vec_access_str (vecname, element) (6/12/2024)
=================
*/
static void PF_vec_access_str(void)
{
	progvector_t* vec = G_CPPVECTOR(G_STRING(OFS_PARM0));
	int pos = (int)G_FLOAT(OFS_PARM1);

	if (!vec || vec->data->GetBase()[0] == nullptr || vec->data->GetNumAllocated() == 0)
		return Con_Warning("WARNING: attempted to access empty vector!\n");

	if (pos > vec->data->GetNumAllocated())
		return Con_Warning("WARNING: vector \"%s\" attempted to access an element beyond its allocated size!\n", PR_GetString(vec->name));

	if (vec->data->GetBase()[pos] == nullptr)
		return Con_Warning("WARNING: vector \"%s\" attempted to access a null element!\n", PR_GetString(vec->name));

	string_t* test2 = (string_t*)vec->data->GetBase()[pos];

	G_STRING_MOD(OFS_RETURN) = *test2;
}

/*
=================
Missi: PF_vec_size

obtains the size of a C++-like vector in QC

PF_vec_size (vecname) (6/12/2024)
=================
*/
static void PF_vec_size(void)
{
	progvector_t* vec = G_CPPVECTOR(G_STRING(OFS_PARM0));
	int size = vec->data->GetNumAllocated();
	float flsize = (float)size;

	G_FLOAT(OFS_RETURN) = flsize;
}

/*
=================
Missi: PF_findfield

finds a field in an entity

findfield (entity, field) (6/12/2024)
=================
*/
static void PF_findfield(void)
{
	edict_t* ed = G_EDICT(OFS_PARM0);
	const char* field = G_STRING(OFS_PARM1);

	eval_t* def = GetEdictFieldValue(ed, field);

	if (def && def->string == G_STRING_MOD(OFS_PARM1))
	{
		G_STRING_MOD(OFS_RETURN) = def->string;
	}
}

static void _PF_RunMultiManager()
{
    edict_t* ed = G_EDICT(OFS_PARM0);

    const char* edict = ED_FindEdictTextBlock(PR_GetString(ed->v.targetname));

    _ED_ParseMultiManager(edict, ed);
}

/*
=================
Missi: PF_findfield

cycles the targetnames listed in a multi_manager entity

multimanager (entity) (6/12/2024)
=================
*/
static void PF_multimanager(void)
{
    _PF_RunMultiManager();

    /*
    while (1)
    {
        edict = g_Common->COM_Parse (edict);

        if (g_Common->com_token[0] == '}')
            break;

        if (!Q_strncmp(edict, "\"targetname\"", 13))
            break;

		char targettofire[MAX_VALUE] = {};
        float time = 0.0f;

        sscanf(edict, "\"%s\"", targettofire);
        targettofire[Q_strlen(targettofire) - 1] = '\0';

        edict = g_Common->COM_ParseStringNewline(edict);

        sscanf(edict, "\"%f\"", &time);

        Con_DPrintf("Firing \"%s\" from multi_manager!\n", targettofire);

		edict_t* ed = ED_FindEdict(targettofire);

        if ((ed) && ed->v.use != 0)
        {
            int old_self = pr_global_struct->self;
            int old_other = pr_global_struct->other;

            pr_global_struct->self = EDICT_TO_PROG(ed);
            pr_global_struct->time = sv->GetServerTime();
            PR_ExecuteProgram (ed->v.use);

            pr_global_struct->self = old_self;
        }
	}
    */
}

/*
=================
Missi: PF_setcontents

mainly used for func_water. sets the contents of a brush model

setcontents (entity, contents) (7/15/2024)
=================
*/
static void PF_setcontents(void)
{
	edict_t* ed = nullptr;

	ed = G_EDICT(OFS_PARM0);

	if (!ed)
		return;

	model_t* mod = Mod_FindName(PR_GetString(ed->v.model));

	mod->leafs->contents = G_FLOAT(OFS_PARM1);
}

/*
=================
Missi: PF_setcontents

mainly used for func_water. sets the contents of a brush model

setcontents (entity, contents) (7/15/2024)
=================
*/
static void PF_precache_sentence(void)
{
	const char* sentence = nullptr;

	sentence = G_STRING(OFS_PARM0);

	if (!sentence)
		return;

	cxxifstream f;
	uintptr_t path_id = 0;
	cxxstring sound = {};

	g_Common->COM_FOpenFile_FStream("sound/sentences.txt", &f, &path_id);

	if (f.bad())
	{
		f.close();
		return;
	}

	char line[256] = {};

	while (f.getline(line, sizeof(line)))
	{
		if (Q_strncmp(line, sentence+1, Q_strlen(sentence) - 1))
			continue;

		sound = line + Q_strlen(sentence);
		sound.append(".wav");
		
		for (int i = 0; i < MAX_SOUNDS; i++)
		{
			if (!sv->GetSoundPrecacheEntry(i))
			{
				sv->SetSoundPrecacheEntry(i, sound.c_str());
				break;
			}
			if (!strcmp(sv->GetSoundPrecacheEntry(i), sound.c_str()))
			{
				break;
			}
		}
	}

	f.close();
}

/*
=================
Missi: PF_setcontents

mainly used for func_water. sets the contents of a brush model

setcontents (entity, contents) (7/15/2024)
=================
*/
static void PF_speak_sentence(void)
{
	edict_t* ed = nullptr;
	const char* sentence = nullptr;

	ed = G_EDICT(OFS_PARM0);
	sentence = G_STRING(OFS_PARM1);

	if (!ed || !sentence)
		return;

	cxxifstream f;
	uintptr_t path_id = 0;

	g_Common->COM_FOpenFile_FStream("sound/sentences.txt", &f, &path_id);

	if (f.bad())
	{
		f.close();
		return;
	}

	char line[256] = {};
	cxxstring test(sentence);
	size_t pos = 0;

	while ((pos = test.find("!")) != cxxstring::npos)
		test.erase(pos, 1);

	while (f.getline(line, sizeof(line)))
	{
		if (Q_strncmp(line, test.c_str(), Q_strlen(sentence) - 1))
			continue;

		cxxstring sound = line + Q_strlen(sentence);
		sound.append(".wav");

		sv->SV_StartSound(ed, 0, sound.c_str(), 255, 1.0f);
	}
}

/*
=================
Missi: PF_adjusttrain

mainly used for func_water. sets the contents of a brush model

adjusttrain(entity, targetname) (7/15/2024)
=================
*/
static void PF_adjusttrain(void)
{
	edict_t* ed = G_EDICT(OFS_PARM0);
	const char* ed2 = G_STRING(OFS_PARM1);

	edict_t* targ = ED_FindEdict(ed2);

	if (!ed || !targ)
		return;

	vec3_t dist = {};
	vec3_t minsfinal = {};
	vec3_t maxsfinal = {};
	vec3_t forward = {}, right = {}, up = {};
	mplane_t mins[4] = {}, maxs[4] = {};

	edict_t* secondpath = ED_FindEdict(PR_GetString(targ->v.target));

	if (!secondpath)
	{
		VectorSubtract(targ->v.origin, ed->v.origin, dist);

		float yaw = atan2(dist[1], dist[0]) * (180 / M_PI);

		float pitch = atan2(dist[2], sqrt(dist[0] * dist[0] + dist[1] * dist[1])) * (180 / M_PI);

		vec3_t finalang = { pitch, yaw, 0.0f };

		// Missi: before copying off the vectors, we have to update mins/maxs, or else the collision will be wonky (7/20/2024)

		vec3_t dest;

		RotatePointAroundVector(dest, finalang, minsfinal, finalang[0]);
		RotatePointAroundVector(dest, finalang, minsfinal, finalang[1]);
		RotatePointAroundVector(dest, finalang, maxsfinal, finalang[0]);
		RotatePointAroundVector(dest, finalang, maxsfinal, finalang[1]);

		SetMinMaxSize(ed, minsfinal, maxsfinal, false);

		VectorCopy(finalang, ed->v.angles);

		eval_t* height = GetEdictFieldValue(ed, "height");

		if (height)
		{
			vec3_t offset = { 0.0f, 0.0f, height->_float };
			vec3_t offset_output;

			VectorAdd(ed->v.origin, offset, offset_output);
			VectorCopy(offset_output, ed->v.origin);
		}
	}
	else
	{
		VectorSubtract(secondpath->v.origin, targ->v.origin, dist);

		Vector3 distVec = dist;
		AngleVectors(ed->v.angles, forward, right, up);

		Vector3 vpn = forward;
		Vector3 vpr = right;
		Vector3 vpu = up;
		
		Vector3 test = distVec.Rotation(dist);
		
		Vector3 vMins = ed->v.mins;
		Vector3 vMaxs = ed->v.maxs;

		// Missi: before copying off the vectors, we have to update mins/maxs, or else the collision will be wonky (7/20/2024)
		vMins.RotateAlongAxis(vec_null, vpr * -1, vec_null, 90.0f);
		vMaxs.RotateAlongAxis(vec_null, vpr, vec_null, 90.0f);

		SetMinMaxSize(ed, vMins.ToVec3_t(), vMaxs.ToVec3_t(), true);

		VectorCopy(test.ToVec3_t(), ed->v.angles);

		eval_t* height = GetEdictFieldValue(ed, "height");

		if (height)
		{
			Vector3 offset = ed->v.origin;
			offset += Vector3(0.0f, 0.0f, height->_float);

			VectorCopy(offset.ToVec3_t(), ed->v.origin);
		}
	}
}

void PF_Fixme (void)
{
	PR_RunError ("unimplemented bulitin");
}

builtin_t pr_builtin[] =
{
PF_Fixme,
PF_makevectors,	// void(entity e)	makevectors 		= #1;
PF_setorigin,	// void(entity e, vector o) setorigin	= #2;
PF_setmodel,	// void(entity e, string m) setmodel	= #3;
PF_setsize,	// void(entity e, vector min, vector max) setsize = #4;
PF_Fixme,	// void(entity e, vector min, vector max) setabssize = #5;
PF_break,	// void() break						= #6;
PF_random,	// float() random						= #7;
PF_sound,	// void(entity e, float chan, string samp) sound = #8;
PF_normalize,	// vector(vector v) normalize			= #9;
PF_error,	// void(string e) error				= #10;
PF_objerror,	// void(string e) objerror				= #11;
PF_vlen,	// float(vector v) vlen				= #12;
PF_vectoyaw,	// float(vector v) vectoyaw		= #13;
PF_Spawn,	// entity() spawn						= #14;
PF_Remove,	// void(entity e) remove				= #15;
PF_traceline,	// float(vector v1, vector v2, float tryents) traceline = #16;
PF_checkclient,	// entity() clientlist					= #17;
PF_Find,	// entity(entity start, .string fld, string match) find = #18;
PF_precache_sound,	// void(string s) precache_sound		= #19;
PF_precache_model,	// void(string s) precache_model		= #20;
PF_stuffcmd,	// void(entity client, string s)stuffcmd = #21;
PF_findradius,	// entity(vector org, float rad) findradius = #22;
PF_bprint,	// void(string s) bprint				= #23;
PF_sprint,	// void(entity client, string s) sprint = #24;
PF_dprint,	// void(string s) dprint				= #25;
PF_ftos,	// void(string s) ftos				= #26;
PF_vtos,	// void(string s) vtos				= #27;
PF_coredump,
PF_traceon,
PF_traceoff,
PF_eprint,	// void(entity e) debug print an entire entity
PF_walkmove, // float(float yaw, float dist) walkmove
PF_Fixme, // float(float yaw, float dist) walkmove
PF_droptofloor,
PF_lightstyle,
PF_rint,
PF_floor,
PF_ceil,
PF_Fixme,
PF_checkbottom,
PF_pointcontents,
PF_Fixme,
PF_fabs,
PF_aim,
PF_cvar,
PF_localcmd,
PF_nextent,
PF_particle,
PF_changeyaw,
PF_Fixme,
PF_vectoangles,

PF_WriteByte,
PF_WriteChar,
PF_WriteShort,
PF_WriteLong,
PF_WriteCoord,
PF_WriteAngle,
PF_WriteString,
PF_WriteEntity,

PF_sin,
PF_cos,
PF_sqrt,
PF_changepitch,
PF_Fixme,
PF_etos,
PF_WaterMove,

PF_MoveToGoal,
PF_precache_file,
PF_makestatic,

PF_changelevel,
PF_Fixme,

PF_cvar_set,
PF_centerprint,

PF_ambientsound,

PF_precache_model,
PF_precache_sound,		// precache_sound2 is different only for qcc
PF_precache_file,

PF_setspawnparms,

//===================
// Missi: BananaQuake .qc stuff
//===================

PF_cxxvectoradd_flt,	// void(string vecname, float elem) CPPVectorAdd
PF_cxxvectoradd_int,	// void(string vecname, float elem) CPPVectorAdd
PF_cxxvectoradd_ent,	// void(string vecname, float elem) CPPVectorAdd
PF_cxxvectoradd_str,	// void(string vecname, float elem) CPPVectorAdd
PF_vec_access_flt,		// float(string vecname, float elem) vec_access_flt
PF_vec_access_int,		// float(string vecname, float elem) vec_access_int
PF_vec_access_ent,		// entity(string vecname, float elem) vec_access_ent
PF_vec_access_str,		// string(string vecname, float elem) vec_access_str
PF_vec_size,			// float(vecname) vec_size

PF_findfield,
PF_multimanager,
PF_setcontents,
PF_drawdebugline,
PF_speak_sentence,
PF_precache_sentence,
PF_adjusttrain
};

builtin_t *pr_builtins = pr_builtin;
int pr_numbuiltins = sizeof(pr_builtin)/sizeof(pr_builtin[0]);

