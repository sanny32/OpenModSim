QT += core gui widgets qml network printsupport serialbus serialport help

CONFIG += c++17
CONFIG -= debug_and_release
CONFIG -= debug_and_release_target

VERSION = 1.7.0

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
               jsobjects \
               modbusmessages \

SOURCES += \
    cmdlineparser.cpp \
    controls/addressbasecombobox.cpp \
    controls/booleancombobox.cpp \
    controls/bytelisttextedit.cpp \
    controls/clickablelabel.cpp \
    controls/consoleoutput.cpp \
    controls/customframe.cpp \
    controls/customlineedit.cpp \
    controls/flowcontroltypecombobox.cpp \
    controls/helpwidget.cpp \
    controls/jscodeeditor.cpp \
    controls/mainstatusbar.cpp \
    controls/modbuslogwidget.cpp \
    controls/modbusmessagewidget.cpp \
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
    dialogs/dialogmsgparser.cpp \
    dialogs/dialogprintsettings.cpp \
    dialogs/dialogscriptsettings.cpp \
    dialogs/dialogselectserviceport.cpp \
    dialogs/dialogsetuppresetdata.cpp \
    dialogs/dialogsetupserialport.cpp \
    dialogs/dialogwindowsmanager.cpp \
    dialogs/dialogwritecoilregister.cpp \
    dialogs/dialogwriteholdingregister.cpp \
    dialogs/dialogwriteholdingregisterbits.cpp \
    htmldelegate.cpp \
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
    modbusmessages/modbusmessage.cpp \
    modbusmultiserver.cpp \
    qfixedsizedialog.cpp \
    qhexvalidator.cpp \
    qint64validator.cpp \
    quintvalidator.cpp \
    recentfileactionlist.cpp \
    windowactionlist.cpp

HEADERS += \
    byteorderutils.h \
    cmdlineparser.h \
    connectiondetails.h \
    controls/addressbasecombobox.h \
    controls/booleancombobox.h \
    controls/bytelisttextedit.h \
    controls/clickablelabel.h \
    controls/consoleoutput.h \
    controls/customframe.h \
    controls/customlineedit.h \
    controls/flowcontroltypecombobox.h \
    controls/helpwidget.h \
    controls/jscodeeditor.h \
    controls/mainstatusbar.h \
    controls/modbuslogwidget.h \
    controls/modbusmessagewidget.h \
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
    dialogs/dialogmsgparser.h \
    dialogs/dialogprintsettings.h \
    dialogs/dialogscriptsettings.h \
    dialogs/dialogselectserviceport.h \
    dialogs/dialogsetuppresetdata.h \
    dialogs/dialogsetupserialport.h \
    dialogs/dialogwindowsmanager.h \
    dialogs/dialogwritecoilregister.h \
    dialogs/dialogwriteholdingregister.h \
    dialogs/dialogwriteholdingregisterbits.h \
    formatutils.h \
    htmldelegate.h \
    jscompleter.h \
    jsobjects/console.h \
    jsobjects/script.h \
    jsobjects/server.h \
    displaydefinition.h \
    enums.h \
    formmodsim.h \
    jshighlighter.h \
    jsobjects/storage.h \
    mainwindow.h \
    menuconnect.h \
    modbusdataunitmap.h \
    modbuslimits.h \
    modbusmessages/diagnostics.h \
    modbusmessages/getcommeventcounter.h \
    modbusmessages/getcommeventlog.h \
    modbusmessages/maskwriteregister.h \
    modbusmessages/modbusmessage.h \
    modbusmessages/modbusmessages.h \
    modbusmessages/readcoils.h \
    modbusmessages/readdiscreteinputs.h \
    modbusmessages/readexceptionstatus.h \
    modbusmessages/readfifoqueue.h \
    modbusmessages/readfilerecord.h \
    modbusmessages/readholdingregisters.h \
    modbusmessages/readinputregisters.h \
    modbusmessages/readwritemultipleregisters.h \
    modbusmessages/reportserverid.h \
    modbusmessages/writefilerecord.h \
    modbusmessages/writemultiplecoils.h \
    modbusmessages/writemultipleregisters.h \
    modbusmessages/writesinglecoil.h \
    modbusmessages/writesingleregister.h \
    modbusmultiserver.h \
    modbussimulationparams.h \
    modbuswriteparams.h \
    numericutils.h \
    qfixedsizedialog.h \
    qhexvalidator.h \
    qint64validator.h \
    qmodbusadu.h \
    qmodbusadurtu.h \
    qmodbusadutcp.h \
    qrange.h \
    quintvalidator.h \
    recentfileactionlist.h \
    scriptsettings.h \
    serialportutils.h \
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
    dialogs/dialogmsgparser.ui \
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

DISTFILES += \
    docs/jshelp.qhcp \
    docs/jshelp.qhp

# Genreate docs files
INPUT = $$PWD/docs/jshelp.qhcp
HELPGENERATOR = $$[QT_INSTALL_BINS]/qhelpgenerator
greaterThan(QT_MAJOR_VERSION, 5) {
unix:HELPGENERATOR = $$[QT_INSTALL_LIBEXECS]/qhelpgenerator
}
helpgenerator.commands = $$quote($$HELPGENERATOR) $$quote($$INPUT)
QMAKE_EXTRA_TARGETS += helpgenerator
PRE_TARGETDEPS += helpgenerator

# Create outpus docs directory
OUT_DOCS = $$OUT_PWD/docs
win32:OUT_DOCS ~= s,/,\\,g
create_dir.commands = $$sprintf($$QMAKE_MKDIR_CMD, $$quote($$OUT_DOCS))
QMAKE_EXTRA_TARGETS += create_dir
POST_TARGETDEPS += create_dir

# Copy doc files
DOC_FILES = $$PWD/docs/jshelp.qhc \
            $$PWD/docs/jshelp.qch
for(file, DOC_FILES) {
    win32:file ~= s,/,\\,g
    !isEmpty(QMAKE_POST_LINK) QMAKE_POST_LINK += &&
    QMAKE_POST_LINK += $$QMAKE_MOVE $$quote($$file) $$quote($$OUT_DOCS)
}

