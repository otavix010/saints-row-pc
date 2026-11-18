// The game's own keys for keyboard and mouse (port/port_app.h applies them to the engine's keybind_* options,
// where the player left them alone). The same defaults as the game's profile in the Otavix Launcher.
// Written by teclas_360.py: the table of the four ports is there.

#pragma once

inline constexpr const char* kPortKeys =
    "lstick_up=w;lstick_down=s;lstick_left=a;lstick_right=d;a=lshift enter;b=r backspace;x=space;y=e;left_shoulder=q;right_shoulder=f;left_trigger=mouse2;right_trigger=mouse1;start=escape;back=tab;dpad_up=up;dpad_down=down;dpad_left=left;dpad_right=right;rstick_up=num8;rstick_down=num2;rstick_left=num4;rstick_right=num6;lstick_press=lctrl;rstick_press=mouse3 c";
inline constexpr bool kPortMouse = true;
inline constexpr int kPortMouseSensitivity = 100;  // percent
