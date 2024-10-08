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
// cvar.c -- dynamic variable tracking

#include "quakedef.h"

cvar_t	*cvar_vars;
char	cvar_null_string[] = "";

/*
============
Cvar_FindVar
============
*/
cvar_t *Cvar_FindVar (const char *var_name)
{
	cvar_t	*var = NULL;
	
	for (var=cvar_vars ; var ; var=var->next)
		if (!Q_strcmp (var_name, var->name))
			return var;

	return NULL;
}

/*
============
Cvar_VariableValue
============
*/
float	Cvar_VariableValue (const char *var_name)
{
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return Q_atof (var->string);
}


/*
============
Cvar_VariableString
============
*/
const char *Cvar_VariableString (const char *var_name)
{
	cvar_t *var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return cvar_null_string;
	return var->string;
}


/*
============
Cvar_CompleteVariable
============
*/
const char *Cvar_CompleteVariable (const char *partial)
{
	cvar_t		*cvar;
	int			len;
	
	len = Q_strlen(partial);
	
	if (!len)
		return NULL;
		
// check functions
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
		if (!Q_strncmp (partial,cvar->name, len))
			return cvar->name;

	return NULL;
}


/*
============
Cvar_Set
============
*/
void Cvar_Set (const char *var_name, const char *value, bool progs)
{
	cvar_t	*var;
	bool changed;
	
	var = Cvar_FindVar (var_name);
	if (!var)
	{	// there is an error in C code if this happens
		Con_Printf ("Cvar_Set: variable %s not found\n", var_name);
		return;
    }

	changed = Q_strcmp(var->string, value);

	if (var->flags & CVAR_SERVER)
		var->src = cvar_source::SRC_CVAR_SERVER;
	else if (progs)
		var->src = cvar_source::SRC_CVAR_PROGS;
	else
		var->src = cvar_source::SRC_CVAR_CLIENT;

	if ((var->flags & CVAR_READONLY) && var->src != cvar_source::SRC_CVAR_PROGS)
	{
		Con_Warning("Player %s attempted to change read-only cvar %s!\n", cl_name.string, var->name);
	}

    if ((var->flags & CVAR_CHEAT) && sv_cheats.value == 0)
    {
        Con_Printf("Player %s attempted to set cheat value on %s without sv_cheats enabled!\n", cl_name.string, var->name);
        return;
    }

	var->string = "";	// free the old value string
	
	char str[256];
	memset(str, 0, sizeof(str));

	Q_strncpy (str, value, sizeof(str));
#ifdef _WIN32
	var->string = _strdup(str);
#else
    var->string = strdup(str);
#endif
	var->value = Q_atof (str);
	if ((var->flags & CVAR_NOTIFY) && changed)
	{
        if (sv->IsServerActive())
			sv->SV_BroadcastPrintf ("\"%s\" changed to \"%s\"\n", var->name, var->string);
	}
}

/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue (const char *var_name, float value)
{
	char	val[32];
	
	snprintf (val, sizeof(val), "%4.8f", value);
	Cvar_Set (var_name, val);
}


/*
============
Cvar_RegisterVariable

Adds a freestanding variable to the variable list.
============
*/
void Cvar_RegisterVariable (cvar_t *variable)
{
	const char	*oldstr;

// first check to see if it has allready been defined
	if (Cvar_FindVar (variable->name))
	{
		Con_Printf ("Can't register variable %s, allready defined\n", variable->name);
		return;
	}
	
// check for overlap with a command
	if (g_pCmds->Cmd_Exists(variable->name))
	{
		Con_Printf ("Cvar_RegisterVariable: %s is a command\n", variable->name);
		return;
	}
		
// copy the value off, because future sets will Z_Free it
#if 0
	oldstr = variable->string;
	variable->string = mainzone->Z_Malloc<char>(Q_strlen(variable->string)+1);
	Q_strcpy (variable->string, oldstr);
	variable->value = Q_atof (variable->string);
#endif

	oldstr = variable->string;
	//variable->string = mainzone->Z_Malloc<const char>(Q_strlen(variable->string) + 1);
#ifdef _WIN32
	variable->string = _strdup(oldstr);
#else
    variable->string = strdup(oldstr);
#endif
	variable->value = Q_atof(variable->string);

// link the variable in
	variable->next = cvar_vars;
	cvar_vars = variable;
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
bool	Cvar_Command ()
{
	cvar_t			*v;

// check variables
	v = Cvar_FindVar (g_pCmds->Cmd_Argv(0));
	if (!v)
		return false;
		
// perform a variable print or set
	if (g_pCmds->Cmd_Argc() == 1)
	{
		Con_Printf ("\"%s\" is \"%s\"\n", v->name, v->string);
		return true;
	}

	Cvar_Set (v->name, g_pCmds->Cmd_Argv(1));
	return true;
}


/*
============
Cvar_WriteVariables

Writes lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (cxxofstream& f)
{
	cvar_t	*var;
	
	for (var = cvar_vars ; var ; var = var->next)
    {
		if (var->archive)
        {
            char output[256] = {};
            snprintf(output, sizeof(output), "%s \"%s\"\n", var->name, var->string);

            f.write(output, Q_strlen(output));
        }
    }
}

