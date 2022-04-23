target_sources(${PROJECT_NAME} PRIVATE
  context/context.cpp

  detail/defer.hpp
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

  instance/vf_instance.cpp
)
