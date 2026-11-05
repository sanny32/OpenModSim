// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file bitnumberlabel.cpp
/// \brief Implements the bitnumberlabel functionality.
///

#include "bitnumberlabel.h"
#include <QFont>

///
/// \brief BitNumberLabel::BitNumberLabel
/// \param parent
/// \param f
///
BitNumberLabel::BitNumberLabel(QWidget* parent, Qt::WindowFlags f)
    : ClickableLabel(parent, f)
{
    setAlignment(Qt::AlignCenter);
    setTextFormat(Qt::PlainText);
    updateLabel();
}

///
/// \brief BitNumberLabel::bitNumber
/// \return
///
int BitNumberLabel::bitNumber() const
{
    return _bitNumber;
}

///
/// \brief BitNumberLabel::setBitNumber
/// \param bitNumber
///
void BitNumberLabel::setBitNumber(int bitNumber)
{
    if (_bitNumber == bitNumber)
        return;

    _bitNumber = bitNumber;
    updateLabel();
}

///
/// \brief BitNumberLabel::updateLabel
///
void BitNumberLabel::updateLabel()
{
    QFont font = this->font();
    font.setBold(true);
#ifdef Q_OS_MAC
    font.setPointSize(10);
#else
    font.setPointSize(7);
#endif
    setFont(font);
    setText(QString::number(_bitNumber));
}
