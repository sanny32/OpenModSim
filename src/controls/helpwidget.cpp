#include <QEvent>
#include <QToolButton>
#include <QHelpContentWidget>
#include "helpwidget.h"

///
/// \brief HelpWidget::HelpWidget
/// \param parent
///
HelpWidget::HelpWidget(QWidget *parent)
    : QWidget(parent)
    ,_helpBrowser(new HelpBrowser(this))
{
    auto header = new QWidget(this);

    auto headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(2, 2, 4, 2);

    auto collapseButton = createToolButton(header, "✕");

    _findLabel = new QLabel(tr("Find:"), header);
    _findLabel->setAlignment(Qt::AlignVCenter);

    auto searchEdit = new QLineEdit(header);
    searchEdit->setClearButtonEnabled(true);

    _findPrevButton = createToolButton(header, tr("Find Previous"), QIcon(), QString(), QSize());
    _findNextButton = createToolButton(header, tr("Find Next"), QIcon(), QString(), QSize());

    headerLayout->addWidget(_findLabel);
    headerLayout->addWidget(searchEdit, 1);
    headerLayout->addWidget(_findPrevButton);
    headerLayout->addWidget(_findNextButton);
    headerLayout->addSpacerItem(new QSpacerItem(10, 0, QSizePolicy::Fixed));
    headerLayout->addWidget(collapseButton);

    auto pal = _helpBrowser->palette();
    pal.setColor(QPalette::Base, Qt::white);
    pal.setColor(QPalette::Window, Qt::white);
    _helpBrowser->setPalette(pal);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(header);
    mainLayout->addWidget(_helpBrowser);

    header->setFixedHeight(header->sizeHint().height());

    connect(collapseButton, &QToolButton::clicked, this, &HelpWidget::collapse);

    connect(searchEdit, &QLineEdit::returnPressed, this, [=] {
        _helpBrowser->find(searchEdit->text());
    });

    connect(_findNextButton, &QToolButton::clicked, this, [=] {
        _helpBrowser->find(searchEdit->text());
    });

    connect(_findPrevButton, &QToolButton::clicked, this, [=] {
        _helpBrowser->find(searchEdit->text(),
                           QTextDocument::FindBackward);
    });
}

///
/// \brief HelpWidget::changeEvent
/// \param event
///
void HelpWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        _findLabel->setText(tr("Find:"));
        _findPrevButton->setText(tr("Find Previous"));
        _findNextButton->setText(tr("Find Next"));
    }
}

///
/// \brief HelpWidget::setHelp
/// \param helpFile
///
void HelpWidget::setHelp(const QString& helpFile)
{
    _helpBrowser->setHelp(helpFile);
}

///
/// \brief HelpWidget::showHelp
/// \param helpKey
///
void HelpWidget::showHelp(const QString& helpKey)
{
    _helpBrowser->showHelp(helpKey);
}

///
/// \brief HelpWidget::createToolButton
/// \param parent
/// \param text
/// \param icon
/// \param toolTip
/// \param size
/// \param iconSize
/// \return
///
QToolButton* HelpWidget::createToolButton(QWidget* parent, const QString& text, const QIcon& icon, const QString& toolTip, const QSize& size, const QSize& iconSize)
{
    auto btn = new QToolButton(parent);
    btn->setText(text);
    btn->setIcon(icon);

    if(!iconSize.isEmpty())
        btn->setIconSize(iconSize);

    if(!size.isEmpty())
        btn->setFixedSize(size);

    btn->setToolTip(toolTip);
    btn->setAutoRaise(true);

    if(text.isEmpty() && !icon.isNull()) {
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
    else if(!text.isEmpty() && !icon.isNull()) {
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }
    else if(!text.isEmpty() && icon.isNull()) {
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    }

    return btn;
}
