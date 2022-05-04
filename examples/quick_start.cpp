#include <vulkify/vulkify.hpp>
#include <iostream>

#include <ktl/enumerate.hpp>
#include <array>

namespace {
class StarShape : public vf::Shape {
  public:
	StarShape() = default;
	StarShape(vf::Context const& context, std::string name, float diameter) : Shape(context, std::move(name)) { set(diameter); }

	void set(float diameter) {
		auto geo = vf::Geometry{};
		if (diameter <= 0.0f) { return; }
		auto const radius = diameter * 0.5f;
		auto const colour = vf::white_v.normalize();
		auto makeTri = [&](vf::nvec2 dir) {
			auto const a = static_cast<glm::vec2>(dir) * radius;
			dir.rotate(vf::Degree{120.0f});
			auto const b = static_cast<glm::vec2>(dir) * radius;
			dir.rotate(vf::Degree{120.0f});
			auto const c = static_cast<glm::vec2>(dir) * radius;
			geo.vertices.push_back({a, {}, colour});
			geo.vertices.push_back({b, {}, colour});
			geo.vertices.push_back({c, {}, colour});
		};

		auto dir = vf::nvec2{0.0f, 1.0f};
		makeTri(dir);
		dir = vf::nvec2{0.0f, -1.0f};
		makeTri(dir);
		m_gbo.write(std::move(geo));
	}
};

vf::Geometry makeStar(float diameter) {
	auto ret = vf::Geometry{};
	auto const radius = diameter * 0.5f;
	auto const colour = vf::white_v.normalize();
	auto makeTri = [&](vf::nvec2 dir) {
		ret.vertices.push_back({static_cast<glm::vec2>(dir) * radius, {}, colour});
		dir.rotate(vf::Degree{120.0f});
		ret.vertices.push_back({static_cast<glm::vec2>(dir) * radius, {}, colour});
		dir.rotate(vf::Degree{120.0f});
		ret.vertices.push_back({static_cast<glm::vec2>(dir) * radius, {}, colour});
	};

	auto dir = vf::nvec2{0.0f, 1.0f};
	makeTri(dir);
	dir = vf::nvec2{0.0f, -1.0f};
	makeTri(dir);
	return ret;
}

void interpolateRgba(std::span<vf::Vertex> vertices, vf::Rgba from, vf::Rgba to) {
	for (auto [vertex, i] : ktl::enumerate(vertices)) {
		auto const t = static_cast<float>(i) / static_cast<float>(vertices.size());
		vertex.rgba = vf::Rgba::lerp(from, to, t).normalize();
	}
}
void test(vf::UContext ctx) {
	std::cout << "using GPU: " << ctx->gpu().name << '\n';

	static constexpr auto padding_v = glm::vec2(200.0f, 100.0f);

	auto const viewRect = ctx->space().rect;

	// triangle
	auto geometry = vf::Geometry{};
	geometry.vertices.push_back(vf::Vertex{{-50.0f, -50.0f}});
	geometry.vertices.push_back(vf::Vertex{{50.0f, -50.0f}});
	geometry.vertices.push_back(vf::Vertex{{0.0f, 50.0f}});
	auto triangle = vf::Mesh(*ctx, "triangle");
	triangle.gbo.write(std::move(geometry));
	triangle.instance.transform.position = viewRect.topLeft() + glm::vec2(padding_v.x, -padding_v.y);

	// RGBW texture
	auto bitmap = vf::Bitmap(vf::white_v, {2, 2});
	bitmap[vf::Index2D{0, 0}] = vf::red_v;
	bitmap[vf::Index2D{0, 1}] = vf::green_v;
	bitmap[vf::Index2D{1, 0}] = vf::blue_v;
	auto rgbTexture = vf::Texture(*ctx, "rgb_texture", bitmap.image());

	// RGB quad
	auto quadCreateInfo = vf::QuadShape::CreateInfo{glm::vec2(100.0f, 100.0f)};
	auto rgbQuad = vf::QuadShape(*ctx, "rgb_quad", quadCreateInfo);
	rgbQuad.setTexture(rgbTexture.clone("rgb_texture_clone"), false);
	rgbQuad.transform().position = viewRect.topRight() + glm::vec2(-padding_v.x, -padding_v.y);

	// pentagon
	auto pentagon = vf::Mesh(*ctx, "pentagon");
	pentagon.gbo.write(vf::Geometry::makeRegularPolygon(100.0f, 6));
	bitmap = vf::Bitmap(vf::magenta_v);
	rgbTexture.overwrite(bitmap.image(), vf::Texture::TopLeft{1, 1});
	pentagon.texture = std::move(rgbTexture);
	pentagon.instance.transform.position = viewRect.bottomLeft() + glm::vec2(padding_v.x, padding_v.y);

	// circle
	auto circle = vf::Mesh(*ctx, "circle");
	geometry = vf::Geometry::makeRegularPolygon(100.0f, 64);
	auto const circleRgbaStart = vf::yellow_v;
	auto const circleRgbaEnd = vf::cyan_v;
	auto span = std::span<vf::Vertex>(geometry.vertices).subspan(1); // exclude centre
	interpolateRgba(span.subspan(0, span.size() / 2), circleRgbaStart, circleRgbaEnd);
	interpolateRgba(span.subspan(span.size() / 2), circleRgbaEnd, circleRgbaStart);
	circle.gbo.write(std::move(geometry));
	circle.instance.transform.position = viewRect.bottomRight() + glm::vec2(-padding_v.x, padding_v.y);
	auto iris = vf::CircleShape(*ctx, "iris", vf::CircleShape::CreateInfo{50.0f});
	iris.transform().position = circle.instance.transform.position;
	iris.tint() = vf::black_v;

	// image
	auto image = vf::Image{};
	auto const imagePath = "test_image.png";
	auto loadResult = image.load(imagePath);
	if (loadResult) { std::cout << imagePath << " [" << loadResult->x << 'x' << loadResult->y << "] loaded sucessfully\n"; }

	// textured quad
	quadCreateInfo.size = {200.0f, 200.0f};
	auto texturedQuad = vf::QuadShape(*ctx, "textured_quad", quadCreateInfo);
	texturedQuad.setTexture(vf::Texture(*ctx, "texture", image), false); // should be magenta if image is bad

	// stars
	using InstanceStorage = std::array<vf::DrawInstance, 4>;
	using InstancedMesh = vf::InstancedMesh<InstanceStorage>;
	auto stars = InstancedMesh(*ctx, "stars");
	stars.gbo.write(makeStar(100.0f));
	stars.instances[0].transform.position = {0.0f, +(viewRect.extent.y * 0.5f - padding_v.y)};
	stars.instances[1].transform.position = {+(viewRect.extent.x * 0.5f - padding_v.x), 0.0f};
	stars.instances[2].transform.position = {0.0f, -(viewRect.extent.y * 0.5f - padding_v.y)};
	stars.instances[3].transform.position = {-(viewRect.extent.x * 0.5f - padding_v.x), 0.0f};
	float starDScale[4]{-1.0f, -0.5f, 0.25f, 0.75f};
	float starDRot[4]{1.0f, -2.0f, 5.0f, 3.0f};

	vf::Primitive const* primitives[] = {&triangle, &rgbQuad, &pentagon, &circle, &iris, &texturedQuad, &stars};

	ctx->show();
	auto elapsed = vf::Time{};
	while (!ctx->closing()) {
		auto frame = ctx->frame();
		elapsed += frame.dt();

		for (std::size_t i = 0; i < 4; ++i) {
			auto const ds = std::cos(elapsed.count() + starDScale[i]) * 0.5f;
			stars.instances[i].transform.scale = glm::vec2(1.0f) + ds;
			auto const dr = frame.dt().count() * 50.0f * starDRot[i];
			stars.instances[i].transform.orientation.rotate(vf::Degree{dr});
		}

		for (auto primitive : primitives) { frame.draw(*primitive); }
	}
	return;

	auto const clearA = vf::Rgba::make(0xfff000ff);
	auto const clearB = vf::Rgba::make(0x000fffff);

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
	auto image1 = bmp.image();
	auto tex0 = vf::Texture(*ctx, "test_tex", image1);
	mesh.texture = tex0.clone("tex_clone");
	// mesh.texture = std::move(tex0);
	// mesh.texture = vf::Texture(ctx->vram(), "bad", {});
	{
		auto bmp = vf::Bitmap::View{{&vf::magenta_v, 1}};
		image1 = bmp.image();
		mesh.texture.overwrite(image1, vf::Texture::TopLeft{1, 1});
	}

	geo = vf::Geometry{};
	geo.vertices.push_back(vf::Vertex{{-100.0f, -100.0f}});
	geo.vertices.push_back(vf::Vertex{{100.0f, -100.0f}});
	geo.vertices.push_back(vf::Vertex{{0.0f, 100.0f}});
	auto mesh1 = vf::Mesh(*ctx, "mesh1");
	// mesh1.gbo.write(std::move(tri));
	geo = vf::Geometry::makeRegularPolygon(300.0f, 32);
	mesh1.gbo.write(std::move(geo));
	// mesh1.gbo.state.polygonMode = vf::PolygonMode::eLine;
	mesh1.gbo.state.lineWidth = 3.0f;
	mesh1.instance.transform.position = {200.0f, -200.0f};
	mesh1.instance.tint = vf::yellow_v;

	auto quad = vf::QuadShape(*ctx);
	quad.transform().position.y = -200.0f;
	// quad.tint() = vf::Rgba::make(0xc382bf).linear();
	tex0.rescale(10.0f);
	quad.setTexture(std::move(tex0), true);

	{
		auto image = vf::Image{};
		image.load("awesomeface.png");
		tex0 = vf::Texture(*ctx, "awesomeface.png", image);
		// quad.setTexture(std::move(tex0), true);
		mesh1.texture = std::move(tex0);
	}

	auto shader = vf::Shader(*ctx);
	shader.load("test.frag", true);

	auto star = StarShape(*ctx, "star", 100.0f);

	// ctx->updateWindowFlags(vf::WindowFlag::eResizable);

	ctx->show();
	elapsed = vf::Time{};
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
				if (key.key == vf::Key::eSpace && key.action == vf::Action::eRelease) {
					auto set = vf::WindowFlags(vf::WindowFlag::eBorderless);
					auto unset = vf::WindowFlags{};
					if (ctx->windowFlags().test(vf::WindowFlag::eBorderless)) { std::swap(set, unset); }
					ctx->updateWindowFlags(set, unset);
				}
				auto const dpos = frame.dt().count() * 1000.0f;
				if (key.key == vf::Key::eLeft && key.action == vf::Action::ePress) { ctx->view().transform.position.x -= dpos; }
				if (key.key == vf::Key::eRight && key.action == vf::Action::ePress) { ctx->view().transform.position.x += dpos; }
				if (key.key == vf::Key::eUp && key.action == vf::Action::ePress) { ctx->view().transform.position.y += dpos; }
				if (key.key == vf::Key::eDown && key.action == vf::Action::ePress) { ctx->view().transform.position.y -= dpos; }
				break;
			}
			default: break;
			}
		}
		for (auto const code : frame.poll().scancodes) { std::cout << static_cast<char>(code) << '\n'; }

		elapsed += frame.dt();
		mesh.instances[0].transform.orientation.rotate(vf::Degree{frame.dt().count() * 180.0f});
		mesh.instances[1].transform.orientation = mesh.instances[0].transform.orientation;

		auto const dscale = std::cos(elapsed.count()) * 0.25f;
		star.transform().scale = glm::vec2(1.0f) + dscale;

		// auto spec = vf::PipelineState{};
		// spec.flags.set(vf::PipelineState::Flag::eWireframe);
		// frame.surface.bind(spec);
		// ctx->view().transform.orientation = vf::nvec2{1.0f, 0.0f};
		frame.draw(mesh);
		frame.draw(star);
		// ctx->view().viewport.origin = {0.25f, 0.25f};
		// ctx->view().viewport.extent = {0.75f, 0.75f};
		frame.draw(mesh1);
		frame.surface().setShader(shader);
		frame.draw(quad);
		auto const clear = vf::Rgba::lerp(clearA, clearB, (std::sin(elapsed.count()) + 1.0f) * 0.5f);
		frame.setClear(clear.linear());
	}
}
} // namespace

std::ostream& operator<<(std::ostream& o, vf::Version const& v) { return o << 'v' << v.major << '.' << v.minor << '.' << v.patch; }

int main() {
	std::cout << "vulkify " << vf::version_v << '\n';
	// auto context = vf::Builder{}.setFlag(vf::Builder::Flag::eLinearSwapchain).build();
	auto context = vf::Builder{}.build();
	// auto context = vf::Builder{}.setFlag(vf::Builder::Flag::eHeadless).build();
	if (!context) { return EXIT_FAILURE; }
	test(std::move(context.value()));
}
