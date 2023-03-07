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

public slots:
    Q_INVOKABLE void clear();
    Q_INVOKABLE void log(QString msg);
};

#endif // CONSOLE_H
