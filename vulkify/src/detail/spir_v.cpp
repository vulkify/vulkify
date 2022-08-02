#include <detail/spir_v.hpp>
#include <detail/trace.hpp>
#include <ktl/kformat.hpp>
#include <filesystem>
#include <fstream>

namespace vf {
namespace stdfs = std::filesystem;

[[maybe_unused]] static constexpr auto name_v = "vf::SpirV";

namespace {
constexpr std::string_view redirect_null_v =
#if defined(_WIN32)
	"nul";
#else
	"/dev/null";
#endif
} // namespace

bool SpirV::glslc_available() {
	auto const str = ktl::kformat("glslc --version > {} 2>&1", redirect_null_v);
	auto const ret = std::system(str.data());
	return ret == 0;
}

bool SpirV::is_glsl(char const* path) {
	auto p = stdfs::path(path);
	auto const ext = p.extension().string();
	return ext == ".vert" || ext == ".frag";
}

SpirV SpirV::make(std::span<std::byte const> bytes) {
	auto ret = SpirV{};
	if (bytes.size() % 4 != 0) {
		VF_TRACEI(name_v, "Invalid SPIR-V codesize: {}", bytes.size());
		return {};
	}
	ret.code = std::make_unique<std::uint32_t[]>(bytes.size() / 4);
	std::memcpy(ret.code.get(), bytes.data(), bytes.size());
	ret.codesize = static_cast<std::uint32_t>(bytes.size());
	return ret;
}

SpirV SpirV::load(std::string path) {
	if (path.empty()) {
		VF_TRACE(name_v, trace::Type::eWarn, "Cannot load empty Spir-V path");
		return {};
	}
	auto ret = SpirV{std::move(path)};
	auto file = std::ifstream(ret.path, std::ios::binary | std::ios::ate);
	if (!file) {
		VF_TRACEW(name_v, "Failed to open Spir-V: {}", ret.path);
		return {};
	}

	auto const ssize = file.tellg();
	if (ssize <= 0 || ssize % 4 != 0) {
		VF_TRACEW(name_v, "Invalid Spir-V [{}] size: {}", ret.path, ssize);
		return {};
	}
	file.seekg({});

	ret.codesize = static_cast<std::uint32_t>(ssize);
	ret.code = std::make_unique<std::uint32_t[]>(static_cast<std::size_t>(ssize / 4));
	file.read(reinterpret_cast<char*>(ret.code.get()), ssize);

	return ret;
}

SpirV SpirV::compile(char const* glsl, std::string path) {
	if (!glslc_available()) {
		VF_TRACE(name_v, trace::Type::eWarn, "glslc not available");
		return {};
	}
	if (!glsl || !*glsl || path.empty()) {
		VF_TRACE(name_v, trace::Type::eWarn, "Empty path");
		return {};
	}
	if (!stdfs::is_regular_file(glsl)) {
		VF_TRACEW(name_v, "Failed to open glsl file: [{}]", glsl);
		return {};
	}
	auto file = std::ofstream(path);
	if (!file) {
		VF_TRACEW(name_v, "Failed to open file for write: [{}]", path);
		return {};
	}
	auto str = ktl::kformat("glslc {} -o {}", glsl, path);
	if (std::system(str.data()) != 0) {
		VF_TRACEW(name_v, "Failed to compile glsl [{}] to Spir-V", glsl);
		return {};
	}
	VF_TRACEI(name_v, "Glsl [{}] compiled to Spir-V [{}] successfully", glsl, path);
	return load(std::move(path));
}

SpirV SpirV::load_or_compile(std::string path) {
	if (is_glsl(path.c_str())) {
		static bool s_glslc = glslc_available();
		auto spv = std::string{};
		if (s_glslc) {
			// try-compile Glsl to Spir-V
			spv = spirv_path(path.c_str());
			auto ret = SpirV::compile(path.c_str(), spv);
			if (ret.code) { return ret; }
		}
		// search for pre-compiled Spir-V
		path = spv.empty() ? spirv_path(path.c_str()) : std::move(spv);
		if (!stdfs::is_regular_file(path)) { return {}; }
	}
	// load Spir-V
	auto ret = SpirV::load(path);
	if (!ret.code) { return {}; }
	return ret;
}
} // namespace vf
