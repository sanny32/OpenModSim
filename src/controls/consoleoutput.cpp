#include <QMenu>
#include <QClipboard>
#include <QApplication>
#include <QListWidget>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAbstractItemView>
#include "consoleoutput.h"

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

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        const auto type  = static_cast<ConsoleOutput::MessageType>(
                               index.data(MessageTypeRole).toInt());
        const auto style = styleForType(type);

        painter->save();

        // Background (alternating rows for log/debug)
        QColor bg = style.bg;
        if (!style.border.isValid() && index.row() % 2 != 0)
            bg = QColor("#F8F8F8");
        painter->fillRect(option.rect, bg);

        // Left border strip (3 px)
        if (style.border.isValid())
            painter->fillRect(option.rect.left(), option.rect.top(),
                              3, option.rect.height(), style.border);

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
            const QRect iconRect(option.rect.left() + leftPad, option.rect.top(),
                                 iconW, option.rect.height());
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
    , _listWidget(new QListWidget(this))
{
    // ── Header ───────────────────────────────────────────────────────────────
    auto header       = new QWidget(this);
    auto headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(4, 0, 4, 2);
    headerLayout->setSpacing(4);

    _clearButton = createToolButton(header, QString(),
                                    QIcon(":/res/edit-delete.svg"),
                                    tr("Clear console"));

    // Filter buttons factory (lambda)
    auto makeFilter = [&](const QString& initText,
                          const QString& checkedQss,
                          const QString& uncheckedQss) -> QToolButton* {
        auto btn = new QToolButton(header);
        btn->setCheckable(true);
        btn->setChecked(true);
        btn->setText(initText);
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        btn->setAutoRaise(false);
        btn->setVisible(false);
        btn->setStyleSheet(
            QString("QToolButton { %1 }"
                    "QToolButton:!checked { %2 }").arg(checkedQss, uncheckedQss));
        return btn;
    };

    const QString uncheckedQss =
        "color:#9E9E9E; background:transparent;"
        "border:1px solid #BDBDBD; border-radius:3px; padding:1px 5px; font-size:11px;";

    _filterLog = makeFilter("ℹ 0",
        "color:#1565C0; background:#E3F2FD;"
        "border:1px solid #64B5F6; border-radius:3px; padding:1px 5px; font-size:11px;",
        uncheckedQss);

    _filterWarn = makeFilter("⚠ 0",
        "color:#E65100; background:#FFF8E1;"
        "border:1px solid #FFA726; border-radius:3px; padding:1px 5px; font-size:11px;",
        uncheckedQss);

    _filterError = makeFilter("✖ 0",
        "color:#C62828; background:#FFEBEE;"
        "border:1px solid #E57373; border-radius:3px; padding:1px 5px; font-size:11px;",
        uncheckedQss);

    auto collapseButton = createToolButton(header, "✕");

    headerLayout->addWidget(_clearButton);
    headerLayout->addSpacerItem(new QSpacerItem(10, 0, QSizePolicy::Expanding));
    headerLayout->addWidget(_filterLog);
    headerLayout->addWidget(_filterWarn);
    headerLayout->addWidget(_filterError);
    headerLayout->addWidget(collapseButton);

    // ── List widget ──────────────────────────────────────────────────────────
    _listWidget->setItemDelegate(new ConsoleItemDelegate(_listWidget));
    _listWidget->setFont(QFont("Fira Code"));
    _listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _listWidget->setFrameShape(QFrame::NoFrame);
    _listWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    auto pal = _listWidget->palette();
    pal.setColor(QPalette::Base, Qt::white);
    _listWidget->setPalette(pal);

    // ── Main layout ──────────────────────────────────────────────────────────
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(header);
    mainLayout->addWidget(_listWidget);

    header->setFixedHeight(header->sizeHint().height());

    const int lineHeight = QFontMetrics(QFont("Fira Code")).lineSpacing() * 2;
    setMinimumHeight(header->height() + lineHeight);

    // ── Connections ──────────────────────────────────────────────────────────
    connect(_clearButton,  &QToolButton::clicked,  this, &ConsoleOutput::clear);
    connect(collapseButton, &QToolButton::clicked, this, &ConsoleOutput::collapse);
    connect(_filterLog,    &QToolButton::toggled,  this, &ConsoleOutput::applyFilters);
    connect(_filterWarn,   &QToolButton::toggled,  this, &ConsoleOutput::applyFilters);
    connect(_filterError,  &QToolButton::toggled,  this, &ConsoleOutput::applyFilters);
    connect(_listWidget, &QWidget::customContextMenuRequested,
            this, &ConsoleOutput::on_customContextMenuRequested);
}

