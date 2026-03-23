#include <QStyle>
#include <QPainter>
#include <QMouseEvent>
#include <QSignalBlocker>
#include "findreplacebar.h"
#include "ui_findreplacebar.h"

///
/// \brief The LeftSizeGrip class
///
class LeftSizeGrip : public QWidget
{
public:
    explicit LeftSizeGrip(QWidget* target, QWidget* parent)
        : QWidget(parent), _target(target)
    {
        setCursor(Qt::SizeHorCursor);
        setFixedSize(16, 16);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        // Dots forming a triangle pointing toward the bottom-left corner
        static const struct { int x, y; } dots[] = {
            { 2,  4},
            { 2,  8}, { 6,  8},
            { 2, 12}, { 6, 12}, {10, 12},
        };
        QPainter p(this);
        const QColor shadow = palette().color(QPalette::Mid);
        const QColor light  = palette().color(QPalette::Light);
        for(const auto& d : dots) {
            p.fillRect(d.x + 1, d.y + 1, 2, 2, light);
            p.fillRect(d.x,     d.y,     2, 2, shadow);
        }
    }

    void mousePressEvent(QMouseEvent* e) override
    {
        if(e->button() == Qt::LeftButton) {
            _resizing          = true;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            _resizeGlobalStart = e->globalPosition().toPoint();
#else
            _resizeGlobalStart = e->globalPos();
#endif
            _resizeStartX      = _target->x();
            _resizeStartWidth  = _target->width();
            e->accept();
        }
    }

    void mouseMoveEvent(QMouseEvent* e) override
    {
        if(_resizing) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            const int dx   = e->globalPosition().toPoint().x() - _resizeGlobalStart.x();
#else
            const int dx   = e->globalPos().x() - _resizeGlobalStart.x();
#endif
            const int newW = qMax(_target->minimumWidth(), _resizeStartWidth - dx);
            _target->move(_resizeStartX + _resizeStartWidth - newW, _target->y());
            _target->resize(newW, _target->height());
            e->accept();
        }
    }

    void mouseReleaseEvent(QMouseEvent* e) override
    {
        if(e->button() == Qt::LeftButton) {
            _resizing = false;
            e->accept();
        }
    }

private:
    QWidget* _target;
    bool     _resizing         = false;
    QPoint   _resizeGlobalStart;
    int      _resizeStartX     = 0;
    int      _resizeStartWidth = 0;
};

///
/// \brief FindReplaceBar::FindReplaceBar
/// \param parent
///
FindReplaceBar::FindReplaceBar(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::FindReplaceBar)
{
    ui->setupUi(this);

    ui->searchEdit->installEventFilter(this);
    ui->replaceEdit->installEventFilter(this);

    const int iconSize = 12;
    const QIcon iconFindeNext = QIcon(":/res/icon-arrow-right.svg").pixmap(iconSize, iconSize);
    const QIcon iconFindPrev = QIcon(":/res/icon-arrow-left.svg").pixmap(iconSize, iconSize);

    auto actionFindNext = new QAction(iconFindeNext, tr("Find Next"), this);
    auto actionFindPrev = new QAction(iconFindPrev, tr("Find Previous"), this);

    actionFindNext->setShortcut(Qt::Key_F3);
    actionFindPrev->setShortcut(Qt::SHIFT | Qt::Key_F3);

    ui->findButton->addAction(actionFindNext);
    ui->findButton->addAction(actionFindPrev);
    ui->findButton->setDefaultAction(actionFindNext);

    ui->horizontalLayout->insertWidget(0, new LeftSizeGrip(this, this));

    connect(actionFindNext,      &QAction::triggered,    this,              &FindReplaceBar::onFindNext);
    connect(actionFindPrev,      &QAction::triggered,    this,              &FindReplaceBar::onFindPrevious);
    connect(ui->expandButton,    &QToolButton::clicked,  this,              &FindReplaceBar::onToggleReplace);
    connect(ui->searchEdit,      &QLineEdit::textEdited, this,              &FindReplaceBar::onSearchTextEdited);
    connect(ui->matchCaseButton, &QToolButton::toggled,  this,              &FindReplaceBar::onOptionsChanged);
    connect(ui->matchWordButton, &QToolButton::toggled,  this,              &FindReplaceBar::onOptionsChanged);
    connect(ui->replaceButton,   &QToolButton::clicked,  this,              &FindReplaceBar::onReplace);
    connect(ui->replaceAllButton,&QToolButton::clicked,  this,              &FindReplaceBar::onReplaceAll);
    connect(ui->closeButton,     &QToolButton::clicked,  this,              &FindReplaceBar::onClose);
    connect(ui->findButton,      &QToolButton::triggered, ui->findButton,   &QToolButton::setDefaultAction);

    setVisible(false);
}

///
/// \brief FindReplaceBar::~FindReplaceBar
///
FindReplaceBar::~FindReplaceBar()
{
    delete ui;
}

///
/// \brief FindReplaceBar::searchText
/// \return
///
QString FindReplaceBar::searchText() const
{
    return ui->searchEdit->text();
}

///
/// \brief FindReplaceBar::replaceText
/// \return
///
QString FindReplaceBar::replaceText() const
{
    return ui->replaceEdit->text();
}

