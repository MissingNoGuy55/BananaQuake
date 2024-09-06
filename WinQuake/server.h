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
// server.h

#ifndef SERVER_H
#define SERVER_H

#ifndef GLQUAKE
#include "model.h"
#else
#include "gl_model.h"
#endif
#include "world.h"
typedef struct server_static_s
{
	int			maxclients;
	int			maxclientslimit;
	struct client_s	*clients;		// [maxclients]
	int			serverflags;		// episode completion information
	bool	changelevel_issued;	// cleared when at SV_SpawnServer
} server_static_t;

//=============================================================================

typedef enum {ss_loading, ss_active} server_state_t;

#define	NUM_PING_TIMES		16
#define	NUM_SPAWN_PARMS		16

typedef struct client_s
{
	bool		active;				// false = client is free
	bool		spawned;			// false = don't send datagrams
	bool		dropasap;			// has been told to go to another level
	bool		privileged;			// can execute any host command
	bool		sendsignon;			// only valid before spawned

	double			last_message;		// reliable messages must be sent
										// periodically

	struct qsocket_s *netconnection;	// communications handle

	usercmd_t		cmd;				// movement
	vec3_t			wishdir;			// intended motion calced from cmd

	sizebuf_t		message;			// can be added to at any time,
										// copied and clear once per frame
	byte			msgbuf[MAX_MSGLEN];
	edict_t			*edict;				// EDICT_NUM(clientnum+1)
	char			name[32];			// for printing to other people
	int				colors;
		
	float			ping_times[NUM_PING_TIMES];
	int				num_pings;			// ping_times[num_pings%NUM_PING_TIMES]

// spawn parms are carried from level to level
	float			spawn_parms[NUM_SPAWN_PARMS];

// client known data for deltas	
	int				old_frags;
} client_t;

//=============================================================================

// edict->movetype values
#define	MOVETYPE_NONE			0		// never moves
#define	MOVETYPE_ANGLENOCLIP	1
#define	MOVETYPE_ANGLECLIP		2
#define	MOVETYPE_WALK			3		// gravity
#define	MOVETYPE_STEP			4		// gravity, special edge handling
#define	MOVETYPE_FLY			5
#define	MOVETYPE_TOSS			6		// gravity
#define	MOVETYPE_PUSH			7		// no clip to world, push and crush
#define	MOVETYPE_NOCLIP			8
#define	MOVETYPE_FLYMISSILE		9		// extra size to monsters
#define	MOVETYPE_BOUNCE			10
#define MOVETYPE_LADDER			13
#ifdef QUAKE2
#define MOVETYPE_BOUNCEMISSILE	11		// bounce w/o gravity
#define MOVETYPE_FOLLOW			12		// track movement of aiment
#endif

// edict->solid values
#define	SOLID_NOT				0		// no interaction with other objects
#define	SOLID_TRIGGER			1		// touch on edge, but not blocking
#define	SOLID_BBOX				2		// touch on edge, block
#define	SOLID_SLIDEBOX			3		// touch on edge, but not an onground
#define	SOLID_BSP				4		// bsp clip, touch on edge, block
#define SOLID_FOG_VOLUME        5       //
#define	SOLID_MONSTERONLY		6

// edict->deadflag values
#define	DEAD_NO					0
#define	DEAD_DYING				1
#define	DEAD_DEAD				2

#define	DAMAGE_NO				0
#define	DAMAGE_YES				1
#define	DAMAGE_AIM				2

// edict->flags
#define	FL_FLY					1
#define	FL_SWIM					2
//#define	FL_GLIMPSE				4
#define	FL_CONVEYOR				4
#define	FL_CLIENT				8
#define	FL_INWATER				16
#define	FL_MONSTER				32
#define	FL_GODMODE				64
#define	FL_NOTARGET				128
#define	FL_ITEM					256
#define	FL_ONGROUND				512
#define	FL_PARTIALGROUND		1024	// not all corners are valid
#define	FL_WATERJUMP			2048	// player jumping out of water
#define	FL_JUMPRELEASED			4096	// for jump debouncing
#define FL_ONLADDER				8192
#define FL_FAKECLIENT			16384
#ifdef QUAKE2
#define FL_FLASHLIGHT			8192
#define FL_ARCHIVE_OVERRIDE		1048576
#endif

