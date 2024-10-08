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
// cmd.c -- Quake script command processing module

#include "quakedef.h"

#define	MAX_ALIAS_NAME	32

typedef struct cmdalias_s
{
	struct cmdalias_s	*next;
	char	name[MAX_ALIAS_NAME];
	char	*value;
} cmdalias_t;

cmdalias_t	*cmd_alias;

int		trashtest;
int		*trashspot;

bool	cmd_wait;

CCommandBuffer* g_pCmdBuf = nullptr;
CCommand* g_pCmds = nullptr;

//=============================================================================

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
void CCommand::Cmd_Wait_f ()
{
	cmd_wait = true;
}

/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

sizebuf_t	cmd_text;

/*
============
Cbuf_Init
============
*/
void CCommandBuffer::Cbuf_Init ()
{
	SZ_Alloc (&cmd_text, 524288);		// space for commands and script files
}


/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void CCommandBuffer::Cbuf_AddText (const char *text)
{
	int		l;
	
	l = Q_strlen (text);

	if (cmd_text.cursize + l >= cmd_text.maxsize)
	{
		Con_Printf ("Cbuf_AddText: overflow\n");
		return;
	}

    SZ_Write (&cmd_text, text, Q_strlen (text));	// something's going wrong here
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
FIXME: actually change the command buffer to do less copying
============
*/
void CCommandBuffer::Cbuf_InsertText (const char *text)
{
	char	*temp;
	int		templen;

// copy off any commands still remaining in the exec buffer
	templen = cmd_text.cursize;
	if (templen)
	{
		temp = mainzone->Z_Malloc<char>(templen);
		Q_memcpy (temp, cmd_text.data, templen);
		SZ_Clear (&cmd_text);
	}
	else
		temp = NULL;	// shut up compiler
		
// add the entire text of the file
	Cbuf_AddText (text);
    SZ_Write (&cmd_text, "\n", 1);
	
// add the copied off data
	if (templen)
	{
		SZ_Write (&cmd_text, temp, templen);
		mainzone->Z_Free (temp);
	}
}

/*
============
Cbuf_Execute
============
*/
void CCommandBuffer::Cbuf_Execute ()
{
	int		i;
	char	*text;
	char	line[1024];
	int		quotes;
	
	while (cmd_text.cursize)
	{
// find a \n or ; line break
		text = (char *)cmd_text.data;

		quotes = 0;
		for (i=0 ; i< cmd_text.cursize ; i++)
		{
			if (text[i] == '"')
				quotes++;
			if ( !(quotes&1) &&  text[i] == ';')
				break;	// don't break if inside a quoted string
			if (text[i] == '\n')
				break;
		}
			
				
		memcpy (line, text, i);
		line[i] = 0;
		
// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec, alias) can insert data at the
// beginning of the text buffer

		if (i == cmd_text.cursize)
			cmd_text.cursize = 0;
		else
		{
			i++;
			cmd_text.cursize -= i;
			Q_memcpy (text, text+i, cmd_text.cursize);
		}

// execute the command line
		g_pCmds->Cmd_ExecuteString (line, src_command);
		
		if (cmd_wait)
		{	// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait = false;
			break;
		}
	}
}

/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/

/*
===============
Cmd_StuffCmds_f

Adds command line parameters as script statements
Commands lead with a +, and continue until a - or another +
quake +prog jctest.qp +cmd amlev1
quake -nosound +cmd amlev1
===============
*/
void CCommand::Cmd_StuffCmds_f ()
{
	int		i, j;
	int		s;
	char	*text, *build, c;
		
	if (g_pCmds->Cmd_Argc () != 1)
	{
		Con_Printf ("stuffcmds : execute command line parameters\n");
		return;
	}

// build the combined string to parse from
	s = 0;
	for (i=1 ; i<g_Common->com_argc ; i++)
	{
		if (!g_Common->com_argv[i])
			continue;		// NEXTSTEP nulls out -NXHost
		s += Q_strlen (g_Common->com_argv[i]) + 1;
	}
	if (!s)
		return;
		
	text = mainzone->Z_Malloc<char>(s+1);
	text[0] = 0;
	for (i=1 ; i< g_Common->com_argc ; i++)
	{
		if (!g_Common->com_argv[i])
			continue;		// NEXTSTEP nulls out -NXHost
		Q_strcat (text, g_Common->com_argv[i]);
		if (i != g_Common->com_argc-1)
			Q_strcat (text, " ");
	}
	
// pull out the commands
	build = mainzone->Z_Malloc<char>(s+1);
	build[0] = 0;
	
	for (i=0 ; i<s-1 ; i++)
	{
		if (text[i] == '+')
		{
			i++;

			for (j=i ; (text[j] != '+') && (text[j] != '-') && (text[j] != 0) ; j++)
				;

			c = text[j];
			text[j] = 0;
			
			Q_strcat (build, text+i);
			Q_strcat (build, "\n");
			text[j] = c;
			i = j-1;
		}
	}
	
	if (build[0])
		g_pCmdBuf->Cbuf_InsertText (build);
	
	mainzone->Z_Free (text);
	mainzone->Z_Free (build);
}


/*
===============
Cmd_Exec_f
===============
*/
void CCommand::Cmd_Exec_f ()
{
    char *f = nullptr;
	int		mark = 0;

	if (g_pCmds->Cmd_Argc () != 2)
	{
		Con_Printf ("exec <filename> : execute a script file\n");
		return;
	}

	mark = g_MemCache->Hunk_LowMark ();
    f = COM_LoadHunkFile_IFStream<char> (g_pCmds->Cmd_Argv(1), NULL);
	if (!f)
	{
		Con_Printf ("couldn't exec %s\n", g_pCmds->Cmd_Argv(1));
		return;
	}
	Con_Printf ("execing %s\n", g_pCmds->Cmd_Argv(1));
	
	g_pCmdBuf->Cbuf_InsertText (f);
	g_MemCache->Hunk_FreeToLowMark(mark);
}


/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
void CCommand::Cmd_Echo_f ()
{
	int		i;
	
	for (i=1 ; i< g_pCmds->Cmd_Argc() ; i++)
		Con_Printf ("%s ", g_pCmds->Cmd_Argv(i));
	Con_Printf ("\n");
}

/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/

char* CCommand::CopyString (char *in)
{
	char	*out;
	
	out = mainzone->Z_Malloc<char>(strlen(in)+1);
	Q_strcpy (out, in);
	return out;
}

void CCommand::Cmd_Alias_f ()
{
	cmdalias_t	*a;
	char		cmd[1024];
	int			i, c;
	const char		*s = NULL;

	if (g_pCmds->Cmd_Argc() == 1)
	{
		Con_Printf ("Current alias commands:\n");
		for (a = cmd_alias ; a ; a=a->next)
			Con_Printf ("%s : %s\n", a->name, a->value);
		return;
	}

	s = g_pCmds->Cmd_Argv(1);
	if (strlen(s) >= MAX_ALIAS_NAME)
	{
		Con_Printf ("Alias name is too long\n");
		return;
	}

	// if the alias allready exists, reuse it
	for (a = cmd_alias ; a ; a=a->next)
	{
		if (!strcmp(s, a->name))
		{
			mainzone->Z_Free (a->value);
			break;
		}
	}

	if (!a)
	{
		a = mainzone->Z_Malloc<cmdalias_t>(sizeof(cmdalias_t));
		a->next = cmd_alias;
		cmd_alias = a;
	}
	Q_strcpy (a->name, s);	

// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	c = g_pCmds->Cmd_Argc();
	for (i=2 ; i< c ; i++)
	{
		Q_strcat (cmd, g_pCmds->Cmd_Argv(i));
		if (i != c)
			Q_strcat (cmd, " ");
	}
	Q_strcat (cmd, "\n");
	
	a->value = g_pCmds->CopyString (cmd);
}

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

struct cmd_function_t
{
	struct cmd_function_t	*next = NULL;
	const char					*name = "";
	xcommand_t				function;
    unsigned int            flags;
}; // Missi: changed from typedef struct cmd_function_s


#define	MAX_ARGS		80

int			cmd_argc;
char		*cmd_argv[MAX_ARGS];
const char		cmd_null_string[] = "";
const char		*cmd_args = NULL;

cmd_source_t	cmd_source;


cmd_function_t*	cmd_functions;		// possible commands to execute

CCommand::CCommand()
{
	Cmd_Init();
}

/*
============
Cmd_Init
============
*/
void CCommand::Cmd_Init()
{
//
// register our commands
//

	//cmd_functions.AddToTail(new cmd_function_t);

	Cmd_AddCommand ("stuffcmds",Cmd_StuffCmds_f);
	Cmd_AddCommand ("exec",Cmd_Exec_f);
	Cmd_AddCommand ("echo",Cmd_Echo_f);
	Cmd_AddCommand ("alias",Cmd_Alias_f);
	Cmd_AddCommand ("cmd", Cmd_ForwardToServer);
	Cmd_AddCommand ("wait", Cmd_Wait_f);
}

/*
============
Cmd_Argc
============
*/
int CCommand::Cmd_Argc ()
{
	return cmd_argc;
}

/*
============
Cmd_Argv
============
*/
const char* CCommand::Cmd_Argv (int arg)
{
	if ( (unsigned)arg >= (unsigned)cmd_argc )
		return cmd_null_string;
	return cmd_argv[arg];	
}

/*
============
Cmd_Args
============
*/
const char* CCommand::Cmd_Args ()
{
	return cmd_args;
}

/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
============
*/
void CCommand::Cmd_TokenizeString (const char *text)
{
	int		i;
	
// clear the args from the last string
	for (i=0 ; i<cmd_argc ; i++)
		mainzone->Z_Free (cmd_argv[i]);
		
	cmd_argc = 0;
	cmd_args = NULL;
	
	while (1)
	{
// skip whitespace up to a /n
		while (*text && *text <= ' ' && *text != '\n')
		{
			text++;
		}
		
		if (*text == '\n')
		{	// a newline seperates commands in the buffer
			text++;
			break;
		}

		if (!*text)
			return;
	
		if (cmd_argc == 1)
			 cmd_args = text;
			
		text = g_Common->COM_Parse (text);
		if (!text)
			return;

		if (cmd_argc < MAX_ARGS)
		{
			cmd_argv[cmd_argc] = mainzone->Z_Malloc<char>(Q_strlen(g_Common->com_token) + 1);
			Q_strcpy (cmd_argv[cmd_argc], g_Common->com_token);
			cmd_argc++;
		}
	}
}

/*
============
Cmd_AddCommand
============
*/
void CCommand::Cmd_AddCommand (const char *cmd_name, xcommand_t function, unsigned int flags)
{
	cmd_function_t	*cmd = NULL;

	if (host->host_initialized)	// because hunk allocation would get stomped
		Sys_Error ("Cmd_AddCommand after host_initialized");
		
// fail if the command is a variable name
	if ((Cvar_VariableString(cmd_name)) && Cvar_VariableString(cmd_name)[0])
	{
		Con_Printf ("Cmd_AddCommand: %s already defined as a var\n", cmd_name);
		return;
	}
	
// fail if the command already exists
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		if (!Q_strcmp (cmd_name, cmd->name))
		{
			Con_Printf ("Cmd_AddCommand: %s already defined\n", cmd_name);
			return;
		}
	}

	cmd = g_MemCache->Hunk_Alloc<cmd_function_t>(sizeof(cmd_function_t));
	cmd->name = cmd_name;
	cmd->function = function;
	cmd->next = cmd_functions;
    cmd->flags = flags;
	cmd_functions = cmd;
}

