#include <base.hpp>
#include <vulkify/graphics/primitives/all.hpp>
#include <vulkify/graphics/shader.hpp>
#include <vulkify/graphics/texture.hpp>

namespace example {
template <std::derived_from<vf::Primitive> T>
class Shaded : public vf::Primitive {
  public:
	struct Uniform {
		float alpha{};
	};

	Uniform uniform{};
	vf::TextureHandle blend_texture{};

	Shaded() = default;
	Shaded(T&& t, vf::Shader const& shader) : m_t(std::move(t)), m_shader(&shader) {}

	T& get() { return m_t; }
	T const& get() const { return m_t; }

	void draw(vf::Surface const& surface, vf::RenderState const& state) const override {
		auto copy = state;
		auto set = vf::DescriptorSet(*m_shader);
		set.write(uniform);
		set.write(blend_texture);
		copy.descriptor_set = &set;
		m_t.draw(surface, copy);
	}

  private:
	T m_t{};
	vf::Ptr<vf::Shader const> m_shader{};
};

using ShadedMesh = Shaded<vf::Mesh>;

class Shader : public Base {
	std::string title() const override { return "Shaders"; }

	bool load_shader() {
		auto path = std::string_view("shaders/texture_mask.frag");
		if (env.args.size() > 1) { path = env.args[1]; }
		m_shader = vf::Shader(context());
		if (!m_shader.load(env.dataPath(path).c_str(), true)) {
			log.error("Failed to load Shader [{}]", path);
			return false;
		}
		return true;
	}

	bool load_texture(vf::Texture& out, std::string_view uri) const {
		auto image = vf::Image{};
		if (!image.load(env.dataPath(uri).c_str())) {
			log.error("Failed to load image [{}]", uri);
			return false;
		}
		out = vf::Texture(context(), image);
		if (!out) {
			log.error("Failed to load texture [{}]", uri);
			return false;
		}
		return true;
	}

	bool load_textures() {
		if (!load_texture(m_textures.crate, "textures/crate.png")) { return false; }
		if (!load_texture(m_textures.overlay, "textures/overlay.jpg")) { return false; }
		return true;
	}

	bool load_assets() {
		if (!load_shader()) { return false; }
		if (!load_textures()) { return false; }
		return true;
	}

	glm::vec2 quad_size() const {
		glm::vec2 const extent = context().framebuffer_extent();
		auto const side = extent.y * 0.8f;
		return {side, side};
	}

	ShadedMesh& make_quad(vf::Geometry geometry) {
		auto& ret = scene.add(ShadedMesh(vf::Mesh(context()), m_shader));
		ret.get().buffer.write(std::move(geometry));
		ret.get().texture = m_textures.crate.handle();
		return ret;
	}

	void make_quads() {
		glm::vec2 const extent = context().framebuffer_extent();
		auto const maxWidth = extent.x * 0.33f;
		auto const maxHeight = extent.y;
		auto const side = std::min(maxWidth * 0.8f, maxHeight * 0.8f);
		auto const gap = (extent.x - 3.0f * side) * 0.25f;
		auto const dx = side + gap;
		auto geometry = vf::Geometry::make_quad({{side, side}});

		auto& left = make_quad(geometry);
		auto& centre = make_quad(geometry);
		auto& right = make_quad(geometry);

		left.get().storage.transform.position.x -= dx;
		right.get().storage.transform.position.x += dx;

		centre.blend_texture = right.blend_texture = m_textures.overlay.handle();
		right.uniform.alpha = 1.0f;

		m_blend_quad = &centre;
	}

	bool setup() override {
		if (!load_assets()) { return false; }
		make_quads();
		return true;
	}

	void tick(vf::Time dt) override {
		m_elapsed += dt;
		m_blend_quad->uniform.alpha = (std::sin(m_elapsed.count()) + 1.0f) * 0.5f;
	}

	struct {
		vf::Texture crate{};
		vf::Texture overlay{};
	} m_textures{};
	vf::Shader m_shader{};
	vf::Ptr<ShadedMesh> m_blend_quad{};

	vf::Time m_elapsed{};
};
} // namespace example

int main(int argc, char** argv) {
	auto shader = example::Shader{};
	return shader(argc, argv);
}
