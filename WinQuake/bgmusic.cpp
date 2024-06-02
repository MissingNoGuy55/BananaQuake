#include "quakedef.h"
#include "snd_codec.h"

#define MUSIC_DIRNAME	"music"

CBackgroundMusic* g_BGM;

bool	CBackgroundMusic::no_extmusic = false;
float	CBackgroundMusic::old_volume = -1.0f;

bool CBackgroundMusic::bgmloop = false;
cvar_t CBackgroundMusic::bgm_extmusic = { "bgm_extmusic", "1", true };

typedef enum _bgm_player
{
	BGM_NONE = -1,
	BGM_MIDIDRV = 1,
	BGM_STREAMER
} bgm_player_t;

typedef struct music_handler_s
{
	unsigned int	type;	/* 1U << n (see snd_codec.h)	*/
	bgm_player_t	player;	/* Enumerated bgm player type	*/
	int	is_available;	/* -1 means not present		*/
	const char* ext;	/* Expected file extension	*/
	const char* dir;	/* Where to look for music file */
	struct music_handler_s* next;
} music_handler_t;

static music_handler_t wanted_handlers[] =
{
	{ CODECTYPE_VORBIS,BGM_STREAMER,-1,  "ogg", MUSIC_DIRNAME, NULL },
	//{ CODECTYPE_OPUS, BGM_STREAMER, -1, "opus", MUSIC_DIRNAME, NULL },
	{ CODECTYPE_MP3,  BGM_STREAMER, -1,  "mp3", MUSIC_DIRNAME, NULL },
	//{ CODECTYPE_FLAC, BGM_STREAMER, -1, "flac", MUSIC_DIRNAME, NULL },
	{ CODECTYPE_WAV,  BGM_STREAMER, -1,  "wav", MUSIC_DIRNAME, NULL },
	//{ CODECTYPE_MOD,  BGM_STREAMER, -1,  "it",  MUSIC_DIRNAME, NULL },
	//{ CODECTYPE_MOD,  BGM_STREAMER, -1,  "s3m", MUSIC_DIRNAME, NULL },
	//{ CODECTYPE_MOD,  BGM_STREAMER, -1,  "xm",  MUSIC_DIRNAME, NULL },
	//{ CODECTYPE_MOD,  BGM_STREAMER, -1,  "mod", MUSIC_DIRNAME, NULL },
	//{ CODECTYPE_UMX,  BGM_STREAMER, -1,  "umx", MUSIC_DIRNAME, NULL },
	{ CODECTYPE_NONE, BGM_NONE,     -1,   NULL,         NULL,  NULL }
};

static music_handler_t* music_handlers = NULL;

#define ANY_CODECTYPE	0xFFFFFFFF
#define CDRIP_TYPES	(CODECTYPE_VORBIS | CODECTYPE_MP3 | CODECTYPE_FLAC | CODECTYPE_WAV | CODECTYPE_OPUS)
#define CDRIPTYPE(x)	(((x) & CDRIP_TYPES) != 0)

static snd_stream_t* bgmstream = NULL;

void CBackgroundMusic::BGM_Play_f(void)
{
	if (g_pCmds->Cmd_Argc() == 2) {
		BGM_Play(g_pCmds->Cmd_Argv(1));
	}
	else {
		Con_Printf("music <musicfile>\n");
	}
}

void CBackgroundMusic::BGM_Pause_f(void)
{
	BGM_Pause();
}

void CBackgroundMusic::BGM_Resume_f(void)
{
	BGM_Resume();
}

void CBackgroundMusic::BGM_Loop_f(void)
{
	if (g_pCmds->Cmd_Argc() == 2) {
		if (Q_strcasecmp(g_pCmds->Cmd_Argv(1), "0") == 0 ||
			Q_strcasecmp(g_pCmds->Cmd_Argv(1), "off") == 0)
			bgmloop = false;
		else if (Q_strcasecmp(g_pCmds->Cmd_Argv(1), "1") == 0 ||
			Q_strcasecmp(g_pCmds->Cmd_Argv(1), "on") == 0)
			bgmloop = true;
		else if (Q_strcasecmp(g_pCmds->Cmd_Argv(1), "toggle") == 0)
			bgmloop = !bgmloop;

		if (bgmstream) bgmstream->loop = bgmloop;
	}

	if (bgmloop)
		Con_Printf("Music will be looped\n");
	else
		Con_Printf("Music will not be looped\n");
}

void CBackgroundMusic::BGM_Stop_f(void)
{
	BGM_Stop();
}

void CBackgroundMusic::BGM_Jump_f(void)
{
	if (g_pCmds->Cmd_Argc() != 2) {
		Con_Printf("music_jump <ordernum>\n");
	}
	else if (bgmstream) {
		S_CodecJumpToOrder(bgmstream, atoi(g_pCmds->Cmd_Argv(1)));
	}
}

CBackgroundMusic::CBackgroundMusic()
{
}

