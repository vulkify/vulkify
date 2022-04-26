#pragma once
#include <ktl/hash_table.hpp>
#include <memory>
#include <string>

namespace vf {
struct SpirV {
	std::string path{};
	std::unique_ptr<std::uint32_t[]> bytes{};
	std::uint32_t codesize{};

	static SpirV load(std::string path);

	static bool glslcAvailable();
	static SpirV compile(char const* glsl, std::string path);
};
} // namespace vf
