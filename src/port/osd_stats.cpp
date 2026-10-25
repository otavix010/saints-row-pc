// The on-screen meter's numbers that come from Windows (fps_overlay.h), read once a second on a thread of their own,
// and only while the meter is on screen, so that no frame of the game waits for them:
//   - the processor: the system's use (GetSystemTimes) and the hottest thermal zone (the "Thermal Zone Information"
//     counters, in tenths of a kelvin; some desktops have none, and then it shows --);
//   - the graphics card the runtime takes (the first hardware adapter): the busiest of its 3D engines, the counter
//     Task Manager shows ("GPU Engine", only the instances of that adapter's LUID: a laptop's other GPU is left out),
//     its temperature (D3DKMTQueryAdapterInfo, the adapter's performance data, tenths of a degree: Task Manager's too,
//     for NVIDIA, AMD and Intel alike) and this process's video memory against its budget (DXGI);
//   - the system's memory in use and in all.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <dxgi1_4.h>
#include <pdh.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cwchar>
#include <cwctype>
#include <map>
#include <mutex>
#include <string>
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

// the kernel's adapter queries (gdi32), as d3dkmthk.h has them
using KmtHandle = UINT;
struct KmtOpenFromLuid {
  LUID luid;
  KmtHandle adapter;
};
struct KmtQueryInfo {
  KmtHandle adapter;
  UINT type;
  void* data;
  UINT size;
};
struct KmtAdapterPerfData {  // D3DKMT_ADAPTER_PERFDATA
  UINT32 physical_adapter_index;
  ULONGLONG memory_frequency, max_memory_frequency, max_memory_frequency_oc, memory_bandwidth, pcie_bandwidth;
  ULONG fan_rpm, power, temperature;  // temperature in tenths of a degree Celsius
  UCHAR power_state_override;
};
constexpr UINT kAdapterPerfData = 62;  // KMTQAITYPE_ADAPTERPERFDATA
using OpenFn = LONG(APIENTRY*)(KmtOpenFromLuid*);
using QueryFn = LONG(APIENTRY*)(KmtQueryInfo*);

// a PDH counter with instances: each instance's value
bool Values(PDH_HCOUNTER counter, std::vector<char>& buffer, std::vector<std::pair<std::wstring, double>>& out) {
  out.clear();
  DWORD size = 0, count = 0;
  PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &size, &count, nullptr);
  if (!size) return false;
  buffer.resize(size);
  auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
  if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &size, &count, items) != ERROR_SUCCESS) return false;
  for (DWORD i = 0; i < count; ++i)
    if (items[i].FmtValue.CStatus == 0) out.push_back({items[i].szName, items[i].FmtValue.doubleValue});  // valid data
  return true;
}

void Run() {
  uint64_t idle0 = 0, kernel0 = 0, user0 = 0;
  // the adapter: DXGI for its memory and its LUID
  IDXGIAdapter3* adapter = nullptr;
  LUID luid{};
  IDXGIFactory1* factory = nullptr;
  if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
    IDXGIAdapter1* a = nullptr;
    for (UINT i = 0; factory->EnumAdapters1(i, &a) == S_OK; ++i) {
      DXGI_ADAPTER_DESC1 desc = {};
      a->GetDesc1(&desc);
      bool hardware = !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE);
      if (hardware) {
        a->QueryInterface(IID_PPV_ARGS(&adapter));
        luid = desc.AdapterLuid;
      }
      a->Release();
      if (hardware) break;
    }
    factory->Release();
  }
  wchar_t luid_text[48];
  std::swprintf(luid_text, 48, L"luid_0x%08x_0x%08x", unsigned(luid.HighPart), unsigned(luid.LowPart));
  // its temperature, through the kernel; the adapter stays open as long as the game
  HMODULE gdi = GetModuleHandleW(L"gdi32.dll");
  if (!gdi) gdi = LoadLibraryW(L"gdi32.dll");
  auto open = gdi ? reinterpret_cast<OpenFn>(GetProcAddress(gdi, "D3DKMTOpenAdapterFromLuid")) : nullptr;
  auto query = gdi ? reinterpret_cast<QueryFn>(GetProcAddress(gdi, "D3DKMTQueryAdapterInfo")) : nullptr;
  KmtOpenFromLuid opened{luid, 0};
  bool kmt = adapter && open && query && open(&opened) == 0;
  // the counters
  PDH_HQUERY pdh = nullptr;
  PDH_HCOUNTER gpu_counter = nullptr, zone_counter = nullptr;
  bool zone_tenths = true;
  if (PdhOpenQueryW(nullptr, 0, &pdh) == ERROR_SUCCESS) {
    PdhAddEnglishCounterW(pdh, L"\\GPU Engine(*engtype_3D)\\Utilization Percentage", 0, &gpu_counter);
    if (PdhAddEnglishCounterW(pdh, L"\\Thermal Zone Information(*)\\High Precision Temperature", 0, &zone_counter) !=
        ERROR_SUCCESS) {
      zone_tenths = false;
      PdhAddEnglishCounterW(pdh, L"\\Thermal Zone Information(*)\\Temperature", 0, &zone_counter);
    }
    PdhCollectQueryData(pdh);
  }
  std::vector<char> buffer;
  std::vector<std::pair<std::wstring, double>> values;
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
    if (pdh && PdhCollectQueryData(pdh) == ERROR_SUCCESS) {
      // the adapter's engines, each summed over the processes using it: the busiest
      if (gpu_counter && Values(gpu_counter, buffer, values)) {
        std::map<std::wstring, double> engines;
        for (auto& [name, v] : values) {
          std::wstring lower(name);
          std::transform(lower.begin(), lower.end(), lower.begin(), [](wchar_t c) { return wchar_t(std::towlower(c)); });
          size_t at = lower.find(luid_text);
          if (at != std::wstring::npos) engines[lower.substr(at)] += v;  // the name without its process
        }
        double busiest = 0;
        for (auto& [_, v] : engines) busiest = std::max(busiest, v);
        if (!engines.empty()) s.gpu = float(std::min(busiest, 100.0));
      }
      if (zone_counter && Values(zone_counter, buffer, values)) {
        double hottest = -1;
        for (auto& [_, v] : values) {
          double c = zone_tenths ? v / 10.0 - 273.15 : v - 273.15;
          if (c > 0 && c < 150) hottest = std::max(hottest, c);
        }
        s.cpu_temp = float(hottest);
      }
    }
    if (kmt) {
      KmtAdapterPerfData perf{};
      KmtQueryInfo q{opened.adapter, kAdapterPerfData, &perf, UINT(sizeof(perf))};
      if (query(&q) == 0 && perf.temperature > 0) s.gpu_temp = float(perf.temperature) / 10.0f;
    }
    DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
    if (adapter && SUCCEEDED(adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info))) {
      s.vram_used_mb = double(info.CurrentUsage) / (1024.0 * 1024.0);
      s.vram_budget_mb = double(info.Budget) / (1024.0 * 1024.0);
    }
    MEMORYSTATUSEX memory{};
    memory.dwLength = sizeof(memory);
    if (GlobalMemoryStatusEx(&memory)) {
      s.ram_total_mb = double(memory.ullTotalPhys) / (1024.0 * 1024.0);
      s.ram_used_mb = double(memory.ullTotalPhys - memory.ullAvailPhys) / (1024.0 * 1024.0);
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
