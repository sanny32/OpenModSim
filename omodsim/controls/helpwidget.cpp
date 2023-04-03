#include <QVBoxLayout>
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
/// \brief HelpWidget::loadResource
/// \param type
/// \param name
/// \return
///
QVariant HelpWidget::loadResource (int type, const QUrl& name)
{
    qDebug() << name;
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

    setSource(QUrl("qthelp://omodsim/doc/index.html"));
}
