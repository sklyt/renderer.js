#ifndef _WIN32

#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include "console_control.h"

using namespace ConsoleControl;

Napi::Value HideConsole(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // Create a new session so we are not attached to a controlling tty
  if (setsid() == -1) {
    // continue anyway; not fatal
  }

  FILE *devnull = freopen("/dev/null", "w", stdout);
  if (!devnull) {
    return Napi::Boolean::New(env, false);
  }
  freopen("/dev/null", "w", stderr);
  // stdin left alone intentionally
  return Napi::Boolean::New(env, true);
}

Napi::Value ShowConsole(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // Best-effort: try to reopen controlling tty
  FILE *tty = fopen("/dev/tty", "r+");
  if (!tty) {
    // No tty available (e.g. backgrounded, launched by systemd). Cannot reattach.
    return Napi::Boolean::New(env, false);
  }

  fflush(stdout);
  fflush(stderr);
  int fd = fileno(tty);
  if (fd < 0) {
    fclose(tty);
    return Napi::Boolean::New(env, false);
  }

  dup2(fd, STDOUT_FILENO);
  dup2(fd, STDERR_FILENO);
  // optional: dup2(fd, STDIN_FILENO);

  fclose(tty);
  return Napi::Boolean::New(env, true);
}

#endif // !_WIN32
