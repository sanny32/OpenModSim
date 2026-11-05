// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file registerwritecontroller.cpp
/// \brief Implements the registerwritecontroller functionality.
///

#include "apppreferences.h"
#include "formdataview.h"
#include "modbusmultiserver.h"
#include "dialogforcestatusregisters.h"
#include "dialogforcemultipleregisters.h"
#include "registerwritecontroller.h"

///
/// \brief RegisterWriteController::RegisterWriteController
/// \param server The Modbus server the accepted values are written to.
/// \param parent The parent object.
///
RegisterWriteController::RegisterWriteController(ModbusMultiServer& server, QObject* parent)
    : QObject(parent)
    , _mbServer(server)
{
}

///
/// \brief RegisterWriteController::prepareWrite resolves the register range and
/// snapshots the current server values for a force/preset dialog.
/// \param type The register type the dialog is opened for.
/// \param frm The active data view, or nullptr when none is open.
/// \return The prepared write parameters, view definitions and range length.
///
RegisterWriteController::PreparedWrite RegisterWriteController::prepareWrite(QModbusDataUnit::RegisterType type, FormDataView* frm) const
{
    PreparedWrite out;
    const auto& prefs = AppPreferences::instance();
    out.Definitions = frm ? frm->displayDefinition() : prefs.dataViewDefinitions();
    const bool zeroBasedAddress = frm ? frm->zeroBasedAddress() : (prefs.globalAddressBase() == AddressBase::Base0);
    const auto addrSpace = _mbServer.getModbusDefinitions().AddrSpace;

    const auto range = resolveForceRange(type, out.Definitions, frm != nullptr, zeroBasedAddress, addrSpace, _forceRangeParams);

    out.Length                  = range.Length;
    out.Params.DeviceId         = range.DeviceId;
    out.Params.Address          = range.Address;
    out.Params.ZeroBasedAddress = range.ZeroBasedAddress;
    out.Params.AddrSpace        = range.AddrSpace;
    out.Params.LeadingZeros     = range.LeadingZeros;
    out.Params.Server           = &_mbServer;

    const auto data = _mbServer.data(static_cast<quint8>(range.DeviceId), type,
        range.Address - (range.ZeroBasedAddress ? 0 : 1),
        range.Length);
    out.Params.Value = QVariant::fromValue(data.values());

    return out;
}

///
/// \brief RegisterWriteController::forceCoils runs the force dialog for coils or
/// discrete inputs and writes the accepted values.
/// \param type The register type: Coils or DiscreteInputs.
/// \param activeForm The active data view, or nullptr when none is open.
/// \param dialogParent The parent widget for the dialog.
///
void RegisterWriteController::forceCoils(QModbusDataUnit::RegisterType type, FormDataView* activeForm, QWidget* dialogParent)
{
    auto prepared = prepareWrite(type, activeForm);

    const bool displayHexAddresses = AppPreferences::instance().globalHexView();
    DialogForceStatusRegisters dlg(prepared.Params, type, prepared.Length, displayHexAddresses, dialogParent);
    if(dlg.exec() == QDialog::Accepted) {
        _forceRangeParams[type] = forceRangeFromWriteParams(prepared.Params);
        _mbServer.writeRegister(type, prepared.Params);
    }
}

///
/// \brief RegisterWriteController::presetRegisters runs the preset dialog for input
/// or holding registers and writes the accepted values.
/// \param type The register type: InputRegisters or HoldingRegisters.
/// \param activeForm The active data view, or nullptr when none is open.
/// \param dialogParent The parent widget for the dialog.
///
void RegisterWriteController::presetRegisters(QModbusDataUnit::RegisterType type, FormDataView* activeForm, QWidget* dialogParent)
{
    auto prepared = prepareWrite(type, activeForm);

    const bool useFormDisplay = activeForm && prepared.Definitions.PointType == type;
    prepared.Params.DataMode = useFormDisplay ? activeForm->dataType()      : DataType::Hex;
    prepared.Params.RegOrder = useFormDisplay ? activeForm->registerOrder() : RegisterOrder::MSRF;
    prepared.Params.Order    = useFormDisplay ? activeForm->byteOrder()     : ByteOrder::Direct;
    prepared.Params.Codepage = useFormDisplay ? activeForm->codepage()      : QString();

    const bool displayHexAddresses = AppPreferences::instance().globalHexView();
    DialogForceMultipleRegisters dlg(prepared.Params, type, prepared.Length, displayHexAddresses, dialogParent);
    if(dlg.exec() == QDialog::Accepted) {
        _forceRangeParams[type] = forceRangeFromWriteParams(prepared.Params);
        _mbServer.writeRegister(type, prepared.Params);
    }
}
