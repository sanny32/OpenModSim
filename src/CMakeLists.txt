cmake_minimum_required(VERSION 3.21)

project(omodsim
        VERSION 2.0.0
        DESCRIPTION "An Open Source Modbus Slave (Server) Utility"
        LANGUAGES CXX)

include(GNUInstallDirs)

option(USE_QT5 "Force Qt5 usage" OFF)
option(USE_QT6 "Force Qt6 usage" OFF)

if(APPLE)
    option(USE_QLEMENTINE_APP_STYLE "Use Qlementine application style when available (requires Qt6 >= 6.8)" ON)
else()
    option(USE_QLEMENTINE_APP_STYLE "Use Qlementine application style when available (requires Qt6 >= 6.8)" OFF)
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(PRODUCT_NAME "Open ModSim")
set(PRERELEASE_SUFFIX "" CACHE STRING "Pre-release suffix, e.g. beta1")

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
    ${PROJECT_SOURCE_DIR}/../README.md.in
    ${PROJECT_SOURCE_DIR}/../README.md
    @ONLY
)

if(USE_QT5)
    find_package(Qt5 COMPONENTS Core Gui Widgets Network Xml PrintSupport SerialBus SerialPort Qml Help Svg LinguistTools REQUIRED)
elseif(USE_QT6)
    find_package(Qt6 COMPONENTS Core Gui Widgets Network Xml PrintSupport SerialBus SerialPort Core5Compat Qml Help Svg LinguistTools REQUIRED)
else()
    # fallback: Qt6 preferred
    find_package(Qt6 COMPONENTS Core Gui Widgets Network Xml PrintSupport SerialBus SerialPort Core5Compat Qml Help Svg LinguistTools QUIET)
    if (NOT Qt6_FOUND)
        find_package(Qt5 COMPONENTS Core Gui Widgets Network Xml PrintSupport SerialBus SerialPort Qml Help Svg LinguistTools REQUIRED)
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

if(USE_QLEMENTINE_APP_STYLE AND Qt6_FOUND AND Qt6_VERSION VERSION_GREATER_EQUAL "6.8")
    include(FetchContent)
    set(QLEMENTINE_VERSION "1.4.2")
    FetchContent_Declare(qlementine
        GIT_REPOSITORY "https://github.com/oclero/qlementine.git"
        GIT_TAG        "v${QLEMENTINE_VERSION}"
    )
    FetchContent_MakeAvailable(qlementine)

    set(QLEMENTINE_ICONS_VERSION "1.15.0")
    FetchContent_Declare(qlementine-icons
        GIT_REPOSITORY "https://github.com/oclero/qlementine-icons.git"
        GIT_TAG        "v${QLEMENTINE_ICONS_VERSION}"
    )
    FetchContent_MakeAvailable(qlementine-icons)
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
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/..
            OUTPUT_VARIABLE GIT_BRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    else()
        message(STATUS "Git not found - branch detection is not possible")
    endif()
endif()

if(DEFINED GIT_BRANCH AND "${GIT_BRANCH}" MATCHES "^(dev|release/.*)$")
    set(PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}-dev)
elseif(PRERELEASE_SUFFIX)
    set(PROJECT_VERSION ${PROJECT_VERSION}-${PRERELEASE_SUFFIX})
endif()

if(USE_QLEMENTINE_APP_STYLE AND
   NOT (Qt6_FOUND AND Qt6_VERSION VERSION_GREATER_EQUAL "6.8" AND TARGET qlementine AND TARGET qlementine-icons))
    set(USE_QLEMENTINE_APP_STYLE OFF CACHE BOOL
        "Use Qlementine application style when available (requires Qt6 >= 6.8)" FORCE)
endif()

# Target compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    APP_NAME="${PRODUCT_NAME} ${VERSION_MAJOR}"
    APP_PRODUCT_NAME="${PRODUCT_NAME}"
    APP_DESCRIPTION="${PROJECT_DESCRIPTION}"
    APP_VERSION_MAJOR="${VERSION_MAJOR}"
    APP_VERSION_MINOR="${VERSION_MINOR}"
    APP_VERSION_PATCH="${VERSION_PATCH}"
    APP_VERSION="${PROJECT_VERSION}"
    BUILD_YEAR="${CURRENT_YEAR}"
)

