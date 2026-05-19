// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file consoleoutput.cpp
/// \brief Implements the consoleoutput functionality.
///

#include <QApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QSizePolicy>
#include <QStyledItemDelegate>
#include <QToolBar>
#include <QToolButton>
#include "../styles/appcolors.h"
#include "consoleoutput.h"
#include "themedicons.h"
#include "ui_consoleoutput.h"

namespace {
static const int MessageTypeRole = Qt::UserRole;

struct MessageStyle {
    QColor bg;
    QColor border;
    QColor textColor;
    QIcon icon;
};

MessageStyle styleForType(ConsoleOutput::MessageType type)
{
    switch (type) {
        case ConsoleOutput::MessageType::Warning:
            return MessageStyle{ AppColors::warningBackground(), AppColors::warningBorder(), AppColors::warningForeground(), themedIcon(QStringLiteral("omodsim/warning"), ThemedIcons::Fallback) };
        case ConsoleOutput::MessageType::Error:
            return MessageStyle{ AppColors::errorBackground(), AppColors::errorBorder(), AppColors::errorForeground(), themedIcon(QStringLiteral("omodsim/error"), ThemedIcons::Fallback) };
        case ConsoleOutput::MessageType::Debug:
            return MessageStyle{ AppColors::canvasBackground(), QColor(), AppColors::debugForeground(), themedIcon(QStringLiteral("omodsim/information"), ThemedIcons::Fallback) };
        default:
            return MessageStyle{ AppColors::canvasBackground(), QColor(), AppColors::logForeground(), themedIcon(QStringLiteral("omodsim/information"), ThemedIcons::Fallback) };
    }
}

///
/// \brief iconForType
/// \param type
/// \return
///
QIcon iconForType(ConsoleOutput::MessageType type)
{
    switch (type) {
        case ConsoleOutput::MessageType::Warning:
            return themedIcon(QStringLiteral("omodsim/warning"));
        case ConsoleOutput::MessageType::Error:
            return themedIcon(QStringLiteral("omodsim/error"));
        case ConsoleOutput::MessageType::Debug:
        case ConsoleOutput::MessageType::Log:
        default:
            return themedIcon(QStringLiteral("omodsim/information"));
    }
}

class ConsoleItemDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        const auto type = static_cast<ConsoleOutput::MessageType>(index.data(MessageTypeRole).toInt());
        const auto style = styleForType(type);

        painter->save();

        QColor bg = style.bg;
        if (!style.border.isValid() && (option.features & QStyleOptionViewItem::Alternate))
            bg = AppColors::alternateBackground();
        painter->fillRect(option.rect, bg);

        if (style.border.isValid())
            painter->fillRect(option.rect.left(), option.rect.top(), 3, option.rect.height(), style.border);

        if (option.state & QStyle::State_Selected) {
            QColor sel = option.palette.highlight().color();
            sel.setAlpha(55);
            painter->fillRect(option.rect, sel);
        }

        painter->setPen(AppColors::divider());
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());

        constexpr int leftPad = 8;
        constexpr int iconW = 16;
        const bool hasIcon = !style.icon.isNull();
        const int textLeft = leftPad + (hasIcon ? iconW + 4 : 0);
        const QRect textRect = option.rect.adjusted(textLeft, 2, -6, -2);

        if (hasIcon) {
            const QRect iconRect(option.rect.left() + leftPad,
                                 option.rect.top() + (option.rect.height() - iconW) / 2,
                                 iconW,
                                 iconW);
            style.icon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal, QIcon::On);
        }

        painter->setPen(style.textColor);
        painter->setFont(option.font);
        const QString text = index.data(Qt::DisplayRole).toString();
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine, text);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        const auto type = static_cast<ConsoleOutput::MessageType>(index.data(MessageTypeRole).toInt());
        const auto style = styleForType(type);

        constexpr int leftPad = 8;
        constexpr int iconW = 16;
        constexpr int rightPad = 6;
        const bool hasIcon = !style.icon.isNull();
        const int textLeft = leftPad + (hasIcon ? iconW + 4 : 0);
        const QString text = index.data(Qt::DisplayRole).toString();
        const int textWidth = option.fontMetrics.horizontalAdvance(text);

        return { textLeft + textWidth + rightPad, option.fontMetrics.height() + 10 };
    }
};
} // namespace

///
/// \brief ConsoleOutput::ConsoleOutput
///
ConsoleOutput::ConsoleOutput(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ConsoleOutput)
{
    ui->setupUi(this);

    auto setupToolbarBtn = [&](QAction* action) {
        auto* btn = qobject_cast<QToolButton*>(ui->toolBar->widgetForAction(action));
        if (!btn) return;
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        btn->setFixedSize(24, 24);
        btn->setProperty("preservePressedIconAlignment", true);
    };

    setupToolbarBtn(ui->actionFilterLog);
    setupToolbarBtn(ui->actionFilterWarn);
    setupToolbarBtn(ui->actionFilterError);
    setupToolbarBtn(ui->actionClear);

    auto* filterSpacer = new QWidget(ui->toolBar);
    filterSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->toolBar->insertWidget(ui->actionFilterLog, filterSpacer);

    ui->actionClear->setIcon(themedIcon(QStringLiteral("omodsim/clear")));

    ui->listWidget->setItemDelegate(new ConsoleItemDelegate(ui->listWidget));
    ui->listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->listWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->listWidget->setTextElideMode(Qt::ElideNone);

    const int lineHeight = QFontMetrics(QFont("Fira Code")).lineSpacing() * 2;
    setMinimumHeight(ui->toolBar->sizeHint().height() + lineHeight);

    connect(ui->actionClear, &QAction::triggered, this, &ConsoleOutput::clear);
    connect(ui->actionFilterLog, &QAction::toggled, this, &ConsoleOutput::applyFilters);
    connect(ui->actionFilterWarn, &QAction::toggled, this, &ConsoleOutput::applyFilters);
    connect(ui->actionFilterError, &QAction::toggled, this, &ConsoleOutput::applyFilters);
    connect(ui->listWidget, &QWidget::customContextMenuRequested,
            this, &ConsoleOutput::on_customContextMenuRequested);
}

