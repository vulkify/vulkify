#include <vulkify/context/context.hpp>
#include <vulkify/instance/headless_instance.hpp>
#include <vulkify/instance/vf_instance.hpp>
#include <cmath>
#include <iostream>

namespace {
void test(vf::UContext ctx) {
	std::cout << "using GPU: " << ctx->instance().gpu().name << '\n';
	ctx->show();
	auto const clearA = vf::Rgba::make(0xfff000ff);
	auto const clearB = vf::Rgba::make(0x000fffff);
	auto elapsed = vf::Time{};
	while (!ctx->closing()) {
		auto const frame = ctx->nextFrame();
		for (auto const& event : frame.poll.events) {
			switch (event.type) {
			case vf::EventType::eClose: return;
			case vf::EventType::eMove: {
				auto const& pos = event.get<vf::EventType::eMove>();
				std::cout << "window moved to [" << pos.x << ", " << pos.y << "]\n";
				break;
			}
			case vf::EventType::eKey: {
				auto const& key = event.get<vf::EventType::eKey>();
				if (key.key == vf::Key::eW && key.action == vf::Action::ePress && key.mods.test(vf::Mod::eCtrl)) { ctx->close(); }
				break;
			}
			default: break;
			}
		}
		for (auto const code : frame.poll.scancodes) { std::cout << static_cast<char>(code) << '\n'; }

		elapsed += frame.dt;
		auto const clear = vf::Rgba::lerp(clearA, clearB, (std::sin(elapsed.count()) + 1.0f) * 0.5f);
		ctx->submit(clear.linear());
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