// entity effects

#define	EF_BRIGHTFIELD			1
#define	EF_MUZZLEFLASH 			2
#define	EF_BRIGHTLIGHT 			4
#define	EF_DIMLIGHT 			8
#define EF_NODRAW				128
#ifdef QUAKE2
#define EF_DARKLIGHT			16
#define EF_DARKFIELD			32
#define EF_LIGHT				64
#endif

#define	SPAWNFLAG_NOT_EASY			256
#define	SPAWNFLAG_NOT_MEDIUM		512
#define	SPAWNFLAG_NOT_HARD			1024
#define	SPAWNFLAG_NOT_DEATHMATCH	2048

#ifdef QUAKE2
// server flags
#define	SFL_EPISODE_1		1
#define	SFL_EPISODE_2		2
#define	SFL_EPISODE_3		4
#define	SFL_EPISODE_4		8
#define	SFL_NEW_UNIT		16
#define	SFL_NEW_EPISODE		32
#define	SFL_CROSS_TRIGGERS	65280
#endif

//============================================================================
class CQuakeServer
{

public:

	CQuakeServer();
	~CQuakeServer();

	void SV_Init();

	void SV_StartParticle(vec3_t org, vec3_t dir, int color, int count);
	void SV_StartSound(edict_t* entity, int channel, const char* sample, int vol,
		float attenuation);

	void SV_SendServerinfo(client_t* client);

	void SV_DropClient(bool crash);

	void SV_SendNop(client_t* client);

	void SV_SendClientMessages();
	void SV_ClearDatagram();

	int SV_ModelIndex(const char* modname);

	void SV_CreateBaseline();

	void SV_SendReconnect();

	void SV_SetIdealPitch();

	void SV_UserFriction();

	void SV_Accelerate();

	void SV_AirAccelerate(vec3_t wishveloc);

	void SV_AddUpdates();

	void SV_WaterMove();

	void SV_WaterJump();

	void SV_AirMove();

	void SV_ClientThink();
	void SV_ReadClientMove(usercmd_t* move);
	void SV_AddClientToServer(struct qsocket_s* ret);

	static void SV_ClientPrintf(const char* fmt, ...);
	void SV_BroadcastPrintf(const char* fmt, ...);

	void SV_Physics_Step(edict_t* ent);

	void SV_Physics_Ladder(edict_t* ent);

	void SV_Physics();

	bool SV_CheckBottom(edict_t* ent);
	bool SV_movestep(edict_t* ent, vec3_t move, bool relink);

	bool SV_StepDirection(edict_t* ent, float yaw, float dist);

	bool SV_LadderStepDirection(edict_t* ent, float yaw, float dist);

	void SV_FixCheckBottom(edict_t* ent);

	void SV_NewChaseDir(edict_t* actor, edict_t* enemy, float dist);

	bool SV_CloseEnough(edict_t* ent, edict_t* goal, float dist);

	void SV_AddToFatPVS(vec3_t org, mnode_s* node);

	byte* SV_FatPVS(vec3_t org);

	void SV_WriteEntitiesToClient(edict_t* clent, sizebuf_t* msg);

	void SV_CleanupEnts();

	void SV_WriteClientdataToMessage(edict_t* ent, sizebuf_t* msg);

	bool SV_SendClientDatagram(client_t* client);

	void SV_UpdateToReliableMessages();

	void SV_MoveToGoal();

	void SV_ConnectClient(int clientnum, bool bot = false);

	void SV_CheckForNewClients();
	bool SV_ReadClientMessage();
	void SV_RunClients();
	void SV_SaveSpawnparms();
#ifdef QUAKE2
	void SV_SpawnServer(char* server, char* startspot);
#else
	void SV_SpawnServer(char* server);
#endif

	void SV_CheckAllEnts();

	void SV_CheckVelocity(edict_t* ent);

