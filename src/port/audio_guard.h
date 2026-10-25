// The runtime's AudioSystem::SubmitFrame uses a client's driver without checking it is still there, and a game's audio
// callback (on the runtime's audio thread) can submit a frame for a client the game has just unregistered: Saints Row
// did, starting up (a null read in SubmitFrame, falha.log). Here the game's render-driver imports go through one lock
// to the runtime's, a client is known alive between its registration and its unregistration, and a frame for one that
// is not is dropped. The audio thread lets go of the runtime's own lock before calling the game, so the two locks never
// wait on each other.

#pragma once

#include <array>
#include <mutex>

#include <rex/hook.h>

#include "input_lock.h"

namespace audio_guard {

inline std::mutex g_lock;
inline std::array<bool, 0x10000> g_alive{};

constexpr uint32_t kHandleTag = 0x41550000;  // the driver handles the runtime hands out: 0x4155xxxx, xxxx the client

inline uint32_t LoadGuest32(uint8_t* base, uint32_t address) {
  return __builtin_bswap32(*reinterpret_cast<uint32_t*>(base + address));
}

}  // namespace audio_guard

// XAudioRegisterRenderDriverClient(callback_ptr, driver_ptr): the handle written to *driver_ptr is alive from here
REX_HOOK_RAW(__imp__XAudioRegisterRenderDriverClient) {
  static const input_lock::GuestFunc original = input_lock::Original("__imp__XAudioRegisterRenderDriverClient");
  std::lock_guard<std::mutex> lock(audio_guard::g_lock);
  uint32_t driver_ptr = ctx.r4.u32;
  original(ctx, base);
  if (ctx.r3.u32 == 0 && driver_ptr) {
    uint32_t handle = audio_guard::LoadGuest32(base, driver_ptr);
    if ((handle & 0xFFFF0000) == audio_guard::kHandleTag) audio_guard::g_alive[handle & 0xFFFF] = true;
  }
}

// XAudioUnregisterRenderDriverClient(driver): not alive from here
REX_HOOK_RAW(__imp__XAudioUnregisterRenderDriverClient) {
  static const input_lock::GuestFunc original = input_lock::Original("__imp__XAudioUnregisterRenderDriverClient");
  std::lock_guard<std::mutex> lock(audio_guard::g_lock);
  audio_guard::g_alive[ctx.r3.u32 & 0xFFFF] = false;
  original(ctx, base);
}

// XAudioSubmitRenderDriverFrame(driver, samples): only to a client alive; otherwise dropped, X_ERROR_SUCCESS
REX_HOOK_RAW(__imp__XAudioSubmitRenderDriverFrame) {
  static const input_lock::GuestFunc original = input_lock::Original("__imp__XAudioSubmitRenderDriverFrame");
  std::lock_guard<std::mutex> lock(audio_guard::g_lock);
  uint32_t handle = ctx.r3.u32;
  if ((handle & 0xFFFF0000) != audio_guard::kHandleTag || !audio_guard::g_alive[handle & 0xFFFF]) {
    ctx.r3.u64 = 0;
    return;
  }
  original(ctx, base);
}
