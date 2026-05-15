# Windows installation rules
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "C:/Program Files/Open ModSim ${VERSION_MAJOR}" CACHE PATH "Install path prefix" FORCE)
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../demos" DESTINATION .)
install(DIRECTORY "${PROJECT_BINARY_DIR}/docs" DESTINATION .)

if(WINDEPLOYQT_EXECUTABLE)
    install(CODE "
        set(_installed_exe \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/omodsim.exe\")
        get_filename_component(_installed_exe_dir \"\${_installed_exe}\" DIRECTORY)
        set(_plugin_dir \"\${_installed_exe_dir}/plugins\")

        if(NOT EXISTS \"\${_installed_exe}\")
            message(FATAL_ERROR \"Installed executable not found: \${_installed_exe}\")
        endif()

        if(CMAKE_INSTALL_CONFIG_NAME MATCHES \"^[Dd]ebug$\")
            set(_deploy_mode --debug)
        else()
            set(_deploy_mode --release)
        endif()

        set(_windeploy_args
            \${_deploy_mode}
            --plugindir \"\${_plugin_dir}\"
            --no-opengl-sw
            --no-system-d3d-compiler
        )

        if(\"${Qt6_FOUND}\" STREQUAL \"TRUE\")
            list(APPEND _windeploy_args
                 --no-system-dxc-compiler
                 --skip-plugin-types help,generic,networkinformation,qmltooling,tls
                 --exclude-plugins qgif,qjpeg,qpdf,qsqlibase,qsqlmimer,qsqloci,qsqlodbc,qsqlpsql)
        elseif(\"${Qt5_FOUND}\" STREQUAL \"TRUE\")
            list(APPEND _windeploy_args
                 --no-angle
                 --no-quick)
        endif()

        list(APPEND _windeploy_args \"\${_installed_exe}\")

        execute_process(
            COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" \${_windeploy_args}
            RESULT_VARIABLE _windeployqt_result
        )

        if(NOT _windeployqt_result EQUAL 0)
            message(FATAL_ERROR \"windeployqt failed with exit code \${_windeployqt_result}\")
        endif()

        set(_qml_dir \"\${_installed_exe_dir}/qml\")
        if(EXISTS \"\${_qml_dir}\")
            file(GLOB _qml_entries \"\${_qml_dir}/*\")
            if(_qml_entries)
                message(STATUS \"Keeping non-empty qml directory: \${_qml_dir}\")
            else()
                message(STATUS \"Removing empty qml directory: \${_qml_dir}\")
                file(REMOVE_RECURSE \"\${_qml_dir}\")
            endif()
        endif()
    ")
endif()
