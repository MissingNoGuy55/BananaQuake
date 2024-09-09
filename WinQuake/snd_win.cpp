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
#include "winquake.h"
//#include <mmeapi.h>

#define iDirectSoundCreate(a,b,c)	pDirectSoundCreate(a,b,c)

// typedef HRESULT (CALLBACK *PDIRECTSOUNDCREATE)(GUID FAR *lpGUID, LPDIRECTSOUND FAR *lplpDS, IUnknown FAR *pUnkOuter);

// PDIRECTSOUNDCREATE pDirectSoundCreate;

// 64K is > 1 second at 16-bit, 22050 Hz
#define	WAV_BUFFERS				64
#define	WAV_MASK				0x3F
#define	WAV_BUFFER_SIZE			0x0400
#define SECONDARY_BUFFER_SIZE	0x10000

typedef enum {SIS_SUCCESS, SIS_FAILURE, SIS_NOTAVAIL} sndinitstat;

bool	wavonly;
bool	wav_init;
bool	snd_firsttime = true, snd_isdirect, snd_iswave;
bool	primary_format_set;

int	sample16;
int	snd_sent, snd_completed;

CSoundDMA* g_SoundSystem;
// SDL_AudioDeviceID g_SoundDeviceID;

//dma_t CSoundInternal::sn = { NULL };
dma_t* CSoundDMA::shm;
dma_t* CSoundDMA::shm_voice;

static int	buffersize;
static int	buffersize_voice;

/*
==================
S_BlockSound
==================
*/
//void CSoundDMA::S_BlockSound ()
//{
//
//	if (sound_started && snd_blocked == 0)	/* ++snd_blocked == 1 */
//	{
//		snd_blocked = 1;
//		S_ClearBuffer();
//		if (shm)
//			SDL_LockAudioDevice(g_SoundDeviceID);
//	}
//}


/*
==================
S_UnblockSound
==================
*/
//void CSoundDMA::S_UnblockSound ()
//{
//	if (!sound_started || !snd_blocked)
//		return;
//	if (snd_blocked == 1)			/* --snd_blocked == 0 */
//	{
//		snd_blocked = 0;
//		SDL_UnlockAudioDevice(g_SoundDeviceID);
//		S_ClearBuffer();
//	}
//}

/*
==================
SNDDMA_Init

Try to find a sound device to mix for.
Returns false if nothing is found.
==================
*/

