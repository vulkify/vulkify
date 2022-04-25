#include <filesystem>
#include <fstream>
#include <iostream>

namespace stdfs = std::filesystem;
using namespace std::string_view_literals;

namespace {
struct Doc {
	std::ostream& out;
	Doc(std::string_view const name, std::ostream& out) : out(out) {
		out << "#pragma once\n\n";
		out << "namespace vf {\n";
		out << "constexpr unsigned char " << name << "[] = {\n\t";
	}
	~Doc() { out << "};\n} // namespace vf\n"; }
};

bool burn(char const* const source, [[maybe_unused]] std::string_view const name, std::ostream& out) {
	auto in = std::ifstream(source, std::ios::binary);
	if (!in) {
		std::cerr << "[shabu] Failed to open source file [" << source << "]\n";
		return false;
	}
	out << std::hex << std::setfill('0');

	auto ch = char{};
	auto column = int{};
	auto doc = Doc(name, out);
	while (in.get(ch)) {
		auto const byte = static_cast<std::uint32_t>(static_cast<unsigned char>(ch));
		out << "0x" << std::setw(2) << byte << ", ";
		if (++column >= 16) {
			out << "\n\t";
			column = 0;
		}
	}
	out << std::endl;
	return true;
}
} // namespace

int main(int argc, char** argv) {
	if (argc < 3) {
		std::cerr << "[shabu] Invalid arguments\nUsage: shabu <source> <name> [target]\n";
		return EXIT_FAILURE;
	}

	auto const source = std::string_view(argv[1]);
	auto const name = std::string_view(argv[2]);

	std::ostream* out = &std::cout;
	auto file = std::ofstream{};
	if (argc > 3) {
		file.open(argv[3]);
		if (!file) {
			std::cerr << "[shabu] Failed to open target file [" << argv[2] << "]\n";
			return EXIT_FAILURE;
		}
		out = &file;
	}
	if (!burn(source.data(), name, *out)) {
		std::cerr << "[shabu] Failed to burn [" << source << "]\n";
		return EXIT_FAILURE;
	}
}
