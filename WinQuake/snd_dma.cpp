/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2021-2024 Stephen "Missi" Schimedeberg

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
// snd_dma.c -- main control for any streaming sound output device

#include "quakedef.h"
#include "in_win.h"
#include "snd_codec.h"

#ifdef _WIN32
#include "winquake.h"
#endif

int CSoundDMA::paintedtime = 0;
int CSoundDMA::paintedtime_voice = 0;
float CSoundDMA::sound_nominal_clip_dist = 1000.0f;

// =======================================================================
// Internal sound data & structures
// =======================================================================

cvar_t bgmvolume = {"bgmvolume", "1", true};
cvar_t volume = {"volume", "0.7", true};

cvar_t nosound = {"nosound", "0"};
cvar_t precache = {"precache", "1"};
cvar_t loadas8bit = {"loadas8bit", "0"};
cvar_t bgmbuffer = {"bgmbuffer", "4096"};
cvar_t ambient_level = {"ambient_level", "0.3"};
cvar_t ambient_fade = {"ambient_fade", "100"};
cvar_t snd_noextraupdate = {"snd_noextraupdate", "0"};
cvar_t snd_show = {"snd_show", "0"};
cvar_t _snd_mixahead = {"_snd_mixahead", "0.1", true};

cvar_t voice_loopback = {"voice_loopback", "0", false};

#if defined(_WIN32)
#define SND_FILTERQUALITY_DEFAULT "5"
#else
#define SND_FILTERQUALITY_DEFAULT "1"
#endif

cvar_t		snd_filterquality = { "snd_filterquality", SND_FILTERQUALITY_DEFAULT};
cvar_t		snd_mixspeed = { "snd_mixspeed", "44100" };
cvar_t		snd_speed = { "snd_speed", "11025" };


channel_t CSoundDMA::channels[128];
int CSoundDMA::total_channels = 0;
int CSoundDMA::num_sfx = 0;
int CSoundDMA::sound_started = 0;
vec3_t listener_origin = {};
vec3_t listener_forward = {};
vec3_t listener_right = {};
vec3_t listener_up = {};
sfx_t* CSoundDMA::known_sfx[MAX_SFX] = { (sfx_t*)calloc(1, sizeof(sfx_t)) };

SDL_AudioDeviceID g_SoundDeviceID;
SDL_AudioDeviceID g_SoundDeviceID_voice;

// ====================================================================
// User-setable variables
// ====================================================================

//
// Fake dma is a synchronous faking of the DMA progress used for
// isolating performance in the renderer.  The fakedma_updates is
// number of times S_Update() is called per second.
//

bool fakedma = false;
int fakedma_updates = 15;


void CSoundDMA::S_AmbientOff ()
{
	g_SoundSystem->snd_ambient = false;
}


void CSoundDMA::S_AmbientOn ()
{
	snd_ambient = true;
}

void CSoundDMA::S_SoundInfo_f()
{
	g_SoundSystem->S_SoundInfo();
}

void CSoundDMA::S_SoundInfo()
{
    if (!sound_started || !shm)
	{
		Con_Printf ("sound system not started\n");
		return;
	}
	
    Con_Printf("%5d stereo\n", shm->channels - 1);
    Con_Printf("%5d samples\n", shm->samples);
    Con_Printf("%5d samplepos\n", shm->samplepos);
    Con_Printf("%5d samplebits\n", shm->samplebits);
    Con_Printf("%5d submission_chunk\n", shm->submission_chunk);
    Con_Printf("%5d speed\n", shm->speed);
    Con_Printf("0x%x dma buffer\n", shm->buffer);
	Con_Printf("%5d total_channels\n", total_channels);
}

/*
================
S_Startup
================
*/