/*
============
Cmd_Exists
============
*/
bool CCommand::Cmd_Exists (const char *cmd_name)
{
	cmd_function_t	*cmd = NULL;
	int	i;

	for (i=0, cmd=cmd_functions; cmd; cmd = cmd->next, i++)
	{
		if (!Q_strcmp (cmd_name,cmd->name))
			return true;
	}

	return false;
}



/*
============
Cmd_CompleteCommand
============
*/
const char* CCommand::Cmd_CompleteCommand (const char *partial)
{
	cmd_function_t	*cmd;
	int				len;
	int				i;

	len = Q_strlen(partial);
	
	if (!len)
		return NULL;
		
// check functions
	for (i=0, cmd=cmd_functions; cmd; cmd = cmd->next, i++)
		if (!Q_strncmp (partial,cmd->name, len))
			return cmd->name;

	return NULL;
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
============
*/
void CCommand::Cmd_ExecuteString (const char *text, cmd_source_t src)
{	
	cmd_function_t	*cmd;
	cmdalias_t		*a;

	cmd_source = src;
	Cmd_TokenizeString (text);
			
// execute the command line
	if (!Cmd_Argc())
		return;		// no tokens

// check functions
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		if (!Q_strcasecmp (cmd_argv[0],cmd->name))
		{
            if ((cmd->flags & CVAR_CHEAT) && sv_cheats.value == 0)
            {
                Con_Printf("Player %s attempted to run cheat command \"%s\" without cheats enabled!\n", cl_name.string, cmd->name);
                return;
            }

			cmd->function ();
			return;
		}
	}

// check alias
	for (a=cmd_alias ; a ; a=a->next)
	{
		if (!Q_strcasecmp (cmd_argv[0], a->name))
		{
			g_pCmdBuf->Cbuf_InsertText (a->value);
			return;
		}
	}
	
// check cvars
	if (!Cvar_Command ())
		Con_Warning ("Unknown command \"%s\"\n", Cmd_Argv(0));
	
}


