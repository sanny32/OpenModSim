// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file formdataview.h
/// \brief Declares the formdataview interfaces.
///

#ifndef FORMDATAVIEW_H
#define FORMDATAVIEW_H

#include <QWidget>
#include <QTimer>
#include <QPrinter>
#include <QActionGroup>
#include <QMap>
#include <QXmlStreamWriter>
#include "fontutils.h"
#include "datasimulator.h"
#include "modbusmultiserver.h"
#include "displaydefinition.h"
#include "outputtypes.h"
#include "ansimenu.h"

///
/// \brief Forward declaration of the MainWindow
///
class MainWindow;
class FindReplaceBar;

namespace Ui {
class FormDataView;
}

///
/// \brief The FormDataView class
///
class FormDataView : public QWidget
{
    Q_OBJECT

    friend QSettings& operator <<(QSettings& out, FormDataView* frm);
    friend QSettings& operator >>(QSettings& in, FormDataView* frm);

    friend QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, FormDataView* frm);
    friend QXmlStreamReader& operator >>(QXmlStreamReader& xml, FormDataView* frm);

public:
    explicit FormDataView(ModbusMultiServer& server, DataSimulator* simulator, MainWindow* parent);
    ~FormDataView();

    QVector<quint16> data() const;

    DataViewDefinitions displayDefinition() const;
    void setDisplayDefinition(const DataViewDefinitions& dd);

    ByteOrder byteOrder() const;
    void setByteOrder(ByteOrder order);

    QString codepage() const;
    void setCodepage(const QString& name);

    DataType dataType() const;
    void setDataType(DataType type);

    RegisterOrder registerOrder() const;
    void setRegisterOrder(RegisterOrder order);

    bool zeroBasedAddress() const;
    void setAddressBase(AddressBase base);

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& clr);

    QColor foregroundColor() const;
    void setForegroundColor(const QColor& clr);

    QColor addressColor() const;
    void setAddressColor(const QColor& clr);

    QColor commentColor() const;
    void setCommentColor(const QColor& clr);

    QFont font() const;
    void setFont(const QFont& font);

    int zoomPercent() const;
    void setZoomPercent(int zoomPercent);

    void print(QPrinter* painter);

    ModbusSimulationMap2 simulationMap() const;
    void startSimulation(QModbusDataUnit::RegisterType type, quint16 addr, const ModbusSimulationParams& params);

    QModbusDataUnit serializeModbusDataUnit(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const;
    void configureModbusDataUnit(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, const QVector<quint16>& values) const;

    AddressDescriptionMap descriptionMap() const;
    void setDescription(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc);

    AddressColorMap colorMap() const;
    void setColor(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QColor& clr);

    void linkTo(FormDataView* other);

    void saveSettings(QSettings& out) const;
    void loadSettings(QSettings& in);
    void saveXml(QXmlStreamWriter& xml) const;
    void loadXml(QXmlStreamReader& xml);

protected:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

public slots:
    void show();
    void showFind();

signals:
    void showed();
    void closing();
    void helpContextRequested(const QString& helpKey);
    void byteOrderChanged(ByteOrder);
    void codepageChanged(const QString&);
    void definitionChanged();
    void pointTypeChanged(QModbusDataUnit::RegisterType);
    void dataTypeChanged(DataType);
    void registerOrderChanged(RegisterOrder);
    void fontChanged(const QFont&);
    void foregroundColorChanged(const QColor&);
    void backgroundColorChanged(const QColor&);
    void addressColorChanged(const QColor&);
    void commentColorChanged(const QColor&);
    void colorChanged(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QColor& clr);
    void descriptionChanged(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc);

private slots:
    void on_lineEditAddress_valueChanged(const QVariant&);
    void on_lineEditLength_valueChanged(const QVariant&);
    void on_lineEditDeviceId_valueChanged(const QVariant&, const QVariant&);
    void on_comboBoxModbusPointType_pointTypeChanged(QModbusDataUnit::RegisterType value);
    void on_outputWidget_itemDoubleClicked(quint16 addr, const QVariant& value);
    void on_mbDataChanged(quint8 deviceId, const QModbusDataUnit& data);
    void on_mbDefinitionsChanged(const ModbusDefinitions& defs);
    void on_mbDescriptionChanged(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc);
    void on_simulationStarted(DataType type, RegisterOrder order, quint8 deviceId, QModbusDataUnit::RegisterType regType, const QVector<quint16>& addresses);
    void on_simulationStopped(DataType type, RegisterOrder order, quint8 deviceId, QModbusDataUnit::RegisterType regType, const QVector<quint16>& addresses);
    void on_dataSimulated(DataType type, RegisterOrder order, quint8 deviceId, QModbusDataUnit::RegisterType regType, quint16 startAddress, QVariant value);
    void on_dataViewFlushTimeout();

private:
    void updateStatus();
    void refreshDisplayedData();
    void reapplyFind();
    void syncDescriptionsFromServer();
    void onDefinitionChanged();
    void setDisplayDefinitionSilent(const DataViewDefinitions& dd);
    void applyGlobalHexView(bool enabled);

    void setupDisplayBar();
    void updateSettingsControls();
    void setLeadingZerosEnabled(bool on);
    void setColumnsDistance(int value);
    void updateDisplayBar();

private:
    Ui::FormDataView *ui;
    MainWindow* _parent;
    ModbusMultiServer& _mbMultiServer;
    DataSimulator* _dataSimulator;
    bool _initialMapSynced = false;
    FindReplaceBar* _findReplaceBar = nullptr;
    QTimer* _dataViewFlushTimer = nullptr;

    AnsiMenu*  _ansiMenu = nullptr;
    QMap<QPair<DataType, RegisterOrder>, QAction*> _displayModeActions;
};

