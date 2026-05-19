// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file qdoublevalidatorex.h
/// \brief Declares the qdoublevalidatorex interfaces.
///

#ifndef QDOUBLEVALIDATOREX_H
#define QDOUBLEVALIDATOREX_H

#include <QDoubleValidator>

///
/// \brief The QDoubleValidatorEx class
///
class QDoubleValidatorEx final : public QDoubleValidator
{
public:
    QDoubleValidatorEx(double bottom, double top, int decimals, bool allowEmpty, QObject* parent = nullptr);

    State validate(QString& input, int& pos) const override;
    void fixup(QString& input) const override;

private:
    bool _allowEmpty;
};

#endif // QDOUBLEVALIDATOREX_H

