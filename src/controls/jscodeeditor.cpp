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
/// \brief JSCodeEditor::highlightAllMatches
/// \param text
/// \return
///
int JSCodeEditor::highlightAllMatches(const QString& text)
{
    _searchSelections.clear();
    _currentMatchIndex = -1;

    if(text.isEmpty())
    {
        highlightCurrentLine();
        return 0;
    }

    const auto cur = textCursor();
    QTextCursor findCursor(document());

    while(!(findCursor = document()->find(text, findCursor)).isNull())
    {
        QTextEdit::ExtraSelection extra;
        extra.format.setBackground(Qt::yellow);
        extra.cursor = findCursor;
        _searchSelections.append(extra);
    }

    // Determine current match based on cursor position
    for(int i = 0; i < _searchSelections.size(); ++i)
    {
        if(_searchSelections[i].cursor.selectionStart() >= cur.position())
        {
            _currentMatchIndex = i;
            break;
        }
    }

    if(_currentMatchIndex == -1 && !_searchSelections.isEmpty())
        _currentMatchIndex = 0;

    // Highlight current match differently
    if(_currentMatchIndex >= 0 && _currentMatchIndex < _searchSelections.size())
        _searchSelections[_currentMatchIndex].format.setBackground(QColor(255, 150, 50));

    highlightCurrentLine();
    return _searchSelections.size();
}

///
/// \brief JSCodeEditor::clearSearchHighlights
///
void JSCodeEditor::clearSearchHighlights()
{
    _searchSelections.clear();
    _currentMatchIndex = -1;
    highlightCurrentLine();
}

///
/// \brief JSCodeEditor::findNext
/// \param text
/// \return
///
bool JSCodeEditor::findNext(const QString& text)
{
    if(_searchSelections.isEmpty() || text.isEmpty())
        return false;

    _currentMatchIndex++;
    if(_currentMatchIndex >= _searchSelections.size())
        _currentMatchIndex = 0;

    // Reset all to yellow
    for(auto& sel : _searchSelections)
        sel.format.setBackground(Qt::yellow);

    // Highlight current match
    _searchSelections[_currentMatchIndex].format.setBackground(QColor(255, 150, 50));

    // Move cursor to current match
    auto cursor = _searchSelections[_currentMatchIndex].cursor;
    setTextCursor(cursor);

    highlightCurrentLine();
    return true;
}

///
/// \brief JSCodeEditor::findPrevious
/// \param text
/// \return
///
bool JSCodeEditor::findPrevious(const QString& text)
{
    if(_searchSelections.isEmpty() || text.isEmpty())
        return false;

    _currentMatchIndex--;
    if(_currentMatchIndex < 0)
        _currentMatchIndex = _searchSelections.size() - 1;

    // Reset all to yellow
    for(auto& sel : _searchSelections)
        sel.format.setBackground(Qt::yellow);

    // Highlight current match
    _searchSelections[_currentMatchIndex].format.setBackground(QColor(255, 150, 50));

    // Move cursor to current match
    auto cursor = _searchSelections[_currentMatchIndex].cursor;
    setTextCursor(cursor);

    highlightCurrentLine();
    return true;
}

///
/// \brief JSCodeEditor::replaceCurrent
/// \param text
/// \param replacement
///
void JSCodeEditor::replaceCurrent(const QString& text, const QString& replacement)
{
    auto cursor = textCursor();
    if(cursor.hasSelection() && cursor.selectedText().compare(text, Qt::CaseInsensitive) == 0)
    {
        cursor.insertText(replacement);
        setTextCursor(cursor);
    }

    highlightAllMatches(text);
    if(!_searchSelections.isEmpty())
        findNext(text);
}

///
/// \brief JSCodeEditor::replaceAll
/// \param text
/// \param replacement
/// \return
///
int JSCodeEditor::replaceAll(const QString& text, const QString& replacement)
{
    if(text.isEmpty())
        return 0;

    int count = 0;
    auto cursor = textCursor();
    cursor.beginEditBlock();

    QTextCursor findCursor(document());
    while(!(findCursor = document()->find(text, findCursor)).isNull())
    {
        findCursor.insertText(replacement);
        count++;
    }

    cursor.endEditBlock();

    clearSearchHighlights();
    return count;
}

///
/// \brief JSCodeEditor::currentMatchIndex
/// \return
///
int JSCodeEditor::currentMatchIndex() const
{
    return _currentMatchIndex;
}

///
/// \brief JSCodeEditor::totalMatchCount
/// \return
///
int JSCodeEditor::totalMatchCount() const
{
    return _searchSelections.size();
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

    if(e->key() == Qt::Key_F1)
    {
        QStringList keyList;

        QTextCursor tc = textCursor();
        tc.select(QTextCursor::WordUnderCursor);
        keyList << tc.selectedText();

        tc.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor, 2);
        tc.select(QTextCursor::WordUnderCursor);
        if(tc.selectedText() == ".")
        {
            keyList << ".";
            tc.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor, 2);
            tc.select(QTextCursor::WordUnderCursor);
            keyList << tc.selectedText();
        }

        QString helpKey;
        for(auto it = keyList.rbegin(); it != keyList.rend(); ++it)
            helpKey+= it->trimmed();

        if(!helpKey.isEmpty())
            emit helpContext(helpKey);

        return;
    }

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

    if (hasModifier || e->text().isEmpty() ||
            e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete ||
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

    QList<QTextEdit::ExtraSelection> extraSelections;

    // Add search highlights
    extraSelections.append(_searchSelections);

    // Add current line highlight
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
