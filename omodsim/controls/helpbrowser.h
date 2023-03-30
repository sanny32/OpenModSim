
#ifndef HELPBROWSER_H
#define HELPBROWSER_H

#include <QHelpEngine>
#include <QTextBrowser>


///
/// \brief The HelpBrowser class
///
class HelpBrowser : public QTextBrowser
{
    Q_OBJECT

public:
    explicit HelpBrowser(QHelpEngine* helpEngine, QWidget* parent = 0);
    QVariant loadResource (int type, const QUrl& name) override;

private:
    QHelpEngine* _helpEngine;
};

#endif // HELPBROWSER_H
