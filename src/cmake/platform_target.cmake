function(omodsim_configure_platform_target target_name)
    if(WIN32)
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
    elseif(APPLE)
        set(MACOSX_ICON "${CMAKE_CURRENT_SOURCE_DIR}/res/omodsim.icns")
        set_source_files_properties(${MACOSX_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        target_sources(${target_name} PRIVATE ${MACOSX_ICON})

        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/omodsim.plist.in"
            "${PROJECT_BINARY_DIR}/Info.plist"
            @ONLY
        )

        set_target_properties(${target_name} PROPERTIES
            MACOSX_BUNDLE ON
            MACOSX_BUNDLE_INFO_PLIST "${PROJECT_BINARY_DIR}/Info.plist"
            MACOSX_BUNDLE_ICON_FILE omodsim.icns
            MACOSX_BUNDLE_BUNDLE_NAME "${PRODUCT_NAME}"
            MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        )
    elseif(LINUX)
        target_link_options(${target_name} PRIVATE -static-libgcc -static-libstdc++)
    endif()
endfunction()
