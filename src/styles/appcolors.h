#ifndef APPCOLORS_H
#define APPCOLORS_H

#include <QApplication>
#include <array>
#include <QColor>
#include <QGuiApplication>
#include <QPalette>
#include <QStyleHints>
#include "apppreferences.h"

namespace AppColors {

enum class MarkerColorId
{
    Yellow,
    Cyan,
    Magenta,
    LightGreen,
    Orange,
    LightBlue,
    LightGray
};

inline bool isDark()
{
    switch (AppPreferences::instance().themeMode()) {
        case AppThemeMode::Light:
            return false;
        case AppThemeMode::Dark:
            return true;
        case AppThemeMode::System:
        default:
            return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    }
}

inline QColor canvasBackground()
{
    return QApplication::palette().color(QPalette::Base);
}

inline QColor windowBackground()
{
    return QApplication::palette().color(QPalette::Window);
}

inline QColor canvasForeground()
{
    return QApplication::palette().color(QPalette::Text);
}

inline QColor windowForeground()
{
    return QApplication::palette().color(QPalette::WindowText);
}

inline QColor alternateBackground()
{
    return QApplication::palette().color(QPalette::AlternateBase);
}

inline QColor mutedForeground()
{
    return QApplication::palette().color(QPalette::PlaceholderText);
}

inline QColor divider()
{
    return QApplication::palette().color(QPalette::Mid);
}

inline QColor searchMatchBackground()
{
    const QColor h = QApplication::palette().color(QPalette::Highlight);
    return h.lighter(isDark() ? 60 : 160);
}

inline QColor searchCurrentMatchBackground()
{
    return QApplication::palette().color(QPalette::Highlight);
}

inline QColor lineNumberAreaBackground()
{
    return QApplication::palette().color(QPalette::AlternateBase);
}

inline QColor lineNumberColor()
{
    return QApplication::palette().color(QPalette::PlaceholderText);
}

inline QColor defaultAddress()
{
    return QApplication::palette().color(QPalette::PlaceholderText);
}

// Semantic log colors — warning
inline QColor warningBackground()
{
    return isDark() ? QColor("#3a2e00") : QColor("#FFF8E1");
}

inline QColor warningBorder()
{
    return QColor("#F9A825");
}

inline QColor warningForeground()
{
    return isDark() ? QColor("#FFD54F") : QColor("#4A3000");
}

// Semantic log colors — error
inline QColor errorBackground()
{
    return isDark() ? QColor("#3b0000") : QColor("#FFEBEE");
}

inline QColor errorBorder()
{
    return QColor("#E53935");
}

inline QColor errorForeground()
{
    return isDark() ? QColor("#FF8A80") : QColor("#7F0000");
}

// Semantic log colors — debug/info
inline QColor debugForeground()
{
    return isDark() ? QColor("#56B6C2") : QColor("#37474F");
}

inline QColor logForeground()
{
    return isDark() ? QColor("#abb2bf") : QApplication::palette().color(QPalette::Text);
}

// Status indicator colors
inline QColor statusWarningColor()
{
    return QColor("#f97316");
}

inline QColor statusOkColor()
{
    return QColor("#22c55e");
}

// Modbus message HTML colors
inline QColor modbusErrorColor()
{
    return isDark() ? QColor("#ff6b6b") : QColor("#cc0000");
}

inline QColor modbusValueColor()
{
    return isDark() ? QColor("#c792ea") : QColor("#663399");
}

inline QColor modbusDataColor()
{
    return isDark() ? QColor("#abb2bf") : QColor("#444444");
}

inline QColor modbusRequestColor()
{
    return isDark() ? QColor("#98c379") : QColor("#009933");
}

inline QColor modbusResponseColor()
{
    return isDark() ? QColor("#61afef") : QColor("#0066cc");
}

inline QColor modbusLabelColor()
{
    return QApplication::palette().color(QPalette::Text);
}

// Remove-color icon palette
inline QColor removeIconBackground()
{
    return QApplication::palette().color(QPalette::Base);
}

inline QColor removeIconBorder()
{
    return QApplication::palette().color(QPalette::Mid);
}

inline QColor markerColor(MarkerColorId id, bool darkMode)
{
    switch (id) {
        case MarkerColorId::Yellow:
            return darkMode ? QColor("#B08A2E") : QColor("#FFF59D");
        case MarkerColorId::Cyan:
            return darkMode ? QColor("#2C7E8F") : QColor("#B2EBF2");
        case MarkerColorId::Magenta:
            return darkMode ? QColor("#8E4C8B") : QColor("#E1BEE7");
        case MarkerColorId::LightGreen:
            return darkMode ? QColor("#4F8A57") : QColor("#C8E6C9");
        case MarkerColorId::Orange:
            return darkMode ? QColor("#A56A2A") : QColor("#FFE0B2");
        case MarkerColorId::LightBlue:
            return darkMode ? QColor("#4F78A8") : QColor("#BBDEFB");
        case MarkerColorId::LightGray:
            return darkMode ? QColor("#6B7280") : QColor("#E5E7EB");
    }
    return {};
}

inline QColor markerColor(MarkerColorId id)
{
    return markerColor(id, isDark());
}

inline QColor normalizeMarkerColor(const QColor& color)
{
    if (!color.isValid())
        return color;

    constexpr std::array<MarkerColorId, 7> ids = {
        MarkerColorId::Yellow,
        MarkerColorId::Cyan,
        MarkerColorId::Magenta,
        MarkerColorId::LightGreen,
        MarkerColorId::Orange,
        MarkerColorId::LightBlue,
        MarkerColorId::LightGray
    };

    for (const MarkerColorId id : ids) {
        if (color == markerColor(id, false) || color == markerColor(id, true))
            return markerColor(id);
    }

    return color;
}

// Marker colors for row/address highlighting
inline QColor markerYellow()
{
    return markerColor(MarkerColorId::Yellow);
}

inline QColor markerCyan()
{
    return markerColor(MarkerColorId::Cyan);
}

inline QColor markerMagenta()
{
    return markerColor(MarkerColorId::Magenta);
}

inline QColor markerLightGreen()
{
    return markerColor(MarkerColorId::LightGreen);
}

inline QColor markerOrange()
{
    return markerColor(MarkerColorId::Orange);
}

inline QColor markerLightBlue()
{
    return markerColor(MarkerColorId::LightBlue);
}

inline QColor markerLightGray()
{
    return markerColor(MarkerColorId::LightGray);
}

// Version label (about widget)
inline QColor versionLabelColor()
{
    return QApplication::palette().color(QPalette::PlaceholderText);
}

// JavaScript syntax highlight colors
inline QColor syntaxDefaultColor()
{
    return isDark() ? QColor(0xabb2bf) : canvasForeground();
}

inline QColor syntaxKeywordColor()
{
    return isDark() ? QColor(0xFF6B9D) : QColor(0xF92672);
}

inline QColor syntaxStringColor()
{
    return isDark() ? QColor(0xE5C07B) : QColor(0xa39b4e);
}

inline QColor syntaxCommentColor()
{
    return isDark() ? QColor(0x676e95) : QColor(0x75715E);
}

inline QColor syntaxTypeColor()
{
    return isDark() ? QColor(0x56B6C2) : QColor(0x54aebf);
}

inline QColor syntaxFunctionColor()
{
    return isDark() ? QColor(0xe5a96b) : QColor(0xdb8744);
}

inline QColor syntaxNumLiteralColor()
{
    return isDark() ? QColor(0xC678DD) : QColor(0xAE81FF);
}

inline QColor syntaxBuiltInColor()
{
    return isDark() ? QColor(0x98C379) : QColor(0x018a0f);
}

} // namespace AppColors

#endif // APPCOLORS_H
