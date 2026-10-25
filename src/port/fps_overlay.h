// The game's frame rate, counted at each frame the game hands to the GPU (right after its VdSwap, the midasm hook
// PortFrame in config/ganchos.toml), and what is done with it:
//   mostrar_fps      the frame rate and the frame's time in the top-left corner (the game's own option)
//   OTAVIX_OSD=1     the launcher's performance meter: also the processor's and the graphics card's use and the video
//                    memory (osd_stats.cpp); F10 shows or hides either during the game
//   OTAVIX_DESEMPENHO=file   the session's frame rate for the launcher, written every 30 s and as the game closes:
//                    {"versao":1,"build":...,"fps":{"medio","p1","min","max","amostras"},"minutos":...}; a sample is a
//                    second, the first 30 s (the logos and the first load) and seconds above the frame rate asked
//                    (unpaced loading) are left out, p1 is the 1st percentile
// Development: PORT_FPS_LOG=file gets "<seconds since the start> <frames in that second>" once a second.

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include <imgui.h>
#include <rex/cvar.h>
#include <rex/ui/imgui_dialog.h>

#include "frame_rate.h"
#include "osd_stats.h"

REXCVAR_DEFINE_BOOL(mostrar_fps, false, "Port", "Show the frame rate in the top-left corner");


namespace port_perf {

inline std::atomic<uint64_t> g_frames{0};

inline bool OsdWanted() {
  const char* v = std::getenv("OTAVIX_OSD");
  return v && v[0] == '1';
}

// the session's seconds, and the file the launcher reads
class Recorder {
 public:
  // a second above the frame rate asked is no frame the player saw: a game that swaps unpaced while it loads (Saints
  // Row's first load: thousands a second)
  void Add(uint32_t fps, double t) {
    if (t < 30 || fps > uint32_t(TargetFps()) * 5 / 4 + 2) return;
    samples_.push_back(fps);
  }

  void Write() const {
    static const char* path = std::getenv("OTAVIX_DESEMPENHO");
    if (!path || samples_.empty()) return;
    std::vector<uint32_t> sorted(samples_);
    std::sort(sorted.begin(), sorted.end());
    double sum = 0;
    for (uint32_t v : sorted) sum += v;
    size_t n = sorted.size();
#ifdef PORT_CPU_NATIVO
    const char* build = "nativo";
#else
    const char* build = "generico";
#endif
    char json[512];
    std::snprintf(json, sizeof(json),
                  "{\"versao\":1,\"build\":\"%s\",\"fps\":{\"medio\":%.2f,\"p1\":%.2f,\"min\":%u,\"max\":%u,"
                  "\"amostras\":%zu},\"minutos\":%.1f}\n",
                  build, sum / double(n), double(sorted[n / 100]), sorted.front(), sorted.back(), n, double(n) / 60.0);
    std::string tmp = std::string(path) + ".tmp";
    if (FILE* f = std::fopen(tmp.c_str(), "wb")) {
      std::fputs(json, f);
      std::fclose(f);
      std::error_code ec;
      std::filesystem::rename(tmp, path, ec);
    }
  }

 private:
  std::vector<uint32_t> samples_;
};

inline Recorder g_recorder;

}  // namespace port_perf

// the frames come from one guest thread, so the second's state needs no lock
void PortFrame() {
  PortFrameLimit();
  uint64_t frames = port_perf::g_frames.fetch_add(1, std::memory_order_relaxed) + 1;
  using Clock = std::chrono::steady_clock;
  static const Clock::time_point start = Clock::now();
  static FILE* log = [] {
    const char* path = std::getenv("PORT_FPS_LOG");
    std::atexit([] { port_perf::g_recorder.Write(); });
    return path ? std::fopen(path, "w") : nullptr;
  }();
  static double next = 1, next_write = 60;
  static uint64_t last = 0;
  double t = std::chrono::duration<double>(Clock::now() - start).count();
  if (t < next) return;
  uint32_t fps = uint32_t(frames - last);
  last = frames;
  next = t + 1;
  port_perf::g_recorder.Add(fps, t);
  if (t >= next_write) {
    port_perf::g_recorder.Write();
    next_write = t + 30;
  }
  if (log) {
    std::fprintf(log, "%.0f %u\n", t, fps);
    std::fflush(log);
  }
}

class FpsOverlay final : public rex::ui::ImGuiDialog {
 public:
  explicit FpsOverlay(rex::ui::ImGuiDrawer* drawer)
      : ImGuiDialog(drawer), last_time_(Clock::now()), full_(port_perf::OsdWanted()) {}

 protected:
  void OnDraw(ImGuiIO& io) override {
    if (ImGui::IsKeyPressed(ImGuiKey_F10, false)) shown_ = !shown_;
    auto now = Clock::now();
    double seconds = std::chrono::duration<double>(now - last_time_).count();
    if (seconds >= 0.5) {
      uint64_t frames = port_perf::g_frames.load(std::memory_order_relaxed);
      fps_ = double(frames - last_frames_) / seconds;
      last_frames_ = frames;
      last_time_ = now;
    }
    port::osd::Stats stats = port::osd::Latest(shown_ && full_);
    if (!shown_) return;
    float scale = std::max(1.0f, io.DisplaySize.y / 1080.0f);
    ImGui::SetNextWindowPos(ImVec2(8 * scale, 8 * scale));
    ImGui::SetNextWindowBgAlpha(0.45f);
    constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("##fps", nullptr, kFlags)) {
      ImGui::SetWindowFontScale(full_ ? scale : 1.0f);
      if (fps_ > 0)
        ImGui::Text("%.0f fps  %.1f ms", fps_, 1000.0 / fps_);
      else
        ImGui::Text("-- fps");
      if (full_) {
        auto pct = [](float v) { return v < 0 ? std::string("--") : std::to_string(int(v + 0.5f)) + "%"; };
        ImGui::Text("CPU %s  GPU %s", pct(stats.cpu).c_str(), pct(stats.gpu).c_str());
        if (stats.vram_used_mb >= 0)
          ImGui::Text("VRAM %.1f / %.1f GB", stats.vram_used_mb / 1024.0, stats.vram_budget_mb / 1024.0);
      }
    }
    ImGui::End();
  }

 private:
  using Clock = std::chrono::steady_clock;
  Clock::time_point last_time_;
  uint64_t last_frames_ = 0;
  double fps_ = 0;
  bool full_;
  bool shown_ = true;
};
