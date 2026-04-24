cmake_minimum_required(VERSION 3.16)

project(omodsim
        VERSION 2.0.0
        DESCRIPTION "An Open Source Modbus Slave (Server) Utility"
        LANGUAGES CXX)

include(GNUInstallDirs)

option(USE_QT5 "Force Qt5 usage" OFF)
option(USE_QT6 "Force Qt6 usage" OFF)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(PRODUCT_NAME "Open ModSim")
set(PROJECT_BETARELEASE "" CACHE STRING "Beta release suffix, e.g. beta1")

# Split version to major, minor, patch
string(REPLACE "." ";" VERSION_LIST ${PROJECT_VERSION})
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)
list(GET VERSION_LIST 2 VERSION_PATCH)

# Define current year
string(TIMESTAMP CURRENT_YEAR "%Y")

# Generate license.txt
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/res/license.txt.in
    ${CMAKE_CURRENT_SOURCE_DIR}/res/license.txt
    @ONLY
)

# Generate README.md
configure_file(
    ${CMAKE_SOURCE_DIR}/../README.md.in
    ${CMAKE_SOURCE_DIR}/../README.md
    @ONLY
)

if(USE_QT5)
    find_package(Qt5 COMPONENTS Core Gui Widgets Network Xml PrintSupport SerialBus SerialPort Qml Help LinguistTools REQUIRED)
elseif(USE_QT6)
    find_package(Qt6 COMPONENTS Core Gui Widgets Network Xml PrintSupport SerialBus SerialPort Core5Compat Qml Help LinguistTools REQUIRED)
else()
    # fallback: Qt6 preferred
    find_package(Qt6 COMPONENTS Core Gui Widgets Network Xml PrintSupport SerialBus SerialPort Core5Compat Qml Help LinguistTools QUIET)
    if (NOT Qt6_FOUND)
        find_package(Qt5 COMPONENTS Core Gui Widgets Network Xml PrintSupport SerialBus SerialPort Qml Help LinguistTools REQUIRED)
    endif()
endif()

if (NOT Qt6_FOUND AND NOT Qt5_FOUND)
    message(FATAL_ERROR "One or more Qt development packages not found. Please install Qt5 or Qt6 development packages.")
endif()

if(Qt6_FOUND)
    get_target_property(QT_BINARY_DIR Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BINARY_DIR "${QT_BINARY_DIR}" DIRECTORY)
    get_filename_component(QT_INSTALL_LIBEXECS "${QT6_INSTALL_LIBEXECS}" ABSOLUTE BASE_DIR "/usr")
    get_filename_component(QT_LIBEXEC_DIR "${QT_BINARY_DIR}/../libexec" REALPATH)
else()
    get_target_property(QT_BINARY_DIR Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BINARY_DIR "${QT_BINARY_DIR}" DIRECTORY)
    set(QT_LIBEXEC_DIR "${QT_BINARY_DIR}")
endif()

if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BINARY_DIR}" "${QT_LIBEXEC_DIR}" "${QT_INSTALL_LIBEXECS}")
    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "windeployqt found at: ${WINDEPLOYQT_EXECUTABLE}")
    else()
        message(WARNING "windeployqt not found - Windows install will not deploy Qt runtime")
    endif()
endif()

if(APPLE)
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${QT_BINARY_DIR}" "${QT_LIBEXEC_DIR}" "${QT_INSTALL_LIBEXECS}")
    if(MACDEPLOYQT_EXECUTABLE)
        message(STATUS "macdeployqt found at: ${MACDEPLOYQT_EXECUTABLE}")
    else()
        message(WARNING "macdeployqt not found - macOS install will not deploy Qt runtime")
    endif()
endif()

# Define target
add_executable(${PROJECT_NAME})

if(DEFINED ENV{GITHUB_REF})
    string(REGEX MATCH "refs/heads/(.*)" MATCH_RESULT $ENV{GITHUB_REF})
    if(MATCH_RESULT AND CMAKE_MATCH_1)
        set(GIT_BRANCH ${CMAKE_MATCH_1})
    endif()
else()
    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/../.git
            OUTPUT_VARIABLE GIT_BRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    else()
        message(STATUS "Git not found - branch detection is not possible")
    endif()
endif()

if(DEFINED GIT_BRANCH AND "${GIT_BRANCH}" MATCHES "^(dev|release/.*)$")
    set(PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}-dev)
elseif(PROJECT_BETARELEASE)
    set(PROJECT_VERSION ${PROJECT_VERSION}-${PROJECT_BETARELEASE})
