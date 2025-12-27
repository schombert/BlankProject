#pragma once
#include <functional>

#include "game_scene_default.hpp"

namespace game_scene {

void on_rbutton_down(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mod);
void on_lbutton_down(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mod);
void on_lbutton_up(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mod);

void switch_scene(sys::state& state, scene_id ui_scene);

void start_dragging(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mod);
void do_nothing_screen(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mod);

sys::virtual_key replace_keycodes_identity(sys::state& state, sys::virtual_key keycode, sys::key_modifiers mod);

void do_nothing_hotkeys(sys::state& state, sys::virtual_key keycode, sys::key_modifiers mod);
void in_game_hotkeys(sys::state& state, sys::virtual_key keycode, sys::key_modifiers mod);

void on_key_down(sys::state& state, sys::virtual_key keycode, sys::key_modifiers mod);

}
