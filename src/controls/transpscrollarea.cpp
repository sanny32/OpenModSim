// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file transpscrollarea.cpp
/// \brief Implements the transpscrollarea functionality.
///

#include <QPalette>
#include "transpscrollarea.h"

///
/// \brief TranspScrollArea::TranspScrollArea
/// \param parent
///
TranspScrollArea::TranspScrollArea(QWidget* parent)
    : QScrollArea(parent)
{
    setAutoFillBackground(true);
    QPalette p = palette();
    p.setColor(QPalette::Window, Qt::transparent);
    setPalette(p);
}

