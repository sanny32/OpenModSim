#include <QMenu>
#include <QPlainTextEdit>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QContextMenuEvent>
#include "consoleoutput.h"

///
/// \brief ConsoleOutput::ConsoleOutput
/// \param parent
///
ConsoleOutput::ConsoleOutput(QWidget* parent)
    : QWidget(parent)
    ,_textEdit(new QPlainTextEdit(this))
{
    auto header = new QWidget(this);

    auto headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(4, 0, 4, 2);

    auto clearButton = createToolButton(header, QString(), QIcon(":/res/edit-delete.svg"), tr("Clear console"));
    auto collapseButton = createToolButton(header, "✕");

    headerLayout->addWidget(clearButton);
    headerLayout->addSpacerItem(new QSpacerItem(10, 0, QSizePolicy::Expanding));
    headerLayout->addWidget(collapseButton);

    _textEdit->setReadOnly(true);
    _textEdit->setUndoRedoEnabled(true);
    _textEdit->setFont(QFont("Fira Code"));
    _textEdit->setTabStopDistance(_textEdit->fontMetrics().horizontalAdvance(' ') * 2);
    _textEdit->setContextMenuPolicy(Qt::CustomContextMenu);

    auto pal = _textEdit->palette();
    pal.setColor(QPalette::Base, Qt::white);
    pal.setColor(QPalette::Window, Qt::white);
    _textEdit->setPalette(pal);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(header);
    mainLayout->addWidget(_textEdit);

    header->setFixedHeight(header->sizeHint().height());

    const int lineHeight = _textEdit->fontMetrics().lineSpacing() * 2;
    const int extraHeight = _textEdit->frameWidth() * 2;
    const int minHeight = header->height() + lineHeight + extraHeight;
    setMinimumHeight(minHeight);

    connect(clearButton, &QToolButton::clicked, this, &ConsoleOutput::clear);
    connect(collapseButton, &QToolButton::clicked, this, &ConsoleOutput::collapse);
    connect(_textEdit, &QWidget::customContextMenuRequested, this, &ConsoleOutput::on_customContextMenuRequested);
}

///
/// \brief ConsoleOutput::addText
/// \param text
/// \param clr
///
void ConsoleOutput::addText(const QString& text, const QColor& clr)
{
    QTextCharFormat fmt;
    fmt.setForeground(clr);
    fmt.setFontWeight(800);
    _textEdit->mergeCurrentCharFormat(fmt);
    _textEdit->insertPlainText(">>\t");

    fmt.setFontWeight(400);
    _textEdit->mergeCurrentCharFormat(fmt);
    _textEdit->insertPlainText(QString("%1\n").arg(text));
    _textEdit->moveCursor(QTextCursor::End);
}

///
/// \brief ConsoleOutput::clear
///
void ConsoleOutput::clear()
{
    _textEdit->setPlainText(QString());
}

///
/// \brief ConsoleOutput::isEmpty
/// \return
///
bool ConsoleOutput::isEmpty() const
{
    return _textEdit->toPlainText().isEmpty();
}

///
/// \brief ConsoleOutput::createToolButton
/// \param parent
/// \param text
/// \param icon
/// \param toolTip
/// \param size
/// \param iconSize
/// \return
///
QToolButton* ConsoleOutput::createToolButton(QWidget* parent, const QString& text, const QIcon& icon, const QString& toolTip, const QSize& size, const QSize& iconSize)
{
    auto btn = new QToolButton(parent);
    btn->setText(text);
    btn->setIcon(icon);

    if(!iconSize.isEmpty())
        btn->setIconSize(iconSize);

    if(!size.isEmpty())
        btn->setFixedSize(size);

    btn->setToolTip(toolTip);
    btn->setAutoRaise(true);

    if(text.isEmpty() && !icon.isNull()) {
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
    else if(!text.isEmpty() && !icon.isNull()) {
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }
    else if(!text.isEmpty() && icon.isNull()) {
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    }

    return btn;
}

///
/// \brief ConsoleOutput::on_customContextMenuRequested
/// \param pos
///
void ConsoleOutput::on_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(_textEdit);

    auto clearAction = menu.addAction(tr("Clear"), this, [this](){
        clear();
    });
    clearAction->setEnabled(!isEmpty());

    menu.exec(_textEdit->mapToGlobal(pos));
}