if(USE_QLEMENTINE_APP_STYLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        HAVE_QLEMENTINE_APP_STYLE
        QLEMENTINE_VERSION="${QLEMENTINE_VERSION}"
        QLEMENTINE_ICONS_VERSION="${QLEMENTINE_ICONS_VERSION}"
    )
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/controls
    ${CMAKE_CURRENT_SOURCE_DIR}/styles
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/jsobjects
    ${CMAKE_CURRENT_SOURCE_DIR}/modbusmessages
)

# Source files
set(SOURCES
    ansimenu.cpp
    application.cpp
    applogger.cpp
    apppreferences.cpp
    appproject.cpp
    cmdlineparser.cpp

    controls/aboutdatawidget.cpp
    controls/addressbasecombobox.cpp
    controls/addressspacecombobox.cpp
    controls/applogoutput.cpp
    controls/bitnumberlabel.cpp
    controls/bitpatterncontrol.cpp
    controls/booleancombobox.cpp
    controls/bytelisttextedit.cpp
    controls/checkablegroupbox.cpp
    controls/clickablelabel.cpp
    controls/coloredpushbutton.cpp
    controls/coloredtoolbutton.cpp
    controls/consoleoutput.cpp
    controls/customframe.cpp
    controls/customlineedit.cpp
    controls/findreplacebar.cpp
    controls/flowcontroltypecombobox.cpp
    controls/funccodefiltercombobox.cpp
    controls/helpbrowser.cpp
    controls/helpwidget.cpp
    controls/jscodeeditor.cpp
    controls/jscriptcontrol.cpp
    controls/macsplittoolbutton.cpp
    controls/mainstatusbar.cpp
    controls/mdiarea.cpp
    controls/mdiareaex.cpp
    controls/mditabbar.cpp
    controls/modbuslogwidget.cpp
    controls/modbusmessagewidget.cpp
    controls/numericcombobox.cpp
    controls/numericlineedit.cpp
    controls/outputdatawidget.cpp
    controls/outputpanel.cpp
    controls/outputtrafficwidget.cpp
    controls/paritytypecombobox.cpp
    controls/pointtypecombobox.cpp
    controls/projecttreewidget.cpp
    controls/runmodecombobox.cpp
    controls/scripteditorwindow.cpp
    controls/searchlineedit.cpp
    controls/simulationmodecombobox.cpp
    controls/statisticwidget.cpp
    controls/toolbar.cpp
    controls/trafficlogwindow.cpp
    controls/transpscrollarea.cpp

    datadelegate.cpp
    datamapdatamodel.cpp
    datasimulator.cpp

    dialogs/dialogabout.cpp
    dialogs/dialogautosimulation.cpp
    dialogs/dialogcoilsimulation.cpp
    dialogs/dialogforcemultipleregisters.cpp
    dialogs/dialogforcestatusregisters.cpp
    dialogs/dialogmodbusdefinitions.cpp
    dialogs/dialogmsgparser.cpp
    dialogs/dialogpreferences.cpp
    dialogs/dialogprintsettings.cpp
    dialogs/dialogselectserviceport.cpp
    dialogs/dialogsetupserialport.cpp
    dialogs/dialogwelcome.cpp
    dialogs/dialogwindowsmanager.cpp
    dialogs/dialogwriteregister.cpp
    dialogs/dialogwritestatusregister.cpp

    formdatamapview.cpp
    formdataview.cpp
    formscriptview.cpp
    formtrafficview.cpp

    htmldelegate.cpp
    jscompleter.cpp
    jshighlighter.cpp
    main.cpp
    mainwindow.cpp
    menuconnect.cpp

    jsobjects/console.cpp
    jsobjects/script.cpp
    jsobjects/server.cpp
    jsobjects/storage.cpp

    modbusdataunitmap.cpp
    modbuserrorsimulations.cpp
    modbusmessages/modbusmessage.cpp
    modbusmultiserver.cpp
    modbusrtuserialserver.cpp
    modbusserver.cpp
    modbustcpserver.cpp

    qadjustedsizedialog.cpp
    qdoublevalidatorex.cpp
    qhexvalidator.cpp
    qint64validator.cpp
    qintvalidatorex.cpp
    qfixedsizedialog.cpp
    quintvalidator.cpp
    recentfileactionlist.cpp
    scriptdocument.cpp

    styles/appstyle.cpp
    styles/apptheme.cpp
    styles/macappstyle.cpp
    styles/qlementineappstyle.cpp
    styles/themedicons.cpp

    updatechecker.cpp
)

