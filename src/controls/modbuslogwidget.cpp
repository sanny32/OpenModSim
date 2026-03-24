#include <QMenu>
#include <QEvent>
#include <QClipboard>
#include <QPainter>
#include <QTextLayout>
#include <QApplication>
#include <QStyledItemDelegate>
#include <QMap>
#include "fontutils.h"
#include "modbuslogwidget.h"

namespace {

constexpr int HorizontalPadding = 2;
constexpr int VerticalPadding   = 3;
constexpr int LayoutCacheLimit  = 4096;

static qreal doLayout(QTextLayout& layout, qreal lineWidth)
{
    layout.beginLayout();
    qreal y = 0;
    while (true) {
        QTextLine line = layout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(lineWidth);
        line.setPosition(QPointF(0, y));
        y += line.height();
    }
    layout.endLayout();
    return y;
}

struct LogTextData
{
    QString text;
    int timestampLen = 0;
    int directionStart = 0;
    int directionLen = 0;
    int dataStart = 0;
};

struct LayoutCacheKey
{
    quintptr messagePtr = 0;
    quint64 messageSignature = 0;
    QString fontKey;
    int lineWidth = 0;
    quint32 modeKey = 0;

    bool operator<(const LayoutCacheKey& other) const noexcept
    {
        if (messagePtr != other.messagePtr) return messagePtr < other.messagePtr;
        if (messageSignature != other.messageSignature) return messageSignature < other.messageSignature;
        if (fontKey != other.fontKey)       return fontKey < other.fontKey;
        if (lineWidth != other.lineWidth)   return lineWidth < other.lineWidth;
        return modeKey < other.modeKey;
    }
};

struct CachedLayoutEntry
{
    explicit CachedLayoutEntry(const LogTextData& data, const QFont& font, qreal lineWidth)
        : textData(data)
        , layout(textData.text, font)
    {
        QTextOption textOption;
        textOption.setWrapMode(QTextOption::WordWrap);
        layout.setTextOption(textOption);
        height = doLayout(layout, lineWidth);
    }