///
/// \brief ConsoleOutput::changeEvent
///
void ConsoleOutput::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        _clearButton->setToolTip(tr("Clear console"));
}

///
/// \brief ConsoleOutput::addMessage
///
void ConsoleOutput::addMessage(const QString& text, MessageType type)
{
    auto item = new QListWidgetItem(text, _listWidget);
    item->setData(MessageTypeRole, static_cast<int>(type));
    item->setToolTip(text);

    bool visible = true;
    switch (type) {
    case MessageType::Warning:
        visible = _filterWarn->isChecked();
        _warnCount++;
        break;
    case MessageType::Error:
        visible = _filterError->isChecked();
        _errorCount++;
        break;
    default:
        visible = _filterLog->isChecked();
        _logCount++;
        break;
    }
    item->setHidden(!visible);

    updateFilterButtons();
    _listWidget->scrollToBottom();
}

///
/// \brief ConsoleOutput::clear
///
void ConsoleOutput::clear()
{
    _listWidget->clear();
    _logCount = _warnCount = _errorCount = 0;
    updateFilterButtons();
}

///
/// \brief ConsoleOutput::isEmpty
///
bool ConsoleOutput::isEmpty() const
{
    return _listWidget->count() == 0;
}

///
/// \brief ConsoleOutput::applyFilters — hide/show items per active filters
///
void ConsoleOutput::applyFilters()
{
    const bool showLog   = _filterLog->isChecked();
    const bool showWarn  = _filterWarn->isChecked();
    const bool showError = _filterError->isChecked();

    for (int i = 0; i < _listWidget->count(); ++i) {
        auto item = _listWidget->item(i);
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
    _filterLog->setText(QString("ℹ %1").arg(_logCount));
    _filterWarn->setText(QString("⚠ %1").arg(_warnCount));
    _filterError->setText(QString("✖ %1").arg(_errorCount));

    _filterLog->setVisible(_logCount > 0);
    _filterWarn->setVisible(_warnCount > 0);
    _filterError->setVisible(_errorCount > 0);
}

///
/// \brief ConsoleOutput::createToolButton
///
QToolButton* ConsoleOutput::createToolButton(QWidget* parent, const QString& text,
                                              const QIcon& icon, const QString& toolTip,
                                              const QSize& size, const QSize& iconSize)
{
    auto btn = new QToolButton(parent);
    btn->setText(text);
    btn->setIcon(icon);

    if (!iconSize.isEmpty())
        btn->setIconSize(iconSize);

    if (!size.isEmpty())
        btn->setFixedSize(size);

    btn->setToolTip(toolTip);
    btn->setAutoRaise(true);

    if (text.isEmpty() && !icon.isNull())
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    else if (!text.isEmpty() && !icon.isNull())
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    else
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);

    return btn;
}

///
/// \brief ConsoleOutput::on_customContextMenuRequested
///
void ConsoleOutput::on_customContextMenuRequested(const QPoint& pos)
{
    QMenu menu(_listWidget);

    auto copyAction = menu.addAction(tr("Copy"), this, [this]() {
        QStringList lines;
        for (auto* item : _listWidget->selectedItems())
            lines << item->text();
        if (!lines.isEmpty())
            QApplication::clipboard()->setText(lines.join('\n'));
    });
    copyAction->setEnabled(!_listWidget->selectedItems().isEmpty());

    menu.addSeparator();

    auto clearAction = menu.addAction(tr("Clear"), this, [this]() { clear(); });
    clearAction->setEnabled(!isEmpty());

    menu.exec(_listWidget->mapToGlobal(pos));
}
