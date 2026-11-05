// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file transpscrollarea.h
/// \brief Declares the transpscrollarea interfaces.
///

#ifndef TRANSPSCROLLAREA_H
#define TRANSPSCROLLAREA_H

#include <QObject>
#include <QScrollArea>

///
/// \brief The Transparent ScrollArea class
///
class TranspScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    TranspScrollArea(QWidget* parent = nullptr);
};

#endif // TRANSPSCROLLAREA_H

