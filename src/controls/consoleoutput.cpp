#include <QMenu>
#include <QClipboard>
#include <QApplication>
#include <QListWidget>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QToolButton>
#include "consoleoutput.h"
#include "ui_consoleoutput.h"

// ── Custom data role ─────────────────────────────────────────────────────────
static const int MessageTypeRole = Qt::UserRole;

// ── Per-type visual style ────────────────────────────────────────────────────
struct MessageStyle {
    QColor bg;
    QColor border;      // left border; invalid = none
    QColor iconColor;
    QColor textColor;
    QString icon;
};

static MessageStyle styleForType(ConsoleOutput::MessageType type)
{
    switch (type) {
        case ConsoleOutput::MessageType::Warning:
            return { QColor("#FFF8E1"), QColor("#F9A825"), QColor("#F9A825"), QColor("#4a3000"), "⚠" };
        case ConsoleOutput::MessageType::Error:
            return { QColor("#FFEBEE"), QColor("#E53935"), QColor("#E53935"), QColor("#7f0000"), "✖" };
        case ConsoleOutput::MessageType::Debug:
            return { Qt::white, QColor(), QColor("#1565C0"), QColor("#37474F"), "●" };
        default: // Log
            return { Qt::white, QColor(), QColor(), Qt::black, "" };
    }
}

// ── Item delegate ────────────────────────────────────────────────────────────
class ConsoleItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        const auto type  = static_cast<ConsoleOutput::MessageType>(index.data(MessageTypeRole).toInt());
        const auto style = styleForType(type);

        painter->save();

        // Background
        QColor bg = style.bg;
        if (!style.border.isValid() && (option.features & QStyleOptionViewItem::Alternate))
            bg = QColor("#F8F8F8");
        painter->fillRect(option.rect, bg);

        // Left border strip (3 px)
        if (style.border.isValid())
            painter->fillRect(option.rect.left(), option.rect.top(), 3, option.rect.height(), style.border);

        // Selection overlay
        if (option.state & QStyle::State_Selected) {
            QColor sel = option.palette.highlight().color();
            sel.setAlpha(55);
            painter->fillRect(option.rect, sel);
        }

        // Bottom separator
        painter->setPen(QColor("#E0E0E0"));
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());

        // Content geometry
        constexpr int leftPad   = 8;
        constexpr int iconW     = 16;
        const bool    hasIcon   = !style.icon.isEmpty();
        const int     textLeft  = leftPad + (hasIcon ? iconW + 4 : 0);
        const QRect   textRect  = option.rect.adjusted(textLeft, 2, -6, -2);

        // Icon
        if (hasIcon) {
            painter->setPen(style.iconColor);
            QFont f = option.font;
            f.setPointSize(9);
            painter->setFont(f);
            const QRect iconRect(option.rect.left() + leftPad, option.rect.top(), iconW, option.rect.height());
            painter->drawText(iconRect, Qt::AlignVCenter | Qt::AlignHCenter, style.icon);
        }

        // Text (elided to fit width)
        painter->setPen(style.textColor);
        painter->setFont(option.font);
        const QString text = index.data(Qt::DisplayRole).toString();
        const QString elided = option.fontMetrics.elidedText(text, Qt::ElideRight, textRect.width());
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elided);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const override
    {
        return { -1, option.fontMetrics.height() + 10 };
    }
};

// ── ConsoleOutput ────────────────────────────────────────────────────────────
ConsoleOutput::ConsoleOutput(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ConsoleOutput)
{
    ui->setupUi(this);

    // Filter button stylesheets
    const QString uncheckedQss =
        "color:#9E9E9E; background:transparent;"
        "border:1px solid #BDBDBD; border-radius:3px; padding:1px 5px; font-size:11px;";

    auto applyFilterStyle = [&](QToolButton* btn, const QString& checkedQss) {
        btn->setStyleSheet(
            QString("QToolButton { %1 }"
                    "QToolButton:!checked { %2 }").arg(checkedQss, uncheckedQss));
    };

    applyFilterStyle(ui->filterLog,
        "color:#1565C0; background:#E3F2FD;"
        "border:1px solid #64B5F6; border-radius:3px; padding:1px 5px; font-size:11px;");

    applyFilterStyle(ui->filterWarn,
        "color:#E65100; background:#FFF8E1;"
        "border:1px solid #FFA726; border-radius:3px; padding:1px 5px; font-size:11px;");

    applyFilterStyle(ui->filterError,
        "color:#C62828; background:#FFEBEE;"
        "border:1px solid #E57373; border-radius:3px; padding:1px 5px; font-size:11px;");

    // List widget setup
    ui->listWidget->setItemDelegate(new ConsoleItemDelegate(ui->listWidget));

    // Computed minimum height
    const int lineHeight = QFontMetrics(QFont("Fira Code")).lineSpacing() * 2;
    setMinimumHeight(ui->clearButton->sizeHint().height() + lineHeight);

    // Connections
    connect(ui->clearButton,    &QToolButton::clicked,  this, &ConsoleOutput::clear);
    connect(ui->filterLog,      &QToolButton::toggled,  this, &ConsoleOutput::applyFilters);
    connect(ui->filterWarn,     &QToolButton::toggled,  this, &ConsoleOutput::applyFilters);
    connect(ui->filterError,    &QToolButton::toggled,  this, &ConsoleOutput::applyFilters);
    connect(ui->listWidget,     &QWidget::customContextMenuRequested, this, &ConsoleOutput::on_customContextMenuRequested);
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
/// \brief ConsoleOutput::addMessage
///
void ConsoleOutput::addMessage(const QString& text, MessageType type, const QString& source)
{
    const QString displayText = source.isEmpty() ? text : QString("[%1] %2").arg(source, text);
    auto item = new QListWidgetItem(displayText, ui->listWidget);
    item->setData(MessageTypeRole, static_cast<int>(type));
    item->setToolTip(displayText);

    bool visible = true;
    switch (type) {
    case MessageType::Warning:
        visible = ui->filterWarn->isChecked();
        _warnCount++;
        break;
    case MessageType::Error:
        visible = ui->filterError->isChecked();
        _errorCount++;
        break;
    default:
        visible = ui->filterLog->isChecked();
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
/// \brief ConsoleOutput::applyFilters — hide/show items per active filters
///
void ConsoleOutput::applyFilters()
{
    const bool showLog   = ui->filterLog->isChecked();
    const bool showWarn  = ui->filterWarn->isChecked();
    const bool showError = ui->filterError->isChecked();

    for (int i = 0; i < ui->listWidget->count(); ++i) {
        auto item = ui->listWidget->item(i);
        const auto type = static_cast<MessageType>(item->data(MessageTypeRole).toInt());
        bool visible;
        switch (type) {
            case MessageType::Warning: visible = showWarn;  break;
            case MessageType::Error:   visible = showError; break;
            default:                   visible = showLog;   break;
        }
        item->setHidden(!visible);
    }
}

///
/// \brief ConsoleOutput::updateFilterButtons — refresh labels and visibility
///
void ConsoleOutput::updateFilterButtons()
{
    ui->filterLog->setText(QString("ℹ %1").arg(_logCount));
    ui->filterWarn->setText(QString("⚠ %1").arg(_warnCount));
    ui->filterError->setText(QString("✖ %1").arg(_errorCount));

    ui->filterLog->setVisible(_logCount > 0);
    ui->filterWarn->setVisible(_warnCount > 0);
    ui->filterError->setVisible(_errorCount > 0);
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
