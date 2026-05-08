#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QToolBar>

class QActionEvent;
class QChildEvent;
class QPaintEvent;
class QShowEvent;

class ToolBar : public QToolBar
{
    Q_OBJECT
public:
    explicit ToolBar(QWidget* parent = nullptr);

protected:
    void actionEvent(QActionEvent* event) override;
    void childEvent(QChildEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void applyPlatformFont();
};

#endif // TOOLBAR_H
