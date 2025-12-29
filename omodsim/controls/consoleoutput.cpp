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
{
    _header = new QWidget(this);

    auto headerLayout = new QHBoxLayout(_header);
    headerLayout->setContentsMargins(4, 0, 4, 2);

    _clearButton = createToolButton(QString(), QIcon(":/res/edit-delete.svg"), tr("Clear console"));
    _collapseButton = createToolButton("✕");

    headerLayout->addWidget(_clearButton);
    headerLayout->addSpacerItem(new QSpacerItem(10, 0, QSizePolicy::Expanding));
    headerLayout->addWidget(_collapseButton);

    _textEdit = new QPlainTextEdit(this);
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

    mainLayout->addWidget(_header);
    mainLayout->addWidget(_textEdit);

    _header->setFixedHeight(_header->sizeHint().height());

    const int lineHeight = _textEdit->fontMetrics().lineSpacing() * 2;
    const int extraHeight = _textEdit->frameWidth() * 2;
    const int minHeight = _header->height() + lineHeight + extraHeight;
    setMinimumHeight(minHeight);

    connect(_clearButton, &QPushButton::clicked, this, &ConsoleOutput::clear);
    connect(_collapseButton, &QPushButton::clicked, this, &ConsoleOutput::collapse);
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
/// \param text
/// \param icon
/// \param toolTip
/// \return
///
QToolButton* ConsoleOutput::createToolButton(const QString& text, const QIcon& icon, const QString& toolTip)
{
    const QSize iconSize = { 12, 12 };
    const QSize toolButtonSize = { 24, 24 };

    auto btn = new QToolButton(_header);
    btn->setText(text);
    btn->setIcon(icon);
    btn->setIconSize(iconSize);
    btn->setFixedSize(toolButtonSize);
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