    LogTextData textData;
    QTextLayout layout;
    qreal height = 0;
};

static quint32 makeModeKey(const ModbusLogWidget* widget)
{
    if (!widget) {
        return (static_cast<quint32>(DataDisplayMode::Hex) << 1u) | 1u;
    }

    return (static_cast<quint32>(widget->dataDisplayMode()) << 1u)
           | (widget->showLeadingZeros() ? 1u : 0u);
}

static quint64 makeMessageSignature(const ModbusMessage& msg)
{
    const quint64 ts = static_cast<quint64>(msg.timestamp().toMSecsSinceEpoch());
    const quint64 req = msg.isRequest() ? (1ull << 63) : 0ull;
    const quint64 fc = static_cast<quint64>(msg.functionCode()) << 48;
    const quint64 dev = static_cast<quint64>(msg.deviceId() & 0xFF) << 40;
    const quint64 proto = static_cast<quint64>(msg.protocolType()) << 32;
    const quint64 rawHash = static_cast<quint64>(qHash(msg.rawData()));
    return ts ^ req ^ fc ^ dev ^ proto ^ rawHash;
}

static LogTextData buildLogTextData(const ModbusMessage& msg, const ModbusLogWidget* widget)
{
    const QString timestamp = msg.timestamp().toString(Qt::ISODateWithMs);
    const QString direction = msg.isRequest() ? QStringLiteral("[Rx] \u2190")
                                              : QStringLiteral("[Tx] \u2192");
    const QString data = widget
                         ? msg.toString(widget->dataDisplayMode(), widget->showLeadingZeros())
                         : msg.toString(DataDisplayMode::Hex, true);

    LogTextData result;
    result.timestampLen  = timestamp.length();
    result.directionStart = result.timestampLen + 1;
    result.directionLen   = direction.length();
    result.dataStart      = result.directionStart + result.directionLen + 1;

    result.text.reserve(result.dataStart + data.length());
    result.text += timestamp;
    result.text += QLatin1Char(' ');
    result.text += direction;
    result.text += QLatin1Char(' ');
    result.text += data;
    return result;
}

static QList<QTextLayout::FormatRange> buildLogFormats(const ModbusMessage& msg,
                                                       const QStyleOptionViewItem& opt,
                                                       const LogTextData& textData)
{
    static const QColor TsColor(0x44, 0x44, 0x44);
    static const QColor RxColor(0x00, 0x99, 0x33);
    static const QColor TxColor(0x00, 0x66, 0xcc);
    static const QColor ErrorColor(0xcc, 0x00, 0x00);
    static const QColor NormalDataColor(0x00, 0x00, 0x00);

    const bool selected = opt.state & QStyle::State_Selected;
    const int textLen = textData.text.length();
    const int dataLen = qMax(0, textLen - textData.dataStart);

    auto makeRange = [](int start, int len, const QColor& color) {
        QTextLayout::FormatRange r;
        r.start  = start;
        r.length = len;
        r.format.setForeground(color);
        return r;
    };

    QList<QTextLayout::FormatRange> formats;
    if (selected) {
        const QPalette::ColorGroup group = (opt.state & QStyle::State_Active)
                                           ? QPalette::Active
                                           : QPalette::Inactive;
        const QColor fg = opt.palette.color(group, QPalette::HighlightedText);

        formats.reserve(2);
        formats.append(makeRange(0, textLen, fg));

        auto dirRange = makeRange(textData.directionStart, textData.directionLen, fg);
        dirRange.format.setFontWeight(QFont::Bold);
        formats.append(dirRange);
        return formats;
    }

    formats.reserve(3);
    formats.append(makeRange(0, textData.timestampLen, TsColor));

    auto dirRange = makeRange(textData.directionStart,
                              textData.directionLen,
                              msg.isRequest() ? RxColor : TxColor);
    dirRange.format.setFontWeight(QFont::Bold);
    formats.append(dirRange);

    const QColor dataColor = (msg.isException() || !msg.isValid()) ? ErrorColor : NormalDataColor;
    formats.append(makeRange(textData.dataStart, dataLen, dataColor));
    return formats;
}

///
/// \brief The ModbusLogDelegate class
/// Lightweight delegate for ModbusLogWidget.
/// Paints colored text segments directly via QTextLayout (word-wrap aware),
/// avoiding QTextDocument/HTML parsing on every repaint.
///
class ModbusLogDelegate : public QStyledItemDelegate
{
public:
    explicit ModbusLogDelegate(ModbusLogWidget* parent = nullptr)
        : QStyledItemDelegate(parent)
        , _widget(parent)
    {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        const QStyleOptionViewItem opt(option);

        {
            const bool selected = opt.state & QStyle::State_Selected;
            const QPalette::ColorGroup cg = (opt.state & QStyle::State_Active)
                                            ? QPalette::Active : QPalette::Inactive;
            if (selected)
                painter->fillRect(opt.rect, opt.palette.brush(cg, QPalette::Highlight));
            else if (opt.features & QStyleOptionViewItem::Alternate)
                painter->fillRect(opt.rect, opt.palette.brush(cg, QPalette::AlternateBase));
            else
                painter->fillRect(opt.rect, opt.palette.brush(cg, QPalette::Base));
        }

        const auto msg = index.data(Qt::UserRole).value<QSharedPointer<const ModbusMessage>>();
        if (!msg)
            return;

        const int lineWidth = qMax(1, opt.rect.width() - HorizontalPadding * 2);
        const auto& cachedLayout = ensureLayout(*msg, opt.font, lineWidth);
        const QList<QTextLayout::FormatRange> formats = buildLogFormats(*msg, opt, cachedLayout.textData);

        painter->save();
        painter->setClipRect(opt.rect, Qt::ReplaceClip);
        cachedLayout.layout.draw(painter,
                                 QPointF(opt.rect.left() + HorizontalPadding,
                                         opt.rect.top()  + VerticalPadding),
                                 formats);
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override
    {
        const auto msg = index.data(Qt::UserRole).value<QSharedPointer<const ModbusMessage>>();
        if (!msg) {
            const QFontMetrics fm(option.font);
            return QSize(option.rect.width(), fm.height() + VerticalPadding * 2);
        }

        const int lineWidth = option.rect.width() > 0
                              ? qMax(1, option.rect.width() - HorizontalPadding * 2)
                              : 9999;
        const auto& cachedLayout = ensureLayout(*msg, option.font, lineWidth);

        return QSize(option.rect.width(),
                     static_cast<int>(cachedLayout.height) + VerticalPadding * 2);
    }

private:
    const CachedLayoutEntry& ensureLayout(const ModbusMessage& msg,
                                          const QFont& font,
                                          int lineWidth) const
    {
        const LayoutCacheKey key{
            reinterpret_cast<quintptr>(&msg),
            makeMessageSignature(msg),
            font.key(),
            lineWidth,
            makeModeKey(_widget)
        };

        auto it = _layoutCache.constFind(key);
        if (it != _layoutCache.constEnd())
            return *it.value();

        if (_layoutCache.size() >= LayoutCacheLimit)
            _layoutCache.clear();

        auto cached = QSharedPointer<CachedLayoutEntry>::create(
            buildLogTextData(msg, _widget), font, lineWidth);
        _layoutCache.insert(key, cached);
        return *cached;
    }

private:
    const ModbusLogWidget* _widget = nullptr;
    mutable QMap<LayoutCacheKey, QSharedPointer<CachedLayoutEntry>> _layoutCache;
};

} // namespace

///
/// \brief ModbusLogModel::ModbusLogModel
/// \param parent
///
ModbusLogModel::ModbusLogModel(ModbusLogWidget* parent)
    : BufferingListModel(parent)
    ,_parentWidget(parent)
{
}

///
/// \brief ModbusLogModel::data
/// \param index
/// \param role
/// \return
///
QVariant ModbusLogModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row() >= rowCount())
        return QVariant();

    const auto item = itemAt(index.row());
    switch(role)
    {
        case Qt::DisplayRole:
            return QString("%1 %2 %3")
                .arg(item->timestamp().toString(Qt::ISODateWithMs),
                     item->isRequest() ? "[Rx] \u2190" : "[Tx] \u2192",
                     item->toString(_parentWidget->dataDisplayMode(), _parentWidget->showLeadingZeros()));

        case Qt::UserRole:
            return QVariant::fromValue(item);
    }

    return QVariant();
}

