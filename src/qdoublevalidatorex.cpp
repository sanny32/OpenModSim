#include "qdoublevalidatorex.h"

///
/// \brief QDoubleValidatorEx::QDoubleValidatorEx
/// \param bottom
/// \param top
/// \param decimals
/// \param allowEmpty
/// \param parent
///
QDoubleValidatorEx::QDoubleValidatorEx(double bottom, double top, int decimals, bool allowEmpty, QObject* parent)
    : QDoubleValidator(bottom, top, decimals, parent)
    , _allowEmpty(allowEmpty)
{
}

///
/// \brief QDoubleValidatorEx::validate
/// \param input
/// \param pos
/// \return
///
QValidator::State QDoubleValidatorEx::validate(QString& input, int& pos) const
{
    if(_allowEmpty && input.isEmpty()) return Acceptable;
    return QDoubleValidator::validate(input, pos);
}

///
/// \brief QDoubleValidatorEx::fixup
/// \param input
///
void QDoubleValidatorEx::fixup(QString& input) const
{
    if(_allowEmpty && input.isEmpty()) return;
    QDoubleValidator::fixup(input);
}
