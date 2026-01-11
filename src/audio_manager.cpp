#include "audio_manager.h"
#include <iostream>
#include "debugger.h"

AudioManager& AudioManager::Instance() { 
    static AudioManager instance;
    return instance;
}

AudioManager::AudioManager() {}

AudioManager::~AudioManager() {
    Shutdown();
}

bool AudioManager::Initialize() {
    if (initialized_) return true;
    
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        Debugger::Instance().LogError("Failed to initialize audio device");
        return false;
    }
    
    initialized_ = true;
    Debugger::Instance().LogInfo("Audio manager initialized");
    return true;
}

void AudioManager::Shutdown(){
    if (!initialized_) return;
    
    for (auto& [handle, soundData] : sounds_) {
        if (soundData.loaded) {
            ::UnloadSound(soundData.sound);
        }
    }
    sounds_.clear();
    
    for (auto& [handle, musicData] : music_) {
        if (musicData.loaded) {
            UnloadMusicStream(musicData.music);
        }
    }
    music_.clear();
    
    for (auto& [handle, streamData] : audioStreams_) {
        if (streamData.loaded) {
            ::UnloadAudioStream(streamData.stream);
        }
    }
    audioStreams_.clear();
    
    CloseAudioDevice();
    initialized_ = false;
    Debugger::Instance().LogInfo("Audio manager shutdown");
}

SoundHandle AudioManager::LoadSound(const std::string& filePath) {
    if (!initialized_) return 0;
    
    Sound sound = ::LoadSound(filePath.c_str());
    if (sound.frameCount == 0) {
        Debugger::Instance().LogError("Failed to load sound: " + filePath);
        return 0;
    }
    
    SoundHandle handle = nextSoundHandle_++;
    sounds_[handle] = {sound, filePath, true};
    return handle;
}

SoundHandle AudioManager::LoadSoundFromMemory(const std::string &fileType, const uint8_t* data, int dataSize) {
    if (!initialized_) return 0;
    if (!data || dataSize <= 0) return 0;

    Wave wave = ::LoadWaveFromMemory(fileType.c_str(), data, dataSize);
    if (wave.frameCount == 0) {
        Debugger::Instance().LogError("Failed to load wave from memory");
        return 0;
    }

    Sound sound = ::LoadSoundFromWave(wave);
    ::UnloadWave(wave);

    if (sound.frameCount == 0) {
        Debugger::Instance().LogError("Failed to create sound from memory wave");
        return 0;
    }

    SoundHandle handle = nextSoundHandle_++;
    sounds_[handle] = {sound, std::string("memory:" ) + fileType, true};
    return handle;
}

void AudioManager::PlaySound(SoundHandle handle) {
    if (!initialized_) return;
    
    auto it = sounds_.find(handle);
    if (it != sounds_.end() && it->second.loaded) {
        ::PlaySound(it->second.sound);
    }
}
void AudioManager::StopSound(SoundHandle handle) {
    if (!initialized_) return;
    
    auto it = sounds_.find(handle);
    if (it != sounds_.end() && it->second.loaded) {
        ::StopSound(it->second.sound);
    }

}

void AudioManager::PauseSound(SoundHandle handle) {
    if (!initialized_) return;
    
    auto it = sounds_.find(handle);
    if (it != sounds_.end() && it->second.loaded) {
        ::PauseSound(it->second.sound);
    }
}   


void AudioManager::ResumeSound(SoundHandle handle) {
    if (!initialized_) return;
    
    auto it = sounds_.find(handle);
    if (it != sounds_.end() && it->second.loaded) {
        ::ResumeSound(it->second.sound);
    }
}   


void AudioManager::SetSoundVolume(SoundHandle handle, float volume) {
    if (!initialized_) return;
    
    auto it = sounds_.find(handle);
    if (it != sounds_.end() && it->second.loaded) {
        ::SetSoundVolume(it->second.sound, volume * masterVolume_);
    }
}   


bool AudioManager::IsSoundPlaying(SoundHandle handle) const {
    if (!initialized_) return false;
    
    auto it = sounds_.find(handle);
    if (it != sounds_.end() && it->second.loaded) {
        return ::IsSoundPlaying(it->second.sound);
    }
    return false;
}

void AudioManager::UnloadSound(SoundHandle handle) {
    if (!initialized_) return;
    
    auto it = sounds_.find(handle);
    if (it != sounds_.end() && it->second.loaded) {
        ::UnloadSound(it->second.sound);
        sounds_.erase(it);
    }
}

MusicHandle AudioManager::LoadMusic(const std::string& filePath) {
    if (!initialized_) return 0;
    
    Music music = LoadMusicStream(filePath.c_str());
    if (music.frameCount == 0) {
        Debugger::Instance().LogError("Failed to load music: " + filePath);
        return 0;
    }
    
    MusicHandle handle = nextMusicHandle_++;
    music_[handle] = {music, filePath, true};
    return handle;
}

