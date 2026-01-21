#include  "audio_wrapper.h"
#include "napi.h"

Napi::FunctionReference AudioWrapper::constructor;
 

Napi::Object AudioWrapper::Init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);

        Napi::Function func = DefineClass(env, "Audio", {
        InstanceMethod("initialize", &AudioWrapper::Initialize),
        InstanceMethod("shutdown", &AudioWrapper::Shutdown),
     

        InstanceMethod("loadSound", &AudioWrapper::LoadSound),
        InstanceMethod("loadSoundFromMemory", &AudioWrapper::LoadSoundFromMemory),
        InstanceMethod("playSound", &AudioWrapper::PlaySound),
        InstanceMethod("stopSound", &AudioWrapper::StopSound),
        InstanceMethod("setSoundVolume", &AudioWrapper::SetSoundVolume),
        InstanceMethod("isSoundPlaying", &AudioWrapper::IsSoundPlaying),
        InstanceMethod("unloadSound", &AudioWrapper::UnloadSound),
        
        
        InstanceMethod("loadMusic", &AudioWrapper::LoadMusic),
        InstanceMethod("loadMusicFromMemory", &AudioWrapper::LoadMusicFromMemory),
        InstanceMethod("playMusic", &AudioWrapper::PlayMusic),
        InstanceMethod("stopMusic", &AudioWrapper::StopMusic),
        InstanceMethod("setMusicVolume", &AudioWrapper::SetMusicVolume),
        InstanceMethod("isMusicPlaying", &AudioWrapper::IsMusicPlaying),
        InstanceMethod("unloadMusic", &AudioWrapper::UnloadMusic),
        InstanceMethod("setMusicLooping", &AudioWrapper::SetMusicLooping),
        
        InstanceMethod("createAudioStream", &AudioWrapper::CreateAudioStream),
        InstanceMethod("updateAudioStream", &AudioWrapper::UpdateAudioStream),
        InstanceMethod("playAudioStream", &AudioWrapper::PlayAudioStream),
        InstanceMethod("stopAudioStream", &AudioWrapper::StopAudioStream),
        InstanceMethod("pauseAudioStream", &AudioWrapper::PauseAudioStream),
        InstanceMethod("resumeAudioStream", &AudioWrapper::ResumeAudioStream),
        InstanceMethod("unloadAudioStream", &AudioWrapper::UnloadAudioStream),
        InstanceMethod("isAudioStreamProcessed", &AudioWrapper::IsAudioStreamProcessed),

        InstanceMethod("setMasterVolume", &AudioWrapper::SetMasterVolume),
        InstanceMethod("getMasterVolume", &AudioWrapper::GetMasterVolume),
    });
    
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    
    exports.Set("Audio", func);
    return exports;

}

AudioWrapper::AudioWrapper(const Napi::CallbackInfo& info) 
    : Napi::ObjectWrap<AudioWrapper>(info) {
}

Napi::Value AudioWrapper::Initialize(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    bool success = AudioManager::Instance().Initialize();
    return Napi::Boolean::New(env, success);
}

Napi::Value AudioWrapper::LoadSound(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected for file path").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    std::string filePath = info[0].As<Napi::String>().Utf8Value();
    SoundHandle handle = AudioManager::Instance().LoadSound(filePath);
    
    return Napi::Number::New(env, handle);
}

