#pragma once

#include <chrono>
#include "graphics\opengl_wrapper_containers.hpp"

namespace ui {

struct state {
	element_base* under_mouse = nullptr;
	element_base* left_mouse_hold_target = nullptr;
	
	std::chrono::time_point<std::chrono::steady_clock> last_render_time{};
	std::chrono::microseconds time_since_last_render{ };

	element_base* scroll_target = nullptr;
	element_base* drag_target = nullptr;
	element_base* edit_target_internal = nullptr;
	element_base* last_tooltip = nullptr;
	element_base* mouse_sensitive_target = nullptr;
	xy_pair target_ul_bounds = xy_pair{ 0, 0 };
	xy_pair target_lr_bounds = xy_pair{ 0, 0 };
	int32_t last_tooltip_sub_index = -1;
	uint32_t cursor_size = 16;
	int32_t target_distance = 0;
	edit_selection_mode selecting_edit_text = edit_selection_mode::none;

	xy_pair relative_mouse_location = xy_pair{ 0, 0 };

	std::unique_ptr<element_base> root;


	template<typename F>
	void for_each_root(F&& func) {
		if(root)
			func(*root);

		// etc for all added roots
	}

	std::unique_ptr<tool_tip> tooltip;
	alice_ui::pop_up_menu_container* popup_menu = nullptr;
	ankerl::unordered_dense::map<std::string, sys::aui_pending_bytes> new_ui_windows;
	std::vector<simple_fs::file> held_open_ui_files;

	// elements we are keeping track of
	ogl::captured_element drag_and_drop_image;
	std::any current_drag_and_drop_data;
	ui::drag_and_drop_data current_drag_and_drop_data_type = ui::drag_and_drop_data::none;

	uint16_t default_header_font = 0;
	uint16_t default_body_font = 0;
	bool ctrl_held_down = false;
	bool shift_held_down = false;
	

	void set_mouse_sensitive_target(sys::state& state, element_base* target);
	void set_focus_target(sys::state& state, element_base* target);
	void update_tooltip(sys::state& state, ui::mouse_probe tooltip_probe, int32_t tooltip_sub_index, int16_t max_height);
	void populate_tooltip(sys::state& state, ui::mouse_probe tooltip_probe, int32_t tooltip_sub_index, int16_t max_height);
	void reposition_tooltip(ui::urect tooltip_bounds, int16_t root_height, int16_t root_width);
	void render_tooltip(sys::state& state, bool follow_mouse, int32_t mouse_x, int32_t mouse_y, int32_t screen_size_x, int32_t screen_size_y, float ui_scale);

	state();
	~state();
};

}
