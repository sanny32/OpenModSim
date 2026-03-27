#ifndef QUINTVALIDATOR_H
#define QUINTVALIDATOR_H

#include <QValidator>
#include <QObject>

///
/// \brief The QUIntValidator class
///
class QUIntValidator : public QValidator
{
    Q_OBJECT
public:
    explicit QUIntValidator(QObject *parent = nullptr);
    QUIntValidator(quint64 bottom, quint64 top, QObject *parent = nullptr);
    QUIntValidator(quint64 bottom, quint64 top, bool allowEmpty, QObject *parent = nullptr);

    State validate(QString&, int&) const override;
    void fixup(QString& input) const override;

private:
    quint64 _bottom;
    quint64 _top;
    bool _allowEmpty = false;
};

#endif // QUINTVALIDATOR_H
