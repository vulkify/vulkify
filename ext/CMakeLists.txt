set(ext_targets "")

# glfw
set(GLFW_VULKAN_STATIC OFF)
set(GLFW_INSTALL ${VULKIFY_INSTALL})
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw
  GIT_TAG 3.3.7
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/glfw"
)
FetchContent_MakeAvailable(glfw)

if(VULKIFY_INSTALL)
  export(EXPORT glfwTargets)
endif()

# dyvk
set(DYVK_INSTALL ${VULKIFY_INSTALL})
FetchContent_Declare(
  dyvk
  GIT_REPOSITORY https://github.com/karnkaul/dyvk
  GIT_TAG 036c72d
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/dyvk"
)
FetchContent_MakeAvailable(dyvk)

# vk-bootstrap
set(VK_BOOTSTRAP_VULKAN_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/dyvk/include" CACHE STRING "" FORCE)

if(NOT EXISTS "${VK_BOOTSTRAP_VULKAN_HEADER_DIR}/vulkan/vulkan.h")
  message(FATAL_ERROR "Missing vulkan headers")
endif()

FetchContent_Declare(
  vk-bootstrap
  GIT_REPOSITORY https://github.com/karnkaul/vk-bootstrap
  GIT_TAG 3480e74
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/vk-bootstrap"
)
FetchContent_MakeAvailable(vk-bootstrap)

if(VULKIFY_INSTALL)
  install(EXPORT vk-bootstrap-targets
    FILE vk-bootstrap-targets.cmake
    NAMESPACE vk-bootstrap::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )
  export(EXPORT vk-bootstrap-targets)
endif()

# glm
set(GLM_INSTALL ${VULKIFY_INSTALL})
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/karnkaul/glm
  GIT_TAG 9ee2352f
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/glm"
)
FetchContent_MakeAvailable(glm)
target_compile_options(glm INTERFACE
  $<$<OR:$<BOOL:${LINUX_GCC}>,$<BOOL:${WINDOWS_GCC}>>:-Wno-volatile>
  $<$<OR:$<BOOL:${LINUX_CLANG}>,$<BOOL:${WINDOWS_CLANG}>>:-Wno-deprecated-volatile>
)
list(APPEND ext_targets glm)

# vma
add_subdirectory(src/vma)
list(APPEND ext_targets vma)

# stb
add_subdirectory(src/stb)
list(APPEND ext_targets stb-image)

if(VULKIFY_INSTALL)
  install(TARGETS ${ext_targets} EXPORT vulkify-ext-targets)
  install(EXPORT vulkify-ext-targets
    FILE vulkify-ext-targets.cmake
    NAMESPACE vulkify::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vulkify
  )
  export(EXPORT vulkify-ext-targets)
endif()
