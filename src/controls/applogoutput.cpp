#include <QApplication>
#include <atomic>
#include <QClipboard>
#include <QGuiApplication>
#include <QStyleHints>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QPointer>
#include <QSizePolicy>
#include <QStyledItemDelegate>
#include <QToolBar>
#include <QToolButton>
#include "applogoutput.h"
#include "themedicons.h"
#include "ui_applogoutput.h"

Q_LOGGING_CATEGORY(lcApp, "omodsim")

namespace {
static const int EventTypeRole = Qt::UserRole;

///
/// \brief The EventStyle class
///
struct EventStyle {
    QColor bg;
    QColor border;
    QColor textColor;
    QString iconPath;
};

///
/// \brief styleForType
/// \param type
/// \return
///
EventStyle styleForType(AppLogOutput::EventType type)
{
    const bool dark = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    switch (type) {
        case AppLogOutput::EventType::Warning:
            return dark
                ? EventStyle{ QColor("#3a2e00"), QColor("#F9A825"), QColor("#FFD54F"), QStringLiteral(":/res/icon-log-warning.svg") }
                : EventStyle{ QColor("#FFF8E1"), QColor("#F9A825"), QColor("#4A3000"), QStringLiteral(":/res/icon-log-warning.svg") };
        case AppLogOutput::EventType::Error:
            return dark
                ? EventStyle{ QColor("#3b0000"), QColor("#E53935"), QColor("#FF8A80"), QStringLiteral(":/res/icon-log-critical.svg") }
                : EventStyle{ QColor("#FFEBEE"), QColor("#E53935"), QColor("#7F0000"), QStringLiteral(":/res/icon-log-critical.svg") };
        default:
            return dark
                ? EventStyle{ QColor("#1c1c1e"), QColor(), QColor("#abb2bf"), QStringLiteral(":/res/icon-log-info.svg") }
                : EventStyle{ Qt::white,         QColor(), QColor("#202020"), QStringLiteral(":/res/icon-log-info.svg") };
    }
}

///
/// \brief iconForType
/// \param type
/// \return
///
QIcon iconForType(AppLogOutput::EventType type)
{
    switch (type) {
        case AppLogOutput::EventType::Warning:
            return themedIcon(QStringLiteral("omodsim/warning"));
        case AppLogOutput::EventType::Error:
            return themedIcon(QStringLiteral("omodsim/error"));
        case AppLogOutput::EventType::Info:
        default:
            return themedIcon(QStringLiteral("omodsim/information"));
    }
}

///
/// \brief The AppLogItemDelegate class
///
class AppLogItemDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        const auto type = static_cast<AppLogOutput::EventType>(index.data(EventTypeRole).toInt());
        const auto style = styleForType(type);

        painter->save();

        painter->fillRect(option.rect, style.bg);

        if (style.border.isValid())
            painter->fillRect(option.rect.left(), option.rect.top(), 3, option.rect.height(), style.border);

        if (option.state & QStyle::State_Selected) {
            QColor sel = option.palette.highlight().color();
            sel.setAlpha(55);
            painter->fillRect(option.rect, sel);
        }

        constexpr int leftPad = 8;
        constexpr int iconW = 16;
        const bool hasIcon = !style.iconPath.isEmpty();
        const int textLeft = leftPad + (hasIcon ? iconW + 4 : 0);
        const QRect textRect = option.rect.adjusted(textLeft, 2, -6, -2);

        if (hasIcon) {
            const QRect iconRect(option.rect.left() + leftPad,
                                 option.rect.top() + (option.rect.height() - iconW) / 2,
                                 iconW,
                                 iconW);
            QIcon(style.iconPath).paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal, QIcon::On);
        }

        painter->setPen(style.textColor);
        painter->setFont(option.font);
        const QString text = index.data(Qt::DisplayRole).toString();
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine, text);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        const auto type = static_cast<AppLogOutput::EventType>(index.data(EventTypeRole).toInt());
        const auto style = styleForType(type);

        constexpr int leftPad = 8;
        constexpr int iconW = 16;
        constexpr int rightPad = 6;
        const bool hasIcon = !style.iconPath.isEmpty();
        const int textLeft = leftPad + (hasIcon ? iconW + 4 : 0);
        const QString text = index.data(Qt::DisplayRole).toString();
        const int textWidth = option.fontMetrics.horizontalAdvance(text);

        return { textLeft + textWidth + rightPad, option.fontMetrics.height() + 4 };
    }
};
} // namespace

static AppLogOutput*   s_instance    = nullptr;
static QtMessageHandler s_prevHandler = nullptr;
static std::atomic<quint64> s_logEpoch { 0 };

