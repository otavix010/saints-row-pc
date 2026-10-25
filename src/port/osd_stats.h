// The on-screen meter's numbers that come from Windows (osd_stats.cpp), for fps_overlay.h.

#pragma once

namespace port::osd {

struct Stats {
  float cpu = -1, gpu = -1;  // percent, -1 unknown
  float cpu_temp = -1, gpu_temp = -1;  // degrees Celsius, -1 unknown
  double vram_used_mb = -1, vram_budget_mb = -1;
  double ram_used_mb = -1, ram_total_mb = -1;  // the system's
};

// the last reading, a second old at most; shown keeps the readings going, and not shown for a while lets them rest
Stats Latest(bool shown);

}  // namespace port::osd
