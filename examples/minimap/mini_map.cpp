#include <base.hpp>
#include <vulkify/graphics/primitives/all.hpp>
#include <vulkify/graphics/shader.hpp>
#include <vulkify/graphics/texture.hpp>

namespace example {
struct Controls {
	vf::Key up{vf::Key::eUp};
	vf::Key right{vf::Key::eRight};
	vf::Key down{vf::Key::eDown};
	vf::Key left{vf::Key::eLeft};
};

inline constexpr auto wasd_v = Controls{
	.up = vf::Key::eW,
	.right = vf::Key::eD,
	.down = vf::Key::eS,
	.left = vf::Key::eA,
};

constexpr glm::vec2 clamp(glm::vec2 in, glm::vec2 const a, glm::vec2 const b) {
	in.x = std::clamp(in.x, a.x, b.x);
	in.y = std::clamp(in.y, a.y, b.y);
	return in;
}

struct Controller {
	Controls controls{};
	vf::Transform transform{};
	float speed{1000.0f};

	void tick(Base::Input const& input, vf::Time dt) {
		auto dxy = glm::vec2{};
		if (input.held(controls.up)) { dxy.y += 1.0f; }
		if (input.held(controls.right)) { dxy.x += 1.0f; }
		if (input.held(controls.down)) { dxy.y -= 1.0f; }
		if (input.held(controls.left)) { dxy.x -= 1.0f; }
		if (dxy.x != 0.0f || dxy.y != 0.0f) {
			auto const ndxy = vf::nvec2{dxy};
			transform.position += speed * dt.count() * ndxy.value();
			transform.orientation = ndxy;
		}
	}
};

class Minimap : public Base {
	std::string title() const override { return "Shaders"; }

	bool load_texture(vf::Texture& out, std::string_view uri) const {
		auto image = vf::Image{};
		if (!image.load(env.data_path(uri).c_str())) {
			log.error("Failed to load image [{}]", uri);
			return false;
		}
		auto const tci = vf::TextureCreateInfo{.filtering = vf::Filtering::eLinear};
		out = vf::Texture(device(), image, tci);
		if (!out) {
			log.error("Failed to load texture [{}]", uri);
			return false;
		}
		return true;
	}

	bool load_textures() {
		if (!load_texture(m_background, "textures/sky_bg_2048x2048.jpg")) { return false; }
		return true;
	}

	bool load_assets() {
		if (!load_textures()) { return false; }
		return true;
	}

	glm::vec2 quad_size() const {
		glm::vec2 const extent = context().framebuffer_extent();
		auto const side = extent.y * 0.8f;
		return {side, side};
	}

	void make_quads() {
		auto const bg_scale = static_cast<float>(context().framebuffer_extent().x) / static_cast<float>(context().window_extent().x);
		m_bg_size = bg_scale * glm::vec2(m_background.extent());
		auto& mesh = scene.add<vf::Mesh>(context().device());
		mesh.texture = m_background.handle();
		mesh.buffer.write(vf::Geometry::make_quad({.size = m_bg_size}));

		m_player = &scene.add<vf::Mesh>(context().device());
		auto triangle = vf::Geometry{};
		triangle.vertices.push_back(vf::Vertex::make(bg_scale * glm::vec2{-50.0f, -50.0f}));
		triangle.vertices.push_back(vf::Vertex::make(bg_scale * glm::vec2{50.0f, 0.0f}));
		triangle.vertices.push_back(vf::Vertex::make(bg_scale * glm::vec2{-50.f, 50.0f}));
		m_player->buffer.write(std::move(triangle));
		m_player_size = bg_scale * glm::vec2{100.0f};
	}

	bool setup() override {
		if (!load_assets()) { return false; }

		make_quads();
		m_controller.transform.orientation = vf::nvec2::up_v;
		m_camera = &context().camera();

		m_framebuffer_size = glm::vec2{context().framebuffer_extent()};
		clear = vf::Rgba::make(0x113388ff).linear();

		auto const bg_ar = m_bg_size.x / m_bg_size.y;
		auto const fb_ar = m_framebuffer_size.x / m_framebuffer_size.y;
		if (fb_ar > bg_ar) {
			m_viewport_size.y = m_bg_size.y;
			m_viewport_size.x = m_bg_size.x * fb_ar;
		} else {
			m_viewport_size.x = m_bg_size.x;
			m_viewport_size.y = m_bg_size.y / fb_ar;
		}
		return true;
	}

	void tick(vf::Time dt) override {
		m_controller.tick(input, dt);
		auto const player_limit = 0.5f * (m_bg_size - m_player_size);
		m_controller.transform.position = clamp(m_controller.transform.position, -player_limit, player_limit);
		m_player->instance().transform = m_controller.transform;

		auto const camera_limit = 0.5f * (m_bg_size - m_framebuffer_size);
		m_camera->position = clamp(m_controller.transform.position, -camera_limit, camera_limit);
	}

	void render(vf::Frame const& frame) const override {
		auto const camera_limit = 0.5f * (m_bg_size - m_framebuffer_size);
		m_camera->position = clamp(m_controller.transform.position, -camera_limit, camera_limit);
		m_camera->view.set_scale(glm::vec2{1.0f});
		m_camera->viewport.extent = glm::vec2{1.0f};
		m_camera->viewport.offset = {};
		Base::render(frame);
		m_camera->position = {};
		m_camera->viewport.extent = glm::vec2{0.15f};
		m_camera->viewport.offset = glm::vec2{0.80f, 0.05f};
		m_camera->view.set_extent(m_viewport_size);
		Base::render(frame);
	}

	vf::Texture m_background{};
	Controller m_controller{};
	vf::Ptr<vf::Mesh> m_player{};
	vf::Ptr<vf::Camera> m_camera{};

	glm::vec2 m_framebuffer_size{};
	glm::vec2 m_bg_size{};
	glm::vec2 m_viewport_size{};
	glm::vec2 m_player_size{};
};
} // namespace example

int main(int argc, char** argv) {
	auto shader = example::Minimap{};
	return shader(argc, argv);
}
