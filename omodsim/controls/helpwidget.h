#ifndef HELPWIDGET_H
#define HELPWIDGET_H

#include <QHelpEngine>
#include "helpbrowser.h"

///
/// \brief The HelpWidget class
///
class HelpWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HelpWidget(QWidget* parent = nullptr);

    void setHelp(const QString& helpFile);
    void showHelp(const QString& helpKey);

signals:
    void collapse();

private:
    QToolButton* createToolButton(QWidget* parent,
                                  const QString& text,
                                  const QIcon& icon = QIcon(),
                                  const QString& toolTip = QString(),
                                  const QSize& size = {24, 24},
                                  const QSize& iconSize = {12, 12});

private:
    HelpBrowser*    _helpBrowser;
};

#endif // HELPWIDGET_H
