float	IT_BULLET_TIME			= 4294967296;

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

string(entity e, string f)	findfield = #88;
string(entity e)	multimanager = #89;
void(entity e, float contents)	setcontents = #90;
void(vector org, vector end, vector color) drawdebugline = #91;
void(entity ent, string sentence) speak_sentence = #92;
void(string sound) precache_sentence = #93;
void(entity e1, string e2) adjusttrain = #94;