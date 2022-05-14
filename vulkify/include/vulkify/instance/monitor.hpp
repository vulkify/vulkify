#pragma once
#include <vulkify/instance/video_mode.hpp>
#include <string>
#include <vector>

namespace vf {
struct Monitor {
	using Handle = void*;

	std::string name{};
	VideoMode current{};
	std::vector<VideoMode> supported{};
	glm::ivec2 position{};
	Handle handle{};

	bool operator==(Monitor const& rhs) const { return handle == rhs.handle; }
};

///
/// \brief List of connected Monitors
///
struct MonitorList {
	Monitor primary{};
	std::vector<Monitor> others{};
};
} // namespace vf
