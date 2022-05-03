#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <ktl/kvariant.hpp>
#include <vector>

namespace vf {
struct VideoMode {
	glm::uvec2 extent{};
	glm::uvec3 bitDepth{};
	std::uint32_t refreshRate{};
};

struct Monitor {
	using Handle = ktl::fixed_any<8>;

	VideoMode current{};
	std::vector<VideoMode> supported{};
	Handle handle{};

	bool operator==(Monitor const& rhs) const { return handle.data() == rhs.handle.data(); }
};

struct MonitorList {
	Monitor primary{};
	std::vector<Monitor> others{};
};

struct Display {
	struct Window {
		glm::uvec2 extent{};
		glm::ivec2 position{};
	};
	struct Fullscreen {
		glm::uvec2 resolution{};
		std::uint32_t refreshRate{};
		Monitor::Handle monitor{};
	};
	struct Borderless {
		Monitor::Handle monitor{};
	};

	ktl::kvariant<Window, Fullscreen, Borderless> mode{};
};
} // namespace vf
