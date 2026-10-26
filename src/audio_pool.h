// The game's audio worker pool (one guest thread here, sub_8260CAD8) and a race in its shutdown that froze the start
// about 1 in 10 times, on a black screen (every thread waiting, the GPU idle):
//   - the game's audio callback (sub_826230C8, on the runtime's audio thread) hands each pass to the worker
//     (sub_8260D180): it sets the pool's job word (+300), sets the work event (0x832AD680, auto-reset) and waits for
//     the done event, and then clears the job word;
//   - the pool's shutdown (sub_8260C5F8, on the main thread, starting up) unregisters the callback, sets the same work
//     event to tell the worker to leave (it leaves when it wakes to a clear job word) and waits for its thread.
// The runtime unregisters an audio client without waiting for a callback already running, so the shutdown's event
// could reach the worker while a pass was still being handed over: the worker woke to the job word still set, did
// the pass and went back to wait for an event nobody would set, and the main thread waited for it forever.
// Here the shutdown waits for a pass in flight to end before it sets the event, and a pass that starts while the pool
// is shutting down is skipped (a callback the game has just unregistered: one pass of mixing, no sound lost).
// The hooks are in config/ganchos.toml.

#pragma once

#include <atomic>
#include <chrono>
#include <thread>

#include <rex/logging.h>

namespace audio_pool {

inline std::atomic<int> g_in_flight{0};
inline std::atomic<bool> g_closing{false};

}  // namespace audio_pool

// at the start of the hand-over (sub_8260D180): true skips it, returning 0
bool PortAudioPassBegin(PPCRegister& r3) {
  audio_pool::g_in_flight.fetch_add(1);
  if (!audio_pool::g_closing.load()) return false;
  audio_pool::g_in_flight.fetch_sub(1);
  r3.u64 = 0;
  return true;
}

// at its end, after it let go of its lock
void PortAudioPassEnd() { audio_pool::g_in_flight.fetch_sub(1); }

// in the shutdown, after the callback is unregistered and before the worker is told to leave
void PortAudioPoolClosing() {
  audio_pool::g_closing.store(true);
  auto start = std::chrono::steady_clock::now();
  bool waited = false;
  while (audio_pool::g_in_flight.load() > 0 && std::chrono::steady_clock::now() - start < std::chrono::seconds(5)) {
    waited = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  if (waited)
    REXLOG_INFO("audio pool: shutdown waited {} ms for a pass in flight",
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());
}

// after the worker's thread is gone: a new pool hands passes over again
void PortAudioPoolClosed() { audio_pool::g_closing.store(false); }
