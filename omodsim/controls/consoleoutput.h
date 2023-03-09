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
    explicit ConsoleOutput(QWidget *parent = nullptr);
    void setBackgroundColor(const QColor& clr);

private slots:
    void on_textChanged();
};

#endif // CONSOLEOUTPUT_H
