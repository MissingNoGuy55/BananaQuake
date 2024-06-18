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


/*

*/

typedef struct
{
	int				s;
	dfunction_t		*f;
} prstack_t;

#define	MAX_STACK_DEPTH		32
static prstack_t	pr_stack[MAX_STACK_DEPTH];
static int			pr_depth;

#define	LOCALSTACK_SIZE		2048
static int			localstack[LOCALSTACK_SIZE];
static int			localstack_used;


bool	pr_trace;
dfunction_t	*pr_xfunction;
int			pr_xstatement;


int		pr_argc;

static const char *pr_opnames[] =
{
"DONE",

"MUL_F",
"MUL_V", 
"MUL_FV",
"MUL_VF",
 
"DIV",

"ADD_F",
"ADD_V", 
  
"SUB_F",
"SUB_V",

"EQ_F",
"EQ_V",
"EQ_S", 
"EQ_E",
"EQ_FNC",
 
"NE_F",
"NE_V", 
"NE_S",
"NE_E", 
"NE_FNC",
 
"LE",
"GE",
"LT",
"GT", 

"INDIRECT",
"INDIRECT",
"INDIRECT", 
"INDIRECT", 
"INDIRECT",
"INDIRECT", 

"ADDRESS", 

"STORE_F",
"STORE_V",
"STORE_S",
"STORE_ENT",
"STORE_FLD",
"STORE_FNC",

"STOREP_F",
"STOREP_V",
"STOREP_S",
"STOREP_ENT",
"STOREP_FLD",
"STOREP_FNC",

"RETURN",
  
"NOT_F",
"NOT_V",
"NOT_S", 
"NOT_ENT", 
"NOT_FNC", 
  
"IF",
"IFNOT",
  
"CALL0",
"CALL1",
"CALL2",
"CALL3",
"CALL4",
"CALL5",
"CALL6",
"CALL7",
"CALL8",
  
"STATE",
  
"GOTO", 
  
"AND",
"OR", 

"BITAND",
"BITOR",

"SWITCH",
"CASE",
"DEFAULT",
"ARRAY_OPEN",
"ARRAY_CLOSE"
};

const char *PR_GlobalString (int ofs);
const char *PR_GlobalStringNoContents (int ofs);


//=============================================================================

/*
=================
PR_PrintStatement
=================
*/
void PR_PrintStatement (dstatement_t *s)
{
	int		i;
	
	if ( (unsigned)s->op < sizeof(pr_opnames)/sizeof(pr_opnames[0]))
	{
		Con_Printf ("%s ",  pr_opnames[s->op]);
		i = strlen(pr_opnames[s->op]);
		for ( ; i<10 ; i++)
			Con_Printf (" ");
	}
		
	if (s->op == OP_IF || s->op == OP_IFNOT)
		Con_Printf ("%sbranch %i",PR_GlobalString(s->a),s->b);
	else if (s->op == OP_GOTO)
	{
		Con_Printf ("branch %i",s->a);
	}
	else if ( (unsigned)(s->op - OP_STORE_F) < 6)
	{
		Con_Printf ("%s",PR_GlobalString(s->a));
		Con_Printf ("%s", PR_GlobalStringNoContents(s->b));
	}
	else
	{
		if (s->a)
			Con_Printf ("%s",PR_GlobalString(s->a));
		if (s->b)
			Con_Printf ("%s",PR_GlobalString(s->b));
		if (s->c)
			Con_Printf ("%s", PR_GlobalStringNoContents(s->c));
	}
	Con_Printf ("\n");
}

/*
============
PR_StackTrace
============
*/
void PR_StackTrace (void)
{
	dfunction_t	*f;
	int			i;
	
	if (pr_depth == 0)
	{
		Con_Printf ("<NO STACK>\n");
		return;
	}
	
	pr_stack[pr_depth].f = pr_xfunction;
	for (i=pr_depth ; i>=0 ; i--)
	{
		f = pr_stack[i].f;
		
		if (!f)
		{
			Con_Printf ("<NO FUNCTION>\n");
		}
		else
			Con_Printf ("%12s : %s\n", PR_GetString(f->s_file), PR_GetString(f->s_name));
	}
}


