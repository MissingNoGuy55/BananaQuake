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
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <linux/soundcard.h>
#include <stdio.h>
#include "quakedef.h"

int audio_fd;
int snd_inited;


dma_t* CSoundDMA::shm;
CSoundDMA* g_SoundSystem;
static size_t	buffersize;

static int tryrates[] = { 11025, 22051, 44100, 8000 };

bool CSoundDMA::SNDDMA_Init(dma_t* dma)
{

    SDL_AudioSpec desired = {};
    SDL_AudioSpec obtained = {};
    int		tmp = 0, val = 0;
    char	drivername[128] = {};
    
    shm = g_MemCache->Hunk_AllocName<dma_t>(sizeof(*shm), "shm");

	snd_inited = 0;

// open /dev/dsp, confirm capability to mmap, and get size of dma buffer

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
        Sys_Error("Couldn't load SDL audio");
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
    if ((g_SoundDeviceID = SDL_OpenAudioDevice(NULL, SDL_FALSE, &desired, &obtained, SDL_AUDIO_ALLOW_FORMAT_CHANGE)) == -1)
    {
        Con_Printf("Couldn't open SDL audio: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }

	memset ((void *) dma, 0, sizeof(dma_t));
	shm = dma;

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

    Con_PrintColor (TEXT_COLOR_CYAN, "SDL audio spec  : %d Hz, %d samples, %d channels\n",
			desired.freq, desired.samples, desired.channels);
    

    int audioNum = SDL_GetNumAudioDevices(SDL_FALSE);
    const char *driver = SDL_GetCurrentAudioDriver();
    const char *device = SDL_GetAudioDeviceName(0, SDL_FALSE);

    snprintf(drivername, sizeof(drivername), "%s - %s",
        driver != NULL ? driver : "(UNKNOWN)",
        device != NULL ? device : "(UNKNOWN)");

	buffersize = shm->samples * (shm->samplebits / 8);
	Con_Printf ("SDL audio driver: %s, %d bytes buffer\n", drivername, buffersize);

	shm->buffer = (unsigned char *) calloc (1, buffersize);
	if (!shm->buffer)
	{
		SDL_CloseAudio();
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		shm = NULL;
		Con_Printf ("Failed allocating memory for SDL audio\n");
		return false;
	}

    SDL_PauseAudioDevice(g_SoundDeviceID, 0);

    SDL_AudioStatus status = SDL_GetAudioDeviceStatus(g_SoundDeviceID);

	return true;

}

int CSoundDMA::SNDDMA_GetDMAPos(void)
{
	return shm->samplepos;
}

void CSoundDMA::SNDDMA_Shutdown(void)
{
	if (snd_inited)
	{
		close(audio_fd);
		snd_inited = 0;
	}
}

static SDLCALL void paint_audio(void* userdata, Uint8* stream, int len)
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

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void CSoundDMA::SNDDMA_Submit(void)
{
    SDL_UnlockAudioDevice(g_SoundDeviceID);
}

