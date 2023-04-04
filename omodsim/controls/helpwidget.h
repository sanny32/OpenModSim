#ifndef HELPWIDGET_H
#define HELPWIDGET_H

#include <QHelpEngine>
#include <QTextBrowser>

///
/// \brief The HelpWidget class
///
class HelpWidget : public QTextBrowser
{
    Q_OBJECT

public:
    explicit HelpWidget(QWidget *parent = nullptr);
    ~HelpWidget();

    void setHelp(const QString& helpFile);
    QVariant loadResource (int type, const QUrl& name) override;

    void showHelp(const QString& helpKey);

private:
    QSharedPointer<QHelpEngine> _helpEngine;
};

#endif // HELPWIDGET_H
