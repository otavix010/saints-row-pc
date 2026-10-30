// Fatal exceptions the runtime's own handler leaves alone go to falha.log beside the executable: the code, the
// address and the stack as module+offset (scripts/pilha.py names the frames from the linker map and the runtime's
// exports). Registered last, so the guest memory watches the runtime resolves never reach it; only logs, never handles.
// So do the exits that raise no exception: abort() (an assert, the runtime's fatal errors, std::terminate, all of them
// 0xC0000409 to the launcher) and std::terminate's exception, which reaches abort with its message.
// Writes with a stack buffer and WriteFile: the process heap may be the thing that broke.
// A lost GPU ends in abort() too ("Graphics device lost": the presenter's path logs nothing), so the entry then carries
// the driver's reason, DRED's breadcrumbs and page fault, and the video memory in use (report 430).

#pragma once

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <exception>

namespace crash_log {

inline wchar_t g_path[MAX_PATH];
inline volatile LONG g_count = 0;
inline ID3D12Device* g_device = nullptr;  // once the runtime made it (PortApp::OnPostSetup)

inline void Write(HANDLE f, const char* s, int n) {
  DWORD written;
  if (n > 0) WriteFile(f, s, static_cast<DWORD>(n), &written, nullptr);
}

inline void Frame(HANDLE f, int i, DWORD64 pc) {
  wchar_t mod[MAX_PATH] = L"?";
  HMODULE h = nullptr;
  GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                     reinterpret_cast<LPCWSTR>(pc), &h);
  if (h) GetModuleFileNameW(h, mod, MAX_PATH);
  const wchar_t* name = std::wcsrchr(mod, L'\\');
  name = name ? name + 1 : mod;
  char line[400];
  Write(f, line, std::snprintf(line, sizeof line, "  %2d %ls+0x%llx\r\n", i, name,
                               static_cast<unsigned long long>(pc - reinterpret_cast<DWORD64>(h))));
}

inline void Stack(HANDLE f, CONTEXT ctx) {
  ULONG_PTR low = 0, high = 0;
  GetCurrentThreadStackLimits(&low, &high);
  for (int i = 0; i < 48 && ctx.Rip; ++i) {
    Frame(f, i, ctx.Rip);
    DWORD64 base = 0;
    if (PRUNTIME_FUNCTION fe = RtlLookupFunctionEntry(ctx.Rip, &base, nullptr)) {
      void* handler_data;
      DWORD64 frame;
      RtlVirtualUnwind(UNW_FLAG_NHANDLER, base, ctx.Rip, fe, &ctx, &handler_data, &frame, nullptr);
    } else {  // a leaf: the return address is on top of the stack
      if (ctx.Rsp < low || ctx.Rsp + 8 > high) break;
      ctx.Rip = *reinterpret_cast<DWORD64*>(ctx.Rsp);
      ctx.Rsp += 8;
    }
    if (ctx.Rsp < low || ctx.Rsp > high) break;
  }
}

// the entry's first line: the time, the thread and what happened
inline HANDLE Begin(const char* what) {
  HANDLE f = CreateFileW(g_path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, 0,
                         nullptr);
  if (f == INVALID_HANDLE_VALUE) return f;
  SYSTEMTIME t;
  GetLocalTime(&t);
  char line[400];
  Write(f, line, std::snprintf(line, sizeof line, "%02d:%02d:%02d.%03d thread %lu %s", t.wHour, t.wMinute, t.wSecond,
                               t.wMilliseconds, GetCurrentThreadId(), what));
  return f;
}

inline LONG CALLBACK Handler(EXCEPTION_POINTERS* ep) {
  DWORD code = ep->ExceptionRecord->ExceptionCode;
  bool fatal = code == EXCEPTION_ACCESS_VIOLATION || code == 0xC0000374 /* heap corruption */ ||
               code == 0xC0000409 /* stack buffer overrun */ || code == EXCEPTION_ILLEGAL_INSTRUCTION ||
               code == EXCEPTION_PRIV_INSTRUCTION || code == EXCEPTION_INT_DIVIDE_BY_ZERO;
  if (!fatal || InterlockedIncrement(&g_count) > 8) return EXCEPTION_CONTINUE_SEARCH;
  char what[64];
  std::snprintf(what, sizeof what, "exception 0x%08lX", code);
  HANDLE f = Begin(what);
  if (f == INVALID_HANDLE_VALUE) return EXCEPTION_CONTINUE_SEARCH;
  char line[400];
  if (code == EXCEPTION_ACCESS_VIOLATION && ep->ExceptionRecord->NumberParameters >= 2) {
    const ULONG_PTR* info = ep->ExceptionRecord->ExceptionInformation;
    Write(f, line, std::snprintf(line, sizeof line, " (%s 0x%llx)", info[0] == 1 ? "write" : info[0] == 8 ? "exec" : "read",
                                 static_cast<unsigned long long>(info[1])));
  }
  Write(f, "\r\n", 2);
  Stack(f, *ep->ContextRecord);
  CloseHandle(f);
  return EXCEPTION_CONTINUE_SEARCH;
}

