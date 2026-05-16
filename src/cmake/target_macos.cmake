function(omodsim_configure_target_macos target_name)
    set(MACOSX_COMPOSER_ICON_NAME "omodsim")
    set(MACOSX_COMPOSER_ICON "${CMAKE_CURRENT_SOURCE_DIR}/res/${MACOSX_COMPOSER_ICON_NAME}.icon")
    set(MACOSX_BUNDLE_ICON_FILE "${MACOSX_COMPOSER_ICON_NAME}")
    set(MACOSX_ASSET_ICON_PLIST_ENTRY "")

    if(EXISTS "${MACOSX_COMPOSER_ICON}")
        file(GLOB_RECURSE MACOSX_COMPOSER_ICON_FILES
            CONFIGURE_DEPENDS
            "${MACOSX_COMPOSER_ICON}/*"
        )

        find_program(ACTOOL_EXECUTABLE actool REQUIRED)
        set(MACOSX_ACTOOL_SCRIPT "${PROJECT_BINARY_DIR}/run_actool.cmake")

        file(WRITE "${MACOSX_ACTOOL_SCRIPT}" [=[
execute_process(
    COMMAND "${ACTOOL_EXECUTABLE}"
        "${MACOSX_COMPOSER_ICON}"
        --app-icon "${MACOSX_COMPOSER_ICON_NAME}"
        --compile "${MACOSX_ICON_OUTPUT_DIR}"
        --output-partial-info-plist "${MACOSX_ICON_PARTIAL_INFO_PLIST}"
        --minimum-deployment-target "11.0"
        --platform macosx
        --target-device mac
        --include-all-app-icons
    RESULT_VARIABLE ACTOOL_RESULT
    OUTPUT_FILE "${MACOSX_ICON_ACTOOL_OUTPUT}"
)

if(NOT ACTOOL_RESULT EQUAL 0)
    file(READ "${MACOSX_ICON_ACTOOL_OUTPUT}" ACTOOL_OUTPUT)
    message(FATAL_ERROR
        "actool failed with exit code ${ACTOOL_RESULT}\n"
        "${ACTOOL_OUTPUT}"
    )
endif()
]=])

        set(MACOSX_ASSET_ICON_PLIST_ENTRY
"    <key>CFBundleIconName</key>
    <string>${MACOSX_COMPOSER_ICON_NAME}</string>")

        set(MACOSX_ICON_OUTPUT_DIR "${PROJECT_BINARY_DIR}/${target_name}.app/Contents/Resources")

        add_custom_target(${target_name}_macos_appicon ALL
            COMMAND "${CMAKE_COMMAND}" -E make_directory
                "${MACOSX_ICON_OUTPUT_DIR}"
            COMMAND "${CMAKE_COMMAND}" -E remove_directory
                "${MACOSX_ICON_OUTPUT_DIR}/${MACOSX_COMPOSER_ICON_NAME}.icon"
            COMMAND "${CMAKE_COMMAND}"
                -DACTOOL_EXECUTABLE="${ACTOOL_EXECUTABLE}"
                -DMACOSX_COMPOSER_ICON="${MACOSX_COMPOSER_ICON}"
                -DMACOSX_COMPOSER_ICON_NAME="${MACOSX_COMPOSER_ICON_NAME}"
                -DMACOSX_ICON_OUTPUT_DIR="${MACOSX_ICON_OUTPUT_DIR}"
                -DMACOSX_ICON_PARTIAL_INFO_PLIST="${PROJECT_BINARY_DIR}/assetcatalog_generated_info.plist"
                -DMACOSX_ICON_ACTOOL_OUTPUT="${PROJECT_BINARY_DIR}/actool_output.plist"
                -P "${MACOSX_ACTOOL_SCRIPT}"
            DEPENDS ${MACOSX_COMPOSER_ICON_FILES}
            COMMENT "Synchronizing macOS Icon Composer app icon assets"
        )
        add_dependencies(${target_name}_macos_appicon ${target_name})
    else()
        message(FATAL_ERROR
            "No macOS app icon found. Expected ${MACOSX_COMPOSER_ICON}."
        )
    endif()

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/omodsim.plist.in"
        "${PROJECT_BINARY_DIR}/Info.plist"
        @ONLY
    )

    set_target_properties(${target_name} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST "${PROJECT_BINARY_DIR}/Info.plist"
        MACOSX_BUNDLE_ICON_FILE "${MACOSX_BUNDLE_ICON_FILE}"
        MACOSX_BUNDLE_BUNDLE_NAME "${PRODUCT_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endfunction()
