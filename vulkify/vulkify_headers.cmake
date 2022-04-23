target_sources(${PROJECT_NAME} PRIVATE
  include/vulkify/core/ptr.hpp
  include/vulkify/core/result.hpp
  include/vulkify/core/unique.hpp

  include/vulkify/context/context.hpp

  include/vulkify/instance/event_type.hpp
  include/vulkify/instance/event.hpp
  include/vulkify/instance/gpu.hpp
  include/vulkify/instance/headless_instance.hpp
  include/vulkify/instance/key_event.hpp
  include/vulkify/instance/instance.hpp
  include/vulkify/instance/vf_instance.hpp
)