/*
============
PR_Profile_f

============
*/
void PR_Profile_f (void)
{
	dfunction_t	*f, *best;
	int			max;
	int			num;
	int			i;
	
	num = 0;	
	do
	{
		max = 0;
		best = NULL;
		for (i=0 ; i<progs->numfunctions ; i++)
		{
			f = &pr_functions[i];
			if (f->profile > max)
			{
				max = f->profile;
				best = f;
			}
		}
		if (best)
		{
			if (num < 10)
				Con_Printf ("%7i %s\n", best->profile, PR_GetString(best->s_name));
			num++;
			best->profile = 0;
		}
	} while (best);
}


/*
============
PR_RunError

Aborts the currently executing function
============
*/
void PR_RunError (const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,error);
	vsprintf (string,error,argptr);
	va_end (argptr);

	PR_PrintStatement (pr_statements + pr_xstatement);
	PR_StackTrace ();
	Con_Printf ("%s\n", string);
	
	pr_depth = 0;		// dump the stack so host_error can shutdown functions

	host->Host_Error ("Program error");
}

/*
====================
PR_EnterFunction

Returns the new program statement counter
====================
*/
int PR_EnterFunction(dfunction_t* f)
{
	int	i, j, c, o;

	pr_stack[pr_depth].s = pr_xstatement;
	pr_stack[pr_depth].f = pr_xfunction;
	pr_depth++;
	if (pr_depth >= MAX_STACK_DEPTH)
		PR_RunError("stack overflow");

	// save off any locals that the new function steps on
	c = f->locals;
	if (localstack_used + c > LOCALSTACK_SIZE)
		PR_RunError("PR_ExecuteProgram: locals stack overflow");

	for (i = 0; i < c; i++)
		localstack[localstack_used + i] = ((int*)pr_globals)[f->parm_start + i];
	localstack_used += c;

	// copy parameters
	o = f->parm_start;
	for (i = 0; i < f->numparms; i++)
	{
		for (j = 0; j < f->parm_size[i]; j++)
		{
			((int*)pr_globals)[o] = ((int*)pr_globals)[OFS_PARM0 + i * 3 + j];
			o++;
		}
	}

	pr_xfunction = f;
	return f->first_statement - 1;	// offset the s++
}

/*
====================
PR_LeaveFunction
====================
*/
int PR_LeaveFunction(void)
{
	int		i, c;

	if (pr_depth <= 0)
		Sys_Error("prog stack underflow");

	// restore locals from the stack
	c = pr_xfunction->locals;
	localstack_used -= c;
	if (localstack_used < 0)
		PR_RunError("PR_ExecuteProgram: locals stack underflow\n");

	for (i = 0; i < c; i++)
		((int*)pr_globals)[pr_xfunction->parm_start + i] = localstack[localstack_used + i];

	// up stack
	pr_depth--;
	pr_xfunction = pr_stack[pr_depth].f;
	return pr_stack[pr_depth].s;
}

/*
============================================================================
PR_ExecuteProgram

The interpretation main loop
============================================================================
*/

/*	
	Missi: The comments on these are nonexistant and what they do is pretty much
	left up to interpretation without comments. These seem to correspond to...:

	Generally, opb and opa will only be used together with "opa = opb" or "opa == opb" declarations in QC, while opc is only used in
	comparisons or arithmetic in C/C++ i.e "opc = (opa == opb)" to set the actual value. However, opb is always the set value in declarations only.

	So...

	opa: a passed argument or value
	opb: resulting var (declarations only) or value to be compared against
	opc: the value set from comparison or arithmetic (9/12/2023)
*/

