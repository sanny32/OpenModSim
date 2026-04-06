#ifndef QINTVALIDATOREX_H
#define QINTVALIDATOREX_H

#include <QIntValidator>

///
/// \brief The QIntValidatorEx class
///
class QIntValidatorEx final : public QIntValidator
{
public:
    QIntValidatorEx(int bottom, int top, bool allowEmpty, QObject* parent = nullptr);

    State validate(QString& input, int& pos) const override;
    void fixup(QString& input) const override;

private:
    bool _allowEmpty;
};

#endif // QINTVALIDATOREX_H