///
/// \brief FindReplaceBar::findFlags
/// \return
///
QTextDocument::FindFlags FindReplaceBar::findFlags() const
{
    QTextDocument::FindFlags flags;
    if (!_searchOptionsVisible)
        return flags;

    if (ui->matchCaseButton->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (ui->matchWordButton->isChecked())
        flags |= QTextDocument::FindWholeWords;
    return flags;
}

///
/// \brief FindReplaceBar::updatePosition
///
void FindReplaceBar::updatePosition()
{
    if(!parentWidget())
        return;

    const int scrollBarWidth = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    const int x = parentWidget()->width() - width() - scrollBarWidth;
    const int y = parentWidget()->contentsRect().top();
    move(qMax(0, x), y);
}

///
/// \brief FindReplaceBar::setReplaceEnabled
/// \param enabled
///
void FindReplaceBar::setReplaceEnabled(bool enabled)
{
    if (_replaceEnabled == enabled)
        return;

    _replaceEnabled = enabled;
    ui->expandButton->setVisible(_replaceEnabled);
    if (!_replaceEnabled)
        setReplaceVisible(false);
}

///
/// \brief FindReplaceBar::isReplaceEnabled
/// \return
///
bool FindReplaceBar::isReplaceEnabled() const
{
    return _replaceEnabled;
}

///
/// \brief FindReplaceBar::setSearchOptionsVisible
/// \param visible
///
void FindReplaceBar::setSearchOptionsVisible(bool visible)
{
    if (_searchOptionsVisible == visible)
        return;

    _searchOptionsVisible = visible;
    ui->matchCaseButton->setVisible(_searchOptionsVisible);
    ui->matchWordButton->setVisible(_searchOptionsVisible);

    if (_searchOptionsVisible) {
        ui->horizontalSpacer->changeSize(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    } else {
        {
            QSignalBlocker caseBlocker(ui->matchCaseButton);
            QSignalBlocker wordBlocker(ui->matchWordButton);
            ui->matchCaseButton->setChecked(false);
            ui->matchWordButton->setChecked(false);
        }
    }

    ui->horizontalLayout->invalidate();
    layout()->activate();
    adjustSize();

    if(!ui->searchEdit->text().isEmpty())
        emit searchTextChanged(ui->searchEdit->text());
}

///
/// \brief FindReplaceBar::isSearchOptionsVisible
/// \return
///
bool FindReplaceBar::isSearchOptionsVisible() const
{
    return _searchOptionsVisible;
}

///
/// \brief FindReplaceBar::setReplaceVisible
/// \param visible
///
void FindReplaceBar::setReplaceVisible(bool visible)
{
    const bool showReplace = _replaceEnabled && visible;
    ui->replaceRow->setVisible(showReplace);
    ui->expandButton->setArrowType(showReplace ? Qt::DownArrow : Qt::RightArrow);
}

///
/// \brief FindReplaceBar::showFind
/// \param selectedText
///
void FindReplaceBar::showFind(const QString& selectedText)
{
    setReplaceVisible(false);

    if(!selectedText.isEmpty())
        ui->searchEdit->setText(selectedText);

    setVisible(true);
    adjustSize();
    updatePosition();
    raise();

    ui->searchEdit->setFocus();
    ui->searchEdit->selectAll();

    if(!ui->searchEdit->text().isEmpty())
        emit searchTextChanged(ui->searchEdit->text());
}

///
/// \brief FindReplaceBar::showReplace
/// \param selectedText
///
void FindReplaceBar::showReplace(const QString& selectedText)
{
    if (!_replaceEnabled) {
        showFind(selectedText);
        return;
    }

    setReplaceVisible(true);

    if(!selectedText.isEmpty())
        ui->searchEdit->setText(selectedText);

    setVisible(true);
    adjustSize();
    updatePosition();
    raise();

    ui->searchEdit->setFocus();
    ui->searchEdit->selectAll();

    if(!ui->searchEdit->text().isEmpty())
        emit searchTextChanged(ui->searchEdit->text());
}

///
/// \brief FindReplaceBar::eventFilter
/// \param obj
/// \param event
/// \return
///
bool FindReplaceBar::eventFilter(QObject* obj, QEvent* event)
{
    if((obj == ui->searchEdit || obj == ui->replaceEdit) && event->type() == QEvent::KeyPress)
    {
        auto keyEvent = static_cast<QKeyEvent*>(event);
        if(keyEvent->key() == Qt::Key_Escape)
        {
            onClose();
            return true;
        }
        if(obj == ui->searchEdit && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter))
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
    const auto text = ui->searchEdit->text();
    if(!text.isEmpty())
        emit findNext(text);
}

///
/// \brief FindReplaceBar::onFindPrevious
///
void FindReplaceBar::onFindPrevious()
{
    const auto text = ui->searchEdit->text();
    if(!text.isEmpty())
        emit findPrevious(text);
}

///
/// \brief FindReplaceBar::onReplace
///
void FindReplaceBar::onReplace()
{
    const auto text = ui->searchEdit->text();
    if(!text.isEmpty())
        emit replaceRequested(text, ui->replaceEdit->text());
}

///
/// \brief FindReplaceBar::onReplaceAll
///
void FindReplaceBar::onReplaceAll()
{
    const auto text = ui->searchEdit->text();
    if(!text.isEmpty())
        emit replaceAllRequested(text, ui->replaceEdit->text());
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
/// \brief FindReplaceBar::onOptionsChanged
///
void FindReplaceBar::onOptionsChanged()
{
    if(!ui->searchEdit->text().isEmpty())
        emit searchTextChanged(ui->searchEdit->text());
}

///
/// \brief FindReplaceBar::onToggleReplace
///
void FindReplaceBar::onToggleReplace()
{
    if (!_replaceEnabled)
        return;

    setReplaceVisible(!ui->replaceRow->isVisible());

    layout()->activate();
    resize(width(), sizeHint().height());

    updatePosition();
}

///
/// \brief FindReplaceBar::onClose
///
void FindReplaceBar::onClose()
{
    setVisible(false);
    emit closed();
}
