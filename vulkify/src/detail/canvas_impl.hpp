#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkify/context/canvas.hpp>

namespace vf {
struct Canvas::Impl {
	vk::CommandBuffer commandBuffer{};
};
} // namespace vf
