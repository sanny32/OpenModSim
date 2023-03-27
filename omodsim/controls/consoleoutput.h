#ifndef CONSOLEOUTPUT_H
#define CONSOLEOUTPUT_H

#include <QPlainTextEdit>

///
/// \brief The ConsoleOutput class
///
class ConsoleOutput : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit ConsoleOutput(QWidget* parent = nullptr);

protected:
    void contextMenuEvent(QContextMenuEvent* event);
};

#endif // CONSOLEOUTPUT_H
