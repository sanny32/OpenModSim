#include <QMenu>
#include <QEvent>
#include <QClipboard>
#include <QPainter>
#include <QTextLayout>
#include <QApplication>
#include <QStyledItemDelegate>
#include "fontutils.h"
#include "modbuslogwidget.h"

namespace {

constexpr int HorizontalPadding = 2;
constexpr int VerticalPadding   = 3;

static QString buildLogLayout(const ModbusMessage& msg,
                              const ModbusLogWidget* widget,
                              const QStyleOptionViewItem& opt,
                              QList<QTextLayout::FormatRange>& formats)
{
    const QString timestamp = msg.timestamp().toString(Qt::ISODateWithMs);
    const QString direction = msg.isRequest() ? "[Rx] \u2190" : "[Tx] \u2192";
    const QString data      = widget
                              ? msg.toString(widget->dataDisplayMode(), widget->showLeadingZeros())
                              : msg.toString(DataDisplayMode::Hex, true);

    const bool selected = opt.state & QStyle::State_Selected;

    QColor tsColor, dirColor, dataColor;
    if (selected) {
        const QPalette::ColorGroup group = (opt.state & QStyle::State_Active)
                                           ? QPalette::Active : QPalette::Inactive;
        const QColor fg = opt.palette.color(group, QPalette::HighlightedText);
        tsColor = dirColor = dataColor = fg;
    } else {
        tsColor   = QColor(0x44, 0x44, 0x44);
        dirColor  = msg.isRequest() ? QColor(0x00, 0x99, 0x33) : QColor(0x00, 0x66, 0xcc);
        dataColor = (msg.isException() || !msg.isValid()) ? QColor(0xcc, 0x00, 0x00)
                                                          : QColor(0x00, 0x00, 0x00);
    }

    const QString sep  = " ";
    const QString full = timestamp + sep + direction + sep + data;

    const int tsEnd     = timestamp.length();
    const int dirStart  = tsEnd + sep.length();
    const int dirEnd    = dirStart + direction.length();
    const int dataStart = dirEnd + sep.length();

    auto makeRange = [](int start, int len, const QColor& color) {
        QTextLayout::FormatRange r;
        r.start  = start;
        r.length = len;
        r.format.setForeground(color);
        return r;
    };

    auto dirRange = makeRange(dirStart, direction.length(), dirColor);
    dirRange.format.setFontWeight(QFont::Bold);

    formats.append(makeRange(0,         tsEnd,           tsColor));
    formats.append(dirRange);
    formats.append(makeRange(dataStart, data.length(),   dataColor));

    return full;
}

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

///
/// \brief The ModbusLogDelegate class
/// Lightweight delegate for ModbusLogWidget.
/// Paints colored text segments directly via QTextLayout (word-wrap aware),
/// avoiding QTextDocument/HTML parsing on every repaint.
///
class ModbusLogDelegate : public QStyledItemDelegate
{
public:
    explicit ModbusLogDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

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

        const auto* widget = qobject_cast<const ModbusLogWidget*>(parent());

        QList<QTextLayout::FormatRange> formats;
        const QString text = buildLogLayout(*msg, widget, opt, formats);

        QTextLayout layout(text, opt.font);
        layout.setFormats(formats);

        QTextOption textOption;
        textOption.setWrapMode(QTextOption::WordWrap);
        layout.setTextOption(textOption);

        const qreal lineWidth = opt.rect.width() - HorizontalPadding * 2;
        doLayout(layout, lineWidth);

        painter->save();
        painter->setClipRect(opt.rect, Qt::ReplaceClip);
        layout.draw(painter, QPointF(opt.rect.left() + HorizontalPadding,
                                     opt.rect.top()  + VerticalPadding));
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

        const auto* widget = qobject_cast<const ModbusLogWidget*>(parent());

        QList<QTextLayout::FormatRange> formats;
        const QString text = buildLogLayout(*msg, widget, option, formats);

        QTextLayout layout(text, option.font);
        layout.setFormats(formats);

        QTextOption textOption;
        textOption.setWrapMode(QTextOption::WordWrap);
        layout.setTextOption(textOption);

        const qreal lineWidth = option.rect.width() > 0
                                ? option.rect.width() - HorizontalPadding * 2
                                : 9999.0;
        const qreal contentH = doLayout(layout, lineWidth);

        return QSize(option.rect.width(),
                     static_cast<int>(contentH) + VerticalPadding * 2);
    }
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
