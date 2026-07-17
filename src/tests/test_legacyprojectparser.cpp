// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_legacyprojectparser.cpp
/// \brief Unit tests for parsing legacy project data.
///

#include <QColor>
#include <QTest>
#include <QXmlStreamReader>

#include "legacyprojectparser.h"

namespace {
///
/// \brief readLegacyDataViewState parses a legacy data view XML fragment.
/// \param xmlText XML text to parse.
/// \return Parsed legacy data view state.
///
LegacyDataViewState readLegacyDataViewState(const QByteArray& xmlText)
{
    QXmlStreamReader reader(xmlText);
    reader.readNextStartElement();
    return LegacyProjectParser::readDataViewState(reader);
}
}

///
/// \brief The TestLegacyProjectParser class validates legacy project parsing.
///
class TestLegacyProjectParser : public QObject
{
    Q_OBJECT

private slots:
    void recognizesLegacyDataViewElement();
    void readsLegacyFormModSimDataViewState();
    void readsLegacyDisplayDefinitionDefaultsAndNormalizes();
    void preservesZeroBasedPointAddress();
    void readsLegacyColorFallbackKeys();
    void ignoresMalformedLegacySimulationsAndValues();
    void mapsDataUnitValuesWithoutStoppingAfterMalformedAddress();
    void clampsCoilValues();
};

///
/// \brief TestLegacyProjectParser::recognizesLegacyDataViewElement
///
void TestLegacyProjectParser::recognizesLegacyDataViewElement()
{
    QVERIFY(LegacyProjectParser::isDataViewElement(QStringLiteral("FormModSim")));
    QVERIFY(!LegacyProjectParser::isDataViewElement(QStringLiteral("FormDataView")));
    QVERIFY(!LegacyProjectParser::isDataViewElement(QStringLiteral("DisplayDefinition")));
}

///
/// \brief TestLegacyProjectParser::readsLegacyFormModSimDataViewState
///
void TestLegacyProjectParser::readsLegacyFormModSimDataViewState()
{
    const QByteArray xml = R"xml(
<FormModSim Title='Legacy Registers' DataDisplayMode='UInt32' RegisterOrder='LSRF'>
  <DisplayDefinition FormName='' DeviceId='7' PointType='4' PointAddress='10' Length='3'
                     DataViewColumnsDistance='4' LeadingZeros='false' ZeroBasedAddress='true'/>
  <ModbusSimulationMap>
    <Simulation Address='10'>
      <ModbusSimulationParams Mode='Increment' Interval='250' DataType='UInt32' RegisterOrder='LSRF'>
        <IncrementSimulationParams Step='2'>
          <Range From='0' To='100'/>
        </IncrementSimulationParams>
      </ModbusSimulationParams>
    </Simulation>
  </ModbusSimulationMap>
  <AddressDescriptionMap>
    <Description Address='11'><![CDATA[legacy description]]></Description>
  </AddressDescriptionMap>
  <AddressColorMap>
    <Color Address='12' Value='#112233'/>
  </AddressColorMap>
  <ModbusDataUnit>
    <Value Address='10'>100</Value>
    <Value Address='12'>300</Value>
  </ModbusDataUnit>
</FormModSim>
)xml";

    const auto state = readLegacyDataViewState(xml);

    QCOMPARE(state.DataTypeValue, DataType::UInt32);
    QCOMPARE(state.RegisterOrderValue, RegisterOrder::LSRF);
    QCOMPARE(state.Definitions.FormName, QStringLiteral("Legacy Registers"));
    QCOMPARE(state.Definitions.DeviceId, quint8(7));
    QCOMPARE(state.Definitions.PointType, QModbusDataUnit::HoldingRegisters);
    QCOMPARE(state.Definitions.PointAddress, quint16(10));
    QCOMPARE(state.Definitions.Length, quint16(3));
    QCOMPARE(state.Definitions.DataViewColumnsDistance, quint16(4));
    QCOMPARE(state.Definitions.LeadingZeros, false);

    QVERIFY(state.Simulations.contains(10));
    QCOMPARE(state.Simulations.value(10).Mode, SimulationMode::Increment);
    QCOMPARE(state.Simulations.value(10).Interval, 250u);
    QCOMPARE(state.Simulations.value(10).DataMode, DataType::UInt32);
    QCOMPARE(state.Simulations.value(10).RegOrder, RegisterOrder::LSRF);
    QCOMPARE(state.Simulations.value(10).IncrementParams.Step, 2.);

    const ItemMapKey descriptionKey { 0, QModbusDataUnit::Invalid, 11 };
    QCOMPARE(state.Descriptions.value(descriptionKey), QStringLiteral("legacy description"));

    const ItemMapKey colorKey { 0, QModbusDataUnit::Invalid, 12 };
    QVERIFY(state.Colors.contains(colorKey));
    QCOMPARE(state.Colors.value(colorKey), QColor(QStringLiteral("#112233")));

    QCOMPARE(state.Data.value(10), quint16(100));
    QCOMPARE(state.Data.value(12), quint16(300));
}

///
/// \brief TestLegacyProjectParser::readsLegacyDisplayDefinitionDefaultsAndNormalizes
///
void TestLegacyProjectParser::readsLegacyDisplayDefinitionDefaultsAndNormalizes()
{
    const QByteArray xml = R"xml(
<FormModSim Title='Fallback Title'>
  <DisplayDefinition DeviceId='0' PointType='999' Length='0'
                     DataViewColumnsDistance='100' LeadingZeros='true'/>
</FormModSim>
)xml";

    const auto state = readLegacyDataViewState(xml);

    QCOMPARE(state.Definitions.FormName, QStringLiteral("Fallback Title"));
    QCOMPARE(state.Definitions.DeviceId, quint8(0));
    QCOMPARE(state.Definitions.PointType, QModbusDataUnit::HoldingRegisters);
    QCOMPARE(state.Definitions.PointAddress, quint16(1));
    QCOMPARE(state.Definitions.Length, quint16(1));
    QCOMPARE(state.Definitions.DataViewColumnsDistance, quint16(32));
    QCOMPARE(state.Definitions.LeadingZeros, true);
}

