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
// sv_edict.c -- entity dictionary

#include "quakedef.h"
#include <vector>

dprograms_t		*progs;
dfunction_t* pr_functions;
char			*pr_strings;
int				pr_stringssize;
ddef_t			*pr_fielddefs;
ddef_t			*pr_globaldefs;
dstatement_t	*pr_statements;
globalvars_t	*pr_global_struct;
float*			pr_globals;			// same as pr_global_struct
int				pr_edict_size;	// in bytes
progvector_t*	pr_vectors;

int		pr_maxknownstrings;
int		pr_numknownstrings;
int		pr_numknownvectors;
CQVector<const char*> pr_knownstrings;

CProgVector<void*>* pr_knownvectors;
int	pr_vecsize;

unsigned short		pr_crc;

int		type_size[9] = { 1,sizeof(string_t) / 4,1,3,1,1,sizeof(func_t) / 4,sizeof(void*) / 4, sizeof(CProgVector<void*>) / 4};

ddef_t *ED_FieldAtOfs (int ofs);
bool	ED_ParseEpair (void *base, ddef_t *key, char *s);

cvar_t	nomonsters = {"nomonsters", "0"};
cvar_t	gamecfg = {"gamecfg", "0"};
cvar_t	scratch1 = {"scratch1", "0"};
cvar_t	scratch2 = {"scratch2", "0"};
cvar_t	scratch3 = {"scratch3", "0"};
cvar_t	scratch4 = {"scratch4", "0"};
cvar_t	savedgamecfg = {"savedgamecfg", "0", true};
cvar_t	saved1 = {"saved1", "0", true};
cvar_t	saved2 = {"saved2", "0", true};
cvar_t	saved3 = {"saved3", "0", true};
cvar_t	saved4 = {"saved4", "0", true};

#define	MAX_FIELD_LEN	64
#define GEFV_CACHESIZE	2

typedef struct {
	ddef_t	*pcache;
	char	field[MAX_FIELD_LEN];
} gefv_cache;

static gefv_cache	gefvCache[GEFV_CACHESIZE] = {{NULL, ""}, {NULL, ""}};

#define	PR_STRING_ALLOCSLOTS	256


static void PR_AllocStringSlots()
{

	pr_maxknownstrings += PR_STRING_ALLOCSLOTS;
    Con_PrintColor(TEXT_COLOR_CYAN, "PR_AllocStringSlots: realloc'ing for %d slots\n", pr_maxknownstrings);
	pr_knownstrings.Expand(pr_maxknownstrings); // (const char**)realloc((void*)pr_knownstrings, pr_maxknownstrings * sizeof(char*));
}

int PR_AllocString(int size, char** ptr)
{
	int		i;

	if (!size)
		return 0;
	for (i = 0; i < pr_numknownstrings; i++)
	{
		if (!pr_knownstrings[i])
			break;
	}
	//	if (i >= pr_numknownstrings)
	//	{
	if (i >= pr_maxknownstrings)
		PR_AllocStringSlots();
	pr_numknownstrings++;
	//	}
	pr_knownstrings[i] = g_MemCache->Hunk_AllocName<char>(size, "string");
	if (ptr)
		*ptr = (char*)pr_knownstrings[i];
	return -1 - i;
}

const char* PR_GetString(int num)
{
	if (num >= 0 && num < pr_stringssize)
		return pr_strings + num;

	else if (num < 0 && num >= -pr_numknownstrings)
	{
		if (!pr_knownstrings[-1 - num])
		{
			host->Host_Error("PR_GetString: attempt to get a non-existant string %d\n", num);
			return "";
		}
		return pr_knownstrings[-1 - num];
	}
	else
	{
		host->Host_Error("PR_GetString: invalid string offset %d\n", num);
		return "";
	}

	return NULL;
}

progvector_t* PR_GetCPPVector(int num)
{
	return &pr_vectors[num];
}

int PR_SetEngineString(const char* s)
{
	int		i;

	if (!s)
		return 0;

	if (s >= pr_strings && s <= pr_strings + pr_stringssize - 2)
		return (int)(s - pr_strings);

	for (i = 0; i < pr_numknownstrings; i++)
	{
		if (pr_knownstrings[i] == s)
			return -1 - i;
	}
	// new unknown engine string
	//Con_DPrintf ("PR_SetEngineString: new engine string %p\n", s);
	//	if (i >= pr_numknownstrings)
	//	{
	if (i >= pr_maxknownstrings)
		PR_AllocStringSlots();
	pr_numknownstrings++;
	//	}
	pr_knownstrings[i] = s;
	return -1 - i;
}

/*
=================
ED_ClearEdict

Sets everything to NULL
=================
*/
void ED_ClearEdict (edict_t *e)
{
	memset (&e->v, 0, progs->entityfields * 4);
	e->free = false;
}

