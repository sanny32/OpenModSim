function(omodsim_configure_tests)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    if(Qt6_FOUND)
        find_package(Qt6 REQUIRED COMPONENTS Test)
    else()
        find_package(Qt5 REQUIRED COMPONENTS Test)
    endif()

    add_library(omodsim_testable STATIC
        modbuserrorsimulations.cpp
        modbusdataunitmap.cpp
        projectaddressspacexml.cpp
        modbusmultiserver.cpp
        modbusrtuserialserver.cpp
        modbusrtutcpserver.cpp
        modbusserver.cpp
        modbustcpserver.cpp
        datasimulator.cpp
        legacyprojectparser.cpp
        projectaddressspacefilter.cpp
        recentprojectsprompt.cpp
        qhexvalidator.cpp
        qintvalidatorex.cpp
        qint64validator.cpp
        quintvalidator.cpp
        qdoublevalidatorex.cpp
        modbusmessages/modbusmessage.cpp
        jsobjects/storage.cpp
        helpdockpolicy.cpp
        jsobjects/script.cpp
        jsobjects/server.cpp
        controls/consoleoutput.cpp
        controls/consoleoutput.ui
        controls/toolbar.cpp
        styles/themedicons.cpp
        styles/apptheme.cpp
        application.cpp
        apppreferences.cpp
        modbusmultiserver.cpp
        modbusserver.cpp
        modbusrtutcpserver.cpp
        modbustcpserver.cpp
        modbusrtuserialserver.cpp
    )

    target_include_directories(omodsim_testable PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/controls
        ${CMAKE_CURRENT_SOURCE_DIR}/modbusmessages
        ${CMAKE_CURRENT_SOURCE_DIR}/styles
    )

    target_link_libraries(omodsim_testable PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Network
        Qt::SerialBus
        Qt::SerialPort
        Qt::Qml
    )

    if(Qt6_FOUND)
        target_link_libraries(omodsim_testable PUBLIC Qt::Core5Compat)
    endif()

    if(MSVC)
        target_compile_options(omodsim_testable PUBLIC /utf-8)
    endif()

    omodsim_apply_coverage(omodsim_testable)
    set_target_properties(omodsim_testable PROPERTIES FOLDER "Tests")

    add_custom_target(omodsim_tests)
    set_target_properties(omodsim_tests PROPERTIES FOLDER "Tests")

    function(omodsim_add_test test_name source_file)
        add_executable(${test_name} tests/${source_file})
        target_link_libraries(${test_name} PRIVATE
            omodsim_testable
            Qt::Test
        )
        omodsim_apply_coverage(${test_name})
        set_target_properties(${test_name} PROPERTIES FOLDER "Tests")
        add_dependencies(omodsim_tests ${test_name})
        add_test(NAME ${test_name} COMMAND ${test_name})
    endfunction()

    omodsim_add_test(omodsim_tests_numericutils         test_numericutils.cpp)
    omodsim_add_test(omodsim_tests_byteorderutils       test_byteorderutils.cpp)
    omodsim_add_test(omodsim_tests_enums                test_enums.cpp)
    omodsim_add_test(omodsim_tests_modbusfunction       test_modbusfunction.cpp)
    omodsim_add_test(omodsim_tests_formatutils          test_formatutils.cpp)
    omodsim_add_test(omodsim_tests_errorsimulations     test_modbuserrorsimulations.cpp)
    omodsim_add_test(omodsim_tests_forcerangeparams     test_forcerangeparams.cpp)
    omodsim_add_test(omodsim_tests_validators           test_validators.cpp)
    omodsim_add_test(omodsim_tests_dataunitmap          test_modbusdataunitmap.cpp)
    omodsim_add_test(omodsim_tests_multiserver          test_modbusmultiserver.cpp)
    omodsim_add_test(omodsim_tests_datasimulator        test_datasimulator.cpp)
    omodsim_add_test(omodsim_tests_modbusmessage        test_modbusmessage.cpp)
    omodsim_add_test(omodsim_tests_modbusmessages       test_modbusmessages.cpp)
    omodsim_add_test(omodsim_tests_ansiutils            test_ansiutils.cpp)
    omodsim_add_test(omodsim_tests_serialportutils      test_serialportutils.cpp)
    omodsim_add_test(omodsim_tests_simulationparams     test_simulationparams.cpp)
    omodsim_add_test(omodsim_tests_storage              test_storage.cpp)
    omodsim_add_test(omodsim_tests_script               test_script.cpp)
    omodsim_add_test(omodsim_tests_consoleoutput        test_consoleoutput.cpp)
    omodsim_add_test(omodsim_tests_serverapi            test_server_api.cpp)
    omodsim_add_test(omodsim_tests_helpdockpolicy       test_helpdockpolicy.cpp)
    omodsim_add_test(omodsim_tests_legacyprojectparser  test_legacyprojectparser.cpp)
    omodsim_add_test(omodsim_tests_projectaddressspacefilter test_projectaddressspacefilter.cpp)
    omodsim_add_test(omodsim_tests_projectaddressspacexml test_projectaddressspacexml.cpp)
    omodsim_add_test(omodsim_tests_connectiondetails     test_connectiondetails.cpp)
    omodsim_add_test(omodsim_tests_recentprojectsprompt  test_recentprojectsprompt.cpp)
    omodsim_add_test(omodsim_tests_modbusserver          test_modbusserver.cpp)
    omodsim_add_test(omodsim_tests_modbustransports      test_modbustransports.cpp)

    set(omodsim_app_test_sources ${SOURCES})
    list(REMOVE_ITEM omodsim_app_test_sources main.cpp)
    add_executable(omodsim_tests_appproject
        tests/test_appproject.cpp
        resources.qrc
        ${omodsim_app_test_sources}
        ${HEADERS}
        ${UI_FILES}
    )
    target_include_directories(omodsim_tests_appproject PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/controls
        ${CMAKE_CURRENT_SOURCE_DIR}/styles
        ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
        ${CMAKE_CURRENT_SOURCE_DIR}/jsobjects
        ${CMAKE_CURRENT_SOURCE_DIR}/modbusmessages
    )
    target_precompile_headers(omodsim_tests_appproject PRIVATE pch.h)
    target_link_libraries(omodsim_tests_appproject PRIVATE
        Qt::Test
        Qt::Widgets
        Qt::Network
        Qt::Xml
        Qt::PrintSupport
        Qt::SerialBus
        Qt::SerialPort
        Qt::Qml
        Qt::Help
        Qt::Svg
    )
    if(Qt6_FOUND)
        target_link_libraries(omodsim_tests_appproject PRIVATE Qt::Core5Compat)
    endif()
    target_compile_definitions(omodsim_tests_appproject PRIVATE
        APP_NAME="Open ModSim Test"
        APP_PRODUCT_NAME="Open ModSim"
        APP_DESCRIPTION="Open ModSim tests"
        APP_VERSION_MAJOR="2"
        APP_VERSION_MINOR="0"
        APP_VERSION_PATCH="0"
        APP_VERSION="2.0.0-test"
        BUILD_YEAR="2026"
    )
    if(MSVC)
        target_compile_options(omodsim_tests_appproject PRIVATE /utf-8)
    endif()
    set_target_properties(omodsim_tests_appproject PROPERTIES FOLDER "Tests")
    add_dependencies(omodsim_tests omodsim_tests_appproject)
    add_test(NAME omodsim_tests_appproject COMMAND omodsim_tests_appproject)
endfunction()
