#pragma once

#include <memory>
#include <vector>

class AudioSource;
class AudioSound;

class AudioLoopInfo
{
public:
	bool Looped = false;
	uint64_t LoopStart = 0;
	uint64_t LoopEnd = 0;
};

class AudioMixer
{
public:
	static std::unique_ptr<AudioMixer> Create(const void* zipData, size_t zipSize);

	virtual ~AudioMixer() = default;
	virtual AudioSound* AddSound(std::unique_ptr<AudioSource> source, const AudioLoopInfo& loopinfo = {}) = 0;
	virtual void RemoveSound(AudioSound* sound) = 0;
	virtual float GetSoundDuration(AudioSound* sound) = 0;
	virtual int PlaySound(int channel, AudioSound* sound, float volume, float pan, float pitch, float x, float y, float z) = 0;
	virtual void UpdateSound(int channel, AudioSound* sound, float volume, float pan, float pitch, float x, float y, float z) = 0;
	virtual void StopSound(int channel) = 0;
	virtual bool SoundFinished(int channel) = 0;
	virtual void PlayMusic(std::unique_ptr<AudioSource> source) = 0;
	virtual void SetMusicVolume(float volume) = 0;
	virtual void SetSoundVolume(float volume) = 0;
	virtual void SetReverb(float volume, float hfcutoff, std::vector<float> time, std::vector<float> gain) = 0;
	virtual void Update() = 0;
};
