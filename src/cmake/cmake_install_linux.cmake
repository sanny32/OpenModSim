# Linux installation rules
set(APP_NAME "${PRODUCT_NAME} ${VERSION_MAJOR}")
set(EXECUTABLE_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME}${VERSION_MAJOR}")
set(APP_EXECUTABLE_NAME "${PROJECT_NAME}${VERSION_MAJOR}")
set(APP_ICON "${APP_EXECUTABLE_NAME}")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/omodsim.desktop.in"
    "${CMAKE_CURRENT_BINARY_DIR}/omodsim.desktop"
    @ONLY
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../demos/" DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/demos")
install(DIRECTORY "${PROJECT_BINARY_DIR}/docs/" DESTINATION "${CMAKE_INSTALL_DOCDIR}")

install(PROGRAMS "$<TARGET_FILE:${PROJECT_NAME}>" DESTINATION bin RENAME "${APP_EXECUTABLE_NAME}")
install(FILES res/omodsim16.png DESTINATION share/icons/hicolor/16x16/apps RENAME "${APP_ICON}.png")
install(FILES res/omodsim32.png DESTINATION share/icons/hicolor/32x32/apps RENAME "${APP_ICON}.png")
install(FILES res/omodsim64.png DESTINATION share/icons/hicolor/64x64/apps RENAME "${APP_ICON}.png")
install(FILES res/omodsim128.png DESTINATION share/icons/hicolor/128x128/apps RENAME "${APP_ICON}.png")
install(FILES res/omodsim-project16.png DESTINATION share/icons/hicolor/16x16/mimetypes RENAME omodsim-project.png)
install(FILES res/omodsim-project32.png DESTINATION share/icons/hicolor/32x32/mimetypes RENAME omodsim-project.png)
install(FILES res/omodsim-project64.png DESTINATION share/icons/hicolor/64x64/mimetypes RENAME omodsim-project.png)
install(FILES res/omodsim-project128.png DESTINATION share/icons/hicolor/128x128/mimetypes RENAME omodsim-project.png)
install(FILES res/omodsim-project256.png DESTINATION share/icons/hicolor/256x256/mimetypes RENAME omodsim-project.png)
install(FILES res/omodsim-project.svg DESTINATION share/icons/hicolor/scalable/mimetypes RENAME omodsim-project.svg)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/omodsim-project.xml" DESTINATION share/mime/packages)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/omodsim.desktop" DESTINATION share/applications RENAME "${APP_EXECUTABLE_NAME}.desktop")

find_program(SETCAP_EXECUTABLE setcap)
if(SETCAP_EXECUTABLE)
    install(CODE "
        set(_installed_exe \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${APP_EXECUTABLE_NAME}\")
        if(NOT EXISTS \"\${_installed_exe}\")
            message(FATAL_ERROR \"Installed executable not found: \${_installed_exe}\")
        endif()

        execute_process(
            COMMAND \"${SETCAP_EXECUTABLE}\" cap_net_bind_service=+ep \"\${_installed_exe}\"
            RESULT_VARIABLE _setcap_result
        )

        if(NOT _setcap_result EQUAL 0)
            message(FATAL_ERROR \"setcap failed with exit code \${_setcap_result}\")
        endif()
    ")
endif()

install(CODE "execute_process(COMMAND update-mime-database ${CMAKE_INSTALL_PREFIX}/share/mime)")
install(CODE "execute_process(COMMAND xdg-desktop-menu forceupdate)")
install(CODE "execute_process(COMMAND update-desktop-database -q ${CMAKE_INSTALL_PREFIX}/share/applications)")
install(CODE "execute_process(COMMAND gtk-update-icon-cache -q -t -f ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor)")