/*
=================
ED_Alloc

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t *ED_Alloc ()
{
	int			i;
	edict_t		*e;

	for ( i=svs.maxclients+1 ; i<sv->GetNumEdicts() ; i++)
	{
		e = EDICT_NUM(i);
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (e->free && ( e->freetime < 2 || sv->GetServerTime() - e->freetime > 0.5 ) )
		{
			ED_ClearEdict (e);
			return e;
		}
	}
	
	if (i == MAX_EDICTS)
		Sys_Error ("ED_Alloc: no free edicts");
		
	sv->IncrementEdicts();
	e = EDICT_NUM(i);
	ED_ClearEdict (e);

	return e;
}

/*
=================
ED_Free

Marks the edict as free
FIXME: walk all entities and NULL out references to this entity
=================
*/
void ED_Free (edict_t *ed)
{
	sv->SV_UnlinkEdict (ed);		// unlink from world bsp

	ed->free = true;
	ed->v.model = 0;
	ed->v.takedamage = 0;
	ed->v.modelindex = 0;
	ed->v.colormap = 0;
	ed->v.skin = 0;
	ed->v.frame = 0;
	VectorCopy (vec3_origin, ed->v.origin);
	VectorCopy (vec3_origin, ed->v.angles);
	ed->v.nextthink = -1;
	ed->v.solid = 0;
	
	ed->freetime = sv->GetServerTime();
}

//===========================================================================

/*
============
ED_GlobalAtOfs
============
*/
ddef_t *ED_GlobalAtOfs (int ofs)
{
	ddef_t		*def;
	int			i;
	
	for (i=0 ; i<progs->numglobaldefs ; i++)
	{
		def = &pr_globaldefs[i];
		if (def->ofs == ofs)
			return def;
	}
	return NULL;
}

/*
============
ED_FieldAtOfs
============
*/
ddef_t *ED_FieldAtOfs (int ofs)
{
	ddef_t		*def;
	int			i;
	
	for (i=0 ; i<progs->numfielddefs ; i++)
	{
		def = &pr_fielddefs[i];
		if (def->ofs == ofs)
			return def;
	}
	return NULL;
}

/*
============
ED_FindField
============
*/
ddef_t *ED_FindField (const char *name)
{
	ddef_t		*def;
	int			i;
	
	for (i=0 ; i<progs->numfielddefs ; i++)
	{
		def = &pr_fielddefs[i];
		if (!strcmp(PR_GetString(def->s_name),name) )
			return def;
	}
	return NULL;
}

eval_t* GetEdictFieldValue(edict_t* ed, const char* field)
{
	ddef_t* def = NULL;
	int				i;
	static int		rep = 0;

	for (i = 0; i < GEFV_CACHESIZE; i++)
	{
		if (!strcmp(field, gefvCache[i].field))
		{
			def = gefvCache[i].pcache;
			goto Done;
		}
	}

	def = ED_FindField(field);

	if (strlen(field) < MAX_FIELD_LEN)
	{
		gefvCache[rep].pcache = def;
		Q_strcpy(gefvCache[rep].field, field);
		rep ^= 1;
	}

Done:
	if (!def)
		return NULL;

	return (eval_t*)((char*)&ed->v + def->ofs * 4);
}

edict_t* ED_FindEdict(const char* targetname)
{
	for (int i = 1; i < sv->GetNumEdicts(); i++)
	{
		edict_t* ed = EDICT_NUM(i);

		if (!Q_strcmp(targetname, PR_GetString(ed->v.targetname)))
			return ed;
	}

	return nullptr;
}

const char* ED_FindEdictTextBlock(const char* targetname)
{
	const char* start = nullptr;

    for (const char* edict = sv->GetWorldModel()->entities; edict[0]; edict = g_Common->COM_ParseStringNewline(edict))
	{
        if (!Q_strncmp(edict, "{", 1))
		{
            start = edict+1;
		}

		if (!Q_strncmp(edict, "\"targetname\"", 12))
		{
			char tgtname[128] = {};

			edict = g_Common->COM_ParseStringNewline(edict);

            Q_snscanf(edict, sizeof(edict), "\"%s\"", tgtname);

            tgtname[Q_strlen(tgtname) - 1] = '\0';

			if (!Q_strcmp(targetname, tgtname))
			{
				edict = start;

				return edict;
			}
		}
	}

	return nullptr;
}
/*
============
ED_FindGlobal
============
*/
ddef_t *ED_FindGlobal (const char *name)
{
	ddef_t		*def;
	int			i;
	
	for (i=0 ; i<progs->numglobaldefs ; i++)
	{
		def = &pr_globaldefs[i];
		if (!strcmp(PR_GetString(def->s_name),name) )
			return def;
	}
	return NULL;
}


/*
============
ED_FindFunction
============
*/
dfunction_t *ED_FindFunction (const char *name)
{
	dfunction_t		*func;
	int				i;
	
	for (i=0 ; i<progs->numfunctions ; i++)
	{
		func = &pr_functions[i];
		if (!strcmp(PR_GetString(func->s_name), name))
			return func;
	}
	return NULL;
}

/*
============
ED_FindCPPVector
============
*/
progvector_t* ED_FindCPPVector(const char* name)
{
	int				i;
	progvector_t* vec;

	for (i = 0; i < progs->numvectors; i++)
	{
		vec = &pr_vectors[i];

		if (!strcmp(PR_GetString(vec->name), name))
			return vec;
	}
	return nullptr;
}


