#include <QMenu>
#include "consoleoutput.h"

///
/// \brief ConsoleOutput::ConsoleOutput
/// \param parent
///
ConsoleOutput::ConsoleOutput(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setReadOnly(true);
    setUndoRedoEnabled(false);
}

///
/// \brief ConsoleOutput::contextMenuEvent
/// \param event
///
void ConsoleOutput::contextMenuEvent(QContextMenuEvent* event)
{
    auto menu = new QMenu(this);
    auto action = menu->addAction(tr("Clear"), this, [this]()
    {
        setPlainText(QString());
    });
    action->setEnabled(!toPlainText().isEmpty());
    menu->exec(event->globalPos());
    delete menu;
}