# Header files
set(HEADERS
    ansimenu.h
    ansiutils.h
    application.h
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
    controls/applogoutput.h
    controls/bitnumberlabel.h
    controls/bitpatterncontrol.h
    controls/booleancombobox.h
    controls/bytelisttextedit.h
    controls/checkablegroupbox.h
    controls/clickablelabel.h
    controls/coloredpushbutton.h
    controls/coloredtoolbutton.h
    controls/consoleoutput.h
    controls/customframe.h
    controls/customlineedit.h
    controls/findreplacebar.h
    controls/flowcontroltypecombobox.h
    controls/funccodefiltercombobox.h
    controls/helpbrowser.h
    controls/helpwidget.h
    controls/jscodeeditor.h
    controls/jscriptcontrol.h
    controls/macsplittoolbutton.h
    controls/mainstatusbar.h
    controls/mdiarea.h
    controls/mdiareaex.h
    controls/mditabbar.h
    controls/modbuslogwidget.h
    controls/modbusmessagewidget.h
    controls/numericcombobox.h
    controls/numericlineedit.h
    controls/outputdatawidget.h
    controls/outputpanel.h
    controls/outputtrafficwidget.h
    controls/outputtypes.h
    controls/paritytypecombobox.h
    controls/pointtypecombobox.h
    controls/projecttreewidget.h
    controls/runmodecombobox.h
    controls/scripteditorwindow.h
    controls/searchlineedit.h
    controls/simulationmodecombobox.h
    controls/statisticwidget.h
    controls/toolbar.h
    controls/trafficlogwindow.h
    controls/transpscrollarea.h

    datadelegate.h
    datamapdatamodel.h
    datasimulator.h

    dialogs/dialogabout.h
    dialogs/dialogautosimulation.h
    dialogs/dialogcoilsimulation.h
    dialogs/colorswatch.h
    dialogs/dialogforcemultipleregisters.h
    dialogs/dialogforcestatusregisters.h
    dialogs/dialogmodbusdefinitions.h
    dialogs/dialogmsgparser.h
    dialogs/dialogpreferences.h
    dialogs/dialogprintsettings.h
    dialogs/dialogselectserviceport.h
    dialogs/dialogsetupserialport.h
    dialogs/dialogwelcome.h
    dialogs/dialogwindowsmanager.h
    dialogs/dialogwriteregister.h
    dialogs/dialogwritestatusregister.h

    displaydefinition.h
    enums.h
    fontutils.h
    formatutils.h

    formdatamapview.h
    formdataview.h
    formscriptview.h
    formtrafficview.h

    htmldelegate.h
    jscompleter.h
    jshighlighter.h

    jsobjects/console.h
    jsobjects/script.h
    jsobjects/server.h
    jsobjects/storage.h

    mainwindow.h
    menuconnect.h

    modbusdataunitmap.h
    modbusdefinitions.h
    modbusexception.h
    modbuserrorsimulations.h
    modbusfunction.h
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
    modbusserver.h
    modbustcpserver.h
    modbussimulationparams.h
    modbuswriteparams.h

    numericutils.h
    pch.h
    qadjustedsizedialog.h
    qcountedset.h
    qdoublevalidatorex.h
    qfixedsizedialog.h
    qhexvalidator.h
    qint64validator.h
    qintvalidatorex.h
    qmodbusadu.h
    qmodbusadurtu.h
    qmodbusadutcp.h
    qmodbuscommevent.h
    qrange.h
    quintvalidator.h
    recentfileactionlist.h
    scriptdocument.h
    scriptsettings.h
    serialportutils.h
    translationutils.h
    uiutils.h

    styles/appcolors.h
    styles/appstyle.h
    styles/apptheme.h
    styles/macappstyle.h
    styles/qlementineappstyle.h
    styles/themedicons.h

    updatechecker.h
)

