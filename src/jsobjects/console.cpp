#include "console.h"

///
/// \brief console::console
/// \param parent
///
console::console(QObject* parent)
    : QObject(parent)
{
}

///
/// \brief console::clear
///
void console::clear()
{
    emit clearRequested();
}

///
/// \brief console::log
/// \param msg
///
void console::log(const QString& msg)
{
    emit messageAdded(msg, ConsoleOutput::MessageType::Log);
}

///
/// \brief console::debug
/// \param msg
///
void console::debug(const QString& msg)
{
    emit messageAdded(msg, ConsoleOutput::MessageType::Debug);
}

///
/// \brief console::warning
/// \param msg
///
void console::warning(const QString& msg)
{
    emit messageAdded(msg, ConsoleOutput::MessageType::Warning);
}

///
/// \brief console::error
/// \param msg
///
void console::error(const QString& msg)
{
    emit messageAdded(msg, ConsoleOutput::MessageType::Error);
}
