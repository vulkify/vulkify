#include <vulkify/vulkify.hpp>
#include <iostream>

#include <ktl/enumerate.hpp>
#include <array>

namespace {
using InstanceStorage = std::array<vf::DrawInstance, 4>;
using InstancedMesh = vf::InstancedMesh<InstanceStorage>;
constexpr auto padding_v = glm::vec2(200.0f, 100.0f);

struct Helper {
	vf::Context& context;
	vf::Rect viewRect = context.space().rect;

	static vf::Geometry makeStar(float diameter) {
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

	static void interpolateRgba(std::span<vf::Vertex> vertices, vf::Rgba from, vf::Rgba to) {
		for (auto [vertex, i] : ktl::enumerate(vertices)) {
			auto const t = static_cast<float>(i) / static_cast<float>(vertices.size());
			vertex.rgba = vf::Rgba::lerp(from, to, t).normalize();
		}
	}

	vf::Mesh makeTriangle() {
		auto geometry = vf::Geometry{};
		geometry.vertices.push_back(vf::Vertex{{-50.0f, -50.0f}});
		geometry.vertices.push_back(vf::Vertex{{50.0f, -50.0f}});
		geometry.vertices.push_back(vf::Vertex{{0.0f, 50.0f}});
		auto ret = vf::Mesh(context, "triangle");
		ret.gbo.write(std::move(geometry));
		ret.instance.transform.position = viewRect.topLeft() + glm::vec2(padding_v.x, -padding_v.y);
		return ret;
	}

	vf::Texture makeRgbTexture() {
		auto bitmap = vf::Bitmap(vf::white_v, {2, 2});
		bitmap[vf::Index2D{0, 0}] = vf::red_v;
		bitmap[vf::Index2D{0, 1}] = vf::green_v;
		bitmap[vf::Index2D{1, 0}] = vf::blue_v;
		return vf::Texture(context, "rgb_texture", bitmap.image());
	}

	vf::QuadShape makeRgbQuad(vf::Texture const& texture) {
		auto ret = vf::QuadShape(context, "rgb_quad", vf::QuadShape::State{glm::vec2(100.0f, 100.0f)});
		ret.setTexture(texture.clone("rgb_texture_clone"), false);
		ret.transform().position = viewRect.topRight() + glm::vec2(-padding_v.x, -padding_v.y);
		ret.setOutline(5.0f, vf::magenta_v);
		return ret;
	}

	vf::CircleShape makeHexagon(vf::Texture texture) {
		auto ret = vf::CircleShape(context, "hexagon", {100.0f, 6});
		auto bitmap = vf::Bitmap(vf::magenta_v);
		texture.overwrite(bitmap.image(), vf::Texture::Rect{{1, 1}});
		ret.setTexture(std::move(texture), false);
		ret.transform().position = viewRect.bottomLeft() + glm::vec2(padding_v.x, padding_v.y);
		ret.setOutline(5.0f, vf::white_v);
		return ret;
	}

	std::pair<vf::Mesh, vf::CircleShape> makeCircles() {
		auto circle = vf::Mesh(context, "circle");
		auto geometry = vf::Geometry::makeRegularPolygon({100.0f, 64});
		auto const circleRgbaStart = vf::yellow_v;
		auto const circleRgbaEnd = vf::cyan_v;
		auto span = std::span<vf::Vertex>(geometry.vertices).subspan(1); // exclude centre
		interpolateRgba(span.subspan(0, span.size() / 2), circleRgbaStart, circleRgbaEnd);
		interpolateRgba(span.subspan(span.size() / 2), circleRgbaEnd, circleRgbaStart);
		circle.gbo.write(std::move(geometry));
		circle.instance.transform.position = viewRect.bottomRight() + glm::vec2(-padding_v.x, padding_v.y);
		auto iris = vf::CircleShape(context, "iris", vf::CircleShape::State{50.0f});
		iris.transform().position = circle.instance.transform.position;
		iris.tint() = vf::black_v;
		return {std::move(circle), std::move(iris)};
	}

	vf::QuadShape makeTexturedQuad(char const* imagePath) {
		auto image = vf::Image{};
		auto loadResult = image.load(imagePath);
		if (loadResult) { std::cout << imagePath << " [" << loadResult->x << 'x' << loadResult->y << "] loaded sucessfully\n"; }

		auto ret = vf::QuadShape(context, "textured_quad", vf::QuadShape::State{glm::vec2(200.0f, 200.0f)});
		ret.setTexture(vf::Texture(context, "texture", image), false); // should be magenta if image is bad
		return ret;
	}

	InstancedMesh makeStars() {
		auto stars = InstancedMesh(context, "stars");
		stars.gbo.write(makeStar(100.0f));
		stars.instances[0].transform.position = {0.0f, +(viewRect.extent.y * 0.5f - padding_v.y)};
		stars.instances[1].transform.position = {+(viewRect.extent.x * 0.5f - padding_v.x), 0.0f};
		stars.instances[2].transform.position = {0.0f, -(viewRect.extent.y * 0.5f - padding_v.y)};
		stars.instances[3].transform.position = {-(viewRect.extent.x * 0.5f - padding_v.x), 0.0f};
		return stars;
	}
};

void test(vf::Context context) {
	std::cout << "using GPU: " << context.gpu().name << '\n';
	auto helper = Helper{context};

	auto triangle = helper.makeTriangle();
	auto rgbTexture = helper.makeRgbTexture();
	auto rgbQuad = helper.makeRgbQuad(rgbTexture);
	auto hexagon = helper.makeHexagon(std::move(rgbTexture));
	auto [circle, iris] = helper.makeCircles();
	auto texturedQuad = helper.makeTexturedQuad("test_image.png");
	auto stars = helper.makeStars();

	struct StarOffset {
		float dscale{};
		float drot{};
	};

	static constexpr StarOffset starOffsets[stars.instances.size()] = {{-1.0f, 1.0f}, {-0.5f, 2.0f}, {0.25f, 5.0f}, {0.75f, 3.0f}};
	static constexpr auto clearA = vf::Rgba::make(0xfff000ff);
	static constexpr auto clearB = vf::Rgba::make(0x000fffff);

	vf::Primitive const* primitives[] = {&triangle, &rgbQuad, &hexagon, &circle, &iris, &texturedQuad, &stars};

	context.show();
	auto elapsed = vf::Time{};

	while (!context.closing()) {
		auto const clear = vf::Rgba::lerp(clearA, clearB, (std::sin(elapsed.count()) + 1.0f) * 0.5f);
		auto frame = context.frame(clear);
		elapsed += frame.dt();

		for (auto const& event : frame.poll().events) {
			switch (event.type) {
			case vf::EventType::eKey: {
				auto const& key = event.get<vf::EventType::eKey>();
				if (key.key == vf::Key::eW && key.action == vf::Action::eRelease && key.mods.test(vf::Mod::eCtrl)) { return; }
			}
			default: break;
			}
		}

		if (auto pad = vf::Gamepad{0}) {
			auto const dx = pad(vf::GamepadAxis::eLeftX) * frame.dt().count() * 100.0f;
			context.view().position.x += dx;
		}

		for (auto [star, index] : ktl::enumerate(stars.instances)) {
			auto const ds = std::cos(elapsed.count() + starOffsets[index].dscale) * 0.5f;
			star.transform.scale = glm::vec2(1.0f) + ds;
			auto const dr = frame.dt().count() * 50.0f * starOffsets[index].drot;
			stars.instances[index].transform.orientation.rotate(vf::Degree{dr});
		}

		circle.instance.transform.orientation.rotate(vf::Radian{frame.dt().count()});

		for (auto primitive : primitives) { frame.draw(*primitive); }
	}
}
} // namespace

std::ostream& operator<<(std::ostream& o, vf::Version const& v) { return o << 'v' << v.major << '.' << v.minor << '.' << v.patch; }

int main(int argc, char** argv) {
	bool const headless = argc > 1 && argv[1] == std::string_view("--headless");
	std::cout << "vulkify " << vf::version_v << '\n';
	auto context = vf::Builder{}.setFlag(vf::WindowFlag::eResizable).setFlag(vf::InstanceFlag::eHeadless, headless).build();
	if (!context) { return EXIT_FAILURE; }
	test(std::move(context.value()));
}
