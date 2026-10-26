// saintsrow - ReXGlue Recompiled Project
//
// The port's app: the common base of the Xbox 360 ports (port/port_app.h) and Saints Row's own part: the game's D3D (an
// older XDK) presents a frame every 2 vertical blanks and times its play by the clock, so its frame rate is the
// blank's: port/frame_rate.h runs the console's blank at twice the fps option (60 fps, a blank at 120 Hz).
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
};
