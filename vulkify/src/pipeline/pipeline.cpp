#include <detail/trace.hpp>
#include <vulkify/pipeline/pipeline.hpp>
#include <fstream>

namespace vf {
namespace {
constexpr std::string_view redirect_null_v =
#if defined(_WIN32)
	"nul";
#else
	"/dev/null";
#endif
} // namespace

bool SpirV::glslcAvailable() {
	auto const str = ktl::str_format("glslc --version > {} 2>&1", redirect_null_v);
	auto const ret = std::system(str.data());
	return ret == 0;
}

SpirV SpirV::load(std::string path) {
	if (path.empty()) {
		VF_TRACE("Cannot load empty Spir-V path");
		return {};
	}
	auto ret = SpirV{std::move(path)};
	auto file = std::ifstream(ret.path, std::ios::binary | std::ios::ate);
	if (!file) {
		VF_TRACEF("Failed to open Spir-V: {}", ret.path);
		return {};
	}

	auto const ssize = file.tellg();
	if (ssize <= 0 || ssize % 4 != 0) {
		VF_TRACEF("Invalid Spir-V size: {}", ssize);
		return {};
	}
	file.seekg({});

	ret.codesize = static_cast<std::uint32_t>(ssize);
	ret.bytes = std::make_unique<std::uint32_t[]>(static_cast<std::size_t>(ssize / 4));
	file.read(reinterpret_cast<char*>(ret.bytes.get()), ssize);
	VF_TRACEF("Spir-V [{}] loaded successfully (codesize: {})", ret.path, ret.codesize);

	return ret;
}

SpirV SpirV::compile(char const* glsl, std::string path) {
	if (!glslcAvailable()) {
		VF_TRACE("glslc not available");
		return {};
	}
	if (!glsl || !*glsl || path.empty()) {
		VF_TRACE("Empty path");
		return {};
	}
	auto file = std::ofstream(path);
	if (!file) {
		VF_TRACEF("Failed to open file for write: [{}]", path);
		return {};
	}
	auto str = ktl::str_format("glslc {} -o {}", glsl, path);
	if (std::system(str.data()) != 0) {
		VF_TRACEF("Failed to compile glsl [{}] to Spir-V", glsl);
		return {};
	}
	VF_TRACEF("Glsl [{}] compiled to Spir-V [{}] successfully", glsl, path);
	return load(std::move(path));
}
} // namespace vf
