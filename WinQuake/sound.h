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
// sound.h -- client sound i/o functions

#ifndef __SOUND__
#define __SOUND__

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

// ====================================================================
// User-setable variables
// ====================================================================

#define	MAX_CHANNELS			128
#define	MAX_DYNAMIC_CHANNELS	8

// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct
{
	int left;
	int right;
} portable_samplepair_t;

struct sfx_t
{
	char 	name[MAX_QPATH] = { NULL };
	cache_user_t	cache = { NULL };
};

// !!! if this is changed, it much be changed in asm_i386.h too !!!
// Missi: needs to be un-typedef'd to avoid cache issues
struct sfxcache_t
{
	int 	length = 0;
	int 	loopstart = 0;
	int 	speed = 0;
	int 	width = 0;
	int 	stereo = 0;
	byte	data[1];		// variable sized
};

typedef struct
{
	bool		gamealive;
	bool		soundalive;
	bool		splitbuffer;
	bool		signed8;
	int				channels;
	int				samples;				// mono samples in buffer
	int				submission_chunk;		// don't mix less than this #
	int				samplepos;				// in mono samples
	int				samplebits;
	int				speed;
	unsigned char	*buffer;
} dma_t;

// !!! if this is changed, it much be changed in asm_i386.h too !!!
struct channel_t
{
	public:
		sfx_t	*sfx = NULL;			// sfx number
		int		leftvol = 0;		// 0-255 volume
		int		rightvol = 0;		// 0-255 volume
		int		end = 0;			// end time in global paintsamples
		int 	pos = 0;			// sample position in sfx
		int		looping = 0;		// where to loop, -1 = no looping
		int		entnum = 0;			// to allow overriding a specific sound
		int		entchannel = 0;		//
		vec3_t	origin = { 0, 0, 0 };			// origin of sound effect
		vec_t	dist_mult;		// distance multiplier (attenuation/clipK)
		int		master_vol = 0;		// 0-255 master volume
};

typedef struct
{
	int		rate;
	int		width;
	int		channels;
	int		loopstart;
	int		samples;
	int		dataofs;		// chunk starts this many bytes from file start
} wavinfo_t;

class CSoundInternal
{
public:

	CSoundInternal();

	static channel_t   channels[MAX_CHANNELS];
	static int	total_channels;

	int			snd_blocked = 0;
	bool		snd_ambient = 1;
	bool		snd_initialized = false;

	static void S_Play(void);
	static void S_PlayVol(void);
	static void S_SoundList(void);
	static void S_StopAllSoundsC(void);
	virtual void S_Update_();
	static void S_StopAllSounds(bool clear);
	static void S_SoundInfo_f(void);

	// pointer should go away
	static volatile dma_t* shm;
	dma_t sn;

	static vec3_t		listener_origin;
	static vec3_t		listener_forward;
	static vec3_t		listener_right;
	static vec3_t		listener_up;
	vec_t		sound_nominal_clip_dist = 1000.0;

	int			soundtime;		// sample PAIRS
	int   		paintedtime; 	// sample PAIRS


#define	MAX_SFX		512
	static sfx_t* known_sfx;		// hunk allocated [MAX_SFX]
	static int			num_sfx;

	CQVector<sfx_t*> ambient_sfx;

	int 		desired_speed = 11025;
	int 		desired_bits = 16;

	static int sound_started;

	DWORD	gSndBufSize;

	MMTIME		mmstarttime;

};

class CSoundSystemWin : public CSoundInternal
{
public:

	typedef void (*snd_callback)(void);

	CSoundSystemWin();

	// Global crap

	channel_t   channels[MAX_CHANNELS];
	DWORD		gSndBufSize = 0;
	int paintedtime = 0;

	void S_Init(void);
	void S_Startup(void);
	void S_Shutdown(void);
	void S_StartSound(int entnum, int entchannel, sfx_t* sfx, vec3_t origin, float fvol, float attenuation);
	void S_StaticSound(sfx_t* sfx, vec3_t origin, float vol, float attenuation);
	void S_UpdateAmbientSounds(void);
	void S_StopSound(int entnum, int entchannel);
	//void S_StopAllSounds(bool clear);
	//void S_StopAllSoundsC(void);
	void S_ClearBuffer(void);
	void S_Update(vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);
	void GetSoundtime(void);
	void S_ExtraUpdate(void);

	//void S_Update_(void);

	sfx_t* S_PrecacheSound(char* sample);
	sfx_t* S_FindName(char* name);
	void S_TouchSound(char* sample);
	void S_ClearPrecache(void);
	void S_BeginPrecaching(void);
	void S_EndPrecaching(void);
	void Snd_WriteLinearBlastStereo16(void);
	void S_TransferStereo16(int endtime);
	void S_TransferPaintBuffer(int endtime);
	void S_PaintChannels(int endtime);

	void SND_PaintChannelFrom8(channel_t* ch, sfxcache_t* sc, int endtime, int paintbufferstart);
	void SND_PaintChannelFrom16(channel_t* ch, sfxcache_t* sc, int endtime, int paintbufferstart);

	// picks a channel based on priorities, empty slots, number of channels
	channel_t* SND_PickChannel(int entnum, int entchannel);

	// spatializes a channel
	void SND_Spatialize(channel_t* ch);

	// initializes cycling through a DMA buffer and returns information on it
	bool SNDDMA_Init(dma_t* dma);

	// gets the current DMA position
	int SNDDMA_GetDMAPos(void);

	// shutdown the DMA xfer.
	void SNDDMA_Shutdown(void);

	void S_LocalSound(char* s);
	void ResampleSfx(sfx_t* sfx, int inrate, int inwidth, byte* data);
	sfxcache_t* S_LoadSound(sfx_t* s);

	wavinfo_t GetWavinfo(char* name, byte* wav, int wavlength);

	void SND_InitScaletable(void);
	void SNDDMA_Submit(void);

	static void paint_audio(void* unused, Uint8* stream, int len);

	void S_AmbientOff(void);
	void S_AmbientOn(void);

	void SNDDMA_LockBuffer(void);
	void SNDDMA_UnlockBuffer(void);

	void S_BlockSound(void);
	void S_UnblockSound(void);

	/*
*  Global variables. Must be visible to window-procedure function
*  so it can unlock and free the data block after it has been played.
*/

	// void GetCaps(void) { this->GetCaps(); };

};

extern	channel_t   channels[MAX_CHANNELS];
// 0 to MAX_DYNAMIC_CHANNELS-1	= normal entity sounds
// MAX_DYNAMIC_CHANNELS to MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS -1 = water, etc
// MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS to total_channels = static sounds

extern	int			total_channels;

//
// Fake dma is a synchronous faking of the DMA progress used for
// isolating performance in the renderer.  The fakedma_updates is
// number of times S_Update() is called per second.
//

extern bool 		fakedma;
extern int 			fakedma_updates;
extern int		paintedtime;
extern vec3_t listener_origin;
extern vec3_t listener_forward;
extern vec3_t listener_right;
extern vec3_t listener_up;
//extern volatile dma_t *shm;
//extern volatile dma_t sn;
extern vec_t sound_nominal_clip_dist;

extern	cvar_t	loadas8bit;
extern	cvar_t	bgmvolume;
extern	cvar_t	volume;
extern	cvar_t	snd_filterquality;
extern	cvar_t	snd_mixspeed;
extern	cvar_t	snd_speed;

extern bool	snd_initialized;

extern int		snd_blocked;

extern CSoundSystemWin* g_SoundSystem;

extern SDL_AudioDeviceID g_SoundDeviceID;

#endif
