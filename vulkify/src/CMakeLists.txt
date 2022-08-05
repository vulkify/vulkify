target_sources(${PROJECT_NAME} PRIVATE
  context/builder.cpp
  context/context.cpp

  core/rgba.cpp

  graphics/atlas.cpp
  graphics/bitmap.cpp
  graphics/geometry.cpp
  graphics/image.cpp
  graphics/shader.cpp
  graphics/surface.cpp

  graphics/resources/geometry_buffer.cpp
  graphics/resources/resource.cpp
  graphics/resources/texture.cpp

  graphics/primitives/circle_shape.cpp
  graphics/primitives/quad_shape.cpp
  graphics/primitives/shape.cpp
  graphics/primitives/text.cpp

  instance/vf_instance.cpp

  detail/cache.hpp
  detail/command_pool.hpp
  detail/defer_queue.hpp
  detail/descriptor_set_factory.hpp
  detail/pipeline_factory.cpp
  detail/pipeline_factory.hpp
  detail/render_pass.hpp
  detail/renderer.cpp
  detail/renderer.hpp
  detail/rotator.hpp
  detail/set_writer.hpp
  detail/shared_impl.hpp
  detail/spir_v.cpp
  detail/trace.cpp
  detail/trace.hpp
  detail/vk_device.hpp
  detail/vk_instance.cpp
  detail/vk_instance.hpp
  detail/vk_surface.cpp
  detail/vk_surface.hpp
  detail/vram.cpp
  detail/vram.hpp

  detail/window/window.hpp
  detail/window/window.cpp

  spir_v/default.frag.hpp
  spir_v/default.vert.hpp

  ttf/ft.cpp
  ttf/ft.hpp
)
