#pragma once
#include <vulkify/core/ptr.hpp>

namespace vf {
class Shader;

enum class PolygonMode { eFill, eLine, ePoint };
enum class Topology { eTriangleList, eTriangleStrip, eLineList, eLineStrip, ePointList };

struct Pipeline {
	struct State {
		PolygonMode polygonMode{PolygonMode::eFill};
		Topology topology{Topology::eTriangleList};
		float lineWidth{1.0f};
	};

	State state{};
	Ptr<Shader const> shader{};
};
} // namespace vf
