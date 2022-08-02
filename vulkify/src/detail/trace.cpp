#include <detail/trace.hpp>
#include <ktl/kformat.hpp>
#include <cstdio>
#include <ctime>

namespace vf {
namespace {
constexpr char prefix_char(trace::Type type) {
	switch (type) {
	case trace::Type::eError: return 'E';
	case trace::Type::eWarn: return 'W';
	default:
	case trace::Type::eInfo: return 'I';
	}
}

void add_timestamp(std::string& out) {
	std::time_t time = std::time(nullptr);
	char buf[16]{};
	std::strftime(buf, sizeof(buf), " [%H:%M:%S]", std::localtime(&time));
	out += buf;
}
} // namespace

void trace::log(Payload payload) {
	payload.message = ktl::kformat("[{}] [{}] {}", prefix_char(payload.type), payload.prefix, payload.message);
	add_timestamp(payload.message);
	if (payload.type == Type::eError) {
		std::fprintf(stderr, "%s\n", payload.message.c_str());
	} else {
		std::fprintf(stdout, "%s\n", payload.message.c_str());
	}
}
} // namespace vf
