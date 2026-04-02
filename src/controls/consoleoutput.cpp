#include <QApplication>
#include <QClipboard>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QSizePolicy>
#include <QStyledItemDelegate>
#include <QToolBar>
#include <QToolButton>
#include "consoleoutput.h"
#include "ui_consoleoutput.h"

namespace {
static const int MessageTypeRole = Qt::UserRole;

struct MessageStyle {
    QColor bg;
    QColor border;
    QColor iconColor;
    QColor textColor;
    QString icon;
};

MessageStyle styleForType(ConsoleOutput::MessageType type)
{
    switch (type) {
        case ConsoleOutput::MessageType::Warning:
            return { QColor("#FFF8E1"), QColor("#F9A825"), QColor("#F9A825"), QColor("#4A3000"), QStringLiteral("\u26A0") };
        case ConsoleOutput::MessageType::Error:
            return { QColor("#FFEBEE"), QColor("#E53935"), QColor("#E53935"), QColor("#7F0000"), QStringLiteral("\u2716") };
        case ConsoleOutput::MessageType::Debug:
            return { Qt::white, QColor(), QColor("#1565C0"), QColor("#37474F"), QStringLiteral("\u25CF") };
        default:
            return { Qt::white, QColor(), QColor("#1565C0"), Qt::black, QStringLiteral("\u2139") };
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
            bg = QColor("#F8F8F8");
        painter->fillRect(option.rect, bg);

        if (style.border.isValid())
            painter->fillRect(option.rect.left(), option.rect.top(), 3, option.rect.height(), style.border);

        if (option.state & QStyle::State_Selected) {
            QColor sel = option.palette.highlight().color();
            sel.setAlpha(55);
            painter->fillRect(option.rect, sel);
        }

        painter->setPen(QColor("#E0E0E0"));
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());

        constexpr int leftPad = 8;
        constexpr int iconW = 16;
        const bool hasIcon = !style.icon.isEmpty();
        const int textLeft = leftPad + (hasIcon ? iconW + 4 : 0);
        const QRect textRect = option.rect.adjusted(textLeft, 2, -6, -2);

        if (hasIcon) {
            painter->setPen(style.iconColor);
            QFont iconFont = option.font;
            iconFont.setPointSize(9);
            painter->setFont(iconFont);
            const QRect iconRect(option.rect.left() + leftPad, option.rect.top(), iconW, option.rect.height());
            painter->drawText(iconRect, Qt::AlignVCenter | Qt::AlignHCenter, style.icon);
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
        const bool hasIcon = !style.icon.isEmpty();
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

    ui->toolBar->setStyleSheet(
        "QToolBar { border: none; background: transparent; spacing: 2px; padding: 1px 2px; }"
        "QToolBar::separator { width: 1px; background: #BDBDBD; margin: 4px 2px; }");

    const QString uncheckedQss =
        "color:#9E9E9E; background:transparent;"
        "border:1px solid #BDBDBD; border-radius:4px;"
        "min-width:22px; max-width:22px; min-height:22px; max-height:22px;"
        "padding:0px; font-size:11px;";

    auto styleFilterBtn = [&](QAction* action, const QString& checkedQss) {
        auto* btn = qobject_cast<QToolButton*>(ui->toolBar->widgetForAction(action));
        if (!btn) return;
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        btn->setStyleSheet(
            QString("QToolButton { %1 }"
                    "QToolButton:!checked { %2 }").arg(checkedQss, uncheckedQss));
    };

    styleFilterBtn(ui->actionFilterLog,
        "color:#1565C0; background:#E3F2FD;"
        "border:1px solid #64B5F6; border-radius:4px;"
        "min-width:22px; max-width:22px; min-height:22px; max-height:22px;"
        "padding:0px; font-size:11px;");

    styleFilterBtn(ui->actionFilterWarn,
        "color:#E65100; background:#FFF8E1;"
        "border:1px solid #FFA726; border-radius:4px;"
        "min-width:22px; max-width:22px; min-height:22px; max-height:22px;"
        "padding:0px; font-size:11px;");

    styleFilterBtn(ui->actionFilterError,
        "color:#C62828; background:#FFEBEE;"
        "border:1px solid #E57373; border-radius:4px;"
        "min-width:22px; max-width:22px; min-height:22px; max-height:22px;"
        "padding:0px; font-size:11px;");

    auto* filterSpacer = new QWidget(ui->toolBar);
    filterSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->toolBar->insertWidget(ui->actionFilterLog, filterSpacer);

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
    ui->actionFilterLog->setText(QStringLiteral("\u2139"));
    ui->actionFilterWarn->setText(QStringLiteral("\u26A0"));
    ui->actionFilterError->setText(QStringLiteral("\u2716"));

    ui->actionFilterLog->setToolTip(QStringLiteral("\u2139 %1").arg(_logCount));
    ui->actionFilterWarn->setToolTip(QStringLiteral("\u26A0 %1").arg(_warnCount));
    ui->actionFilterError->setToolTip(QStringLiteral("\u2716 %1").arg(_errorCount));

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

    auto copyAction = menu.addAction(QIcon(":/res/actionCopy.png"), tr("Copy"), this, [this]() {
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