void CSoundDMA::S_Startup ()
{
	if (!snd_initialized)
		return;

    sound_started = g_SoundSystem->SNDDMA_Init(&sn);
	if (!sound_started)
	{
		Con_Printf("Failed initializing sound\n");
	}
	else
	{
		Con_Printf("Audio: %d bit, %s, %d Hz\n",
			shm->samplebits,
			(shm->channels == 2) ? "stereo" : "mono",
			shm->speed);
	}
}
/*
================
S_Init
================
*/
void CSoundDMA::S_Init ()
{

	Con_Printf("\nSound Initialization\n");

	if (g_Common->COM_CheckParm("-nosound"))
		return;

	if (g_Common->COM_CheckParm("-simsound"))
		fakedma = true;

	
	g_pCmds->Cmd_AddCommand("play", &CSoundDMA::S_Play);
	g_pCmds->Cmd_AddCommand("playvol", &CSoundDMA::S_PlayVol);
	g_pCmds->Cmd_AddCommand("stopsound", &CSoundDMA::S_StopAllSoundsC);
	g_pCmds->Cmd_AddCommand("soundlist", &CSoundDMA::S_SoundList);
	g_pCmds->Cmd_AddCommand("soundinfo", &CSoundDMA::S_SoundInfo_f);

		// Missi: enable this later
	Cvar_RegisterVariable(&nosound);
	Cvar_RegisterVariable(&volume);
	Cvar_RegisterVariable(&precache);
	Cvar_RegisterVariable(&loadas8bit);
	Cvar_RegisterVariable(&bgmvolume);
	Cvar_RegisterVariable(&bgmbuffer);
	Cvar_RegisterVariable(&ambient_level);
	Cvar_RegisterVariable(&ambient_fade);
	Cvar_RegisterVariable(&snd_noextraupdate);
	Cvar_RegisterVariable(&snd_show);
	Cvar_RegisterVariable(&_snd_mixahead);
	Cvar_RegisterVariable(&snd_mixspeed);
	Cvar_RegisterVariable(&snd_speed);
	Cvar_RegisterVariable(&snd_filterquality);
	Cvar_RegisterVariable(&voice_loopback);

	if (host->host_parms.memsize < 0x800000)
	{
		Cvar_Set ("loadas8bit", "1");
		Con_Printf ("loading all sounds as 8bit\n");
	}

	snd_initialized = true;

	S_Startup ();

	SND_InitScaletable ();

	//known_sfx = static_cast<sfx_t*>(g_MemCache->Hunk_AllocName(MAX_SFX * sizeof(sfx_t), "sfx_t"));

	num_sfx = 0;

// create a piece of DMA memory

	if (fakedma)
	{
        shm = g_MemCache->Hunk_AllocName<dma_t>(sizeof(*shm), "shm");
		shm->splitbuffer = 0;
		shm->samplebits = 16;
		shm->speed = 22050;
		shm->channels = 2;
		shm->samples = 32768;
		shm->samplepos = 0;
		shm->soundalive = true;
		shm->gamealive = true;
		shm->submission_chunk = 1;
		shm->buffer = g_MemCache->Hunk_AllocName<unsigned char>(1<<16, "shmbuf");
	}

	Con_Printf ("Sound sampling rate: %i\n", shm->speed);

	ambient_sfx[AMBIENT_WATER] = S_PrecacheSound("ambience/water1.wav");
	ambient_sfx[AMBIENT_SKY] = S_PrecacheSound("ambience/wind2.wav");

	S_CodecInit();

	S_StopAllSounds (true);
}


// =======================================================================
// Shutdown sound engine
// =======================================================================

void CSoundDMA::S_Shutdown()
{

	if (!sound_started)
		return;

	if (shm)
		shm->gamealive = 0;

	shm = 0;
	sound_started = 0;

	if (!fakedma)
	{
        g_SoundSystem->SNDDMA_Shutdown();
	}
}


// =======================================================================
// Load a sound
// =======================================================================

