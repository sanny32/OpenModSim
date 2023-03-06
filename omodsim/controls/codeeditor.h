#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>

///
/// \brief The CodeEditor class
///
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget* _lineNumberArea;
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
