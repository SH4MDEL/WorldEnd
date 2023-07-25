#include "soundManager.h"

SoundManager::SoundManager() : m_musicVolume{ 5, 0.5f }, m_soundVolume{ 5, 0.5f }
{
	m_result = FMOD::System_Create(&m_system);
	m_result = m_system->init(32, FMOD_INIT_NORMAL, nullptr);

	m_result = m_system->createSound("Resource/Sound/Title.mp3", FMOD_LOOP_NORMAL, 0, &m_music[Music::Title]);
	m_result = m_system->createSound("Resource/Sound/Village.mp3", FMOD_LOOP_NORMAL, 0, &m_music[Music::Village]);
	m_result = m_system->createSound("Resource/Sound/Dungeon.mp3", FMOD_LOOP_NORMAL, 0, &m_music[Music::Dungeon]);
	m_result = m_system->createSound("Resource/Sound/Battle.mp3", FMOD_LOOP_NORMAL, 0, &m_music[Music::Dungeon]);
	m_result = m_system->createSound("Resource/Sound/Boss.mp3", FMOD_LOOP_NORMAL, 0, &m_music[Music::Boss]);

	m_result = m_system->createSound("Resource/Sound/Title.mp3", FMOD_LOOP_OFF, 0, &m_sound[Sound::Sample]);
}

SoundManager::~SoundManager()
{
	m_system->release();
	
}

void SoundManager::Update(FLOAT timeElapsed)
{
	m_result = m_system->update();
}

void SoundManager::PlayMusic(Music tag)
{
	bool playing;
	m_result = m_musicChannel->isPlaying(&playing);
	if (playing) m_musicChannel->stop();
	m_result = m_system->playSound(m_music[tag], 0, false, &m_musicChannel);
}

int SoundManager::MusicVolumeUp()
{
	if (m_musicVolume.first < MAX_SOUND) {
		m_musicVolume.first += 1;
		m_musicVolume.second += SOUND_WEIGHT;
		m_musicChannel->setVolume(m_musicVolume.second);
	}
	return m_musicVolume.first;
}

int SoundManager::MusicVolumeDown()
{
	if (m_musicVolume.first > MIN_SOUND) {
		m_musicVolume.first -= 1;
		m_musicVolume.second -= SOUND_WEIGHT;
		m_musicChannel->setVolume(m_musicVolume.second);
	}
	return m_musicVolume.first;
}

void SoundManager::PlaySound(Sound tag)
{
	bool playing;
	for (auto& channel : m_soundChannel) {
		m_result = channel->isPlaying(&playing);
		if (!playing) {
			m_result = m_system->playSound(m_sound[tag], 0, false, &channel);
			break;
		}
	}
}

int SoundManager::SoundVolumeUp()
{
	if (m_soundVolume.first < MAX_SOUND) {
		m_soundVolume.first += 1;
		m_soundVolume.second += SOUND_WEIGHT;
		for (auto& channel : m_soundChannel) {
			channel->setVolume(m_soundVolume.second);
		}
	}
	return m_soundVolume.first;
}

int SoundManager::SoundVolumeDown()
{
	if (m_soundVolume.first > MIN_SOUND) {
		m_soundVolume.first -= 1;
		m_soundVolume.second += SOUND_WEIGHT;
		for (auto& channel : m_soundChannel) {
			channel->setVolume(m_soundVolume.second);
		}
	}
	return m_soundVolume.first;
}
