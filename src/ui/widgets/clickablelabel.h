// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file clickablelabel.h
/// \brief Declares the clickablelabel interfaces.
///

#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>

///
/// \brief The ClickableLabel class
///
class ClickableLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ClickableLabel(QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());

signals:
    void clicked();

protected:
    void mouseReleaseEvent(QMouseEvent* event);

};

#endif // CLICKABLELABEL_H

