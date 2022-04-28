#include <vulkify/context/context.hpp>
#include <vulkify/instance/headless_instance.hpp>
#include <vulkify/instance/vf_instance.hpp>
#include <cmath>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/spir_v.hpp>

#include <vulkify/graphics/drawable.hpp>

namespace {
void test(vf::UContext ctx) {
	bool const glslc = vf::SpirV::glslcAvailable();
	std::cout << "glslc available: " << std::boolalpha << glslc << '\n';
	std::cout << "using GPU: " << ctx->instance().gpu().name << '\n';

	ctx->show();
	auto const clearA = vf::Rgba::make(0xfff000ff);
	auto const clearB = vf::Rgba::make(0x000fffff);

	auto quad = vf::Drawable(ctx->vram(), "test_quad");
	auto geo = vf::makeQuad(glm::vec2(100.0f));
	geo.vertices[0].rgba = vf::red_v.normalize();
	geo.vertices[1].rgba = vf::green_v.normalize();
	geo.vertices[2].rgba = vf::blue_v.normalize();
	quad.setGeometry(std::move(geo));

	auto elapsed = vf::Time{};
	quad.transform().position = {-100.0f, 100.0f};
	while (!ctx->closing()) {
		auto const frame = ctx->frame();
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
		quad.transform().rotation = vf::Radian{elapsed.count()};
		quad.tint() = vf::magenta_v;

		if (frame.surface.bind({})) { quad.draw(frame.surface); }
		auto const clear = vf::Rgba::lerp(clearA, clearB, (std::sin(elapsed.count()) + 1.0f) * 0.5f);
		frame.surface.setClear(clear.linear());
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