///
/// \brief ConsoleOutput::~ConsoleOutput
///
ConsoleOutput::~ConsoleOutput()
{
    delete ui;
}

///
/// \brief ConsoleOutput::changeEvent
///
void ConsoleOutput::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);
    QWidget::changeEvent(event);
}

///
/// \brief ConsoleOutput::setMaxLines
///
void ConsoleOutput::setMaxLines(int n)
{
    _maxLines = qMax(1, n);
    while (ui->listWidget->count() > _maxLines) {
        const auto evictType = static_cast<MessageType>(ui->listWidget->item(0)->data(MessageTypeRole).toInt());
        switch (evictType) {
            case MessageType::Warning: _warnCount--; break;
            case MessageType::Error:   _errorCount--; break;
            default:                   _logCount--;   break;
        }
        delete ui->listWidget->takeItem(0);
    }
    updateFilterButtons();
}

///
/// \brief ConsoleOutput::addMessage
///
void ConsoleOutput::addMessage(const QString& text, MessageType type, const QString& source)
{
    while (ui->listWidget->count() >= _maxLines) {
        const auto evictType = static_cast<MessageType>(ui->listWidget->item(0)->data(MessageTypeRole).toInt());
        switch (evictType) {
            case MessageType::Warning: _warnCount--; break;
            case MessageType::Error:   _errorCount--; break;
            default:                   _logCount--;   break;
        }
        delete ui->listWidget->takeItem(0);
    }

    const QString displayText = source.isEmpty() ? text : QString("[%1] %2").arg(source, text);
    auto* item = new QListWidgetItem(displayText, ui->listWidget);
    item->setData(MessageTypeRole, static_cast<int>(type));

    bool visible = true;
    switch (type) {
        case MessageType::Warning:
            visible = ui->actionFilterWarn->isChecked();
            _warnCount++;
            break;
        case MessageType::Error:
            visible = ui->actionFilterError->isChecked();
            _errorCount++;
            break;
        default:
            visible = ui->actionFilterLog->isChecked();
            _logCount++;
            break;
    }
    item->setHidden(!visible);

    updateFilterButtons();
    ui->listWidget->scrollToBottom();
}

///
/// \brief ConsoleOutput::clear
///
void ConsoleOutput::clear()
{
    ui->listWidget->clear();
    _logCount = _warnCount = _errorCount = 0;
    updateFilterButtons();
}

///
/// \brief ConsoleOutput::isEmpty
///
bool ConsoleOutput::isEmpty() const
{
    return ui->listWidget->count() == 0;
}

///
/// \brief ConsoleOutput::applyFilters
///
void ConsoleOutput::applyFilters()
{
    const bool showLog = ui->actionFilterLog->isChecked();
    const bool showWarn = ui->actionFilterWarn->isChecked();
    const bool showError = ui->actionFilterError->isChecked();

    for (int i = 0; i < ui->listWidget->count(); ++i) {
        auto* item = ui->listWidget->item(i);
        const auto type = static_cast<MessageType>(item->data(MessageTypeRole).toInt());
        bool visible = true;
        switch (type) {
            case MessageType::Warning: visible = showWarn;  break;
            case MessageType::Error:   visible = showError; break;
            default:                   visible = showLog;   break;
        }
        item->setHidden(!visible);
    }
}

///
/// \brief ConsoleOutput::updateFilterButtons
///
void ConsoleOutput::updateFilterButtons()
{
    ui->actionFilterLog->setIcon(iconForType(MessageType::Log));
    ui->actionFilterWarn->setIcon(iconForType(MessageType::Warning));
    ui->actionFilterError->setIcon(iconForType(MessageType::Error));

    ui->actionFilterLog->setToolTip(QStringLiteral("Info: %1").arg(_logCount));
    ui->actionFilterWarn->setToolTip(QStringLiteral("Warnings: %1").arg(_warnCount));
    ui->actionFilterError->setToolTip(QStringLiteral("Errors: %1").arg(_errorCount));

    ui->actionFilterLog->setVisible(_logCount > 0);
    ui->actionFilterWarn->setVisible(_warnCount > 0);
    ui->actionFilterError->setVisible(_errorCount > 0);
}

///
/// \brief ConsoleOutput::on_customContextMenuRequested
///
void ConsoleOutput::on_customContextMenuRequested(const QPoint& pos)
{
    QMenu menu(ui->listWidget);

    auto copyAction = menu.addAction(themedIcon(QStringLiteral("omodsim/copy")), tr("Copy"), this, [this]() {
        QStringList lines;
        for (auto* item : ui->listWidget->selectedItems())
            lines << item->text();
        if (!lines.isEmpty())
            QApplication::clipboard()->setText(lines.join('\n'));
    });
    copyAction->setEnabled(!ui->listWidget->selectedItems().isEmpty());

    menu.addSeparator();

    auto clearAction = menu.addAction(tr("Clear"), this, [this]() { clear(); });
    clearAction->setEnabled(!isEmpty());

    menu.exec(ui->listWidget->mapToGlobal(pos));
}
