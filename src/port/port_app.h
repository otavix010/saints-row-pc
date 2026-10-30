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
//     (input_lock.h), falha.log on a crash (crash_log.h), a log line repeated thousands of times written once in
//     10000 (log_repeat.h);
//   - development: PORT_PAD scripts a controller (pad_script.h), PORT_DUMP_IMAGE=file writes the loaded image for
//     scripts/varrer_ponteiros.py, PORT_CAPTURA=file.bmp the game's picture a few times a second (captura.h).
// A game's app derives from PortApp and adds its own hooks (its frame rate, its fixes).

#pragma once

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/filesystem/devices/disc_image_device.h>
#include <rex/filesystem/devices/host_path_device.h>
#include <rex/filesystem/vfs.h>
#include <rex/input/device_assignment.h>
#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/rex_app.h>
#include <rex/system/gpu_plugin.h>
#include <rex/system/xmemory.h>
#include <rex/ui/window.h>

#include "audio_guard.h"
#include "cache_drive.h"
#include "captura.h"
#include "crash_log.h"
#include "fps_overlay.h"
#include "frame_rate.h"
#include "input_lock.h"
#include "log_repeat.h"
#include "pad_script.h"
#include "port_options.h"
#include "window_icon.h"

REXCVAR_DEFINE_STRING(iso, "", "Port",
                      "The game's disc image (.iso), read in place; game_data_root then needs only default.xex");

namespace port {

inline std::filesystem::path FromUtf8(const char* s) {
  return std::filesystem::path(reinterpret_cast<const char8_t*>(s));
}

inline std::string ToUtf8(const std::filesystem::path& p) {
  std::u8string u = p.u8string();
  return std::string(u.begin(), u.end());
}

}  // namespace port

class PortApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  void OnConfigurePaths(rex::PathConfig& paths) override {
    auto exe_dir = rex::filesystem::GetExecutableFolder();
    if (const char* options = std::getenv("PORT_OPCOES")) paths.config_path = port::FromUtf8(options);
    if (paths.game_data_root.empty()) paths.game_data_root = exe_dir / "assets";
    // an empty --iso from the launcher took "--user_data_root" for the disc and left its folder out (report 326): the
    // launcher's saves folder at the port's root, three folders above the build
    const std::string& iso = REXCVAR_GET(iso);
    std::error_code ec;
    if (iso.rfind("--", 0) == 0 && paths.user_data_root == rex::filesystem::GetUserFolder() / GetName() &&
        std::filesystem::is_directory(exe_dir / ".." / ".." / ".." / "saves", ec)) {
      paths.user_data_root = std::filesystem::weakly_canonical(exe_dir / ".." / ".." / ".." / "saves", ec);
      paths.cache_root = paths.user_data_root.parent_path() / "cache";
    } else if (paths.user_data_root == rex::filesystem::GetUserFolder() / GetName()) {
      paths.user_data_root = exe_dir / "usuario";
      paths.cache_root = paths.user_data_root / "cache";
    }
    port::g_user_data_root = paths.user_data_root;
  }

  // the same log line thousands of times written once in 10000 (log_repeat.h)
  void OnPostInitLogging() override { port::log_repeat::Install(); }

  void OnPreSetup(rex::RuntimeConfig& config) override {
    if (config.gpu_plugin.empty() && !config.graphics) config.gpu_plugin = "xenos";
    // the GPU plugin loaded here, not after this hook as the runtime would: its options (resolution_scale,
    // anisotropic_override, readback_resolve...) register when it loads, and the port's defaults below can only be set
    // on registered options (the options file, the environment and the command line still win: they are applied as the
    // plugin registers them). Should it fail, the runtime tries again and reports it.
    if (!config.graphics && !config.gpu_plugin.empty()) config.graphics = rex::system::LoadGpuPlugin(config.gpu_plugin);
    port::log_repeat::Install();  // the plugin's own category, registered as it loaded
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

  void OnPostSetup() override {
    port_options::Log();
    if (const char* file = std::getenv("PORT_CAPTURA"); file && runtime()->graphics_system()) {
      const char* ms = std::getenv("PORT_CAPTURA_MS");
      captura_ = std::make_unique<captura::Captura>(runtime()->graphics_system(), port::FromUtf8(file).wstring(), ms ? std::atoi(ms) : 150);
    }
  }

  void OnShutdown() override { captura_.reset(); }

  // the game's own icon on the window and the taskbar
  void OnPostLaunchModule(rex::system::XThread*) override {
    port::SetWindowIconFromTitle(runtime()->kernel_state(), app_context());
  }

  // what the generator made the game from (out/gerar/iso.txt, two folders above the build): the disc image or the
  // unpacked game's folder
  static std::string GeneratorIso() {
    std::ifstream f(rex::filesystem::GetExecutableFolder() / ".." / ".." / "gerar" / "iso.txt", std::ios::binary);
    std::string line;
    std::getline(f, line);
    if (line.rfind("\xEF\xBB\xBF", 0) == 0) line.erase(0, 3);  // PowerShell's UTF-8 mark
    while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
    return line;
  }

  // the disc: the iso option, else the generator's, for a game the launcher keeps no disc for (report 326: made by
  // the generator alone, the launcher passed an empty --iso and the next argument was taken for it)
  static std::string IsoPath() {
    std::string iso = REXCVAR_GET(iso);
    // a folder ending with its separator, quoted without doubling it ("F:\"): the quote came in with the path
    while (!iso.empty() && iso.back() == '"') iso.pop_back();
    if (!iso.empty() && iso.rfind("--", 0) != 0) return iso;
    std::string line = GeneratorIso();
    if (!line.empty()) REXLOG_INFO("iso: none given ('{}'); the generator's: {}", iso, line);
    return line;
  }

  // the game unpacked from its disc (report 214): the folder with default.xex at the top, the one given or one of the
  // two above it (a folder of the disc picked, as its $SystemUpdate), else the one the generator found; empty if none
  static std::filesystem::path UnpackedGame(const std::filesystem::path& folder) {
    std::error_code ec;
    std::filesystem::path at = folder;
    for (int i = 0; i <= 2 && !at.empty(); ++i, at = at.parent_path()) {
      if (std::filesystem::is_regular_file(at / "default.xex", ec)) return at;
      if (at == at.parent_path()) break;
    }
    std::filesystem::path made = port::FromUtf8(GeneratorIso().c_str());
    if (!made.empty() && std::filesystem::is_regular_file(made / "default.xex", ec)) return made;
    return {};
  }

  void OnLoadXexImage(std::string& xex_image) override {
    (void)xex_image;
    const std::string iso = IsoPath();
    if (iso.empty()) return;
    constexpr const char* kMount = "\\Device\\Cdrom0";
    auto* fs = runtime()->file_system();
    std::error_code ec;
    std::filesystem::path path = port::FromUtf8(iso.c_str());
    std::unique_ptr<rex::filesystem::Device> disc;
    if (std::filesystem::is_directory(path, ec)) {
      path = UnpackedGame(path);
      if (path.empty()) {
        REXLOG_ERROR("iso: {} is a folder without the game (default.xex); playing from the game folder", iso);
        return;
      }
      disc = std::make_unique<rex::filesystem::HostPathDevice>(kMount, path, true);
    } else {
      disc = std::make_unique<rex::filesystem::DiscImageDevice>(kMount, path);
    }
    if (!disc->Initialize() || !fs->RegisterDevice(std::move(disc))) {
      REXLOG_ERROR("iso: could not read {}; playing from the game folder", iso);
      return;
    }
    for (const char* link : {"game:", "d:"}) {
      fs->UnregisterSymbolicLink(link);
      fs->RegisterSymbolicLink(link, kMount);
    }
    REXLOG_INFO("iso: game: and d: read {}", port::ToUtf8(path));
  }

  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {
    // the counter and the meter, hidden until F10 unless an option asks for it at the start (reports 307, 327: with
    // both options off, F10 did nothing); lives as long as the drawer
    new FpsOverlay(drawer);
    // the mouse pointer hides after a moment still over the game, and comes back when it moves (reports 307, 327:
    // the runtime only hides it while the mouse plays the game, so it stayed over the picture)
    if (window()) window()->SetCursorVisibility(rex::ui::Window::CursorVisibility::kAutoHidden);
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

 private:
  std::unique_ptr<captura::Captura> captura_;
};
