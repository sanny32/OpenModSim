#ifndef APPTRACE_H
#define APPTRACE_H

#include <QByteArray>
#include <QDebug>
#include <QElapsedTimer>
#include <QEvent>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QWidget>

namespace AppTrace
{

inline bool isEnabled()
{
    static const bool enabled = []() {
        const QByteArray raw = qgetenv("OMODSIM_APP_TRACE").trimmed().toLower();
        return !raw.isEmpty() && raw != "0" && raw != "false" && raw != "off" && raw != "no";
    }();
    return enabled;
}

inline qint64 elapsedMs()
{
    static QElapsedTimer timer;
    static const bool started = []() {
        timer.start();
        return true;
    }();
    Q_UNUSED(started)
    return timer.elapsed();
}

inline QString pointerText(const void* ptr)
{
    return QStringLiteral("0x%1")
        .arg(reinterpret_cast<quintptr>(ptr), QT_POINTER_SIZE * 2, 16, QLatin1Char('0'));
}

inline QString objectTag(const QObject* obj)
{
    if (!obj)
        return QStringLiteral("null");

    QString type = QString::fromLatin1(obj->metaObject()->className());
    if (!obj->objectName().isEmpty())
        type += QStringLiteral("(#%1)").arg(obj->objectName());
    return QStringLiteral("%1@%2").arg(type, pointerText(obj));
}

inline QString widgetTag(const QWidget* widget)
{
    if (!widget)
        return QStringLiteral("null");

    QString tag = objectTag(widget);
    const QString title = widget->windowTitle();
    if (!title.isEmpty()) {
        QString normalized = title;
        normalized.replace(QLatin1Char('\n'), QLatin1Char(' '));
        tag += QStringLiteral(" title=\"%1\"").arg(normalized);
    }
    return tag;
}

inline QString subWindowTag(const QMdiSubWindow* wnd)
{
    if (!wnd)
        return QStringLiteral("null");

    const QWidget* child = wnd->widget();
    QString tag = widgetTag(wnd);
    tag += QStringLiteral(" child=%1").arg(widgetTag(child));
    return tag;
}

inline QString eventTypeName(QEvent::Type type)
{
    switch (type) {
        case QEvent::FocusIn: return QStringLiteral("FocusIn");
        case QEvent::FocusOut: return QStringLiteral("FocusOut");
        case QEvent::WindowActivate: return QStringLiteral("WindowActivate");
        case QEvent::WindowDeactivate: return QStringLiteral("WindowDeactivate");
        case QEvent::MouseButtonPress: return QStringLiteral("MouseButtonPress");
        case QEvent::MouseButtonRelease: return QStringLiteral("MouseButtonRelease");
        case QEvent::KeyPress: return QStringLiteral("KeyPress");
        case QEvent::KeyRelease: return QStringLiteral("KeyRelease");
        case QEvent::ShortcutOverride: return QStringLiteral("ShortcutOverride");
        case QEvent::Close: return QStringLiteral("Close");
        case QEvent::Hide: return QStringLiteral("Hide");
        case QEvent::Show: return QStringLiteral("Show");
        default: return QStringLiteral("Type(%1)").arg(int(type));
    }
}

inline QString mdiAreaState(const QMdiArea* area)
{
    if (!area)
        return QStringLiteral("area=null");

    const auto windows = area->subWindowList();
    QStringList titles;
    titles.reserve(windows.size());
    for (auto* wnd : windows) {
        if (!wnd) {
            titles << QStringLiteral("<null>");
            continue;
        }
        if (auto* widget = wnd->widget())
            titles << widget->windowTitle();
        else
            titles << wnd->windowTitle();
    }

    return QStringLiteral("%1 count=%2 active=%3 current=%4 tabs=[%5]")
        .arg(objectTag(area))
        .arg(windows.size())
        .arg(subWindowTag(area->activeSubWindow()))
        .arg(subWindowTag(area->currentSubWindow()))
        .arg(titles.join(QStringLiteral(", ")));
}

inline void log(const char* scope, const QString& message)
{
    if (!isEnabled())
        return;

    qInfo().noquote()
        << QStringLiteral("[APP +%1ms] %2 | %3")
               .arg(elapsedMs())
               .arg(QString::fromLatin1(scope))
               .arg(message);
}

} // namespace AppTrace

#endif // APPTRACE_H
