
#ifndef HOST_H
#define HOST_H

template<typename T = byte>
struct quakeparms_t
{
	const char* basedir;
	const char* cachedir;		// for development over ISDN lines
	const char* userdir;
	int		argc;
	char**	argv;
	T		membase;
	size_t	memsize;
};	// Missi: had to be changed from a typedef struct. also moved this from quakedef.h (6/4/2022)

class CQuakeHost
{
public:
	CQuakeHost();
	CQuakeHost(quakeparms_t<byte*> parms);
	//
	// host
	//

	quakeparms_t<byte*> host_parms;	// Missi: needed an address pointer (6/4/2022)

	// cvar_t		sys_ticrate;
	cvar_t		sys_nostdout;
	//cvar_t		developer;

    bool        host_initialized;		// true if into command execution
	double		host_frametime;
    byte*       host_basepal;
    byte*       host_colormap;
	int			host_framecount;	// incremented every frame, never reset
	double		realtime;			// not bounded in any way, changed at
                                    // start of every frame, never reset

    bool		msg_suppress_1;		// suppresses resolution and cache size console output
                                    //  an fullscreen DIB focus gain/loss
    static int	current_skill;		// skill level for currently loaded level (in case
                                    //  the user changes the cvar while the level is
                                    //  running, this reflects the level actually in use)

    bool		isDedicated;

    int			minimum_memory;

	void Host_ClearMemory(void);
	bool Host_FilterTime(float time);
	void Host_GetConsoleCommands(void);
	void Host_ServerFrame(void);

	void Host_InitCommands(void);
	//void Host_InitVCR(quakeparms_t<byte*> parms);
	void Host_Init(quakeparms_t<byte*> parms);
	void Host_Shutdown(void);
	void Host_Error(const char* error, ...);
	void Host_EndGame(const char* message, ...);
	void Host_Frame(float time);
	void Host_ClientCommands(const char* fmt, ...);
	void Host_ShutdownServer(bool crash);

	void Host_InitLocal(void);
	void Host_WriteConfiguration(void);
	void _Host_Frame(float time);

//	quakeparms_t host_parms;

	//bool	host_initialized;		// true if into command execution

	//double		host_frametime;
	//double		host_time;
	//double		realtime;				// without any filtering or bounding
	double		oldrealtime;			// last frame run
	//int			host_framecount;

	int			host_hunklevel;

	//int			minimum_memory;

	bool 	host_died;

	cvar_t	host_framerate = { "host_framerate","0" };	// set for slow motion
	cvar_t	host_speeds = { "host_speeds","0" };			// set for running times

	cvar_t	sys_ticrate = { "sys_ticrate","0.05" };
	cvar_t	serverprofile = { "serverprofile","0" };

	cvar_t	fraglimit = { "fraglimit","0",false,true };
	cvar_t	timelimit = { "timelimit","0",false,true };
	cvar_t	teamplay = { "teamplay","0",false,true };

	cvar_t	samelevel = { "samelevel","0" };
	cvar_t	noexit = { "noexit","0",false,true };

#ifdef QUAKE2
	cvar_t	developer = { "developer","1" };	// should be 0 for release!
#else
	cvar_t	developer = { "developer","0" };
#endif

	cvar_t	skill = { "skill","1" };						// 0 - 3
	cvar_t	deathmatch = { "deathmatch","0" };			// 0, 1, or 2
	cvar_t	coop = { "coop","0" };			// 0 or 1

	cvar_t	pausable = { "pausable","1" };

	cvar_t	temp1 = { "temp1","0" };

	void Host_FindMaxClients(void);

	// Missi: console commands (6/21/2024)
	static void Host_SavegameComment(char* text);
	static void Host_Color_f(void);
	static void Host_Kill_f(void);
	static void Host_Pause_f(void);
	static void Host_PreSpawn_f(void);
	static void Host_Spawn_f(void);
	static void Host_Begin_f(void);
	static void Host_Kick_f(void);
	static void Host_Give_f(void);
	static void Host_Viewmodel_f(void);
	static void Host_Viewframe_f(void);
	static void Host_Viewnext_f(void);
	static void Host_Viewprev_f(void);
	static void Host_Demos_f(void);
	static void Host_Startdemos_f(void);
	static void Host_Stopdemo_f(void);
	static void Host_Fly_f(void);
	static void Host_Ping_f(void);
	static void Host_Map_f(void);
	static void Host_Changelevel_f(void);
	static void Host_Restart_f(void);
	static void Host_Reconnect_f(void);
	static void Host_Connect_f(void);
	static void Host_Savegame_f(void);
	static void Host_Loadgame_f(void);
	static void Host_Name_f(void);
	static void Host_Version_f(void);
	static void Host_Say(bool teamonly);
	static void Host_Say_f(void);
	static void Host_Say_Team_f(void);
	static void Host_Tell_f(void);
	static void Host_Quit_f(void);
	static void Host_Status_f(void);
	static void Host_God_f(void);
	static void Host_Notarget_f(void);
	static void Host_Noclip_f(void);

private:

	CQuakeHost(const CQuakeHost& src);

};

extern CQuakeHost* host;
extern client_t* host_client;			// current client

#endif
