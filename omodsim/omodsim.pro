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
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc
