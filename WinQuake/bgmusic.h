#pragma once

class CBackgroundMusic
{
public:

	CBackgroundMusic();
	~CBackgroundMusic();

	bool BGM_Init();

	void BGM_Shutdown(void);

	static void BGM_Play_noext(const char* filename, unsigned int allowed_types);

	static void BGM_Play(const char* filename);
	void BGM_PlayCDtrack(byte track, bool looping);
	static void BGM_Play_f();

	static void BGM_Stop(void);

	static void BGM_Pause();

	static void BGM_Resume(void);

	void BGM_UpdateStream(void);

	void BGM_Update(void);

	static void BGM_Pause_f(void);

	static void BGM_Resume_f(void);

	static void BGM_Loop_f(void);

	static void BGM_Stop_f(void);
	static void BGM_Jump_f(void);

	static bool		bgmloop;
	static cvar_t	bgm_extmusic;

	static bool	no_extmusic;
	static float	old_volume;

};

extern CBackgroundMusic* g_BGM;

extern cvar_t		bgm_extmusic;