endif()

# Define compile definitions
add_compile_definitions(APP_NAME="${PRODUCT_NAME} ${VERSION_MAJOR}")
add_compile_definitions(APP_PRODUCT_NAME="${PRODUCT_NAME}")
add_compile_definitions(APP_DESCRIPTION="${PROJECT_DESCRIPTION}")
add_compile_definitions(APP_VERSION_MAJOR="${VERSION_MAJOR}")
add_compile_definitions(APP_VERSION_MINOR="${VERSION_MINOR}")
add_compile_definitions(APP_VERSION_PATCH="${VERSION_PATCH}")
add_compile_definitions(APP_VERSION="${PROJECT_VERSION}")
add_compile_definitions(BUILD_YEAR="${CURRENT_YEAR}")

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/controls
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/jsobjects
    ${CMAKE_CURRENT_SOURCE_DIR}/modbusmessages
)

# Source files
set(SOURCES
    ansimenu.cpp
    applogger.cpp
    apppreferences.cpp
    appproject.cpp
    cmdlineparser.cpp
    controls/aboutdatawidget.cpp
    controls/addressbasecombobox.cpp
    controls/addressspacecombobox.cpp
    controls/bitpatterncontrol.cpp
    controls/funccodefiltercombobox.cpp
    controls/booleancombobox.cpp
    controls/bytelisttextedit.cpp
    controls/checkablegroupbox.cpp
    controls/clickablelabel.cpp
    controls/applogoutput.cpp
    controls/consoleoutput.cpp
    controls/outputpanel.cpp
    controls/customframe.cpp
    controls/findreplacebar.cpp
    controls/customlineedit.cpp
    controls/flowcontroltypecombobox.cpp
    controls/helpwidget.cpp
    controls/jscodeeditor.cpp
    controls/jscriptcontrol.cpp
    controls/mainstatusbar.cpp
    controls/mdiarea.cpp
    controls/mdiareaex.cpp
    controls/mditabbar.cpp
    controls/modbuslogwidget.cpp
    controls/projecttreewidget.cpp
    controls/modbusmessagewidget.cpp
    controls/numericcombobox.cpp
    controls/numericlineedit.cpp
    controls/outputdatawidget.cpp
    controls/outputtrafficwidget.cpp
    controls/paritytypecombobox.cpp
    controls/pointtypecombobox.cpp
    controls/runmodecombobox.cpp
    controls/searchlineedit.cpp
    controls/simulationmodecombobox.cpp
    controls/statisticwidget.cpp
    controls/transpscrollarea.cpp
    datadelegate.cpp
    datasimulator.cpp
    dialogs/dialogautosimulation.cpp
    dialogs/dialogcoilsimulation.cpp
    dialogs/dialogabout.cpp
    dialogs/dialogwelcome.cpp
    dialogs/dialogforcemultiplecoils.cpp
    dialogs/dialogforcemultipleregisters.cpp
    dialogs/dialogmsgparser.cpp
    dialogs/dialogprintsettings.cpp
    dialogs/dialogpreferences.cpp
    dialogs/dialogselectserviceport.cpp
    dialogs/dialogmodbusdefinitions.cpp
    dialogs/dialogsetuppresetdata.cpp
    dialogs/dialogsetupserialport.cpp
    dialogs/dialogwindowsmanager.cpp
    dialogs/dialogwritestatusregister.cpp
    dialogs/dialogwriteregister.cpp
    htmldelegate.cpp
    jscompleter.cpp
    jsobjects/console.cpp
    jsobjects/script.cpp
    jsobjects/server.cpp
    jsobjects/storage.cpp
    formdataview.cpp
    formtrafficview.cpp
    formscriptview.cpp
    formdatamapview.cpp
    datamapdatamodel.cpp
    jshighlighter.cpp
    main.cpp
    mainwindow.cpp
    menuconnect.cpp
    modbusdataunitmap.cpp
    modbuserrorsimulations.cpp
    modbusmessages/modbusmessage.cpp
    modbusrtuserialserver.cpp
    modbusserver.cpp
    modbustcpserver.cpp
    modbusmultiserver.cpp
    qadjustedsizedialog.cpp
    qfixedsizedialog.cpp
    qhexvalidator.cpp
    qint64validator.cpp
    qdoublevalidatorex.cpp
    qintvalidatorex.cpp
    quintvalidator.cpp
    recentfileactionlist.cpp
    updatechecker.cpp
)