/*
==================
S_FindName

==================
*/
sfx_t* CSoundDMA::S_FindName (const char *name)
{
	int		i;
	sfx_t*	sfx;

	if (!name)
		Sys_Error ("S_FindName: NULL\n");

	if (Q_strlen(name) >= MAX_QPATH)
		Sys_Error ("Sound name too long: %s", name);

// see if already loaded
	for (i=0 ; i < num_sfx ; i++)
		if (!Q_strcmp(known_sfx[i]->name, name))
		{
			return known_sfx[i];
		}

	if (num_sfx == MAX_SFX)
		Sys_Error ("S_FindName: out of sfx_t");
	
	sfx = known_sfx[i];
	Q_strcpy (sfx->name, name);

	num_sfx++;
	
	return sfx;
}


/*
==================
S_TouchSound

==================
*/
void CSoundDMA::S_TouchSound (const char *name)
{
	sfx_t	*sfx;
	
	if (!sound_started)
		return;

	sfx = S_FindName (name);
	g_MemCache->Cache_Check<sfx_t>(&sfx->cache);
}

/*
==================
S_PrecacheSound

==================
*/
sfx_t* CSoundDMA::S_PrecacheSound (const char *name)
{
	sfx_t	*sfx;

	if (!sound_started || nosound.value)
		return NULL;

	sfx = S_FindName (name);
	
// cache it in
	if (precache.value)
		S_LoadSound (sfx);
	
	return sfx;
}


//=============================================================================

/*
=================
SND_PickChannel
=================
*/
channel_t* CSoundDMA::SND_PickChannel(int entnum, int entchannel)
{
    int ch_idx;
    int first_to_die;
    int life_left;

// Check for replacement sound, or find the best one to replace
    first_to_die = -1;
    life_left = 0x7fffffff;
    for (ch_idx=NUM_AMBIENTS ; ch_idx < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS ; ch_idx++)
    {
		if (entchannel != 0		// channel 0 never overrides
		&& channels[ch_idx].entnum == entnum
		&& (channels[ch_idx].entchannel == entchannel || entchannel == -1) )
		{	// allways override sound from same entity
			first_to_die = ch_idx;
			break;
		}

		// don't let monster sounds override player sounds
		if (channels[ch_idx].entnum == cl.viewentity && entnum != cl.viewentity && channels[ch_idx].sfx)
			continue;

		if (channels[ch_idx].end - paintedtime < life_left)
		{
			life_left = channels[ch_idx].end - paintedtime;
			first_to_die = ch_idx;
		}
   }

	if (first_to_die == -1)
		return NULL;

	if (channels[first_to_die].sfx)
		channels[first_to_die].sfx = NULL;

    return &channels[first_to_die];    
}       

/*
=================
SND_Spatialize
=================
*/
void CSoundDMA::SND_Spatialize(channel_t *ch)
{
    vec_t dot;
    vec_t dist;
    vec_t lscale, rscale, scale;
    vec3_t source_vec;
	sfx_t *snd;

// anything coming from the view entity will allways be full volume
	if (ch->entnum == cl.viewentity)
	{
		ch->leftvol = ch->master_vol;
		ch->rightvol = ch->master_vol;
		return;
	}

// calculate stereo seperation and distance attenuation

	snd = ch->sfx;
	VectorSubtract(ch->origin, listener_origin, source_vec);
	
	dist = VectorNormalize(source_vec) * ch->dist_mult;
	
	dot = DotProduct(listener_right, source_vec);

	if (shm->channels == 1)
	{
		rscale = 1.0;
		lscale = 1.0;
	}
	else
	{
		rscale = 1.0 + dot;
		lscale = 1.0 - dot;
	}

// add in distance effect
	scale = (1.0 - dist) * rscale;
	ch->rightvol = (int) (ch->master_vol * scale);
	if (ch->rightvol < 0)
		ch->rightvol = 0;

	scale = (1.0 - dist) * lscale;
	ch->leftvol = (int) (ch->master_vol * scale);
	if (ch->leftvol < 0)
		ch->leftvol = 0;
}           


