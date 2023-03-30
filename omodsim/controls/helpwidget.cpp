#include <QVBoxLayout>
#include <QHelpContentWidget>
#include "helpbrowser.h"
#include "helpwidget.h"

///
/// \brief HelpWidget::HelpWidget
/// \param parent
///
HelpWidget::HelpWidget(QWidget *parent)
    : QStackedWidget(parent)
    ,_helpView(nullptr)
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
/// \brief HelpWidget::setHelp
/// \param helpFile
///
void HelpWidget::setHelp(const QString& helpFile)
{
    if(_helpEngine)
    {
        removeWidget(_helpView.get());
        removeWidget(_helpEngine->contentWidget());
    }

    _helpEngine = QSharedPointer<QHelpEngine>(new QHelpEngine(helpFile, this));
    _helpEngine->setupData();

    _helpView = QSharedPointer<HelpBrowser>(new HelpBrowser(_helpEngine.get(), this));

    addWidget(_helpEngine->contentWidget());
    addWidget(_helpView.get());

    connect(_helpEngine->contentWidget(), &QHelpContentWidget::linkActivated, this,
            [this](const QUrl& link)
            {
                _helpView->setSource(link);
                setCurrentIndex(1);
            });
}
