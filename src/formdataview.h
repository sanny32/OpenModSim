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
    explicit FormDataView(int id, ModbusMultiServer& server, DataSimulator* simulator, MainWindow* parent);
    ~FormDataView();

    QVector<quint16> data() const;

    DataViewDefinitions displayDefinition() const;
    void setDisplayDefinition(const DataViewDefinitions& dd);

    ByteOrder byteOrder() const;
    void setByteOrder(ByteOrder order);

    QString codepage() const;
    void setCodepage(const QString& name);

    DataDisplayMode dataDisplayMode() const;
    void setDataDisplayMode(DataDisplayMode mode);

    bool displayHexAddresses() const;
    void setDisplayHexAddresses(bool on);

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& clr);

    QColor foregroundColor() const;
    void setForegroundColor(const QColor& clr);

    QColor statusColor() const;
    void setStatusColor(const QColor& clr);

    QFont font() const;
    void setFont(const QFont& font);

    int zoomPercent() const;
    void setZoomPercent(int zoomPercent);

    void print(QPrinter* painter);

    ModbusSimulationMap2 simulationMap() const;
    void startSimulation(QModbusDataUnit::RegisterType type, quint16 addr, const ModbusSimulationParams& params);

    QModbusDataUnit serializeModbusDataUnit(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const;
    void configureModbusDataUnit(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, const QVector<quint16>& values) const;

    AddressDescriptionMap2 descriptionMap() const;
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
    void connectEditSlots();
    void disconnectEditSlots();

signals:
    void showed();
    void closing();
    void helpContextRequested(const QString& helpKey);
    void byteOrderChanged(ByteOrder);
    void codepageChanged(const QString&);
    void definitionChanged();
    void pointTypeChanged(QModbusDataUnit::RegisterType);
    void dataDisplayModeChanged(DataDisplayMode);
    void displayHexAddressesChanged(bool);
    void fontChanged(const QFont&);
    void foregroundColorChanged(const QColor&);
    void backgroundColorChanged(const QColor&);
    void statusColorChanged(const QColor&);

private slots:
    void on_awake();
    void on_lineEditAddress_valueChanged(const QVariant&);
    void on_lineEditLength_valueChanged(const QVariant&);
    void on_lineEditDeviceId_valueChanged(const QVariant&, const QVariant&);
    void on_comboBoxAddressBase_addressBaseChanged(AddressBase base);
    void on_comboBoxModbusPointType_pointTypeChanged(QModbusDataUnit::RegisterType value);
    void on_outputWidget_itemDoubleClicked(quint16 addr, const QVariant& value);
    void on_mbDataChanged(quint8 deviceId, const QModbusDataUnit& data);
    void on_mbDefinitionsChanged(const ModbusDefinitions& defs);
    void on_simulationStarted(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, const QVector<quint16>& addresses);
    void on_simulationStopped(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, const QVector<quint16>& addresses);
    void on_dataSimulated(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, QVariant value);

private:
    void updateStatus();
    void reapplyFind();
    void onDefinitionChanged();
    void setDisplayDefinitionSilent(const DataViewDefinitions& dd);

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
    FindReplaceBar* _findReplaceBar = nullptr;

    AnsiMenu*  _ansiMenu = nullptr;
    QMap<DataDisplayMode, QAction*> _displayModeActions;
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

    out.setValue("Font", frm->font());
    out.setValue("ForegroundColor", frm->foregroundColor());
    out.setValue("BackgroundColor", frm->backgroundColor());
    out.setValue("StatusColor", frm->statusColor());
    out.setValue("ZoomPercent", frm->zoomPercent());

    const auto wnd = frm->parentWidget();
    out.setValue("ViewMinimized", wnd->isMinimized());
    out.setValue("ViewMaximized", wnd->isMaximized());
    out.setValue("ViewRect", wnd->geometry());

    out << frm->dataDisplayMode();
    out << frm->byteOrder();
    out << frm->displayDefinition();
    out.setValue("DisplayHexAddresses", frm->displayHexAddresses());
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

    DataDisplayMode dataDisplayMode;
    in >> dataDisplayMode;

    ByteOrder byteOrder;
    in >> byteOrder;

    DataViewDefinitions displayDefinition;
    in >> displayDefinition;


    AddressDescriptionMap2 descriptionMap;
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
    frm->setFont(in.value("Font", defaultMonospaceFont()).value<QFont>());
    frm->setForegroundColor(in.value("ForegroundColor", QColor(Qt::black)).value<QColor>());
    frm->setBackgroundColor(in.value("BackgroundColor", QColor(Qt::white)).value<QColor>());
    frm->setStatusColor(in.value("StatusColor", QColor(Qt::red)).value<QColor>());
    frm->setZoomPercent(in.value("ZoomPercent", 100).toInt());
    if (wnd && wndRect.isValid() && !wnd->isMaximized() && !wnd->isMinimized())
        wnd->setGeometry(wndRect);
    if(isMinimized) wnd->setWindowState(Qt::WindowMinimized);
    if(isMaximized) wnd->setWindowState(Qt::WindowMaximized);

    frm->setDataDisplayMode(dataDisplayMode);
    frm->setByteOrder(byteOrder);
    frm->setDisplayDefinition(displayDefinition);

    frm->setDisplayHexAddresses(in.value("DisplayHexAddresses").toBool());
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
inline QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, FormDataView* frm)
{
    if (!frm) return xml;

    xml.writeStartElement("FormDataView");

    const auto panel = frm->property("SplitPanel").toString();
    if(!panel.isEmpty())
        xml.writeAttribute("Panel", panel);
    if(frm->property("Closed").toBool())
        xml.writeAttribute("Closed", "1");
    xml.writeAttribute("DataDisplayMode", enumToString<DataDisplayMode>(frm->dataDisplayMode()));
    xml.writeAttribute("DisplayHexAddresses", boolToString(frm->displayHexAddresses()));
    xml.writeAttribute("Codepage", frm->codepage());
    xml.writeAttribute("ByteOrder", enumToString<ByteOrder>(frm->byteOrder()));

    const auto wnd = frm->parentWidget();
    xml.writeStartElement("Window");
    xml.writeAttribute("Maximized", boolToString(wnd->isMaximized()));
    xml.writeAttribute("Minimized", boolToString(wnd->isMinimized()));

    const auto windowPos = wnd->pos();
    xml.writeAttribute("Left", QString::number(windowPos.x()));
    xml.writeAttribute("Top", QString::number(windowPos.y()));

    const auto windowSize = (wnd->isMinimized() || wnd->isMaximized()) ? wnd->sizeHint() : wnd->size();
    xml.writeAttribute("Width", QString::number(windowSize.width()));
    xml.writeAttribute("Height", QString::number(windowSize.height()));
    xml.writeEndElement();

    xml.writeStartElement("Colors");
    xml.writeAttribute("Background", frm->backgroundColor().name());
    xml.writeAttribute("Foreground", frm->foregroundColor().name());
    xml.writeAttribute("Status", frm->statusColor().name());
    xml.writeEndElement();

    xml.writeStartElement("Font");
    const QFont font = frm->font();
    xml.writeAttribute("Family", font.family());
    xml.writeAttribute("Size", QString::number(font.pointSize()));
    xml.writeAttribute("Bold", boolToString(font.bold()));
    xml.writeAttribute("Italic", boolToString(font.italic()));
    xml.writeEndElement();

    xml.writeStartElement("Zoom");
    xml.writeAttribute("Value", QString("%1%").arg(frm->zoomPercent()));
    xml.writeEndElement();

    const auto dd = frm->displayDefinition();
    xml << dd;

    {
        const auto simulationMap = frm->simulationMap();
        xml.writeStartElement("ModbusSimulationMap");

        for (auto it = simulationMap.constBegin(); it != simulationMap.constEnd(); ++it) {
            const ModbusSimulationMapKey& key = it.key();
            const ModbusSimulationParams& params = it.value();

            if(params.Mode != SimulationMode::Off && params.Mode != SimulationMode::Disabled && key.DeviceId == dd.DeviceId && key.Type == dd.PointType)
            {
                xml.writeStartElement("Simulation");
                xml.writeAttribute("Address", QString::number(key.Address + (dd.ZeroBasedAddress ? 0 : 1)));
                xml << params;
                xml.writeEndElement();
            }
        }

        xml.writeEndElement(); // ModbusSimulationMap
    }

    xml << frm->descriptionMap();
    xml << frm->colorMap();

    {
        const auto unit = frm->serializeModbusDataUnit(dd.DeviceId, dd.PointType, dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1), dd.Length);
        xml.writeStartElement("ModbusDataUnit");

        quint16 address = dd.PointAddress;
        const auto values = unit.values();
        for (const auto& value : values) {
            if(value != 0) {
                xml.writeStartElement("Value");
                xml.writeAttribute("Address", QString::number(address));
                xml.writeCharacters(QString::number(value));
                xml.writeEndElement();
            }
            address++;
        }

        xml.writeEndElement(); // ModbusDataUnit
    }

    xml.writeEndElement(); // FormDataView

    return xml;
}

///
/// \brief operator >>
/// \param xml
/// \param frm
/// \return
///
inline QXmlStreamReader& operator >>(QXmlStreamReader& xml, FormDataView* frm)
{
    if (!frm) return xml;

    if (xml.isStartElement() && xml.name() == QLatin1String("FormDataView")) {
        DataDisplayMode ddm;
        DataViewDefinitions dd;
        QHash<quint16, quint16> data;
        QHash<quint16, ModbusSimulationParams> simulations;

        const QXmlStreamAttributes attributes = xml.attributes();

        if (attributes.hasAttribute("DataDisplayMode")) {
            ddm = enumFromString<DataDisplayMode>(attributes.value("DataDisplayMode").toString());
        }

        if (attributes.hasAttribute("DisplayHexAddresses")) {
            const bool displayHex = stringToBool(attributes.value("DisplayHexAddresses").toString());
            frm->setDisplayHexAddresses(displayHex);
        }

        if (attributes.hasAttribute("Codepage")) {
            frm->setCodepage(attributes.value("Codepage").toString());
        }

        if (attributes.hasAttribute("ByteOrder")) {
            const ByteOrder order = enumFromString<ByteOrder>(attributes.value("ByteOrder").toString());
            frm->setByteOrder(order);
        }

        while (xml.readNextStartElement()) {
            if (xml.name() == QLatin1String("Window")) {
                const QXmlStreamAttributes windowAttrs = xml.attributes();

                const auto wnd = frm->parentWidget();
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
            else if (xml.name() == QLatin1String("Colors")) {
                const QXmlStreamAttributes colorAttrs = xml.attributes();

                if (colorAttrs.hasAttribute("Background")) {
                    QColor color(colorAttrs.value("Background").toString());
                    if (color.isValid()) frm->setBackgroundColor(color);
                }

                if (colorAttrs.hasAttribute("Foreground")) {
                    QColor color(colorAttrs.value("Foreground").toString());
                    if (color.isValid()) frm->setForegroundColor(color);
                }

                if (colorAttrs.hasAttribute("Status")) {
                    QColor color(colorAttrs.value("Status").toString());
                    if (color.isValid()) frm->setStatusColor(color);
                }
                xml.skipCurrentElement();
            }
            else if (xml.name() == QLatin1String("Font")) {
                const QXmlStreamAttributes fontAttrs = xml.attributes();

                QFont font = frm->font();

                if (fontAttrs.hasAttribute("Family")) {
                    font.setFamily(fontAttrs.value("Family").toString());
                }

                if (fontAttrs.hasAttribute("Size")) {
                    bool ok; const int size = fontAttrs.value("Size").toInt(&ok);
                    if (ok && size > 0) font.setPointSize(size);
                }

                if (fontAttrs.hasAttribute("Bold")) {
                    font.setBold(stringToBool(fontAttrs.value("Bold").toString()));
                }

                if (fontAttrs.hasAttribute("Italic")) {
                    font.setItalic(stringToBool(fontAttrs.value("Italic").toString()));
                }

                frm->setFont(font);
                xml.skipCurrentElement();
            }
            else if(xml.name() == QLatin1String("Zoom")) {
                const QXmlStreamAttributes zoomAttrs = xml.attributes();
                if (zoomAttrs.hasAttribute("Value")) {
                    bool ok; const int zoom = zoomAttrs.value("Value").toString().remove("%").toInt(&ok);
                    if(ok) frm->setZoomPercent(zoom);
                    xml.skipCurrentElement();
                }
            }
            else if (xml.name() == QLatin1String("DataViewDefinitions")) {
                xml >> dd;
                xml.skipCurrentElement();
                frm->setDisplayDefinition(dd);
            }
            else if (xml.name() == QLatin1String("ModbusSimulationMap")) {
                while (xml.readNextStartElement()) {
                    if (xml.name() == QLatin1String("Simulation")) {

                        const QXmlStreamAttributes attributes = xml.attributes();
                        bool ok; const quint16 address = attributes.value("Address").toUShort(&ok);

                        if(ok) {
                            xml.readNextStartElement();

                            ModbusSimulationParams params;
                            xml >> params;

                            simulations[address] = params;
                        }

                        xml.skipCurrentElement();

                    } else {
                        xml.skipCurrentElement();
                    }
                }
            }
            else if (xml.name() == QLatin1String("JScriptControl")) {
                xml.skipCurrentElement();
            }
            else if (xml.name() == QLatin1String("AddressDescriptionMap")) {
                AddressDescriptionMap2 map;
                xml >> map;
                for(auto it = map.cbegin(); it != map.cend(); ++it)
                {
                    const auto device_id = it.key().DeviceId;
                    const auto type = it.key().Type;
                    frm->setDescription(device_id ? device_id : dd.DeviceId, type ? type : dd.PointType, it.key().Address, it.value());
                }
            }
            else if (xml.name() == QLatin1String("AddressColorMap")) {
                AddressColorMap map;
                xml >> map;
                for(auto it = map.cbegin(); it != map.cend(); ++it)
                {
                    const auto device_id = it.key().DeviceId;
                    const auto type = it.key().Type;
                    frm->setColor(device_id ? device_id : dd.DeviceId, type ? type : dd.PointType, it.key().Address, it.value());
                }
            }
            else if (xml.name() == QLatin1String("ModbusDataUnit")) {
                while (xml.readNextStartElement()) {
                    if (xml.name() == QLatin1String("Value")) {
                        QXmlStreamAttributes attributes = xml.attributes();
                        bool ok; const quint16 address = attributes.value("Address").toUShort(&ok);
                        if(ok) {
                            const quint16 value = xml.readElementText().toUShort(&ok);
                            if (ok) {
                                data[address] = value;
                            }
                        }
                    } else {
                        xml.skipCurrentElement();
                    }
                }
            }
            else {
                xml.skipCurrentElement();
            }
        }

        if(dd.PointType != QModbusDataUnit::Invalid) {
            frm->setDataDisplayMode(ddm);

            if(!simulations.isEmpty()) {
                QHashIterator it(simulations);
                while(it.hasNext()) {
                    const auto item = it.next();
                    switch(dd.PointType) {
                        case QModbusDataUnit::Coils:
                        case QModbusDataUnit::DiscreteInputs:
                            if(item->Mode == SimulationMode::Toggle || item->Mode == SimulationMode::Random)
                                frm->startSimulation(dd.PointType, item.key() - (dd.ZeroBasedAddress ? 0 : 1), item.value());
                            break;
                        case QModbusDataUnit::InputRegisters:
                        case QModbusDataUnit::HoldingRegisters:
                            if(item->Mode != SimulationMode::Off && item->Mode != SimulationMode::Toggle)
                                frm->startSimulation(dd.PointType, item.key() - (dd.ZeroBasedAddress ? 0 : 1), item.value());
                            break;
                        default: break;
                    }
                }
            }

            if (!data.isEmpty()) {
                QVector<quint16> values(dd.Length);

                QHashIterator it(data);
                while(it.hasNext()) {
                    const auto item = it.next();
                    const auto index = item.key() - dd.PointAddress;
                    switch(dd.PointType) {
                        case QModbusDataUnit::Coils:
                        case QModbusDataUnit::DiscreteInputs:
                            values[index] = qBound<quint16>(0, item.value(), 1);
                            break;
                        case QModbusDataUnit::InputRegisters:
                        case QModbusDataUnit::HoldingRegisters:
                            values[index] = item.value();
                            break;
                        default: break;
                    }
                    if(index >= values.length()) break;
                }

                frm->configureModbusDataUnit(dd.DeviceId, dd.PointType, dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1), values);
            }
        }
    }
    else {
        xml.skipCurrentElement();
    }

    return xml;
}

#endif // FORMDATAVIEW_H