CBackgroundMusic::~CBackgroundMusic()
{
}

bool CBackgroundMusic::BGM_Init()
{
	music_handler_t* handlers = NULL;
	int i;

	Cvar_RegisterVariable(&bgm_extmusic);
	g_pCmds->Cmd_AddCommand("music", BGM_Play_f);
	g_pCmds->Cmd_AddCommand("music_pause", BGM_Pause_f);
	g_pCmds->Cmd_AddCommand("music_resume", BGM_Resume_f);
	g_pCmds->Cmd_AddCommand("music_loop", BGM_Loop_f);
	g_pCmds->Cmd_AddCommand("music_stop", BGM_Stop_f);
	g_pCmds->Cmd_AddCommand("music_jump", BGM_Jump_f);

	if (g_Common->COM_CheckParm("-noextmusic") != 0)
		no_extmusic = true;

	bgmloop = true;

	for (i = 0; wanted_handlers[i].type != CODECTYPE_NONE; i++)
	{
		switch (wanted_handlers[i].player)
		{
		case BGM_MIDIDRV:
			/* not supported in quake */
			break;
		case BGM_STREAMER:
			wanted_handlers[i].is_available = S_CodecIsAvailable(wanted_handlers[i].type);
			Con_Printf("Found audio handler for %s\n", wanted_handlers[i].ext);
			break;
		case BGM_NONE:
		default:
			break;
		}
 		if (wanted_handlers[i].is_available != -1)
		{
			if (handlers)
			{
				handlers->next = &wanted_handlers[i];
				handlers = handlers->next;
			}
			else
			{
				music_handlers = &wanted_handlers[i];
				handlers = music_handlers;
			}
		}
	}

	return true;
}

void CBackgroundMusic::BGM_Shutdown(void)
{
	BGM_Stop();
	/* sever our connections to
	 * midi_drv and snd_codec */
	music_handlers = NULL;
}

void CBackgroundMusic::BGM_Play_noext(const char* filename, unsigned int allowed_types)
{
	char tmp[MAX_QPATH];
	music_handler_t* handler;

	handler = music_handlers;
	while (handler)
	{
		if (!(handler->type & allowed_types))
		{
			handler = handler->next;
			continue;
		}
		if (!handler->is_available)
		{
			handler = handler->next;
			continue;
		}
		snprintf(tmp, sizeof(tmp), "%s/%s.%s",
			handler->dir, filename, handler->ext);
		switch (handler->player)
		{
		case BGM_MIDIDRV:
			/* not supported in quake */
			break;
		case BGM_STREAMER:
			bgmstream = S_CodecOpenStreamType(tmp, handler->type, bgmloop);
			if (bgmstream)
				return;		/* success */
			break;
		case BGM_NONE:
		default:
			break;
		}
		handler = handler->next;
	}

	Con_Printf("Couldn't handle music file %s\n", filename);
}

void CBackgroundMusic::BGM_Play(const char* filename)
{
	char tmp[MAX_QPATH];
	const char* ext;
	music_handler_t* handler;

	BGM_Stop();

	if (music_handlers == NULL)
		return;

	if (!filename || !*filename)
	{
		Con_DPrintf("null music file name\n");
		return;
	}

	ext = g_Common->COM_FileGetExtension(filename);
	if (!*ext)	/* try all things */
	{
		BGM_Play_noext(filename, ANY_CODECTYPE);
		return;
	}

	handler = music_handlers;
	while (handler)
	{
		if (handler->is_available &&
			!Q_strcasecmp(ext, handler->ext))
			break;
		handler = handler->next;
	}
	if (!handler)
	{
		Con_Printf("Unhandled extension for %s\n", filename);
		return;
	}
	snprintf(tmp, sizeof(tmp), "%s/%s", handler->dir, filename);
	switch (handler->player)
	{
	case BGM_MIDIDRV:
		/* not supported in quake */
		break;
	case BGM_STREAMER:
		bgmstream = S_CodecOpenStreamType(tmp, handler->type, bgmloop);
		if (bgmstream)
			return;		/* success */
		break;
	case BGM_NONE:
	default:
		break;
	}

	Con_Printf("Couldn't handle music file %s\n", filename);
}

