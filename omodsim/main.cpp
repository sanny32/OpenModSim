#include <QApplication>
#include <QFontDatabase>
#include <QMessageBox>
#include "mainwindow.h"
#include "cmdlineparser.h"
#include "fontutils.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
namespace {
    static const int reg1 = qRegisterMetaType<QModbusRequest>();
    static const int reg2 = qRegisterMetaType<QModbusResponse>();
    static const int reg3 = qRegisterMetaType<QModbusDataUnit>();
    static const int reg4 = qRegisterMetaType<ModbusMessage::ProtocolType>();
    static const int reg5 = qRegisterMetaType<QSharedPointer<const ModbusMessage>>();
    static const int reg6 = qRegisterMetaType<ModbusDefinitions>();
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
        QMessageBox msg(QMessageBox::Information, APP_NAME, qPrintable(version));
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
        QMessageBox msg(QMessageBox::Information, APP_NAME, qPrintable(helpText));
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
        QMessageBox msg(QMessageBox::Critical, APP_NAME, qPrintable(message));
        msg.setFont(defaultMonospaceFont());
        msg.exec();
    }
    else {
        fputs(qPrintable(message), stderr);
        fflush(stderr);
    }
}

///
/// \brief The PaletteGuard class
///
class PaletteGuard : public QObject {
public:
    PaletteGuard(QObject* parent) : QObject(parent) { }
    bool eventFilter(QObject *obj, QEvent *ev) override {
        if (ev->type() == QEvent::ApplicationPaletteChange) {
            QTimer::singleShot(0, [](){
                QApplication::setPalette(QApplication::style()->standardPalette());
            });
        }
        return QObject::eventFilter(obj, ev);
    }
};


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

#ifdef Q_OS_WIN
    a.setStyle("windowsvista");
#else
    a.setStyle("Fusion");
    qApp->installEventFilter(new PaletteGuard(qApp));
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
