#include <vulkify/vulkify.hpp>
#include <iostream>

#include <array>

namespace {
void test(vf::UContext ctx) {
	std::cout << "using GPU: " << ctx->instance().gpu().name << '\n';

	ctx->show();
	auto const clearA = vf::Rgba::make(0xfff000ff);
	auto const clearB = vf::Rgba::make(0x000fffff);

	using InstanceStorage = std::array<vf::DrawInstance, 2>;
	using Mesh2D = vf::InstancedMesh<InstanceStorage>;
	auto mesh = Mesh2D(*ctx, "instanced_mesh");
	auto geo = vf::Geometry::makeQuad(glm::vec2(100.0f));
	// geo.vertices[0].rgba = vf::red_v.normalize();
	// geo.vertices[1].rgba = vf::green_v.normalize();
	// geo.vertices[2].rgba = vf::blue_v.normalize();
	mesh.gbo.write(std::move(geo));

	mesh.instances[0].transform.position = {-100.0f, 100.0f};
	mesh.instances[0].tint = vf::Rgba::make(0xc73a58ff).linear();
	mesh.instances[1].transform.position = mesh.instances[0].transform.position + glm::vec2(200.0f, 0.0f);

	auto bmp = vf::Bitmap(vf::cyan_v, {2, 2});
	bmp[{0, 0}] = vf::red_v;
	bmp[{0, 1}] = vf::green_v;
	bmp[{1, 0}] = vf::blue_v;
	auto image = bmp.image();
	auto tex0 = vf::Texture(ctx->vram(), "test_tex", image);
	mesh.texture = tex0.clone("tex_clone");
	// mesh.texture = std::move(tex0);
	// mesh.texture = vf::Texture(ctx->vram(), "bad", {});
	{
		auto bmp = vf::Bitmap::View{{&vf::magenta_v, 1}};
		image = bmp.image();
		mesh.texture.overwrite(image, {1, 1});
	}

	auto tri = vf::Geometry{};
	tri.vertices.push_back(vf::Vertex{{-100.0f, -100.0f}});
	tri.vertices.push_back(vf::Vertex{{100.0f, -100.0f}});
	tri.vertices.push_back(vf::Vertex{{0.0f, 100.0f}});
	auto mesh1 = vf::Mesh(*ctx, "mesh1");
	mesh1.gbo.write(std::move(tri));
	mesh1.instance.transform.position = {200.0f, -200.0f};
	mesh1.instance.tint = vf::yellow_v;

	auto quad = vf::QuadShape(*ctx);
	quad.transform().position.y = -200.0f;
	// quad.tint() = vf::Rgba::make(0xc382bf).linear();
	tex0.rescale(10.0f);
	quad.setTexture(std::move(tex0), true);

	auto elapsed = vf::Time{};
	while (!ctx->closing()) {
		auto const frame = ctx->frame();
		for (auto const& event : frame.poll().events) {
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
		for (auto const code : frame.poll().scancodes) { std::cout << static_cast<char>(code) << '\n'; }

		elapsed += frame.dt();
		mesh.instances[0].transform.orientation.rotate(vf::Degree{frame.dt().count() * 180.0f});
		mesh.instances[1].transform.orientation = mesh.instances[0].transform.orientation;

		// auto spec = vf::PipelineState{};
		// spec.flags.set(vf::PipelineState::Flag::eWireframe);
		// frame.surface.bind(spec);
		frame.draw(mesh);
		frame.draw(mesh1);
		frame.draw(quad);
		auto const clear = vf::Rgba::lerp(clearA, clearB, (std::sin(elapsed.count()) + 1.0f) * 0.5f);
		frame.setClear(clear.linear());
	}
}
} // namespace

int main() {
	// auto context = vf::Builder{}.setFlag(vf::Builder::Flag::eLinearSwapchain).build();
	auto context = vf::Builder{}.build();
	// auto context = vf::Builder{}.setFlag(vf::Builder::Flag::eHeadless).build();
	if (!context) { return EXIT_FAILURE; }
	test(std::move(context.value()));
}
