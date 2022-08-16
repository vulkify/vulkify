#include <detail/descriptor_set_factory.hpp>
#include <detail/gfx_allocations.hpp>
#include <detail/pipeline_factory.hpp>
#include <detail/render_pass.hpp>
#include <detail/trace.hpp>
#include <ktl/fixed_vector.hpp>
#include <vulkify/graphics/descriptor_set.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/geometry_buffer.hpp>
#include <vulkify/graphics/shader.hpp>
#include <vulkify/graphics/texture.hpp>
#include <vulkify/instance/instance.hpp>

namespace vf {
namespace {
constexpr vk::PolygonMode polygon_mode(PolygonMode mode) {
	switch (mode) {
	case PolygonMode::ePoint: return vk::PolygonMode::ePoint;
	case PolygonMode::eLine: return vk::PolygonMode::eLine;
	case PolygonMode::eFill:
	default: return vk::PolygonMode::eFill;
	}
}

constexpr vk::PrimitiveTopology topology(Topology topo) {
	switch (topo) {
	case Topology::eLineStrip: return vk::PrimitiveTopology::eLineStrip;
	case Topology::eLineList: return vk::PrimitiveTopology::eLineList;
	case Topology::ePointList: return vk::PrimitiveTopology::ePointList;
	case Topology::eTriangleList: return vk::PrimitiveTopology::eTriangleList;
	case Topology::eTriangleStrip:
	default: return vk::PrimitiveTopology::eTriangleStrip;
	}
}

[[maybe_unused]] constexpr auto name_v = "vf::(internal)";
} // namespace

DescriptorSet::DescriptorSet(ktl::not_null<Shader const*> shader) : m_shader(shader->handle()) {}

CombinedImageSampler RenderPass::image_sampler(Handle<Texture> texture) const {
	auto const image = static_cast<GfxImage const*>(texture.allocation);
	if (!image) { return white_texture(); }
	assert(image->type() == GfxAllocation::Type::eImage);
	if (image->image.cache.view && image->image.sampler) { return CombinedImageSampler{*image->image.cache.view, *image->image.sampler}; }
	return white_texture();
}

CombinedImageSampler RenderPass::white_texture() const { return CombinedImageSampler{*shader_input.textures->white.view, *shader_input.textures->sampler}; }

void RenderPass::write_view(SetWriter& set) const {
	if (!set) {
		VF_TRACE(name_v, trace::Type::eWarn, "Failed to write view set");
		return;
	}
	auto const scale = cam.camera->view.get_scale(cam.extent);
	// invert transformation
	auto const dm = DrawModel{{-cam.camera->position, cam.camera->orientation.inverted().value()}, {scale, glm::vec2()}};
	set.write(shader_input.one.bindings.ubo, &dm, sizeof(dm));
}

void RenderPass::write_models(SetWriter& set, std::span<DrawModel const> instances, Handle<Texture> texture) const {
	auto const tex = image_sampler(texture);
	if (!set || instances.empty() || !tex.sampler || !tex.view) {
		VF_TRACE(name_v, trace::Type::eWarn, "Failed to write models set");
		return;
	}
	auto const& sb = shader_input.one;
	set.write(sb.bindings.ssbo, instances.data(), instances.size_bytes());
	set.update(sb.bindings.sampler, tex.sampler, tex.view);
	set.bind(command_buffer, bound);
}

void RenderPass::write_custom(SetWriter& set, std::span<std::byte const> ubo, Handle<Texture> texture) const {
	auto const tex = image_sampler(texture);
	if (!set || !tex.sampler || !tex.view) {
		VF_TRACE(name_v, trace::Type::eWarn, "Failed to write custom set");
		return;
	}
	static constexpr auto byte_v = std::byte{};
	if (ubo.empty()) { ubo = {&byte_v, 1}; }
	auto const& sb = shader_input.two;
	set.write(sb.bindings.ubo, ubo.data(), ubo.size_bytes());
	set.update(sb.bindings.sampler, tex.sampler, tex.view);
	set.bind(command_buffer, bound);
}

void RenderPass::bind(vk::PipelineLayout layout, vk::Pipeline pipeline) const {
	if (layout == bound) { return; }
	if (!layout || !command_buffer) {
		VF_TRACE(name_v, trace::Type::eWarn, "Failed to bind pipeline");
		return;
	}
	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	shader_input.mat_p.bind(command_buffer, layout);
	bound = layout;
}

void RenderPass::set_viewport() const {
	if (!command_buffer || !cam.camera) {
		VF_TRACE(name_v, trace::Type::eWarn, "Failed to set viewport");
		return;
	}
	auto const vp = Rect{{cam.extent * cam.camera->viewport.extent, cam.extent * cam.camera->viewport.offset}};
	command_buffer.setViewport(0, vk::Viewport(vp.offset.x, vp.offset.y + vp.extent.y, vp.extent.x, -vp.extent.y)); // flip x / negative y
}

Surface::~Surface() {
	if (m_render_pass && m_render_pass->instance) { m_render_pass->instance.value->end_pass(); }
}

Surface::operator bool() const { return m_render_pass && m_render_pass->instance; }

bool Surface::draw(Drawable const& drawable, RenderState const& state) const {
	if (!m_render_pass || !m_render_pass->pipeline_factory || !m_render_pass->render_pass) { return false; }
	if (drawable.instances.empty() || !drawable.buffer) { return false; }
	if (drawable.instances.size() <= small_buffer_v) {
		auto buffer = ktl::fixed_vector<DrawModel, small_buffer_v>{};
		add_draw_models(drawable.instances, std::back_inserter(buffer));
		return draw(buffer, drawable, state);
	} else {
		auto buffer = std::vector<DrawModel>{};
		buffer.reserve(drawable.instances.size());
		add_draw_models(drawable.instances, std::back_inserter(buffer));
		return draw(buffer, drawable, state);
	}
}

bool Surface::bind(RenderState const& state) const {
	if (!m_render_pass) { return false; }
	auto program = PipelineFactory::Spec::ShaderProgram{};
	if (state.descriptor_set && state.descriptor_set->m_shader && state.descriptor_set->m_shader.allocation) {
		assert(state.descriptor_set->m_shader.allocation->type() == GfxAllocation::Type::eShader);
		program.frag = *static_cast<GfxShader const*>(state.descriptor_set->m_shader.allocation)->module;
	}
	auto const spec = PipelineFactory::Spec{
		.shader = program,
		.mode = polygon_mode(state.polygon_mode),
		.topology = topology(state.topology),
		.depth_test = state.force_z_order.value_or(m_render_pass->device->default_z_order) == ZOrder::eOn,
	};
	auto const [pipe, layout] = m_render_pass->pipeline_factory->pipeline(spec, m_render_pass->render_pass);
	if (!pipe || !layout) { return false; }
	m_render_pass->bind(layout, pipe);
	return true;
}

bool Surface::draw(std::span<DrawModel const> models, Drawable const& drawable, RenderState const& state) const {
	if (!m_render_pass || !m_render_pass->pipeline_factory || !m_render_pass->render_pass || !m_render_pass->render_mutex) { return false; }
	if (drawable.instances.empty() || !drawable.buffer) { return false; }
	auto lock = std::scoped_lock(*m_render_pass->render_mutex);
	if (!bind(state)) { return false; }

	auto set = m_render_pass->set_factory->post_increment(m_render_pass->shader_input.one.set);
	if (!set) { return false; }
	m_render_pass->write_view(set);
	m_render_pass->write_models(set, models, drawable.texture);
	if (state.descriptor_set) {
		set = m_render_pass->set_factory->post_increment(m_render_pass->shader_input.two.set);
		if (!set) { return false; }
		m_render_pass->write_custom(set, state.descriptor_set->m_data.bytes, state.descriptor_set->m_data.texture);
	}
	m_render_pass->set_viewport();
	auto const lineWidth = std::clamp(state.line_width, m_render_pass->line_width_limit.first, m_render_pass->line_width_limit.second);
	m_render_pass->command_buffer.setLineWidth(lineWidth);

	auto const* gbo = static_cast<GfxGeometryBuffer const*>(drawable.buffer.allocation);
	assert(gbo && gbo->type() == GfxAllocation::Type::eBuffer);
	auto const& vbo = gbo->buffers[0].get(true);
	m_render_pass->command_buffer.bindVertexBuffers(0, vbo.resource, vk::DeviceSize{});
	auto const instanceCount = static_cast<std::uint32_t>(models.size());
	if (gbo->indices > 0) {
		auto const& ibo = gbo->buffers[1].get(true);
		m_render_pass->command_buffer.bindIndexBuffer(ibo.resource, vk::DeviceSize{}, vk::IndexType::eUint32);
		m_render_pass->command_buffer.drawIndexed(gbo->indices, instanceCount, 0, 0, 0);
	} else {
		m_render_pass->command_buffer.draw(gbo->vertices, instanceCount, 0, 0);
	}
	return true;
}
} // namespace vf
