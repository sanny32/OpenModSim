#ifndef CONSOLEOUTPUT_H
#define CONSOLEOUTPUT_H

#include <QWidget>

class QListWidget;
class QToolButton;

///
/// \brief The ConsoleOutput class
///
class ConsoleOutput : public QWidget
{
    Q_OBJECT
public:
    enum class MessageType { Log, Debug, Warning, Error };

    explicit ConsoleOutput(QWidget* parent = nullptr);

    void addMessage(const QString& text, MessageType type);
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
    QToolButton* createToolButton(QWidget* parent,
                                  const QString& text,
                                  const QIcon& icon = QIcon(),
                                  const QString& toolTip = QString(),
                                  const QSize& size = {24, 24},
                                  const QSize& iconSize = {12, 12});
    void updateFilterButtons();

private:
    QListWidget*  _listWidget;
    QToolButton*  _clearButton;
    QToolButton*  _filterLog;
    QToolButton*  _filterWarn;
    QToolButton*  _filterError;
    int           _logCount   = 0;
    int           _warnCount  = 0;
    int           _errorCount = 0;
};

#endif // CONSOLEOUTPUT_H
