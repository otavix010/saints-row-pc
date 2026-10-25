// Development only (port.cmake, PORT_D3D12_AGILITY_DIR): Direct3D 12 from the D3D12 folder beside the executable,
// the Agility SDK's, with its validation layer.

#include <windows.h>

extern "C" {
__declspec(dllexport) extern const UINT D3D12SDKVersion = 619;
__declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}
