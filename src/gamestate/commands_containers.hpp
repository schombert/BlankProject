#pragma once


namespace command {
enum class command_type : uint8_t;

// padding due to alignment
struct cmd_header {
	uint32_t payload_size = 0;
	command_type type;
};

struct command_data {
	cmd_header header{};
	std::vector<uint8_t> payload;
	command_data() {
	};
	command_data(command_type _type) {
		header.type = _type;
	};
	// add data to the payload
	template<typename data_type>
	friend command_data& operator << (command_data& msg, data_type& data) {

		static_assert(std::is_standard_layout<data_type>::value, "Data type is too complex");
		size_t curr_size = msg.payload.size();
		msg.payload.resize(msg.payload.size() + sizeof(data_type));

		std::memcpy(msg.payload.data() + curr_size, &data, sizeof(data_type));

		msg.header.payload_size = (uint32_t)msg.payload.size();

		return msg;
	}
	// adds data from pointer to the payload
	template<typename data_type>
	void push_ptr(data_type* ptr, size_t size) {
		size_t curr_size = payload.size();
		payload.resize(payload.size() + sizeof(data_type) * size);

		std::memcpy(payload.data() + curr_size, ptr, sizeof(data_type) * size);

		header.payload_size = (uint32_t)payload.size();
	}


	// grab data from the payload
	template<typename data_type>
	friend command_data& operator >> (command_data& msg, data_type& data) {

		static_assert(std::is_standard_layout<data_type>::value, "Data type is too complex");

		size_t i = msg.payload.size() - sizeof(data_type);
		std::memcpy(&data, msg.payload.data() + i, sizeof(data_type));
		msg.payload.resize(i);

		msg.header.payload_size = (uint32_t)msg.payload.size();

		return msg;



	}
	// Makes a copy of the data and returns it
	/*template<typename data_type>
	data_type copy_payload() const {

		static_assert(std::is_standard_layout<data_type>::value, "Data type is too complex");
		static_assert(sizeof(data_type) <= MAX_PAYLOAD_SIZE, "data type used is larger than MAX_PAYLOAD_SIZE. Did you forget to add it?");

		data_type output{ };
		std::memcpy(&output, payload.data() + (payload.size() - sizeof(data_type)), sizeof(data_type));
		return output;
	}*/
	// returns a reference to the payload of the desired type, starting from the start of the vector
	template<typename data_type>
	data_type& get_payload() {
		static_assert(std::is_standard_layout<data_type>::value, "Data type is too complex");
		uint8_t* ptr = payload.data();
		return reinterpret_cast<data_type&>(*ptr);
	}
	// Checks if the payload of the given type has an additional variable payload of size "expected_size" (in bytes). Returns true if that is the case, false otherwise
	template<typename data_type>
	bool check_variable_size_payload(uint32_t expected_size) {
		return expected_size == (payload.size() - sizeof(data_type));
	}

};
static_assert(sizeof(command_data) == sizeof(command_data::header) + sizeof(command_data::payload));

}

namespace event {


}
