include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# usage: get_get_commit_hash(<cache_var>)
# sets ${cache_var} to commit hash at HEAD, or "unknown"
function(git_get_commit_hash OUTPUT_VAR)
    set(git_commit_hash "[unknown]")
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE git_commit_hash
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(${OUTPUT_VAR} ${git_commit_hash} CACHE STRING "Git commit hash")
    message(STATUS "Git commit hash [${git_commit_hash}] saved to [${OUTPUT_VAR}]")
endfunction()

# usage: git_update_submodules([message_type=WARNING])
# calls git submodule update --init --recursive, outputs message(${message_type}) on failure
function(git_update_submodules)
    set(msg_type WARNING)

    if(${ARGC} GREATER 0)
        set(msg_type ${ARGV0})
    endif()

    message(STATUS "Updating git submodules...")
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE UPDATE_SUBMODULES_RESULT
    )

    if(NOT UPDATE_SUBMODULES_RESULT EQUAL "0")
        message(${msg_type} "git submodule update failed!")
    endif()
endfunction()

# usage: unzip_archive(ARCHIVE <name> SUBDIR <subdir>)
# unzips ${subdir}/${name} at ${subdir}, hard error if file doesn't exist
function(unzip_archive)
    cmake_parse_arguments(args "" "ARCHIVE;SUBDIR" "" ${ARGN})

    if(args_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid args: ${args_UNPARSED_ARGUMENTS}")
    endif()

    if(args_MISSING_KEYWORDS)
        message(FATAL_ERROR "Args missing keywords: ${args_MISSING_KEYWORDS}")
    endif()

    if(NOT EXISTS "${args_SUBDIR}/${args_ARCHIVE}")
        message(FATAL_ERROR "Required archvive(s) missing!\n${args_SUBDIR}/${args_ARCHIVE}")
    endif()

    message(STATUS "Extracting ${args_ARCHIVE}...")
    execute_process(COMMAND
        ${CMAKE_COMMAND} -E tar -xf "${args_ARCHIVE}"
        WORKING_DIRECTORY "${args_SUBDIR}"
    )
endfunction()

# usage: target_source_group(TARGET <name> [PREFIX])
# sets source group on all sources using current source dir; optionally with prefix
function(target_source_group)
    cmake_parse_arguments(args "" "TARGET;PREFIX" "" ${ARGN})

    if(args_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid args: ${args_UNPARSED_ARGUMENTS}")
    endif()

    if("${args_TARGET}" STREQUAL "")
        message(FATAL_ERROR "Missing required args: TARGET")
    endif()

    get_target_property(sources ${args_TARGET} SOURCES)

    if(NOT "${args_PREFIX}" STREQUAL "")
        source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX ${args_PREFIX} FILES ${sources})
    else()
        source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})
    endif()
endfunction()

