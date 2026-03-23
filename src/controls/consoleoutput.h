#ifndef CONSOLEOUTPUT_H
#define CONSOLEOUTPUT_H

#include <QWidget>

namespace Ui {
class ConsoleOutput;
}

///
/// \brief The ConsoleOutput class
///
class ConsoleOutput : public QWidget
{
    Q_OBJECT
public:
    enum class MessageType { Log, Debug, Warning, Error };

    explicit ConsoleOutput(QWidget* parent = nullptr);
    ~ConsoleOutput();

    void addMessage(const QString& text, MessageType type, const QString& source = {});
    bool isEmpty() const;

public slots:
    void clear();

signals:
    void collapse();

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_customContextMenuRequested(const QPoint& pos);
    void applyFilters();

private:
    void updateFilterButtons();

private:
    Ui::ConsoleOutput* ui;
    int _logCount   = 0;
    int _warnCount  = 0;
    int _errorCount = 0;
};

#endif // CONSOLEOUTPUT_H
