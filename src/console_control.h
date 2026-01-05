#pragma once
#include <napi.h>

namespace ConsoleControl {
  Napi::Value HideConsole(const Napi::CallbackInfo &info);
  Napi::Value ShowConsole(const Napi::CallbackInfo &info);
}
