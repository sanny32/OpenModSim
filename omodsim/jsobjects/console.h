#ifndef CONSOLE_H
#define CONSOLE_H

#include <QObject>
#include <QPlainTextEdit>

///
/// \brief The console class
///
class console : public QObject
{
    Q_OBJECT
public:
    explicit console(QPlainTextEdit* edit);

    Q_INVOKABLE void clear();
    Q_INVOKABLE void log(const QString& msg);
    Q_INVOKABLE void debug(const QString& msg);
    Q_INVOKABLE void warning(const QString& msg);
    Q_INVOKABLE void error(const QString& msg);

private:
    void addText(const QString& text);
    void setBackgroundColor(const QColor& clr);

private:
    QPlainTextEdit* _edit;
};

#endif // CONSOLE_H