MusicHandle AudioManager::LoadMusicFromMemory(const std::string &fileType, const uint8_t* data, int dataSize) {
    if (!initialized_) return 0;
    if (!data || dataSize <= 0) return 0;

    Music music = LoadMusicStreamFromMemory(fileType.c_str(), data, dataSize);
    if (music.frameCount == 0) {
        Debugger::Instance().LogError("Failed to load music from memory");
        return 0;
    }

    MusicHandle handle = nextMusicHandle_++;
    music_[handle] = {music, std::string("memory:") + fileType, true};
    return handle;
}

void AudioManager::PlayMusic(MusicHandle handle) {
    if (!initialized_) return;
    
    auto it = music_.find(handle);
    if (it != music_.end() && it->second.loaded) {
        PlayMusicStream(it->second.music);
    }
}
void AudioManager::StopMusic(MusicHandle handle) {
    if (!initialized_) return;
    
    auto it = music_.find(handle);
    if (it != music_.end() && it->second.loaded) {
        StopMusicStream(it->second.music);
    }
}

void AudioManager::PauseMusic(MusicHandle handle) {
    if (!initialized_) return;
    
    auto it = music_.find(handle);
    if (it != music_.end() && it->second.loaded) {
        PauseMusicStream(it->second.music);
    }
}   


void AudioManager::ResumeMusic(MusicHandle handle) {
    if (!initialized_) return;
    
    auto it = music_.find(handle);
    if (it != music_.end() && it->second.loaded) {
        ResumeMusicStream(it->second.music);
    }
}

void AudioManager::SetMusicVolume(MusicHandle handle, float volume) {
    if (!initialized_) return;
    
    auto it = music_.find(handle);
    if (it != music_.end() && it->second.loaded) {
        ::SetMusicVolume(it->second.music, volume * masterVolume_);
    }
}   


void AudioManager::UnloadMusic(MusicHandle handle) {
    if (!initialized_) return;
    
    auto it = music_.find(handle);
    if (it != music_.end() && it->second.loaded) {
        UnloadMusicStream(it->second.music);
        music_.erase(it);
    }
}   


bool AudioManager::IsMusicPlaying(MusicHandle handle) const {
    if (!initialized_) return false;
    
    auto it = music_.find(handle);
    if (it != music_.end() && it->second.loaded) {
        return ::IsMusicStreamPlaying(it->second.music);
    }
    return false;
}

void AudioManager::SetMasterVolume(float volume) {
    masterVolume_ = volume;
    ::SetMasterVolume(volume);
}

float AudioManager::GetMasterVolume() const {
    return masterVolume_;
}

AudioStreamHandle AudioManager::CreateAudioStream(uint32_t sampleRate, uint32_t sampleSize, uint32_t channels) {
    if (!initialized_) return 0;
    
    AudioStream stream = ::LoadAudioStream(sampleRate, sampleSize, channels);
    if (stream.buffer == nullptr) {
        Debugger::Instance().LogError("Failed to create audio stream");
        return 0;
    }
    
    AudioStreamHandle handle = nextAudioStreamHandle_++;
    audioStreams_[handle] = {stream, sampleRate, sampleSize, channels, true};
    return handle;
}

void AudioManager::UpdateAudioStream(AudioStreamHandle handle, const void* data, int frameCount) {
    if (!initialized_) return;
    
    auto it = audioStreams_.find(handle);
    if (it != audioStreams_.end() && it->second.loaded && data) {
        ::UpdateAudioStream(it->second.stream, data, frameCount);
    }
}

void AudioManager::PlayAudioStream(AudioStreamHandle handle) {
    if (!initialized_) return;
    
    auto it = audioStreams_.find(handle);
    if (it != audioStreams_.end() && it->second.loaded) {
        ::PlayAudioStream(it->second.stream);
    }
}

void AudioManager::StopAudioStream(AudioStreamHandle handle) {
    if (!initialized_) return;
    
    auto it = audioStreams_.find(handle);
    if (it != audioStreams_.end() && it->second.loaded) {
        ::StopAudioStream(it->second.stream);
    }
}

void AudioManager::PauseAudioStream(AudioStreamHandle handle) {
    if (!initialized_) return;
    
    auto it = audioStreams_.find(handle);
    if (it != audioStreams_.end() && it->second.loaded) {
        ::PauseAudioStream(it->second.stream);
    }
}

void AudioManager::ResumeAudioStream(AudioStreamHandle handle) {
    if (!initialized_) return;
    
    auto it = audioStreams_.find(handle);
    if (it != audioStreams_.end() && it->second.loaded) {
        ::ResumeAudioStream(it->second.stream);
    }
}

void AudioManager::UnloadAudioStream(AudioStreamHandle handle) {
    if (!initialized_) return;
    
    auto it = audioStreams_.find(handle);
    if (it != audioStreams_.end() && it->second.loaded) {
        ::UnloadAudioStream(it->second.stream);
        audioStreams_.erase(it);
    }
}

bool AudioManager::IsAudioStreamProcessed(AudioStreamHandle handle) const {
    if (!initialized_) return false;
    
    auto it = audioStreams_.find(handle);
    if (it != audioStreams_.end() && it->second.loaded) {
        return ::IsAudioStreamProcessed(it->second.stream);
    }
    return false;
}
