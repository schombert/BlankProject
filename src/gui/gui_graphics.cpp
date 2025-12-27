#include "gui_graphics.hpp"
#include "simple_fs.hpp"
#include "gui_element_base.hpp"
#include "system_state.hpp"

namespace ui {

int16_t child_relative_location_y_component(element_base const& parent, element_base const& child) {
	auto orientation = child.base_data.get_orientation();
	switch(orientation) {
	case orientation::upper_left:
		return int16_t(child.base_data.position.y);
	case orientation::upper_right:
		return int16_t(child.base_data.position.y);
	case orientation::lower_left:
		return int16_t(parent.base_data.size.y + child.base_data.position.y);
	case orientation::lower_right:
		return int16_t(parent.base_data.size.y + child.base_data.position.y);
	case orientation::upper_center:
		return int16_t(child.base_data.position.y);
	case orientation::lower_center:
		return int16_t(parent.base_data.size.y + child.base_data.position.y);
	case orientation::center:
		return int16_t(parent.base_data.size.y / 2 + child.base_data.position.y);
	default:
		return int16_t(child.base_data.position.y);
	}
}

xy_pair child_relative_non_mirror_location(sys::state& state, element_base const& parent, element_base const& child) {
	auto orientation = child.base_data.get_orientation();
	int16_t y = child_relative_location_y_component(parent, child);
	switch(orientation) {
	case orientation::upper_left:
	case orientation::lower_left:
	default:
		return xy_pair{ int16_t(child.base_data.position.x), y };
	case orientation::upper_right:
	case orientation::lower_right:
		return xy_pair{ int16_t(parent.base_data.size.x + child.base_data.position.x), y };
	case orientation::upper_center:
	case orientation::lower_center:
	case orientation::center:
		return xy_pair{ int16_t(parent.base_data.size.x / 2 + child.base_data.position.x), y };
	}
}

xy_pair child_relative_location(sys::state& state, element_base const& parent, element_base const& child) {
	auto orientation = child.base_data.get_orientation();
	if(state.world.locale_get_native_rtl(state.font_collection.get_current_locale())) {
		int16_t y = child_relative_location_y_component(parent, child);
		switch(orientation) {
		case orientation::upper_left:
		case orientation::lower_left:
		default:
			return xy_pair{ int16_t(parent.base_data.size.x - child.base_data.position.x - child.base_data.size.x), y };
		case orientation::lower_right:
		case orientation::upper_right:
			return xy_pair{ int16_t(-child.base_data.position.x - child.base_data.size.x), y };
		case orientation::upper_center:
		case orientation::lower_center:
		case orientation::center:
			return xy_pair{ int16_t(parent.base_data.size.x / 2 - child.base_data.position.x - child.base_data.size.x), y };
		}
	}
	return child_relative_non_mirror_location(state, parent, child);
}

uint8_t element_base::get_pixel_opacity(sys::state& state, int32_t x, int32_t y, dcon::texture_id tid) {
	uint8_t* pixels = state.open_gl.asset_textures[tid].data;
	int32_t width = state.open_gl.asset_textures[tid].size_x;
	int32_t stride = state.open_gl.asset_textures[tid].channels;
	if(pixels && 0 <= x && x < width && 0 <= y && y < state.open_gl.asset_textures[tid].size_y)
		return pixels[(y * width * stride) + (x * stride) + stride - 1];
	else
		return 0;
}

mouse_probe element_base::impl_probe_mouse(sys::state& state, int32_t x, int32_t y, mouse_probe_type type) noexcept {
	mouse_probe probe_result = mouse_probe{nullptr, xy_pair{int16_t(x), int16_t(y)}};
	if(0 <= x && x < base_data.size.x && 0 <= y && y < base_data.size.y && test_mouse(state, x, y, type) == message_result::consumed) {
		probe_result.under_mouse = this;
	}
	return probe_result;
}
message_result element_base::impl_on_lbutton_down(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mods) noexcept {
	return on_lbutton_down(state, x, y, mods);
}
message_result element_base::impl_on_lbutton_up(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mods, bool under_mouse) noexcept {
	return on_lbutton_up(state, x, y, mods, under_mouse);
}
message_result element_base::impl_on_rbutton_down(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mods) noexcept {
	return on_rbutton_down(state, x, y, mods);
}
message_result element_base::impl_on_key_down(sys::state& state, sys::virtual_key key, sys::key_modifiers mods) noexcept {
	return on_key_down(state, key, mods);
}
message_result element_base::impl_on_scroll(sys::state& state, int32_t x, int32_t y, float amount,
		sys::key_modifiers mods) noexcept {
	return on_scroll(state, x, y, amount, mods);
}
message_result element_base::impl_on_mouse_move(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mods) noexcept {
	return on_mouse_move(state, x, y, mods);
}
void element_base::impl_on_update(sys::state& state) noexcept {
	on_update(state);
}
void element_base::impl_on_reset_text(sys::state& state) noexcept {
	on_reset_text(state);
}

message_result element_base::test_mouse(sys::state& state, int32_t x, int32_t y, mouse_probe_type t) noexcept {
	return message_result::unseen;
}
message_result element_base::on_lbutton_down(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mods) noexcept {
	return message_result::unseen;
}
message_result element_base::on_lbutton_up(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mods, bool under_mouse) noexcept {
	return message_result::unseen;
}
message_result element_base::on_rbutton_down(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mods) noexcept {
	return message_result::unseen;
}

void element_base::on_drag(sys::state& state, int32_t oldx, int32_t oldy, int32_t x, int32_t y,
		sys::key_modifiers mods) noexcept { }
message_result element_base::on_key_down(sys::state& state, sys::virtual_key key, sys::key_modifiers mods) noexcept {
	return message_result::unseen;
}
message_result element_base::on_scroll(sys::state& state, int32_t x, int32_t y, float amount, sys::key_modifiers mods) noexcept {
	return message_result::unseen;
}
message_result element_base::on_mouse_move(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mods) noexcept {
	return message_result::unseen;
}
void element_base::on_update(sys::state& state) noexcept { }

void element_base::impl_render(sys::state& state, int32_t x, int32_t y) noexcept {
	render(state, x, y);
}

xy_pair get_absolute_location(sys::state& state, element_base const& node) {
	if(node.parent) {
		auto parent_loc = get_absolute_location(state, *node.parent);
		auto rel_loc = child_relative_location(state, *node.parent, node);
		return xy_pair{int16_t(parent_loc.x + rel_loc.x), int16_t(parent_loc.y + rel_loc.y)};
	} else {
		if(state.world.locale_get_native_rtl(state.font_collection.get_current_locale())) {
			auto pos = node.base_data.position;
			pos.x = int16_t(state.x_size) - node.base_data.position.x - node.base_data.size.x;
			return pos;
		}
		return node.base_data.position;
	}
}

xy_pair get_absolute_non_mirror_location(sys::state& state, element_base const& node) {
	if(node.parent) {
		auto parent_loc = get_absolute_non_mirror_location(state, *node.parent);
		auto rel_loc = child_relative_non_mirror_location(state, *node.parent, node);
		return xy_pair{ int16_t(parent_loc.x + rel_loc.x), int16_t(parent_loc.y + rel_loc.y) };
	} else {
		return node.base_data.position;
	}
}

int32_t ui_width(sys::state const& state) {
	return int32_t(state.x_size / state.user_settings.ui_scale);
}
int32_t ui_height(sys::state const& state) {
	return int32_t(state.y_size / state.user_settings.ui_scale);
}


void place_in_drag_and_drop(sys::state& state, element_base& elm, std::any const& data, drag_and_drop_data type) {
	state.ui_state.drag_and_drop_image.capture_element(state, elm);
	state.ui_state.current_drag_and_drop_data = data;
	state.ui_state.current_drag_and_drop_data_type = type;
}

} // namespace ui
