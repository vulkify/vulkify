#pragma once
#include <vulkify/instance/event.hpp>
#include <span>
#include <string>

namespace vf {
struct EventQueue {
	std::span<Event const> events{};
	std::span<std::uint32_t const> scancodes{};
	std::span<std::string const> file_drops{};
};
} // namespace vf
