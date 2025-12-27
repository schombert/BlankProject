#include "parsers.hpp"
#include <charconv>
#include <algorithm>

namespace parsers {
bool ignorable_char(char c) {
	return (c == ' ') || (c == '\r') || (c == '\f') || (c == '\n') || (c == '\t') || (c == ',') || (c == ';');
}

bool special_identifier_char(char c) {
	return (c == '!') || (c == '=') || (c == '<') || (c == '>');
}

bool breaking_char(char c) {
	return ignorable_char(c) || (c == '{') || (c == '}') || special_identifier_char(c) || (c == '#');
}

bool not_special_identifier_char(char c) {
	return !special_identifier_char(c);
}

bool line_termination(char c) {
	return (c == '\r') || (c == '\n');
}

bool double_quote_termination(char c) {
	return (c == '\r') || (c == '\n') || (c == '\"');
}

bool single_quote_termination(char c) {
	return (c == '\r') || (c == '\n') || (c == '\'');
}

bool is_positive_integer(char const* start, char const* end) {
	if(start == end)
		return false;
	while(start < end) {
		if(!isdigit(*start))
			return false;
		++start;
	}
	return true;
}

bool is_integer(char const* start, char const* end) {
	if(start == end)
		return false;
	if(*start == '-')
		return is_positive_integer(start + 1, end);
	else
		return is_positive_integer(start, end);
}

bool is_positive_fp(char const* start, char const* end) {
	auto const decimal = std::find(start, end, '.');
	if(decimal == end) {
		return is_positive_integer(start, end);
	} else if(decimal == start) {
		return is_positive_integer(decimal + 1, end);
	} else {
		return is_positive_integer(start, decimal) && (decimal + 1 == end || is_positive_integer(decimal + 1, end));
	}
}

bool is_fp(char const* start, char const* end) {
	if(start == end)
		return false;
	if(*start == '-')
		return is_positive_fp(start + 1, end);
	else
		return is_positive_fp(start, end);
}

template<typename T>
char const* scan_for_match(char const* start, char const* end, int32_t& current_line, T&& condition) {
	while(start < end) {
		if(condition(*start))
			return start;
		if(*start == '\n')
			++current_line;
		++start;
	}
	return start;
}
template<typename T>
char const* scan_for_not_match(char const* start, char const* end, int32_t& current_line, T&& condition) {
	while(start < end) {
		if(!condition(*start))
			return start;
		if(*start == '\n')
			++current_line;
		++start;
	}
	return start;
}

char const* advance_position_to_next_line(char const* start, char const* end, int32_t& current_line) {
	auto const start_lterm = scan_for_match(start, end, current_line, line_termination);
	return scan_for_not_match(start_lterm, end, current_line, line_termination);
}

char const* advance_position_to_non_whitespace(char const* start, char const* end, int32_t& current_line) {
	return scan_for_not_match(start, end, current_line, ignorable_char);
}

char const* advance_position_to_non_comment(char const* start, char const* end, int32_t& current_line) {
	auto position = advance_position_to_non_whitespace(start, end, current_line);
	while(position < end && *position == '#') {
		auto start_of_new_line = advance_position_to_next_line(position, end, current_line);
		position = advance_position_to_non_whitespace(start_of_new_line, end, current_line);
	}
	return position;
}

char const* advance_position_to_breaking_char(char const* start, char const* end, int32_t& current_line) {
	return scan_for_match(start, end, current_line, breaking_char);
}

token_and_type token_generator::internal_next() {
	if(position >= file_end)
		return token_and_type{std::string_view(), current_line, token_type::unknown};

	auto non_ws = advance_position_to_non_comment(position, file_end, current_line);
	if(non_ws < file_end) {
		if(*non_ws == '{') {
			position = non_ws + 1;
			return token_and_type{std::string_view(non_ws, 1), current_line, token_type::open_brace};
		} else if(*non_ws == '}') {
			position = non_ws + 1;
			return token_and_type{std::string_view(non_ws, 1), current_line, token_type::close_brace};
		} else if(*non_ws == '\"') {
			auto const close = scan_for_match(non_ws + 1, file_end, current_line, double_quote_termination);
			position = close + 1;
			return token_and_type{std::string_view(non_ws + 1, close - (non_ws + 1)), current_line, token_type::quoted_string};
		} else if(*non_ws == '\'') {
			auto const close = scan_for_match(non_ws + 1, file_end, current_line, single_quote_termination);
			position = close + 1;
			return token_and_type{std::string_view(non_ws + 1, close - (non_ws + 1)), current_line, token_type::quoted_string};
		} else if(has_fixed_prefix(non_ws, file_end, "==") || has_fixed_prefix(non_ws, file_end, "<=") ||
							has_fixed_prefix(non_ws, file_end, ">=") || has_fixed_prefix(non_ws, file_end, "<>") ||
							has_fixed_prefix(non_ws, file_end, "!=")) {

			position = non_ws + 2;
			return token_and_type{std::string_view(non_ws, 2), current_line, token_type::special_identifier};
		} else if(*non_ws == '<' || *non_ws == '>' || *non_ws == '=') {

			position = non_ws + 1;
			return token_and_type{std::string_view(non_ws, 1), current_line, token_type::special_identifier};
		} else {
			position = advance_position_to_breaking_char(non_ws + 1, file_end, current_line);
			return token_and_type{std::string_view(non_ws, position - non_ws), current_line, token_type::identifier};
		}
	} else {
		position = file_end;
		return token_and_type{std::string_view(), current_line, token_type::unknown};
	}
}

token_and_type token_generator::get() {
	if(peek_1.type != token_type::unknown) {
		auto const temp = peek_1;
		peek_1 = peek_2;
		peek_2.type = token_type::unknown;
		return temp;
	}

	return internal_next();
}

token_and_type token_generator::next() {
	if(peek_1.type == token_type::unknown) {
		peek_1 = internal_next();
	}
	return peek_1;
}

token_and_type token_generator::next_next() {
	if(peek_1.type == token_type::unknown) {
		peek_1 = internal_next();
	}
	if(peek_2.type == token_type::unknown) {
		peek_2 = internal_next();
	}
	return peek_2;
}

void token_generator::discard_group() {
	int32_t brace_count = 0;

	while(brace_count >= 0 && !at_end()) {
		auto gotten = get();
		if(gotten.type == token_type::open_brace) {
			brace_count++;
		} else if(gotten.type == token_type::close_brace) {
			brace_count--;
		}
	}
}

bool parse_bool(std::string_view content, int32_t, error_handler&) {
	if(content.length() == 0)
		return false;
	else
		return (content[0] == 'Y') || (content[0] == 'y') || (content[0] == '1');
}

float parse_float(std::string_view content, int32_t line, error_handler& err) {
	float rvalue = 0.0f;

	if(!float_from_chars(content.data(), content.data() + content.length(), rvalue)) {
		err.bad_float(content, line);
	}

	return rvalue;
}

double parse_double(std::string_view content, int32_t line, error_handler& err) {
	double rvalue = 0.0;
	if(!double_from_chars(content.data(), content.data() + content.length(), rvalue)) {
		err.bad_float(content, line);
	}
	return rvalue;
}

int32_t parse_int(std::string_view content, int32_t line, error_handler& err) {
	int32_t rvalue = 0;
	auto result = std::from_chars(content.data(), content.data() + content.length(), rvalue);
	if(result.ec == std::errc::invalid_argument) {
		err.bad_int(content, line);
	}
	return rvalue;
}

uint32_t parse_uint(std::string_view content, int32_t line, error_handler& err) {
	uint32_t rvalue = 0;
	auto result = std::from_chars(content.data(), content.data() + content.length(), rvalue);
	if(result.ec == std::errc::invalid_argument) {
		err.bad_unsigned_int(content, line);
	}
	return rvalue;
}

bool starts_with(std::string_view content, char v) {
	return content.length() != 0 && content[0] == v;
}

association_type parse_association_type(std::string_view content, int32_t line, error_handler& err) {
	if(content.length() == 1) {
		if(content[0] == '>')
			return association_type::gt;
		else if(content[0] == '<')
			return association_type::lt;
		else if(content[0] == '=')
			return association_type::eq_default;
	} else if(content.length() == 2) {
		if(content[0] == '=' && content[1] == '=')
			return association_type::eq;
		else if(content[0] == '<' && content[1] == '=')
			return association_type::le;
		else if(content[0] == '>' && content[1] == '=')
			return association_type::ge;
		else if(content[0] == '!' && content[1] == '=')
			return association_type::ne;
		else if(content[0] == '<' && content[1] == '>')
			return association_type::ne;
	}
	err.bad_association_token(content, line);
	return association_type::none;
}


separator_scan_result csv_find_separator_token(char const* start, char const* end, char seperator) {
	while(start != end) {
		if(line_termination(*start))
			return separator_scan_result{start, false};
		else if(*start == seperator)
			return separator_scan_result{start, true};
		else
			++start;
	}
	return separator_scan_result{start, false};
}

char const* csv_advance(char const* start, char const* end, char seperator) {
	while(start != end) {
		if(line_termination(*start))
			return start;
		else if(*start == seperator)
			return start + 1;
		else
			++start;
	}
	return start;
}

char const* csv_advance_n(uint32_t n, char const* start, char const* end, char seperator) {

	if(n == 0)
		return start;
	--n;

	while(start != end) {
		if(line_termination(*start))
			return start;
		else if(*start == seperator) {
			if(n == 0)
				return start + 1;
			else
				--n;
		}
		++start;
	}
	return start;
}

char const* csv_advance_to_next_line(char const* start, char const* end) {

	while(start != end && !line_termination(*start)) {
		++start;
	}
	while(start != end && line_termination(*start))
		++start;
	if(start == end || *start != '#')
		return start;
	else
		return csv_advance_to_next_line(start, end);
}

std::string_view remove_surrounding_whitespace(std::string_view txt) {
	char const* start = txt.data();
	char const* end = txt.data() + txt.length();
	for(; start < end; ++start) {
		if(*start != ' ' && *start != '\t' && *start != '\r' && *start != '\n')
			break;
	}
	for(; start < end; --end) {
		if(*(end - 1) != ' ' && *(end - 1) != '\t' && *(end - 1) != '\r' && *(end - 1) != '\n')
			break;
	}
	return std::string_view(start, end - start);
}

// this was copied from RapidJson (thanks RapidJson)
inline double pow_10(int n) {
	static double const e[] = {// 1e-0...1e308: 309 * 8 bytes = 2472 bytes
			1e+0, 1e+1, 1e+2, 1e+3, 1e+4, 1e+5, 1e+6, 1e+7, 1e+8, 1e+9, 1e+10, 1e+11, 1e+12, 1e+13, 1e+14, 1e+15, 1e+16, 1e+17, 1e+18,
			1e+19, 1e+20, 1e+21, 1e+22, 1e+23, 1e+24, 1e+25, 1e+26, 1e+27, 1e+28, 1e+29, 1e+30, 1e+31, 1e+32, 1e+33, 1e+34, 1e+35,
			1e+36, 1e+37, 1e+38, 1e+39, 1e+40, 1e+41, 1e+42, 1e+43, 1e+44, 1e+45, 1e+46, 1e+47, 1e+48, 1e+49, 1e+50, 1e+51, 1e+52,
			1e+53, 1e+54, 1e+55, 1e+56, 1e+57, 1e+58, 1e+59, 1e+60, 1e+61, 1e+62, 1e+63, 1e+64, 1e+65, 1e+66, 1e+67, 1e+68, 1e+69,
			1e+70, 1e+71, 1e+72, 1e+73, 1e+74, 1e+75, 1e+76, 1e+77, 1e+78, 1e+79, 1e+80, 1e+81, 1e+82, 1e+83, 1e+84, 1e+85, 1e+86,
			1e+87, 1e+88, 1e+89, 1e+90, 1e+91, 1e+92, 1e+93, 1e+94, 1e+95, 1e+96, 1e+97, 1e+98, 1e+99, 1e+100, 1e+101, 1e+102, 1e+103,
			1e+104, 1e+105, 1e+106, 1e+107, 1e+108, 1e+109, 1e+110, 1e+111, 1e+112, 1e+113, 1e+114, 1e+115, 1e+116, 1e+117, 1e+118,
			1e+119, 1e+120, 1e+121, 1e+122, 1e+123, 1e+124, 1e+125, 1e+126, 1e+127, 1e+128, 1e+129, 1e+130, 1e+131, 1e+132, 1e+133,
			1e+134, 1e+135, 1e+136, 1e+137, 1e+138, 1e+139, 1e+140, 1e+141, 1e+142, 1e+143, 1e+144, 1e+145, 1e+146, 1e+147, 1e+148,
			1e+149, 1e+150, 1e+151, 1e+152, 1e+153, 1e+154, 1e+155, 1e+156, 1e+157, 1e+158, 1e+159, 1e+160, 1e+161, 1e+162, 1e+163,
			1e+164, 1e+165, 1e+166, 1e+167, 1e+168, 1e+169, 1e+170, 1e+171, 1e+172, 1e+173, 1e+174, 1e+175, 1e+176, 1e+177, 1e+178,
			1e+179, 1e+180, 1e+181, 1e+182, 1e+183, 1e+184, 1e+185, 1e+186, 1e+187, 1e+188, 1e+189, 1e+190, 1e+191, 1e+192, 1e+193,
			1e+194, 1e+195, 1e+196, 1e+197, 1e+198, 1e+199, 1e+200, 1e+201, 1e+202, 1e+203, 1e+204, 1e+205, 1e+206, 1e+207, 1e+208,
			1e+209, 1e+210, 1e+211, 1e+212, 1e+213, 1e+214, 1e+215, 1e+216, 1e+217, 1e+218, 1e+219, 1e+220, 1e+221, 1e+222, 1e+223,
			1e+224, 1e+225, 1e+226, 1e+227, 1e+228, 1e+229, 1e+230, 1e+231, 1e+232, 1e+233, 1e+234, 1e+235, 1e+236, 1e+237, 1e+238,
			1e+239, 1e+240, 1e+241, 1e+242, 1e+243, 1e+244, 1e+245, 1e+246, 1e+247, 1e+248, 1e+249, 1e+250, 1e+251, 1e+252, 1e+253,
			1e+254, 1e+255, 1e+256, 1e+257, 1e+258, 1e+259, 1e+260, 1e+261, 1e+262, 1e+263, 1e+264, 1e+265, 1e+266, 1e+267, 1e+268,
			1e+269, 1e+270, 1e+271, 1e+272, 1e+273, 1e+274, 1e+275, 1e+276, 1e+277, 1e+278, 1e+279, 1e+280, 1e+281, 1e+282, 1e+283,
			1e+284, 1e+285, 1e+286, 1e+287, 1e+288, 1e+289, 1e+290, 1e+291, 1e+292, 1e+293, 1e+294, 1e+295, 1e+296, 1e+297, 1e+298,
			1e+299, 1e+300, 1e+301, 1e+302, 1e+303, 1e+304, 1e+305, 1e+306, 1e+307, 1e+308 };
	return e[n];
}

bool float_from_chars(char const* start, char const* end, float& float_out) { // returns true on success
	// first read the chars into an int, keeping track of the magnitude
	// multiply by a pow of 10

	int32_t magnitude = 0;
	int64_t accumulated = 0;
	bool after_decimal = false;

	if(start == end) {
		float_out = 0.0f;
		return true;
	}

	bool is_negative = false;
	if(*start == '-') {
		is_negative = true;
		++start;
	} else if(*start == '+') {
		++start;
	}

	for(; start < end; ++start) {
		if(*start >= '0' && *start <= '9') {
			accumulated = accumulated * 10 + (*start - '0');
			magnitude += int32_t(after_decimal);
		} else if(*start == '.') {
			after_decimal = true;
		} else {
			// maybe check for non space and throw an error?
		}
	}
	if(!is_negative) {
		if(magnitude > 0)
			float_out = float(double(accumulated) / pow_10(magnitude));
		else
			float_out = float(accumulated);
	} else {
		if(magnitude > 0)
			float_out = -float(double(accumulated) / pow_10(magnitude));
		else
			float_out = -float(accumulated);
	}
	return true;
}

bool double_from_chars(char const* start, char const* end, double& dbl_out) { // returns true on success
	int32_t magnitude = 0;
	int64_t accumulated = 0;
	bool after_decimal = false;

	if(start == end) {
		dbl_out = 0.0;
		return true;
	}

	bool is_negative = false;
	if(*start == '-') {
		is_negative = true;
		++start;
	} else if(*start == '+') {
		++start;
	}

	for(; start < end; ++start) {
		if(*start >= '0' && *start <= '9') {
			accumulated = accumulated * 10 + (*start - '0');
			magnitude += int32_t(after_decimal);
		} else if(*start == '.') {
			after_decimal = true;
		} else {
		}
	}
	if(!is_negative) {
		if(magnitude > 0)
			dbl_out = double(accumulated) / pow_10(magnitude);
		else
			dbl_out = double(accumulated);
	} else {
		if(magnitude > 0)
			dbl_out = -double(accumulated) / pow_10(magnitude);
		else
			dbl_out = -double(accumulated);
	}
	return true;
}

} // namespace parsers


