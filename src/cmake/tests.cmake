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
        datasimulator.cpp
        qhexvalidator.cpp
        qintvalidatorex.cpp
        qint64validator.cpp
        quintvalidator.cpp
        qdoublevalidatorex.cpp
        modbusmessages/modbusmessage.cpp
    )

    target_include_directories(omodsim_testable PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/controls
        ${CMAKE_CURRENT_SOURCE_DIR}/modbusmessages
    )

    target_link_libraries(omodsim_testable PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::SerialBus
    )

    if(Qt6_FOUND)
        target_link_libraries(omodsim_testable PUBLIC Qt::Core5Compat)
    endif()

    if(MSVC)
        target_compile_options(omodsim_testable PUBLIC /utf-8)
    endif()

    omodsim_apply_coverage(omodsim_testable)
    set_target_properties(omodsim_testable PROPERTIES FOLDER "Tests")

    function(omodsim_add_test test_name source_file)
        add_executable(${test_name} tests/${source_file})
        target_link_libraries(${test_name} PRIVATE
            omodsim_testable
            Qt::Test
        )
        omodsim_apply_coverage(${test_name})
        set_target_properties(${test_name} PROPERTIES FOLDER "Tests")
        add_test(NAME ${test_name} COMMAND ${test_name})
    endfunction()

    omodsim_add_test(omodsim_tests_numericutils         test_numericutils.cpp)
    omodsim_add_test(omodsim_tests_byteorderutils       test_byteorderutils.cpp)
    omodsim_add_test(omodsim_tests_enums                test_enums.cpp)
    omodsim_add_test(omodsim_tests_modbusfunction       test_modbusfunction.cpp)
    omodsim_add_test(omodsim_tests_formatutils          test_formatutils.cpp)
    omodsim_add_test(omodsim_tests_errorsimulations     test_modbuserrorsimulations.cpp)
    omodsim_add_test(omodsim_tests_validators           test_validators.cpp)
    omodsim_add_test(omodsim_tests_dataunitmap          test_modbusdataunitmap.cpp)
    omodsim_add_test(omodsim_tests_datasimulator        test_datasimulator.cpp)
    omodsim_add_test(omodsim_tests_modbusmessage        test_modbusmessage.cpp)
endfunction()
