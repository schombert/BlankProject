#include <cmath>
#include <bit>

#include "hb.h"
#include "hb-ft.h"
#include "freetype/ftoutln.h"

#include "fonts.hpp"
#include "parsers.hpp"
#include "simple_fs.hpp"
#include "system_state.hpp"
#include "constants.hpp"
#ifdef _WIN32
#include <icu.h>
#else
#include <unicode/ubrk.h>
#include <unicode/utypes.h>
#include <unicode/ubidi.h>
#endif

namespace text {

constexpr uint16_t pack_font_handle(uint32_t font_index, bool black, uint32_t size) {
	return uint16_t(uint32_t((font_index - 1) << 7) | uint32_t(black ? (1 << 6) : 0) | uint32_t(size & 0x3F));
}

bool is_black_font(std::string_view txt) {
	if(parsers::has_fixed_suffix_ci(txt.data(), txt.data() + txt.length(), "_bl") ||
			parsers::has_fixed_suffix_ci(txt.data(), txt.data() + txt.length(), "black") ||
			parsers::has_fixed_suffix_ci(txt.data(), txt.data() + txt.length(), "black_bold")) {
		return true;
	} else {
		return false;
	}
}

uint32_t font_size(std::string_view txt) {
	char const* first_int = txt.data();
	char const* end = txt.data() + txt.size();
	while(first_int != end && !isdigit(*first_int))
		++first_int;
	char const* last_int = first_int;
	while(last_int != end && isdigit(*last_int))
		++last_int;

	if(first_int == last_int) {
		if(parsers::has_fixed_prefix_ci(txt.data(), txt.data() + txt.size(), "fps_font"))
			return uint32_t(14);
		else if(parsers::has_fixed_prefix_ci(txt.data(), txt.data() + txt.size(), "tooltip_font"))
			return uint32_t(16);
		else if(parsers::has_fixed_prefix_ci(txt.data(), txt.data() + txt.size(), "frangoth_bold"))
			return uint32_t(18);
		else if(parsers::has_fixed_prefix_ci(txt.data(), txt.data() + txt.size(), "impact_small"))
			return uint32_t(24);
		else if(parsers::has_fixed_prefix_ci(txt.data(), txt.data() + txt.size(), "old_english"))
			return uint32_t(50);
		else if(parsers::has_fixed_prefix_ci(txt.data(), txt.data() + txt.size(), "timefont"))
			return uint32_t(24);
		else if(parsers::has_fixed_prefix_ci(txt.data(), txt.data() + txt.size(), "vic_title"))
			return uint32_t(42);
		else
			return uint32_t(14);
	}

	uint32_t rvalue = 0;
	std::from_chars(first_int, last_int, rvalue);
	return rvalue;
}

uint32_t font_index(std::string_view txt) {
	if(parsers::has_fixed_prefix_ci(txt.data(), txt.data() + txt.size(), "arial"))
		return 1;
	else if(parsers::has_fixed_prefix_ci(txt.data(), txt.data() + txt.size(), "fps"))
		return 1;
	else if(parsers::has_fixed_prefix_ci(txt.data(), txt.data() + txt.size(), "main"))
		return 2;
	else if(parsers::has_fixed_prefix_ci(txt.data(), txt.data() + txt.size(), "tooltip"))
		return 1;
	else if(parsers::has_fixed_prefix_ci(txt.data(), txt.data() + txt.size(), "frangoth"))
		return 2;
	else if(parsers::has_fixed_prefix_ci(txt.data(), txt.data() + txt.size(), "garamond"))
		return 2;
	else if(parsers::has_fixed_prefix_ci(txt.data(), txt.data() + txt.size(), "impact"))
		return 2;
	else if(parsers::has_fixed_prefix_ci(txt.data(), txt.data() + txt.size(), "old"))
		return 2;
	else if(parsers::has_fixed_prefix_ci(txt.data(), txt.data() + txt.size(), "timefont"))
		return 1;
	else if(parsers::has_fixed_prefix_ci(txt.data(), txt.data() + txt.size(), "vic"))
		return 2;
	else
		return 1;
}

uint16_t name_into_font_id(sys::state& state, std::string_view txt) {
	auto base_id = pack_font_handle(font_index(txt), is_black_font(txt), font_size(txt));
	std::string txt_copy = [&]() {
		if(parsers::has_fixed_suffix_ci(txt.data(), txt.data() + txt.length(), "_black")) {
			return std::string(txt.substr(0, txt.length() - 6));
		}
		if(parsers::has_fixed_suffix_ci(txt.data(), txt.data() + txt.length(), "_black_bold")) {
			return std::string(txt.substr(0, txt.length() - 11)) + "_bold";
		}
		return std::string(txt);
	}();
	uint16_t individuator = 0;
	auto it = state.font_collection.font_names.find(uint16_t(base_id | (individuator << 8)));
	while(it != state.font_collection.font_names.end()) {
		if(state.to_string_view(it->second) == txt_copy) {
			return uint16_t(base_id | (individuator << 8));
		}
		++individuator;
		it = state.font_collection.font_names.find(uint16_t(base_id | (individuator << 8)));
	}
	auto new_key = state.add_key_utf8(txt_copy);
	auto new_handle = uint16_t(base_id | (individuator << 8));
	state.font_collection.font_names.insert_or_assign(new_handle, new_key);
	return new_handle;
}

int32_t size_from_font_id(uint16_t id) {
	auto index = uint32_t(((id >> 7) & 0x01) + 1);
	if(index == 2)
		return (int32_t(id & 0x3F) * 3) / 4;
	else
		return (int32_t(id & 0x3F) * 5) / 6;
}

bool is_black_from_font_id(uint16_t id) {
	return ((id >> 6) & 0x01) != 0;
}
font_selection font_index_from_font_id(sys::state& state, uint16_t id) {
	uint32_t offset = 0;
	if(((id >> 7) & 0x01) == 0)
		return font_selection::body_font;
	else
		return font_selection::header_font;
}

float font_manager::text_extent(sys::state& state, stored_glyphs const& txt, uint32_t starting_offset, uint32_t count, uint16_t font_id) {
	auto& font = get_font(state, text::font_index_from_font_id(state, font_id));
	auto size = text::size_from_font_id(font_id);
	return float(font.retrieve_instance(state, size).text_extent(state, txt, starting_offset, count));
}

float font_manager::line_height(sys::state& state, uint16_t font_id) {
	return float(get_font(state, text::font_index_from_font_id(state, font_id)).retrieve_instance(state, text::size_from_font_id(font_id)).line_height(state));
}

font_manager::font_manager() {
	FT_Init_FreeType(&ft_library);
}
font_manager::~font_manager() {
	//FT_Done_FreeType(ft_library);
}

void font_at_size::reset() {
	if(hb_font_face)
		hb_font_destroy(hb_font_face);
	if(hb_buf)
		hb_buffer_destroy(hb_buf);
	if(font_face)
		FT_Done_Face(font_face);
	hb_font_face = nullptr;
	hb_buf = nullptr;
	font_face = nullptr;

	internal_tx_line_height = 0;
	internal_tx_line_xpos = 1024;
	internal_tx_line_ypos = 1024;

	for(auto& t : textures) {
		glDeleteTextures(1, &t);
	}
	glyph_positions.clear();
	textures.clear();
}

font::~font() {

}

void font::reset_instances() {
	for(auto& inst : sized_fonts)
		inst.second.reset();
	sized_fonts.clear();
}

void font_manager::reset_fonts() {
	for(auto& f : font_array)
		f.reset_instances();
}
void font_manager::change_locale(sys::state& state, dcon::locale_id l) {
	current_locale = l;

	uint32_t end_language = 0;
	auto locale_name = state.world.locale_get_locale_name(l);
	std::string_view localename_sv((char const*)locale_name.begin(), locale_name.size());
	while(end_language < locale_name.size()) {
		if(localename_sv[end_language] == '-')
			break;
		++end_language;
	}

	std::string lang_str{ localename_sv .substr(0, end_language) };
	
	state.world.locale_set_resolved_language(l, hb_language_from_string(localename_sv.data(), int(end_language)));

	{
		auto f = state.world.locale_get_body_font(l);
		std::string fname((char const*)f.begin(), (char const*)f.end());
		font* resolved = nullptr;
		uint16_t count = 0;

		for(auto& fnt : font_array) {
			if(fnt.file_name == fname) {
				resolved = &fnt;
				break;
			}
			++count;
		}

		if(!resolved) {
			auto r = simple_fs::get_root(state.common_fs);
			auto assets = simple_fs::open_directory(r, NATIVE("assets"));
			auto fonts = simple_fs::open_directory(assets, NATIVE("fonts"));
			auto ff = simple_fs::open_file(fonts, simple_fs::utf8_to_native(fname));
			if(!ff) {
				std::abort();
			}

			font_array.emplace_back();
			auto content = simple_fs::view_contents(*ff);
			load_font(font_array.back(), content.data, content.file_size);
			font_array.back().file_name = fname;
			resolved = &(font_array.back());
		}

		state.world.locale_set_resolved_body_font(l, count);
	}
	{
		auto f = state.world.locale_get_header_font(l);
		std::string fname((char const*)f.begin(), (char const*)f.end());
		font* resolved = nullptr;
		uint16_t count = 0;

		for(auto& fnt : font_array) {
			if(fnt.file_name == fname) {
				resolved = &fnt;
				break;
			}
			++count;
		}

		if(!resolved) {
			auto r = simple_fs::get_root(state.common_fs);
			auto assets = simple_fs::open_directory(r, NATIVE("assets"));
			auto fonts = simple_fs::open_directory(assets, NATIVE("fonts"));
			auto ff = simple_fs::open_file(fonts, simple_fs::utf8_to_native(fname));
			if(!ff) {
				std::abort();
			}

			font_array.emplace_back();
			auto content = simple_fs::view_contents(*ff);
			load_font(font_array.back(), content.data, content.file_size);
			font_array.back().file_name = fname;
			resolved = &(font_array.back());
		}

		state.world.locale_set_resolved_header_font(l, count);
	}

	state.reset_locale_pool();

	auto fb_name = state.world.locale_get_fallback(l);
	if(fb_name.size() > 0) {
		std::string_view fb_name_sv((char const*)fb_name.begin(), fb_name.size());
		state.load_locale_strings(fb_name_sv);
	}

	{
		UErrorCode errorCode = U_ZERO_ERROR;
		UBreakIterator* lb_it = ubrk_open(UBreakIteratorType::UBRK_LINE, lang_str.c_str(), nullptr, 0, &errorCode);
		if(!lb_it || !U_SUCCESS(errorCode)) {
			std::abort(); // couldn't create iterator
		}
		auto rule_size = ubrk_getBinaryRules(lb_it, nullptr, 0, &errorCode);
		if(rule_size == 0 || !U_SUCCESS(errorCode)) {
			std::abort(); // couldn't get_rules
		}

		state.font_collection.compiled_ubrk_rules.resize(uint32_t(rule_size));
		ubrk_getBinaryRules(lb_it, state.font_collection.compiled_ubrk_rules.data(), rule_size, &errorCode);

		ubrk_close(lb_it);
	}
	{
		UErrorCode errorCode = U_ZERO_ERROR;
		UBreakIterator* ch_it = ubrk_open(UBreakIteratorType::UBRK_CHARACTER, lang_str.c_str(), nullptr, 0, &errorCode);
		if(!ch_it || !U_SUCCESS(errorCode)) {
			std::abort(); // couldn't create iterator
		}
		auto rule_size = ubrk_getBinaryRules(ch_it, nullptr, 0, &errorCode);
		if(rule_size == 0 || !U_SUCCESS(errorCode)) {
			std::abort(); // couldn't get_rules
		}

		state.font_collection.compiled_char_ubrk_rules.resize(uint32_t(rule_size));
		ubrk_getBinaryRules(ch_it, state.font_collection.compiled_char_ubrk_rules.data(), rule_size, &errorCode);

		ubrk_close(ch_it);
	}
	{
		UErrorCode errorCode = U_ZERO_ERROR;
		UBreakIterator* ch_it = ubrk_open(UBreakIteratorType::UBRK_WORD, lang_str.c_str(), nullptr, 0, &errorCode);
		if(!ch_it || !U_SUCCESS(errorCode)) {
			std::abort(); // couldn't create iterator
		}
		auto rule_size = ubrk_getBinaryRules(ch_it, nullptr, 0, &errorCode);
		if(rule_size == 0 || !U_SUCCESS(errorCode)) {
			std::abort(); // couldn't get_rules
		}

		state.font_collection.compiled_word_ubrk_rules.resize(uint32_t(rule_size));
		ubrk_getBinaryRules(ch_it, state.font_collection.compiled_word_ubrk_rules.data(), rule_size, &errorCode);

		ubrk_close(ch_it);
	}

	state.load_locale_strings(localename_sv);
}

font& font_manager::get_font(sys::state& state, font_selection s) {
	if(!current_locale)
		std::abort();
	switch(s) {
	case font_selection::body_font:
	default:
		return font_array[state.world.locale_get_resolved_body_font(current_locale)];
	case font_selection::header_font:
		return font_array[state.world.locale_get_resolved_header_font(current_locale)];
	}
}

font_at_size& font::retrieve_instance(sys::state& state, int32_t base_size) {
	if(auto it = sized_fonts.find(int32_t(base_size * state.user_settings.ui_scale)); it != sized_fonts.end()) {
		return it->second;
	}
	auto t = sized_fonts.insert_or_assign(int32_t(base_size * state.user_settings.ui_scale), font_at_size{});
	t.first->second.create(state.font_collection.ft_library, file_data.get(), file_size, int32_t(base_size * state.user_settings.ui_scale));
	return t.first->second;
}

font_at_size& font::retrieve_stateless_instance(FT_Library lib, int32_t base_size) {
	if(auto it = sized_fonts.find(base_size); it != sized_fonts.end()) {
		return it->second;
	}
	auto t = sized_fonts.insert_or_assign(base_size , font_at_size{});
	t.first->second.create(lib, file_data.get(), file_size, base_size);
	return t.first->second;
}

void font_at_size::create(FT_Library lib, FT_Byte* file_data, size_t file_size, int32_t real_size) {
	FT_New_Memory_Face(lib, file_data, FT_Long(file_size), 0, &font_face);
	FT_Select_Charmap(font_face, FT_ENCODING_UNICODE);
	FT_Set_Pixel_Sizes(font_face, real_size, real_size);
	hb_font_face = hb_ft_font_create(font_face, nullptr);
	hb_buf = hb_buffer_create();
	px_size = real_size;

	internal_line_height = float(font_face->size->metrics.height) / text::fixed_to_fp;
	internal_ascender = float(font_face->size->metrics.ascender) / text::fixed_to_fp;
	internal_descender = -float(font_face->size->metrics.descender) / text::fixed_to_fp;
	internal_top_adj = (internal_line_height - (internal_ascender + internal_descender)) / 2.0f;
}

void font_manager::load_font(font& fnt, char const* file_data, uint32_t fz) {
	fnt.file_data = std::unique_ptr<FT_Byte[]>(new FT_Byte[fz]);
	fnt.file_size = fz;
	memcpy(fnt.file_data.get(), file_data, fz);
}

float font_at_size::line_height(sys::state& state) const {
	return internal_line_height / state.user_settings.ui_scale;
}
float font_at_size::ascender(sys::state& state) const {
	return internal_ascender / state.user_settings.ui_scale;
}
float font_at_size::descender(sys::state& state) const {
	return internal_descender / state.user_settings.ui_scale;
}
float font_at_size::top_adjustment(sys::state& state) const {
	return internal_top_adj  / state.user_settings.ui_scale;
}

bool font::can_display(char32_t ch_in) const {
	if(sized_fonts.empty())
		return true;
	return FT_Get_Char_Index(sized_fonts.begin()->second.font_face, ch_in) != 0;
}

glyph_sub_offset& font_at_size:: get_glyph(uint16_t glyph_in, int32_t subpixel) {
	return glyph_positions[(uint32_t(glyph_in) << 2) | uint32_t(subpixel & 3)];
}
void font_at_size::make_glyph(uint16_t glyph_in, int32_t subpixel) {
	if(glyph_positions.find((uint32_t(glyph_in) << 2) | uint32_t(subpixel & 3)) != glyph_positions.end())
		return;

	// load all glyph metrics
	if(glyph_in) {
		FT_Load_Glyph(font_face, glyph_in, FT_LOAD_TARGET_LIGHT);
		glyph_sub_offset gso;

		if(subpixel == 1) {
			FT_Outline_Translate(&(font_face->glyph->outline), 16, 0);
		} else if(subpixel == 2) {
			FT_Outline_Translate(&(font_face->glyph->outline), 32, 0);
		} else if(subpixel == 3) {
			FT_Outline_Translate(&(font_face->glyph->outline), 48, 0);
		}

		FT_Render_Glyph(font_face->glyph, FT_RENDER_MODE_NORMAL);

		FT_Glyph g_result;
		auto err = FT_Get_Glyph(font_face->glyph, &g_result);
		if(err != 0) {
			glyph_positions.insert_or_assign((uint32_t(glyph_in) << 2) | uint32_t(subpixel & 3), gso);
			return;
		}
		
		FT_Bitmap const& bitmap = ((FT_BitmapGlyphRec*)g_result)->bitmap;

		assert(bitmap.rows <= 1024 && bitmap.width <= 1024);
		if(bitmap.rows > 1024 || bitmap.width > 1024) { // too large to render
			FT_Done_Glyph(g_result);
			glyph_positions.insert_or_assign((uint32_t(glyph_in) << 2) | uint32_t(subpixel & 3), gso);
			return;
		}
		if(bitmap.width + internal_tx_line_xpos >= 1024) { // new line
			internal_tx_line_xpos = 0;
			internal_tx_line_ypos += internal_tx_line_height;
			internal_tx_line_height = 0;
		}
		GLuint texid = 0;
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		if(bitmap.rows + internal_tx_line_ypos >= 1024) { // new bitmap
			internal_tx_line_xpos = 0;
			internal_tx_line_ypos = 0;
			internal_tx_line_height = 0;

			glGenTextures(1, &texid);
			glBindTexture(GL_TEXTURE_2D, texid);
			glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, 1024, 1024);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			textures.push_back(texid);
			uint32_t clearvalue = 0;
			glClearTexImage(texid, 0, GL_RED, GL_UNSIGNED_BYTE, &clearvalue);
		} else {
			texid = textures.back();
			glBindTexture(GL_TEXTURE_2D, texid);
		}
		gso.x = uint16_t(internal_tx_line_xpos);
		gso.y = uint16_t(internal_tx_line_ypos );
		gso.width = uint16_t(bitmap.width);
		gso.height = uint16_t(bitmap.rows);
		gso.tx_sheet = uint16_t(textures.size() - 1);
		gso.bitmap_left = int16_t(((FT_BitmapGlyphRec*)g_result)->left);
		gso.bitmap_top = int16_t(((FT_BitmapGlyphRec*)g_result)->top);

		internal_tx_line_xpos += bitmap.width + 1;
		internal_tx_line_height = std::max(internal_tx_line_height, bitmap.rows + 1);

		if(bitmap.pitch == int32_t(bitmap.width)) {
			glTexSubImage2D(GL_TEXTURE_2D, 0, int32_t(gso.x), int32_t(gso.y), bitmap.width, bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, bitmap.buffer);
		} else {
			uint8_t* temp = new uint8_t[bitmap.width * bitmap.rows];
			for(uint32_t j = 0; j < bitmap.rows; ++j) {
				for(uint32_t i = 0; i < bitmap.width; ++i) {
					temp[i + j * bitmap.width] = uint8_t(bitmap.buffer[i + j * bitmap.pitch]);
				}
			}
			glTexSubImage2D(GL_TEXTURE_2D, 0, int32_t(gso.x), int32_t(gso.y), bitmap.width, bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, temp);
			delete[] temp;
		}
		FT_Done_Glyph(g_result);
		glyph_positions.insert_or_assign((uint32_t(glyph_in) << 2) | uint32_t(subpixel & 3), gso);
	}
}

