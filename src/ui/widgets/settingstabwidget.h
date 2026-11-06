// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file settingstabwidget.h
/// \brief Declares settings tabs with an accent-colored selection.
///

#ifndef SETTINGSTABWIDGET_H
#define SETTINGSTABWIDGET_H

#include <QTabWidget>

class SettingsTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    ///
    /// \brief Creates settings navigation with a palette-aware selected tab.
    /// \param parent Parent widget.
    ///
    explicit SettingsTabWidget(QWidget* parent = nullptr);

#ifdef Q_OS_MAC
protected:
    void paintEvent(QPaintEvent* event) override;
#endif
};

#endif
