#pragma once

// Prevent pulling in winsock.h in windows.h, which breaks uv.h
#define WIN32_LEAN_AND_MEAN

#include <functional>
#include <thread>
#include <unordered_map>

#include <Windows.h>

#include "napi/async.hh"

template <typename T>
using unique_function = std::unique_ptr<std::function<T>>;

struct EnvData;

struct NotifyIconMessageLoop {
  HWND hwnd = nullptr;
  std::thread thread;
  NapiThreadsafeFunction run_on_env_thread;

  ~NotifyIconMessageLoop();

  UINT notify_message();

  napi_status init(EnvData* data);
  void quit();

  void run_on_msg_thread_blocking_(unique_function<void()> body);
  void run_on_msg_thread_nonblocking_(unique_function<void()> body);

  template <typename Body>
  void run_on_msg_thread_blocking(Body&& body) {
    run_on_msg_thread_blocking_(
        std::make_unique<std::function<void()>>(std::move(body)));
  }

  template <typename Body>
  void run_on_msg_thread_nonblocking(Body&& body) {
    run_on_msg_thread_nonblocking_(
        std::make_unique<std::function<void()>>(std::move(body)));
  }
};
