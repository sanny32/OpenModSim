#include <QEvent>
#include <QStyle>
#include <QKeyEvent>
#include <QToolButton>
#include <QPushButton>
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
    auto pal = _helpBrowser->palette();
    pal.setColor(QPalette::Base, Qt::white);
    pal.setColor(QPalette::Window, Qt::white);
    _helpBrowser->setPalette(pal);

    // Find bar (bottom, styled like FindReplaceBar)
    _findBar = new QWidget(this);
    _findBar->setAutoFillBackground(true);
    auto findPal = _findBar->palette();
    findPal.setColor(QPalette::Window, QColor(240, 240, 240));
    _findBar->setPalette(findPal);
    _findBar->setVisible(false);

    auto findLayout = new QHBoxLayout(_findBar);
    findLayout->setContentsMargins(6, 4, 6, 4);
    findLayout->setSpacing(2);

    _searchEdit = new QLineEdit(_findBar);
    _searchEdit->setPlaceholderText(tr("Find"));
    _searchEdit->setClearButtonEnabled(true);
    _searchEdit->installEventFilter(this);

    _prevButton = new QPushButton(_findBar);
    _prevButton->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    _prevButton->setToolTip(tr("Previous (Shift+Enter)"));
    _prevButton->setFixedSize(24, 24);
    _prevButton->setFlat(true);

    _nextButton = new QPushButton(_findBar);
    _nextButton->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    _nextButton->setToolTip(tr("Next (Enter)"));
    _nextButton->setFixedSize(24, 24);
    _nextButton->setFlat(true);

    _matchCountLabel = new QLabel(_findBar);
    _matchCountLabel->setMinimumWidth(60);
    _matchCountLabel->setAlignment(Qt::AlignCenter);

    _closeButton = new QToolButton(_findBar);
    _closeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    _closeButton->setToolTip(tr("Close (Escape)"));
    _closeButton->setAutoRaise(true);
    _closeButton->setFixedSize(24, 24);

    findLayout->addWidget(_searchEdit, 1);
    findLayout->addWidget(_prevButton);
    findLayout->addWidget(_nextButton);
    findLayout->addWidget(_matchCountLabel);
    findLayout->addWidget(_closeButton);

    // Main layout: browser on top, find bar on bottom
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(_helpBrowser);
    mainLayout->addWidget(_findBar);

    // Connections
    connect(_searchEdit, &QLineEdit::textEdited, this, &HelpWidget::onSearchTextEdited);
    connect(_prevButton, &QPushButton::clicked, this, &HelpWidget::onFindPrevious);
    connect(_nextButton, &QPushButton::clicked, this, &HelpWidget::onFindNext);
    connect(_closeButton, &QToolButton::clicked, this, &HelpWidget::onClose);
}

///
/// \brief HelpWidget::changeEvent
/// \param event
///
void HelpWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        _searchEdit->setPlaceholderText(tr("Find"));
        _prevButton->setToolTip(tr("Previous (Shift+Enter)"));
        _nextButton->setToolTip(tr("Next (Enter)"));
        _closeButton->setToolTip(tr("Close (Escape)"));
    }
}

///
/// \brief HelpWidget::keyPressEvent
/// \param event
///
void HelpWidget::keyPressEvent(QKeyEvent* event)
{
    if(event->matches(QKeySequence::Find))
    {
        showFind();
        return;
    }
    QWidget::keyPressEvent(event);
}

///
/// \brief HelpWidget::eventFilter
/// \param obj
/// \param event
/// \return
///
bool HelpWidget::eventFilter(QObject* obj, QEvent* event)
{
    if(obj == _searchEdit && event->type() == QEvent::KeyPress)
    {
        auto keyEvent = static_cast<QKeyEvent*>(event);
        if(keyEvent->key() == Qt::Key_Escape)
        {
            onClose();
            return true;
        }
        if(keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
        {
            if(keyEvent->modifiers() & Qt::ShiftModifier)
                onFindPrevious();
            else
                onFindNext();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

///
/// \brief HelpWidget::onFindNext
///
void HelpWidget::onFindNext()
{
    const auto text = _searchEdit->text();
    if(!text.isEmpty())
        _helpBrowser->find(text);
}

///
/// \brief HelpWidget::onFindPrevious
///
void HelpWidget::onFindPrevious()
{
    const auto text = _searchEdit->text();
    if(!text.isEmpty())
        _helpBrowser->find(text, QTextDocument::FindBackward);
}

///
/// \brief HelpWidget::onSearchTextEdited
/// \param text
///
void HelpWidget::onSearchTextEdited(const QString& text)
{
    if(!text.isEmpty())
    {
        auto cursor = _helpBrowser->textCursor();
        cursor.movePosition(QTextCursor::Start);
        _helpBrowser->setTextCursor(cursor);
        _helpBrowser->find(text);
    }
}

///
/// \brief HelpWidget::onClose
///
void HelpWidget::onClose()
{
    _findBar->setVisible(false);
    _helpBrowser->setFocus();
}

///
/// \brief HelpWidget::showFind
///
void HelpWidget::showFind()
{
    _findBar->setVisible(true);
    _searchEdit->setFocus();
    _searchEdit->selectAll();
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