/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
const char *PR_ValueString (int type, eval_t *val)
{
	static char	line[256];
	ddef_t		*def;
	dfunction_t	*f;
	
	type &= ~DEF_SAVEGLOBAL;

	switch (type)
	{
	case ev_string:
		snprintf (line, sizeof(line), "%s", PR_GetString(val->string));
		break;
	case ev_entity:	
		snprintf (line, sizeof(line), "entity %i", NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)) );
		break;
	case ev_function:
		f = pr_functions + val->function;
		snprintf(line, sizeof(line), "%s()", PR_GetString(f->s_name));
		break;
	case ev_field:
		def = ED_FieldAtOfs ( val->_int );
		snprintf(line, sizeof(line), ".%s", PR_GetString(def->s_name));
		break;
	case ev_void:
		snprintf (line, sizeof(line), "void");
		break;
	case ev_float:
		snprintf (line, sizeof(line), "%5.1f", val->_float);
		break;
	case ev_vector:
		snprintf (line, sizeof(line), "'%5.1f %5.1f %5.1f'", val->vector[0], val->vector[1], val->vector[2]);
		break;
	case ev_pointer:
		snprintf (line, sizeof(line), "pointer");
		break;
	default:
		snprintf (line, sizeof(line), "bad type %i", type);
		break;
	}
	
	return line;
}

/*
============
PR_UglyValueString

Returns a string describing *data in a type specific manner
Easier to parse than PR_ValueString
=============
*/
const char *PR_UglyValueString (int type, eval_t *val)
{
	static char	line[1024];
	ddef_t* def;
	dfunction_t* f;

	type &= ~DEF_SAVEGLOBAL;

	switch (type)
	{
	case ev_string:
		snprintf(line, sizeof(line), "%s", PR_GetString(val->string));
		break;
	case ev_entity:
		snprintf(line, sizeof(line), "%i", NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)));
		break;
	case ev_function:
		f = pr_functions + val->function;
		snprintf(line, sizeof(line), "%s", PR_GetString(f->s_name));
		break;
	case ev_field:
		def = ED_FieldAtOfs(val->_int);
		snprintf(line, sizeof(line), "%s", PR_GetString(def->s_name));
		break;
	case ev_void:
		snprintf(line, sizeof(line), "void");
		break;
	case ev_float:
		snprintf(line, sizeof(line), "%f", val->_float);
		break;
	case ev_vector:
		snprintf(line, sizeof(line), "%f %f %f", val->vector[0], val->vector[1], val->vector[2]);
		break;
	default:
		snprintf(line, sizeof(line), "bad type %i", type);
		break;
	}

	return line;
}

/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
const char *PR_GlobalString (int ofs)
{
	static char	line[512];
	const char* s;
	int		i;
	ddef_t* def;
	void* val;

	val = (void*)&pr_globals[ofs];
	def = ED_GlobalAtOfs(ofs);
	if (!def)
		snprintf(line, sizeof(line), "%i(?)", ofs);
	else
	{
		s = PR_ValueString(def->type, (eval_t*)val);
		snprintf(line, sizeof(line), "%i(%s)%s", ofs, PR_GetString(def->s_name), s);
	}

	i = strlen(line);
	for (; i < 20; i++)
        Q_strcat(line, " ");
    Q_strcat(line, " ");

	return line;
}

const char *PR_GlobalStringNoContents (int ofs)
{
	int		i;
	ddef_t	*def;
	static char	line[128];
	
	def = ED_GlobalAtOfs(ofs);
	if (!def)
        snprintf (line, sizeof(line), "%i(?)", ofs);    // Missi: -Wtrigraphs fix (6/19/2024)
	else
		snprintf (line, sizeof(line), "%i(%s)", ofs, PR_GetString(def->s_name));
	
	i = strlen(line);
	for ( ; i<20 ; i++)
        Q_strcat (line," ");
    Q_strcat (line," ");
		
	return line;
}


/*
=============
ED_Print

For debugging
=============
*/
void ED_Print (edict_t *ed)
{
	int		l;
	ddef_t	*d;
	int		*v;
	int		i, j;
	const char	*name;
	int		type;

	if (ed->free)
	{
		Con_Printf ("FREE\n");
		return;
	}

	Con_Printf("\nEDICT %i:\n", NUM_FOR_EDICT(ed));
	for (i=1 ; i<progs->numfielddefs ; i++)
	{
		d = &pr_fielddefs[i];
		name = PR_GetString(d->s_name);
		if (name[strlen(name)-2] == '_')
			continue;	// skip _x, _y, _z vars
			
		v = (int *)((char *)&ed->v + d->ofs*4);

	// if the value is still all 0, skip the field
		type = d->type & ~DEF_SAVEGLOBAL;
		
		for (j=0 ; j<type_size[type] ; j++)
			if (v[j])
				break;
		if (j == type_size[type])
			continue;
	
		Con_Printf ("%s",name);
		l = strlen (name);
		while (l++ < 15)
			Con_Printf (" ");

		Con_Printf ("%s\n", PR_ValueString(static_cast<etype_t>(d->type), (eval_t *)v));
	}
}

