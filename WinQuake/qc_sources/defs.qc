// Missi: put these in your defs.qc file to use the added functions in pr_builtins

void(string v, float f) cxxvectoradd_flt	= #79;		// set parm1... to the
void(string v, float f) cxxvectoradd_int	= #80;		// set parm1... to the
void(string v, entity e) cxxvectoradd_ent	= #81;		// set parm1... to the
void(string v, string s) cxxvectoradd_str	= #82;		// set parm1... to the
float(string v, float f) vec_access_flt		= #83;
float(string v, float f) vec_access_int		= #84;
entity(string v, float f) vec_access_ent		= #85;
string(string v, float f) vec_access_str		= #86;
float(string v) vec_size			= #87;