#pragma once
#include <vulkify/core/ptr.hpp>
#include <vulkify/graphics/descriptor_set.hpp>

namespace vf {
class Shader;

enum class PolygonMode { eFill, eLine, ePoint };
enum class Topology { eTriangleList, eTriangleStrip, eLineList, eLineStrip, ePointList };

struct RenderState {
	struct Pipeline {
		PolygonMode polygonMode{PolygonMode::eFill};
		Topology topology{Topology::eTriangleList};
		float lineWidth{1.0f};
	};

	Pipeline pipeline{};
	Ptr<DescriptorSet const> descriptorSet{};
};
} // namespace vf