bool CSoundDMA::SNDDMA_Init(dma_t* dma)
{
	SDL_AudioSpec desired = {};
	SDL_AudioSpec desired_voice = {};
	int		tmp, tmp2, val;

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		Sys_Error("Couldn't init SDL audio: %s\n", SDL_GetError());
		return false;
	}

	/* Set up the desired format */
	desired.freq = snd_mixspeed.value;
	desired.format = (loadas8bit.value) ? AUDIO_U8 : AUDIO_S16SYS;
	desired.channels = 2; /* = desired_channels; */
	if (desired.freq <= 11025)
		desired.samples = 256;
	else if (desired.freq <= 22050)
		desired.samples = 512;
	else if (desired.freq <= 44100)
		desired.samples = 1024;
	else if (desired.freq <= 56000)
		desired.samples = 2048; /* for 48 kHz */
	else
		desired.samples = 4096; /* for 96 kHz */
	desired.callback = paint_audio;
	desired.userdata = NULL;
	
	/* Set up the desired format */
	desired_voice.freq = snd_mixspeed.value;
	desired_voice.format = (loadas8bit.value) ? AUDIO_U8 : AUDIO_S16SYS;
	desired_voice.channels = 2; /* = desired_channels; */
	if (desired_voice.freq <= 11025)
		desired_voice.samples = 256;
	else if (desired_voice.freq <= 22050)
		desired_voice.samples = 512;
	else if (desired_voice.freq <= 44100)
		desired_voice.samples = 1024;
	else if (desired_voice.freq <= 56000)
		desired_voice.samples = 2048; /* for 48 kHz */
	else
		desired_voice.samples = 4096; /* for 96 kHz */
	desired_voice.callback = paint_audio_voice;
	desired_voice.userdata = NULL;

	/* Open the audio device */

	g_SoundDeviceID = SDL_OpenAudioDevice(NULL, SDL_FALSE, &desired, NULL, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

	if (g_SoundDeviceID == -1)
	{
		Con_Printf("Couldn't open SDL audio: %s\n", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return false;
	}

	const char* device = SDL_GetAudioDeviceName(g_SoundDeviceID_voice, SDL_TRUE);
	g_SoundDeviceID_voice = SDL_OpenAudioDevice(device, SDL_TRUE, &desired_voice, NULL, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

	memset((void*)dma, 0, sizeof(dma_t));
	memset((void*)&sn_voice, 0, sizeof(dma_t));

	shm = dma;
	shm_voice = &sn_voice;

	if (g_Common->COM_CheckParm ("-wavonly"))
		wavonly = true;

	wav_init = 0;

	/* Fill the audio DMA information block */
	/* Since we passed NULL as the 'obtained' spec to SDL_OpenAudio(),
	 * SDL will convert to hardware format for us if needed, hence we
	 * directly use the desired values here. */
	shm->samplebits = (desired.format & 0xFF); /* first byte of format is bits */
	shm->signed8 = (desired.format == AUDIO_S8);
	shm->speed = desired.freq;
	shm->channels = desired.channels;
 	tmp = (desired.samples * desired.channels) * 10;
	if (tmp & (tmp - 1))
	{	/* make it a power of two */
		val = 1;
		while (val < tmp)
			val <<= 1;

		tmp = val;
	}
	shm->samples = tmp;
	shm->samplepos = 0;
	shm->submission_chunk = 1;

	/* Fill the audio DMA information block for SDL voice */
	/* Since we passed NULL as the 'obtained' spec to SDL_OpenAudio(),
	 * SDL will convert to hardware format for us if needed, hence we
	 * directly use the desired values here. */
	shm_voice->samplebits = (desired_voice.format & 0xFF); /* first byte of format is bits */
	shm_voice->signed8 = (desired_voice.format == AUDIO_S8);
	shm_voice->speed = desired_voice.freq;
	shm_voice->channels = desired_voice.channels;
	tmp2 = (desired_voice.samples * desired_voice.channels) * 10;
	if (tmp2 & (tmp2 - 1))
	{	/* make it a power of two */
		val = 1;
		while (val < tmp2)
			val <<= 1;

		tmp2 = val;
	}
	shm_voice->samples = tmp;
	shm_voice->samplepos = 0;
	shm_voice->submission_chunk = 1;

	Con_Printf("SDL audio spec : %d Hz, %d samples, %d channels\n",
		desired.freq, desired.samples, desired.channels);
	Con_Printf("SDL recording audio spec : %d Hz, %d samples, %d channels\n\n",
		desired_voice.freq, desired_voice.samples, desired_voice.channels);

	buffersize = shm->samples * (shm->samplebits / 8);
	buffersize_voice = shm_voice->samples * (shm_voice->samplebits / 8);

	const char* audioDriver = SDL_GetCurrentAudioDriver();

	Con_Printf("SDL audio driver: %s, %d bytes buffer\n", audioDriver ? audioDriver : "NULL", buffersize);
	Con_Printf("SDL recording audio driver: %s, %d bytes buffer\n\n", device, buffersize_voice);

	snd_firsttime = false;

	shm->buffer = (unsigned char*)calloc(1, buffersize);
	if (!shm->buffer)
	{
		SDL_CloseAudio();
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		shm = NULL;
		Con_Printf("Failed allocating memory for SDL audio\n");
		return false;
	}

	shm_voice->buffer = (unsigned char*)calloc(1, buffersize_voice);
	if (!shm_voice->buffer)
	{
		SDL_CloseAudio();
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		shm_voice = NULL;
		Con_Printf("Failed allocating memory for SDL audio recording\n");
		return false;
	}

	SDL_PauseAudioDevice(g_SoundDeviceID, SDL_FALSE);
	SDL_PauseAudioDevice(g_SoundDeviceID_voice, SDL_TRUE);

	return true;
}

/*
==============
SNDDMA_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
int CSoundDMA::SNDDMA_GetDMAPos()
{
	return shm->samplepos;
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void CSoundDMA::SNDDMA_Submit()
{
	SDL_UnlockAudioDevice(g_SoundDeviceID);
}

void paint_audio(void* userdata, Uint8* stream, int len)
{
	int	pos, tobufend;
	int	len1, len2;

	if (!g_SoundSystem->shm)
	{	/* shouldn't happen, but just in case */
		memset(stream, 0, len);
		return;
	}

	pos = (g_SoundSystem->shm->samplepos * (g_SoundSystem->shm->samplebits / 8));
	if (pos >= buffersize)
		g_SoundSystem->shm->samplepos = pos = 0;

	tobufend = buffersize - pos;  /* bytes to buffer's end. */
	len1 = len;
	len2 = 0;

	if (len1 > tobufend)
	{
		len1 = tobufend;
		len2 = len - len1;
	}

	memcpy(stream, g_SoundSystem->shm->buffer + pos, len1);

	if (len2 <= 0)
	{
		g_SoundSystem->shm->samplepos += (len1 / (g_SoundSystem->shm->samplebits / 8));
	}
	else
	{	/* wraparound? */
		memcpy(stream + len1, g_SoundSystem->shm->buffer, len2);
		g_SoundSystem->shm->samplepos = (len2 / (g_SoundSystem->shm->samplebits / 8));
	}

	if (g_SoundSystem->shm->samplepos >= buffersize)
		g_SoundSystem->shm->samplepos = 0;
}

