set(ext_targets "")
set(BUILD_SHARED_LIBS OFF)

# ktl
if(VULKIFY_INSTALL)
  set(KTL_INSTALL ON)

  if(NOT KTL_INSTALL)
    message(WARNING "Invalid INSTALL flags")
  endif()
endif()

include(FetchContent)
FetchContent_Declare(ktl
  GIT_REPOSITORY https://github.com/karnkaul/ktl
  GIT_TAG 66d3807 # manually update this to keep commits pinned
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/src/ktl"
)
FetchContent_MakeAvailable(ktl)

file(ARCHIVE_EXTRACT
  INPUT "${CMAKE_CURRENT_SOURCE_DIR}/src.zip"
  DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}"
)

# glfw
set(GLFW_VULKAN_STATIC OFF)
set(GLFW_INSTALL ${VULKIFY_INSTALL})
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
add_subdirectory(src/glfw)

if(VULKIFY_INSTALL)
  export(EXPORT glfwTargets)
endif()

# dyvk
set(DYVK_INSTALL ${VULKIFY_INSTALL})
set(DYVK_VK_BETA ON)
add_subdirectory(src/dyvk)

# glm
set(GLM_INSTALL ${VULKIFY_INSTALL})
add_subdirectory(src/glm)
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

# freetype
add_subdirectory(src/freetype)

if(VULKIFY_INSTALL)
  export(EXPORT freetype-targets)
endif()

if(VULKIFY_INSTALL)
  install(TARGETS ${ext_targets} EXPORT vulkify-ext-targets)
  install(EXPORT vulkify-ext-targets
    FILE vulkify-ext-targets.cmake
    NAMESPACE vulkify::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vulkify
  )
  export(EXPORT vulkify-ext-targets)
endif()