# Header files
set(HEADERS
    ansimenu.h
    ansiutils.h
    applogger.h
    apppreferences.h
    appproject.h
    apptrace.h
    bufferinglistmodel.h
    byteorderutils.h
    cmdlineparser.h
    connectiondetails.h
    controls/aboutdatawidget.h
    controls/addressbasecombobox.h
    controls/addressspacecombobox.h
    controls/bitpatterncontrol.h
    controls/funccodefiltercombobox.h
    controls/booleancombobox.h
    controls/bytelisttextedit.h
    controls/checkablegroupbox.h
    controls/clickablelabel.h
    controls/applogoutput.h
    controls/consoleoutput.h
    controls/outputpanel.h
    controls/customframe.h
    controls/findreplacebar.h
    controls/customlineedit.h
    controls/flowcontroltypecombobox.h
    controls/helpbrowser.h
    controls/helpwidget.h
    controls/jscodeeditor.h
    controls/jscriptcontrol.h
    controls/mainstatusbar.h
    controls/mdiarea.h
    controls/mdiareaex.h
    controls/mditabbar.h
    controls/modbuslogwidget.h
    controls/projecttreewidget.h
    controls/modbusmessagewidget.h
    controls/numericcombobox.h
    controls/numericlineedit.h
    controls/outputdatawidget.h
    controls/outputtrafficwidget.h
    controls/outputtypes.h
    controls/paritytypecombobox.h
    controls/pointtypecombobox.h
    controls/runmodecombobox.h
    controls/searchlineedit.h
    controls/simulationmodecombobox.h
    controls/statisticwidget.h
    controls/transpscrollarea.h
    datadelegate.h
    datasimulator.h
    dialogs/dialogautosimulation.h
    dialogs/dialogcoilsimulation.h
    dialogs/dialogabout.h
    dialogs/dialogwelcome.h
    dialogs/dialogforcemultiplecoils.h
    dialogs/dialogforcemultipleregisters.h
    dialogs/dialogmsgparser.h
    dialogs/dialogprintsettings.h
    dialogs/colorswatch.h
    dialogs/dialogpreferences.h
    dialogs/dialogmodbusdefinitions.h
    dialogs/dialogselectserviceport.h
    dialogs/dialogsetuppresetdata.h
    dialogs/dialogsetupserialport.h
    dialogs/dialogwindowsmanager.h
    dialogs/dialogwritestatusregister.h
    dialogs/dialogwriteregister.h
    fontutils.h
    formatutils.h
    htmldelegate.h
    jscompleter.h
    jsobjects/console.h
    jsobjects/script.h
    jsobjects/server.h
    jsobjects/storage.h
    displaydefinition.h
    enums.h
    modbusserver.h
    formdataview.h
    formtrafficview.h
    formscriptview.h
    formdatamapview.h
    datamapdatamodel.h
    jshighlighter.h
    mainwindow.h
    menuconnect.h
    modbusdataunitmap.h
    modbusdefinitions.h
    modbuserrorsimulations.h
    modbuslimits.h
    modbusmessages/diagnostics.h
    modbusmessages/getcommeventcounter.h
    modbusmessages/getcommeventlog.h
    modbusmessages/maskwriteregister.h
    modbusmessages/modbusmessage.h
    modbusmessages/modbusmessages.h
    modbusmessages/readcoils.h
    modbusmessages/readdiscreteinputs.h
    modbusmessages/readexceptionstatus.h
    modbusmessages/readfifoqueue.h
    modbusmessages/readfilerecord.h
    modbusmessages/readholdingregisters.h
    modbusmessages/readinputregisters.h
    modbusmessages/readwritemultipleregisters.h
    modbusmessages/reportserverid.h
    modbusmessages/writefilerecord.h
    modbusmessages/writemultiplecoils.h
    modbusmessages/writemultipleregisters.h
    modbusmessages/writesinglecoil.h
    modbusmessages/writesingleregister.h
    modbusmultiserver.h
    modbusrtuserialserver.h
    modbustcpserver.h
    modbussimulationparams.h
    modbuswriteparams.h
    numericutils.h
    pch.h
    qadjustedsizedialog.h
    qfixedsizedialog.h
    qhexvalidator.h
    qint64validator.h
    qmodbusadu.h
    qmodbusadurtu.h
    qcountedset.h
    qmodbuscommevent.h
    qmodbusadutcp.h
    qrange.h
    qdoublevalidatorex.h
    qintvalidatorex.h
    quintvalidator.h
    recentfileactionlist.h
    updatechecker.h
    scriptsettings.h
    serialportutils.h
    uiutils.h
)