/*
=============
ED_Write

For savegames
=============
*/
void ED_Write (cxxofstream *f, edict_t *ed)
{
	ddef_t	*d;
	int		*v;
	int		i, j;
	const char	*name;
	int		type;

	f->write("{\n", 2);

	if (ed->free)
	{
		f->write("}\n", 2);
		return;
	}
	
	for (i=1 ; i<progs->numfielddefs ; i++)
	{
		d = &pr_fielddefs[i];
		name = PR_GetString(d->s_name);
		if (name[strlen(name)-2] == '_')
			continue;	// skip _x, _y, _z vars
			
		v = (int *)((char *)&ed->v + d->ofs*4);

	// if the value is still all 0, skip the field
		type = d->type & ~DEF_SAVEGLOBAL;
		for (j=0 ; j<type_size[type] ; j++)
			if (v[j])
				break;
		if (j == type_size[type])
			continue;
	
		const char* str = PR_UglyValueString(static_cast<etype_t>(d->type), (eval_t*)v);

		f->write("\"", 1);
		f->write(name, Q_strlen(name));
		f->write("\"", 1);
		f->write(" ", 1);
		f->write("\"", 1);
		f->write(str, Q_strlen(str));
		f->write("\"", 1);
		f->write("\n", 1);
	}

	f->write("}\n", 2);
}

void ED_PrintNum (int ent)
{
	ED_Print (EDICT_NUM(ent));
}

/*
=============
ED_PrintEdicts

For debugging, prints all the entities in the current server
=============
*/
void ED_PrintEdicts ()
{
	int		i;
	
	Con_Printf ("%i entities\n", sv->GetNumEdicts());
	for (i=0 ; i<sv->GetNumEdicts() ; i++)
		ED_PrintNum (i);
}

/*
=============
ED_PrintEdict_f

For debugging, prints a single edict
=============
*/
void ED_PrintEdict_f ()
{
	int		i;
	
	i = Q_atoi (g_pCmds->Cmd_Argv(1));
	if (i >= sv->GetNumEdicts())
	{
		Con_Printf("Bad edict number\n");
		return;
	}
	ED_PrintNum (i);
}

/*
=============
ED_Count

For debugging
=============
*/
void ED_Count ()
{
	int		i;
	edict_t	*ent;
	int		active, models, solid, step;

	active = models = solid = step = 0;
	for (i=0 ; i<sv->GetNumEdicts() ; i++)
	{
		ent = EDICT_NUM(i);
		if (ent->free)
			continue;
		active++;
		if (ent->v.solid)
			solid++;
		if (ent->v.model)
			models++;
		if (ent->v.movetype == MOVETYPE_STEP)
			step++;
	}

	Con_Printf ("num_edicts:%3i\n", sv->GetNumEdicts());
	Con_Printf ("active    :%3i\n", active);
	Con_Printf ("view      :%3i\n", models);
	Con_Printf ("touch     :%3i\n", solid);
	Con_Printf ("step      :%3i\n", step);

}

/*
==============================================================================

					ARCHIVING GLOBALS

FIXME: need to tag constants, doesn't really work
==============================================================================
*/

/*
=============
ED_WriteGlobals
=============
*/
void ED_WriteGlobals (cxxofstream *f)
{
	ddef_t		*def;
	int			i;
	const char		*name;
	int			type;

	f->write("{\n", 2);
	for (i=0 ; i<progs->numglobaldefs ; i++)
	{
		def = &pr_globaldefs[i];
		type = def->type;
		if ( !(def->type & DEF_SAVEGLOBAL) )
			continue;
		type &= ~DEF_SAVEGLOBAL;

		if (type != ev_string
		&& type != ev_float
		&& type != ev_entity)
			continue;

		name = PR_GetString(def->s_name);

		const char* str = PR_UglyValueString(static_cast<etype_t>(type), (eval_t*)&pr_globals[def->ofs]);

		f->write("\"", 1);
		f->write(name, Q_strlen(name));
		f->write("\"", 1);
		f->write(" ", 1);
		f->write("\"", 1);
		f->write(str, Q_strlen(str));
		f->write("\"", 1);
		f->write("\n", 1);
	}
	f->write("}\n", 2);
}

/*
=============
ED_ParseGlobals
=============
*/
const char* ED_ParseGlobals (const char *data)
{
	char	keyname[64];
	ddef_t	*key;

	while (1)
	{	
	// parse key
		data = g_Common->COM_Parse (data);
		if (g_Common->com_token[0] == '}')
			break;
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");

		Q_strcpy (keyname, g_Common->com_token);

	// parse value	
		data = g_Common->COM_Parse (data);
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");

		if (g_Common->com_token[0] == '}')
			Sys_Error ("ED_ParseEntity: closing brace without data");

		key = ED_FindGlobal (keyname);
		if (!key)
		{
			Con_Printf ("'%s' is not a global\n", keyname);
			continue;
		}

		if (!ED_ParseEpair ((void *)pr_globals, key, g_Common->com_token))
			host->Host_Error ("ED_ParseGlobals: parse error");
	}

    return data;
}

//============================================================================


