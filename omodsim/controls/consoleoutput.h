#ifndef CONSOLEOUTPUT_H
#define CONSOLEOUTPUT_H

#include <QPlainTextEdit>

///
/// \brief The ConsoleOutput class
///
class ConsoleOutput : public QWidget
{
    Q_OBJECT
public:
    explicit ConsoleOutput(QWidget* parent = nullptr);

    void addText(const QString& text, const QColor& clr);
    bool isEmpty() const;

public slots:
    void clear();

signals:
    void collapse();

private slots:
    void on_customContextMenuRequested(const QPoint &pos);

private:
    QToolButton* createToolButton(const QString& text, const QIcon& icon = QIcon(), const QString& toolTip = QString());

private:
    QWidget*        _header;
    QToolButton*    _clearButton;
    QToolButton*    _collapseButton;
    QPlainTextEdit* _textEdit;
};
#endif // CONSOLEOUTPUT_H
