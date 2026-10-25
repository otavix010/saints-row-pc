// The base of the Xbox 360 ports' apps (ReXGlue), the same in every port (x360-ports/comum, copied by sincronizar.py):
//   - paths: the game in assets beside the executable unless game_data_root says otherwise, saves and caches in
//     usuario; the options in PORT_OPCOES when given (the launcher keeps them at the port's root, which a new build
//     leaves alone);
//   - iso: game: and d: read the disc image in place, so the game folder needs only default.xex; the utility
//     drive (cache:) as a folder for the games that hook their XMountUtilityDrive (cache_drive.h);
//   - the Xenos GPU; the system's prompts (create the save, pick the storage) answered with their default button
//     unless headless says otherwise, since they only take keyboard and mouse;
//   - the frame rate (frame_rate.h), the port's picture options (port_options.h), the counter (fps_overlay.h), the audio
//     guard (audio_guard.h), the input lock
//     (input_lock.h), falha.log on a crash (crash_log.h);
//   - development: PORT_PAD scripts a controller (pad_script.h), PORT_DUMP_IMAGE=file writes the loaded image for
//     scripts/varrer_ponteiros.py.
// A game's app derives from PortApp and adds its own hooks (its frame rate, its fixes).

#pragma once

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/filesystem/devices/disc_image_device.h>
#include <rex/filesystem/vfs.h>
#include <rex/input/device_assignment.h>
#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/rex_app.h>
#include <rex/system/xmemory.h>

#include "audio_guard.h"
#include "cache_drive.h"
#include "crash_log.h"
#include "fps_overlay.h"
#include "frame_rate.h"
#include "input_lock.h"
#include "pad_script.h"
#include "port_options.h"

REXCVAR_DEFINE_STRING(iso, "", "Port",
                      "The game's disc image (.iso), read in place; game_data_root then needs only default.xex");

namespace port {

inline std::filesystem::path FromUtf8(const char* s) {
  return std::filesystem::path(reinterpret_cast<const char8_t*>(s));
}

}  // namespace port

class PortApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  void OnConfigurePaths(rex::PathConfig& paths) override {
    auto exe_dir = rex::filesystem::GetExecutableFolder();
    if (const char* options = std::getenv("PORT_OPCOES")) paths.config_path = port::FromUtf8(options);
    if (paths.game_data_root.empty()) paths.game_data_root = exe_dir / "assets";
    if (paths.user_data_root == rex::filesystem::GetUserFolder() / GetName()) {
      paths.user_data_root = exe_dir / "usuario";
      paths.cache_root = paths.user_data_root / "cache";
    }
    port::g_user_data_root = paths.user_data_root;
  }

  void OnPreSetup(rex::RuntimeConfig& config) override {
    if (config.gpu_plugin.empty() && !config.graphics) config.gpu_plugin = "xenos";
    if (rex::cvar::GetFlagSource("headless") == rex::cvar::Source::kDefault)
      rex::cvar::SetFlagByName("headless", "true");
    ApplyFrameRate();
    port_options::Apply();
    if (const char* script = std::getenv("PORT_PAD")) {
      config.input_factory = [script = std::string(script)](bool tool_mode)
          -> std::unique_ptr<rex::system::IInputSystem> {
        // the script alone: it is player one, the profile that is signed in (the default inputs' stand-in pad
        // would take player one and leave the script a player with no profile)
        (void)tool_mode;
        auto input = std::make_unique<rex::input::InputSystem>(nullptr);
        input->AddDriver(std::make_unique<ScriptedPad>(script, std::getenv("PORT_PAD_LIVE")));
        input->SetDeviceAssignment(std::make_unique<rex::input::SlotAssignment>());
        return input;
      };
    }
  }

  void OnLoadXexImage(std::string& xex_image) override {
    (void)xex_image;
    const std::string& iso = REXCVAR_GET(iso);
    if (iso.empty()) return;
    constexpr const char* kMount = "\\Device\\Cdrom0";
    auto* fs = runtime()->file_system();
    auto disc = std::make_unique<rex::filesystem::DiscImageDevice>(kMount, port::FromUtf8(iso.c_str()));
    if (!disc->Initialize() || !fs->RegisterDevice(std::move(disc))) {
      REXLOG_ERROR("iso: could not read {}; playing from the game folder", iso);
      return;
    }
    for (const char* link : {"game:", "d:"}) {
      fs->UnregisterSymbolicLink(link);
      fs->RegisterSymbolicLink(link, kMount);
    }
    REXLOG_INFO("iso: game: and d: read {}", iso);
  }

  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {
    if (REXCVAR_GET(mostrar_fps)) new FpsOverlay(drawer);  // lives as long as the drawer
  }

  void OnPostLoadXexImage() override {
    const char* out = std::getenv("PORT_DUMP_IMAGE");
    if (!out) return;
    const auto* image = runtime()->memory()->TranslateVirtual<const uint8_t*>(PPCImageConfig.image_base);
    if (FILE* f = std::fopen(out, "wb")) {
      std::fwrite(image, 1, PPCImageConfig.image_size, f);
      std::fclose(f);
    }
  }
};
