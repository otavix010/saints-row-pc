// fps: the frame rate, the same option in every port (x360-ports/comum). Two parts hold here; a game's own header adds
// what its engine needs besides (a tick-rate cap, say):
//   - PortPresentInterval: a midasm hook right after the XDK's D3D sets the presentation interval of a game at 30,
//     li r11,2 (a frame every 2 vertical blanks); at 60 or more it becomes 1. The same library code is in many
//     games, found by its shape: the interval read from the device, compared with 0, 1 and 2, then addi r11,r11,-4.
//   - ApplyFrameRate: above 60 the console's vertical blank itself runs faster (video_mode_refresh_rate, unless set):
//     120 fps is a blank every 8.3 ms. A game at 60 on the console follows it with no hook.

#pragma once

#include <string>

#include <rex/cvar.h>
#include <rex/ppc.h>

REXCVAR_DEFINE_INT32(fps, 60, "Port", "Frame rate: 30, 60, 120, 144...");

inline bool HighFrameRate() { return REXCVAR_GET(fps) >= 60; }

// before the GPU starts (PortApp::OnPreSetup)
inline void ApplyFrameRate() {
  int fps = REXCVAR_GET(fps);
  if (fps > 60 && rex::cvar::GetFlagSource("video_mode_refresh_rate") == rex::cvar::Source::kDefault)
    rex::cvar::SetFlagByName("video_mode_refresh_rate", std::to_string(fps));
}

void PortPresentInterval(PPCRegister& r11) {
  if (HighFrameRate() && r11.u32 == 2) r11.u64 = 1;
}