	bool SV_RunThink(edict_t* ent);

	void SV_Impact(edict_t* e1, edict_t* e2);

	int SV_FlyMove(edict_t* ent, float tm, trace_t* steptrace);

	trace_t SV_PushEntity(edict_t* ent, vec3_t push);

	void SV_PushMove(edict_t* pusher, float movetime);

	void SV_Physics_Pusher(edict_t* ent);

	void SV_CheckStuck(edict_t* ent);

	bool SV_CheckWater(edict_t* ent);

	void SV_WallFriction(edict_t* ent, trace_t* trace);

	int SV_TryUnstick(edict_t* ent, vec3_t oldvel);

	void SV_WalkMove(edict_t* ent);

	void SV_LadderMove(edict_t* ent);

	void SV_Physics_Client(edict_t* ent, int num);

	void SV_Physics_None(edict_t* ent);

	void SV_Physics_Noclip(edict_t* ent);

	void SV_CheckWaterTransition(edict_t* ent);

	void SV_Physics_Toss(edict_t* ent);

	void SV_ClearWorld();
	// called after the world model has been loaded, before linking any entities

	void SV_UnlinkEdict(edict_t* ent);

    void SV_AreaTriggerEdicts ( edict_t *ent, struct areanode_s *node, edict_t **list, int *listcount, const int listspace );
    void SV_TouchLinks(edict_t* ent);
	// call before removing an entity, and before trying to move one,
	// so it doesn't clip against itself
    // flags ent->v.modified

	void SV_InitBoxHull();
    void SV_CheckFogVolumes(edict_t *ent);

	hull_t* SV_HullForBox(vec3_t mins, vec3_t maxs);

	hull_t* SV_HullForEntity(edict_t* ent, vec3_t mins, vec3_t maxs, vec3_t offset);

	void SV_LinkEdict(edict_t* ent, bool touch_triggers);
	int SV_HullPointContents(hull_t* hull, int num, vec3_t p);
	// Needs to be called any time an entity changes origin, mins, maxs, or solid
	// flags ent->v.modified
	// sets ent->v.absmin and ent->v.absmax
	// if touchtriggers, calls prog functions for the intersected triggers

	int SV_PointContents(vec3_t p);
	int SV_TruePointContents(vec3_t p);
	// returns the CONTENTS_* value from the world at the given point.
	// does not check any entities at all
	// the non-true version remaps the water current contents to content_water

#ifndef GLQUAKE
	bool SV_RecursiveHullCheck(hull_t* hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t* trace);
#else
	bool SV_RecursiveHullCheck(hull_t* hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t* trace);
#endif
	edict_t* SV_TestEntityPosition(edict_t* ent);
	trace_t SV_ClipMoveToEntity(edict_t* ent, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);

	void SV_ClipToLinks(struct areanode_s* node, struct moveclip_s* clip);

	void SV_MoveBounds(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, vec3_t boxmins, vec3_t boxmaxs);

	trace_t SV_Move(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int type, edict_t* passedict);

    void SV_AddGravity (edict_t *ent);

    // mins and maxs are reletive

	// if the entire move stays in a solid volume, trace.allsolid will be set

	// if the starting point is in a solid, it will be allowed to move out
	// to an open area

	// nomonsters is used for line of sight or edge testing, where mosnters
	// shouldn't be considered solid objects

	// passedict is explicitly excluded from clipping checks (normally NULL)

	// Missi: set/get stuff (7/3/2024)
	const bool	DoesLevelHaveFog() const { return level_has_fog; }
	void		SetLevelHasFog(bool bNewVal) { level_has_fog = bNewVal; }

	const bool	IsServerActive() const { return active; }
	void		SetServerActive(bool bNewVal) { active = bNewVal; }

	const bool	IsServerPaused() const { return paused; }
	void		SetServerPaused(bool bNewVal) { paused = bNewVal; }

    edict_t*	GetServerEdicts() { return edicts; }
	const double 	GetServerTime() const { return time; }

	void		SetServerTime(double dNewVal) { time = dNewVal; }