# UI Forms
set(UI_FILES
    controls/applogoutput.ui
    controls/bitpatterncontrol.ui
    controls/consoleoutput.ui
    controls/findreplacebar.ui
    controls/jscriptcontrol.ui
    controls/mdiareaex.ui
    controls/outputdatawidget.ui
    controls/outputpanel.ui
    controls/outputtrafficwidget.ui
    controls/statisticwidget.ui

    dialogs/dialogabout.ui
    dialogs/dialogautosimulation.ui
    dialogs/dialogcoilsimulation.ui
    dialogs/dialogforcemultipleregisters.ui
    dialogs/dialogforcestatusregisters.ui
    dialogs/dialogmodbusdefinitions.ui
    dialogs/dialogmsgparser.ui
    dialogs/dialogpreferences.ui
    dialogs/dialogprintsettings.ui
    dialogs/dialogselectserviceport.ui
    dialogs/dialogsetupserialport.ui
    dialogs/dialogwelcome.ui
    dialogs/dialogwindowsmanager.ui
    dialogs/dialogwriteregister.ui
    dialogs/dialogwritestatusregister.ui

    formdatamapview.ui
    formdataview.ui
    formscriptview.ui
    formtrafficview.ui

    mainwindow.ui
)

set(UI_FILE_LIST "${CMAKE_CURRENT_BINARY_DIR}/ui_enum_files.txt")
set(UI_FILES_ABSOLUTE)
file(WRITE "${UI_FILE_LIST}" "")
foreach(UI_FILE IN LISTS UI_FILES)
    set(UI_FILE_ABSOLUTE "${CMAKE_CURRENT_SOURCE_DIR}/${UI_FILE}")
    list(APPEND UI_FILES_ABSOLUTE "${UI_FILE_ABSOLUTE}")
    file(APPEND "${UI_FILE_LIST}" "${UI_FILE_ABSOLUTE}\n")
endforeach()

add_custom_target(normalize_ui_enums ALL
    COMMAND ${CMAKE_COMMAND}
            "-DUI_FILE_LIST=${UI_FILE_LIST}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NormalizeUiEnums.cmake"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NormalizeUiEnums.cmake"
        ${UI_FILES_ABSOLUTE}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Normalizing Qt Designer UI enum names for Qt5 compatibility..."
    VERBATIM
)

add_dependencies(${PROJECT_NAME} normalize_ui_enums)
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY AUTOGEN_TARGET_DEPENDS normalize_ui_enums)

set(TS_FILES
    res/translations/omodsim_ru.ts
    res/translations/omodsim_zh_CN.ts
    res/translations/omodsim_zh_TW.ts
)

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

target_precompile_headers(${PROJECT_NAME} PRIVATE pch.h)
target_sources(${PROJECT_NAME} PRIVATE resources.qrc ${HEADERS} ${SOURCES} ${UI_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE Qt::Widgets Qt::Network Qt::Xml Qt::PrintSupport Qt::SerialBus Qt::SerialPort Qt::Qml Qt::Help Qt::Svg)

if(Qt6_FOUND)
    qt_add_lupdate(${PROJECT_NAME}
        TS_FILES ${TS_FILES}
        OPTIONS -no-obsolete
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt::Core5Compat)
    if(USE_QLEMENTINE_APP_STYLE)
        target_link_libraries(${PROJECT_NAME} PRIVATE qlementine qlementine-icons)
    endif()
else()
    add_custom_command(
        OUTPUT ${TS_FILES}
        COMMAND ${Qt5_LUPDATE_EXECUTABLE}
                ${SOURCES} ${HEADERS} ${UI_FILES}
                -no-obsolete
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
    set(PROJECT_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/res/omodsim-project.ico")

    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/omodsim.rc.in"
            "${PROJECT_BINARY_DIR}/omodsim.rc"
            @ONLY
        )

    target_sources(${PROJECT_NAME} PRIVATE "${PROJECT_BINARY_DIR}/omodsim.rc")
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)
elseif(APPLE)
    set(MACOSX_ICON "${CMAKE_CURRENT_SOURCE_DIR}/res/omodsim.icns")
    set_source_files_properties(${MACOSX_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(${PROJECT_NAME} PRIVATE ${MACOSX_ICON})

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/omodsim.plist.in"
        "${PROJECT_BINARY_DIR}/Info.plist"
        @ONLY
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST "${PROJECT_BINARY_DIR}/Info.plist"
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
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/docs
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
elseif(APPLE)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/Applications" CACHE PATH "Install path prefix" FORCE)
    endif()
    
    install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION .)

    install(CODE "
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
endif()
