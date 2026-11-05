get_target_property(OMODSIM_CORE_SOURCES omodsim_core SOURCES)
get_target_property(OMODSIM_UI_SOURCES omodsim_ui SOURCES)
set(OMODSIM_TRANSLATION_SOURCES)
foreach(source IN LISTS OMODSIM_CORE_SOURCES)
    list(APPEND OMODSIM_TRANSLATION_SOURCES "${PROJECT_SOURCE_DIR}/core/${source}")
endforeach()
foreach(source IN LISTS OMODSIM_UI_SOURCES)
    list(APPEND OMODSIM_TRANSLATION_SOURCES "${PROJECT_SOURCE_DIR}/ui/${source}")
endforeach()

set(TS_FILES
    translations/omodsim_it.ts
    translations/omodsim_ru.ts
    translations/omodsim_zh_CN.ts
    translations/omodsim_zh_TW.ts
)

if(Qt6_FOUND)
    qt_add_lupdate(${PROJECT_NAME}
        TS_FILES ${TS_FILES}
        SOURCES ${OMODSIM_TRANSLATION_SOURCES}
        OPTIONS -no-obsolete
    )
else()
    add_custom_command(
        OUTPUT ${TS_FILES}
        COMMAND ${Qt5_LUPDATE_EXECUTABLE}
                ${OMODSIM_TRANSLATION_SOURCES}
                -no-obsolete
                -ts ${TS_FILES}
        DEPENDS ${OMODSIM_TRANSLATION_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Updating translation files..."
    )
    add_custom_target(update_translations ALL DEPENDS ${TS_FILES})
endif()

if(TARGET update_translations)
    add_dependencies(${PROJECT_NAME} update_translations)
endif()
