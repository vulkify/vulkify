#include <detail/spir_v.hpp>
#include <detail/trace.hpp>
#include <ktl/str_format.hpp>
#include <filesystem>
#include <fstream>

namespace vf {
namespace stdfs = std::filesystem;

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

bool SpirV::isGlsl(char const* path) {
	auto p = stdfs::path(path);
	auto const ext = p.extension().string();
	return ext == ".vert" || ext == ".frag";
}

SpirV SpirV::make(std::span<std::byte const> bytes) {
	auto ret = SpirV{};
	if (bytes.size() % 4 != 0) {
		VF_TRACEF("[vf::SpirV] Invalid SPIR-V codesize: {}", bytes.size());
		return {};
	}
	ret.code = std::make_unique<std::uint32_t[]>(bytes.size() / 4);
	std::memcpy(ret.code.get(), bytes.data(), bytes.size());
	ret.codesize = static_cast<std::uint32_t>(bytes.size());
	return ret;
}

SpirV SpirV::load(std::string path) {
	if (path.empty()) {
		VF_TRACE("[vf::SpirV] Cannot load empty Spir-V path");
		return {};
	}
	auto ret = SpirV{std::move(path)};
	auto file = std::ifstream(ret.path, std::ios::binary | std::ios::ate);
	if (!file) {
		VF_TRACEF("[vf::SpirV] Failed to open Spir-V: {}", ret.path);
		return {};
	}

	auto const ssize = file.tellg();
	if (ssize <= 0 || ssize % 4 != 0) {
		VF_TRACEF("[vf::SpirV] Invalid Spir-V size: {}", ssize);
		return {};
	}
	file.seekg({});

	ret.codesize = static_cast<std::uint32_t>(ssize);
	ret.code = std::make_unique<std::uint32_t[]>(static_cast<std::size_t>(ssize / 4));
	file.read(reinterpret_cast<char*>(ret.code.get()), ssize);
	VF_TRACEF("[vf::SpirV] [{}] loaded successfully (codesize: {})", ret.path, ret.codesize);

	return ret;
}

SpirV SpirV::compile(char const* glsl, std::string path) {
	if (!glslcAvailable()) {
		VF_TRACE("[vf::SpirV] glslc not available");
		return {};
	}
	if (!glsl || !*glsl || path.empty()) {
		VF_TRACE("[vf::SpirV] Empty path");
		return {};
	}
	auto file = std::ofstream(path);
	if (!file) {
		VF_TRACEF("[vf::SpirV] Failed to open file for write: [{}]", path);
		return {};
	}
	auto str = ktl::str_format("glslc {} -o {}", glsl, path);
	if (std::system(str.data()) != 0) {
		VF_TRACEF("[vf::SpirV] Failed to compile glsl [{}] to Spir-V", glsl);
		return {};
	}
	VF_TRACEF("[vf::SpirV] Glsl [{}] compiled to Spir-V [{}] successfully", glsl, path);
	return load(std::move(path));
}

SpirV SpirV::loadOrCompile(std::string path) {
	if (isGlsl(path.c_str())) {
		static bool s_glslc = glslcAvailable();
		auto spv = std::string{};
		if (s_glslc) {
			// try-compile Glsl to Spir-V
			spv = spirvPath(path.c_str());
			auto ret = SpirV::compile(path.c_str(), spv);
			if (ret.code) { return ret; }
		}
		// search for pre-compiled Spir-V
		path = spv.empty() ? spirvPath(path.c_str()) : std::move(spv);
		if (!stdfs::is_regular_file(path)) { return {}; }
	}
	// load Spir-V
	auto ret = SpirV::load(path);
	if (!ret.code) { return {}; }
	return ret;
}
} // namespace vf