///
/// \brief operator <<
/// \param out
/// \param frm
/// \return
///
inline QSettings& operator <<(QSettings& out, FormDataView* frm)
{
    if(!frm) return out;

    const auto wnd = frm->parentWidget();
    out.setValue("ViewMinimized", wnd->isMinimized());
    out.setValue("ViewMaximized", wnd->isMaximized());
    out.setValue("ViewRect", wnd->geometry());

    out << frm->dataType();
    out << frm->registerOrder();
    out << frm->byteOrder();
    out << frm->displayDefinition();
    out.setValue("Codepage", frm->codepage());
    out << frm->descriptionMap();
    out << frm->colorMap();

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param frm
/// \return
///
inline QSettings& operator >>(QSettings& in, FormDataView* frm)
{
    if(!frm) return in;

    DataType dataType;
    in >> dataType;

    RegisterOrder regOrder;
    in >> regOrder;

    ByteOrder byteOrder;
    in >> byteOrder;

    DataViewDefinitions displayDefinition;
    in >> displayDefinition;


    AddressDescriptionMap descriptionMap;
    in >> descriptionMap;

    AddressColorMap colorMap;
    in >> colorMap;

    bool isMinimized;
    isMinimized = in.value("ViewMinimized").toBool();
    bool isMaximized;
    isMaximized = in.value("ViewMaximized").toBool();

    QRect wndRect;
    wndRect = in.value("ViewRect").toRect();

    auto wnd = frm->parentWidget();
    if (wnd && wndRect.isValid() && !wnd->isMaximized() && !wnd->isMinimized())
        wnd->setGeometry(wndRect);
    if(isMinimized) wnd->setWindowState(Qt::WindowMinimized);
    if(isMaximized) wnd->setWindowState(Qt::WindowMaximized);

    frm->setDataType(dataType);
    frm->setRegisterOrder(regOrder);
    frm->setByteOrder(byteOrder);
    frm->setDisplayDefinition(displayDefinition);
    frm->setCodepage(in.value("Codepage").toString());

    for(auto it = descriptionMap.cbegin(); it != descriptionMap.cend(); ++it)
    {
        frm->setDescription(it.key().DeviceId, it.key().Type, it.key().Address, it.value());
    }
    for(auto it = colorMap.cbegin(); it != colorMap.cend(); ++it)
    {
        frm->setColor(it.key().DeviceId, it.key().Type, it.key().Address, it.value());
    }

    return in;
}

///
/// \brief operator <<
/// \param xml
/// \param frm
/// \return
///
QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, FormDataView* frm);

///
/// \brief operator >>
/// \param xml
/// \param frm
/// \return
///
QXmlStreamReader& operator >>(QXmlStreamReader& xml, FormDataView* frm);

#endif // FORMDATAVIEW_H



