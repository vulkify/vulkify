target_sources(${PROJECT_NAME} PRIVATE
  include/vulkify/vulkify.hpp

  include/vulkify/core/defines.hpp
  include/vulkify/core/dirty_flag.hpp
  include/vulkify/core/float_eq.hpp
  include/vulkify/core/nvec.hpp
  include/vulkify/core/per_thread.hpp
  include/vulkify/core/ptr.hpp
  include/vulkify/core/radian.hpp
  include/vulkify/core/result.hpp
  include/vulkify/core/rect.hpp
  include/vulkify/core/rgba.hpp
  include/vulkify/core/time.hpp
  include/vulkify/core/transform.hpp
  include/vulkify/core/unique.hpp

  include/vulkify/context/builder.hpp
  include/vulkify/context/context.hpp
  include/vulkify/context/frame.hpp

  include/vulkify/graphics/atlas.hpp
  include/vulkify/graphics/bitmap.hpp
  include/vulkify/graphics/camera.hpp
  include/vulkify/graphics/descriptor_set.hpp
  include/vulkify/graphics/drawable.hpp
  include/vulkify/graphics/geometry.hpp
  include/vulkify/graphics/image.hpp
  include/vulkify/graphics/primitive.hpp
  include/vulkify/graphics/render_state.hpp
  include/vulkify/graphics/shader.hpp
  include/vulkify/graphics/surface.hpp

  include/vulkify/graphics/geometry_buffer.hpp
  include/vulkify/graphics/texture_handle.hpp
  include/vulkify/graphics/texture.hpp

  include/vulkify/graphics/primitives/all.hpp
  include/vulkify/graphics/primitives/circle_shape.hpp
  include/vulkify/graphics/primitives/mesh.hpp
  include/vulkify/graphics/primitives/prop.hpp
  include/vulkify/graphics/primitives/quad_shape.hpp
  include/vulkify/graphics/primitives/shape.hpp
  include/vulkify/graphics/primitives/sprite.hpp
  include/vulkify/graphics/primitives/text.hpp

  include/vulkify/graphics/detail/draw_model.hpp
  include/vulkify/graphics/detail/resource.hpp

  include/vulkify/instance/cursor.hpp
  include/vulkify/instance/event_queue.hpp
  include/vulkify/instance/event_type.hpp
  include/vulkify/instance/event.hpp
  include/vulkify/instance/gamepad.hpp
  include/vulkify/instance/gpu.hpp
  include/vulkify/instance/headless_instance.hpp
  include/vulkify/instance/icon.hpp
  include/vulkify/instance/instance_create_info.hpp
  include/vulkify/instance/instance_enums.hpp
  include/vulkify/instance/instance.hpp
  include/vulkify/instance/key_event.hpp
  include/vulkify/instance/monitor.hpp
  include/vulkify/instance/vf_instance.hpp
  include/vulkify/instance/video_mode.hpp

  include/vulkify/ttf/character.hpp
  include/vulkify/ttf/glyph.hpp
  include/vulkify/ttf/scribe.hpp
  include/vulkify/ttf/ttf.hpp
)
