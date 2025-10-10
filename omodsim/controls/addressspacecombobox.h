#ifndef ADDRESSSPACECOMBOBOX_H
#define ADDRESSSPACECOMBOBOX_H

#include <QComboBox>
#include "enums.h"

class AddressSpaceComboBox : public QComboBox
{
    Q_OBJECT
public:
    AddressSpaceComboBox(QWidget *parent = nullptr);

    AddressSpace currentAddressSpace() const;
    void setCurrentAddressSpace(AddressSpace asp);

signals:
    void addressSpaceChanged(AddressSpace value);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_currentIndexChanged(int);
};

#endif // ADDRESSSPACECOMBOBOX_H
