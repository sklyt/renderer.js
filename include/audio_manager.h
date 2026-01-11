#pragma once

#include <raylib.h>
#include <unordered_map>
#include <memory>
#include <string>
#include <cstdint>

using SoundHandle = uint32_t;
using MusicHandle = uint32_t;
using AudioStreamHandle = uint32_t;

class AudioManager
{

public:
    static AudioManager &Instance();

    bool Initialize();
    void Shutdown();

    SoundHandle LoadSound(const std::string &filePath);
    SoundHandle LoadSoundFromMemory(const std::string &fileType, const uint8_t* data, int dataSize);
    void PlaySound(SoundHandle handle);
    void StopSound(SoundHandle handle);
    void PauseSound(SoundHandle handle);
    void ResumeSound(SoundHandle handle);
    void SetSoundVolume(SoundHandle handle, float volume);
    void UnloadSound(SoundHandle handle);
    bool IsSoundPlaying(SoundHandle handle) const;


    MusicHandle LoadMusic(const std::string &filePath);
    MusicHandle LoadMusicFromMemory(const std::string &fileType, const uint8_t* data, int dataSize);
    void PlayMusic(MusicHandle handle);
    void StopMusic(MusicHandle handle);
    void PauseMusic(MusicHandle handle);
    void ResumeMusic(MusicHandle handle);
    void SetMusicVolume(MusicHandle handle, float volume);
    void UnloadMusic(MusicHandle handle);
    bool IsMusicPlaying(MusicHandle handle) const;

    // Audio streaming (push model)
    AudioStreamHandle CreateAudioStream(uint32_t sampleRate, uint32_t sampleSize, uint32_t channels);
    void UpdateAudioStream(AudioStreamHandle handle, const void* data, int frameCount);
    void PlayAudioStream(AudioStreamHandle handle);
    void StopAudioStream(AudioStreamHandle handle);
    void PauseAudioStream(AudioStreamHandle handle);
    void ResumeAudioStream(AudioStreamHandle handle);
    void UnloadAudioStream(AudioStreamHandle handle);
    bool IsAudioStreamProcessed(AudioStreamHandle handle) const;

    void SetMasterVolume(float volume);
    float GetMasterVolume() const;

private:
    AudioManager();
    ~AudioManager();

    struct SoundData
    {
        Sound sound;
        std::string path;
        bool loaded = false;
    };

    struct MusicData
    {
        Music music;
        std::string path;
        bool loaded = false;
    };

    struct AudioStreamData
    {
        AudioStream stream;
        uint32_t sampleRate;
        uint32_t sampleSize;
        uint32_t channels;
        bool loaded = false;
    };

    std::unordered_map<SoundHandle, SoundData> sounds_;
    std::unordered_map<MusicHandle, MusicData> music_;
    std::unordered_map<AudioStreamHandle, AudioStreamData> audioStreams_;

    SoundHandle nextSoundHandle_ = 1;
    MusicHandle nextMusicHandle_ = 1;
    AudioStreamHandle nextAudioStreamHandle_ = 1;

    float masterVolume_ = 1.0f;
    bool initialized_ = false;
};
