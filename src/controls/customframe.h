// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file customframe.h
/// \brief Declares the customframe interfaces.
///

#ifndef CUSTOMFRAME_H
#define CUSTOMFRAME_H

#include <QFrame>

///
/// \brief The CustomFrame class
///
class CustomFrame : public QFrame
{
    Q_OBJECT
public:
    CustomFrame(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

protected:
    void paintEvent(QPaintEvent *) override;
};

#endif // CUSTOMFRAME_H

