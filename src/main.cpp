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
/// \brief The PaletteGuard class
///
class PaletteGuard : public QObject {
public:
    PaletteGuard(QObject* parent) : QObject(parent) { }
    bool eventFilter(QObject *obj, QEvent *ev) override {
        if (ev->type() == QEvent::ApplicationPaletteChange) {
            QTimer::singleShot(0, [this](){
                QApplication::setPalette(lightPalette());
            });
        }
        return QObject::eventFilter(obj, ev);
    }

    static QPalette lightPalette() {
        const QColor backGround(239, 239, 239);
        const QColor light = backGround.lighter(150);
        const QColor mid(backGround.darker(130));
        const QColor midLight = mid.lighter(110);
        const QColor base = Qt::white;
        const QColor disabledBase(backGround);
        const QColor dark = backGround.darker(150);
        const QColor darkDisabled = QColor(209, 209, 209).darker(110);
        const QColor text = Qt::black;
        const QColor hightlightedText = Qt::white;
        const QColor disabledText = QColor(190, 190, 190);
        const QColor button = backGround;
        const QColor shadow = dark.darker(135);
        const QColor disabledShadow = shadow.lighter(150);
        QPalette pal(Qt::black, backGround, light, dark, mid, text, base);
        pal.setBrush(QPalette::Midlight, midLight);
        pal.setBrush(QPalette::Button, button);
        pal.setBrush(QPalette::Shadow, shadow);
        pal.setBrush(QPalette::HighlightedText, hightlightedText);
        pal.setBrush(QPalette::Disabled, QPalette::Text, disabledText);
        pal.setBrush(QPalette::Disabled, QPalette::WindowText, disabledText);
        pal.setBrush(QPalette::Disabled, QPalette::ButtonText, disabledText);
        pal.setBrush(QPalette::Disabled, QPalette::Base, disabledBase);
        pal.setBrush(QPalette::Disabled, QPalette::Dark, darkDisabled);
        pal.setBrush(QPalette::Disabled, QPalette::Shadow, disabledShadow);
        pal.setBrush(QPalette::Active, QPalette::Highlight, QColor(48, 140, 198));
        pal.setBrush(QPalette::Inactive, QPalette::Highlight, QColor(48, 140, 198));
        pal.setBrush(QPalette::Disabled, QPalette::Highlight, QColor(145, 145, 145));
        return pal;
    }
};

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
    a.setPalette(PaletteGuard::lightPalette());
    a.installEventFilter(new PaletteGuard(&a));
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

    QString cfg;
    if(parser.isSet(CmdLineParser::_config))
    {
        cfg = parser.value(CmdLineParser::_config);
    }

    const bool noSession = parser.isSet(CmdLineParser::_no_session);

    MainWindow w(profile, !noSession);
    if(!cfg.isEmpty()) {
        w.loadConfig(cfg, true);
    }
    w.show();

    return a.exec();
}
