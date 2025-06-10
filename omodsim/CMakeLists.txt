cmake_minimum_required(VERSION 3.25)

project(OpenModSim 
        VERSION 1.8.1 
        LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PRODUCT_NAME "Open ModSim")

# Find Qt6 or fallback to Qt5
find_package(Qt6 COMPONENTS Core Gui Widgets Network PrintSupport SerialBus SerialPort Core5Compat Qml Help LinguistTools QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Core Gui Widgets Network PrintSupport SerialBus SerialPort Qml Help LinguistTools QUIET)
endif()

# Define target
add_executable(omodsim
    ansiutils.h
    ansimenu.cpp ansimenu.h)

# Define compile definitions
add_compile_definitions(APP_NAME="${PRODUCT_NAME}")
add_compile_definitions(APP_DESCRIPTION="${PROJECT_DESCRIPTION}")
add_compile_definitions(APP_VERSION="${PROJECT_VERSION}")

# Include directories
target_include_directories(omodsim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/controls
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/jsobjects
    ${CMAKE_CURRENT_SOURCE_DIR}/modbusmessages
)

# Source files
set(SOURCES
    cmdlineparser.cpp
    controls/addressbasecombobox.cpp
    controls/booleancombobox.cpp
    controls/bytelisttextedit.cpp
    controls/clickablelabel.cpp
    controls/consoleoutput.cpp
    controls/customframe.cpp
    controls/customlineedit.cpp
    controls/flowcontroltypecombobox.cpp
    controls/helpwidget.cpp
    controls/jscodeeditor.cpp
    controls/mainstatusbar.cpp
    controls/modbuslogwidget.cpp
    controls/modbusmessagewidget.cpp
    controls/numericcombobox.cpp
    controls/numericlineedit.cpp
    controls/outputwidget.cpp
    controls/paritytypecombobox.cpp
    controls/pointtypecombobox.cpp
    controls/runmodecombobox.cpp
    controls/scriptcontrol.cpp
    controls/searchlineedit.cpp
    controls/simulationmodecombobox.cpp
    datasimulator.cpp
    dialogs/dialogautosimulation.cpp
    dialogs/dialogcoilsimulation.cpp
    dialogs/dialogabout.cpp
    dialogs/dialogdisplaydefinition.cpp
    dialogs/dialogforcemultiplecoils.cpp
    dialogs/dialogforcemultipleregisters.cpp
    dialogs/dialogmsgparser.cpp
    dialogs/dialogprintsettings.cpp
    dialogs/dialogscriptsettings.cpp
    dialogs/dialogselectserviceport.cpp
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
    formmodsim.cpp
    jshighlighter.cpp
    jsobjects/storage.cpp
    main.cpp
    mainwindow.cpp
    menuconnect.cpp
    modbusdataunitmap.cpp
    modbusmessages/modbusmessage.cpp
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
    byteorderutils.h
    cmdlineparser.h
    connectiondetails.h
    controls/addressbasecombobox.h
    controls/booleancombobox.h
    controls/bytelisttextedit.h
    controls/clickablelabel.h
    controls/consoleoutput.h
    controls/customframe.h
    controls/customlineedit.h
    controls/flowcontroltypecombobox.h
    controls/helpwidget.h
    controls/jscodeeditor.h
    controls/mainstatusbar.h
    controls/modbuslogwidget.h
    controls/modbusmessagewidget.h
    controls/numericcombobox.h
    controls/numericlineedit.h
    controls/outputwidget.h
    controls/paritytypecombobox.h
    controls/pointtypecombobox.h
    controls/runmodecombobox.h
    controls/scriptcontrol.h
    controls/searchlineedit.h
    controls/simulationmodecombobox.h
    datasimulator.h
    dialogs/dialogautosimulation.h
    dialogs/dialogcoilsimulation.h
    dialogs/dialogabout.h
    dialogs/dialogdisplaydefinition.h
    dialogs/dialogforcemultiplecoils.h
    dialogs/dialogforcemultipleregisters.h
    dialogs/dialogmsgparser.h
    dialogs/dialogprintsettings.h
    dialogs/dialogscriptsettings.h
    dialogs/dialogselectserviceport.h
    dialogs/dialogsetuppresetdata.h
    dialogs/dialogsetupserialport.h
    dialogs/dialogwindowsmanager.h
    dialogs/dialogwritecoilregister.h
    dialogs/dialogwriteholdingregister.h
    dialogs/dialogwriteholdingregisterbits.h
    formatutils.h
    htmldelegate.h
    jscompleter.h
    jsobjects/console.h
    jsobjects/script.h
    jsobjects/server.h
    displaydefinition.h
    enums.h
    formmodsim.h
    jshighlighter.h
    jsobjects/storage.h
    mainwindow.h
    menuconnect.h
    modbusdataunitmap.h
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
    modbussimulationparams.h
    modbuswriteparams.h
    numericutils.h
    qfixedsizedialog.h
    qhexvalidator.h
    qint64validator.h
    qmodbusadu.h
    qmodbusadurtu.h
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
    controls/outputwidget.ui
    controls/scriptcontrol.ui
    dialogs/dialogautosimulation.ui
    dialogs/dialogcoilsimulation.ui
    dialogs/dialogabout.ui
    dialogs/dialogdisplaydefinition.ui
    dialogs/dialogforcemultiplecoils.ui
    dialogs/dialogforcemultipleregisters.ui
    dialogs/dialogmsgparser.ui
    dialogs/dialogprintsettings.ui
    dialogs/dialogscriptsettings.ui
    dialogs/dialogselectserviceport.ui
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
)

