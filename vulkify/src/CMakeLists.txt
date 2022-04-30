target_sources(${PROJECT_NAME} PRIVATE
  context/context.cpp

  graphics/geometry.cpp
  core/rgba.cpp

  graphics/bitmap.cpp
  graphics/buffer.cpp
  graphics/drawable.cpp
  graphics/primitive.cpp
  graphics/resource.cpp
  graphics/spir_v.cpp
  graphics/surface.cpp
  graphics/texture.cpp

  instance/vf_instance.cpp

  detail/command_pool.hpp
  detail/defer_queue.hpp
  detail/image_cache.hpp
  detail/pipeline_factory.cpp
  detail/pipeline_factory.hpp
  detail/renderer.cpp
  detail/renderer.hpp
  detail/rotator.hpp
  detail/shared_impl.hpp
  detail/trace.cpp
  detail/trace.hpp
  detail/vk_device.hpp
  detail/vk_instance.cpp
  detail/vk_instance.hpp
  detail/vk_surface.cpp
  detail/vk_surface.hpp
  detail/vram.cpp
  detail/vram.hpp

  spir_v/default.frag.hpp
  spir_v/default.vert.hpp
)