# UI Forms
set(UI_FILES
    controls/bitpatterncontrol.ui
    controls/applogoutput.ui
    controls/consoleoutput.ui
    controls/mdiareaex.ui
    controls/outputpanel.ui
    controls/findreplacebar.ui
    controls/jscriptcontrol.ui
    controls/outputdatawidget.ui
    controls/outputtrafficwidget.ui
    controls/statisticwidget.ui
    formdataview.ui
    formtrafficview.ui
    formscriptview.ui
    formdatamapview.ui
    dialogs/dialogautosimulation.ui
    dialogs/dialogcoilsimulation.ui
    dialogs/dialogabout.ui
    dialogs/dialogwelcome.ui
    dialogs/dialogforcemultiplecoils.ui
    dialogs/dialogforcemultipleregisters.ui
    dialogs/dialogmsgparser.ui
    dialogs/dialogprintsettings.ui
    dialogs/dialogpreferences.ui
    dialogs/dialogselectserviceport.ui
    dialogs/dialogmodbusdefinitions.ui
    dialogs/dialogsetuppresetdata.ui
    dialogs/dialogsetupserialport.ui
    dialogs/dialogwindowsmanager.ui
    dialogs/dialogwritestatusregister.ui
    dialogs/dialogwriteregister.ui
    mainwindow.ui
)

set(TS_FILES
    translations/omodsim_ru.ts
    translations/omodsim_zh_CN.ts
    translations/omodsim_zh_TW.ts
)

set(JSHELP_INPUT "${CMAKE_SOURCE_DIR}/docs/jshelp.qhcp")
set(JSHELP_QHCP "${CMAKE_BINARY_DIR}/docs/jshelp.qhcp")
set(JSHELP_QHP "${CMAKE_SOURCE_DIR}/docs/jshelp.qhp")
set(JSHELP_QCH "${CMAKE_BINARY_DIR}/docs/jshelp.qch")
set(JSHELP_QHC "${CMAKE_BINARY_DIR}/docs/jshelp.qhc")

file(GLOB JSHELP_CONTENT
    "${CMAKE_SOURCE_DIR}/docs/*.html"
    "${CMAKE_SOURCE_DIR}/docs/assets/css/*.css"
)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/docs)

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

