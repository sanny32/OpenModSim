#include <QPainter>
#include <QTextBlock>
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
    connect(this, &CodeEditor::textChanged, this, &CodeEditor::on_textChnaged);

    setBackgroundColor(Qt::white);
    setLineColor(QColor(Qt::lightGray).lighter(125));

    setFont(QFont("Fira Code"));
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);

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
/// \brief CodeEditor::on_textChnaged
///
void CodeEditor::on_textChnaged()
{
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