# usage: configure_src_bin(IN <src> OUT <dst> [TARGET target])
# configures CURRENT_SOURCE_DIR/${src} to CURRENT_BINARY_DIR/${dst}
# if target is set, adds dst to its target_sources, and source_group
function(configure_src_bin)
    cmake_parse_arguments(args "" "IN;OUT;TARGET" "" ${ARGN})

    if(args_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid args: ${args_UNPARSED_ARGUMENTS}")
    endif()

    if(args_MISSING_KEYWORDS)
        message(FATAL_ERROR "Args missing keywords: ${args_MISSING_KEYWORDS}")
    endif()

    set(dst "${CMAKE_CURRENT_BINARY_DIR}/include/${args_OUT}")
    message(STATUS "Configuring ${dst}")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${args_IN}" "${dst}")

    if(NOT "${args_TARGET}" STREQUAL "")
        message(STATUS "Adding ${args_OUT} to ${args_TARGET} sources")
        target_sources(${args_TARGET} PRIVATE "${dst}")
        source_group(TREE "${CMAKE_CURRENT_BINARY_DIR}" FILES "${dst}")
    endif()
endfunction()

# usage: install_targets(TARGETS <target_names...> EXPORT <export_name>)
# installs ${target_names}... and exports under export_name
function(install_targets)
    cmake_parse_arguments(args "" "EXPORT" "TARGETS" ${ARGN})

    if(args_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid args: ${args_UNPARSED_ARGUMENTS}")
    endif()

    if(args_MISSING_KEYWORDS)
        message(FATAL_ERROR "Args missing keywords: ${args_MISSING_KEYWORDS}")
    endif()

    message(STATUS "Install ${args_TARGETS} to ${args_EXPORT} export set")

    # install and export targets
    install(TARGETS ${args_TARGETS} EXPORT ${args_EXPORT}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endfunction()

# usage: install_target_headers()
# installs include/*.hpp and CURRENT_BINARY_DIR/include/*.hpp
function(install_target_headers)
    message(STATUS "Install *.hpp from ${CMAKE_CURRENT_SOURCE_DIR}/include and ${CMAKE_CURRENT_BINARY_DIR}/include")

    # install headers from include
    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp"
    )

    # install generated headers
    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp"
    )
endfunction()

# usage: install_export_package(EXPORT <set> NAMESPACE <namespace> PACKAGE <package_name>)
# installs ${set} as ${set}.cmake under ${namespace} to ${package_name}
function(install_export_package)
    cmake_parse_arguments(args "" "EXPORT;NAMESPACE;PACKAGE" "" ${ARGN})

    if(args_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid args: ${args_UNPARSED_ARGUMENTS}")
    endif()

    if(args_MISSING_KEYWORDS)
        message(FATAL_ERROR "Args missing keywords: ${args_MISSING_KEYWORDS}")
    endif()

    if("${args_PACKAGE}" STREQUAL "" OR "${args_NAMESPACE}" STREQUAL "" OR "${args_EXPORT}" STREQUAL "")
        message(FATAL_ERROR "Missing required args: PACKAGE;NAMESPACE;EXPORT")
    endif()

    message(STATUS "Install export set ${args_EXPORT}.cmake under ${args_NAMESPACE}:: to package ${args_PACKAGE}")
    install(EXPORT ${args_EXPORT}
        FILE ${args_EXPORT}.cmake
        NAMESPACE ${args_NAMESPACE}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${args_PACKAGE}
    )
endfunction()

# usage: install_package_config(PACKAGE <package_name> [IN=config.cmake.in])
# configures and installs package configuration file ${filename} to CURRENT_BINARY_DIR/${package_name}-config.cmake
function(install_package_config)
    cmake_parse_arguments(args "" "IN;PACKAGE" "" ${ARGN})

    if(args_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid args: ${args_UNPARSED_ARGUMENTS}")
    endif()

    if("${args_PACKAGE}" STREQUAL "")
        message(FATAL_ERROR "Missing required args: PACKAGE")
    endif()

    if("${args_IN}" STREQUAL "")
        set(args_IN config.cmake.in)
    endif()

    message(STATUS "Install ${args_PACKAGE}-config.cmake to ${CMAKE_INSTALL_LIBDIR}/cmake/${args_PACKAGE}")

    # configure ${args_PACKAGE}-config.cmake
    configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/${args_IN}"
        "${CMAKE_CURRENT_BINARY_DIR}/${args_PACKAGE}-config.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${args_PACKAGE}
    )

    # install ${args_PACKAGE}-config.cmake
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${args_PACKAGE}-config.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${args_PACKAGE}
    )
endfunction()

# usage: install_package_version(PACKAGE <package_name> VERSION <version>)
# configures and installs package configuration version file to CURRENT_BINARY_DIR/${package_name}-version.cmake
function(install_package_version)
    cmake_parse_arguments(args "" "VERSION;PACKAGE" "" ${ARGN})

    if(args_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid args: ${args_UNPARSED_ARGUMENTS}")
    endif()

    if(args_MISSING_KEYWORDS)
        message(FATAL_ERROR "Args missing keywords: ${args_MISSING_KEYWORDS}")
    endif()

    if("${args_PACKAGE}" STREQUAL "")
        message(FATAL_ERROR "Missing required args: PACKAGE")
    endif()

    message(STATUS "Install ${args_PACKAGE}-config-version.cmake to ${CMAKE_INSTALL_LIBDIR}/cmake/${args_PACKAGE}")

    # configure ${args_PACKAGE}-version.cmake
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${args_PACKAGE}-config-version.cmake"
        VERSION ${args_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    # install ${args_PACKAGE}-config.cmake, ${args_PACKAGE}-version.cmake
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${args_PACKAGE}-config-version.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${args_PACKAGE}
    )
endfunction()

# usage: export_package_to_build_tree(PACKAGE <package_name> NAMESPACE <namespace>)
# exports ${package_name} to CURRENT_BINARY_DIR/${package_name}-targets.cmake
function(export_package_to_build_tree)
    cmake_parse_arguments(args "" "NAMESPACE;PACKAGE" "" ${ARGN})

    if(args_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid args: ${args_UNPARSED_ARGUMENTS}")
    endif()

    if(args_MISSING_KEYWORDS)
        message(FATAL_ERROR "Args missing keywords: ${args_MISSING_KEYWORDS}")
    endif()

    if("${args_PACKAGE}" STREQUAL "")
        message(FATAL_ERROR "Missing required args: PACKAGE")
    endif()

    if("${args_NAMESPACE}" STREQUAL "")
        set(args_NAMESPACE ${args_PACKAGE})
    endif()

    message(STATUS "Exporting ${args_PACKAGE}-targets.cmake under ${args_NAMESPACE}:: to ${CMAKE_CURRENT_BINARY_DIR}")

    # export targets to current build tree
    export(EXPORT ${args_PACKAGE}-targets
        FILE "${CMAKE_CURRENT_BINARY_DIR}/${args_PACKAGE}-targets.cmake"
        NAMESPACE ${args_NAMESPACE}::
    )
endfunction()

# usage: install_and_export_target([NOBINEXPORT] [NOHEADERS] TARGET <target_name> [NAMESPACE=target_name] [VERSION <ver>])
# installs and exports ${target_name}, with headers unless NOHEADERS and to CURRENT_BINARY_DIR unless NOBINEXPORT
# installs package configuration version if VERSION is passed
function(install_and_export_target)
    cmake_parse_arguments(args "NOBINEXPORT;NOHEADERS" "TARGET;NAMESPACE;VERSION;PACKAGE" "" ${ARGN})

    if(args_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid args: ${args_UNPARSED_ARGUMENTS}")
    endif()

    if("${args_TARGET}" STREQUAL "")
        message(FATAL_ERROR "Missing required args: TARGET")
    endif()

    if("${args_NAMESPACE}" STREQUAL "")
        set(args_NAMESPACE ${args_TARGET})
    endif()

    if("${args_PACKAGE}" STREQUAL "")
        set(args_PACKAGE ${args_TARGET})
    endif()

    install_targets(TARGETS ${args_TARGET} EXPORT ${args_PACKAGE}-targets)

    if(NOT args_NOHEADERS)
        install_target_headers()
    endif()

    install_export_package(EXPORT ${args_PACKAGE}-targets NAMESPACE ${args_NAMESPACE} PACKAGE ${args_PACKAGE})
    install_package_config(PACKAGE ${args_PACKAGE})

    if(NOT "${args_VERSION}" STREQUAL "")
        install_package_version(PACKAGE ${args_PACKAGE} VERSION ${args_VERSION})
    endif()

    if(NOT args_NOBINEXPORT)
        export_package_to_build_tree(PACKAGE ${args_PACKAGE} NAMESPACE ${args_NAMESPACE})
    endif()
endfunction()
