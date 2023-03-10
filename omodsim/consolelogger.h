#ifndef CONSOLELOGGER_H
#define CONSOLELOGGER_H

#include <QPlainTextEdit>
#include <QLoggingCategory>

///
/// \brief The ConsoleLogger class
///
class ConsoleLogger final
{
public:
    ConsoleLogger(QPlainTextEdit* edit);
    ~ConsoleLogger() = default;

    void clear();

private:
    ConsoleLogger(ConsoleLogger const&) = delete;
    void operator=(ConsoleLogger const&) = delete;

private:
    static QPlainTextEdit* _edit;
    static QtMessageHandler _defaultHandler;
    static void msgHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg);
};
Q_DECLARE_LOGGING_CATEGORY(js)


#endif // CONSOLELOGGER_H
