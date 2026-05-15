# macOS installation rules
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/Applications" CACHE PATH "Install path prefix" FORCE)
endif()

install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION .)

install(CODE "
    find_program(_macdeployqt_executable macdeployqt
        HINTS
            \"${QT_BINARY_DIR}\"
            \"${QT_LIBEXEC_DIR}\"
            \"${QT_INSTALL_LIBEXECS}\"
    )

    set(_installed_app \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app\")
    set(_resources_dir \"\${_installed_app}/Contents/Resources\")
    set(_docs_source \"${PROJECT_BINARY_DIR}/docs\")
    set(_demos_source \"${CMAKE_CURRENT_SOURCE_DIR}/../demos\")

    if(NOT EXISTS \"\${_installed_app}\")
        message(FATAL_ERROR \"Installed app bundle not found: \${_installed_app}\")
    endif()

    file(MAKE_DIRECTORY \"\${_resources_dir}\")

    if(EXISTS \"\${_docs_source}\")
        execute_process(
            COMMAND \"${CMAKE_COMMAND}\" -E copy_directory
                    \"\${_docs_source}\"
                    \"\${_resources_dir}/docs\"
            RESULT_VARIABLE _copy_docs_result
        )
        if(NOT _copy_docs_result EQUAL 0)
            message(FATAL_ERROR \"Failed to copy docs into app bundle\")
        endif()
    endif()

    if(EXISTS \"\${_demos_source}\")
        execute_process(
            COMMAND \"${CMAKE_COMMAND}\" -E copy_directory
                    \"\${_demos_source}\"
                    \"\${_resources_dir}/demos\"
            RESULT_VARIABLE _copy_demos_result
        )
        if(NOT _copy_demos_result EQUAL 0)
            message(FATAL_ERROR \"Failed to copy demos into app bundle\")
        endif()
    endif()

    if(_macdeployqt_executable)
        message(STATUS \"macdeployqt found at: \${_macdeployqt_executable}\")
        execute_process(
            COMMAND \"\${_macdeployqt_executable}\"
                    \"\${_installed_app}\"
                    -always-overwrite
            RESULT_VARIABLE _macdeployqt_result
        )
        if(NOT _macdeployqt_result EQUAL 0)
            message(FATAL_ERROR \"macdeployqt failed with exit code \${_macdeployqt_result}\")
        endif()
    else()
        message(WARNING \"macdeployqt not found - macOS install will not deploy Qt runtime\")
    endif()
")
