#pragma once
#include <ktl/kunique_ptr.hpp>
#include <vulkify/core/rgba.hpp>
#include <vulkify/instance/event.hpp>
#include <vulkify/instance/gpu.hpp>
#include <span>

namespace vf {
class Instance {
  public:
	static constexpr std::size_t max_events_v = 16;
	static constexpr std::size_t max_scancodes_v = 16;

	struct Poll {
		std::span<Event const> events{};
		std::span<std::uint32_t const> scancodes{};
		std::span<std::string const> fileDrops{};
	};

	virtual ~Instance() = default;

	virtual GPU const& gpu() const = 0;
	virtual glm::ivec2 framebufferSize() const = 0;
	virtual glm::ivec2 windowSize() const = 0;

	virtual bool closing() const = 0;
	virtual void show() = 0;
	virtual void hide() = 0;
	virtual void close() = 0;
	virtual Poll poll() = 0;

	virtual bool beginPass() = 0;
	virtual bool endPass(Rgba clear) = 0;
};

using UInstance = ktl::kunique_ptr<Instance>;
} // namespace vf
