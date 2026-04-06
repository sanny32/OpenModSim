#include "qintvalidatorex.h"

///
/// \brief QIntValidatorEx::QIntValidatorEx
/// \param bottom
/// \param top
/// \param allowEmpty
/// \param parent
///
QIntValidatorEx::QIntValidatorEx(int bottom, int top, bool allowEmpty, QObject* parent)
    : QIntValidator(bottom, top, parent)
    , _allowEmpty(allowEmpty)
{
}

///
/// \brief QIntValidatorEx::validate
/// \param input
/// \param pos
/// \return
///
QValidator::State QIntValidatorEx::validate(QString& input, int& pos) const
{
    if(_allowEmpty && input.isEmpty()) return Acceptable;
    return QIntValidator::validate(input, pos);
}

///
/// \brief QIntValidatorEx::fixup
/// \param input
///
void QIntValidatorEx::fixup(QString& input) const
{
    if(_allowEmpty && input.isEmpty()) return;
    QIntValidator::fixup(input);
}

