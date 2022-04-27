project(${project_name}-interface)

if("${cxx_standard}" STREQUAL "")
  set(cxx_standard cxx_std_20)
endif()

# Interface
add_library(interface INTERFACE)
add_library(${project_name}::interface ALIAS interface)

if(LINUX_GCC OR LINUX_CLANG OR WIN64_GCC OR WIN64_CLANG)
  set(gcc_like TRUE) # indicates gcc-like compiler
else()
  set(gcc_like FALSE)
endif()

target_compile_definitions(interface INTERFACE
  $<$<CONFIG:Debug>:VULKIFY_DEBUG>
  $<$<BOOL:${MSVC_RUNTIME}>:WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS>
)
target_compile_features(interface INTERFACE ${cxx_standard})

# options
add_library(options INTERFACE)
add_library(${project_name}::options ALIAS options)

target_compile_options(options INTERFACE
  $<$<BOOL:${gcc_like}>:-Wall -Wextra -Wunused -Werror=return-type -Wconversion $<$<NOT:$<CONFIG:Debug>>:-Werror>>
  $<$<BOOL:${WIN64_MSBUILD}>:/MP $<$<NOT:$<CONFIG:Debug>>:/WX>>
)
target_link_libraries(options INTERFACE ${project_name}::interface)
