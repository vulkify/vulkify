#include <ktl/enumerate.hpp>
#include <ktl/kformat.hpp>
#include <vulkify/vulkify.hpp>
#include <array>
#include <future>
#include <iostream>

namespace {
using InstanceStorage = std::array<vf::DrawInstance, 3>;
using InstancedMesh = vf::InstancedMesh<InstanceStorage>;
constexpr auto padding_v = glm::vec2(200.0f, 100.0f);

struct Helper {
	vf::Context& context;
	vf::Rect area = context.area();

	static vf::Geometry make_star(float diameter) {
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

	static void interpolate_rgba(std::span<vf::Vertex> vertices, vf::Rgba from, vf::Rgba to) {
		for (auto [vertex, i] : ktl::enumerate(vertices)) {
			auto const t = static_cast<float>(i) / static_cast<float>(vertices.size());
			vertex.rgba = vf::Rgba::lerp(from, to, t).normalize();
		}
	}

	std::future<vf::Texture> load_texture_async(char const* path) {
		return std::async(std::launch::async, [path, this] {
			auto image = vf::Image{};
			auto load_result = image.load(path);
			if (load_result) { std::cout << ktl::kformat("{} [{}x{}] loaded sucessfully\n", path, load_result->x, load_result->y); }
			return vf::Texture(context.device(), image);
		});
	}

	std::future<vf::Ttf> load_ttf_async(char const* path) {
		return std::async(std::launch::async, [this, path] {
			auto ret = vf::Ttf(context.device());
			if (ret.load(path)) { std::cout << ktl::kformat("[{}] loaded successfully\n", path); }
			return ret;
		});
	}

	vf::Mesh make_triangle() {
		auto geometry = vf::Geometry{};
		geometry.vertices.push_back(vf::Vertex::make({-50.0f, -50.0f}));
		geometry.vertices.push_back(vf::Vertex::make({50.0f, -50.0f}));
		geometry.vertices.push_back(vf::Vertex::make({0.0f, 50.0f}));
		auto ret = vf::Mesh(context.device());
		ret.buffer.write(std::move(geometry));
		ret.storage.transform.position = area.top_left() + glm::vec2(padding_v.x, -padding_v.y);
		return ret;
	}

	vf::Bitmap make_rgb_bitmap() {
		auto ret = vf::Bitmap(vf::white_v, {2, 2});
		ret[vf::Index2D{0, 0}] = vf::red_v;
		ret[vf::Index2D{0, 1}] = vf::green_v;
		ret[vf::Index2D{1, 0}] = vf::blue_v;
		return ret;
	}

	vf::QuadShape make_rgb_quad(vf::Texture const& texture) {
		auto ret = vf::QuadShape(context.device(), vf::QuadShape::State{glm::vec2(100.0f, 100.0f)});
		ret.set_texture(&texture, false);
		ret.transform().position = area.top_right() + glm::vec2(-padding_v.x, -padding_v.y);
		ret.set_silhouette(10.0f, vf::magenta_v);
		return ret;
	}

	vf::CircleShape make_hexagon(vf::Texture& out_texture) {
		auto ret = vf::CircleShape(context.device(), vf::CircleShape::State{100.0f, 6});
		auto bitmap = vf::Bitmap(vf::magenta_v);
		out_texture.overwrite(bitmap.image(), vf::Texture::Rect{{1, 1}, {1, 1}});
		ret.set_texture(&out_texture, false);
		ret.transform().position = area.bottom_left() + glm::vec2(padding_v.x, padding_v.y);
		ret.set_silhouette(10.0f, vf::white_v);
		return ret;
	}

	std::pair<vf::Mesh, vf::CircleShape> make_circles() {
		auto circle = vf::Mesh(context.device());
		auto geometry = vf::Geometry::make_regular_polygon({100.0f, 64});
		auto const circleRgbaStart = vf::yellow_v;
		auto const circleRgbaEnd = vf::cyan_v;
		auto span = std::span<vf::Vertex>(geometry.vertices).subspan(1); // exclude centre
		interpolate_rgba(span.subspan(0, span.size() / 2), circleRgbaStart, circleRgbaEnd);
		interpolate_rgba(span.subspan(span.size() / 2), circleRgbaEnd, circleRgbaStart);
		circle.buffer.write(std::move(geometry));
		circle.storage.transform.position = area.bottom_right() + glm::vec2(-padding_v.x, padding_v.y);
		auto iris = vf::CircleShape(context.device(), vf::CircleShape::State{50.0f});
		iris.transform().position = circle.storage.transform.position;
		iris.tint() = vf::black_v;
		return {std::move(circle), std::move(iris)};
	}

	vf::QuadShape make_textured_quad(vf::Texture const& out_texture) {
		auto ret = vf::QuadShape(context.device(), {{200.0f, 200.0f}});
		ret.set_texture(out_texture.handle()); // should be magenta if image is bad
		return ret;
	}

	InstancedMesh make_stars() {
		auto stars = InstancedMesh(context.device());
		stars.buffer.write(make_star(100.0f));
		stars.storage[0].transform.position = {+(area.extent.x * 0.5f - padding_v.x), 0.0f};
		stars.storage[1].transform.position = {0.0f, -(area.extent.y * 0.5f - padding_v.y)};
		stars.storage[2].transform.position = {-(area.extent.x * 0.5f - padding_v.x), 0.0f};
		return stars;
	}

	vf::Text make_text(vf::Ttf& ttf) {
		auto ret = vf::Text(context.device());
		ret.set_ttf(&ttf).set_height(vf::Glyph::Height{80}).set_string("vulkify");
		ret.tint() = vf::Rgba::make(0xec3841ff);
		ret.transform().position.y = area.extent.y * 0.5f - padding_v.y;
		return ret;
	}
};

void test(vf::Context context) {
	std::cout << "using GPU: " << context.gpu().name << '\n';

	auto helper = Helper{context};
	auto texture_future = helper.load_texture_async("test_image.png");
	auto ttf_future = helper.load_ttf_async("test_font.ttf");

	auto triangle = helper.make_triangle();
	auto rgb_bitmap = helper.make_rgb_bitmap();
	auto rgb_texture = vf::Texture(context.device(), rgb_bitmap.image());
	auto rgb_quad = helper.make_rgb_quad(rgb_texture);
	auto hexagon = helper.make_hexagon(rgb_texture);
	auto [circle, iris] = helper.make_circles();
	assert(texture_future.valid());
	auto image_texture = texture_future.get();
	auto textured_quad = helper.make_textured_quad(image_texture);
	auto stars = helper.make_stars();
	assert(ttf_future.valid());
	auto ttf = ttf_future.get();
	auto text = helper.make_text(ttf);

	auto text_y = text.transform().position.y;

	struct StarOffset {
		float dscale{};
		float drot{};
	};

	static constexpr StarOffset starOffsets[stars.storage.size()] = {{-0.5f, 2.0f}, {0.25f, 5.0f}, {0.75f, 3.0f}};
	static constexpr auto clearA = vf::Rgba::make(0xfff000ff);
	static constexpr auto clearB = vf::Rgba::make(0x000fffff);

	vf::Primitive const* primitives[] = {&triangle, &rgb_quad, &hexagon, &circle, &iris, &textured_quad, &stars, &text};

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
				if (key(vf::Key::eW, vf::Action::eRelease, vf::Mod::eCtrl)) { return; }
			}
			default: break;
			}
		}

		for (auto [star, index] : ktl::enumerate(stars.storage)) {
			auto const ds = std::cos(elapsed.count() + starOffsets[index].dscale) * 0.5f;
			star.transform.scale = glm::vec2(1.0f) + ds;
			auto const dr = frame.dt().count() * 50.0f * starOffsets[index].drot;
			stars.storage[index].transform.orientation.rotate(vf::Degree{dr});
		}

		circle.storage.transform.orientation.rotate(vf::Radian{frame.dt().count()});
		auto const text_dy = std::abs(std::sin(elapsed.count()) * 3.0f);
		text.transform().position.y = text_y - (text_dy * 10.0f);

		for (auto primitive : primitives) { frame.draw(*primitive); }
	}
}
} // namespace

int main(int argc, char** argv) {
	bool const headless = argc > 1 && argv[1] == std::string_view("--headless");
	std::cout << "vulkify " << vf::version_v << '\n';
	static constexpr auto extent = vf::Extent{1280, 720};
	auto context = vf::Builder{}.set_extent(extent).set_flag(vf::WindowFlag::eResizable).set_flag(vf::InstanceFlag::eHeadless, headless).build();
	if (!context) { return EXIT_FAILURE; }
	context->camera().view.set_extent(extent);
	context->lock_aspect_ratio(true);
	test(std::move(context.value()));
}
