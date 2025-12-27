#pragma once

namespace sys {
struct state;
}

namespace rng {

struct random_pair {
	uint64_t high;
	uint64_t low;
};

uint64_t get_random(sys::state const& state, uint32_t value_in_hi, uint32_t value_in_lo);
random_pair get_random_pair(sys::state const& state, uint32_t value_in_hi, uint32_t value_in_lo);
uint32_t reduce(uint32_t value_in, uint32_t upper_bound);

} // namespace rng