///
/// \brief ModbusLogWidget::ModbusLogWidget
/// \param parent
///
ModbusLogWidget::ModbusLogWidget(QWidget* parent)
    : QListView(parent)
    , _autoscroll(false)
    , _showLeadingZeros(true)
{
    setFocusPolicy(Qt::StrongFocus);
    setFont(defaultMonospaceFont());
    setContextMenuPolicy(Qt::CustomContextMenu);
    setItemDelegate(new ModbusLogDelegate(this));
    setModel(new ModbusLogModel(this));

    _copyAct = new QAction(QIcon(":/res/actionCopy.png"), tr("Copy Text"), this);
    _copyAct->setShortcut(QKeySequence::Copy);
    _copyAct->setShortcutContext(Qt::WidgetShortcut);
    _copyAct->setShortcutVisibleInContextMenu(true);
    addAction(_copyAct);

    connect(_copyAct, &QAction::triggered, this, [this]() {
        const QModelIndex index = currentIndex();
        if (index.isValid())
            QApplication::clipboard()->setText(index.data(Qt::DisplayRole).toString());
    });

    _copyBytesAct = new QAction(tr("Copy Bytes"), this);
    _copyBytesAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    _copyBytesAct->setShortcutContext(Qt::WidgetShortcut);
    _copyBytesAct->setShortcutVisibleInContextMenu(true);
    addAction(_copyBytesAct);

    connect(_copyBytesAct, &QAction::triggered, this, [this]() {
        QModelIndex index = currentIndex();
        if (index.isValid()) {
            auto msg = index.data(Qt::UserRole).value<QSharedPointer<const ModbusMessage>>();
            if (msg) QApplication::clipboard()->setText(msg->toString(dataDisplayMode(), _showLeadingZeros));
        }
    });

    connect(this, &QWidget::customContextMenuRequested,
            this, &ModbusLogWidget::on_customContextMenuRequested);
    connect(model(), &ModbusLogModel::rowsInserted,
            this, [&]{
        if(_autoscroll) scrollToBottom();
    });
}

///
/// \brief ModbusLogWidget::changeEvent
/// \param event
///
void ModbusLogWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        update();
        _copyAct->setText(tr("Copy Text"));
        _copyBytesAct->setText(tr("Copy Bytes"));
    }
    QListView::changeEvent(event);
}

///
/// \brief ModbusLogWidget::clear
///
void ModbusLogWidget::clear()
{
    if(model())
        ((ModbusLogModel*)model())->clear();
}

///
/// \brief ModbusLogWidget::rowCount
/// \return
///
int ModbusLogWidget::rowCount() const
{
    return model() ? model()->rowCount() : 0;
}

///
/// \brief ModbusLogWidget::index
/// \param row
/// \return
///
QModelIndex ModbusLogWidget::index(int row)
{
    return model() ? model()->index(row, 0) : QModelIndex();
}

///
/// \brief ModbusLogWidget::addItem
/// \param msg
///
void ModbusLogWidget::addItem(QSharedPointer<const ModbusMessage> msg)
{
    if(model()) {
        ((ModbusLogModel*)model())->append(msg);
    }
}

