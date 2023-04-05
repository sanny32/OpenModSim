#include <QEvent>
#include <QHelpContentWidget>
#include "helpwidget.h"

///
/// \brief HelpWidget::HelpWidget
/// \param parent
///
HelpWidget::HelpWidget(QWidget *parent)
    : QTextBrowser(parent)
    ,_helpEngine(nullptr)
{
}

///
/// \brief HelpWidget::~HelpWidget
///
HelpWidget::~HelpWidget()
{
}

///
/// \brief SearchLineEdit::changeEvent
/// \param event
///
void HelpWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        setSource(QUrl(tr("qthelp://omodsim/doc/index.html")));
    }

    QTextBrowser::changeEvent(event);
}

///
/// \brief HelpWidget::loadResource
/// \param type
/// \param name
/// \return
///
QVariant HelpWidget::loadResource (int type, const QUrl& name)
{
    if (name.scheme() == "qthelp" && _helpEngine)
        return QVariant(_helpEngine->fileData(name));
    else
        return QTextBrowser::loadResource(type, name);
}

///
/// \brief HelpWidget::setHelp
/// \param helpFile
///
void HelpWidget::setHelp(const QString& helpFile)
{
    _helpEngine = QSharedPointer<QHelpEngine>(new QHelpEngine(helpFile, this));
    _helpEngine->setupData();

    setSource(QUrl(tr("qthelp://omodsim/doc/index.html")));
}

///
/// \brief HelpWidget::showHelp
/// \param helpKey
///
void HelpWidget::showHelp(const QString& helpKey)
{
    const auto url = QString(tr("qthelp://omodsim/doc/index.html#%1")).arg(helpKey.toLower());
    setSource(QUrl(url));
}
