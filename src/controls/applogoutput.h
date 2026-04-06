#ifndef APPLOGOUTPUT_H
#define APPLOGOUTPUT_H

#include <QLoggingCategory>
#include <QWidget>

Q_DECLARE_LOGGING_CATEGORY(lcApp)

namespace Ui {
class AppLogOutput;
}

///
/// \brief The AppLogOutput class — application event log tab
///
class AppLogOutput : public QWidget
{
    Q_OBJECT
public:
    enum class EventType { Info, Warning, Error };

    explicit AppLogOutput(QWidget* parent = nullptr);
    ~AppLogOutput();

    void addEvent(const QString& text, EventType type);
    bool isEmpty() const;
    void setMaxLines(int n);

public slots:
    void clear();

signals:
    void collapse();

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_customContextMenuRequested(const QPoint& pos);
    void applyFilters();
    void exportLog();
    void copyAllToClipboard();

private:
    void updateFilterButtons();

private:
    Ui::AppLogOutput* ui;
    QAction* _copyAllAction = nullptr;
    int _infoCount  = 0;
    int _warnCount  = 0;
    int _errorCount = 0;
    int _maxLines   = 500;
};

#endif // APPLOGOUTPUT_H

