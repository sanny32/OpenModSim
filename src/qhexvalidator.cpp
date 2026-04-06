#include "qhexvalidator.h"

///
/// \brief QHexValidator::QHexValidator
/// \param parent
/// \param allowEmpty
///
QHexValidator::QHexValidator(QObject *parent, bool allowEmpty)
    : QIntValidator{parent}
    , _allowEmpty(allowEmpty)
{
}

QHexValidator::QHexValidator(int bottom, int top, QObject* parent, bool allowEmpty)
    : QIntValidator(bottom, top, parent)
    , _allowEmpty(allowEmpty)
{
}

///
/// \brief QHexValidator::validate
/// \param input
/// \return
///
QHexValidator::State QHexValidator::validate(QString& input, int&) const
{
    if(input.isEmpty()) {
        return _allowEmpty ? QValidator::Acceptable : QValidator::Intermediate;
    }

    bool ok = false;
    input.toInt(&ok, 16);

    if(ok)
    {
        return QValidator::Acceptable;
    }

    return QValidator::Invalid;
}

