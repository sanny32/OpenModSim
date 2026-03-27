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
