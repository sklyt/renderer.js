#pragma once

#include <napi.h>
#include "audio_manager.h"

class AudioWrapper : public Napi::ObjectWrap<AudioWrapper>
{
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    AudioWrapper(const Napi::CallbackInfo &info);
    static Napi::FunctionReference constructor;
private:

    Napi::Value Initialize(const Napi::CallbackInfo &info);
    Napi::Value Shutdown(const Napi::CallbackInfo &info);

    Napi::Value LoadSound(const Napi::CallbackInfo &info);
    Napi::Value LoadSoundFromMemory(const Napi::CallbackInfo &info);
    Napi::Value PlaySound(const Napi::CallbackInfo &info);
    Napi::Value StopSound(const Napi::CallbackInfo &info);
    Napi::Value SetSoundVolume(const Napi::CallbackInfo &info);
    Napi::Value IsSoundPlaying(const Napi::CallbackInfo &info);
    Napi::Value UnloadSound(const Napi::CallbackInfo &info);

    Napi::Value LoadMusic(const Napi::CallbackInfo &info);
    Napi::Value LoadMusicFromMemory(const Napi::CallbackInfo &info);
    Napi::Value PlayMusic(const Napi::CallbackInfo &info);
    Napi::Value StopMusic(const Napi::CallbackInfo &info);
    Napi::Value SetMusicVolume(const Napi::CallbackInfo &info);
    Napi::Value IsMusicPlaying(const Napi::CallbackInfo &info);
    Napi::Value UnloadMusic(const Napi::CallbackInfo &info);
    Napi::Value SetMusicLooping(const Napi::CallbackInfo &info);

    // Audio streaming
    Napi::Value CreateAudioStream(const Napi::CallbackInfo &info);
    Napi::Value UpdateAudioStream(const Napi::CallbackInfo &info);
    Napi::Value PlayAudioStream(const Napi::CallbackInfo &info);
    Napi::Value StopAudioStream(const Napi::CallbackInfo &info);
    Napi::Value PauseAudioStream(const Napi::CallbackInfo &info);
    Napi::Value ResumeAudioStream(const Napi::CallbackInfo &info);
    Napi::Value UnloadAudioStream(const Napi::CallbackInfo &info);
    Napi::Value IsAudioStreamProcessed(const Napi::CallbackInfo &info);

     Napi::Value SetMasterVolume(const Napi::CallbackInfo& info);
    Napi::Value GetMasterVolume(const Napi::CallbackInfo& info);
};
