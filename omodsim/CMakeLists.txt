cmake_minimum_required(VERSION 3.16)

project(omodsim
        VERSION 1.11.3
        DESCRIPTION "An Open Source Modbus Slave (Server) Utility"
        LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(PRODUCT_NAME "Open ModSim")
set(EXECUTABLE_PATH "${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME}")

# Split version to major, minor, patch
string(REPLACE "." ";" VERSION_LIST ${PROJECT_VERSION})
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)
list(GET VERSION_LIST 2 VERSION_PATCH)

# Find Qt6 or fallback to Qt5
find_package(Qt6 COMPONENTS Core Gui Widgets Network Xml PrintSupport SerialBus SerialPort Core5Compat Qml Help LinguistTools QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Core Gui Widgets Network Xml PrintSupport SerialBus SerialPort Qml Help LinguistTools QUIET)
endif()

if (NOT Qt6_FOUND AND NOT Qt5_FOUND)
    message(FATAL_ERROR "One or more Qt development packages not found. Please install Qt5 or Qt6 development packages.")
endif()

if(Qt6_FOUND)
    get_target_property(QT_BINARY_DIR Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BINARY_DIR "${QT_BINARY_DIR}" DIRECTORY)
    get_filename_component(QT_LIBEXEC_DIR "${QT_BINARY_DIR}/../libexec" REALPATH)
else()
    get_target_property(QT_BINARY_DIR Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BINARY_DIR "${QT_BINARY_DIR}" DIRECTORY)
    set(QT_LIBEXEC_DIR "${QT_BINARY_DIR}")
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

if(${GIT_BRANCH} MATCHES dev)
    set(PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}-dev)
endif()

# Define compile definitions
add_compile_definitions(APP_NAME="${PRODUCT_NAME}")
add_compile_definitions(APP_DESCRIPTION="${PROJECT_DESCRIPTION}")
add_compile_definitions(APP_VERSION="${PROJECT_VERSION}")

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
    cmdlineparser.cpp
    controls/aboutdatawidget.cpp
    controls/addressbasecombobox.cpp
    controls/addressspacecombobox.cpp
    controls/booleancombobox.cpp
    controls/bytelisttextedit.cpp
    controls/clickablelabel.cpp
    controls/consoleoutput.cpp
    controls/customframe.cpp
    controls/customlineedit.cpp
    controls/flowcontroltypecombobox.cpp
    controls/helpwidget.cpp
    controls/jscodeeditor.cpp
    controls/jscriptcontrol.cpp
    controls/mainstatusbar.cpp
    controls/modbuslogwidget.cpp
    controls/modbusmessagewidget.cpp
    controls/numericcombobox.cpp
    controls/numericlineedit.cpp
    controls/outputwidget.cpp
    controls/paritytypecombobox.cpp
    controls/pointtypecombobox.cpp
    controls/runmodecombobox.cpp
    controls/searchlineedit.cpp
    controls/simulationmodecombobox.cpp
    controls/statisticwidget.cpp
    datasimulator.cpp
    dialogs/dialogautosimulation.cpp
    dialogs/dialogcoilsimulation.cpp
    dialogs/dialogabout.cpp
    dialogs/dialogdisplaydefinition.cpp
    dialogs/dialogforcemultiplecoils.cpp
    dialogs/dialogforcemultipleregisters.cpp
    dialogs/dialogmsgparser.cpp
    dialogs/dialogprintsettings.cpp
    dialogs/dialograwdatalog.cpp
    dialogs/dialogscriptsettings.cpp
    dialogs/dialogselectserviceport.cpp
    dialogs/dialogmodbusdefinitions.cpp
    dialogs/dialogsetuppresetdata.cpp
    dialogs/dialogsetupserialport.cpp
    dialogs/dialogwindowsmanager.cpp
    dialogs/dialogwritecoilregister.cpp
    dialogs/dialogwriteholdingregister.cpp
    dialogs/dialogwriteholdingregisterbits.cpp
    htmldelegate.cpp
    jscompleter.cpp
    jsobjects/console.cpp
    jsobjects/script.cpp
    jsobjects/server.cpp
    jsobjects/storage.cpp
    formmodsim.cpp
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
    qfixedsizedialog.cpp
    qhexvalidator.cpp
    qint64validator.cpp
    quintvalidator.cpp
    recentfileactionlist.cpp
    windowactionlist.cpp
)

