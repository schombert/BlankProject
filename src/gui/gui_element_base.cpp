#include "system_state.hpp"
#include "gui_element_base.hpp"

namespace ui {
tooltip_behavior element_base::has_tooltip(sys::state& state) noexcept { // used to test whether a tooltip is possible
	return tooltip_behavior::no_tooltip;
}
void element_base::tooltip_position(sys::state& state, int32_t x, int32_t y, int32_t& ident, urect& subrect) noexcept {
	ident = 0;
	subrect.top_left = ui::get_absolute_location(state, *this);
	subrect.size = base_data.size;
}
void element_base::update_tooltip(sys::state& state, int32_t x, int32_t y, text::columnar_layout& contents) noexcept {
}
}