// =======================================================================
// Start a sound effect
// =======================================================================

void CSoundDMA::S_StartSound(int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation)
{
	channel_t *target_chan, *check;
	sfxcache_t	*sc;
	int		vol;
	int		ch_idx;
	int		skip;

	if (!sound_started)
		return;

	if (!sfx)
		return;

	if (nosound.value)
		return;

	vol = fvol*255;

// pick a channel to play on
	target_chan = SND_PickChannel(entnum, entchannel);
	if (!target_chan)
		return;
		
// spatialize
	memset (target_chan, 0, sizeof(*target_chan));
	VectorCopy(origin, target_chan->origin);
	target_chan->dist_mult = attenuation / sound_nominal_clip_dist;
	target_chan->master_vol = vol;
	target_chan->entnum = entnum;
	target_chan->entchannel = entchannel;
	SND_Spatialize(target_chan);

	if (!target_chan->leftvol && !target_chan->rightvol)
		return;		// not audible at all

// new channel
	sc = S_LoadSound (sfx);
	if (!sc)
	{
		target_chan->sfx = NULL;
		return;		// couldn't load the sound's data
	}

	target_chan->sfx = sfx;
	target_chan->pos = 0.0;
    target_chan->end = paintedtime + sc->length;	

// if an identical sound has also been started this frame, offset the pos
// a bit to keep it from just making the first one louder
	check = &channels[NUM_AMBIENTS];
	for (ch_idx = NUM_AMBIENTS; ch_idx < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS; ch_idx++, check++)
	{
		if (check == target_chan)
			continue;
		if (check->sfx == sfx && !check->pos)
		{
			/*
			skip = rand () % (int)(0.1 * shm->speed);
			if (skip >= target_chan->end)
				skip = target_chan->end - 1;
			*/
			/* Missi: implemented LadyHavoc's skip fixes */
			skip = 0.1 * shm->speed; /* 0.1 * sc->speed */
			if (skip > sc->length)
				skip = sc->length;
			if (skip > 0)
				skip = rand() % skip;
			target_chan->pos += skip;
			target_chan->end -= skip;
			break;
		}
	}
}

void CSoundDMA::S_StopSound(int entnum, int entchannel)
{
	int i;

	for (i=0 ; i<MAX_DYNAMIC_CHANNELS ; i++)
	{
		if (channels[i].entnum == entnum
			&& channels[i].entchannel == entchannel)
		{
			channels[i].end = 0;
			channels[i].sfx = NULL;
			return;
		}
	}
}

void CSoundDMA::S_StopAllSounds(bool clear)
{
	int		i;

	if (!sound_started)
		return;

	total_channels = MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS;	// no statics

	for (i=0 ; i<MAX_CHANNELS ; i++)
		if (channels[i].sfx)
			channels[i].sfx = NULL;

	Q_memset(channels, 0, MAX_CHANNELS * sizeof(channel_t));

	if (clear)
		g_SoundSystem->S_ClearBuffer ();
}

void CSoundDMA::S_StopAllSoundsC ()
{
	S_StopAllSounds (true);
}

void CSoundDMA::S_ClearBuffer ()
{
	int		clear;

	if (!sound_started || !shm)
		return;

	if (shm->samplebits == 8)
		clear = 0x80;
	else
		clear = 0;

	g_SoundSystem->SNDDMA_LockBuffer();
	if (!shm->buffer)
		return;

	Q_memset(shm->buffer, clear, shm->samples * shm->samplebits / 8);

	g_SoundSystem->SNDDMA_UnlockBuffer();

}


