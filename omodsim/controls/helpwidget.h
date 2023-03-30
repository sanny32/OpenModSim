#ifndef HELPWIDGET_H
#define HELPWIDGET_H

#include <QHelpEngine>
#include <QStackedWidget>
#include "helpbrowser.h"

///
/// \brief The HelpWidget class
///
class HelpWidget : public QStackedWidget
{
    Q_OBJECT

public:
    explicit HelpWidget(QWidget *parent = nullptr);
    ~HelpWidget();

    void setHelp(const QString& helpFile);

private:
    QSharedPointer<HelpBrowser> _helpView;
    QSharedPointer<QHelpEngine> _helpEngine;
};

#endif // HELPWIDGET_H
