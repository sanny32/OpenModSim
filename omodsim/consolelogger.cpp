#include "consolelogger.h"

Q_LOGGING_CATEGORY(js, "js")

QPlainTextEdit* ConsoleLogger::_edit = nullptr;
QtMessageHandler ConsoleLogger::_defaultHandler;

///
/// \brief ConsoleLogger::ConsoleLogger
/// \param edit
///
ConsoleLogger::ConsoleLogger(QPlainTextEdit* edit)
{
    Q_ASSERT(edit != nullptr);

    edit->setFont(QFont("Fira Code"));
    edit->setTabStopDistance(edit->fontMetrics().horizontalAdvance(' ') * 2);

    auto pal = edit->palette();
    pal.setColor(QPalette::Base, Qt::white);
    pal.setColor(QPalette::Window, Qt::white);
    edit->setPalette(pal);

    ConsoleLogger::_edit = edit;

    _defaultHandler = qInstallMessageHandler(ConsoleLogger::msgHandler);
}

///
/// \brief ConsoleLogger::clear
///
void ConsoleLogger::clear()
{
    if(ConsoleLogger::_edit)
        ConsoleLogger::_edit->setPlainText(QString());
}

///
/// \brief ConsoleLogger::msgHandler
/// \param type
/// \param ctx
/// \param msg
///
void ConsoleLogger::msgHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
    const QString category = ctx.category;
    if(category != "js" || !_edit){
        _defaultHandler(type, ctx, msg);
        return;
    }

    QTextCharFormat fmt;
    switch (type)
    {
        case QtDebugMsg:
        case QtInfoMsg:
            fmt.setForeground(Qt::black);
        break;

        case QtWarningMsg:
            fmt.setForeground(Qt::yellow);
        break;

        case QtCriticalMsg:
            fmt.setForeground(Qt::red);
        break;

        case QtFatalMsg:
            fmt.setForeground(Qt::darkRed);
        break;
    }

    _edit->mergeCurrentCharFormat(fmt);
    _edit->insertPlainText(QString(_edit->toPlainText().isEmpty() ? ">>\t%1" : "\n>>\t%1").arg(msg));
    _edit->moveCursor(QTextCursor::End);
}
