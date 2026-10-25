// The port's own picture options and its defaults, set before the GPU starts (OnPreSetup) and only where the player
// left the runtime's option alone (its options file, the environment or the command line win):
//   filtro    scaling to the screen: fsr (AMD FSR 1), cas (AMD CAS) or bilinear; present_effect underneath
//   nitidez   0-100, FSR's or CAS's sharpening: FSR's RCAS takes it as stops of reduction, 2 (the softest) at 0% down
//             to 0 at 100% (70% is about the runtime's own 0.2); CAS adds 0 to 1
//   the internal resolution at 2x (1440p) and 16x anisotropic filtering, instead of the runtime's 1x and 4x

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
                           "present_fsr_sharpness_reduction", "swap_post_effect", "readback_resolve", "vsync"})
    line += std::string(name) + "=" + rex::cvar::GetFlagByName(name) + " ";
  REXLOG_INFO("port options: {}", line);
}

inline void Apply() {
  SetUnlessChosen("resolution_scale", "2");
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
