// The guest's atomic sections: "interrupts off" around a lwarx/stwcx. loop (mfmsr / mtmsrd r13,1 ... mtmsrd rN,1),
// the console's way to keep its own core's interrupts out of an update. The SDK's code generation turns them into the
// runtime's global lock, the one the GPU's command processor takes on every draw for its buffers and textures and the
// kernel for its objects: with ten game threads updating counters and lists all the time, they queued on one lock
// and the GPU's thread behind them (Saints Row: 12-16% of that thread waiting). Worst with fewer cores than busy
// threads, as on a 5600X: one preempted while it holds the lock stops all the others (Saints Row on 3 cores of the
// bench, standing in the city without the game's cap: 6770 frames in 80 s; by address, 7549, 11% more).
// The stwcx. is a compare-and-swap of the value, so the lock is still needed for what the console's reservation also
// catches: another thread taking the value and putting it back in between (a lock-free list's pop). Locking by the
// address does that: scripts/reservas.ps1, run after the code generation, moves each section's lock below its lwarx's
// address (REX_ENTER_RESERVATION(ea)), one lock per 128-byte line (the console's reservation granule) out of 1024;
// sections on other lines never wait for each other, nor for the runtime. Every section of the four ports is just its
// lwarx..stwcx. (no call inside), so a thread holds one lock at a time. A section of another shape keeps the global
// lock (REX_ENTER_GLOBAL_LOCK).
//   PORT_TRAVA_GUEST=global   every section on the global lock, as the SDK does (development: to compare)
//   PORT_TRAVA_GUEST=nenhuma  no lock, only the compare-and-swap (development: to compare)
// port.cmake adds this header after the generated one, whose macros it replaces.

#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>

#include <rex/thread/mutex.h>

namespace port::guest_lock {

enum class Mode { kAddress, kGlobal, kNone };

inline Mode ModeFromEnv() {
  const char* v = std::getenv("PORT_TRAVA_GUEST");
  if (v && std::strcmp(v, "global") == 0) return Mode::kGlobal;
  if (v && std::strcmp(v, "nenhuma") == 0) return Mode::kNone;
  return Mode::kAddress;
}

inline const Mode g_mode = ModeFromEnv();

struct alignas(64) Line {
  std::mutex mutex;
};
inline Line g_lines[1024];

// this thread's sections, one inside the other (mfmsr reads it), and the lock its outer one took
inline thread_local int t_depth = 0;
inline thread_local std::mutex* t_line = nullptr;
inline thread_local bool t_global = false;

inline void EnterReservation(uint32_t address) {
  if (t_depth++ != 0 || g_mode == Mode::kNone) return;
  if (g_mode == Mode::kGlobal) {
    rex::thread::global_critical_region::mutex().lock();
    t_global = true;
    return;
  }
  t_line = &g_lines[(address >> 7) & 1023].mutex;
  t_line->lock();
}

inline void EnterGlobal() {
  if (t_depth++ != 0 || g_mode == Mode::kNone) return;
  rex::thread::global_critical_region::mutex().lock();
  t_global = true;
}

inline void Leave() {
  if (--t_depth != 0) return;
  if (t_global) {
    t_global = false;
    rex::thread::global_critical_region::mutex().unlock();
  } else if (t_line) {
    std::mutex* line = t_line;
    t_line = nullptr;
    line->unlock();
  }
}

}  // namespace port::guest_lock

// mfmsr: interrupts on (0x8000) unless this thread is inside a section, as the SDK's count said once the lock was its
#undef REX_CHECK_GLOBAL_LOCK
#undef REX_ENTER_GLOBAL_LOCK
#undef REX_LEAVE_GLOBAL_LOCK
#define REX_CHECK_GLOBAL_LOCK() (::port::guest_lock::t_depth ? uint64_t(0) : uint64_t(0x8000))
#define REX_ENTER_GLOBAL_LOCK() ::port::guest_lock::EnterGlobal()
#define REX_ENTER_RESERVATION(ea) ::port::guest_lock::EnterReservation(ea)
#define REX_LEAVE_GLOBAL_LOCK() ::port::guest_lock::Leave()