void CBackgroundMusic::BGM_PlayCDtrack(byte track, bool looping)
{
	/* instead of searching by the order of music_handlers, do so by
	 * the order of searchpath priority: the file from the searchpath
	 * with the highest path_id is most likely from our own gamedir
	 * itself. This way, if a mod has track02 as a *.mp3 file, which
	 * is below *.ogg in the music_handler order, the mp3 will still
	 * have priority over track02.ogg from, say, id1.
	 */
	char tmp[MAX_QPATH] = {};
	const char* ext = nullptr;
	uintptr_t path_id = 0, prev_id = 0, type = 0;
	music_handler_t* handler = nullptr;

	BGM_Stop();
	if (CDAudio_Play(track, looping) == 0)
		return;			/* success */

	if (music_handlers == NULL)
		return;

	if (no_extmusic || !bgm_extmusic.value)
		return;

	prev_id = 0;
	type = 0;
	ext = NULL;
	handler = music_handlers;
	while (handler)
	{
		if (!handler->is_available)
			goto _next;
		if (!CDRIPTYPE(handler->type))
			goto _next;
		snprintf(tmp, sizeof(tmp), "%s/track%02d.%s",
			MUSIC_DIRNAME, (int)track, handler->ext);
		if (!g_Common->COM_FileExists(tmp, &path_id))
			goto _next;
		if (path_id > prev_id)
		{
			prev_id = path_id;
			type = handler->type;
			ext = handler->ext;
		}
	_next:
		handler = handler->next;
	}
 	if (ext == NULL)
		Con_Printf("Couldn't find a cdrip for track %d\n", (int)track);
	else
	{
		snprintf(tmp, sizeof(tmp), "%s/track%02d.%s",
			MUSIC_DIRNAME, (int)track, ext);
		bgmstream = S_CodecOpenStreamType(tmp, type, bgmloop);
		if (!bgmstream)
			Con_Printf("Couldn't handle music file %s\n", tmp);
	}
}

void CBackgroundMusic::BGM_Stop(void)
{
	if (bgmstream)
	{
		bgmstream->status = STREAM_NONE;
		S_CodecCloseStream(bgmstream);
		bgmstream = NULL;
		g_SoundSystem->s_rawend = 0;
	}
}

void CBackgroundMusic::BGM_Pause(void)
{
	if (bgmstream)
	{
		if (bgmstream->status == STREAM_PLAY)
			bgmstream->status = STREAM_PAUSE;
	}
}

void CBackgroundMusic::BGM_Resume(void)
{
	if (bgmstream)
	{
		if (bgmstream->status == STREAM_PAUSE)
			bgmstream->status = STREAM_PLAY;
	}
}

void CBackgroundMusic::BGM_UpdateStream(void)
{
	bool did_rewind = false;
	int	res = 0;	/* Number of bytes read. */
	int	bufferSamples = 0;
	int	fileSamples = 0;
	int	fileBytes = 0;
	byte	raw[16384] = {};

	if (bgmstream->status != STREAM_PLAY)
		return;

	/* don't bother playing anything if musicvolume is 0 */
	if (bgmvolume.value <= 0)
		return;

	/* see how many samples should be copied into the raw buffer */
	if (g_SoundSystem->s_rawend < g_SoundSystem->paintedtime)
		g_SoundSystem->s_rawend = g_SoundSystem->paintedtime;

	while (g_SoundSystem->s_rawend < g_SoundSystem->paintedtime + MAX_RAW_SAMPLES)
	{
		bufferSamples = MAX_RAW_SAMPLES - (g_SoundSystem->s_rawend - g_SoundSystem->paintedtime);

		/* decide how much data needs to be read from the file */
		fileSamples = bufferSamples * bgmstream->info.rate / g_SoundSystem->shm->speed;
		if (!fileSamples)
			return;

		/* our max buffer size */
		fileBytes = fileSamples * (bgmstream->info.width * bgmstream->info.channels);
		if (fileBytes > (int)sizeof(raw))
		{
			fileBytes = (int)sizeof(raw);
			fileSamples = fileBytes /
				(bgmstream->info.width * bgmstream->info.channels);
		}

		/* Read */
		res = S_CodecReadStream(bgmstream, fileBytes, raw);
		if (res < fileBytes)
		{
			fileBytes = res;
			fileSamples = res / (bgmstream->info.width * bgmstream->info.channels);
		}

		if (res > 0)	/* data: add to raw buffer */
		{
			g_SoundSystem->S_RawSamples(fileSamples, bgmstream->info.rate,
				bgmstream->info.width,
				bgmstream->info.channels,
				raw, bgmvolume.value);
			did_rewind = false;
		}
		else if (res == 0)	/* EOF */
		{
			if (bgmloop)
			{
				if (did_rewind)
				{
					Con_Printf("Stream keeps returning EOF.\n");
					BGM_Stop();
					return;
				}

				res = S_CodecRewindStream(bgmstream);
				if (res != 0)
				{
					Con_Printf("Stream seek error (%i), stopping.\n", res);
					BGM_Stop();
					return;
				}
				did_rewind = true;
			}
			else
			{
				BGM_Stop();
				return;
			}
		}
		else	/* res < 0: some read error */
		{
			Con_Printf("Stream read error (%i), stopping.\n", res);
			BGM_Stop();
			return;
		}
	}
}

void CBackgroundMusic::BGM_Update(void)
{
	if (old_volume != bgmvolume.value)
	{
		if (bgmvolume.value < 0)
			Cvar_Set("bgmvolume", "0");
		else if (bgmvolume.value > 1)
			Cvar_Set("bgmvolume", "1");
		old_volume = bgmvolume.value;
	}
	if (bgmstream)
		BGM_UpdateStream();
}
