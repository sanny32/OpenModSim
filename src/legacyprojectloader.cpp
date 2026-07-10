// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file legacyprojectloader.cpp
/// \brief Implements project file compatibility loading helpers.
///

#include <QHash>
#include <QModbusDataUnit>
#include <QWidget>
#include <QXmlStreamReader>

#include "apppreferences.h"
#include "appproject.h"
#include "displaydefinition.h"
#include "formdataview.h"
#include "formscriptview.h"
#include "formatutils.h"
#include "legacyprojectloader.h"
#include "legacyprojectparser.h"
#include "modbussimulationparams.h"
#include "outputtypes.h"

namespace {
///
/// \brief readWindowState reads legacy MDI window geometry attributes.
/// \param xml XML reader positioned on the Window element.
/// \param form Target data view form.
///
void readWindowState(QXmlStreamReader& xml, FormDataView& form)
{
    const QXmlStreamAttributes windowAttrs = xml.attributes();

    const auto wnd = form.parentWidget();
    if (wnd) {
        if(windowAttrs.hasAttribute("Left") && windowAttrs.hasAttribute("Top")) {
            bool okLeft, okTop;
            const int left = windowAttrs.value("Left").toInt(&okLeft);
            const int top = windowAttrs.value("Top").toInt(&okTop);
            if(okLeft && okTop) {
                wnd->move(left, top);
            }
        }

        if (windowAttrs.hasAttribute("Width") && windowAttrs.hasAttribute("Height")) {
            bool okWidth, okHeight;
            const int width = windowAttrs.value("Width").toInt(&okWidth);
            const int height = windowAttrs.value("Height").toInt(&okHeight);

            if (okWidth && okHeight && !wnd->isMaximized() && !wnd->isMinimized()) {
                wnd->resize(width, height);
            }
        }

        if (windowAttrs.hasAttribute("Maximized")) {
            const bool maximized = stringToBool(windowAttrs.value("Maximized").toString());
            if (maximized) wnd->showMaximized();
        }

        if (windowAttrs.hasAttribute("Minimized")) {
            const bool minimized = stringToBool(windowAttrs.value("Minimized").toString());
            if (minimized) wnd->showMinimized();
        }
    }

    xml.skipCurrentElement();
}

///
/// \brief readDataViewDefinitions reads current or legacy data view definitions.
/// \param xml XML reader positioned on the definitions element.
/// \param form Target data view form.
/// \param formTitle Fallback form title.
/// \return Parsed data view definitions.
///
DataViewDefinitions readDataViewDefinitions(QXmlStreamReader& xml, FormDataView* form, const QString& formTitle)
{
    DataViewDefinitions dd;
    const auto attributes = xml.attributes();

    if (attributes.hasAttribute("FormName")) {
        dd.FormName = attributes.value("FormName").toString();
    }

    if (attributes.hasAttribute("DeviceId")) {
        bool ok;
        const quint8 deviceId = attributes.value("DeviceId").toUShort(&ok);
        if (ok)
            dd.DeviceId = deviceId;
    }

    if (attributes.hasAttribute("PointType")) {
        dd.PointType = enumFromString<QModbusDataUnit::RegisterType>(attributes.value("PointType").toString());
    }

    if (attributes.hasAttribute("PointAddress")) {
        bool ok;
        const quint16 pointAddress = attributes.value("PointAddress").toUShort(&ok);
        if (ok)
            dd.PointAddress = pointAddress;
    }

    if (attributes.hasAttribute("Length")) {
        bool ok;
        const quint16 length = attributes.value("Length").toUShort(&ok);
        if (ok)
            dd.Length = length;
    }

    if (attributes.hasAttribute("DataViewColumnsDistance")) {
        bool ok;
        const quint16 distance = attributes.value("DataViewColumnsDistance").toUShort(&ok);
        if (ok)
            dd.DataViewColumnsDistance = distance;
    }

    if (attributes.hasAttribute("LeadingZeros")) {
        dd.LeadingZeros = stringToBool(attributes.value("LeadingZeros").toString());
    }

    xml.skipCurrentElement();

    if (attributes.hasAttribute("ZeroBasedAddress")) {
        if (form) {
            if (stringToBool(attributes.value("ZeroBasedAddress").toString())) {
                form->setAddressBase(AddressBase::Base0);
            } else {
                form->setAddressBase(AddressBase::Base1);
            }
        }
    }

    if (dd.FormName.isEmpty() && !formTitle.isEmpty())
        dd.FormName = formTitle;

    dd.normalize();

    return dd;
}

///
/// \brief applyAddressDescriptions applies legacy descriptions to the form.
/// \param form Target data view form.
/// \param dd Data view definitions used as fallback address context.
/// \param map Description map read from XML.
///
void applyAddressDescriptions(FormDataView& form, const DataViewDefinitions& dd, const AddressDescriptionMap& map)
{
    for(auto it = map.cbegin(); it != map.cend(); ++it)
    {
        const auto deviceId = it.key().DeviceId;
        const auto type = it.key().Type;
        form.setDescription(deviceId ? deviceId : dd.DeviceId,
                            type ? type : dd.PointType,
                            it.key().Address,
                            it.value());
    }
}

///
/// \brief applyAddressColors applies legacy colors to the form.
/// \param form Target data view form.
/// \param dd Data view definitions used as fallback address context.
/// \param map Color map read from XML.
///
void applyAddressColors(FormDataView& form, const DataViewDefinitions& dd, const AddressColorMap& map)
{
    for(auto it = map.cbegin(); it != map.cend(); ++it)
    {
        const auto deviceId = it.key().DeviceId;
        const auto type = it.key().Type;
        form.setColor(deviceId ? deviceId : dd.DeviceId,
                      type ? type : dd.PointType,
                      it.key().Address,
                      it.value());
    }
}

///
/// \brief applySimulations applies legacy simulation settings to the form.
/// \param form Target data view form.
/// \param dd Data view definitions used to select simulation behavior.
/// \param simulations Legacy simulation map.
///
void applySimulations(FormDataView& form,
                      const DataViewDefinitions& dd,
                      const QHash<quint16, ModbusSimulationParams>& simulations)
{
    if(simulations.isEmpty())
        return;

    QHashIterator it(simulations);
    while(it.hasNext()) {
        const auto item = it.next();
        const auto index = item.key() - (form.zeroBasedAddress() ? 0 : 1);
        if (index < 0) {
            continue;
        }

        switch(dd.PointType) {
            case QModbusDataUnit::Coils:
            case QModbusDataUnit::DiscreteInputs:
                if(item->Mode == SimulationMode::Toggle || item->Mode == SimulationMode::Random)
                    form.startSimulation(dd.PointType, index, item.value());
                break;
            case QModbusDataUnit::InputRegisters:
            case QModbusDataUnit::HoldingRegisters:
                if(item->Mode != SimulationMode::Off && item->Mode != SimulationMode::Toggle)
                    form.startSimulation(dd.PointType, index, item.value());
                break;
            default: break;
        }
    }
}

///
/// \brief applyDataUnit applies legacy Modbus data values to the form.
/// \param form Target data view form.
/// \param dd Data view definitions used to map addresses.
/// \param data Legacy data map.
///
void applyDataUnit(FormDataView& form, const DataViewDefinitions& dd, const QHash<quint16, quint16>& data)
{
    if (data.isEmpty())
        return;

    const auto values = LegacyProjectParser::dataUnitValues(dd, data);

    form.configureModbusDataUnit(dd.DeviceId,
                                 dd.PointType,
                                 qMax(dd.PointAddress - (form.zeroBasedAddress() ? 0 : 1), 0),
                                 values);
}

///
/// \brief finalizeScriptView applies legacy script settings and startup behavior.
/// \param form Source data view form.
/// \param project Project used to close the hidden script form.
/// \param script Script view created for legacy embedded script data.
/// \param definitions Parsed script view definitions.
///
void finalizeScriptView(FormDataView& form,
                        AppProject& project,
                        FormScriptView* script,
                        ScriptViewDefinitions definitions)
{
    if (!script)
        return;

    definitions.FormName = form.windowTitle();
    definitions.normalize();
    script->setDefinitions(definitions);
    script->setFont(AppPreferences::instance().scriptFont());

    project.closeMdiChild(script);

    if (definitions.ScriptCfg.RunOnStartup) {
        script->runScript();
    }
}
}

