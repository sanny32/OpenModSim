// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_validators.cpp
/// \brief Unit tests for the custom input validators.
///

#include <QTest>

#include "qhexvalidator.h"
#include "qint64validator.h"
#include "quintvalidator.h"
#include "qintvalidatorex.h"
#include "qdoublevalidatorex.h"

namespace {

QValidator::State validateState(const QValidator& validator, QString text)
{
    int pos = text.size();
    return validator.validate(text, pos);
}

}

class TestValidators : public QObject
{
    Q_OBJECT

private slots:
    void hexValidatorAcceptsHexRejectsGarbage();
    void hexValidatorEmptyDependsOnAllowEmpty();
    void int64ValidatorRange();
    void int64ValidatorEmptyDependsOnAllowEmpty();
    void uintValidatorRejectsNegativeAndOverflow();
    void intValidatorExAllowsEmpty();
    void doubleValidatorExAllowsEmpty();
};

void TestValidators::hexValidatorAcceptsHexRejectsGarbage()
{
    QHexValidator validator;
    QCOMPARE(validateState(validator, QStringLiteral("1A")), QValidator::Acceptable);
    QCOMPARE(validateState(validator, QStringLiteral("ff")), QValidator::Acceptable);
    QCOMPARE(validateState(validator, QStringLiteral("ZZ")), QValidator::Invalid);
    QCOMPARE(validateState(validator, QStringLiteral("FFFFFFFFFF")), QValidator::Invalid);
}

void TestValidators::hexValidatorEmptyDependsOnAllowEmpty()
{
    QHexValidator strict(nullptr, false);
    QHexValidator lenient(nullptr, true);
    QCOMPARE(validateState(strict, QString()), QValidator::Intermediate);
    QCOMPARE(validateState(lenient, QString()), QValidator::Acceptable);
}

void TestValidators::int64ValidatorRange()
{
    QInt64Validator validator(0, 100);
    QCOMPARE(validateState(validator, QStringLiteral("50")), QValidator::Acceptable);
    QCOMPARE(validateState(validator, QStringLiteral("0")), QValidator::Acceptable);
    QCOMPARE(validateState(validator, QStringLiteral("100")), QValidator::Acceptable);
    QCOMPARE(validateState(validator, QStringLiteral("101")), QValidator::Invalid);
    QCOMPARE(validateState(validator, QStringLiteral("-1")), QValidator::Invalid);
    QCOMPARE(validateState(validator, QStringLiteral("abc")), QValidator::Invalid);
}

void TestValidators::int64ValidatorEmptyDependsOnAllowEmpty()
{
    QInt64Validator strict(0, 100);
    QInt64Validator lenient(0, 100, true, nullptr);
    QCOMPARE(validateState(strict, QString()), QValidator::Intermediate);
    QCOMPARE(validateState(lenient, QString()), QValidator::Acceptable);
}

void TestValidators::uintValidatorRejectsNegativeAndOverflow()
{
    QUIntValidator validator(0, 65535);
    QCOMPARE(validateState(validator, QStringLiteral("1234")), QValidator::Acceptable);
    QCOMPARE(validateState(validator, QStringLiteral("65536")), QValidator::Invalid);
    QCOMPARE(validateState(validator, QStringLiteral("-1")), QValidator::Invalid);
}

void TestValidators::intValidatorExAllowsEmpty()
{
    QIntValidatorEx strict(0, 100, false);
    QIntValidatorEx lenient(0, 100, true);
    QCOMPARE(validateState(lenient, QString()), QValidator::Acceptable);
    QCOMPARE(validateState(strict, QString()), QValidator::Intermediate);
    QCOMPARE(validateState(lenient, QStringLiteral("50")), QValidator::Acceptable);
    QCOMPARE(validateState(lenient, QStringLiteral("abc")), QValidator::Invalid);
}

void TestValidators::doubleValidatorExAllowsEmpty()
{
    QDoubleValidatorEx lenient(0.0, 100.0, 2, true);
    lenient.setLocale(QLocale::c());
    QCOMPARE(validateState(lenient, QString()), QValidator::Acceptable);
    QCOMPARE(validateState(lenient, QStringLiteral("3.14")), QValidator::Acceptable);
    QCOMPARE(validateState(lenient, QStringLiteral("abc")), QValidator::Invalid);
}

QTEST_GUILESS_MAIN(TestValidators)
#include "test_validators.moc"
