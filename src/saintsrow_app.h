// saintsrow - ReXGlue Recompiled Project
//
// The port's app: the common base of the Xbox 360 ports (port/port_app.h) and Saints Row's own hooks.

#pragma once

#include "port/port_app.h"

class SaintsRowApp : public PortApp {
 public:
  using PortApp::PortApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(rex::ui::WindowedAppContext& ctx) {
    crash_log::Install();
    return std::unique_ptr<SaintsRowApp>(new SaintsRowApp(ctx, "saintsrow", PPCImageConfig));
  }
};
