#include <QTextCharFormat>
#include "console.h"

///
/// \brief Console::Console
/// \param doc
///
Console::Console(QPlainTextEdit* edit)
    : _edit(edit)
{
    _edit->setFont(QFont("Fira Code"));
    _edit->setTabStopDistance(_edit->fontMetrics().horizontalAdvance(' ') * 2);
    setBackgroundColor(Qt::white);
}

///
/// \brief Console::clear
///
void Console::clear()
{
    _edit->setPlainText(QString());
}

///
/// \brief Console::log
/// \param msg
///
void Console::log(const QString& msg)
{
    QTextCharFormat fmt;
    fmt.setForeground(Qt::black);
    _edit->mergeCurrentCharFormat(fmt);

    addText(msg);
}

///
/// \brief Console::error
/// \param msg
///
void Console::error(const QString& msg)
{
    QTextCharFormat fmt;
    fmt.setForeground(Qt::red);
    _edit->mergeCurrentCharFormat(fmt);

    addText(msg);
}

///
/// \brief Console::addText
/// \param text
///
void Console::addText(const QString& text)
{
    _edit->insertPlainText(QString(_edit->toPlainText().isEmpty() ? ">>\t%1" : "\n>>\t%1").arg(text));
    _edit->moveCursor(QTextCursor::End);
}

///
/// \brief Console::setBackgroundColor
/// \param clr
///
void Console::setBackgroundColor(const QColor& clr)
{
    auto pal = _edit->palette();
    pal.setColor(QPalette::Base, clr);
    pal.setColor(QPalette::Window, clr);
    _edit->setPalette(pal);
}

