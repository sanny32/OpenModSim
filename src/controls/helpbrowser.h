#ifndef HELPBROWSER_H
#define HELPBROWSER_H

#include <QTextBrowser>

///
/// \brief The HelpBrowser class
///
class HelpBrowser : public QTextBrowser
{
    Q_OBJECT

public:
    explicit HelpBrowser(QWidget* parent = nullptr)
        : QTextBrowser(parent) {
    }

    void setHelp(const QString& helpFile) {
        _helpEngine = QSharedPointer<QHelpEngine>(new QHelpEngine(helpFile, this));
        _helpEngine->setupData();
        setSource(QUrl(tr("qthelp://omodsim/doc/index.html")));
    }

    QVariant loadResource (int type, const QUrl& name) override {
        if (name.scheme() == "qthelp" && _helpEngine)
            return QVariant(_helpEngine->fileData(name));
        else
            return loadResource(type, name);
    }

    void showHelp(const QString& helpKey){
        const auto url = QString(tr("qthelp://omodsim/doc/index.html#%1")).arg(helpKey.toLower());
        setSource(QUrl(url));
    }

protected:
    void changeEvent(QEvent* event) override {
        if (event->type() == QEvent::LanguageChange) {
            setSource(QUrl(tr("qthelp://omodsim/doc/index.html")));
        }
        QTextBrowser::changeEvent(event);
    }

private:
    QSharedPointer<QHelpEngine> _helpEngine;
};

#endif // HELPBROWSER_H