/*
=================
S_StaticSound
=================
*/
void CSoundDMA::S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation)
{
	channel_t	*ss;
	sfxcache_t		*sc;

	if (!sfx)
		return;

	if (total_channels == MAX_CHANNELS)
	{
		Con_Printf ("total_channels == MAX_CHANNELS\n");
		return;
	}

	ss = &channels[total_channels];
	total_channels++;

	sc = S_LoadSound (sfx);
	if (!sc)
		return;

	if (sc->loopstart == -1)
	{
		Con_Printf ("Sound %s not looped\n", sfx->name);
		return;
	}
	
	ss->sfx = sfx;
	VectorCopy (origin, ss->origin);
	ss->master_vol = vol;
	ss->dist_mult = (attenuation/64) / sound_nominal_clip_dist;
    ss->end = paintedtime + sc->length;	
	
	SND_Spatialize (ss);
}


//=============================================================================

/*
===================
S_UpdateAmbientSounds
===================
*/
void CSoundDMA::S_UpdateAmbientSounds ()
{
	mleaf_t		*l = NULL;
	float		vol;
	int			ambient_channel;
	channel_t	*chan = NULL;

	if (!snd_ambient)
		return;

// calc ambient sound levels
	if (!cl.worldmodel)
		return;

	l = Mod_PointInLeaf (listener_origin, cl.worldmodel);
	if (!l || !ambient_level.value)
	{
		for (ambient_channel = 0 ; ambient_channel< NUM_AMBIENTS ; ambient_channel++)
			channels[ambient_channel].sfx = NULL;
		return;
	}

	for (ambient_channel = 0 ; ambient_channel< NUM_AMBIENTS ; ambient_channel++)
	{
		chan = &channels[ambient_channel];	
		chan->sfx = ambient_sfx[ambient_channel]; // Missi: caught ya, you bastard
	
		vol = ambient_level.value * l->ambient_sound_level[ambient_channel];
		if (vol < 8)
			vol = 0;

	// don't adjust volume too fast
		if (chan->master_vol < vol)
		{
			chan->master_vol += host->host_frametime * ambient_fade.value;
			if (chan->master_vol > vol)
				chan->master_vol = vol;
		}
		else if (chan->master_vol > vol)
		{
			chan->master_vol -= host->host_frametime * ambient_fade.value;
			if (chan->master_vol < vol)
				chan->master_vol = vol;
		}
		
		chan->leftvol = chan->rightvol = chan->master_vol;
	}
}

/*
===================
S_RawSamples		(from QuakeII)

Streaming music support. Byte swapping
of data must be handled by the codec.
Expects data in signed 16 bit, or unsigned
8 bit format.
===================
*/
void CSoundDMA::S_RawSamples(int samples, int rate, int width, int channels, byte* data, float volume)
{
	int i;
	int src, dst;
	float scale;
	int intVolume;

	if (s_rawend < paintedtime)
		s_rawend = paintedtime;

	scale = (float)rate / shm->speed;
	intVolume = (int)(256 * volume);

	if (channels == 2 && width == 2)
	{
		for (i = 0; ; i++)
		{
			src = i * scale;
			if (src >= samples)
				break;
			dst = s_rawend & (MAX_RAW_SAMPLES - 1);
			s_rawend++;
			s_rawsamples[dst].left = ((short*)data)[src * 2] * intVolume;
			s_rawsamples[dst].right = ((short*)data)[src * 2 + 1] * intVolume;
		}
	}
	else if (channels == 1 && width == 2)
	{
		for (i = 0; ; i++)
		{
			src = i * scale;
			if (src >= samples)
				break;
			dst = s_rawend & (MAX_RAW_SAMPLES - 1);
			s_rawend++;
			s_rawsamples[dst].left = ((short*)data)[src] * intVolume;
			s_rawsamples[dst].right = ((short*)data)[src] * intVolume;
		}
	}
	else if (channels == 2 && width == 1)
	{
		intVolume *= 256;

		for (i = 0; ; i++)
		{
			src = i * scale;
			if (src >= samples)
				break;
			dst = s_rawend & (MAX_RAW_SAMPLES - 1);
			s_rawend++;
			//	s_rawsamples [dst].left = ((signed char *) data)[src * 2] * intVolume;
			//	s_rawsamples [dst].right = ((signed char *) data)[src * 2 + 1] * intVolume;
			s_rawsamples[dst].left = (((byte*)data)[src * 2] - 128) * intVolume;
			s_rawsamples[dst].right = (((byte*)data)[src * 2 + 1] - 128) * intVolume;
		}
	}
	else if (channels == 1 && width == 1)
	{
		intVolume *= 256;

		for (i = 0; ; i++)
		{
			src = i * scale;
			if (src >= samples)
				break;
			dst = s_rawend & (MAX_RAW_SAMPLES - 1);
			s_rawend++;
			//	s_rawsamples [dst].left = ((signed char *) data)[src] * intVolume;
			//	s_rawsamples [dst].right = ((signed char *) data)[src] * intVolume;
			s_rawsamples[dst].left = (((byte*)data)[src] - 128) * intVolume;
			s_rawsamples[dst].right = (((byte*)data)[src] - 128) * intVolume;
		}
	}
}