	const char*	GetMapName() const { return name; }
	const char*	GetMapFileName() const { return modelname; }

	const int	GetNumEdicts() const { return num_edicts; }
    edict_t*    GetEdict(int pos) { return &edicts[pos]; }
	const int	GetMaxEdicts() const { return max_edicts; }

    const char** GetModelPrecache() { return model_precache; }
    const char** GetSoundPrecache() { return sound_precache; }

    const char* GetModelPrecacheEntry(int pos) const { return model_precache[pos]; }
    void 		SetModelPrecacheEntry(int pos, const char* pszModel) { model_precache[pos] = pszModel; }

    const char* GetSoundPrecacheEntry(int pos) const { return sound_precache[pos]; }
	void 		SetSoundPrecacheEntry(int pos, const char* pszSound) { sound_precache[pos] = pszSound; }

	const char** 	GetLightStyles() { return lightstyles; }
	void 			SetLightStyle(int pos, const char* pszStyle) { lightstyles[pos] = pszStyle; }

	const server_state_t GetServerState() const { return state; }

	struct model_s** GetModels() { return models; }
	struct model_s* GetWorldModel() { return worldmodel; }

    struct model_s* GetModelEntry(int pos) const { return models[pos]; }
    void        SetModelEntry(int pos, struct model_s* pModel) { models[pos] = pModel; }

	void 		SetNumEdicts(int iNewVal) { num_edicts = iNewVal; }
	void 		IncrementEdicts() { num_edicts++; }

	const bool	IsLoadGame() const { return loadgame; }
	void		SetLoadGame(bool bNewVal) { loadgame = bNewVal; }

	sizebuf_t&	GetSignOnBuffer() { return signon; }
	const void*	GetSignOnBufferData() { return signon.data; }
    const int   GetSignOnBufferCursize() const { return signon.cursize; }
    sizebuf_t&	GetReliableDatagramBuffer() { return reliable_datagram; }

	sizebuf_t&	GetDatagramBuffer() { return datagram; }

	int			GetLastCheck() { return lastcheck; }
	double		GetLastCheckTime() { return lastchecktime; }

	void		SetLastCheck(int iNewVal) { lastcheck = iNewVal; }
	void		SetLastCheckTime(double dNewVal) { lastchecktime = dNewVal; }

private:

	bool		level_has_fog;

	bool		active;				// false if only a net client
	bool		paused;
	bool		loadgame;			// handle connections specially

	double		time;

	int			lastcheck;			// used by PF_checkclient
	double		lastchecktime;

	char		name[64];			// map name
#ifdef QUAKE2
	char		startspot[64];
#endif
	char		modelname[64];		// maps/<name>.bsp, for model_precache[0]
	struct model_s* worldmodel;
	const char* model_precache[MAX_MODELS];	// NULL terminated
	struct model_s* models[MAX_MODELS];
	const char* sound_precache[MAX_SOUNDS];	// NULL terminated
	const char* lightstyles[MAX_LIGHTSTYLES];
	int			num_edicts;
	int			max_edicts;
	edict_t*	edicts;				// can NOT be array indexed, because
									// edict_t is variable sized, but can
									// be used to reference the world ent
	server_state_t	state;			// some actions are only valid during load

	sizebuf_t	datagram;
	byte		datagram_buf[MAX_DATAGRAM];

	sizebuf_t	reliable_datagram;	// copied to all clients at end of frame
	byte		reliable_datagram_buf[MAX_DATAGRAM];

	sizebuf_t	signon;
	byte		signon_buf[64000];

};

//============================================================================

extern	cvar_t	teamplay;
extern	cvar_t	skill;
extern	cvar_t	deathmatch;
extern	cvar_t	coop;
extern	cvar_t	fraglimit;
extern	cvar_t	timelimit;

extern	server_static_t	svs;				// persistant server info
extern	CQuakeServer*		sv;					// local server

extern	client_t	*host_client;

extern	double		host_time;

extern	edict_t		*sv_player;

extern  cvar_t      sv_cheats;

//===========================================================

#endif