///
/// \brief LegacyProjectLoader::isDataViewElement
/// \param name Element name to test.
/// \return true for legacy data view elements.
///
bool LegacyProjectLoader::isDataViewElement(const QString& name) noexcept
{
    return LegacyProjectParser::isDataViewElement(name);
}

///
/// \brief LegacyProjectLoader::loadDataView
/// \param xml XML reader positioned on the legacy data view element.
/// \param form Target data view form.
/// \param project Project used to create related forms.
///
void LegacyProjectLoader::loadDataView(QXmlStreamReader& xml, FormDataView& form, AppProject& project)
{
    if (!isDataViewElement(xml.name().toString())) {
        xml.skipCurrentElement();
        return;
    }

    DataType dataType = DataType::UInt16;
    RegisterOrder regOrder = RegisterOrder::MSRF;
    DataViewDefinitions dd;
    QHash<quint16, quint16> data;
    QHash<quint16, ModbusSimulationParams> simulations;
    FormScriptView* script = nullptr;
    ScriptViewDefinitions scriptDefinitions;

    const QXmlStreamAttributes attributes = xml.attributes();
    const QString formTitle = attributes.value("Title").toString();

    if (attributes.hasAttribute("DataType")) {
        dataType = enumFromString<DataType>(attributes.value("DataType").toString(), DataType::UInt16);
    }

    if (attributes.hasAttribute("DataDisplayMode")) {
        dataType = enumFromString<DataType>(attributes.value("DataDisplayMode").toString(), DataType::UInt16);
    }

    if (attributes.hasAttribute("RegisterOrder")) {
        regOrder = enumFromString<RegisterOrder>(attributes.value("RegisterOrder").toString(), RegisterOrder::MSRF);
    }

    if (attributes.hasAttribute("Codepage")) {
        form.setCodepage(attributes.value("Codepage").toString());
    }

    if (attributes.hasAttribute("ByteOrder")) {
        const ByteOrder order = enumFromString<ByteOrder>(attributes.value("ByteOrder").toString());
        form.setByteOrder(order);
    }

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("Window")) {
            readWindowState(xml, form);
        }
        else if (xml.name() == QLatin1String("DataViewDefinitions") ||
                 xml.name() == QLatin1String("DisplayDefinition")) {
            dd = readDataViewDefinitions(xml, &form, formTitle);
            form.setDisplayDefinition(dd);
        }
        else if (xml.name() == QLatin1String("ModbusSimulationMap")) {
            simulations = LegacyProjectParser::readSimulationMap(xml);
        }
        else if (xml.name() == QLatin1String("JScriptControl")) {
            script = qobject_cast<FormScriptView*>(project.createMdiChild(ProjectFormKind::Script));
            if (script) {
                xml >> script->scriptControl();
            } else {
                xml.skipCurrentElement();
            }
        }
        else if (xml.name() == QLatin1String("ScriptSettings")) {
            xml >> scriptDefinitions.ScriptCfg;
        }
        else if (xml.name() == QLatin1String("AddressDescriptionMap")) {
            AddressDescriptionMap map;
            xml >> map;
            applyAddressDescriptions(form, dd, map);
        }
        else if (xml.name() == QLatin1String("AddressColorMap")) {
            const AddressColorMap map = LegacyProjectParser::readAddressColors(xml);
            applyAddressColors(form, dd, map);
        }
        else if (xml.name() == QLatin1String("ModbusDataUnit")) {
            data = LegacyProjectParser::readDataUnit(xml);
        }
        else {
            xml.skipCurrentElement();
        }
    }

    if(dd.PointType != QModbusDataUnit::Invalid) {
        form.setDataType(dataType);
        form.setRegisterOrder(regOrder);
        applySimulations(form, dd, simulations);
        applyDataUnit(form, dd, data);
    }

    finalizeScriptView(form, project, script, scriptDefinitions);
}
