#pragma once

namespace vf {
struct DeferBase {
	static constexpr int default_delay_v = 3;

	int delay;
	DeferBase(int delay = default_delay_v) : delay(delay) {}
	virtual ~DeferBase() = default;
};
} // namespace vf