///
/// \brief ModbusLogWidget::addItems
/// \param messages
///
void ModbusLogWidget::addItems(const QVector<QSharedPointer<const ModbusMessage>>& messages)
{
    if (!model() || messages.isEmpty())
        return;

    ((ModbusLogModel*)model())->appendBatch(messages);
}

///
/// \brief ModbusLogWidget::itemAt
/// \param index
/// \return
///
QSharedPointer<const ModbusMessage> ModbusLogWidget::itemAt(const QModelIndex& index)
{
    if(!index.isValid())
        return nullptr;

    return model() ?
           model()->data(index, Qt::UserRole).value<QSharedPointer<const ModbusMessage>>() :
           nullptr;
}

///
/// \brief ModbusLogWidget::exportToTextFile
/// \param filePath
/// \return
///
bool ModbusLogWidget::exportToTextFile(const QString& filePath)
{
    if (!model())
        return false;

    return ((ModbusLogModel*)model())->exportToTextFile(filePath, [this](const QSharedPointer<const ModbusMessage>& msg) {
        if (!msg)
            return QString();

        const QString direction = msg->isRequest() ? QStringLiteral("[Rx]") : QStringLiteral("[Tx]");
        return QString("%1 %2 %3").arg(
            msg->timestamp().toString(Qt::ISODateWithMs),
            direction,
            msg->toString(dataDisplayMode(), showLeadingZeros()));
    });
}

///
/// \brief ModbusLogWidget::dataDisplayMode
/// \return
///
DataDisplayMode ModbusLogWidget::dataDisplayMode() const
{
    return _dataDisplayMode;
}

///
/// \brief ModbusLogWidget::setDataDisplayMode
/// \param mode
///
void ModbusLogWidget::setDataDisplayMode(DataDisplayMode mode)
{
    _dataDisplayMode = mode;

    if(model()) {
        ((ModbusLogModel*)model())->update();
    }
}

///
/// \brief ModbusLogWidget::showLeadingZeros
/// \return
///
bool ModbusLogWidget::showLeadingZeros() const
{
    return _showLeadingZeros;
}

///
/// \brief ModbusLogWidget::setShowLeadingZeros
/// \param value
///
void ModbusLogWidget::setShowLeadingZeros(bool value)
{
    _showLeadingZeros = value;
    if(model()) {
        ((ModbusLogModel*)model())->update();
    }
}

///
/// \brief ModbusLogWidget::rowLimit
/// \return
///
int ModbusLogWidget::rowLimit() const
{
    return model() ? ((ModbusLogModel*)model())->rowLimit() : 0;
}

///
/// \brief ModbusLogWidget::setRowLimit
/// \param val
///
void ModbusLogWidget::setRowLimit(int val)
{
    if(model()) {
        ((ModbusLogModel*)model())->setRowLimit(val);
    }
}

///
/// \brief ModbusLogWidget::autoscroll
/// \return
///
bool ModbusLogWidget::autoscroll() const
{
    return _autoscroll;
}

///
/// \brief ModbusLogWidget::setAutoscroll
/// \param on
///
void ModbusLogWidget::setAutoscroll(bool on)
{
    _autoscroll = on;
}

///
/// \brief ModbusLogWidget::backgroundColor
/// \return
///
QColor ModbusLogWidget::backgroundColor() const
{
    return palette().color(QPalette::Base);
}

///
/// \brief ModbusLogWidget::setBackGroundColor
/// \param clr
///
void ModbusLogWidget::setBackGroundColor(const QColor& clr)
{
    auto pal = palette();
    pal.setColor(QPalette::Base, clr);
    pal.setColor(QPalette::Window, clr);
    setPalette(pal);
}

///
/// \brief ModbusLogWidget::state
/// \return
///
LogViewState ModbusLogWidget::state() const
{
    return _state;
}

///
/// \brief ModbusLogWidget::setState
/// \param state
///
void ModbusLogWidget::setState(LogViewState state)
{
    _state = state;
    switch (state) {
        case LogViewState::Paused:
            ((ModbusLogModel*)model())->setBufferingMode(true);
        break;

        case LogViewState::Running:
            ((ModbusLogModel*)model())->setBufferingMode(false);
        break;

        default:
        break;
    }
}

///
/// \brief ModbusLogWidget::on_customContextMenuRequested
/// \param pos
///
void ModbusLogWidget::on_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(_copyAct);
    menu.addAction(_copyBytesAct);
    menu.exec(viewport()->mapToGlobal(pos));
}
