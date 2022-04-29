target_sources(${PROJECT_NAME} PRIVATE
  include/vulkify/core/defines.hpp
  include/vulkify/core/float_eq.hpp
  include/vulkify/core/geometry.hpp
  include/vulkify/core/nvec.hpp
  include/vulkify/core/per_thread.hpp
  include/vulkify/core/ptr.hpp
  include/vulkify/core/radian.hpp
  include/vulkify/core/result.hpp
  include/vulkify/core/rgba.hpp
  include/vulkify/core/time.hpp
  include/vulkify/core/transform.hpp
  include/vulkify/core/unique.hpp

  include/vulkify/context/context.hpp

  include/vulkify/graphics/buffer.hpp
  include/vulkify/graphics/draw_model.hpp
  include/vulkify/graphics/draw_object.hpp
  include/vulkify/graphics/spir_v.hpp
  include/vulkify/graphics/surface.hpp
  include/vulkify/graphics/pipeline_state.hpp

  include/vulkify/instance/event_type.hpp
  include/vulkify/instance/event.hpp
  include/vulkify/instance/gpu.hpp
  include/vulkify/instance/headless_instance.hpp
  include/vulkify/instance/key_event.hpp
  include/vulkify/instance/instance.hpp
  include/vulkify/instance/vf_instance.hpp
)
