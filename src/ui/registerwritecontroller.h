// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file registerwritecontroller.h
/// \brief Declares the registerwritecontroller interfaces.
///

#ifndef REGISTERWRITECONTROLLER_H
#define REGISTERWRITECONTROLLER_H

#include <QObject>
#include <QModbusDataUnit>
#include "forcerangeparams.h"

class QWidget;
class FormDataView;
class ModbusMultiServer;

///
/// \brief The RegisterWriteController class runs the force/preset register dialogs
/// and applies the accepted values to the Modbus server, remembering the last
/// edited range per register type.
///
class RegisterWriteController : public QObject
{
    Q_OBJECT

public:
    explicit RegisterWriteController(ModbusMultiServer& server, QObject* parent = nullptr);

    void forceCoils(QModbusDataUnit::RegisterType type, FormDataView* activeForm, QWidget* dialogParent);
    void presetRegisters(QModbusDataUnit::RegisterType type, FormDataView* activeForm, QWidget* dialogParent);

private:
    struct PreparedWrite
    {
        ModbusWriteParams Params{};
        DataViewDefinitions Definitions;
        int Length = 0;
    };
    PreparedWrite prepareWrite(QModbusDataUnit::RegisterType type, FormDataView* frm) const;

private:
    ModbusMultiServer& _mbServer;
    ForceRangeParamsMap _forceRangeParams;
};

#endif // REGISTERWRITECONTROLLER_H