// the device removed: why, where the GPU stopped (DRED), and the video memory against the budget; nothing while it
// is fine
inline void GpuLost(HANDLE f) {
  if (!g_device) return;
  HRESULT why = g_device->GetDeviceRemovedReason();
  if (SUCCEEDED(why)) return;
  const char* name = why == DXGI_ERROR_DEVICE_HUNG             ? "DEVICE_HUNG (TDR)"
                     : why == DXGI_ERROR_DEVICE_REMOVED        ? "DEVICE_REMOVED"
                     : why == DXGI_ERROR_DEVICE_RESET          ? "DEVICE_RESET"
                     : why == DXGI_ERROR_DRIVER_INTERNAL_ERROR ? "DRIVER_INTERNAL_ERROR"
                     : why == DXGI_ERROR_INVALID_CALL          ? "INVALID_CALL"
                     : why == E_OUTOFMEMORY                    ? "E_OUTOFMEMORY"
                                                               : "?";
  char line[400];
  Write(f, line, std::snprintf(line, sizeof line, "  GPU lost: 0x%08lX %s\r\n", static_cast<unsigned long>(why), name));
  ID3D12DeviceRemovedExtendedData* dred = nullptr;
  if (SUCCEEDED(g_device->QueryInterface(__uuidof(ID3D12DeviceRemovedExtendedData), reinterpret_cast<void**>(&dred)))) {
    D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT crumbs = {};
    int lists = 0;
    if (SUCCEEDED(dred->GetAutoBreadcrumbsOutput(&crumbs)))
      for (const D3D12_AUTO_BREADCRUMB_NODE* n = crumbs.pHeadAutoBreadcrumbNode; n && lists < 8; n = n->pNext) {
        if (!n->pLastBreadcrumbValue || !n->pCommandHistory || *n->pLastBreadcrumbValue >= n->BreadcrumbCount) continue;
        UINT last = *n->pLastBreadcrumbValue;  // the first operation the GPU did not finish
        Write(f, line, std::snprintf(line, sizeof line, "  DRED: list %d stopped at op %u of %u (type %d)\r\n", lists,
                                     last, n->BreadcrumbCount, static_cast<int>(n->pCommandHistory[last])));
        ++lists;
      }
    D3D12_DRED_PAGE_FAULT_OUTPUT fault = {};
    if (SUCCEEDED(dred->GetPageFaultAllocationOutput(&fault)) && fault.PageFaultVA)
      Write(f, line, std::snprintf(line, sizeof line, "  DRED: page fault at GPU VA 0x%llx\r\n",
                                   static_cast<unsigned long long>(fault.PageFaultVA)));
    dred->Release();
  }
  // dxgi.dll is loaded already (the swap chain), asked by name so the ports link nothing new
  using Create = HRESULT(WINAPI*)(REFIID, void**);
  HMODULE dxgi = GetModuleHandleW(L"dxgi.dll");
  auto create = dxgi ? reinterpret_cast<Create>(GetProcAddress(dxgi, "CreateDXGIFactory1")) : nullptr;
  IDXGIFactory4* factory = nullptr;
  if (create && SUCCEEDED(create(__uuidof(IDXGIFactory4), reinterpret_cast<void**>(&factory)))) {
    IDXGIAdapter3* adapter = nullptr;
    if (SUCCEEDED(factory->EnumAdapterByLuid(g_device->GetAdapterLuid(), __uuidof(IDXGIAdapter3),
                                             reinterpret_cast<void**>(&adapter)))) {
      DXGI_QUERY_VIDEO_MEMORY_INFO local = {};
      if (SUCCEEDED(adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &local)))
        Write(f, line, std::snprintf(line, sizeof line, "  VRAM: %llu MB in use, budget %llu MB\r\n",
                                     static_cast<unsigned long long>(local.CurrentUsage >> 20),
                                     static_cast<unsigned long long>(local.Budget >> 20)));
      adapter->Release();
    }
    factory->Release();
  }
}

// abort() raises SIGABRT first, on the thread that called it: the stack still holds the caller (an assert, the
// runtime's fatal error, or std::terminate over the frame that threw)
inline void OnAbort(int) {
  if (InterlockedIncrement(&g_count) > 8) return;
  HANDLE f = Begin("abort()\r\n");
  if (f == INVALID_HANDLE_VALUE) return;
  GpuLost(f);
  CONTEXT ctx;
  RtlCaptureContext(&ctx);
  Stack(f, ctx);
  CloseHandle(f);
}

// std::terminate on the main thread: the exception's message first (the other threads keep the default, which
// aborts, and OnAbort still writes their stack)
inline void OnTerminate() {
  char what[400] = "std::terminate";
  if (std::exception_ptr e = std::current_exception()) {
    try {
      std::rethrow_exception(e);
    } catch (const std::exception& x) {
      std::snprintf(what, sizeof what, "std::terminate: %s", x.what());
    } catch (...) {
      std::snprintf(what, sizeof what, "std::terminate: not a std::exception");
    }
  }
  HANDLE f = Begin(what);
  if (f != INVALID_HANDLE_VALUE) {
    Write(f, "\r\n", 2);
    CloseHandle(f);
  }
  std::abort();
}

// falha.log beside the executable
inline void Install() {
  DWORD n = GetModuleFileNameW(nullptr, g_path, MAX_PATH);
  wchar_t* slash = std::wcsrchr(g_path, L'\\');
  if (!n || !slash || slash - g_path + 10 >= MAX_PATH) return;
  std::wcscpy(slash + 1, L"falha.log");
  AddVectoredExceptionHandler(0, Handler);
  std::signal(SIGABRT, OnAbort);
  std::set_terminate(OnTerminate);
}

}  // namespace crash_log