/*
============
S_Update

Called once each time through the main loop
============
*/
void CSoundDMA::S_Update(vec3_t origin, vec3_t forward, vec3_t right, vec3_t up)
{
	int			i, j;
	int			total;
	channel_t	*ch;
	channel_t	*combine;

	if (!sound_started || (snd_blocked > 0))
		return;

	VectorCopy(origin, listener_origin);
	VectorCopy(forward, listener_forward);
	VectorCopy(right, listener_right);
	VectorCopy(up, listener_up);
	
// update general area ambient sound sources
	S_UpdateAmbientSounds ();

	combine = NULL;

// update spatialization for static and dynamic sounds	
	ch = channels+NUM_AMBIENTS;
	for (i=NUM_AMBIENTS ; i<total_channels; i++, ch++)
	{
		if (!ch->sfx)
			continue;
		SND_Spatialize(ch);         // respatialize channel
		if (!ch->leftvol && !ch->rightvol)
			continue;

	// try to combine static sounds with a previous channel of the same
	// sound effect so we don't mix five torches every frame
	
		if (i >= MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS)
		{
		// see if it can just use the last one
			if (combine && combine->sfx == ch->sfx)
			{
				combine->leftvol += ch->leftvol;
				combine->rightvol += ch->rightvol;
				ch->leftvol = ch->rightvol = 0;
				continue;
			}
		// search for one
			combine = channels+MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS;
			for (j=MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS ; j<i; j++, combine++)
				if (combine->sfx == ch->sfx)
					break;
					
			if (j == total_channels)
			{
				combine = NULL;
			}
			else
			{
				if (combine != ch)
				{
					combine->leftvol += ch->leftvol;
					combine->rightvol += ch->rightvol;
					ch->leftvol = ch->rightvol = 0;
				}
				continue;
			}
		}
		
		
	}

//
// debugging output
//
	if (snd_show.value)
	{
		total = 0;
		ch = channels;
		for (i=0 ; i<total_channels; i++, ch++)
			if (ch->sfx && (ch->leftvol || ch->rightvol) )
			{
				Con_Printf ("%3i %3i %s\n", ch->leftvol, ch->rightvol, ch->sfx->name);
				total++;
			}
		
		Con_Printf ("----(%i)----\n", total);
	}

// mix some sound
	S_Update_();
}

void CSoundDMA::S_CheckMDMAMusic()
{
	if (cl.items & IT_QUAD && !songplaying)
	{
		oldtrack = cl.cdtrack;
		oldlooptrack = cl.looptrack;

		cl.cdtrack = 12;
		cl.looptrack = true;
		g_pBGM->BGM_PlayCDtrack((byte)cl.cdtrack, cl.looptrack);
		oldsongplaying = false;
		songplaying = true;
	}
	else if (!(cl.items & IT_QUAD) && !oldsongplaying)
	{
		cl.cdtrack = oldtrack;
		cl.looptrack = oldlooptrack;
		g_pBGM->BGM_PlayCDtrack((byte)cl.cdtrack, cl.looptrack);
		oldsongplaying = true;
		songplaying = false;
	}
}

