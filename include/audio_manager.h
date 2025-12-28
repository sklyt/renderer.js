#pragma once

#include <raylib.h>
#include <unordered_map>
#include <memory>
#include <string>
#include <cstdint>

using SoundHandle = uint32_t;
using MusicHandle = uint32_t;

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

    std::unordered_map<SoundHandle, SoundData> sounds_;
    std::unordered_map<MusicHandle, MusicData> music_;

    SoundHandle nextSoundHandle_ = 1;
    MusicHandle nextMusicHandle_ = 1;

    float masterVolume_ = 1.0f;
    bool initialized_ = false;
};
