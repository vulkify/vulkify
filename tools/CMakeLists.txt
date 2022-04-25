option(VULKIFY_TOOLS_SHABU "Build shabu (shader burner)" ON)

if(VULKIFY_TOOLS_SHABU)
  add_subdirectory(shabu)
endif()
