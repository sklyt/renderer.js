#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include "console_control.h"

using namespace ConsoleControl;

Napi::Value HideConsole(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  HWND hwnd = GetConsoleWindow();
  if (hwnd) {
    ShowWindow(hwnd, SW_HIDE);
    return Napi::Boolean::New(env, true);
  }
  // nothing to hide
  return Napi::Boolean::New(env, false);
}

Napi::Value ShowConsole(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  HWND hwnd = GetConsoleWindow();
  if (!hwnd) {
    if (!AllocConsole()) {
      return Napi::Boolean::New(env, false);
    }
    hwnd = GetConsoleWindow();
  }

  ShowWindow(hwnd, SW_SHOW);

  // Rebind C stdio streams to the console so printf/puts works
  FILE* in = nullptr; FILE* out = nullptr; FILE* err = nullptr;
  freopen_s(&in, "CONIN$", "r", stdin);
  freopen_s(&out, "CONOUT$", "w", stdout);
  freopen_s(&err, "CONOUT$", "w", stderr);

  // Make C++ iostreams use the console
  std::ios::sync_with_stdio();

  return Napi::Boolean::New(env, true);
}

#endif // _WIN32
