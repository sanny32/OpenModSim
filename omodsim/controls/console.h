#ifndef CONSOLE_H
#define CONSOLE_H

#include <QPlainTextEdit>

///
/// \brief The Console class
///
class Console : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit Console(QWidget *parent = nullptr);
    void setBackgroundColor(const QColor& clr);

private slots:
    void on_textChanged();
};

#endif // CONSOLE_H