void CSoundDMA::GetSoundtime()
{
	int		samplepos;
	static	int		buffers;
	static	int		oldsamplepos;
	int		fullsamples;
	
	fullsamples = shm->samples / shm->channels;

// it is possible to miscount buffers if it has wrapped twice between
// calls to S_Update.  Oh well.

    samplepos = SNDDMA_GetDMAPos();

	if (samplepos < oldsamplepos)
	{
		buffers++;					// buffer wrapped
		
		if (paintedtime > 0x40000000)
		{	// time to chop things off to avoid 32 bit limits
			buffers = 0;
			paintedtime = fullsamples;
			S_StopAllSounds (true);
		}
	}
	oldsamplepos = samplepos;

	soundtime = buffers*fullsamples + samplepos/shm->channels;

}

void CSoundDMA::S_ExtraUpdate ()
{

#ifdef _WIN32
	IN_Accumulate ();
#endif

	if (snd_noextraupdate.value)
		return;		// don't pollute timings
	S_Update_();
}

void CSoundDMA::S_Update_()
{
	unsigned int	endtime;
	int				samps;
	
	if (!sound_started || (snd_blocked > 0))
		return;

	SNDDMA_LockBuffer();
	if (!shm->buffer)
		return;

// Updates DMA time
	GetSoundtime();

// check to make sure that we haven't overshot
	if (paintedtime < soundtime)
	{
		//Con_Printf ("S_Update_ : overflow\n");
		paintedtime = soundtime;
	}

// mix ahead of current position

	endtime = soundtime + (unsigned int)(_snd_mixahead.value * shm->speed);
	samps = shm->samples >> (shm->channels - 1);
	endtime = q_min(endtime, (unsigned int)(soundtime + samps));

    S_PaintChannels (endtime);

 	SNDDMA_Submit ();

}

void CSoundDMA::SNDDMA_LockBuffer()
{
	SDL_LockAudioDevice(g_SoundDeviceID);
}

void CSoundDMA::SNDDMA_UnlockBuffer()
{
	SDL_UnlockAudioDevice(g_SoundDeviceID);
}

/*
===============================================================================

console functions

===============================================================================
*/

CSoundDMA::CSoundDMA() : snd_blocked(0),
	snd_ambient(true), 
	snd_initialized(false), 
	soundtime(0),  
	desired_speed(11025),
	desired_bits(16), 
	gSndBufSize(0), 
	mmstarttime(0.f),
	songplaying(false),
	oldsongplaying(false),
	curtrack(0),
	curlooptrack(0),
	oldtrack(0),
	oldlooptrack(0),
	s_rawend(0),
	unused(0)
{
	for (int i = 0; i < NUM_AMBIENTS; i++)
		ambient_sfx[i] = NULL;

	for (int i = 0; i < MAX_SFX; i++)
		known_sfx[i] = (sfx_t*)calloc(1, sizeof(sfx_t));

	memset(&listener_origin, 0, sizeof(vec3_t));
	memset(&listener_forward, 0, sizeof(vec3_t));
	memset(&listener_right, 0, sizeof(vec3_t));
	memset(&listener_up, 0, sizeof(vec3_t));

	memset(&sn, 0, sizeof(dma_t));
}