///
/// \brief appMessageHandler
/// \param type
/// \param ctx
/// \param msg
///
static void appMessageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
    if (s_prevHandler) s_prevHandler(type, ctx, msg);
    if (!s_instance || qstrcmp(ctx.category, "omodsim") != 0) return;

    AppLogOutput::EventType evType;
    switch (type) {
        case QtWarningMsg:  evType = AppLogOutput::EventType::Warning; break;
        case QtCriticalMsg:
        case QtFatalMsg:    evType = AppLogOutput::EventType::Error;   break;
        default:            evType = AppLogOutput::EventType::Info;    break;
    }
    QString text = msg;
    if (text.length() >= 2 && text.startsWith('"') && text.endsWith('"'))
        text = text.mid(1, text.length() - 2);
    text.prepend(QStringLiteral("[") + QDateTime::currentDateTime().toString(Qt::ISODateWithMs) + QStringLiteral("] "));

    const quint64 epoch = s_logEpoch.load(std::memory_order_relaxed);
    QPointer<AppLogOutput> guard(s_instance);
    QMetaObject::invokeMethod(s_instance, [guard, text, evType, epoch]() {
        if (guard && s_logEpoch.load(std::memory_order_relaxed) == epoch)
            guard->addEvent(text, evType);
    }, Qt::QueuedConnection);
}

///
/// \brief AppLogOutput::AppLogOutput
///
AppLogOutput::AppLogOutput(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::AppLogOutput)
{
    ui->setupUi(this);

    auto setupToolbarBtn = [&](QAction* action) {
        auto* btn = qobject_cast<QToolButton*>(ui->toolBar->widgetForAction(action));
        if (!btn) return;
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        btn->setFixedSize(24, 24);
        btn->setProperty("preservePressedIconAlignment", true);
    };

    setupToolbarBtn(ui->actionClear);
    setupToolbarBtn(ui->actionExport);
    setupToolbarBtn(ui->actionFilterInfo);
    setupToolbarBtn(ui->actionFilterWarn);
    setupToolbarBtn(ui->actionFilterError);

    auto* filterSpacer = new QWidget(ui->toolBar);
    filterSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->toolBar->insertWidget(ui->actionFilterInfo, filterSpacer);

    ui->actionClear->setIcon(themedIcon(QStringLiteral("omodsim/clear")));
    ui->actionExport->setIcon(themedIcon(QStringLiteral("omodsim/export")));

    ui->listWidget->setItemDelegate(new AppLogItemDelegate(ui->listWidget));
    ui->listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->listWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->listWidget->setTextElideMode(Qt::ElideNone);

    const int lineHeight = QFontMetrics(QFont("Fira Code")).lineSpacing() * 2;
    setMinimumHeight(ui->toolBar->sizeHint().height() + lineHeight);

    connect(ui->actionClear,       &QAction::triggered, this, &AppLogOutput::clear);
    connect(ui->actionExport,      &QAction::triggered, this, &AppLogOutput::exportLog);
    connect(ui->actionFilterInfo,  &QAction::toggled,   this, &AppLogOutput::applyFilters);
    connect(ui->actionFilterWarn,  &QAction::toggled,   this, &AppLogOutput::applyFilters);
    connect(ui->actionFilterError, &QAction::toggled,   this, &AppLogOutput::applyFilters);
    connect(ui->listWidget, &QWidget::customContextMenuRequested,
            this, &AppLogOutput::on_customContextMenuRequested);

    _copyAllAction = new QAction(tr("Copy All"), this);
    _copyAllAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    _copyAllAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(_copyAllAction);
    connect(_copyAllAction, &QAction::triggered, this, &AppLogOutput::copyAllToClipboard);

    s_instance = this;
    s_prevHandler = qInstallMessageHandler(appMessageHandler);
}

///
/// \brief AppLogOutput::~AppLogOutput
///
AppLogOutput::~AppLogOutput()
{
    qInstallMessageHandler(s_prevHandler);
    s_instance    = nullptr;
    s_prevHandler = nullptr;
    delete ui;
}

///
/// \brief AppLogOutput::instance
/// \return
///
AppLogOutput* AppLogOutput::instance()
{
    return s_instance;
}

///
/// \brief AppLogOutput::changeEvent
///
void AppLogOutput::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);
    QWidget::changeEvent(event);
}

///
/// \brief AppLogOutput::setMaxLines
///
void AppLogOutput::setMaxLines(int n)
{
    _maxLines = qMax(1, n);
    while (ui->listWidget->count() > _maxLines) {
        const auto evictType = static_cast<EventType>(ui->listWidget->item(0)->data(EventTypeRole).toInt());
        switch (evictType) {
            case EventType::Warning: _warnCount--;  break;
            case EventType::Error:   _errorCount--; break;
            default:                 _infoCount--;  break;
        }
        delete ui->listWidget->takeItem(0);
    }
    updateFilterButtons();
}

