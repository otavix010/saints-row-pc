// fps: the frame rate, the same option in every port (x360-ports/comum). Two parts hold here; a game's own header adds
// what its engine needs besides (a tick-rate cap, say):
//   - PortPresentInterval: a midasm hook right after the XDK's D3D sets the presentation interval of a game at 30,
//     li r11,2 (a frame every 2 vertical blanks); at 60 or more it becomes 1. The same library code is in many
//     games, found by its shape: the interval read from the device, compared with 0, 1 and 2, then addi r11,r11,-4.
//   - ApplyFrameRate: the console's vertical blank runs at fps times the blanks the game waits a frame
//     (video_mode_refresh_rate, unless set), when that is above 60. A game at 60 on the console, or one whose interval
//     the hook above makes 1, waits 1: 120 fps is a blank every 8.3 ms. A game whose own code waits 2 and has no such
//     hook (Saints Row, an older XDK) sets g_blanks_per_frame = 2: 60 fps is then a blank at 120 Hz (the runtime goes
//     up to 240). Such a game must time itself by the clock, not by the blanks; one that counts blanks runs fast.
//   - fps 0: unlimited, which is the limit below: the blank at 240 Hz.
//   - PortFrameLimit: no game goes past 240 frames a second. Some swap unpaced while they load (DiRT 2's first load
//     counted 5,000 a second): the frame waits out the rest of its 1/240 s, which is still 4 times the console's pace.

#pragma once

#include <chrono>
#include <string>

#include <rex/cvar.h>
#include <rex/ppc.h>

REXCVAR_DEFINE_INT32(fps, 60, "Port", "Frame rate: 30, 60, 120, 144..., or 0 for unlimited (up to 240)");

namespace port {
inline int g_blanks_per_frame = 1;  // a game's app sets 2 before setup when its code waits 2 blanks a frame
constexpr int kFpsLimit = 240;
void WaitUntil(std::chrono::steady_clock::time_point when);  // frame_limit.cpp
}  // namespace port

// the frame rate asked, unlimited (0) as the limit
inline int TargetFps() {
  int fps = REXCVAR_GET(fps);
  return fps <= 0 || fps > port::kFpsLimit ? port::kFpsLimit : fps;
}

inline bool HighFrameRate() { return TargetFps() >= 60; }

// at each frame the game hands to the GPU (fps_overlay.h's PortFrame), on the game's thread
inline void PortFrameLimit() {
  using Clock = std::chrono::steady_clock;
  static Clock::time_point next{};
  auto now = Clock::now();
  if (now < next) {
    port::WaitUntil(next);
    now = next;
  }
  // a frame late does not earn the next ones a rush
  next = now + std::chrono::nanoseconds(1'000'000'000 / port::kFpsLimit);
}

// before the GPU starts (PortApp::OnPreSetup)
inline void ApplyFrameRate() {
  int refresh = TargetFps() * port::g_blanks_per_frame;
  if (refresh > 60 && rex::cvar::GetFlagSource("video_mode_refresh_rate") == rex::cvar::Source::kDefault)
    rex::cvar::SetFlagByName("video_mode_refresh_rate", std::to_string(refresh > 240 ? 240 : refresh));
}

void PortPresentInterval(PPCRegister& r11) {
  if (HighFrameRate() && r11.u32 == 2) r11.u64 = 1;
}
