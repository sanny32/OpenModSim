QT += core gui widgets network printsupport serialbus serialport

CONFIG += c++17
CONFIG -= debug_and_release
CONFIG -= debug_and_release_target

VERSION = 1.0.0b

QMAKE_TARGET_COMPANY = "Open ModSim"
QMAKE_TARGET_PRODUCT = omodsim
QMAKE_TARGET_DESCRIPTION = "Free modbus rtu/tcp slave (server) utility"
QMAKE_TARGET_DOMAIN = "com.omodsim.app"

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += APP_NAME=\"\\\"$${QMAKE_TARGET_COMPANY}\\\"\"
DEFINES += APP_DOMAIN=\"\\\"$${QMAKE_TARGET_DOMAIN}\\\"\"
DEFINES += APP_DESCRIPTION=\"\\\"$${QMAKE_TARGET_DESCRIPTION}\\\"\"
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

win32:RC_ICONS += res/omodsim.ico

INCLUDEPATH += controls \
               dialogs \

SOURCES += \
    controls/clickablelabel.cpp \
    controls/customframe.cpp \
    controls/customlineedit.cpp \
    controls/mainstatusbar.cpp \
    controls/numericlineedit.cpp \
    controls/outputwidget.cpp \
    controls/pointtypecombobox.cpp \
    controls/statisticwidget.cpp \
    dialogs/dialogabout.cpp \
    dialogs/dialogprintsettings.cpp \
    dialogs/dialogwritecoilregister.cpp \
    dialogs/dialogwriteholdingregister.cpp \
    dialogs/dialogwriteholdingregisterbits.cpp \
    formmodsim.cpp \
    main.cpp \
    mainwindow.cpp \
    modbusserver.cpp \
    qfixedsizedialog.cpp \
    qhexvalidator.cpp \
    recentfileactionlist.cpp \
    windowactionlist.cpp

HEADERS += \
    controls/clickablelabel.h \
    controls/customframe.h \
    controls/customlineedit.h \
    controls/mainstatusbar.h \
    controls/numericlineedit.h \
    controls/outputwidget.h \
    controls/pointtypecombobox.h \
    controls/statisticwidget.h \
    dialogs/dialogabout.h \
    dialogs/dialogprintsettings.h \
    dialogs/dialogwritecoilregister.h \
    dialogs/dialogwriteholdingregister.h \
    dialogs/dialogwriteholdingregisterbits.h \
    displaydefinition.h \
    enums.h \
    floatutils.h \
    formmodsim.h \
    mainwindow.h \
    modbuslimits.h \
    modbusserver.h \
    modbuswriteparams.h \
    qfixedsizedialog.h \
    qhexvalidator.h \
    qrange.h \
    recentfileactionlist.h \
    windowactionlist.h

FORMS += \
    controls/outputwidget.ui \
    controls/statisticwidget.ui \
    dialogs/dialogabout.ui \
    dialogs/dialogprintsettings.ui \
    dialogs/dialogwritecoilregister.ui \
    dialogs/dialogwriteholdingregister.ui \
    dialogs/dialogwriteholdingregisterbits.ui \
    formmodsim.ui \
    mainwindow.ui

RESOURCES += \
    resources.qrc