set(JSHELP_INPUT "${CMAKE_SOURCE_DIR}/docs/jshelp.qhcp")
set(JSHELP_QCH "${CMAKE_BINARY_DIR}/docs/jshelp.qch")
set(JSHELP_QHC "${CMAKE_BINARY_DIR}/docs/jshelp.qhc")

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/docs)

find_program(QHELP_GENERATOR_EXECUTABLE qhelpgenerator
             HINTS "${QT_BIN_DIR}"
             NO_DEFAULT_PATH)
if(NOT QHELP_GENERATOR_EXECUTABLE)
    find_program(QHELP_GENERATOR_EXECUTABLE qhelpgenerator)
endif()

if(QHELP_GENERATOR_EXECUTABLE)
    message(STATUS "qhelpgenerator found at: ${QHELP_GENERATOR_EXECUTABLE}")
else()
    message(FATAL_ERROR "qhelpgenerator not found")
endif()

# Link libraries
target_sources(omodsim PRIVATE resources.qrc ${HEADERS} ${SOURCES} ${UI_FILES} ${TS_FILES})
target_link_libraries(omodsim PRIVATE Qt::Widgets Qt::Network Qt::PrintSupport Qt::SerialBus Qt::SerialPort Qt::Qml Qt::Help)

if(Qt6_FOUND)
    qt_add_lupdate(omodsim
        TS_FILES ${TS_FILES}
        SOURCES ${SOURCES} ${HEADERS} ${UI_FILES}
        OUTPUT_DIR translations
    )
    target_link_libraries(omodsim PRIVATE Qt::Core5Compat)
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
    add_dependencies(omodsim update_translations)
endif()

if(WIN32)
    string(REPLACE "." ";" VERSION_LIST ${PROJECT_VERSION})
    list(GET VERSION_LIST 0 VERSION_MAJOR)
    list(GET VERSION_LIST 1 VERSION_MINOR)
    list(GET VERSION_LIST 2 VERSION_PATCH)

    set(EXECUTABLE_NAME "omodsim")
    set(ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/res/omodsim.ico")
    set(RC_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/omodsim.rc.in")
    set(GENERATED_RC "${CMAKE_BINARY_DIR}/omodsim.rc")

    configure_file(${RC_TEMPLATE} ${GENERATED_RC} @ONLY)

    target_sources(omodsim PRIVATE ${ICON_PATH} ${GENERATED_RC})
    set_target_properties(omodsim PROPERTIES WIN32_EXECUTABLE ON)
endif()

add_custom_command(
    OUTPUT ${JSHELP_QCH} ${JSHELP_QHC}
    COMMAND ${QHELP_GENERATOR_EXECUTABLE} ${JSHELP_INPUT}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs
    COMMENT "Generating help files..."
    VERBATIM
)
add_custom_target(helpgenerator ALL DEPENDS ${JSHELP_QCH} ${JSHELP_QHC})
add_dependencies(omodsim helpgenerator)
