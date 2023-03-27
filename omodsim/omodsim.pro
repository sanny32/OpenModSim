QT += core gui widgets qml network printsupport serialbus serialport

CONFIG += c++17
CONFIG -= debug_and_release
CONFIG -= debug_and_release_target

VERSION = 1.2.0

QMAKE_TARGET_PRODUCT = "Open ModSim"
QMAKE_TARGET_DESCRIPTION = "An Open Source Modbus Slave (Server) Utility"

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += APP_NAME=\"\\\"$${QMAKE_TARGET_PRODUCT}\\\"\"
DEFINES += APP_DESCRIPTION=\"\\\"$${QMAKE_TARGET_DESCRIPTION}\\\"\"
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

win32:RC_ICONS += res/omodsim.ico

INCLUDEPATH += controls \
               dialogs \
               jsobjects

SOURCES += \
    controls/booleancombobox.cpp \
    controls/clickablelabel.cpp \
    controls/consoleoutput.cpp \
    controls/customframe.cpp \
    controls/customlineedit.cpp \
    controls/flowcontroltypecombobox.cpp \
    controls/jscodeeditor.cpp \
    controls/mainstatusbar.cpp \
    controls/numericcombobox.cpp \
    controls/numericlineedit.cpp \
    controls/outputwidget.cpp \
    controls/paritytypecombobox.cpp \
    controls/pointtypecombobox.cpp \
    controls/runmodecombobox.cpp \
    controls/scriptcontrol.cpp \
    controls/searchlineedit.cpp \
    controls/simulationmodecombobox.cpp \
    datasimulator.cpp \
    dialogs/dialogautosimulation.cpp \
    dialogs/dialogcoilsimulation.cpp \
    dialogs/dialogabout.cpp \
    dialogs/dialogdisplaydefinition.cpp \
    dialogs/dialogforcemultiplecoils.cpp \
    dialogs/dialogforcemultipleregisters.cpp \
    dialogs/dialogprintsettings.cpp \
    dialogs/dialogscriptsettings.cpp \
    dialogs/dialogselectserviceport.cpp \
    dialogs/dialogsetuppresetdata.cpp \
    dialogs/dialogsetupserialport.cpp \
    dialogs/dialogwindowsmanager.cpp \
    dialogs/dialogwritecoilregister.cpp \
    dialogs/dialogwriteholdingregister.cpp \
    dialogs/dialogwriteholdingregisterbits.cpp \
    jscompleter.cpp \
    jsobjects/console.cpp \
    jsobjects/script.cpp \
    jsobjects/server.cpp \
    formmodsim.cpp \
    jshighlighter.cpp \
    jsobjects/storage.cpp \
    main.cpp \
    mainwindow.cpp \
    menuconnect.cpp \
    modbusdataunitmap.cpp \
    modbusmultiserver.cpp \
    qfixedsizedialog.cpp \
    qhexvalidator.cpp \
    recentfileactionlist.cpp \
    windowactionlist.cpp

HEADERS += \
    byteorderutils.h \
    connectiondetails.h \
    controls/booleancombobox.h \
    controls/clickablelabel.h \
    controls/consoleoutput.h \
    controls/customframe.h \
    controls/customlineedit.h \
    controls/flowcontroltypecombobox.h \
    controls/jscodeeditor.h \
    controls/mainstatusbar.h \
    controls/numericcombobox.h \
    controls/numericlineedit.h \
    controls/outputwidget.h \
    controls/paritytypecombobox.h \
    controls/pointtypecombobox.h \
    controls/runmodecombobox.h \
    controls/scriptcontrol.h \
    controls/searchlineedit.h \
    controls/simulationmodecombobox.h \
    datasimulator.h \
    dialogs/dialogautosimulation.h \
    dialogs/dialogcoilsimulation.h \
    dialogs/dialogabout.h \
    dialogs/dialogdisplaydefinition.h \
    dialogs/dialogforcemultiplecoils.h \
    dialogs/dialogforcemultipleregisters.h \
    dialogs/dialogprintsettings.h \
    dialogs/dialogscriptsettings.h \
    dialogs/dialogselectserviceport.h \
    dialogs/dialogsetuppresetdata.h \
    dialogs/dialogsetupserialport.h \
    dialogs/dialogwindowsmanager.h \
    dialogs/dialogwritecoilregister.h \
    dialogs/dialogwriteholdingregister.h \
    dialogs/dialogwriteholdingregisterbits.h \
    jscompleter.h \
    jsobjects/console.h \
    jsobjects/script.h \
    jsobjects/server.h \
    displaydefinition.h \
    enums.h \
    floatutils.h \
    formmodsim.h \
    jshighlighter.h \
    jsobjects/storage.h \
    mainwindow.h \
    menuconnect.h \
    modbusdataunitmap.h \
    modbuslimits.h \
    modbusmultiserver.h \
    modbussimulationparams.h \
    modbuswriteparams.h \
    qfixedsizedialog.h \
    qhexvalidator.h \
    qrange.h \
    recentfileactionlist.h \
    scriptsettings.h \
    windowactionlist.h

FORMS += \
    controls/outputwidget.ui \
    controls/scriptcontrol.ui \
    dialogs/dialogautosimulation.ui \
    dialogs/dialogcoilsimulation.ui \
    dialogs/dialogabout.ui \
    dialogs/dialogdisplaydefinition.ui \
    dialogs/dialogforcemultiplecoils.ui \
    dialogs/dialogforcemultipleregisters.ui \
    dialogs/dialogprintsettings.ui \
    dialogs/dialogscriptsettings.ui \
    dialogs/dialogselectserviceport.ui \
    dialogs/dialogsetuppresetdata.ui \
    dialogs/dialogsetupserialport.ui \
    dialogs/dialogwindowsmanager.ui \
    dialogs/dialogwritecoilregister.ui \
    dialogs/dialogwriteholdingregister.ui \
    dialogs/dialogwriteholdingregisterbits.ui \
    formmodsim.ui \
    mainwindow.ui

RESOURCES += \
    resources.qrc

TRANSLATIONS += \
    translations/omodsim_ru.ts
