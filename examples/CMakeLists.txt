add_executable(vulkify-quick-start)
target_link_libraries(vulkify-quick-start PRIVATE vulkify::vulkify vulkify::options)
target_sources(vulkify-quick-start PRIVATE quick_start.cpp)