/*
===================
Cmd_ForwardToServer

Sends the entire command line over to the server
===================
*/
void CCommand::Cmd_ForwardToServer ()
{
	if (cls.state != ca_connected)
	{
		Con_Printf ("Can't \"%s\", not connected\n", g_pCmds->Cmd_Argv(0));
		return;
	}
	
	if (cls.demoplayback)
		return;		// not really connected

	MSG_WriteByte (&cls.message, clc_stringcmd);
	if (Q_strcasecmp(g_pCmds->Cmd_Argv(0), "cmd") != 0)
	{
		SZ_Print (&cls.message, g_pCmds->Cmd_Argv(0));
		SZ_Print (&cls.message, " ");
	}
	if (g_pCmds->Cmd_Argc() > 1)
		SZ_Print (&cls.message, g_pCmds->Cmd_Args());
	else
		SZ_Print (&cls.message, "\n");
}


/*
================
Cmd_CheckParm

Returns the position (1 to argc-1) in the command's argument list
where the given parameter apears, or 0 if not present
================
*/

int CCommand::Cmd_CheckParm (char *parm)
{
	int i;
	
	if (!parm)
		Sys_Error ("Cmd_CheckParm: NULL");

	for (i = 1; i < Cmd_Argc (); i++)
		if (! Q_strcasecmp (parm, Cmd_Argv (i)))
			return i;
			
	return 0;
}

CCommandBuffer::CCommandBuffer()
{
	Cbuf_Init();
}
