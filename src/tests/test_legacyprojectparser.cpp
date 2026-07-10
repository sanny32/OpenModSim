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
