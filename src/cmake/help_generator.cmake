function(omodsim_configure_help_generator target_name)
    set(JSHELP_INPUT "${PROJECT_SOURCE_DIR}/docs/jshelp.qhcp")
    set(JSHELP_QHCP "${PROJECT_BINARY_DIR}/docs/jshelp.qhcp")
    set(JSHELP_QHP "${PROJECT_SOURCE_DIR}/docs/jshelp.qhp")
    set(JSHELP_QCH "${PROJECT_BINARY_DIR}/docs/jshelp.qch")
    set(JSHELP_QHC "${PROJECT_BINARY_DIR}/docs/jshelp.qhc")

    file(GLOB JSHELP_CONTENT
        "${PROJECT_SOURCE_DIR}/docs/*.html"
        "${PROJECT_SOURCE_DIR}/docs/assets/css/*.css"
    )

    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/docs)

    find_program(QHELP_GENERATOR_EXECUTABLE qhelpgenerator
                 HINTS "${QT_BINARY_DIR}" "${QT_LIBEXEC_DIR}" "${QT_INSTALL_LIBEXECS}")

    if(NOT QHELP_GENERATOR_EXECUTABLE)
        # Homebrew on macOS installs qhelpgenerator in share/qt/libexec
        get_filename_component(_qt_share_libexec "${QT_BINARY_DIR}/../share/qt/libexec" REALPATH)
        find_program(QHELP_GENERATOR_EXECUTABLE qhelpgenerator
                     HINTS "${_qt_share_libexec}"
                           "${CMAKE_PREFIX_PATH}/share/qt/libexec"
                           "${CMAKE_PREFIX_PATH}/libexec")
    endif()

    if(NOT QHELP_GENERATOR_EXECUTABLE)
        find_program(QHELP_GENERATOR_EXECUTABLE qhelpgenerator)
    endif()

    if(QHELP_GENERATOR_EXECUTABLE)
        message(STATUS "qhelpgenerator found at: ${QHELP_GENERATOR_EXECUTABLE}")
    else()
        message(FATAL_ERROR "qhelpgenerator not found")
    endif()

    add_custom_command(
        OUTPUT ${JSHELP_QCH} ${JSHELP_QHC}
        COMMAND ${CMAKE_COMMAND} -E copy ${JSHELP_INPUT} ${JSHELP_QHCP}
        COMMAND ${QHELP_GENERATOR_EXECUTABLE} ${JSHELP_QHP} -o ${JSHELP_QCH}
        COMMAND ${QHELP_GENERATOR_EXECUTABLE} ${JSHELP_QHCP} -o ${JSHELP_QHC}
        COMMAND ${CMAKE_COMMAND} -E remove ${JSHELP_QHCP}
        DEPENDS ${JSHELP_QHP} ${JSHELP_INPUT} ${JSHELP_CONTENT}
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/docs
        COMMENT "Generating and moving help files..."
        VERBATIM
    )

    add_custom_target(helpgenerator ALL DEPENDS ${JSHELP_QCH} ${JSHELP_QHC})
    add_dependencies(${target_name} helpgenerator)
endfunction()