///
/// \brief AppLogOutput::addEvent
///
void AppLogOutput::addEvent(const QString& text, EventType type)
{
    while (ui->listWidget->count() >= _maxLines) {
        const auto evictType = static_cast<EventType>(ui->listWidget->item(0)->data(EventTypeRole).toInt());
        switch (evictType) {
            case EventType::Warning: _warnCount--;  break;
            case EventType::Error:   _errorCount--; break;
            default:                 _infoCount--;  break;
        }
        delete ui->listWidget->takeItem(0);
    }

    auto* item = new QListWidgetItem(text, ui->listWidget);
    item->setData(EventTypeRole, static_cast<int>(type));

    bool visible = true;
    switch (type) {
        case EventType::Warning:
            visible = ui->actionFilterWarn->isChecked();
            _warnCount++;
            break;
        case EventType::Error:
            visible = ui->actionFilterError->isChecked();
            _errorCount++;
            break;
        default:
            visible = ui->actionFilterInfo->isChecked();
            _infoCount++;
            break;
    }
    item->setHidden(!visible);

    updateFilterButtons();
    ui->listWidget->scrollToBottom();

    if (type != EventType::Info)
        emit openRequested();
}

///
/// \brief AppLogOutput::clear
///
void AppLogOutput::clear()
{
    s_logEpoch.fetch_add(1, std::memory_order_relaxed);
    ui->listWidget->clear();
    _infoCount = _warnCount = _errorCount = 0;
    updateFilterButtons();
}

///
/// \brief AppLogOutput::isEmpty
///
bool AppLogOutput::isEmpty() const
{
    return ui->listWidget->count() == 0;
}

///
/// \brief AppLogOutput::applyFilters
///
void AppLogOutput::applyFilters()
{
    const bool showInfo = ui->actionFilterInfo->isChecked();
    const bool showWarn = ui->actionFilterWarn->isChecked();
    const bool showError = ui->actionFilterError->isChecked();

    for (int i = 0; i < ui->listWidget->count(); ++i) {
        auto* item = ui->listWidget->item(i);
        const auto type = static_cast<EventType>(item->data(EventTypeRole).toInt());
        bool visible = true;
        switch (type) {
            case EventType::Warning: visible = showWarn;  break;
            case EventType::Error:   visible = showError; break;
            default:                 visible = showInfo;  break;
        }
        item->setHidden(!visible);
    }
}

///
/// \brief AppLogOutput::updateFilterButtons
///
void AppLogOutput::updateFilterButtons()
{
    ui->actionFilterInfo->setIcon(iconForType(EventType::Info));
    ui->actionFilterWarn->setIcon(iconForType(EventType::Warning));
    ui->actionFilterError->setIcon(iconForType(EventType::Error));

    ui->actionFilterInfo->setToolTip(QStringLiteral("Info: %1").arg(_infoCount));
    ui->actionFilterWarn->setToolTip(QStringLiteral("Warnings: %1").arg(_warnCount));
    ui->actionFilterError->setToolTip(QStringLiteral("Errors: %1").arg(_errorCount));

    ui->actionFilterInfo->setVisible(_infoCount > 0);
    ui->actionFilterWarn->setVisible(_warnCount > 0);
    ui->actionFilterError->setVisible(_errorCount > 0);
}

///
/// \brief AppLogOutput::on_customContextMenuRequested
///
void AppLogOutput::on_customContextMenuRequested(const QPoint& pos)
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

    auto copyAllAction = menu.addAction(themedIcon(QStringLiteral("omodsim/copy")), tr("Copy All"), this,
                                        &AppLogOutput::copyAllToClipboard);
    copyAllAction->setShortcut(_copyAllAction->shortcut());
    copyAllAction->setEnabled(!isEmpty());

    menu.addSeparator();

    auto exportAction = menu.addAction(themedIcon(QStringLiteral("omodsim/export")), tr("Export..."), this,
                                       &AppLogOutput::exportLog);
    exportAction->setEnabled(!isEmpty());

    menu.addSeparator();

    auto clearAction = menu.addAction(tr("Clear"), this, [this]() { clear(); });
    clearAction->setEnabled(!isEmpty());

    menu.exec(ui->listWidget->mapToGlobal(pos));
}

///
/// \brief AppLogOutput::copyAllToClipboard
///
void AppLogOutput::copyAllToClipboard()
{
    if (isEmpty()) return;
    QStringList lines;
    for (int i = 0; i < ui->listWidget->count(); ++i)
        lines << ui->listWidget->item(i)->text();
    QApplication::clipboard()->setText(lines.join('\n'));
}

///
/// \brief AppLogOutput::exportLog
///
void AppLogOutput::exportLog()
{
    const QString filename = QFileDialog::getSaveFileName(
        this, QString(), QString(), tr("Text files (*.txt)"));
    if (filename.isEmpty()) return;

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export"), tr("Cannot open file for writing:\n%1").arg(filename));
        return;
    }
    QTextStream out(&file);
    for (int i = 0; i < ui->listWidget->count(); ++i)
        out << ui->listWidget->item(i)->text() << '\n';
}
