#include <base.hpp>
#include <vulkify/graphics/primitives/all.hpp>
#include <vulkify/graphics/shader.hpp>
#include <vulkify/graphics/texture.hpp>

namespace example {
inline constexpr auto world_view_v = vf::Rect{{glm::vec2{1.0f}, glm::vec2{0.0f}}};
inline constexpr auto minimap_view_v = vf::Rect{{glm::vec2{0.15f}, glm::vec2{0.85f, 0.05f}}};

constexpr glm::vec2 clamp(glm::vec2 in, glm::vec2 const a, glm::vec2 const b) {
	in.x = std::clamp(in.x, a.x, b.x);
	in.y = std::clamp(in.y, a.y, b.y);
	return in;
}

struct Controller {
	vf::Transform transform{};
	float speed{1000.0f};

	void tick(Base::Input const& input, vf::Time dt) {
		auto dxy = glm::vec2{};
		if (input.held(vf::Key::eW) || input.held(vf::Key::eUp)) { dxy.y += 1.0f; }
		if (input.held(vf::Key::eD) || input.held(vf::Key::eRight)) { dxy.x += 1.0f; }
		if (input.held(vf::Key::eS) || input.held(vf::Key::eDown)) { dxy.y -= 1.0f; }
		if (input.held(vf::Key::eA) || input.held(vf::Key::eLeft)) { dxy.x -= 1.0f; }
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

		m_cover = vf::Mesh{context().device()};
		auto const viewport = minimap_view_v.extent * m_framebuffer_size;
		auto qci = vf::QuadCreateInfo{};
		qci.size = 1.1f * minimap_view_v.extent * vf::LockedResize<vf::Crop::eFillMin>{m_bg_size}(m_framebuffer_size);
		qci.origin = 0.5f * glm::vec2{viewport.x, -viewport.y};
		m_cover.buffer.write(vf::Geometry::make_quad(qci));
		m_cover.instance().transform.position = viewport_to_world(minimap_view_v.offset, m_framebuffer_size);
	}

	static constexpr glm::vec2 viewport_to_world(glm::vec2 const viewport, glm::vec2 const extent) {
		return glm::vec2{viewport.x - 0.5f, 0.5f - viewport.y} * extent;
	}

	bool setup() override {
		if (!load_assets()) { return false; }

		m_controller.transform.orientation = vf::nvec2::up_v;
		m_framebuffer_size = glm::vec2{context().framebuffer_extent()};
		clear = vf::Rgba::make(0x113388ff).linear();
		make_quads();

		return true;
	}

	void tick(vf::Time dt) override {
		m_controller.tick(input, dt);
		auto const player_limit = 0.5f * (m_bg_size - m_player_size);
		m_controller.transform.position = clamp(m_controller.transform.position, -player_limit, player_limit);
		m_player->instance().transform = m_controller.transform;
	}

	void render(vf::Frame const& frame) const override {
		setup_world(frame.camera());
		Base::render(frame);
		frame.camera().position = {};
		frame.draw(m_cover);
		setup_minimap(frame.camera());
		Base::render(frame);
	}

	void setup_world(vf::Camera& out_camera) const {
		auto const camera_limit = 0.5f * (m_bg_size - m_framebuffer_size);
		out_camera.position = clamp(m_controller.transform.position, -camera_limit, camera_limit);
		out_camera.view.set_scale(glm::vec2{1.0f});
		out_camera.viewport = world_view_v;
	}

	void setup_minimap(vf::Camera& out_camera) const {
		out_camera.position = {};
		out_camera.view.set_extent(m_bg_size, vf::Crop::eFillMax);
		out_camera.viewport = minimap_view_v;
	}

	vf::Texture m_background{};
	Controller m_controller{};
	vf::Mesh m_cover{};
	vf::Ptr<vf::Mesh> m_player{};

	glm::vec2 m_framebuffer_size{};
	glm::vec2 m_bg_size{};
	glm::vec2 m_player_size{};
};
} // namespace example

int main(int argc, char** argv) {
	auto minimap = example::Minimap{};
	return minimap(argc, argv);
}
