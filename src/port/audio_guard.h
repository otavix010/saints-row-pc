// The runtime's AudioSystem::SubmitFrame uses a client's driver without checking it is still there, and a game's audio
// callback (on the runtime's audio thread) can submit a frame for a client the game has just unregistered: Saints Row
// did, starting up (a null read in SubmitFrame, falha.log). Here the game's render-driver imports go through one lock
// to the runtime's, a client is known gone between its unregistration and a new registration of its index, and a frame
// for one that is gone is dropped. The audio thread lets go of the runtime's own lock before calling the game, so the
// two locks never wait on each other.
//
// Only frames for a client known gone are dropped: a frame the game submits is what keeps the runtime's audio thread
// calling the game back, and a game whose sound engine stops (DiRT 2's waits for it to load a race) must never be the
// price of a wrong guess. The handle is read through GuestPtr, since DiRT 2 keeps it in the physical range
// (0xE0000000 and up), which the host maps 0x1000 further on.

#pragma once

#include <array>
#include <mutex>

#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/system/xmemory.h>

#include "input_lock.h"

namespace audio_guard {

inline std::mutex g_lock;
inline std::array<bool, 0x10000> g_gone{};

constexpr uint32_t kHandleTag = 0x41550000;  // the driver handles the runtime hands out: 0x4155xxxx, xxxx the client

inline uint32_t LoadGuest32(uint8_t* base, uint32_t address) {
  return __builtin_bswap32(*rex::memory::GuestPtr<uint32_t*>(base, address));
}

}  // namespace audio_guard

// XAudioRegisterRenderDriverClient(callback_ptr, driver_ptr): the client in the handle written to *driver_ptr is there
REX_HOOK_RAW(__imp__XAudioRegisterRenderDriverClient) {
  static const input_lock::GuestFunc original = input_lock::Original("__imp__XAudioRegisterRenderDriverClient");
  std::lock_guard<std::mutex> lock(audio_guard::g_lock);
  uint32_t driver_ptr = ctx.r4.u32;
  original(ctx, base);
  if (ctx.r3.u32 == 0 && driver_ptr) {
    uint32_t handle = audio_guard::LoadGuest32(base, driver_ptr);
    if ((handle & 0xFFFF0000) == audio_guard::kHandleTag) {
      audio_guard::g_gone[handle & 0xFFFF] = false;
    } else {
      REXLOG_WARN("audio guard: client registered with an unexpected handle {:08X}", handle);
    }
  }
}

// XAudioUnregisterRenderDriverClient(driver): gone from here
REX_HOOK_RAW(__imp__XAudioUnregisterRenderDriverClient) {
  static const input_lock::GuestFunc original = input_lock::Original("__imp__XAudioUnregisterRenderDriverClient");
  std::lock_guard<std::mutex> lock(audio_guard::g_lock);
  audio_guard::g_gone[ctx.r3.u32 & 0xFFFF] = true;
  original(ctx, base);
}

// XAudioSubmitRenderDriverFrame(driver, samples): to a client gone, dropped (X_ERROR_SUCCESS); otherwise to the runtime
REX_HOOK_RAW(__imp__XAudioSubmitRenderDriverFrame) {
  static const input_lock::GuestFunc original = input_lock::Original("__imp__XAudioSubmitRenderDriverFrame");
  static int dropped = 0;
  std::lock_guard<std::mutex> lock(audio_guard::g_lock);
  uint32_t handle = ctx.r3.u32;
  if (audio_guard::g_gone[handle & 0xFFFF]) {
    if (dropped++ < 5) REXLOG_WARN("audio guard: frame for client {:08X}, unregistered, dropped", handle);
    ctx.r3.u64 = 0;
    return;
  }
  original(ctx, base);
}
