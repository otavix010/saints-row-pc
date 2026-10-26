// Development: PORT_CAPTURA=file.bmp writes the game's picture (the guest output, before it is scaled to the window)
// there every PORT_CAPTURA_MS milliseconds (150), at most 1280 wide, the file replaced whole each time. It is the test
// scripts' eyes when the window sits off every screen, where the window itself is never drawn (nor can PrintWindow
// see it), so a test never has to put a window where the person at the PC is working. Never takes the game down:
// a picture of an odd size is skipped and nothing thrown leaves the thread (a std::bad_alloc here once ended a run).

#pragma once

#include <windows.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#include <rex/system/interfaces/graphics.h>
#include <rex/ui/presenter.h>

namespace captura {

class Captura {
 public:
  Captura(rex::system::IGraphicsSystem* graphics, std::wstring path, int ms)
      : graphics_(graphics), path_(std::move(path)), ms_(ms > 20 ? ms : 20), thread_([this] { Run(); }) {}
  ~Captura() {
    stop_ = true;
    thread_.join();
  }

 private:
  void Run() {
    rex::ui::RawImage image;
    while (!stop_) {
      try {
        if (rex::ui::Presenter* presenter = graphics_->presenter(); presenter && presenter->CaptureGuestOutput(image))
          Write(image);
      } catch (...) {
        out_ = {};
        image = {};
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(ms_));
    }
  }

  // a 24-bit BMP, top-down, every f-th pixel so it fits in 1280
  void Write(const rex::ui::RawImage& image) {
    if (!image.width || !image.height || image.width > 8192 || image.height > 8192 ||
        image.data.size() < image.stride * (image.height - 1) + image.width * 4)
      return;
    uint32_t f = (image.width + 1279) / 1280;
    uint32_t w = image.width / f, h = image.height / f, row = (w * 3 + 3) & ~3u;
    std::vector<uint8_t>& out = out_;
    out.assign(54 + size_t(row) * h, 0);
    auto put32 = [&](size_t at, uint32_t v) {
      for (int i = 0; i < 4; ++i) out[at + i] = uint8_t(v >> (8 * i));
    };
    out[0] = 'B';
    out[1] = 'M';
    put32(2, uint32_t(out.size()));
    put32(10, 54);
    put32(14, 40);
    put32(18, w);
    put32(22, uint32_t(-int32_t(h)));
    out[26] = 1;
    out[28] = 24;
    put32(34, uint32_t(size_t(row) * h));
    for (uint32_t y = 0; y < h; ++y) {
      const uint8_t* src = image.data.data() + size_t(y) * f * image.stride;
      uint8_t* dst = out.data() + 54 + size_t(y) * row;
      for (uint32_t x = 0; x < w; ++x, src += 4 * f, dst += 3) {
        dst[0] = src[2];
        dst[1] = src[1];
        dst[2] = src[0];
      }
    }
    std::wstring tmp = path_ + L".tmp";
    HANDLE file = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    BOOL ok = WriteFile(file, out.data(), DWORD(out.size()), &written, nullptr);
    CloseHandle(file);
    if (ok) MoveFileExW(tmp.c_str(), path_.c_str(), MOVEFILE_REPLACE_EXISTING);
  }

  rex::system::IGraphicsSystem* graphics_;
  std::wstring path_;
  int ms_;
  std::vector<uint8_t> out_;
  std::atomic<bool> stop_{false};
  std::thread thread_;
};

}  // namespace captura
