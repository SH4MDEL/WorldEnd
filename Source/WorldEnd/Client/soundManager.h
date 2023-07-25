#pragma once
#include "stdafx.h"
#include "singleton.h"

constexpr int MAX_PLAY_SOUND = 5;
constexpr int MAX_SOUND = 10;
constexpr int MIN_SOUND = 0;
constexpr float SOUND_WEIGHT = 0.1f;

class SoundManager : public Singleton<SoundManager>
{
public:
	enum Music
	{
		Title,
		Village,
		Dungeon,
		Battle,
		Boss,
		MusicCount
	};
	enum Sound
	{
		Sample,
		SoundCount
	};

	SoundManager();
	~SoundManager();

	void Update(FLOAT timeElapsed);

	void PlayMusic(Music tag);
	int MusicVolumeUp();
	int MusicVolumeDown();

	void PlaySound(Sound tag);
	int SoundVolumeUp();
	int SoundVolumeDown();

private:
	FMOD::System*							m_system;

	FMOD::Sound*							m_music[Music::MusicCount];
	FMOD::Channel*							m_musicChannel;
	pair<int, float>						m_musicVolume;

	array<FMOD::Sound*, Sound::SoundCount>	m_sound;
	array<FMOD::Channel*, MAX_PLAY_SOUND>	m_soundChannel;
	pair<int, float>						m_soundVolume;

	FMOD_RESULT								m_result;
};

