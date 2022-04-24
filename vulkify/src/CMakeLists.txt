target_sources(${PROJECT_NAME} PRIVATE
  context/canvas.cpp
  context/context.cpp

  core/rgba.cpp

  instance/vf_instance.cpp

  pipeline/pipeline.cpp

  detail/canvas_impl.hpp
  detail/defer.hpp
  detail/image_cache.hpp
  detail/pipeline_factory.cpp
  detail/pipeline_factory.hpp
  detail/renderer.cpp
  detail/renderer.hpp
  detail/rotator.hpp
  detail/trace.cpp
  detail/trace.hpp
  detail/vk_instance.cpp
  detail/vk_instance.hpp
  detail/vk_surface.cpp
  detail/vk_surface.hpp
  detail/vram.cpp
  detail/vram.hpp
)
