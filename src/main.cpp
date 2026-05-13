// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file main.cpp
/// \brief Defines the application entry point.
///

#include <QFontDatabase>
#include <QIcon>
#include <QMessageBox>
#include "application.h"
#include "mainwindow.h"
#include "cmdlineparser.h"
#include "fontutils.h"
#include "styles/appstyle.h"

#ifdef HAVE_QLEMENTINE_APP_STYLE
#include "styles/qlementineappstyle.h"
#endif

#ifdef Q_OS_MAC
#include "styles/macappstyle.h"
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
namespace {
    static const int reg1 = qRegisterMetaType<QModbusRequest>();
    static const int reg2 = qRegisterMetaType<QModbusResponse>();
    static const int reg3 = qRegisterMetaType<QModbusDataUnit>();
    static const int reg4 = qRegisterMetaType<ModbusMessage::ProtocolType>();
    static const int reg5 = qRegisterMetaType<QSharedPointer<const ModbusMessage>>();
    static const int reg6 = qRegisterMetaType<ModbusDefinitions>();
    static const int reg7 = qRegisterMetaType<QList<int>>();
}
#endif

///
/// \brief isConsoleOutputAvailable
/// \return
///
static inline bool isConsoleOutputAvailable()
{
#ifdef Q_OS_WIN
    return false;
#else
    return true;
#endif
}

///
/// \brief showVersion
///
static inline void showVersion()
{
    const auto version = QString("%1\n").arg(APP_VERSION);
    if(!isConsoleOutputAvailable()){
        QMessageBox msg(QMessageBox::Information, APP_PRODUCT_NAME, qPrintable(version));
        msg.setFont(defaultMonospaceFont());
        msg.exec();
    }
    else {
        fputs(qPrintable(version), stdout);
        fflush(stdout);
    }
}

///
/// \brief showHelp
/// \param helpText
///
static inline void showHelp(const QString& helpText)
{
    if(!isConsoleOutputAvailable()){
        QMessageBox msg(QMessageBox::Information, APP_PRODUCT_NAME, qPrintable(helpText));
        msg.setFont(defaultMonospaceFont());
        msg.exec();
    }
    else {
        fputs(qPrintable(helpText), stdout);
        fflush(stdout);
    }
}

///
/// \brief showErrorMessage
/// \param message
///
static void showErrorMessage(const QString &message)
{
    if(!isConsoleOutputAvailable()){
        QMessageBox msg(QMessageBox::Critical, APP_PRODUCT_NAME, qPrintable(message));
        msg.setFont(defaultMonospaceFont());
        msg.exec();
    }
    else {
        fputs(qPrintable(message), stderr);
        fflush(stderr);
    }
}

///
/// \brief main
/// \param argc
/// \param argv
/// \return
///
int main(int argc, char *argv[])
{
    Application a(argc, argv);
    a.setApplicationName(APP_NAME);
    a.setApplicationVersion(APP_VERSION);
#ifdef Q_OS_LINUX
    a.setDesktopFileName(QStringLiteral("omodsim%1").arg(APP_VERSION_MAJOR));
#endif

#if defined(HAVE_QLEMENTINE_APP_STYLE) && defined(Q_OS_MAC)
    a.setStyle(new MacAppStyle());
#elif defined(HAVE_QLEMENTINE_APP_STYLE)
    a.setStyle(new QlementineAppStyle());
#elif defined(Q_OS_WIN)
    a.setStyle(new AppStyle("windowsvista"));
#else
    a.setStyle(new AppStyle("fusion"));
#endif

    QFontDatabase::addApplicationFont(":/fonts/firacode.ttf");

    CmdLineParser parser;
    if(!parser.parse(a.arguments()))
    {
        showErrorMessage(parser.errorText() + QLatin1Char('\n'));
        return EXIT_FAILURE;
    }

    if(parser.isSet(CmdLineParser::_version))
    {
        showVersion();
        return EXIT_SUCCESS;
    }

    if(parser.isSet(CmdLineParser::_help))
    {
        showHelp(parser.helpText());
        return EXIT_SUCCESS;
    }

    QString profile;
    if(parser.isSet(CmdLineParser::_profile)) {
        profile = parser.value(CmdLineParser::_profile);
    }

    const QString cfg = parser.projectFile();
    const bool noSession = parser.isSet(CmdLineParser::_no_session);

    MainWindow w(profile, !noSession, cfg);
    w.show();

    return a.exec();
}
