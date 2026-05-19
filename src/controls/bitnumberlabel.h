// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file bitnumberlabel.h
/// \brief Declares the bitnumberlabel interfaces.
///

#ifndef BITNUMBERLABEL_H
#define BITNUMBERLABEL_H

#include "clickablelabel.h"

///
/// \brief The BitNumberLabel class
///
class BitNumberLabel : public ClickableLabel
{
    Q_OBJECT
    Q_PROPERTY(int bitNumber READ bitNumber WRITE setBitNumber)

public:
    explicit BitNumberLabel(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    int bitNumber() const;
    void setBitNumber(int bitNumber);

private:
    void updateLabel();

    int _bitNumber = 0;
};

#endif // BITNUMBERLABEL_H
