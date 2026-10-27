// saintsrow - ReXGlue Recompiled Project
//
// The port's app: the common base of the Xbox 360 ports (port/port_app.h) and Saints Row's own part: the game's D3D (an
// older XDK) presents a frame every 2 vertical blanks and times its play by the clock, so its frame rate is the
// blank's: port/frame_rate.h runs the console's blank at twice the fps option (60 fps, a blank at 120 Hz).
// The physics (Havok) steps havok_step_time at a time, at most havok_max_time_slices (3) steps a frame; at a blank of
// 120 Hz the step is 1/120 s, so a frame got at most 0.025 s of physics and anything under 40 fps ran in slow motion
// (walking at 22 fps took twice as long). 6 steps are the console's 0.05 s a frame again (at 30 fps, a 1/60 s step):
// real speed down to 20 fps.
// audio_pool.h: the race in the shutdown of the game's audio worker pool that froze the start now and then.

#pragma once

#include "audio_pool.h"
#include "port/port_app.h"

class SaintsRowApp : public PortApp {
 public:
  using PortApp::PortApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(rex::ui::WindowedAppContext& ctx) {
    crash_log::Install();
    port::g_blanks_per_frame = 2;
    return std::unique_ptr<SaintsRowApp>(new SaintsRowApp(ctx, "saintsrow", PPCImageConfig));
  }

  void OnPostLoadXexImage() override {
    PortApp::OnPostLoadXexImage();
    // havok_max_time_slices, read by sub_8224BF50 (the steps a frame) and set only by its console command
    auto* slices = runtime()->memory()->TranslateVirtual<uint32_t*>(0x827D09E0);
    if (__builtin_bswap32(*slices) == 3) *slices = __builtin_bswap32(6);
  }
};