target_precompile_headers(${PROJECT_NAME} PRIVATE pch.h)
target_sources(${PROJECT_NAME} PRIVATE resources.qrc ${HEADERS} ${SOURCES} ${UI_FILES} ${TS_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE Qt::Widgets Qt::Network Qt::Xml Qt::PrintSupport Qt::SerialBus Qt::SerialPort Qt::Qml Qt::Help)

if(Qt6_FOUND)
    qt_add_lupdate(${PROJECT_NAME}
        TS_FILES ${TS_FILES}
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt::Core5Compat)
else()
    add_custom_command(
        OUTPUT ${TS_FILES}
        COMMAND ${Qt5_LUPDATE_EXECUTABLE}
                ${SOURCES} ${HEADERS} ${UI_FILES}
                -ts ${TS_FILES}
        DEPENDS ${SOURCES} ${HEADERS} ${UI_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Updating translation files..."
    )
    add_custom_target(update_translations ALL DEPENDS ${TS_FILES})
endif()

if(TARGET update_translations)
    add_dependencies(${PROJECT_NAME} update_translations)
endif()

if(WIN32)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
    endif()

    set(FILE_VERSION "${PROJECT_VERSION}")
    set(PRODUCT_VERSION "${PROJECT_VERSION}")
    set(ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/res/omodsim.ico")
    set(OMP_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/res/omp.ico")

    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/omodsim.rc.in"
            "${CMAKE_BINARY_DIR}/omodsim.rc"
            @ONLY
        )

    target_sources(${PROJECT_NAME} PRIVATE ${ICON_PATH} "${CMAKE_BINARY_DIR}/omodsim.rc")
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)
elseif(APPLE)
    set(MACOSX_ICON "${CMAKE_CURRENT_SOURCE_DIR}/res/omodsim.icns")
    set_source_files_properties(${MACOSX_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(${PROJECT_NAME} PRIVATE ${MACOSX_ICON})

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/omodsim.plist.in"
        "${CMAKE_BINARY_DIR}/Info.plist"
        @ONLY
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_BINARY_DIR}/Info.plist"
        MACOSX_BUNDLE_ICON_FILE omodsim.icns
        MACOSX_BUNDLE_BUNDLE_NAME "${PRODUCT_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
elseif(LINUX)
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
endif()

add_custom_command(
    OUTPUT ${JSHELP_QCH} ${JSHELP_QHC}
    COMMAND ${CMAKE_COMMAND} -E copy ${JSHELP_INPUT} ${JSHELP_QHCP}
    COMMAND ${QHELP_GENERATOR_EXECUTABLE} ${JSHELP_QHP} -o ${JSHELP_QCH}
    COMMAND ${QHELP_GENERATOR_EXECUTABLE} ${JSHELP_QHCP} -o ${JSHELP_QHC}
    COMMAND ${CMAKE_COMMAND} -E remove ${JSHELP_QHCP}
    DEPENDS ${JSHELP_QHP} ${JSHELP_INPUT} ${JSHELP_CONTENT}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/docs
    COMMENT "Generating and moving help files..."
    VERBATIM
)

add_custom_target(helpgenerator ALL DEPENDS ${JSHELP_QCH} ${JSHELP_QHC})
add_dependencies(${PROJECT_NAME} helpgenerator)


configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    COMMENT "Uninstalling ${PROJECT_NAME}"
)

# Installation rules
if(WIN32)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "C:/Program Files/Open ModSim ${VERSION_MAJOR}" CACHE PATH "Install path prefix" FORCE)
    endif()

    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../demos" DESTINATION .)
    install(DIRECTORY "${CMAKE_BINARY_DIR}/docs" DESTINATION .)

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
elseif(APPLE)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/Applications" CACHE PATH "Install path prefix" FORCE)
    endif()
    
    install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION .)

    install(CODE "
        set(_installed_app \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app\")
        set(_resources_dir \"\${_installed_app}/Contents/Resources\")
        set(_docs_source \"${CMAKE_BINARY_DIR}/docs\")
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

        if(EXISTS \"${MACDEPLOYQT_EXECUTABLE}\")
            execute_process(
                COMMAND \"${MACDEPLOYQT_EXECUTABLE}\"
                        \"\${_installed_app}\"
                        -always-overwrite
                RESULT_VARIABLE _macdeployqt_result
            )
            if(NOT _macdeployqt_result EQUAL 0)
                message(FATAL_ERROR \"macdeployqt failed with exit code \${_macdeployqt_result}\")
            endif()
        endif()
    ")
elseif(LINUX)
    set(APP_NAME "${PRODUCT_NAME} ${VERSION_MAJOR}")
    set(EXECUTABLE_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME}${VERSION_MAJOR}")
    set(APP_EXECUTABLE_NAME "${PROJECT_NAME}${PROJECT_VERSION_MAJOR}")
    set(APP_ICON "${APP_EXECUTABLE_NAME}")

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/omodsim.desktop.in"
        "${CMAKE_CURRENT_BINARY_DIR}/omodsim.desktop"
        @ONLY
    )

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../demos/" DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/demos")
    install(DIRECTORY "${CMAKE_BINARY_DIR}/docs/" DESTINATION "${CMAKE_INSTALL_DOCDIR}")

    install(PROGRAMS "$<TARGET_FILE:${PROJECT_NAME}>" DESTINATION bin RENAME "${APP_EXECUTABLE_NAME}")
    install(FILES res/omodsim16.png DESTINATION share/icons/hicolor/16x16/apps RENAME "${APP_ICON}.png")
    install(FILES res/omodsim32.png DESTINATION share/icons/hicolor/32x32/apps RENAME "${APP_ICON}.png")
    install(FILES res/omodsim64.png DESTINATION share/icons/hicolor/64x64/apps RENAME "${APP_ICON}.png")
    install(FILES res/omodsim128.png DESTINATION share/icons/hicolor/128x128/apps RENAME "${APP_ICON}.png")
    install(FILES res/omp16.png DESTINATION share/icons/hicolor/16x16/mimetypes RENAME omodsim-project.png)
    install(FILES res/omp32.png DESTINATION share/icons/hicolor/32x32/mimetypes RENAME omodsim-project.png)
    install(FILES res/omp64.png DESTINATION share/icons/hicolor/64x64/mimetypes RENAME omodsim-project.png)
    install(FILES res/omp128.png DESTINATION share/icons/hicolor/128x128/mimetypes RENAME omodsim-project.png)
    install(FILES res/omp256.png DESTINATION share/icons/hicolor/256x256/mimetypes RENAME omodsim-project.png)
    install(FILES res/omp.svg DESTINATION share/icons/hicolor/scalable/mimetypes RENAME omodsim-project.svg)
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
endif()



