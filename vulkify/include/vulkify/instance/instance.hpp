#pragma once
#include <ktl/kunique_ptr.hpp>
#include <vulkify/core/rect.hpp>
#include <vulkify/core/view.hpp>
#include <vulkify/graphics/bitmap.hpp>
#include <vulkify/graphics/surface.hpp>
#include <vulkify/instance/event.hpp>
#include <vulkify/instance/gpu.hpp>
#include <vulkify/instance/instance_enums.hpp>
#include <vulkify/instance/monitor.hpp>
#include <span>

namespace vf {
struct Vram;

enum class CursorMode { eStandard, eHidden, eDisabled };

struct Cursor {
	std::uint64_t handle{};

	constexpr bool operator==(Cursor const&) const = default;
};

class Instance {
  public:
	static constexpr std::size_t max_events_v = 16;
	static constexpr std::size_t max_scancodes_v = 16;

	struct Icon {
		using TopLeft = glm::ivec2;

		Bitmap::View bitmap{};
		TopLeft hotspot{};
	};

	struct Poll {
		std::span<Event const> events{};
		std::span<std::uint32_t const> scancodes{};
		std::span<std::string const> fileDrops{};
	};

	virtual ~Instance() = default;

	virtual Vram const& vram() const = 0;
	virtual Gpu const& gpu() const = 0;
	virtual bool closing() const = 0;
	virtual glm::uvec2 framebufferSize() const = 0;
	virtual glm::uvec2 windowSize() const = 0;
	virtual glm::ivec2 position() const = 0;
	virtual glm::vec2 contentScale() const = 0;
	virtual CursorMode cursorMode() const = 0;
	virtual glm::vec2 cursorPosition() const = 0;
	virtual MonitorList monitors() const = 0;
	virtual WindowFlags windowFlags() const = 0;
	virtual View& view() const = 0;
	virtual Rect& viewport() const = 0;
	virtual AntiAliasing antiAliasing() const = 0;

	virtual void show() = 0;
	virtual void hide() = 0;
	virtual void close() = 0;
	virtual void setPosition(glm::ivec2 xy) = 0;
	virtual void setSize(glm::uvec2 size) = 0;
	virtual void setIcons(std::span<Icon const> icons) = 0;
	virtual void setCursorMode(CursorMode mode) = 0;
	virtual Cursor makeCursor(Icon icon) = 0;
	virtual void destroyCursor(Cursor cursor) = 0;
	virtual bool setCursor(Cursor cursor) = 0;
	virtual void setWindowed(glm::uvec2 extent) = 0;
	virtual void setFullscreen(Monitor const& monitor, glm::uvec2 resolution = {}) = 0;
	virtual void updateWindowFlags(WindowFlags set, WindowFlags unset = {}) = 0;

	virtual Poll poll() = 0;
	virtual Surface beginPass(Rgba clear) = 0;
	virtual bool endPass() = 0;
};

using UInstance = ktl::kunique_ptr<Instance>;
} // namespace vf
