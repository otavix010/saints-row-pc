// The port's own picture options and its defaults, set before the GPU starts (OnPreSetup) and only where the player
// left the runtime's option alone (its options file, the environment or the command line win):
//   filtro    scaling to the screen: fsr (AMD FSR 1), cas (AMD CAS) or bilinear; present_effect underneath
//   nitidez   0-100, FSR's or CAS's sharpening: FSR's RCAS takes it as stops of reduction, 2 (the softest) at 0% down
//             to 0 at 100% (70% is about the runtime's own 0.2); CAS adds 0 to 1
//   the internal resolution at 2x (1440p) and 16x anisotropic filtering, instead of the runtime's 1x and 4x
//   alvos     render targets: auto (ROV where the game measured better with it, on NVIDIA), rov or rtv;
//             render_target_path_d3d12 underneath

#pragma once

#include <algorithm>
#include <string>

#include <rex/audio/downmix.h>
#include <rex/cvar.h>
#include <rex/logging.h>

REXCVAR_DEFINE_STRING(filtro, "fsr", "Port", "Scaling to the screen: fsr (AMD FSR 1), cas (AMD CAS) or bilinear");
REXCVAR_DEFINE_INT32(nitidez, 70, "Port", "Sharpening of FSR or CAS, 0-100");
REXCVAR_DEFINE_INT32(volume, 100, "Port", "The game's volume, 0-100");
REXCVAR_DEFINE_INT32(zona_morta_esquerda, 0, "Port", "Left stick deadzone, 0-50 percent");
REXCVAR_DEFINE_INT32(zona_morta_direita, 0, "Port", "Right stick deadzone, 0-50 percent");
REXCVAR_DEFINE_STRING(alvos, "auto", "Port",
                      "Render targets: auto (the path measured best for the game on this GPU), rov or rtv");

namespace port {
uint32_t GpuVendor();  // gpu_vendor.cpp
inline bool g_rov_on_nvidia = false;  // a game's app sets it before setup when ROV measured better for it on NVIDIA
}

namespace port_options {

// false when the option is not registered (yet): a GPU plugin's options register as the plugin loads
inline bool SetUnlessChosen(const char* name, const std::string& value) {
  if (!rex::cvar::GetFlagInfo(name)) {
    REXLOG_WARN("port option {}: not registered, default {} not applied", name, value);
    return false;
  }
  if (rex::cvar::GetFlagSource(name) == rex::cvar::Source::kDefault) rex::cvar::SetFlagByName(name, value);
  return true;
}

// the picture options in force, once set up (PortApp::OnPostSetup)
inline void Log() {
  std::string line;
  for (const char* name : {"fps", "video_mode_refresh_rate", "resolution_scale", "anisotropic_override", "present_effect", "volume",
                           "present_fsr_sharpness_reduction", "swap_post_effect", "readback_resolve", "vsync",
                           "render_target_path_d3d12"})
    line += std::string(name) + "=" + rex::cvar::GetFlagByName(name) + " ";
  REXLOG_INFO("port options: {}", line);
}

inline void Apply() {
  // the render targets: ROV keeps the EDRAM in a buffer, as the console does, and skips the copies between render
  // target formats that the RTV path pays for, more the higher the scale, but its own blending costs more per pixel.
  // Which wins depends on the game: at 2x on an RTX 5080, Asura's Wrath's fights went from 76-99% of the GPU (RTV) to
  // 38-47% (ROV), Saints Row's streets from 42% to 81-90%, the picture the same in both. So ROV only where a game
  // measured better with it (g_rov_on_nvidia) and on NVIDIA; otherwise the runtime's own choice (RTV, ROV on Intel)
  std::string path = REXCVAR_GET(alvos);
  if (path == "auto") path = port::g_rov_on_nvidia && port::GpuVendor() == 0x10DE ? "rov" : "";
  if (path == "rov" || path == "rtv") SetUnlessChosen("render_target_path_d3d12", path);
  SetUnlessChosen("resolution_scale", "2");
  // the texture cache's budget: the runtime's (384 MB, 768 at most, render targets 24) makes a game at 2x drop and load
  // again textures it still uses; Asura's Wrath's fights went from 53.6 to 56.5 fps on average with these, and no
  // second under 55 (RTX 5080). They are limits, not an allocation: a game uses what it needs
  SetUnlessChosen("texture_cache_memory_limit_soft", "2048");
  SetUnlessChosen("texture_cache_memory_limit_hard", "4096");
  SetUnlessChosen("texture_cache_memory_limit_render_to_texture", "256");
  SetUnlessChosen("anisotropic_override", "5");
  SetUnlessChosen("present_effect", REXCVAR_GET(filtro));
  double n = std::clamp(REXCVAR_GET(nitidez), 0, 100) / 100.0;
  SetUnlessChosen("present_fsr_sharpness_reduction", std::to_string(2.0 * (1.0 - n) * (1.0 - n)));
  SetUnlessChosen("present_cas_additional_sharpness", std::to_string(n * n));
  // the controls and the sound, in the launcher's units (percent)
  SetUnlessChosen("left_stick_deadzone_percentage",
                  std::to_string(std::clamp(REXCVAR_GET(zona_morta_esquerda), 0, 50) / 100.0));
  SetUnlessChosen("right_stick_deadzone_percentage",
                  std::to_string(std::clamp(REXCVAR_GET(zona_morta_direita), 0, 50) / 100.0));
  // the volume as the output's gain, on a curve the ear hears as even (50% is about a quarter of the power)
  double v = std::clamp(REXCVAR_GET(volume), 0, 100) / 100.0;
  rex::audio::SetOutputGain(static_cast<float>(v * v));
}

}  // namespace port_options