/*
=============
ED_NewString
=============
*/
string_t ED_NewString (char *string)
{
	char* new_p;
	int		i, l;
	string_t	num;

	l = strlen(string) + 1;
	num = PR_AllocString(l, &new_p);

	for (i = 0; i < l; i++)
	{
		if (string[i] == '\\' && i < l - 1)
		{
			i++;
			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}

	return num;
}


/*
=============
ED_ParseEval

Can parse either fields or globals
returns false if error
=============
*/
bool	ED_ParseEpair (void *base, ddef_t *key, char *s)
{
	int		i;
	char	string[128];
	ddef_t	*def;
	char	*v, *w;
	void	*d;
	dfunction_t	*func;
	progvector_t*	vec;
	
	d = (void *)((int *)base + key->ofs);
	
	switch (key->type & ~DEF_SAVEGLOBAL)
	{
	case ev_string:
		*(string_t *)d = ED_NewString (s);
		break;
		
	case ev_float:
		*(float *)d = atof (s);
		break;
		
	case ev_vector:
		Q_strcpy (string, s);
		v = string;
		w = string;
		for (i=0 ; i<3 ; i++)
		{
			while (*v && *v != ' ')
				v++;
			*v = 0;
			((float *)d)[i] = atof (w);
			w = v = v+1;
		}
		break;
		
	case ev_entity:
		*(int *)d = EDICT_TO_PROG(EDICT_NUM(atoi (s)));
		break;
		
	case ev_field:
		def = ED_FindField (s);
		if (!def)
		{
			Con_Printf ("Can't find field %s\n", s);
			return false;
		}
		*(int *)d = G_INT(def->ofs);
		break;
	
	case ev_function:
		func = ED_FindFunction (s);
		if (!func)
		{
			Con_Printf ("Can't find function %s\n", s);
			return false;
		}
		*(func_t *)d = func - pr_functions;
		break;

	case ev_cppvector:
		vec = ED_FindCPPVector(s);
		if (!vec)
		{
			Con_Printf("Can't find CXX Vector %s\n", s);
			return false;
		}
		break;
		
	default:
		break;
	}
	return true;
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
Used for initial level load and for savegames.
====================
*/
const char *ED_ParseEdict (const char *data, edict_t *ent)
{
	ddef_t		*key;
	bool	anglehack;
	bool	init;
	char		keyname[256];
	int			n;

	init = false;

// clear it
	if (ent != sv->GetServerEdicts())	// hack
		memset (&ent->v, 0, progs->entityfields * 4);

// go through all the dictionary pairs
	while (1)
	{	
	// parse key
		data = g_Common->COM_Parse (data);
		if (g_Common->com_token[0] == '}')
			break;
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");
		
		// anglehack is to allow QuakeEd to write single scalar angles
		// and allow them to be turned into vectors. (FIXME...)
		if (!strcmp(g_Common->com_token, "angle"))
		{
			Q_strcpy (g_Common->com_token, "angles");
			anglehack = true;
		}
		else
			anglehack = false;

		// FIXME: change light to _light to get rid of this hack
		if (!strcmp(g_Common->com_token, "light"))
			Q_strcpy (g_Common->com_token, "light_lev");	// hack for single light def

		Q_strcpy (keyname, g_Common->com_token);

		// another hack to fix heynames with trailing spaces
		n = strlen(keyname);
		while (n && keyname[n-1] == ' ')
		{
			keyname[n-1] = 0;
			n--;
		}

	// parse value	
		data = g_Common->COM_Parse (data);
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");

		if (g_Common->com_token[0] == '}')
			Sys_Error ("ED_ParseEntity: closing brace without data");

		init = true;

		if (!Q_strncmp(g_Common->com_token, "func_water", 10))
		{
			vec3_t offset = {};

			hull_t* hull = sv->SV_HullForEntity(ent, ent->v.mins, ent->v.maxs, offset);
			mclipnode_t* node = nullptr;
			mplane_t* plane = nullptr;
			float d = 0.0f;
			int hullnum = 0;

			while (hullnum >= 0)
			{
				if (hullnum < hull->firstclipnode || hullnum > hull->lastclipnode)
					Sys_Error("SV_HullPointContents: bad node number");

				node = hull->clipnodes + hullnum;
				plane = hull->planes + node->planenum;

				if (plane->type < 3)
					d = offset[plane->type] - plane->dist;
				else
					d = DotProduct(plane->normal, offset) - plane->dist;
				if (d < 0)
				{
					node->children[1] = CONTENTS_WATER;
					hullnum = node->children[1];
				}
				else
				{
					node->children[0] = CONTENTS_WATER;
					hullnum = node->children[0];
				}

				model_t* model = Mod_FindName(PR_GetString(ent->v.model));

				if (model)
				{
					for (int i = 0; i < model->numleafs; i++)
					{
						mleaf_t* leaf = &model->leafs[i];
						leaf->contents = CONTENTS_WATER;
					}
				}
			}

			Con_DPrintf("Hull contents: %d\n", hullnum);
		}

		// keynames with a leading underscore are used for utility comments,
		// and are immediately discarded by quake
		if (keyname[0] == '_')
			continue;
		
		key = ED_FindField (keyname);
		if (!key)
		{
			Con_Printf ("'%s' is not a field\n", keyname);
			continue;
		}

		if (anglehack)
		{
			char	temp[32];
			Q_strcpy (temp, g_Common->com_token);
			snprintf (g_Common->com_token, sizeof(g_Common->com_token), "0 %s 0", temp);
		}

		if (!ED_ParseEpair ((void *)&ent->v, key, g_Common->com_token))
			host->Host_Error ("ED_ParseEdict: parse error");
	}

	if (!init)
		ent->free = true;

	return data;
}

unsigned int ProgTimerCallback(unsigned int interval, void* data)
{
    edict_t* ed = (edict_t*)data;

	if (!ed)
		return 0;

    const char* targetname = PR_GetString(ed->v.targetname);
    Con_DPrintf("Firing \"%s\" from multi_manager\n", PR_GetString(ed->v.targetname));

    int old_self = pr_global_struct->self;
    int old_other = pr_global_struct->other;

    pr_global_struct->self = EDICT_TO_PROG(ed);
    pr_global_struct->time = sv->GetServerTime();
    PR_ExecuteProgram (ed->v.use);

    pr_global_struct->self = old_self;

    return 0;
}

void TimerCallback(void* parm1, void* parm2, void* parm3)
{
	edict_t* ed = (edict_t*)parm1;

	if (!ed)
		return;

	const char* targetname = PR_GetString(ed->v.targetname);
	Con_DPrintf("Firing \"%s\" from multi_manager\n", PR_GetString(ed->v.targetname));

	int old_self = pr_global_struct->self;
	int old_other = pr_global_struct->other;

	pr_global_struct->self = EDICT_TO_PROG(ed);
	pr_global_struct->time = sv->GetServerTime();
	PR_ExecuteProgram(ed->v.use);

	pr_global_struct->self = old_self;
}

/*
====================
Missi: _ED_ParseMultiManager

Basically a modified version of ED_ParseEdict without intialization of edicts. Used to parse targetnames
listed in a GoldSrc multi_manager entity. Operates on map entity data (7/14/2024)
====================
*/

const char* _ED_ParseMultiManager (const char *data)
{
    bool        anglehack;
    char		keyname[256];
    int			n;

    // go through all the dictionary pairs
    while (1)
    {
    // parse key
        data = g_Common->COM_Parse (data);
        if (g_Common->com_token[0] == '}')
            break;
        if (!data)
            Sys_Error ("ED_ParseEntity: EOF without closing brace");

        // anglehack is to allow QuakeEd to write single scalar angles
        // and allow them to be turned into vectors. (FIXME...)
        if (!strcmp(g_Common->com_token, "angle"))
        {
            Q_strcpy (g_Common->com_token, "angles");
            anglehack = true;
        }
        else
            anglehack = false;

        // FIXME: change light to _light to get rid of this hack
        if (!strcmp(g_Common->com_token, "light"))
            Q_strcpy (g_Common->com_token, "light_lev");	// hack for single light def

        Q_strcpy (keyname, g_Common->com_token);

        // another hack to fix heynames with trailing spaces
        n = strlen(keyname);
        while (n && keyname[n-1] == ' ')
        {
            keyname[n-1] = 0;
            n--;
        }

        if (!Q_strncmp(g_Common->com_token, "origin", 6) || 
			!Q_strncmp(g_Common->com_token, "classname", 9) ||
			!Q_strncmp(g_Common->com_token, "targetname", 10))
        {
            data = g_Common->COM_Parse (data);
            continue;
        }

        cxxstring targetname(g_Common->com_token);

        // parse value
        data = g_Common->COM_Parse (data);
        if (!data)
            Sys_Error ("ED_ParseEntity: EOF without closing brace");

        if (g_Common->com_token[0] == '}')
            Sys_Error ("ED_ParseEntity: closing brace without data");

        if (!targetname.empty())
        {
            size_t pos = targetname.find_first_of('\"');

            while (pos != cxxstring::npos)
            {
                targetname = targetname.erase(pos, 1);
                pos = targetname.find_first_of('\"');
            }

			pos = targetname.find_first_of('#');

			while (pos != cxxstring::npos)
			{
				targetname = targetname.erase(pos, 2);
				pos = targetname.find_first_of('#');
			}

            edict_t* find = ED_FindEdict(targetname.c_str());

			if (!find)
				continue;

            int time = Q_atoi(g_Common->com_token);

			CQTimer* timer = new CQTimer(time, TimerCallback, find, nullptr, nullptr);
        }

        // keynames with a leading underscore are used for utility comments,
        // and are immediately discarded by quake
        if (keyname[0] == '_')
            continue;
    }

    return data;
}

/*
================
ED_LoadFromFile

The entities are directly placed in the array, rather than allocated with
ED_Alloc, because otherwise an error loading the map would have entity
number references out of order.

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.

Used for both fresh maps and savegame loads.  A fresh map would also need
to call ED_CallSpawnFunctions () to let the objects initialize themselves.
================
*/
void ED_LoadFromFile (const char *data)
{	
	edict_t		*ent = NULL;
	int			inhibit = 0;
	dfunction_t	*func = NULL;

	pr_global_struct->time = sv->GetServerTime();
	
// parse ents
	while (1)
	{
// parse the opening brace	
		data = g_Common->COM_Parse (data);
		if (!data)
			break;
		if (g_Common->com_token[0] != '{')
			Sys_Error ("ED_LoadFromFile: found %s when expecting {", g_Common->com_token);

		if (!ent)
			ent = EDICT_NUM(0);
		else
			ent = ED_Alloc ();
		data = ED_ParseEdict (data, ent);

// remove things from different skill levels or deathmatch
		if (host->deathmatch.value)
		{
			if (((int)ent->v.spawnflags & SPAWNFLAG_NOT_DEATHMATCH))
			{
				ED_Free (ent);	
				inhibit++;
				continue;
			}
		}
		else if ((host->current_skill == 0 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_EASY))
				|| (host->current_skill == 1 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_MEDIUM))
				|| (host->current_skill >= 2 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_HARD)) )
		{
			ED_Free (ent);	
			inhibit++;
			continue;
		}

//
// immediately call spawn function
//
		if (!ent->v.classname)
		{
			Con_Printf ("No classname for:\n");
			ED_Print (ent);
			ED_Free (ent);
			continue;
		}

	// look for the spawn function
		func = ED_FindFunction ( PR_GetString(ent->v.classname));

		if (!func)
		{
			Con_Printf ("No spawn function for:\n");
			ED_Print (ent);
			ED_Free (ent);
			continue;
		}

		pr_global_struct->self = EDICT_TO_PROG(ent);
		PR_ExecuteProgram (func - pr_functions);
	}	

	Con_DPrintf ("%i entities inhibited\n", inhibit);
}

