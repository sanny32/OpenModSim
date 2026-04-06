#include "qint64validator.h"

///
/// \brief QInt64Validator::QInt64Validator
/// \param parent
///
QInt64Validator::QInt64Validator(QObject *parent)
    : QValidator{parent}
    ,_bottom(0)
    ,_top(UINT_MAX)
{
}

///
/// \brief QInt64Validator::QInt64Validator
/// \param bottom
/// \param top
/// \param parent
///
QInt64Validator::QInt64Validator(qint64 bottom, qint64 top, QObject *parent)
    : QValidator(parent)
    ,_bottom(bottom)
    ,_top(top)
{
}

///
/// \brief QInt64Validator::QInt64Validator
/// \param bottom
/// \param top
/// \param allowEmpty
/// \param parent
///
QInt64Validator::QInt64Validator(qint64 bottom, qint64 top, bool allowEmpty, QObject *parent)
    : QValidator(parent)
    ,_bottom(bottom)
    ,_top(top)
    ,_allowEmpty(allowEmpty)
{
}

///
/// \brief QInt64Validator::validate
/// \return
///
QValidator::State QInt64Validator::validate(QString& input, int &) const
{
    if(input.isEmpty())
        return _allowEmpty ? QValidator::Acceptable : QValidator::Intermediate;

    bool ok = false;
    const auto value = input.toLongLong(&ok, 10);

    if (ok && value >=_bottom && value <=_top )
    {
        return QValidator::Acceptable;
    }

    return QValidator::Invalid;
}

///
/// \brief QInt64Validator::fixup
/// \param input
///
void QInt64Validator::fixup(QString& input) const
{
    if(_allowEmpty && input.isEmpty())
        return;

    QValidator::fixup(input);
}

