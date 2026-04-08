#ifndef DIALOGSETUPPRESETDATA_H
#define DIALOGSETUPPRESETDATA_H

#include <QModbusDataUnit>
#include "qfixedsizedialog.h"
#include "displaydefinition.h"
#include "enums.h"

struct SetupPresetParams
{
    quint16 DeviceId;
    quint16 PointAddress;
    quint16 Length;
    bool ZeroBasedAddress;
    AddressSpace AddrSpace;
    bool LeadingZeros = false;
};

namespace Ui {
class DialogSetupPresetData;
}

///
/// \brief The DialogSetupPresetData class
///
class DialogSetupPresetData : public QFixedSizeDialog
{
    Q_OBJECT

public:
    explicit DialogSetupPresetData(SetupPresetParams& params, QModbusDataUnit::RegisterType pointType, bool displayHexAddresses, QWidget *parent = nullptr);
    ~DialogSetupPresetData();

    void accept() override;

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_lineEditAddress_valueChanged(const QVariant&);

private:
    Ui::DialogSetupPresetData *ui;
    SetupPresetParams& _params;
    QModbusDataUnit::RegisterType _pointType;
};

#endif // DIALOGSETUPPRESETDATA_H

