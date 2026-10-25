// The window's and the taskbar's icon: the game's own, from the player's executable (its XDBF title data holds a 64x64
// PNG; the four ports' all do), set once the module is loaded (PortApp::OnPostLaunchModule: before that the title data
// is still empty), on the UI thread, which owns the windows.

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <SDL3/SDL.h>

#include <rex/system/kernel_state.h>
#include <rex/ui/image_decode.h>
#include <rex/ui/windowed_app_context.h>

namespace port {

inline void SetWindowIconFromTitle(rex::system::KernelState* kernel, rex::ui::WindowedAppContext& ui) {
  if (!kernel) return;
  auto icon = kernel->title_xdbf().icon();
  if (!icon) return;
  int w = 0, h = 0;
  auto pixels = std::make_shared<std::vector<uint8_t>>(rex::ui::DecodeImageRGBA(icon.buffer, icon.size, w, h));
  if (pixels->empty()) return;
  ui.CallInUIThread([pixels, w, h] {
    SDL_Surface* surface = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, pixels->data(), w * 4);
    if (!surface) return;
    int count = 0;
    SDL_Window** windows = SDL_GetWindows(&count);
    for (int i = 0; i < count; ++i) SDL_SetWindowIcon(windows[i], surface);
    SDL_free(windows);
    SDL_DestroySurface(surface);  // SDL keeps its own copy
  });
}

}  // namespace port
