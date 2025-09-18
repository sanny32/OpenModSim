#include <QApplication>
#include <QFontDatabase>
#include <QMessageBox>
#include "mainwindow.h"
#include "cmdlineparser.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <io.h>
#include <fcntl.h>
#endif

///
/// \brief g_consoleAttached
///
static bool g_consoleAttached = false;


///
/// \brief isConsoleAvailable
/// \return
///
static bool isConsoleAvailable()
{
#ifdef Q_OS_WIN
    if (GetConsoleWindow()){
        return true;
    }
    else if (AttachConsole(ATTACH_PARENT_PROCESS)){
        g_consoleAttached = true;

        FILE* fDummy;
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);

        setvbuf(stdout, nullptr, _IONBF, 0);
        setvbuf(stderr, nullptr, _IONBF, 0);

        return true;
    }
    else {
        STARTUPINFO startupInfo;
        startupInfo.cb = sizeof(STARTUPINFO);
        GetStartupInfo(&startupInfo);
        return (startupInfo.dwFlags & STARTF_USESTDHANDLES);
    }
#else
    return true;
#endif
}

///
/// \brief freeConsoleIfAttached
///
static void freeConsoleIfAttached()
{
#ifdef Q_OS_WIN
    if(g_consoleAttached) {
        //FreeConsole();
        g_consoleAttached = false;
    }
 #endif
}

///
/// \brief showVersion
///
static inline void showVersion()
{
    const auto version = QString("%1\n").arg(APP_VERSION);
    if(!isConsoleAvailable()){
        QMessageBox::information(nullptr, APP_NAME, qPrintable(version));
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
    if(!isConsoleAvailable()){
        QMessageBox::information(nullptr, APP_NAME, qPrintable(helpText));
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
    if(!isConsoleAvailable()){
        QMessageBox::critical(nullptr, APP_NAME, qPrintable(message));
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
    std::atexit(freeConsoleIfAttached);

    QApplication a(argc, argv);
    a.setApplicationName(APP_NAME);
    a.setApplicationVersion(APP_VERSION);
    a.setStyle("windowsvista");

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

    QString cfg;
    if(parser.isSet(CmdLineParser::_config))
    {
        cfg = parser.value(CmdLineParser::_config);
    }

    MainWindow w;
    if(!cfg.isEmpty())
    {
        w.loadConfig(cfg, true);
    }
    w.show();

    return a.exec();
}
