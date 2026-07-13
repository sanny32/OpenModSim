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
    void uintValidatorFixupFillsBottom();
    void int64ValidatorFixupKeepsEmptyWhenAllowed();
    void intValidatorExFixupKeepsEmptyWhenAllowed();
    void doubleValidatorExFixupKeepsEmptyWhenAllowed();
    void defaultConstructedIntegerValidators();
    void uintValidatorRangeEdges();
    void nonEmptyFixupPaths();
    void additionalValidatorEdgeBranches();
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

void TestValidators::uintValidatorFixupFillsBottom()
{
    QUIntValidator strict(5, 100);
    QString empty;
    strict.fixup(empty);
    QCOMPARE(empty, QStringLiteral("5"));

    QUIntValidator lenient(5, 100, true, nullptr);
    QString stillEmpty;
    lenient.fixup(stillEmpty);
    QVERIFY(stillEmpty.isEmpty());
}

void TestValidators::int64ValidatorFixupKeepsEmptyWhenAllowed()
{
    QInt64Validator lenient(0, 100, true, nullptr);
    QString empty;
    lenient.fixup(empty);
    QVERIFY(empty.isEmpty());
}

void TestValidators::intValidatorExFixupKeepsEmptyWhenAllowed()
{
    QIntValidatorEx lenient(0, 100, true);
    QString empty;
    lenient.fixup(empty);
    QVERIFY(empty.isEmpty());
}

void TestValidators::doubleValidatorExFixupKeepsEmptyWhenAllowed()
{
    QDoubleValidatorEx lenient(0.0, 100.0, 2, true);
    QString empty;
    lenient.fixup(empty);
    QVERIFY(empty.isEmpty());
}

void TestValidators::defaultConstructedIntegerValidators()
{
    QInt64Validator signedValidator;
    QUIntValidator unsignedValidator;

    QCOMPARE(validateState(signedValidator, QString()), QValidator::Intermediate);
    QCOMPARE(validateState(unsignedValidator, QString()), QValidator::Intermediate);
    QCOMPARE(validateState(signedValidator, QStringLiteral("0")), QValidator::Acceptable);
    QCOMPARE(validateState(unsignedValidator, QStringLiteral("0")), QValidator::Acceptable);
    QCOMPARE(validateState(signedValidator, QStringLiteral("-1")), QValidator::Invalid);
    QCOMPARE(validateState(unsignedValidator, QStringLiteral("-1")), QValidator::Invalid);
}

void TestValidators::uintValidatorRangeEdges()
{
    QUIntValidator validator(10, 20);

    QCOMPARE(validateState(validator, QString()), QValidator::Intermediate);
    QCOMPARE(validateState(validator, QStringLiteral("9")), QValidator::Invalid);
    QCOMPARE(validateState(validator, QStringLiteral("10")), QValidator::Acceptable);
    QCOMPARE(validateState(validator, QStringLiteral("20")), QValidator::Acceptable);
    QCOMPARE(validateState(validator, QStringLiteral("21")), QValidator::Invalid);
    QCOMPARE(validateState(validator, QStringLiteral("not-a-number")), QValidator::Invalid);
}

void TestValidators::nonEmptyFixupPaths()
{
    QInt64Validator int64Validator(0, 100);
    QString int64Text = QStringLiteral("42");
    int64Validator.fixup(int64Text);
    QCOMPARE(int64Text, QStringLiteral("42"));

    QIntValidatorEx intValidator(0, 100, true);
    QString intText = QStringLiteral("42");
    intValidator.fixup(intText);
    QCOMPARE(intText, QStringLiteral("42"));

    QDoubleValidatorEx doubleValidator(0.0, 100.0, 2, true);
    doubleValidator.setLocale(QLocale::c());
    QString doubleText = QStringLiteral("42.5");
    doubleValidator.fixup(doubleText);
    QVERIFY(!doubleText.isEmpty());
    QCOMPARE(validateState(doubleValidator, doubleText), QValidator::Acceptable);
    bool parsed = false;
    QCOMPARE(doubleValidator.locale().toDouble(doubleText, &parsed), 42.5);
    QVERIFY(parsed);
}

void TestValidators::additionalValidatorEdgeBranches()
{
    QUIntValidator lenientUInt(10, 20, true, nullptr);
    QCOMPARE(validateState(lenientUInt, QString()), QValidator::Acceptable);

    QString uintText = QStringLiteral("not-empty");
    lenientUInt.fixup(uintText);
    QCOMPARE(uintText, QStringLiteral("not-empty"));

    QInt64Validator signedValidator(-10, 10);
    QCOMPARE(validateState(signedValidator, QStringLiteral("-10")), QValidator::Acceptable);
    QCOMPARE(validateState(signedValidator, QStringLiteral("10")), QValidator::Acceptable);
    QCOMPARE(validateState(signedValidator, QStringLiteral("-11")), QValidator::Invalid);

    QHexValidator rangedHex(0, 0xFF, nullptr, false);
    QCOMPARE(validateState(rangedHex, QStringLiteral("ff")), QValidator::Acceptable);
    QCOMPARE(validateState(rangedHex, QStringLiteral("100")), QValidator::Acceptable);

    QDoubleValidatorEx strictDouble(0.0, 10.0, 2, false);
    strictDouble.setLocale(QLocale::c());
    QCOMPARE(validateState(strictDouble, QString()), QValidator::Intermediate);
    QString emptyDouble;
    strictDouble.fixup(emptyDouble);
    QVERIFY(emptyDouble.isEmpty());
}

QTEST_GUILESS_MAIN(TestValidators)
#include "test_validators.moc"