/*
===============
PR_LoadProgs
===============
*/
void PR_LoadProgs ()
{
	int		i;
	uintptr_t	path_id;

// flush the non-C variable lookup cache
	for (i=0 ; i<GEFV_CACHESIZE ; i++)
		gefvCache[i].field[0] = 0;

	g_CRCManager->CRC_Init (&pr_crc);

	progs = COM_LoadHunkFile<dprograms_t>("progs.dat", &path_id);
	
	if (!progs)
		Sys_Error ("PR_LoadProgs: couldn't load progs.dat");
	Con_DPrintf ("Programs occupy %iK.\n", g_Common->com_filesize/1024);

	for (i=0 ; i< g_Common->com_filesize ; i++)
		g_CRCManager->CRC_ProcessByte (&pr_crc, ((byte *)progs)[i]);

// byte swap the header
	for (i=0 ; i<sizeof(*progs)/4 ; i++)
		((int *)progs)[i] = LittleLong ( ((int *)progs)[i] );		

	if (progs->version != PROG_VERSION)
		Sys_Error ("progs.dat has wrong version number (%i should be %i)", progs->version, PROG_VERSION);
	if (progs->crc != PROGHEADER_CRC)
		Sys_Error ("progs.dat system vars have been modified, progdefs.h is out of date");

	pr_functions = (dfunction_t *)((byte *)progs + progs->ofs_functions);
	pr_strings = (char *)progs + progs->ofs_strings;
	
	if (progs->ofs_strings + progs->numstrings >= g_Common->com_filesize)
		host->Host_Error("progs.dat strings go past end of file\n");

	// initialize the strings
	pr_numknownstrings = 0;
	pr_maxknownstrings = 0;
	pr_stringssize = progs->numstrings;
    if (pr_knownstrings.GetNumAllocated() > 0)
        pr_knownstrings.Clear();
	PR_SetEngineString("");

	pr_globaldefs = (ddef_t *)((byte *)progs + progs->ofs_globaldefs);
	pr_fielddefs = (ddef_t *)((byte *)progs + progs->ofs_fielddefs);
	pr_statements = (dstatement_t *)((byte *)progs + progs->ofs_statements);

	pr_global_struct = (globalvars_t *)((byte *)progs + progs->ofs_globals);
	pr_globals = (float *)pr_global_struct;

	// Missi: To maintain compatibility with the old progs.dat, we check if
	// progs->numvectors or progs->ofs_vectors returns with an oddly-high number, 
	// usually past MAX_PROG_VECTORS.
	//
	// This is not the most clean or reliable solution, and may be platform-dependent.
	// But it works for now. (6/12/2024)
	if (progs->numvectors < MAX_PROG_VECTORS || progs->ofs_vectors >= (MAX_PROG_VECTORS * MAX_PROG_VECTOR_ALLOC))
	{
		pr_vectors = (progvector_t*)((byte*)progs + progs->ofs_vectors);
	}
	else
	{
		progs->ofs_vectors = 0;
		progs->numvectors = 0;
	}

	//pr_edict_size = progs->entityfields * 4 + sizeof (edict_t) - sizeof(entvars_t);

// byte swap the lumps
	for (i=0 ; i<progs->numstatements ; i++)
	{
		pr_statements[i].op = LittleShort(pr_statements[i].op);
		pr_statements[i].a = LittleShort(pr_statements[i].a);
		pr_statements[i].b = LittleShort(pr_statements[i].b);
		pr_statements[i].c = LittleShort(pr_statements[i].c);
	}

	for (i=0 ; i<progs->numfunctions; i++)
	{
	pr_functions[i].first_statement = LittleLong (pr_functions[i].first_statement);
	pr_functions[i].parm_start = LittleLong (pr_functions[i].parm_start);
	pr_functions[i].s_name = LittleLong (pr_functions[i].s_name);
	pr_functions[i].s_file = LittleLong (pr_functions[i].s_file);
	pr_functions[i].numparms = LittleLong (pr_functions[i].numparms);
	pr_functions[i].locals = LittleLong (pr_functions[i].locals);
	}	

	for (i=0 ; i<progs->numglobaldefs ; i++)
	{
		pr_globaldefs[i].type = LittleShort (pr_globaldefs[i].type);
		pr_globaldefs[i].ofs = LittleShort (pr_globaldefs[i].ofs);
		pr_globaldefs[i].s_name = LittleLong (pr_globaldefs[i].s_name);
	}

	for (i=0 ; i<progs->numfielddefs ; i++)
	{
		pr_fielddefs[i].type = LittleShort (pr_fielddefs[i].type);
		if (pr_fielddefs[i].type & DEF_SAVEGLOBAL)
			Sys_Error ("PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");
		pr_fielddefs[i].ofs = LittleShort (pr_fielddefs[i].ofs);
		pr_fielddefs[i].s_name = LittleLong (pr_fielddefs[i].s_name);
	}
	if (progs->ofs_vectors > 0)
	{
		for (i = 0; i < progs->numvectors; i++)
		{
			pr_vectors[i].type = LittleShort(pr_vectors[i].type);
			if (pr_vectors[i].type & DEF_SAVEGLOBAL)
				Sys_Error("PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");
			pr_vectors[i].ofs = (LittleShort(pr_vectors[i].ofs));
			pr_vectors[i].name = (LittleLong(pr_vectors[i].name));
			pr_vectors[i].data = new CProgVector<void*>;
		}
	}

	pr_edict_size = progs->entityfields * 4 + sizeof(edict_t) - sizeof(entvars_t);
	// round off to next highest whole word address (esp for Alpha)
	// this ensures that pointers in the engine data area are always
	// properly aligned
	pr_edict_size += sizeof(void*) - 1;
	pr_edict_size &= ~(sizeof(void*) - 1);

	for (i=0 ; i<progs->numglobals ; i++)
		((int *)pr_globals)[i] = LittleLong (((int *)pr_globals)[i]);
}