stored_glyphs::stored_glyphs(sys::state& state, int32_t size, font_selection type, std::span<uint16_t> s, uint32_t details_offset, layout_details* d, uint16_t font_handle) {
	state.font_collection.get_font(state, type).retrieve_instance(state, size).remake_cache(state, type, *this, s, details_offset, d, font_handle);
}
stored_glyphs::stored_glyphs(sys::state& state, int32_t size, font_selection type, std::span<uint16_t> s, no_bidi) {
	state.font_collection.get_font(state, type).retrieve_instance(state, size).remake_bidiless_cache(state, type, *this, s);
}

stored_glyphs::stored_glyphs(stored_glyphs& other, uint32_t offset, uint32_t count) {
	glyph_info.resize(count);
	std::copy_n(other.glyph_info.data() + offset, count, glyph_info.data());
}

void font_at_size::remake_cache(sys::state& state, font_selection type, stored_glyphs& txt, std::span<uint16_t> source, uint32_t details_offset, layout_details* d, uint16_t font_handle) {
	txt.glyph_info.clear();

	if(source.size() == 0)
		return;

	auto locale = state.font_collection.get_current_locale();
	UBiDi* para;
	UErrorCode errorCode = U_ZERO_ERROR;

	para = ubidi_open();
	//para = ubidi_openSized(int32_t(temp_text.size()), 64, pErrorCode);
	if(!para)
		std::abort();

	hb_feature_t feature_buffer[10];
	auto features = type == font_selection::body_font ? state.world.locale_get_body_font_features(locale) :  state.world.locale_get_header_font_features(locale) ;
	for(uint32_t i = 0; i < uint32_t(std::extent_v<decltype(feature_buffer)>) && i < features.size(); ++i) {
		feature_buffer[i].tag = features[i];
		feature_buffer[i].start = 0;
		feature_buffer[i].end = (unsigned int)-1;
		feature_buffer[i].value = 1;
	}
	uint32_t hb_feature_count = std::min(features.size(), uint32_t(std::extent_v<decltype(feature_buffer)>));

	ubidi_setPara(para, (UChar const*)(source.data()), int32_t(source.size()), state.world.locale_get_native_rtl(locale) ? 1 : 0, nullptr, &errorCode);

	if(U_SUCCESS(errorCode)) {
		auto runcount = ubidi_countRuns(para, &errorCode);
		float total_x_advance = 0;

		if(U_SUCCESS(errorCode)) {
			// TODO -- find any previous added to the same line
			int32_t previous_rightmost_in_run = -1;
			int32_t last_run_rightmost = -1;

			for(int32_t i = 0; i < runcount; ++i) {
				int32_t logical_start = 0;
				int32_t length = 0;
				auto direction = ubidi_getVisualRun(para, i, &logical_start, &length);

				// shape run with harfbuzz
				hb_buffer_clear_contents(hb_buf);
				hb_buffer_add_utf16(hb_buf, source.data(), int32_t(source.size()), logical_start, length);

				hb_buffer_set_direction(hb_buf, direction == UBIDI_RTL ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
				hb_buffer_set_script(hb_buf, (hb_script_t)state.world.locale_get_hb_script(locale));
				hb_buffer_set_language(hb_buf, state.world.locale_get_resolved_language(locale));

				hb_shape(hb_font_face, hb_buf, feature_buffer, hb_feature_count);

				uint32_t gcount = 0;
				hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(hb_buf, &gcount);
				hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(hb_buf, &gcount);

				if(d) {
					UBreakIterator* cb_it = ubrk_openBinaryRules(state.font_collection.compiled_char_ubrk_rules.data(), int32_t(state.font_collection.compiled_char_ubrk_rules.size()), (UChar const*)(source.data() + logical_start), int32_t(length), &errorCode);

					if(!cb_it || !U_SUCCESS(errorCode)) {
						std::abort(); // couldn't create iterator
					}

					ubrk_first(cb_it);
					int32_t start_cluster_position = 0;
					int32_t next_cluster_position = 0;
					int32_t previous_placed = -1;
					size_t start_of_new_entries = d->grapheme_placement.size();

					do {
						next_cluster_position = ubrk_next(cb_it);

						auto end_seq = next_cluster_position != UBRK_DONE ? next_cluster_position : int32_t(length);
						if(end_seq == start_cluster_position) // zero sized cluster -- i.e. none found
							continue;

						d->grapheme_placement.emplace_back();
						auto& new_exgc = d->grapheme_placement.back();
						if(direction == UBIDI_RTL)
							new_exgc.flags |= text::ex_grapheme_cluster_info::f_has_rtl_directionality;
						new_exgc.line = d->total_lines;
						new_exgc.source_offset = uint16_t(start_cluster_position + logical_start + details_offset);
						new_exgc.unit_length = uint8_t(end_seq - start_cluster_position);

						if(start_of_new_entries != 0 && start_cluster_position == 0) {
							d->grapheme_placement[start_of_new_entries - 1].line = d->total_lines;
						}

						// link to visually left/right graphemes
						if(direction == UBIDI_RTL) {
							if(previous_placed == -1) {
								previous_rightmost_in_run = int32_t(d->grapheme_placement.size() - 1);
								new_exgc.visual_left = int16_t(previous_rightmost_in_run);
								new_exgc.visual_right = -1;
								if(last_run_rightmost != -1)
									d->grapheme_placement[last_run_rightmost].visual_right = int16_t(d->grapheme_placement.size()) - int16_t(1);
							} else {
								new_exgc.visual_right = int16_t(previous_placed);
								d->grapheme_placement[previous_placed].visual_left = int16_t(d->grapheme_placement.size()) - int16_t(1);

								if(last_run_rightmost != -1)
									d->grapheme_placement[last_run_rightmost].visual_right = int16_t(d->grapheme_placement.size()) - int16_t(1);
							}

							previous_placed = int32_t(d->grapheme_placement.size()) - 1;
						} else {
							if(previous_placed != -1) {
								new_exgc.visual_left = int16_t(previous_placed);
								d->grapheme_placement[previous_placed].visual_right = int16_t(d->grapheme_placement.size()) - int16_t(1);
							} else if(last_run_rightmost != -1) {
								new_exgc.visual_left = int16_t(last_run_rightmost);
								d->grapheme_placement[last_run_rightmost].visual_right = int16_t(d->grapheme_placement.size()) - int16_t(1);
							} else {
								new_exgc.visual_left = -1;
							}

							previous_rightmost_in_run = int32_t(d->grapheme_placement.size()) - 1;
							previous_placed = int32_t(d->grapheme_placement.size()) - 1;
						}

						// find rendered position or the rendering group it is part of
						new_exgc.width = 0;
						new_exgc.x_offset = 0;

						start_cluster_position = next_cluster_position;
					} while(next_cluster_position != UBRK_DONE);

					last_run_rightmost = previous_rightmost_in_run;
					ubrk_close(cb_it);

					// find word breaks
					UBreakIterator* wb_it = ubrk_openBinaryRules(state.font_collection.compiled_word_ubrk_rules.data(), int32_t(state.font_collection.compiled_word_ubrk_rules.size()), (UChar const*)(source.data() + logical_start), int32_t(length), &errorCode);

					if(!wb_it || !U_SUCCESS(errorCode)) {
						std::abort(); // couldn't create iterator
					}
					ubrk_first(wb_it);

					int32_t start_wb_position = 0;
					int32_t next_wb_position = 0;

					do {
						next_wb_position = ubrk_next(wb_it);
						auto end_seq = next_wb_position != UBRK_DONE ? next_wb_position : int32_t(length);

						//find word start
						for(auto k = start_of_new_entries; k < d->grapheme_placement.size(); ++k) {
							if(d->grapheme_placement[k].source_offset == uint16_t(start_wb_position + logical_start + details_offset)) {
								d->grapheme_placement[k].flags |= text::ex_grapheme_cluster_info::f_is_word_start;
								break;
							}
						}
						//find word end
						auto best_found = -1;
						for(auto k = start_of_new_entries; k < d->grapheme_placement.size(); ++k) {
							if(uint16_t(start_wb_position + logical_start + details_offset) <= d->grapheme_placement[k].source_offset
								&& d->grapheme_placement[k].source_offset < uint16_t(end_seq + logical_start + details_offset)) {

								best_found = int32_t(k);
							}
						}
						if(best_found != -1) {
							d->grapheme_placement[best_found].flags |= text::ex_grapheme_cluster_info::f_is_word_end;
						}

						start_wb_position = next_wb_position;
					} while(next_wb_position != UBRK_DONE);
					ubrk_close(wb_it);

					// find visual location of graphemes
					for(auto k = start_of_new_entries; k < d->grapheme_placement.size(); ++k) {
						bool matched_exactly = false;
						int32_t best_match = -1;
						uint32_t best_match_index = 0;
						float accumulated_advance = 0;

						for(unsigned int j = 0; j < gcount; j++) {
							auto rendering_details_for = glyph_info[j].cluster + details_offset;
							if(uint16_t(rendering_details_for) < d->grapheme_placement[k].source_offset) {
								accumulated_advance += glyph_pos[j].x_advance / (text::fixed_to_fp * state.user_settings.ui_scale);
							}
							if(uint16_t(rendering_details_for) == d->grapheme_placement[k].source_offset) {
								matched_exactly = true;
								d->grapheme_placement[k].x_offset = int16_t(accumulated_advance + total_x_advance);
								d->grapheme_placement[k].width = int16_t(glyph_pos[j].x_advance / (text::fixed_to_fp * state.user_settings.ui_scale));
								break;
							} else if(uint16_t(rendering_details_for) < d->grapheme_placement[k].source_offset
								&& int32_t(rendering_details_for) > best_match) {
								best_match = int32_t(rendering_details_for);
								best_match_index = j;
							}
						}

						if(!matched_exactly) {
							if(best_match != -1) {
								// scan added exgc to find the range associated with this grapheme cluster
								auto rendering_details_for = glyph_info[best_match_index].cluster + details_offset;
								accumulated_advance -= glyph_pos[best_match_index].x_advance / (text::fixed_to_fp * state.user_settings.ui_scale);

								int32_t start_exgc = -1;

								for(auto m = start_of_new_entries; m < d->grapheme_placement.size(); ++m) {
									if(d->grapheme_placement[m].source_offset == int16_t(rendering_details_for)) {
										start_exgc = int32_t(m);
										break;
									}
								}

								if(start_exgc != -1 && start_exgc <= int32_t(k)) {
									auto count_in_range = 1 + int32_t(k) - start_exgc;

									// adjust positions and widths for entire cluster range
									if(direction == UBIDI_RTL) {
										for(int32_t m = start_exgc; m <= int32_t(k); ++m) {
											d->grapheme_placement[k].x_offset = int16_t(accumulated_advance + total_x_advance +
												(glyph_pos[best_match_index].x_advance / (text::fixed_to_fp * state.user_settings.ui_scale) * (count_in_range - (m - start_exgc + 1))) / count_in_range);
											d->grapheme_placement[k].width = int16_t(
												(glyph_pos[best_match_index].x_advance / (text::fixed_to_fp * state.user_settings.ui_scale) * (count_in_range - (m - start_exgc))) / count_in_range
												- (glyph_pos[best_match_index].x_advance / (text::fixed_to_fp * state.user_settings.ui_scale) * (count_in_range - (m - start_exgc + 1))) / count_in_range
											);
										}
									} else {
										for(int32_t m = start_exgc; m <= int32_t(k); ++m) {
											d->grapheme_placement[k].x_offset = int16_t(accumulated_advance + total_x_advance +
												(glyph_pos[best_match_index].x_advance / (text::fixed_to_fp * state.user_settings.ui_scale) * (m - start_exgc)) / count_in_range);
											d->grapheme_placement[k].width = int16_t(
												(glyph_pos[best_match_index].x_advance / (text::fixed_to_fp * state.user_settings.ui_scale) * (1 + m - start_exgc)) / count_in_range
												- (glyph_pos[best_match_index].x_advance / (text::fixed_to_fp * state.user_settings.ui_scale) * (m - start_exgc)) / count_in_range
											);
										}
									}
								}
							}
						}
					}
				}

				for(unsigned int j = 0; j < gcount; j++) { // Preload glyphs
					total_x_advance += glyph_pos[j].x_advance / (text::fixed_to_fp * state.user_settings.ui_scale);
					//make_glyph(uint16_t(glyph_info[j].codepoint));
					txt.glyph_info.emplace_back(glyph_info[j], glyph_pos[j]);
				}
			}
		} else {
			// failure to get number of runs
			std::abort();
		}
	} else {
		// failure to add text
		std::abort();
	}

	ubidi_close(para);
}

void font_at_size::remake_bidiless_cache(sys::state& state, font_selection type, stored_glyphs& txt, std::span<uint16_t> source) {
	txt.glyph_info.clear();
	if(source.size() == 0)
		return;


	auto locale = state.font_collection.get_current_locale();
	
	hb_feature_t feature_buffer[10];
	auto features = type == font_selection::body_font ? state.world.locale_get_body_font_features(locale) : state.world.locale_get_header_font_features(locale);
	for(uint32_t i = 0; i < uint32_t(std::extent_v<decltype(feature_buffer)>) && i < features.size(); ++i) {
		feature_buffer[i].tag = features[i];
		feature_buffer[i].start = 0;
		feature_buffer[i].end = (unsigned int)-1;
		feature_buffer[i].value = 1;
	}
	uint32_t hb_feature_count = std::min(features.size(), uint32_t(std::extent_v<decltype(feature_buffer)>));

	// shape run with harfbuzz
	hb_buffer_clear_contents(hb_buf);
	hb_buffer_add_utf16(hb_buf, source.data(), int32_t(source.size()), 0, int32_t(source.size()));

	hb_buffer_set_direction(hb_buf, state.world.locale_get_native_rtl(locale) ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
	hb_buffer_set_script(hb_buf, (hb_script_t)state.world.locale_get_hb_script(locale));
	hb_buffer_set_language(hb_buf, state.world.locale_get_resolved_language(locale));

	hb_shape(hb_font_face, hb_buf, feature_buffer, hb_feature_count);

	uint32_t gcount = 0;
	hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(hb_buf, &gcount);
	hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(hb_buf, &gcount);

	for(unsigned int j = 0; j < gcount; j++) { // Preload glyphs
		//make_glyph(uint16_t(glyph_info[j].codepoint));
		txt.glyph_info.emplace_back(glyph_info[j], glyph_pos[j]);
	}

	if(state.world.locale_get_native_rtl(locale)) {
		std::reverse(txt.glyph_info.begin(), txt.glyph_info.end());
	}
}


float font_at_size::text_extent(sys::state& state, stored_glyphs const& txt, uint32_t starting_offset, uint32_t count) {
	float x_total = 0.0f;
	for(uint32_t i = starting_offset; i < starting_offset + count; i++) {
		hb_codepoint_t glyphid = txt.glyph_info[i].codepoint;
		float x_advance = float(txt.glyph_info[i].x_advance) / text::fixed_to_fp;
		x_total += x_advance;
	}
	return x_total / state.user_settings.ui_scale;
}

float font_at_size::stateless_text_extent(float ui_scale, char const* codepoints, uint32_t count) {
	hb_buffer_clear_contents(hb_buf);
	hb_buffer_add_utf8(hb_buf, codepoints, int(count), 0, int(count));
	hb_buffer_guess_segment_properties(hb_buf);
	hb_shape(hb_font_face, hb_buf, NULL, 0);
	unsigned int glyph_count = 0;
	hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(hb_buf, &glyph_count);
	hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(hb_buf, &glyph_count);
	float x = 0.0f;
	for(unsigned int i = 0; i < glyph_count; i++) {
		make_glyph((uint16_t)glyph_info[i].codepoint, 0);
		hb_codepoint_t glyphid = glyph_info[i].codepoint;
		auto& gso = glyph_positions[glyphid << 2];
		float x_advance = float(glyph_pos[i].x_advance) / text::fixed_to_fp;
		x += x_advance;
	}

	return x / ui_scale;
}

uint16_t make_font_id(sys::state& state, bool as_header, float target_line_size) {
	int32_t calculated_size = int32_t(target_line_size);
	if(as_header) {
		//auto jvalue = state.font_collection.get_font(state, font_selection::header_font).line_height(1);
		//calculated_size = int32_t((target_line_size / jvalue) * 4.0f / 3.0f);
		return uint16_t((1 << 7) | (0x3F & calculated_size));
	} else {
		//auto jvalue = state.font_collection.get_font(state, font_selection::body_font).line_height(1);
		//calculated_size = int32_t((target_line_size / jvalue) * 6.0f / 5.0f);
		return uint16_t((0 << 7) | (0x3F & calculated_size));
	}
}


} // namespace text

