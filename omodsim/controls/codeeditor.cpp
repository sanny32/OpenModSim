#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QAbstractItemView>
#include "jscompleter.h"
#include "codeeditor.h"

///
/// \brief CodeEditor::CodeEditor
/// \param parent
///
CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    _lineNumberArea = new LineNumberArea(this);
    _highlighter = new JSHighlighter(document());

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    setBackgroundColor(Qt::white);
    setLineColor(QColor(Qt::lightGray).lighter(125));

    setFont(QFont("Fira Code"));
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
    setCompleter(new JSCompleter(this));

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

///
/// \brief CodeEditor::lineNumberAreaWidth
/// \return
///
int CodeEditor::lineNumberAreaWidth()
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
/// \brief CodeEditor::setForegroundColor
/// \param clr
///
void CodeEditor::setForegroundColor(const QColor& clr)
{
    auto pal = palette();
    pal.setColor(QPalette::Text, clr);
    setPalette(pal);
}

///
/// \brief CodeEditor::setBackgroundColor
/// \param clr
///
void CodeEditor::setBackgroundColor(const QColor& clr)
{
    auto pal = palette();
    pal.setColor(QPalette::Base, clr);
    pal.setColor(QPalette::Window, clr);
    setPalette(pal);
}

///
/// \brief CodeEditor::setLineColor
/// \param clr
///
void CodeEditor::setLineColor(const QColor& clr)
{
    _lineColor = clr;
}

///
/// \brief CodeEditor::setCompleter
/// \param c
///
void CodeEditor::setCompleter(QCompleter* c)
{
    if (_compliter)
        _compliter->disconnect(this);

    _compliter = c;

    if (!_compliter)
        return;

    _compliter->setWidget(this);
    QObject::connect(c, QOverload<const QString &>::of(&QCompleter::activated),this, &CodeEditor::insertCompletion);
}

///
/// \brief CodeEditor::completer
/// \return
///
QCompleter* CodeEditor::completer() const
{
    return _compliter;
}

///
/// \brief CodeEditor::updateLineNumberAreaWidth
///
void CodeEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

///
/// \brief CodeEditor::updateLineNumberArea
/// \param rect
/// \param dy
///
void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        _lineNumberArea->scroll(0, dy);
    else
        _lineNumberArea->update(0, rect.y(), _lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

///
/// \brief CodeEditor::insertCompletion
/// \param completion
///
void CodeEditor::insertCompletion(const QString& completion)
{
    if (_compliter->widget() != this)
        return;

    QTextCursor tc = textCursor();
    int extra = completion.length() - _compliter->completionPrefix().length();
    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));
    setTextCursor(tc);
}

///
/// \brief CodeEditor::resizeEvent
/// \param e
///
void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    const auto cr = contentsRect();
    _lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

///
/// \brief CodeEditor::focusInEvent
/// \param e
///
void CodeEditor::focusInEvent(QFocusEvent *e)
{
    if (_compliter)
        _compliter->setWidget(this);

    QPlainTextEdit::focusInEvent(e);
}

///
/// \brief CodeEditor::keyPressEvent
/// \param e
///
void CodeEditor::keyPressEvent(QKeyEvent *e)
{
    QAbstractItemView* popup = (_compliter) ? _compliter->popup() : nullptr;
    if (popup && popup->isVisible()) {
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

    const bool isShortcut = (e->modifiers().testFlag(Qt::ControlModifier) && e->key() == Qt::Key_E); // CTRL+E
    if (!_compliter || !isShortcut)
        QPlainTextEdit::keyPressEvent(e);

    const bool ctrlOrShift = e->modifiers().testFlag(Qt::ControlModifier) ||
                             e->modifiers().testFlag(Qt::ShiftModifier);
    if (!_compliter || !popup || (ctrlOrShift && e->text().isEmpty()))
        return;

    static QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-="); // end of word
    const bool hasModifier = (e->modifiers() != Qt::NoModifier) && !ctrlOrShift;
    QString completionPrefix = textUnderCursor();

    qDebug() << completionPrefix;
    if (!isShortcut && (hasModifier || e->text().isEmpty() || completionPrefix.length() < 3
                      /*|| eow.contains(e->text().right(1))*/)) {
        popup->hide();
        return;
    }

    if (completionPrefix != _compliter->completionPrefix())
    {
        _compliter->setCompletionPrefix(completionPrefix);
        popup->setCurrentIndex(_compliter->completionModel()->index(0, 0));
    }

    if(popup)
    {
        QRect cr = cursorRect();
        cr.setWidth(popup->sizeHintForColumn(0) + popup->verticalScrollBar()->sizeHint().width());
        _compliter->complete(cr);
    }
}
///
/// \brief CodeEditor::highlightCurrentLine
///
void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly())
    {
        QTextEdit::ExtraSelection selection;

        selection.format.setBackground(_lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

///
/// \brief CodeEditor::lineNumberAreaPaintEvent
/// \param event
///
void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
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
/// \brief CodeEditor::textUnderCursor
/// \return
///
QString CodeEditor::textUnderCursor() const
{
    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::PreviousWord);
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}