/*
===============
ED_Entfire_f

Missi: ent_fire command similar to the one in Source, but different
due to how Quake interprets targets (7/5/2024)
===============
*/

static void ED_EntFire_f()
{
    if (g_pCmds->Cmd_Argc() < 2)
    {
        Con_Printf("usage: ent_fire <targetname> <targetname2> <targetname3>...\n");
        return;
    }

    const char* targetname = nullptr;

    for (int i = 1; i < g_pCmds->Cmd_Argc(); i++)
    {
        for (int j = 1; j < sv->GetNumEdicts(); j++)    // Missi: skip worldspawn (7/5/2024)
        {
            edict_t* ed = EDICT_NUM(j);

            if (!ed)
                break;

            string_t tgtname = ed->v.targetname;

            targetname = PR_GetString(tgtname);

            if (!Q_strcmp(targetname, g_pCmds->Cmd_Argv(i)))
            {
                if (!ed->v.use)
                    break;

                int old_self = pr_global_struct->self;
                int old_other = pr_global_struct->other;

                pr_global_struct->self = EDICT_TO_PROG(ed);
                pr_global_struct->time = sv->GetServerTime();
                PR_ExecuteProgram (ed->v.use);

                pr_global_struct->self = old_self;

                //PR_ExecuteProgram(ed->v.use);     // Missi: execute the "use" function
                Con_Printf("Firing entity \"%s\"\n", targetname);
            }
        }

        if (!targetname)
        {
            Con_Printf("No entity named %s found", g_pCmds->Cmd_Argv(i));
            break;
        }
    }
}

