// The PCI vendor of the GPU the runtime will take: like its D3D12 provider (with d3d12_adapter left alone), the first
// hardware adapter DXGI lists. 0x10DE NVIDIA, 0x1002 AMD, 0x8086 Intel; 0 when there is none.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <dxgi1_4.h>

#include <cstdint>

namespace port {

uint32_t GpuVendor() {
  IDXGIFactory1* factory = nullptr;
  if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return 0;
  uint32_t vendor = 0;
  IDXGIAdapter1* adapter = nullptr;
  for (UINT i = 0; factory->EnumAdapters1(i, &adapter) == S_OK; ++i) {
    DXGI_ADAPTER_DESC1 desc = {};
    bool hardware = SUCCEEDED(adapter->GetDesc1(&desc)) && !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE);
    adapter->Release();
    if (hardware) {
      vendor = desc.VendorId;
      break;
    }
  }
  factory->Release();
  return vendor;
}

}  // namespace port