#define opa ((eval_t*)&pr_globals[(unsigned short)st->a])
#define opb ((eval_t*)&pr_globals[(unsigned short)st->b])
#define opc ((eval_t*)&pr_globals[(unsigned short)st->c])

/*
====================
PR_ExecuteProgram
====================
*/
void PR_ExecuteProgram(func_t fnum)
{
    eval_t* ptr = nullptr;
    dstatement_t* st = nullptr;
    dfunction_t* f = nullptr, * newf = nullptr;
	int			profile = 0, startprofile = 0;
    edict_t*	ed = nullptr;
	int			exitdepth = 0;
    float		switch_float = 0.0f;
    string_t	switch_string = 0;
    edict_t*	switch_edict = nullptr;
	vec_t*		switch_vector = vec3_origin;

	bool		switch_case = false;
	bool		switch_case_default = false;
	bool		switch_string_matched = false;
	bool		switch_float_matched = false;

	if (!fnum || fnum >= progs->numfunctions)
	{
		if (pr_global_struct->self)
			ED_Print(PROG_TO_EDICT(pr_global_struct->self));
		host->Host_Error("PR_ExecuteProgram: NULL function");
	}

	f = &pr_functions[fnum];

	pr_trace = false;

	// make a stack frame
	exitdepth = pr_depth;

	st = &pr_statements[PR_EnterFunction(f)];

		while (1)
		{
			st++;	/* next statement */

			if (++profile > 100000)
			{
				pr_xstatement = st - pr_statements;
				PR_RunError("runaway loop error");
			}

			if (pr_trace)
				PR_PrintStatement(st);

			switch (st->op)
			{
			case OP_ADD_F:
				opc->_float = opa->_float + opb->_float;
				break;
			case OP_ADD_V:
				opc->vector[0] = opa->vector[0] + opb->vector[0];
				opc->vector[1] = opa->vector[1] + opb->vector[1];
				opc->vector[2] = opa->vector[2] + opb->vector[2];
				break;

			case OP_SUB_F:
				opc->_float = opa->_float - opb->_float;
				break;
			case OP_SUB_V:
				opc->vector[0] = opa->vector[0] - opb->vector[0];
				opc->vector[1] = opa->vector[1] - opb->vector[1];
				opc->vector[2] = opa->vector[2] - opb->vector[2];
				break;

			case OP_MUL_F:
				opc->_float = opa->_float * opb->_float;
				break;
			case OP_MUL_V:
				opc->_float = opa->vector[0] * opb->vector[0] +
					opa->vector[1] * opb->vector[1] +
					opa->vector[2] * opb->vector[2];
				break;
			case OP_MUL_FV:
				opc->vector[0] = opa->_float * opb->vector[0];
				opc->vector[1] = opa->_float * opb->vector[1];
				opc->vector[2] = opa->_float * opb->vector[2];
				break;
			case OP_MUL_VF:
				opc->vector[0] = opb->_float * opa->vector[0];
				opc->vector[1] = opb->_float * opa->vector[1];
				opc->vector[2] = opb->_float * opa->vector[2];
				break;

			case OP_DIV_F:
				opc->_float = opa->_float / opb->_float;
				break;

			case OP_BITAND:
				opc->_float = (int)opa->_float & (int)opb->_float;
				break;

			case OP_BITOR:
				opc->_float = (int)opa->_float | (int)opb->_float;
				break;

			case OP_GE:
				opc->_float = opa->_float >= opb->_float;
				break;
			case OP_LE:
				opc->_float = opa->_float <= opb->_float;
				break;
			case OP_GT:
				opc->_float = opa->_float > opb->_float;
				break;
			case OP_LT:
				opc->_float = opa->_float < opb->_float;
				break;
			case OP_AND:
				opc->_float = opa->_float && opb->_float;
				break;
			case OP_OR:
				opc->_float = opa->_float || opb->_float;
				break;

			case OP_NOT_F:
				opc->_float = !opa->_float;
				break;
			case OP_NOT_V:
				opc->_float = !opa->vector[0] && !opa->vector[1] && !opa->vector[2];
				break;
			case OP_NOT_S:
				opc->_float = !opa->string || !*PR_GetString(opa->string);
				break;
			case OP_NOT_FNC:
				opc->_float = !opa->function;
				break;
			case OP_NOT_ENT:
				opc->_float = (PROG_TO_EDICT(opa->edict) == sv->edicts);
				break;

			case OP_EQ_F:
				opc->_float = opa->_float == opb->_float;
				break;
			case OP_EQ_V:
				opc->_float = (opa->vector[0] == opb->vector[0]) &&
					(opa->vector[1] == opb->vector[1]) &&
					(opa->vector[2] == opb->vector[2]);
				break;
			case OP_EQ_S:
				opc->_float = !strcmp(PR_GetString(opa->string), PR_GetString(opb->string));
				break;
			case OP_EQ_E:
				opc->_float = opa->_int == opb->_int;
				break;
			case OP_EQ_FNC:
				opc->_float = opa->function == opb->function;
				break;

			case OP_NE_F:
				opc->_float = opa->_float != opb->_float;
				break;
			case OP_NE_V:
				opc->_float = (opa->vector[0] != opb->vector[0]) ||
					(opa->vector[1] != opb->vector[1]) ||
					(opa->vector[2] != opb->vector[2]);
				break;
			case OP_NE_S:
				opc->_float = strcmp(PR_GetString(opa->string), PR_GetString(opb->string));
				break;
			case OP_NE_E:
				opc->_float = opa->_int != opb->_int;
				break;
			case OP_NE_FNC:
				opc->_float = opa->function != opb->function;
				break;

			case OP_STORE_F:
			case OP_STORE_ENT:
			case OP_STORE_FLD:	// integers
            case OP_STORE_S:
			case OP_STORE_FNC:	// pointers
				opb->_int = opa->_int;
				break;
			case OP_STORE_V:
				opb->vector[0] = opa->vector[0];
				opb->vector[1] = opa->vector[1];
				opb->vector[2] = opa->vector[2];
				break;

			case OP_STOREP_F:
			case OP_STOREP_ENT:
			case OP_STOREP_FLD:	// integers
			case OP_STOREP_S:
			case OP_STOREP_FNC:	// pointers
				ptr = (eval_t*)((byte*)sv->edicts + opb->_int);
				ptr->_int = opa->_int;
				break;
			case OP_STOREP_V:
				ptr = (eval_t*)((byte*)sv->edicts + opb->_int);
				ptr->vector[0] = opa->vector[0];
				ptr->vector[1] = opa->vector[1];
				ptr->vector[2] = opa->vector[2];
				break;

			case OP_ADDRESS:
				ed = PROG_TO_EDICT(opa->edict);
#ifdef PARANOID
				NUM_FOR_EDICT(ed);	// Make sure it's in range
#endif
				if (ed == (edict_t*)sv->edicts && sv->state == ss_active)
				{
					pr_xstatement = st - pr_statements;
					PR_RunError("assignment to world entity");
				}
				opc->_int = (byte*)((int*)&ed->v + opb->_int) - (byte*)sv->edicts;
				break;

			case OP_LOAD_F:
			case OP_LOAD_FLD:
			case OP_LOAD_ENT:
			case OP_LOAD_S:
			case OP_LOAD_FNC:
				ed = PROG_TO_EDICT(opa->edict);
#ifdef PARANOID
				NUM_FOR_EDICT(ed);	// Make sure it's in range
#endif
				opc->_int = ((eval_t*)((int*)&ed->v + opb->_int))->_int;
				break;

			case OP_LOAD_V:
				ed = PROG_TO_EDICT(opa->edict);
#ifdef PARANOID
				NUM_FOR_EDICT(ed);	// Make sure it's in range
#endif
				ptr = (eval_t*)((int*)&ed->v + opb->_int);
				opc->vector[0] = ptr->vector[0];
				opc->vector[1] = ptr->vector[1];
				opc->vector[2] = ptr->vector[2];
				break;

			case OP_IFNOT:
				if (!opa->_int)
					st += st->b - 1;	/* -1 to offset the st++ */
				break;

			case OP_IF:
				if (opa->_int)
					st += st->b - 1;	/* -1 to offset the st++ */
				break;

			case OP_GOTO:
				st += st->a - 1;		/* -1 to offset the st++ */
				break;

			case OP_CALL0:
			case OP_CALL1:
			case OP_CALL2:
			case OP_CALL3:
			case OP_CALL4:
			case OP_CALL5:
			case OP_CALL6:
			case OP_CALL7:
			case OP_CALL8:
				pr_xfunction->profile += profile - startprofile;
				startprofile = profile;
				pr_xstatement = st - pr_statements;
				pr_argc = st->op - OP_CALL0;
				if (!opa->function)
					PR_RunError("NULL function");
				newf = &pr_functions[opa->function];
				if (newf->first_statement < 0)
				{ // Built-in function
					int i = -newf->first_statement;
					if (i >= pr_numbuiltins)
						PR_RunError("Bad builtin call number %d", i);
					pr_builtins[i]();
					break;
				}
				// Normal function
				st = &pr_statements[PR_EnterFunction(newf)];
				break;

			case OP_DONE:
			case OP_RETURN:
				pr_xfunction->profile += profile - startprofile;
				startprofile = profile;
				pr_xstatement = st - pr_statements;
				pr_globals[OFS_RETURN] = pr_globals[(unsigned short)st->a];
				pr_globals[OFS_RETURN + 1] = pr_globals[(unsigned short)st->a + 1];
				pr_globals[OFS_RETURN + 2] = pr_globals[(unsigned short)st->a + 2];
				st = &pr_statements[PR_LeaveFunction()];
				if (pr_depth == exitdepth)
				{ // Done
					return;
				}
				break;

			case OP_STATE:
				ed = PROG_TO_EDICT(pr_global_struct->self);
				ed->v.nextthink = pr_global_struct->time + 0.1;
				ed->v.frame = opa->_float;
				ed->v.think = opb->function;
				break;

			case OP_SWITCH_F:
			{
				if (opa->_float)
					switch_float = opa->_float;
				break;
			}

			case OP_SWITCH_S:
			{
				if (opa->string)
					switch_string = opa->string;
				break;
			}
			
			case OP_SWITCH_E:
			{
				if (opa->edict)
					switch_edict = PROG_TO_EDICT(opa->edict);
				break;
			}

			case OP_SWITCH_FV:
			{
				if (opa->vector)
				{
					switch_vector = opa->vector;
				}
				break;
			}

			case OP_CASE:
			case OP_DEFAULT:
			case OP_BREAK:
			{
				dstatement_t* matching_statement = NULL;

				if ((opa->string &~ 0 && opa->string == switch_string))
					matching_statement = st;

				if (opa->_float == switch_float)
					matching_statement = st;

				if (opa->vector == switch_vector)
					matching_statement = st;

				if (opa->edict == EDICT_TO_PROG(switch_edict))
					matching_statement = st;

				if (!matching_statement)
				{
					st++;
					bool didbreak = false;
					while (!didbreak)
					{
						st++;
						
						switch (st->op)
						{
							case OP_CASE:
							case OP_DEFAULT:
							case OP_BREAK:
								didbreak = true;
						}
					}
				}
				break;
			}

			default:
				pr_xstatement = st - pr_statements;
				PR_RunError("Bad opcode %i", st->op);
			}
		}	/* end of while(1) loop */
}

#undef opa
#undef opb
#undef opc
