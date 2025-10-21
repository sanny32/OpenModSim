#ifndef DIALOGSETUPPRESETDATA_H
#define DIALOGSETUPPRESETDATA_H

#include <QModbusDataUnit>
#include "qfixedsizedialog.h"
#include "enums.h"

struct SetupPresetParams
{
    quint16 PointAddress;
    quint16 Length;
    bool ZeroBasedAddress;
    AddressSpace AddrSpace;
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
    explicit DialogSetupPresetData(SetupPresetParams& params, QModbusDataUnit::RegisterType pointType, bool hexAddress, QWidget *parent = nullptr);
    ~DialogSetupPresetData();

    void accept() override;

private:
    Ui::DialogSetupPresetData *ui;
    SetupPresetParams& _params;
};

#endif // DIALOGSETUPPRESETDATA_H
