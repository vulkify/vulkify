#pragma once
#include <vulkify/core/ptr.hpp>
#include <vulkify/graphics/z_order.hpp>
#include <optional>

namespace vf {
class DescriptorSet;

enum class PolygonMode { eFill, eLine, ePoint };
enum class Topology { eTriangleList, eTriangleStrip, eLineList, eLineStrip, ePointList };

struct RenderState {
	PolygonMode polygon_mode{PolygonMode::eFill};
	Topology topology{Topology::eTriangleList};
	float line_width{1.0f};
	std::optional<ZOrder> force_z_order{};
	Ptr<DescriptorSet const> descriptor_set{};
};
} // namespace vf
