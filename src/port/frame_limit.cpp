// The frame limiter's wait (frame_rate.h): until a point in time, to within a fraction of a millisecond, on a
// high-resolution waitable timer (Windows 10 1803 and later), the last moments spun.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <chrono>
#include <thread>

namespace port {

void WaitUntil(std::chrono::steady_clock::time_point when) {
  using namespace std::chrono;
  thread_local HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                                     TIMER_ALL_ACCESS);
  auto left = when - steady_clock::now();
  // the timer for all but the last half millisecond, which is spun
  if (timer && left > microseconds(700)) {
    LARGE_INTEGER due;
    due.QuadPart = -duration_cast<nanoseconds>(left - microseconds(500)).count() / 100;  // relative, 100 ns units
    if (SetWaitableTimerEx(timer, &due, 0, nullptr, nullptr, nullptr, 0)) WaitForSingleObject(timer, INFINITE);
  }
  while (steady_clock::now() < when) std::this_thread::yield();
}

}  // namespace port
