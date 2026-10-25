// Fatal exceptions the runtime's own handler leaves alone go to falha.log beside the executable: the code, the
// address and the stack as module+offset (scripts/pilha.py names the frames from the linker map and the runtime's
// exports). Registered last, so the guest memory watches the runtime resolves never reach it; only logs, never handles.
// Writes with a stack buffer and WriteFile: the process heap may be the thing that broke.

#pragma once

#include <windows.h>

#include <cstdio>
#include <cwchar>

namespace crash_log {

inline wchar_t g_path[MAX_PATH];
inline volatile LONG g_count = 0;

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

inline LONG CALLBACK Handler(EXCEPTION_POINTERS* ep) {
  DWORD code = ep->ExceptionRecord->ExceptionCode;
  bool fatal = code == EXCEPTION_ACCESS_VIOLATION || code == 0xC0000374 /* heap corruption */ ||
               code == 0xC0000409 /* stack buffer overrun */ || code == EXCEPTION_ILLEGAL_INSTRUCTION ||
               code == EXCEPTION_PRIV_INSTRUCTION || code == EXCEPTION_INT_DIVIDE_BY_ZERO;
  if (!fatal || InterlockedIncrement(&g_count) > 8) return EXCEPTION_CONTINUE_SEARCH;
  HANDLE f = CreateFileW(g_path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, 0,
                         nullptr);
  if (f == INVALID_HANDLE_VALUE) return EXCEPTION_CONTINUE_SEARCH;
  SYSTEMTIME t;
  GetLocalTime(&t);
  char line[400];
  Write(f, line, std::snprintf(line, sizeof line, "%02d:%02d:%02d.%03d thread %lu exception 0x%08lX", t.wHour,
                               t.wMinute, t.wSecond, t.wMilliseconds, GetCurrentThreadId(), code));
  if (code == EXCEPTION_ACCESS_VIOLATION && ep->ExceptionRecord->NumberParameters >= 2) {
    const ULONG_PTR* info = ep->ExceptionRecord->ExceptionInformation;
    Write(f, line, std::snprintf(line, sizeof line, " (%s 0x%llx)", info[0] == 1 ? "write" : info[0] == 8 ? "exec" : "read",
                                 static_cast<unsigned long long>(info[1])));
  }
  Write(f, "\r\n", 2);
  ULONG_PTR low = 0, high = 0;
  GetCurrentThreadStackLimits(&low, &high);
  CONTEXT ctx = *ep->ContextRecord;
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
  CloseHandle(f);
  return EXCEPTION_CONTINUE_SEARCH;
}

// falha.log beside the executable
inline void Install() {
  DWORD n = GetModuleFileNameW(nullptr, g_path, MAX_PATH);
  wchar_t* slash = std::wcsrchr(g_path, L'\\');
  if (!n || !slash || slash - g_path + 10 >= MAX_PATH) return;
  std::wcscpy(slash + 1, L"falha.log");
  AddVectoredExceptionHandler(0, Handler);
}

}  // namespace crash_log
