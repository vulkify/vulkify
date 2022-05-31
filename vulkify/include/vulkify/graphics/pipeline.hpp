#pragma once
#include <vulkify/core/ptr.hpp>
#include <vulkify/graphics/handles.hpp>

namespace vf {
enum class PolygonMode { eFill, eLine, ePoint };
enum class Topology { eTriangleList, eTriangleStrip, eLineList, eLineStrip, ePointList };

struct Pipeline {
	struct State {
		PolygonMode polygonMode{PolygonMode::eFill};
		Topology topology{Topology::eTriangleList};
		float lineWidth{1.0f};
	};

	State state{};
	ShaderHandle shader{};
};
} // namespace vf