# Header files
set(HEADERS
    ansimenu.h
    ansiutils.h
    bufferinglistmodel.h
    byteorderutils.h
    cmdlineparser.h
    connectiondetails.h
    controls/aboutdatawidget.h
    controls/addressbasecombobox.h
    controls/addressspacecombobox.h
    controls/booleancombobox.h
    controls/bytelisttextedit.h
    controls/clickablelabel.h
    controls/consoleoutput.h
    controls/customframe.h
    controls/customlineedit.h
    controls/flowcontroltypecombobox.h
    controls/helpwidget.h
    controls/jscodeeditor.h
    controls/jscriptcontrol.h
    controls/mainstatusbar.h
    controls/modbuslogwidget.h
    controls/modbusmessagewidget.h
    controls/numericcombobox.h
    controls/numericlineedit.h
    controls/outputwidget.h
    controls/paritytypecombobox.h
    controls/pointtypecombobox.h
    controls/runmodecombobox.h
    controls/searchlineedit.h
    controls/simulationmodecombobox.h
    controls/statisticwidget.h
    datasimulator.h
    dialogs/dialogautosimulation.h
    dialogs/dialogcoilsimulation.h
    dialogs/dialogabout.h
    dialogs/dialogdisplaydefinition.h
    dialogs/dialogforcemultiplecoils.h
    dialogs/dialogforcemultipleregisters.h
    dialogs/dialogmsgparser.h
    dialogs/dialogprintsettings.h
    dialogs/dialograwdatalog.h
    dialogs/dialogscriptsettings.h
    dialogs/dialogmodbusdefinitions.h
    dialogs/dialogselectserviceport.h
    dialogs/dialogsetuppresetdata.h
    dialogs/dialogsetupserialport.h
    dialogs/dialogwindowsmanager.h
    dialogs/dialogwritecoilregister.h
    dialogs/dialogwriteholdingregister.h
    dialogs/dialogwriteholdingregisterbits.h
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
    formmodsim.h
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
    qfixedsizedialog.h
    qhexvalidator.h
    qint64validator.h
    qmodbusadu.h
    qmodbusadurtu.h
    qcountedset.h
    qmodbuscommevent.h
    qmodbusadutcp.h
    qrange.h
    quintvalidator.h
    recentfileactionlist.h
    scriptsettings.h
    serialportutils.h
    windowactionlist.h
)

# UI Forms
set(UI_FILES
    controls/jscriptcontrol.ui
    controls/outputwidget.ui
    controls/statisticwidget.ui
    dialogs/dialogautosimulation.ui
    dialogs/dialogcoilsimulation.ui
    dialogs/dialogabout.ui
    dialogs/dialogdisplaydefinition.ui
    dialogs/dialogforcemultiplecoils.ui
    dialogs/dialogforcemultipleregisters.ui
    dialogs/dialogmsgparser.ui
    dialogs/dialogprintsettings.ui
    dialogs/dialograwdatalog.ui
    dialogs/dialogscriptsettings.ui
    dialogs/dialogselectserviceport.ui
    dialogs/dialogmodbusdefinitions.ui
    dialogs/dialogsetuppresetdata.ui
    dialogs/dialogsetupserialport.ui
    dialogs/dialogwindowsmanager.ui
    dialogs/dialogwritecoilregister.ui
    dialogs/dialogwriteholdingregister.ui
    dialogs/dialogwriteholdingregisterbits.ui
    formmodsim.ui
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

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/docs)

find_program(QHELP_GENERATOR_EXECUTABLE qhelpgenerator
             HINTS "${QT_BINARY_DIR}" "${QT_LIBEXEC_DIR}"
             NO_DEFAULT_PATH)
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
        SOURCES ${SOURCES} ${HEADERS} ${UI_FILES}
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
    set(ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/res/omodsim.ico")

    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/omodsim.rc.in"
            "${CMAKE_BINARY_DIR}/omodsim.rc"
            @ONLY
        )

    target_sources(${PROJECT_NAME} PRIVATE ${ICON_PATH} "${CMAKE_BINARY_DIR}/omodsim.rc")
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)
elseif(LINUX)
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
endif()

add_custom_command(
    OUTPUT ${JSHELP_QCH} ${JSHELP_QHC}
    COMMAND ${CMAKE_COMMAND} -E copy ${JSHELP_INPUT} ${JSHELP_QHCP}
    COMMAND ${QHELP_GENERATOR_EXECUTABLE} ${JSHELP_QHP} -o ${JSHELP_QCH}
    COMMAND ${QHELP_GENERATOR_EXECUTABLE} ${JSHELP_QHCP} -o ${JSHELP_QHC}
    COMMAND ${CMAKE_COMMAND} -E remove ${JSHELP_QHCP}
    DEPENDS ${JSHELP_QHP} ${JSHELP_INPUT}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/docs
    COMMENT "Generating and moving help files..."
    VERBATIM
)

add_custom_target(helpgenerator ALL DEPENDS ${JSHELP_QCH} ${JSHELP_QHC})
add_dependencies(${PROJECT_NAME} helpgenerator)

if(LINUX)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/omodsim.desktop.in"
        "${CMAKE_CURRENT_BINARY_DIR}/omodsim.desktop"
        @ONLY
    )

    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
    install(FILES res/omodsim16.png DESTINATION share/icons/hicolor/16x16/apps RENAME omodsim.png)
    install(FILES res/omodsim32.png DESTINATION share/icons/hicolor/32x32/apps RENAME omodsim.png)
    install(FILES res/omodsim64.png DESTINATION share/icons/hicolor/64x64/apps RENAME omodsim.png)
    install(FILES res/omodsim128.png DESTINATION share/icons/hicolor/128x128/apps RENAME omodsim.png)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/omodsim.desktop" DESTINATION share/applications)

    install(CODE "execute_process(COMMAND update-desktop-database -q ${CMAKE_INSTALL_PREFIX}/share/applications)")
    install(CODE "execute_process(COMMAND gtk-update-icon-cache -q -t -f ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor)")
endif()