/*
===============
PR_Init
===============
*/
void PR_Init ()
{
	g_pCmds->Cmd_AddCommand ("edict", ED_PrintEdict_f);
	g_pCmds->Cmd_AddCommand ("edicts", ED_PrintEdicts);
	g_pCmds->Cmd_AddCommand ("edictcount", ED_Count);
	g_pCmds->Cmd_AddCommand ("profile", PR_Profile_f);
    g_pCmds->Cmd_AddCommand ("ent_fire", ED_EntFire_f, CVAR_CHEAT);
	Cvar_RegisterVariable (&nomonsters);
	Cvar_RegisterVariable (&gamecfg);
	Cvar_RegisterVariable (&scratch1);
	Cvar_RegisterVariable (&scratch2);
	Cvar_RegisterVariable (&scratch3);
	Cvar_RegisterVariable (&scratch4);
	Cvar_RegisterVariable (&savedgamecfg);
	Cvar_RegisterVariable (&saved1);
	Cvar_RegisterVariable (&saved2);
	Cvar_RegisterVariable (&saved3);
	Cvar_RegisterVariable (&saved4);
}



edict_t *EDICT_NUM(int n)
{
	if (n < 0 || n >= sv->GetMaxEdicts())
		Sys_Error ("EDICT_NUM: bad number %i", n);
	return (edict_t *)((byte *)sv->GetServerEdicts() + (n)*pr_edict_size);
}

int NUM_FOR_EDICT(edict_t *e)
{
	int		b;
	
	b = (byte *)e - (byte *)sv->GetServerEdicts();
	b = b / pr_edict_size;
	
	if (b < 0 || b >= sv->GetNumEdicts())
		Sys_Error ("NUM_FOR_EDICT: bad pointer");
	return b;
}
