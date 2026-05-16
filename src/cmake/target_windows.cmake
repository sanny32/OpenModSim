function(omodsim_configure_target_windows target_name)
    if(NOT WIN32)
        return()
    endif()

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /utf-8)
    endif()

    set(FILE_VERSION "${PROJECT_VERSION}")
    set(PRODUCT_VERSION "${PROJECT_VERSION}")
    set(ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/res/omodsim.ico")
    set(PROJECT_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/res/omodsim-project.ico")

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/omodsim.rc.in"
        "${PROJECT_BINARY_DIR}/omodsim.rc"
        @ONLY
    )

    target_sources(${target_name} PRIVATE "${PROJECT_BINARY_DIR}/omodsim.rc")
    set_target_properties(${target_name} PROPERTIES WIN32_EXECUTABLE ON)
endfunction()
