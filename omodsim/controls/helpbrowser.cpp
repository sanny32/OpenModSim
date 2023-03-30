#include "helpbrowser.h"

///
/// \brief HelpBrowser::HelpBrowser
/// \param helpEngine
/// \param parent
///
HelpBrowser::HelpBrowser(QHelpEngine* helpEngine, QWidget* parent)
    : QTextBrowser(parent)
    ,_helpEngine(helpEngine)
{
}

///
/// \brief HelpBrowser::loadResource
/// \param type
/// \param name
/// \return
///
QVariant HelpBrowser::loadResource (int type, const QUrl& name)
{
    if (name.scheme() == "qthelp")
        return QVariant(_helpEngine->fileData(name));
    else
        return QTextBrowser::loadResource(type, name);
}
