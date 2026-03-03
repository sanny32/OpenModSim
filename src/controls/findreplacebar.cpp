#include <QStyle>
#include <QPainter>
#include "findreplacebar.h"

///
/// \brief FindReplaceBar::FindReplaceBar
/// \param parent
///
FindReplaceBar::FindReplaceBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(420);
    setAutoFillBackground(true);

    auto pal = palette();
    pal.setColor(QPalette::Window, QColor(240, 240, 240));
    setPalette(pal);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 4, 6, 4);
    mainLayout->setSpacing(2);

    // Find row
    auto findRow = new QHBoxLayout;
    findRow->setSpacing(2);

    _searchEdit = new QLineEdit(this);
    _searchEdit->setPlaceholderText(tr("Find"));
    _searchEdit->setClearButtonEnabled(true);
    _searchEdit->installEventFilter(this);

    _prevButton = new QPushButton(this);
    _prevButton->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    _prevButton->setToolTip(tr("Previous (Shift+Enter)"));
    _prevButton->setFixedSize(24, 24);
    _prevButton->setFlat(true);

    _nextButton = new QPushButton(this);
    _nextButton->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    _nextButton->setToolTip(tr("Next (Enter)"));
    _nextButton->setFixedSize(24, 24);
    _nextButton->setFlat(true);

    _matchCountLabel = new QLabel(this);
    _matchCountLabel->setMinimumWidth(60);
    _matchCountLabel->setAlignment(Qt::AlignCenter);

    _closeButton = new QToolButton(this);
    _closeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    _closeButton->setToolTip(tr("Close (Escape)"));
    _closeButton->setAutoRaise(true);
    _closeButton->setFixedSize(24, 24);

    findRow->addWidget(_searchEdit, 1);
    findRow->addWidget(_prevButton);
    findRow->addWidget(_nextButton);
    findRow->addWidget(_matchCountLabel);
    findRow->addWidget(_closeButton);

    // Replace row
    _replaceRow = new QWidget(this);
    auto replaceLayout = new QHBoxLayout(_replaceRow);
    replaceLayout->setContentsMargins(0, 0, 0, 0);
    replaceLayout->setSpacing(2);

    _replaceEdit = new QLineEdit(_replaceRow);
    _replaceEdit->setPlaceholderText(tr("Replace"));
    _replaceEdit->setClearButtonEnabled(true);
    _replaceEdit->installEventFilter(this);

    _replaceButton = new QPushButton(tr("Replace"), _replaceRow);
    _replaceButton->setFixedHeight(24);

    _replaceAllButton = new QPushButton(tr("Replace All"), _replaceRow);
    _replaceAllButton->setFixedHeight(24);

    replaceLayout->addWidget(_replaceEdit, 1);
    replaceLayout->addWidget(_replaceButton);
    replaceLayout->addWidget(_replaceAllButton);

    mainLayout->addLayout(findRow);
    mainLayout->addWidget(_replaceRow);

    // Connections
    connect(_searchEdit, &QLineEdit::textEdited, this, &FindReplaceBar::onSearchTextEdited);
    connect(_prevButton, &QPushButton::clicked, this, &FindReplaceBar::onFindPrevious);
    connect(_nextButton, &QPushButton::clicked, this, &FindReplaceBar::onFindNext);
    connect(_replaceButton, &QPushButton::clicked, this, &FindReplaceBar::onReplace);
    connect(_replaceAllButton, &QPushButton::clicked, this, &FindReplaceBar::onReplaceAll);
    connect(_closeButton, &QToolButton::clicked, this, &FindReplaceBar::onClose);

    setVisible(false);
}

///
/// \brief FindReplaceBar::searchText
/// \return
///
QString FindReplaceBar::searchText() const
{
    return _searchEdit->text();
}

///
/// \brief FindReplaceBar::replaceText
/// \return
///
QString FindReplaceBar::replaceText() const
{
    return _replaceEdit->text();
}

///
/// \brief FindReplaceBar::updateMatchCount
/// \param current
/// \param total
///
void FindReplaceBar::updateMatchCount(int current, int total)
{
    if(total == 0)
        _matchCountLabel->setText(_searchEdit->text().isEmpty() ? QString() : tr("No results"));
    else
        _matchCountLabel->setText(tr("%1 of %2").arg(current).arg(total));
}

///
/// \brief FindReplaceBar::updatePosition
///
void FindReplaceBar::updatePosition()
{
    if(!parentWidget())
        return;

    const int x = parentWidget()->width() - width() - 16;
    move(qMax(0, x), 0);
}

///
/// \brief FindReplaceBar::showFind
/// \param selectedText
///
void FindReplaceBar::showFind(const QString& selectedText)
{
    _replaceRow->setVisible(false);

    if(!selectedText.isEmpty())
        _searchEdit->setText(selectedText);

    setVisible(true);
    adjustSize();
    updatePosition();
    raise();

    _searchEdit->setFocus();
    _searchEdit->selectAll();

    if(!_searchEdit->text().isEmpty())
        emit searchTextChanged(_searchEdit->text());
}

///
/// \brief FindReplaceBar::showReplace
/// \param selectedText
///
void FindReplaceBar::showReplace(const QString& selectedText)
{
    _replaceRow->setVisible(true);

    if(!selectedText.isEmpty())
        _searchEdit->setText(selectedText);

    setVisible(true);
    adjustSize();
    updatePosition();
    raise();

    _searchEdit->setFocus();
    _searchEdit->selectAll();

    if(!_searchEdit->text().isEmpty())
        emit searchTextChanged(_searchEdit->text());
}

///
/// \brief FindReplaceBar::eventFilter
/// \param obj
/// \param event
/// \return
///
bool FindReplaceBar::eventFilter(QObject* obj, QEvent* event)
{
    if((obj == _searchEdit || obj == _replaceEdit) && event->type() == QEvent::KeyPress)
    {
        auto keyEvent = static_cast<QKeyEvent*>(event);
        if(keyEvent->key() == Qt::Key_Escape)
        {
            onClose();
            return true;
        }
        if(obj == _searchEdit && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter))
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
/// \brief FindReplaceBar::onFindNext
///
void FindReplaceBar::onFindNext()
{
    const auto text = _searchEdit->text();
    if(!text.isEmpty())
        emit findNext(text);
}

///
/// \brief FindReplaceBar::onFindPrevious
///
void FindReplaceBar::onFindPrevious()
{
    const auto text = _searchEdit->text();
    if(!text.isEmpty())
        emit findPrevious(text);
}

///
/// \brief FindReplaceBar::onReplace
///
void FindReplaceBar::onReplace()
{
    const auto text = _searchEdit->text();
    if(!text.isEmpty())
        emit replaceRequested(text, _replaceEdit->text());
}

///
/// \brief FindReplaceBar::onReplaceAll
///
void FindReplaceBar::onReplaceAll()
{
    const auto text = _searchEdit->text();
    if(!text.isEmpty())
        emit replaceAllRequested(text, _replaceEdit->text());
}

///
/// \brief FindReplaceBar::onSearchTextEdited
/// \param text
///
void FindReplaceBar::onSearchTextEdited(const QString& text)
{
    emit searchTextChanged(text);
}

///
/// \brief FindReplaceBar::onClose
///
void FindReplaceBar::onClose()
{
    setVisible(false);
    emit closed();
}
