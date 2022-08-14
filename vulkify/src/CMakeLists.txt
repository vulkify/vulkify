target_sources(${PROJECT_NAME} PRIVATE
  context/builder.cpp
  context/context.cpp

  core/rgba.cpp

  graphics/atlas.cpp
  graphics/bitmap.cpp
  graphics/geometry_buffer.cpp
  graphics/geometry.cpp
  graphics/image.cpp
  graphics/shader.cpp
  graphics/surface.cpp
  graphics/texture.cpp

  graphics/primitives/circle_shape.cpp
  graphics/primitives/quad_shape.cpp
  graphics/primitives/sprite.cpp
  graphics/primitives/shape.cpp
  graphics/primitives/text.cpp

  instance/vf_instance.cpp

  detail/command_pool.hpp
  detail/defer_base.hpp
  detail/defer_queue.hpp
  detail/descriptor_set_factory.hpp
  detail/detail.cpp
  detail/gfx_allocation.hpp
  detail/gfx_allocations.hpp
  detail/gfx_command_buffer.hpp
  detail/gfx_device.hpp
  detail/gfx_font.hpp
  detail/pipeline_factory.cpp
  detail/pipeline_factory.hpp
  detail/render_pass.hpp
  detail/renderer.cpp
  detail/renderer.hpp
  detail/rotator.hpp
  detail/set_writer.hpp
  detail/spir_v.cpp
  detail/trace.cpp
  detail/trace.hpp
  detail/verify.cpp
  detail/vulkan_device.hpp
  detail/vulkan_instance.hpp
  detail/vulkan_swapchain.cpp
  detail/vulkan_swapchain.hpp

  detail/window/window.hpp
  detail/window/window.cpp

  spir_v/default.frag.hpp
  spir_v/default.vert.hpp

  ttf/ft.cpp
  ttf/ft.hpp
)
