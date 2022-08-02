#pragma once
#include <ktl/hash_table.hpp>
#include <memory>
#include <span>
#include <string>

namespace vf {
struct SpirV {
	std::string path{};
	std::unique_ptr<std::uint32_t[]> code{};
	std::uint32_t codesize{};

	static SpirV make(std::span<std::byte const> bytes);
	static SpirV load(std::string path);

	static bool is_glsl(char const* path);
	static std::string spirv_path(char const* glsl) { return std::string(glsl) + ".spv"; }

	static bool glslc_available();
	static SpirV compile(char const* glsl, std::string path);
	static SpirV load_or_compile(std::string path);
};
} // namespace vf
