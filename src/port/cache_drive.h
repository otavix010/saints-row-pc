// The utility drive (cache:), the Xbox 360's scratch partition, as a folder beside the saves (cache_drive). The XAPI
// linked into a game mounts it with XMountUtilityDrive: it checks the raw cache partition and formats it when the
// header is wrong, then links cache: to it. The runtime answers the raw partitions with a null device, so the check
// never passes and the game goes on without the drive (DiRT 2 then cannot keep its replays and flashbacks). A game
// that needs it hooks its own XMountUtilityDrive with MountCacheDrive and reports success.

#pragma once

#include <filesystem>
#include <memory>
#include <mutex>

#include <rex/filesystem/devices/host_path_device.h>
#include <rex/filesystem/vfs.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>

namespace port {

inline std::filesystem::path g_user_data_root;  // set by PortApp::OnConfigurePaths

inline bool MountCacheDrive() {
  static std::once_flag once;
  static bool mounted = false;
  std::call_once(once, [] {
    auto dir = g_user_data_root / "cache_drive";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    auto device = std::make_unique<rex::filesystem::HostPathDevice>("\\CACHE", dir, false);
    if (device->Initialize() && REX_KERNEL_FS()->RegisterDevice(std::move(device))) {
      REX_KERNEL_FS()->RegisterSymbolicLink("cache:", "\\CACHE");
      mounted = true;
      REXLOG_INFO("cache: is {}", dir.string());
    } else {
      REXLOG_ERROR("cache: could not use {}", dir.string());
    }
  });
  return mounted;
}

}  // namespace port
