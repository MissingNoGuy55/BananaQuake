#pragma once

#include "snd_codec.h"

#define ANY_CODECTYPE	0xFFFFFFFF
#define CDRIP_TYPES	(CODECTYPE_VORBIS | CODECTYPE_MP3 | CODECTYPE_FLAC | CODECTYPE_WAV | CODECTYPE_OPUS)
#define CDRIPTYPE(x)	(((x) & CDRIP_TYPES) != 0)

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

typedef struct artistinfo_s
{
    char band[256];
	char song[256];
} artistinfo_t;

class CBackgroundMusic
{
public:

	CBackgroundMusic();
	~CBackgroundMusic();

	bool BGM_Init();

	void BGM_Shutdown();

	static void BGM_Play_noext(const char* filename, unsigned int allowed_types);

	static void BGM_Play(const char* filename);
	void BGM_PlayCDtrack(byte track, bool looping);
	static void BGM_Play_f();

	static void BGM_Stop();

	static void BGM_Pause();

	static void BGM_Resume();

	void BGM_UpdateStream();

	void BGM_Update();

	static void BGM_Pause_f();

	static void BGM_Resume_f();

	static void BGM_Loop_f();

	static void BGM_Stop_f();
	static void BGM_Jump_f();

    void GetSongArtistAndName(const char* filename, uintptr_t* path_id, const char* ext, artistinfo_t& artistinfo);

	static bool		bgmloop;

	static bool	no_extmusic;
	static float	old_volume;

	static music_handler_t* music_handlers;

	static snd_stream_t* bgmstream;

};

extern CBackgroundMusic* g_pBGM;

extern cvar_t		bgm_extmusic;
