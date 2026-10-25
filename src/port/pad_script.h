// Development: PORT_PAD scripts a controller for tests without a person at the pad (as the FIFA port's FS2_PAD).
//   PORT_PAD="12:START,14.5:A,20-22:LX-,40-300@3:A"
//   at a second: a 0.15 s press; over a range: held; over a range @n: pressed every n seconds (@n/h: held h seconds).
// Buttons: A B X Y LB RB LT RT START BACK UP DOWN LEFT RIGHT LS RS; sticks at full tilt: LX+ LX- LY+ LY- RX+ RX- RY+ RY-
// (Y+ is up); LROT RROT turn a stick round clockwise, twice a second, LROT- RROT- the other way; several at once
// joined by +, as LY-+RY- (both sticks down). Seconds count from the app's start.
// PORT_PAD_LIVE=file takes commands while the game runs: each line appended, "<seconds> <buttons>", holds them from then
// (scripts/tecla.py writes them).

#pragma once

#include <chrono>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <rex/input/input.h>
#include <rex/input/input_driver.h>

using rex::X_RESULT;
using rex::X_STATUS;

class ScriptedPad final : public rex::input::InputDriver {
 public:
  explicit ScriptedPad(const std::string& script, const char* live = nullptr)
      : InputDriver(nullptr, 0), start_(Clock::now()), live_(live ? live : "") {
    Parse(script);
  }

  X_STATUS Setup() override { return X_STATUS_SUCCESS; }

  void EnumerateDevices(std::vector<rex::input::DeviceInfo>& out) override {
    rex::input::DeviceInfo info;
    info.id = kDevice;
    info.name = "Roteiro (PORT_PAD)";
    out.push_back(info);
  }

  X_RESULT GetDeviceState(rex::input::DeviceId id, rex::input::X_INPUT_STATE* out) override {
    if (id != kDevice) return X_ERROR_DEVICE_NOT_CONNECTED;
    double t = std::chrono::duration<double>(Clock::now() - start_).count();
    ReadLive(t);
    Pad pad;
    for (const Press& p : presses_) {
      bool on = t >= p.from && t < p.to;
      if (on && p.every > 0) on = std::fmod(t - p.from, p.every) < p.hold;
      if (on) Apply(p.button, pad, t);
    }
    if (std::memcmp(&pad, &last_, sizeof pad) != 0) {
      packet_++;  // a new packet number for every change, as a real pad
      last_ = pad;
    }
    std::memset(out, 0, sizeof(*out));
    out->packet_number = packet_;
    out->gamepad.buttons = pad.buttons;
    out->gamepad.left_trigger = pad.lt;
    out->gamepad.right_trigger = pad.rt;
    out->gamepad.thumb_lx = pad.lx;
    out->gamepad.thumb_ly = pad.ly;
    out->gamepad.thumb_rx = pad.rx;
    out->gamepad.thumb_ry = pad.ry;
    return X_ERROR_SUCCESS;
  }

  X_RESULT GetDeviceCapabilities(rex::input::DeviceId id, uint32_t,
                                 rex::input::X_INPUT_CAPABILITIES* caps) override {
    if (id != kDevice) return X_ERROR_DEVICE_NOT_CONNECTED;
    if (caps) {
      std::memset(caps, 0, sizeof(*caps));
      caps->type = 0x01;      // gamepad
      caps->sub_type = 0x01;  // gamepad
      caps->gamepad.buttons = 0xFFFF;
      caps->gamepad.left_trigger = 0xFF;
      caps->gamepad.right_trigger = 0xFF;
      caps->gamepad.thumb_lx = static_cast<int16_t>(0x7FFF);
      caps->gamepad.thumb_ly = static_cast<int16_t>(0x7FFF);
      caps->gamepad.thumb_rx = static_cast<int16_t>(0x7FFF);
      caps->gamepad.thumb_ry = static_cast<int16_t>(0x7FFF);
    }
    return X_ERROR_SUCCESS;
  }

  X_RESULT SetDeviceVibration(rex::input::DeviceId id, rex::input::X_INPUT_VIBRATION*) override {
    return id == kDevice ? X_ERROR_SUCCESS : X_ERROR_DEVICE_NOT_CONNECTED;
  }

  X_RESULT GetDeviceKeystroke(rex::input::DeviceId id, uint32_t, rex::input::X_INPUT_KEYSTROKE*) override {
    return id == kDevice ? X_ERROR_EMPTY : X_ERROR_DEVICE_NOT_CONNECTED;
  }

 private:
  using Clock = std::chrono::steady_clock;
  static constexpr rex::input::DeviceId kDevice = static_cast<rex::input::DeviceId>(0x50414453);  // "PADS"

  struct Press {
    double from, to, every;
    std::string button;
    double hold = 0.15;
  };
  struct Pad {
    uint16_t buttons = 0;
    uint8_t lt = 0, rt = 0;
    int16_t lx = 0, ly = 0, rx = 0, ry = 0;
  };

