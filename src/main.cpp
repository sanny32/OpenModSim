#include <QApplication>
#include <QFontDatabase>
#include <QMessageBox>
#include "controls/appstyle.h"
#include "mainwindow.h"
#include "cmdlineparser.h"
#include "fontutils.h"

#if defined(Q_OS_MACOS) && QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include "controls/macappstyle.h"
#include <oclero/qlementine.hpp>
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
    QApplication a(argc, argv);
    a.setApplicationName(APP_NAME);
    a.setApplicationVersion(APP_VERSION);
#ifdef Q_OS_LINUX
    a.setDesktopFileName(QStringLiteral("omodsim%1").arg(APP_VERSION_MAJOR));
#endif

#  ifdef Q_OS_WIN
    a.setStyle(new AppStyle("windowsvista"));
#  elif defined(Q_OS_MAC)
    a.setStyle(new MacAppStyle());
#  else
    a.setStyle(new AppStyle("fusion"));
#  endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
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
