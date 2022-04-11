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
#include <mmeapi.h>

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

CSoundSystemWin* g_SoundSystem;
SDL_AudioDeviceID g_SoundDeviceID;

//dma_t CSoundInternal::sn = { NULL };
volatile dma_t* CSoundInternal::shm = NULL;

static int	buffersize;

/*
==================
S_BlockSound
==================
*/
void CSoundSystemWin::S_BlockSound (void)
{

	if (sound_started && snd_blocked == 0)	/* ++snd_blocked == 1 */
	{
		snd_blocked = 1;
		S_ClearBuffer();
		if (shm)
			SDL_LockAudioDevice(g_SoundDeviceID);
	}
}


/*
==================
S_UnblockSound
==================
*/
void CSoundSystemWin::S_UnblockSound (void)
{
	if (!sound_started || !snd_blocked)
		return;
	if (snd_blocked == 1)			/* --snd_blocked == 0 */
	{
		snd_blocked = 0;
		SDL_UnlockAudioDevice(g_SoundDeviceID);
		S_ClearBuffer();
	}
}

/*
==================
SNDDMA_Init

Try to find a sound device to mix for.
Returns false if nothing is found.
==================
*/

bool CSoundSystemWin::SNDDMA_Init(dma_t* dma)
{
	sndinitstat	stat;
	int		tmp, val;
	char	drivername[128];

	SDL_AudioSpec desired;

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

	/* Open the audio device */

	g_SoundDeviceID = SDL_OpenAudioDevice(NULL, 0, &desired, NULL, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

	if (g_SoundDeviceID == -1)
	{
		Con_Printf("Couldn't open SDL audio: %s\n", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return false;
	}

	memset((void*)dma, 0, sizeof(dma_t));
	shm = dma;

	if (COM_CheckParm ("-wavonly"))
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

	Con_Printf("SDL audio spec  : %d Hz, %d samples, %d channels\n",
		desired.freq, desired.samples, desired.channels);

	buffersize = shm->samples * (shm->samplebits / 8);
	Con_Printf("SDL audio driver: %s, %d bytes buffer\n", SDL_GetCurrentAudioDriver(), buffersize);

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

	SDL_PauseAudioDevice(g_SoundDeviceID, 0);

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
int CSoundSystemWin::SNDDMA_GetDMAPos(void)
{
	return shm->samplepos;
}

void CSoundSystemWin::SNDDMA_LockBuffer(void)
{
	SDL_LockAudioDevice(g_SoundDeviceID);
}

void CSoundSystemWin::SNDDMA_UnlockBuffer(void)
{
	SDL_UnlockAudioDevice(g_SoundDeviceID);
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void CSoundSystemWin::SNDDMA_Submit(void)
{
	SDL_UnlockAudioDevice(g_SoundDeviceID);
}

void CSoundSystemWin::paint_audio(void* unused, Uint8* stream, int len)
{
	int	pos, tobufend;
	int	len1, len2;

	if (!shm)
	{	/* shouldn't happen, but just in case */
		memset(stream, 0, len);
		return;
	}

	pos = (shm->samplepos * (shm->samplebits / 8));
	if (pos >= buffersize)
		shm->samplepos = pos = 0;

	tobufend = buffersize - pos;  /* bytes to buffer's end. */
	len1 = len;
	len2 = 0;

	if (len1 > tobufend)
	{
		len1 = tobufend;
		len2 = len - len1;
	}

	memcpy(stream, shm->buffer + pos, len1);

	if (len2 <= 0)
	{
		shm->samplepos += (len1 / (shm->samplebits / 8));
	}
	else
	{	/* wraparound? */
		memcpy(stream + len1, shm->buffer, len2);
		shm->samplepos = (len2 / (shm->samplebits / 8));
	}

	if (shm->samplepos >= buffersize)
		shm->samplepos = 0;
}


/*
==============
SNDDMA_Shutdown

Reset the sound device for exiting
===============
*/
void CSoundSystemWin::SNDDMA_Shutdown(void)
{
	if (shm)
	{
		Con_Printf("Shutting down SDL sound\n");
		SDL_CloseAudio();
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		if (shm->buffer)
			free(shm->buffer);
		shm->buffer = NULL;
		shm = NULL;
	}
}