Napi::Value AudioWrapper::LoadSoundFromMemory(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (fileType: string, buffer)").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string fileType = info[0].As<Napi::String>().Utf8Value();
    const uint8_t* data = nullptr;
    size_t dataSize = 0;

    if (info[1].IsBuffer()) {
        Napi::Buffer<uint8_t> buf = info[1].As<Napi::Buffer<uint8_t>>();
        data = buf.Data();
        dataSize = buf.Length();
    } else if (info[1].IsArrayBuffer()) {
        Napi::ArrayBuffer ab = info[1].As<Napi::ArrayBuffer>();
        data = static_cast<const uint8_t*>(ab.Data());
        dataSize = ab.ByteLength();
    } else if (info[1].IsTypedArray()) {
        Napi::TypedArray ta = info[1].As<Napi::TypedArray>();
        Napi::ArrayBuffer ab = ta.ArrayBuffer();
        data = static_cast<const uint8_t*>(ab.Data()) + ta.ByteOffset();
        dataSize = ta.ByteLength();
    } else {
        Napi::TypeError::New(env, "buffer/ArrayBuffer/TypedArray expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!data || dataSize == 0) {
        Napi::TypeError::New(env, "empty buffer").ThrowAsJavaScriptException();
        return env.Null();
    }

    SoundHandle handle = AudioManager::Instance().LoadSoundFromMemory(fileType, data, static_cast<int>(dataSize));
    return Napi::Number::New(env, handle);
}

Napi::Value AudioWrapper::PlaySound(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected for sound handle").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    SoundHandle handle = static_cast<SoundHandle>(info[0].As<Napi::Number>().Uint32Value());
    AudioManager::Instance().PlaySound(handle);
    
    return env.Undefined();
}

Napi::Value AudioWrapper::StopSound(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected for sound handle").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    SoundHandle handle = static_cast<SoundHandle>(info[0].As<Napi::Number>().Uint32Value());
    AudioManager::Instance().StopSound(handle);
    
    return env.Undefined();
}   


Napi::Value AudioWrapper::SetSoundVolume(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Number expected for sound handle and volume").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    SoundHandle handle = static_cast<SoundHandle>(info[0].As<Napi::Number>().Uint32Value());
    float volume = info[1].As<Napi::Number>().FloatValue();
    AudioManager::Instance().SetSoundVolume(handle, volume);
    
    return env.Undefined();
}

Napi::Value AudioWrapper::IsSoundPlaying(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected for sound handle").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    SoundHandle handle = static_cast<SoundHandle>(info[0].As<Napi::Number>().Uint32Value());
    bool isPlaying = AudioManager::Instance().IsSoundPlaying(handle);
    
    return Napi::Boolean::New(env, isPlaying);
}

Napi::Value AudioWrapper::UnloadSound(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected for sound handle").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    SoundHandle handle = static_cast<SoundHandle>(info[0].As<Napi::Number>().Uint32Value());
    AudioManager::Instance().UnloadSound(handle);
    
    return env.Undefined();
}

Napi::Value AudioWrapper::LoadMusic(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected for file path").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    std::string filePath = info[0].As<Napi::String>().Utf8Value();
    bool loop = false;
    if (info.Length() >= 2 && info[1].IsBoolean()) {
        loop = info[1].As<Napi::Boolean>().Value();
    }
    
    MusicHandle handle = AudioManager::Instance().LoadMusic(filePath, loop);
    
    return Napi::Number::New(env, handle);
}

Napi::Value AudioWrapper::LoadMusicFromMemory(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (fileType: string, buffer, loop?: boolean)").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string fileType = info[0].As<Napi::String>().Utf8Value();
    const uint8_t* data = nullptr;
    size_t dataSize = 0;

    if (info[1].IsBuffer()) {
        Napi::Buffer<uint8_t> buf = info[1].As<Napi::Buffer<uint8_t>>();
        data = buf.Data();
        dataSize = buf.Length();
    } else if (info[1].IsArrayBuffer()) {
        Napi::ArrayBuffer ab = info[1].As<Napi::ArrayBuffer>();
        data = static_cast<const uint8_t*>(ab.Data());
        dataSize = ab.ByteLength();
    } else if (info[1].IsTypedArray()) {
        Napi::TypedArray ta = info[1].As<Napi::TypedArray>();
        Napi::ArrayBuffer ab = ta.ArrayBuffer();
        data = static_cast<const uint8_t*>(ab.Data()) + ta.ByteOffset();
        dataSize = ta.ByteLength();
    } else {
        Napi::TypeError::New(env, "buffer/ArrayBuffer/TypedArray expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!data || dataSize == 0) {
        Napi::TypeError::New(env, "empty buffer").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool loop = false;
    if (info.Length() >= 3 && info[2].IsBoolean()) {
        loop = info[2].As<Napi::Boolean>().Value();
    }

    MusicHandle handle = AudioManager::Instance().LoadMusicFromMemory(fileType, data, static_cast<int>(dataSize), loop);
    return Napi::Number::New(env, handle);
}

Napi::Value AudioWrapper::PlayMusic(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected for music handle").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    MusicHandle handle = static_cast<MusicHandle>(info[0].As<Napi::Number>().Uint32Value());
    AudioManager::Instance().PlayMusic(handle);
    
    return env.Undefined();
}

Napi::Value AudioWrapper::StopMusic(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected for music handle").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    MusicHandle handle = static_cast<MusicHandle>(info[0].As<Napi::Number>().Uint32Value());
    AudioManager::Instance().StopMusic(handle);
    
    return env.Undefined();
}

Napi::Value AudioWrapper::SetMusicVolume(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Number expected for music handle and volume").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    MusicHandle handle = static_cast<MusicHandle>(info[0].As<Napi::Number>().Uint32Value());
    float volume = info[1].As<Napi::Number>().FloatValue();
    AudioManager::Instance().SetMusicVolume(handle, volume);
    
    return env.Undefined();
}

Napi::Value AudioWrapper::IsMusicPlaying(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected for music handle").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    MusicHandle handle = static_cast<MusicHandle>(info[0].As<Napi::Number>().Uint32Value());
    bool isPlaying = AudioManager::Instance().IsMusicPlaying(handle);
    
    return Napi::Boolean::New(env, isPlaying);
}

Napi::Value AudioWrapper::UnloadMusic(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected for music handle").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    MusicHandle handle = static_cast<MusicHandle>(info[0].As<Napi::Number>().Uint32Value());
    AudioManager::Instance().UnloadMusic(handle);
    
    return env.Undefined();
}

Napi::Value AudioWrapper::Shutdown(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    AudioManager::Instance().Shutdown();
    return env.Undefined();
}

Napi::Value AudioWrapper::GetMasterVolume(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    float volume = AudioManager::Instance().GetMasterVolume();
    return Napi::Number::New(env, volume);
}

Napi::Value AudioWrapper::CreateAudioStream(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsNumber()) {
        Napi::TypeError::New(env, "Expected (sampleRate: number, sampleSize: number, channels: number)").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    uint32_t sampleRate = info[0].As<Napi::Number>().Uint32Value();
    uint32_t sampleSize = info[1].As<Napi::Number>().Uint32Value();
    uint32_t channels = info[2].As<Napi::Number>().Uint32Value();
    
    AudioStreamHandle handle = AudioManager::Instance().CreateAudioStream(sampleRate, sampleSize, channels);
    return Napi::Number::New(env, handle);
}

Napi::Value AudioWrapper::UpdateAudioStream(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected (handle: number, buffer: Buffer|ArrayBuffer|TypedArray)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    AudioStreamHandle handle = static_cast<AudioStreamHandle>(info[0].As<Napi::Number>().Uint32Value());
    
    const uint8_t* data = nullptr;
    size_t dataSize = 0;
    
    if (info[1].IsBuffer()) {
        Napi::Buffer<uint8_t> buf = info[1].As<Napi::Buffer<uint8_t>>();
        data = buf.Data();
        dataSize = buf.Length();
    } else if (info[1].IsArrayBuffer()) {
        Napi::ArrayBuffer ab = info[1].As<Napi::ArrayBuffer>();
        data = static_cast<const uint8_t*>(ab.Data());
        dataSize = ab.ByteLength();
    } else if (info[1].IsTypedArray()) {
        Napi::TypedArray ta = info[1].As<Napi::TypedArray>();
        Napi::ArrayBuffer ab = ta.ArrayBuffer();
        data = static_cast<const uint8_t*>(ab.Data()) + ta.ByteOffset();
        dataSize = ta.ByteLength();
    } else {
        Napi::TypeError::New(env, "buffer/ArrayBuffer/TypedArray expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    if (!data || dataSize == 0) {
        Napi::TypeError::New(env, "empty buffer").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    // Calculate frameCount from buffer size
    // Assumes data is in PCM format: frameCount = bytes / (sampleSize/8 * channels)
    // For simplicity, accept optional frameCount parameter or calculate from buffer
    int frameCount = static_cast<int>(dataSize);
    if (info.Length() >= 3 && info[2].IsNumber()) {
        frameCount = info[2].As<Napi::Number>().Int32Value();
    }
    
    AudioManager::Instance().UpdateAudioStream(handle, data, frameCount);
    return env.Undefined();
}

Napi::Value AudioWrapper::PlayAudioStream(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected for stream handle").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    AudioStreamHandle handle = static_cast<AudioStreamHandle>(info[0].As<Napi::Number>().Uint32Value());
    AudioManager::Instance().PlayAudioStream(handle);
    
    return env.Undefined();
}

Napi::Value AudioWrapper::StopAudioStream(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected for stream handle").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    AudioStreamHandle handle = static_cast<AudioStreamHandle>(info[0].As<Napi::Number>().Uint32Value());
    AudioManager::Instance().StopAudioStream(handle);
    
    return env.Undefined();
}

Napi::Value AudioWrapper::PauseAudioStream(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected for stream handle").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    AudioStreamHandle handle = static_cast<AudioStreamHandle>(info[0].As<Napi::Number>().Uint32Value());
    AudioManager::Instance().PauseAudioStream(handle);
    
    return env.Undefined();
}

Napi::Value AudioWrapper::ResumeAudioStream(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected for stream handle").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    AudioStreamHandle handle = static_cast<AudioStreamHandle>(info[0].As<Napi::Number>().Uint32Value());
    AudioManager::Instance().ResumeAudioStream(handle);
    
    return env.Undefined();
}

Napi::Value AudioWrapper::UnloadAudioStream(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected for stream handle").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    AudioStreamHandle handle = static_cast<AudioStreamHandle>(info[0].As<Napi::Number>().Uint32Value());
    AudioManager::Instance().UnloadAudioStream(handle);
    
    return env.Undefined();
}

Napi::Value AudioWrapper::IsAudioStreamProcessed(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected for stream handle").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    AudioStreamHandle handle = static_cast<AudioStreamHandle>(info[0].As<Napi::Number>().Uint32Value());
    bool processed = AudioManager::Instance().IsAudioStreamProcessed(handle);
    
    return Napi::Boolean::New(env, processed);
}

Napi::Value AudioWrapper::SetMasterVolume(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected for volume").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    float volume = info[0].As<Napi::Number>().FloatValue();
    AudioManager::Instance().SetMasterVolume(volume);
    
    return env.Undefined();
}

Napi::Value AudioWrapper::SetMusicLooping(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsBoolean()) {
        Napi::TypeError::New(env, "Expected (musicHandle: number, loop: boolean)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    MusicHandle handle = static_cast<MusicHandle>(info[0].As<Napi::Number>().Uint32Value());
    bool loop = info[1].As<Napi::Boolean>().Value();
    AudioManager::Instance().SetMusicLooping(handle, loop);
    
    return env.Undefined();
}
