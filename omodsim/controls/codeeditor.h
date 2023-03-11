#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QCompleter>
#include <QPlainTextEdit>
#include "jshighlighter.h"

///
/// \brief The CodeEditor class
///
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    void setForegroundColor(const QColor& clr);
    void setBackgroundColor(const QColor& clr);
    void setLineColor(const QColor& clr);

    QCompleter* completer() const;
    void setCompleter(QCompleter* c);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect& rect, int dy);
    void insertCompletion(const QString& completion);

private:
     QString textUnderCursor() const;

private:
    QColor _lineColor;
    QWidget* _lineNumberArea;
    JSHighlighter* _highlighter;
    QCompleter* _compliter;
};

///
/// \brief The LineNumberArea class
///
class LineNumberArea : public QWidget
{
public:
    LineNumberArea(CodeEditor *editor)
        : QWidget(editor)
        ,_codeEditor(editor)
    {
    }

    QSize sizeHint() const override
    {
        return QSize(_codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        _codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor* _codeEditor;
};

#endif // CODEEDITOR_H
