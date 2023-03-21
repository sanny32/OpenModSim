#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QAbstractItemView>
#include "jscompleter.h"
#include "jscodeeditor.h"

///
/// \brief console::console
/// \param parent
///
JSCodeEditor::JSCodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    ,_lineColor(200,200,200,70)
    ,_compliter(nullptr)
{
    _lineNumberArea = new LineNumberArea(this);
    _highlighter = new JSHighlighter(document());

    connect(this, &JSCodeEditor::blockCountChanged, this, &JSCodeEditor::updateLineNumberAreaWidth);
    connect(this, &JSCodeEditor::updateRequest, this, &JSCodeEditor::updateLineNumberArea);
    connect(this, &JSCodeEditor::cursorPositionChanged, this, &JSCodeEditor::highlightCurrentLine);

    setBackgroundColor(Qt::white);

    setFont(QFont("Fira Code"));
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    enableAutoComplete(true);
}

///
/// \brief JSCodeEditor::~JSCodeEditor
///
JSCodeEditor::~JSCodeEditor()
{
    if(_compliter)
        delete _compliter;

    delete _highlighter;
    delete _lineNumberArea;
}

///
/// \brief console::lineNumberAreaWidth
/// \return
///
int JSCodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    return 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * qMax(3, digits);
}

///
/// \brief console::setForegroundColor
/// \param clr
///
void JSCodeEditor::setForegroundColor(const QColor& clr)
{
    auto pal = palette();
    pal.setColor(QPalette::Text, clr);
    setPalette(pal);
}

///
/// \brief console::setBackgroundColor
/// \param clr
///
void JSCodeEditor::setBackgroundColor(const QColor& clr)
{
    auto pal = palette();
    pal.setColor(QPalette::Base, clr);
    pal.setColor(QPalette::Window, clr);
    setPalette(pal);
}

///
/// \brief JSCodeEditor::isAutoCompleteEnabled
/// \return
///
bool JSCodeEditor::isAutoCompleteEnabled() const
{
    return _compliter != nullptr;
}

///
/// \brief JSCodeEditor::enableAutoComplete
/// \param enable
///
void JSCodeEditor::enableAutoComplete(bool enable)
{
    if(enable)
    {
        if(!_compliter)
        {
            _compliter = new JSCompleter(this);
            connect(_compliter, QOverload<const QString &>::of(&QCompleter::activated),this, &JSCodeEditor::insertCompletion);
        }
    }
    else if(_compliter)
    {
        delete _compliter;
        _compliter = nullptr;
    }
}

///
/// \brief JSCodeEditor::search
/// \param text
///
void JSCodeEditor::search(const QString& text)
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    const auto cur = textCursor();
    moveCursor(QTextCursor::Start);

    while(find(text))
    {
        QTextEdit::ExtraSelection extra;
        extra.format.setBackground(Qt::yellow);

        extra.cursor = textCursor();
        extraSelections.append(extra);
    }
    setExtraSelections(extraSelections);

    setTextCursor(cur);
    highlightCurrentLine();
}

///
/// \brief console::updateLineNumberAreaWidth
///
void JSCodeEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

///
/// \brief console::updateLineNumberArea
/// \param rect
/// \param dy
///
void JSCodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        _lineNumberArea->scroll(0, dy);
    else
        _lineNumberArea->update(0, rect.y(), _lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

///
/// \brief console::insertCompletion
/// \param completion
///
void JSCodeEditor::insertCompletion(const QString& completion)
{
    QTextCursor tc = textCursor();
    int extra = completion.length() - _compliter->completionPrefix().length();
    tc.insertText(completion.right(extra));
    setTextCursor(tc);
}

///
/// \brief console::resizeEvent
/// \param e
///
void JSCodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    const auto cr = contentsRect();
    _lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

///
/// \brief console::keyPressEvent
/// \param e
///
void JSCodeEditor::keyPressEvent(QKeyEvent *e)
{
    QAbstractItemView* popup = (_compliter) ? _compliter->popup() : nullptr;
    if (popup && popup->isVisible())
    {
        // The following keys are forwarded by the completer to the widget
       switch (e->key()) {
       case Qt::Key_Enter:
       case Qt::Key_Return:
       case Qt::Key_Escape:
       case Qt::Key_Tab:
       case Qt::Key_Backtab:
            e->ignore();
            return; // let the completer do default behavior
       default:
           break;
       }
    }

    QPlainTextEdit::keyPressEvent(e);

    if(!_compliter)
        return;

    const bool ctrlOrShift = e->modifiers().testFlag(Qt::ControlModifier) ||
                             e->modifiers().testFlag(Qt::ShiftModifier);
    if (!popup || (ctrlOrShift && e->text().isEmpty()))
        return;

    static QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-="); // end of word
    const bool wordEnds = eow.contains(e->text().right(1));
    const bool hasModifier = (e->modifiers() != Qt::NoModifier) && !ctrlOrShift;
    QString completionPrefix = textUnderCursor();

    if (hasModifier || e->text().isEmpty() || e->key() == Qt::Key_Backspace ||
            (!_compliter->completionKey().isEmpty() && wordEnds))
    {
        _compliter->setCompletionKey(QString());
        popup->hide();
        return;
    }

    if(e->text() == ".")
    {
         QTextCursor tc = textCursor();
         tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 2);
         tc.select(QTextCursor::WordUnderCursor);
         _compliter->setCompletionKey(tc.selectedText());

         completionPrefix = QString();
    }

    if(!_compliter->completionKey().isEmpty())
    {
        auto model = _compliter->completionModel();

        _compliter->setCompletionPrefix(completionPrefix);
        popup->setCurrentIndex(model->index(0, 0));

        QRect cr = cursorRect();
        const int width = popup->sizeHintForColumn(0) + 8;
        cr.setWidth(width + popup->verticalScrollBar()->sizeHint().width());
        _compliter->complete(cr);
    }
}

///
/// \brief console::highlightCurrentLine
///
void JSCodeEditor::highlightCurrentLine()
{
    if (isReadOnly())
        return;

    auto extraSelections = this->extraSelections();
    QMutableListIterator<QTextEdit::ExtraSelection> i(extraSelections);
    while (i.hasNext())
        if(i.next().format.background().color() == _lineColor)
            i.remove();

    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(_lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);


    setExtraSelections(extraSelections);
}

///
/// \brief console::lineNumberAreaPaintEvent
/// \param event
///
void JSCodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(_lineNumberArea);
    painter.fillRect(event->rect(), QColorConstants::Svg::whitesmoke);

    auto block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            const auto number = QString::number(blockNumber + 1);
            painter.setPen(QColor(0x9F9F9F));
            painter.drawText(0, top, _lineNumberArea->width(), fontMetrics().height(), Qt::AlignCenter, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

///
/// \brief console::textUnderCursor
/// \return
///
QString JSCodeEditor::textUnderCursor() const
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}