CSoundDMA::~CSoundDMA()
{
	for (int i = 0; i < NUM_AMBIENTS; i++)
		ambient_sfx[i] = NULL;

	for (int i = 0; i < MAX_SFX; i++)
		known_sfx[i] = (sfx_t*)calloc(1, sizeof(sfx_t));

	memset(&listener_origin, 0, sizeof(vec3_t));
	memset(&listener_forward, 0, sizeof(vec3_t));
	memset(&listener_right, 0, sizeof(vec3_t));
	memset(&listener_up, 0, sizeof(vec3_t));

	memset(&sn, 0, sizeof(dma_t));

	snd_blocked = 0;
	snd_ambient = false;
	snd_initialized = false;
	soundtime = 0;
	desired_speed = 0;
	desired_bits = 0;
	gSndBufSize = 0;
	mmstarttime = 0.0f;
	songplaying = false;
	oldsongplaying = false;
	curtrack = 0;
	curlooptrack = 0;
	oldtrack = 0;
	oldlooptrack = 0;
	s_rawend = 0;
	unused = 0;
}

void CSoundDMA::S_Play()
{
	static int hash = 345;
	int 	i;
	char name[256];
	sfx_t* sfx;

	i = 1;
	while (i < g_pCmds->Cmd_Argc())
	{
		if (!Q_strrchr(g_pCmds->Cmd_Argv(i), '.'))
		{
			Q_strcpy(name, g_pCmds->Cmd_Argv(i));
			Q_strcat(name, ".wav");
		}
		else
			Q_strcpy(name, g_pCmds->Cmd_Argv(i));
		sfx = S_PrecacheSound(name);
		S_StartSound(hash++, 0, sfx, listener_origin, 1.0, 1.0);
		i++;
	}
}

void CSoundDMA::S_PlayVol()
{
	static int hash=543;
	int i;
	float vol;
	char name[256];
	sfx_t	*sfx;
	
	i = 1;
	while (i< g_pCmds->Cmd_Argc())
	{
		if (!Q_strrchr(g_pCmds->Cmd_Argv(i), '.'))
		{
			Q_strcpy(name, g_pCmds->Cmd_Argv(i));
			Q_strcat(name, ".wav");
		}
		else
			Q_strcpy(name, g_pCmds->Cmd_Argv(i));
		sfx = S_PrecacheSound(name);
		vol = Q_atof(g_pCmds->Cmd_Argv(i+1));
		S_StartSound(hash++, 0, sfx, listener_origin, vol, 1.0);
		i+=2;
	}
}

void CSoundDMA::S_SoundList()
{
	int		i;
	sfx_t	*sfx;
	sfxcache_t	*sc;
	int		size, total;

	total = 0;
	for (i=0; i<num_sfx; i++, sfx++)
	{
		sfx = known_sfx[i];
		sc = g_MemCache->Cache_Check<sfxcache_t>(&sfx->cache);
		if (!sc)
			continue;
		size = sc->length*sc->width*(sc->stereo+1);
		total += size;
		if (sc->loopstart >= 0)
			Con_Printf ("L");
		else
			Con_Printf (" ");
		Con_Printf("(%2db) %6i : %s\n",sc->width*8,  size, sfx->name);
	}
	Con_Printf ("Total resident: %i\n", total);
}


void CSoundDMA::S_LocalSound (const char *sound)
{
	sfx_t	*sfx;

	if (nosound.value)
		return;
	if (!sound_started)
		return;
		
	sfx = g_SoundSystem->S_PrecacheSound (sound);
	if (!sfx)
	{
		Con_Printf ("g_SoundSystem->S_LocalSound: can't cache %s\n", sound);
		return;
	}
	g_SoundSystem->S_StartSound (cl.viewentity, -1, sfx, vec3_origin, 1, 1);
}

void CSoundDMA::S_BlockSound()
{
    SDL_PauseAudio(1);
}

void CSoundDMA::S_UnblockSound()
{
    SDL_PauseAudio(0);
}

void CSoundDMA::S_ClearPrecache ()
{
}


void CSoundDMA::S_BeginPrecaching ()
{
}


void CSoundDMA::S_EndPrecaching ()
{
}

