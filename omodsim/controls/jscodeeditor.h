#ifndef JSCODEEDITOR_H
#define JSCODEEDITOR_H

#include <QCompleter>
#include <QPlainTextEdit>
#include "jscompleter.h"
#include "jshighlighter.h"

///
/// \brief The JSCodeEditor class
///
class JSCodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit JSCodeEditor(QWidget *parent = nullptr);
    ~JSCodeEditor() override;

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    void setForegroundColor(const QColor& clr);
    void setBackgroundColor(const QColor& clr);
    void setLineColor(const QColor& clr);

    bool isAutoCompleteEnabled() const;
    void enableAutoComplete(bool enable);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;

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
    JSCompleter* _compliter;
};

///
/// \brief The LineNumberArea class
///
class LineNumberArea : public QWidget
{
public:
    LineNumberArea(JSCodeEditor *editor)
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
    JSCodeEditor* _codeEditor;
};

#endif // JSCODEEDITOR_H