///
/// \brief TestLegacyProjectParser::preservesZeroBasedPointAddress
///
/// Regression test for issue #125: a 0-based version 1.x project stores
/// PointAddress='0', which normalize() would otherwise clamp up to 1.
///
void TestLegacyProjectParser::preservesZeroBasedPointAddress()
{
    const QByteArray xml = R"xml(
<FormModSim Title='Zero Based'>
  <DisplayDefinition DeviceId='1' PointType='4' PointAddress='0' Length='10'
                     DataViewColumnsDistance='16' LeadingZeros='true' ZeroBasedAddress='true'/>
</FormModSim>
)xml";

    const auto state = readLegacyDataViewState(xml);

    QCOMPARE(state.Definitions.PointAddress, quint16(0));
    QCOMPARE(state.Definitions.Length, quint16(10));
}

///
/// \brief TestLegacyProjectParser::readsLegacyColorFallbackKeys
///
void TestLegacyProjectParser::readsLegacyColorFallbackKeys()
{
    const QByteArray xml = R"xml(
<AddressColorMap>
  <Color Address='4' Value='#445566'/>
  <Color DeviceId='3' Type='4' Address='5' Value='#abcdef'/>
  <Color DeviceId='3' Type='4' Address='6'/>
  <Color DeviceId='3' Type='4' Address='bad' Value='#000000'/>
</AddressColorMap>
)xml";

    QXmlStreamReader reader(xml);
    reader.readNextStartElement();

    const auto map = LegacyProjectParser::readAddressColors(reader);

    QCOMPARE(map.size(), 2);
    const ItemMapKey fallbackKey { 0, QModbusDataUnit::Invalid, 4 };
    const ItemMapKey fullKey { 3, QModbusDataUnit::HoldingRegisters, 5 };
    QCOMPARE(map.value(fallbackKey), QColor(QStringLiteral("#445566")));
    QCOMPARE(map.value(fullKey), QColor(QStringLiteral("#abcdef")));
}

///
/// \brief TestLegacyProjectParser::ignoresMalformedLegacySimulationsAndValues
///
void TestLegacyProjectParser::ignoresMalformedLegacySimulationsAndValues()
{
    const QByteArray xml = R"xml(
<FormModSim Title='Malformed Entries'>
  <DisplayDefinition DeviceId='2' PointType='4' PointAddress='10' Length='2'/>
  <ModbusSimulationMap>
    <Simulation Address='bad'>
      <ModbusSimulationParams Mode='Increment' Interval='100'/>
    </Simulation>
    <Simulation Address='11'>
      <ModbusSimulationParams Mode='Decrement' Interval='200'/>
    </Simulation>
  </ModbusSimulationMap>
  <ModbusDataUnit>
    <Value Address='10'>12</Value>
    <Value Address='11'>bad</Value>
    <Value Address='12'>99</Value>
    <Value Address='bad'>7</Value>
  </ModbusDataUnit>
</FormModSim>
)xml";

    const auto state = readLegacyDataViewState(xml);

    QCOMPARE(state.Simulations.size(), 1);
    QVERIFY(state.Simulations.contains(11));
    QCOMPARE(state.Simulations.value(11).Mode, SimulationMode::Decrement);
    QCOMPARE(state.Simulations.value(11).Interval, 200u);

    QCOMPARE(state.Data.size(), 2);
    QCOMPARE(state.Data.value(10), quint16(12));
    QCOMPARE(state.Data.value(12), quint16(99));
}

///
/// \brief TestLegacyProjectParser::mapsDataUnitValuesWithoutStoppingAfterMalformedAddress
///
void TestLegacyProjectParser::mapsDataUnitValuesWithoutStoppingAfterMalformedAddress()
{
    DataViewDefinitions definitions;
    definitions.PointType = QModbusDataUnit::HoldingRegisters;
    definitions.PointAddress = 10;
    definitions.Length = 3;

    const QHash<quint16, quint16> data {
        { 9, 900 },
        { 10, 100 },
        { 12, 300 },
        { 13, 1300 }
    };

    const auto values = LegacyProjectParser::dataUnitValues(definitions, data);

    QCOMPARE(values.size(), 3);
    QCOMPARE(values.at(0), quint16(100));
    QCOMPARE(values.at(1), quint16(0));
    QCOMPARE(values.at(2), quint16(300));
}

///
/// \brief TestLegacyProjectParser::clampsCoilValues
///
void TestLegacyProjectParser::clampsCoilValues()
{
    DataViewDefinitions definitions;
    definitions.PointType = QModbusDataUnit::Coils;
    definitions.PointAddress = 1;
    definitions.Length = 3;

    const QHash<quint16, quint16> data {
        { 1, 0 },
        { 2, 1 },
        { 3, 42 }
    };

    const auto values = LegacyProjectParser::dataUnitValues(definitions, data);

    QCOMPARE(values.size(), 3);
    QCOMPARE(values.at(0), quint16(0));
    QCOMPARE(values.at(1), quint16(1));
    QCOMPARE(values.at(2), quint16(1));
}

QTEST_MAIN(TestLegacyProjectParser)

#include "test_legacyprojectparser.moc"