void paint_audio_voice(void* userdata, Uint8* stream, int len)
{
	int	pos, tobufend;
	int	len1, len2;

	if (!g_SoundSystem->shm_voice)
	{	/* shouldn't happen, but just in case */
		memset(stream, 0, len);
		return;
	}

	pos = (g_SoundSystem->shm_voice->samplepos * (g_SoundSystem->shm_voice->samplebits / 8));
	if (pos >= buffersize)
		g_SoundSystem->shm_voice->samplepos = pos = 0;

	tobufend = buffersize - pos;  /* bytes to buffer's end. */
	len1 = len;
	len2 = 0;

	if (len1 > tobufend)
	{
		len1 = tobufend;
		len2 = len - len1;
	}

	memcpy(stream, g_SoundSystem->shm_voice->buffer + pos, len1);

	if (len2 <= 0)
	{
		g_SoundSystem->shm_voice->samplepos += (len1 / (g_SoundSystem->shm_voice->samplebits / 8));
	}
	else
	{	/* wraparound? */
		memcpy(stream + len1, g_SoundSystem->shm_voice->buffer, len2);
		g_SoundSystem->shm_voice->samplepos = (len2 / (g_SoundSystem->shm_voice->samplebits / 8));
	}

	if (g_SoundSystem->shm_voice->samplepos >= buffersize)
		g_SoundSystem->shm_voice->samplepos = 0;
}

/*
==============
SNDDMA_Shutdown

Reset the sound device for exiting
===============
*/
void CSoundDMA::SNDDMA_Shutdown()
{
	if (shm)
	{
		Con_Printf("Shutting down SDL sound\n");
		SDL_CloseAudio();
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		if (shm->buffer)
			free(shm->buffer);
		if (shm_voice->buffer)
			free(shm_voice->buffer);
		shm->buffer = NULL;
		shm = NULL;
		shm_voice->buffer = NULL;
		shm_voice = NULL;
	}
}

