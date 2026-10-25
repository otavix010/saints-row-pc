// The SDK's InputSystem rebuilds its device list on every XamInputGetState/SetState with no lock (RefreshDevices), and
// the game reads the pad on one thread while it sets vibration on another: the two clear and refill the same vector and
// corrupt the heap (0xC0000374 in RefreshDevices under XamInputSetState, falha.log). Here the game's input imports are
// the port's own and go through one lock to the runtime's (looked up in rexruntime.dll by name).

#pragma once

#include <windows.h>

#include <mutex>

#include <rex/hook.h>

namespace input_lock {

inline std::mutex g_lock;

using GuestFunc = void (*)(PPCContext&, uint8_t*);

inline GuestFunc Original(const char* name) {
  HMODULE runtime = GetModuleHandleW(L"rexruntime.dll");
  return runtime ? reinterpret_cast<GuestFunc>(GetProcAddress(runtime, name)) : nullptr;
}

}  // namespace input_lock

#define PORT_LOCKED_IMPORT(name)                                                             \
  REX_HOOK_RAW(__imp__##name) {                                                            \
    static const input_lock::GuestFunc original = input_lock::Original("__imp__" #name);   \
    std::lock_guard<std::mutex> lock(input_lock::g_lock);                                  \
    original(ctx, base);                                                                   \
  }

PORT_LOCKED_IMPORT(XamInputGetState)
PORT_LOCKED_IMPORT(XamInputSetState)
PORT_LOCKED_IMPORT(XamInputGetCapabilities)
