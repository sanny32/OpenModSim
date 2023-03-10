#include <QTextCharFormat>
#include "console.h"

///
/// \brief console::console
/// \param doc
///
console::console(QPlainTextEdit* edit)
    : _edit(edit)
{
    _edit->setFont(QFont("Fira Code"));
    _edit->setTabStopDistance(_edit->fontMetrics().horizontalAdvance(' ') * 2);
    setBackgroundColor(Qt::white);
}

///
/// \brief console::clear
///
void console::clear()
{
    _edit->setPlainText(QString());
}

///
/// \brief console::log
/// \param msg
///
void console::log(const QString& msg)
{
    QTextCharFormat fmt;
    fmt.setForeground(Qt::black);
    _edit->mergeCurrentCharFormat(fmt);

    addText(msg);
}

///
/// \brief console::debug
/// \param msg
///
void console::debug(const QString& msg)
{
    log(msg);
}

///
/// \brief console::warning
/// \param msg
///
void console::warning(const QString& msg)
{
    QTextCharFormat fmt;
    fmt.setForeground(Qt::yellow);
    _edit->mergeCurrentCharFormat(fmt);

    addText(msg);
}

///
/// \brief console::error
/// \param msg
///
void console::error(const QString& msg)
{
    QTextCharFormat fmt;
    fmt.setForeground(Qt::red);
    _edit->mergeCurrentCharFormat(fmt);

    addText(msg);
}

///
/// \brief console::addText
/// \param text
///
void console::addText(const QString& text)
{
    _edit->insertPlainText(QString(_edit->toPlainText().isEmpty() ? ">>\t%1" : "\n>>\t%1").arg(text));
    _edit->moveCursor(QTextCursor::End);
}

///
/// \brief console::setBackgroundColor
/// \param clr
///
void console::setBackgroundColor(const QColor& clr)
{
    auto pal = _edit->palette();
    pal.setColor(QPalette::Base, clr);
    pal.setColor(QPalette::Window, clr);
    _edit->setPalette(pal);
}

