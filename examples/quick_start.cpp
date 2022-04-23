#include <vulkify/context/context.hpp>
#include <vulkify/instance/vf_instance.hpp>
#include <iostream>

namespace {
void test(vf::UContext ctx) {
	ctx->show();
	while (ctx->isOpen()) {
		auto const poll = ctx->poll();
		for (auto const& event : poll.events) {
			switch (event.type) {
			case vf::EventType::eClose: return;
			case vf::EventType::eMove: {
				auto const& pos = event.get<vf::EventType::eMove>();
				std::cout << "window moved to [" << pos.x << ", " << pos.y << "]\n";
				break;
			}
			default: break;
			}
		}
		for (auto const code : poll.scancodes) { std::cout << static_cast<char>(code) << '\n'; }
	}
}
} // namespace

int main() {
	auto instance = vf::VulkifyInstance::make({});
	if (!instance) { return EXIT_FAILURE; }
	auto context = vf::Context::make({}, std::move(instance.value()));
	if (!context) { return EXIT_FAILURE; }
	test(std::move(context.value()));
}
