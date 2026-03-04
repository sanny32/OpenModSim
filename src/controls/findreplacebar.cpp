#include <QStyle>
#include <QPainter>
#include <QMouseEvent>
#include <QVBoxLayout>
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
            _resizeGlobalStart = e->globalPosition().toPoint();
            _resizeStartX      = _target->x();
            _resizeStartWidth  = _target->width();
            e->accept();
        }
    }

    void mouseMoveEvent(QMouseEvent* e) override
    {
        if(_resizing) {
            const int dx   = e->globalPosition().toPoint().x() - _resizeGlobalStart.x();
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

    ui->prevButton->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    ui->nextButton->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));

    auto labelFont = ui->matchCountLabel->font();
    labelFont.setPointSize(qMax(7, labelFont.pointSize() - 1));
    ui->matchCountLabel->setFont(labelFont);

    ui->searchEdit->installEventFilter(this);
    ui->replaceEdit->installEventFilter(this);

    auto* grip = new LeftSizeGrip(this, this);
    qobject_cast<QVBoxLayout*>(layout())->addWidget(grip, 0, Qt::AlignBottom | Qt::AlignLeft);

    connect(ui->expandButton,    &QToolButton::clicked,  this, &FindReplaceBar::onToggleReplace);
    connect(ui->searchEdit,      &QLineEdit::textEdited, this, &FindReplaceBar::onSearchTextEdited);
    connect(ui->matchCaseButton, &QToolButton::toggled,  this, &FindReplaceBar::onOptionsChanged);
    connect(ui->matchWordButton, &QToolButton::toggled,  this, &FindReplaceBar::onOptionsChanged);
    connect(ui->prevButton,      &QToolButton::clicked,  this, &FindReplaceBar::onFindPrevious);
    connect(ui->nextButton,      &QToolButton::clicked,  this, &FindReplaceBar::onFindNext);
    connect(ui->replaceButton,   &QToolButton::clicked,  this, &FindReplaceBar::onReplace);
    connect(ui->replaceAllButton,&QToolButton::clicked,  this, &FindReplaceBar::onReplaceAll);
    connect(ui->closeButton,     &QToolButton::clicked,  this, &FindReplaceBar::onClose);

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
    if (ui->matchCaseButton->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (ui->matchWordButton->isChecked())
        flags |= QTextDocument::FindWholeWords;
    return flags;
}

///
/// \brief FindReplaceBar::updateMatchCount
/// \param current
/// \param total
///
void FindReplaceBar::updateMatchCount(int current, int total)
{
    if(total == 0)
        ui->matchCountLabel->setText(ui->searchEdit->text().isEmpty() ? QString() : tr("No results"));
    else
        ui->matchCountLabel->setText(tr("%1 of %2").arg(current).arg(total));
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
    move(qMax(0, x), 0);
}

///
/// \brief FindReplaceBar::setReplaceVisible
/// \param visible
///
void FindReplaceBar::setReplaceVisible(bool visible)
{
    ui->replaceRow->setVisible(visible);
    ui->expandButton->setArrowType(visible ? Qt::DownArrow : Qt::RightArrow);
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
    setReplaceVisible(!ui->replaceRow->isVisible());

    layout()->activate();
    setFixedHeight(sizeHint().height());

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