  void Parse(const std::string& all) {
    for (size_t pos = 0; pos < all.size();) {
      size_t comma = all.find(',', pos);
      std::string item = all.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
      pos = comma == std::string::npos ? all.size() : comma + 1;
      size_t colon = item.find(':');
      if (colon == std::string::npos) continue;
      std::string when = item.substr(0, colon);
      Press p{0, 0, 0, item.substr(colon + 1)};
      size_t at = when.find('@');
      if (at != std::string::npos) {
        p.every = std::atof(when.c_str() + at + 1);
        size_t slash = when.find('/', at);
        if (slash != std::string::npos) p.hold = std::atof(when.c_str() + slash + 1);
        when = when.substr(0, at);
      }
      size_t dash = when.find('-');
      p.from = std::atof(when.substr(0, dash).c_str());
      p.to = dash == std::string::npos ? p.from + 0.15 : std::atof(when.substr(dash + 1).c_str());
      presses_.push_back(p);
    }
  }

  // the lines appended to the live file since the last look, held from now
  void ReadLive(double t) {
    if (live_.empty() || t - live_checked_ < 0.1) return;
    live_checked_ = t;
    FILE* f = std::fopen(live_.c_str(), "rb");
    if (!f) return;
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    if (size < live_read_) live_read_ = 0;  // the file was started over
    std::string text(size_t(size - live_read_), '\0');
    std::fseek(f, live_read_, SEEK_SET);
    size_t got = std::fread(text.data(), 1, text.size(), f);
    std::fclose(f);
    size_t end = text.rfind('\n', got);
    if (end == std::string::npos) return;  // no whole line yet
    live_read_ += long(end + 1);
    for (size_t pos = 0; pos <= end;) {
      size_t nl = text.find('\n', pos);
      std::string line = text.substr(pos, nl - pos);
      pos = nl + 1;
      double seconds = 0;
      char buttons[128] = {};
      if (std::sscanf(line.c_str(), "%lf %127s", &seconds, buttons) == 2)
        presses_.push_back({t, t + seconds, 0, buttons});
    }
  }

  static void Apply(const std::string& all, Pad& pad, double t) {
    for (size_t pos = 0; pos < all.size();) {
      // + joins buttons, and is also the sign of a stick: LX++LY- is LX+ and LY-
      size_t plus = all.find('+', pos);
      if (plus != std::string::npos && plus > pos && (all[plus - 1] == 'X' || all[plus - 1] == 'Y') &&
          (all[plus - 2] == 'L' || all[plus - 2] == 'R') && plus - pos == 2)
        plus = all.find('+', plus + 1);
      std::string b = all.substr(pos, plus == std::string::npos ? std::string::npos : plus - pos);
      pos = plus == std::string::npos ? all.size() : plus + 1;
      ApplyOne(b, pad, t);
    }
  }

  static void ApplyOne(const std::string& b, Pad& pad, double t) {
    // a stick going round twice a second, starting up: clockwise is up, right, down, left
    if (b == "LROT" || b == "LROT-" || b == "RROT" || b == "RROT-") {
      double a = t * 2 * 3.14159265358979 * 2;
      int16_t x = static_cast<int16_t>(std::sin(a) * 32767 * (b.back() == '-' ? -1 : 1));
      int16_t y = static_cast<int16_t>(std::cos(a) * 32767);
      (b[0] == 'L' ? pad.lx : pad.rx) = x;
      (b[0] == 'L' ? pad.ly : pad.ry) = y;
      return;
    }
    static const struct {
      const char* name;
      uint16_t bit;
    } kButtons[] = {{"UP", 0x0001},    {"DOWN", 0x0002}, {"LEFT", 0x0004}, {"RIGHT", 0x0008},
                    {"START", 0x0010}, {"BACK", 0x0020}, {"LS", 0x0040},   {"RS", 0x0080},
                    {"LB", 0x0100},    {"RB", 0x0200},   {"A", 0x1000},    {"B", 0x2000},
                    {"X", 0x4000},     {"Y", 0x8000}};
    for (const auto& k : kButtons)
      if (b == k.name) pad.buttons |= k.bit;
    if (b == "LT") pad.lt = 0xFF;
    if (b == "RT") pad.rt = 0xFF;
    if (b == "LX+") pad.lx = 32767;
    if (b == "LX-") pad.lx = -32768;
    if (b == "LY+") pad.ly = 32767;
    if (b == "LY-") pad.ly = -32768;
    if (b == "RX+") pad.rx = 32767;
    if (b == "RX-") pad.rx = -32768;
    if (b == "RY+") pad.ry = 32767;
    if (b == "RY-") pad.ry = -32768;
  }

  Clock::time_point start_;
  std::string live_;
  double live_checked_ = -1;
  long live_read_ = 0;
  std::vector<Press> presses_;
  Pad last_;
  uint32_t packet_ = 0;
};
