// The on-screen meter's numbers that come from Windows (fps_overlay.h): the system's processor use, the graphics
// card's 3D engines' use (the counter Task Manager shows) and this process's video memory against its budget. They are
// read once a second on a thread of their own, and only while the meter is on screen, so that no frame of the game
// waits for them.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <dxgi1_4.h>
#include <pdh.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

#include "osd_stats.h"

namespace port::osd {

namespace {

std::mutex g_lock;
Stats g_latest;
std::atomic<bool> g_wanted{false};
std::once_flag g_started;

uint64_t Ticks(const FILETIME& f) { return (uint64_t(f.dwHighDateTime) << 32) | f.dwLowDateTime; }

void Run() {
  uint64_t idle0 = 0, kernel0 = 0, user0 = 0;
  PDH_HQUERY query = nullptr;
  PDH_HCOUNTER gpu_counter = nullptr;
  if (PdhOpenQueryW(nullptr, 0, &query) == ERROR_SUCCESS &&
      PdhAddEnglishCounterW(query, L"\\GPU Engine(*engtype_3D)\\Utilization Percentage", 0, &gpu_counter) == ERROR_SUCCESS) {
    PdhCollectQueryData(query);
  }
  IDXGIAdapter3* adapter = nullptr;
  IDXGIFactory1* factory = nullptr;
  if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
    IDXGIAdapter1* a = nullptr;
    for (UINT i = 0; factory->EnumAdapters1(i, &a) == S_OK; ++i) {
      DXGI_ADAPTER_DESC1 desc = {};
      a->GetDesc1(&desc);
      bool hardware = !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE);
      if (hardware) a->QueryInterface(IID_PPV_ARGS(&adapter));
      a->Release();
      if (hardware) break;
    }
    factory->Release();
  }
  std::vector<char> buffer;
  for (;;) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (!g_wanted.load(std::memory_order_relaxed)) continue;
    Stats s;
    FILETIME idle, kernel, user;
    if (GetSystemTimes(&idle, &kernel, &user)) {
      uint64_t di = Ticks(idle) - idle0, dk = Ticks(kernel) - kernel0, du = Ticks(user) - user0;
      if (idle0 && dk + du > 0) s.cpu = float(100.0 * double(dk + du - di) / double(dk + du));
      idle0 = Ticks(idle);
      kernel0 = Ticks(kernel);
      user0 = Ticks(user);
    }
    if (gpu_counter && PdhCollectQueryData(query) == ERROR_SUCCESS) {
      DWORD size = 0, count = 0;
      PdhGetFormattedCounterArrayW(gpu_counter, PDH_FMT_DOUBLE, &size, &count, nullptr);
      buffer.resize(size);
      auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
      if (size && PdhGetFormattedCounterArrayW(gpu_counter, PDH_FMT_DOUBLE, &size, &count, items) == ERROR_SUCCESS) {
        double sum = 0;
        for (DWORD i = 0; i < count; ++i)
          if (items[i].FmtValue.CStatus == 0) sum += items[i].FmtValue.doubleValue;  // PDH_CSTATUS_VALID_DATA
        s.gpu = float(sum > 100 ? 100 : sum);
      }
    }
    DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
    if (adapter && SUCCEEDED(adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info))) {
      s.vram_used_mb = double(info.CurrentUsage) / (1024.0 * 1024.0);
      s.vram_budget_mb = double(info.Budget) / (1024.0 * 1024.0);
    }
    std::lock_guard<std::mutex> lock(g_lock);
    g_latest = s;
  }
}

}  // namespace

Stats Latest(bool shown) {
  g_wanted.store(shown, std::memory_order_relaxed);
  if (shown) std::call_once(g_started, [] { std::thread(Run).detach(); });
  std::lock_guard<std::mutex> lock(g_lock);
  return g_latest;
}

}  // namespace port::osd
