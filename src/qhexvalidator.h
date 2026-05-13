// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file qhexvalidator.h
/// \brief Declares the qhexvalidator interfaces.
///

#ifndef QHEXVALIDATOR_H
#define QHEXVALIDATOR_H

#include <QIntValidator>

///
/// \brief The QHexValidator class
///
class QHexValidator : public QIntValidator
{
public:
    explicit QHexValidator(QObject *parent = nullptr, bool allowEmpty = false);
    QHexValidator(int bottom, int top, QObject* parent = nullptr, bool allowEmpty = false);

    State validate(QString &, int &) const override;

private:
    bool _allowEmpty = false;
};

#endif // QHEXVALIDATOR_H

