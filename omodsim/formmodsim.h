#ifndef FORMMODSIM_H
#define FORMMODSIM_H

#include <QWidget>
#include <QTimer>
#include <QPrinter>
#include <QVersionNumber>
#include <QXmlStreamWriter>
#include "fontutils.h"
#include "datasimulator.h"
#include "modbusmultiserver.h"
#include "displaydefinition.h"
#include "outputwidget.h"
#include "jscriptcontrol.h"
#include "scriptsettings.h"

///
/// \brief Forward declaration of the MainWindow
///
class MainWindow;

namespace Ui {
class FormModSim;
}

///
/// \brief The FormModSim class
///
class FormModSim : public QWidget
{
    Q_OBJECT

    friend QSettings& operator <<(QSettings& out, FormModSim* frm);
    friend QSettings& operator >>(QSettings& in, FormModSim* frm);

    friend QDataStream& operator <<(QDataStream& out, FormModSim* frm);
    friend QDataStream& operator >>(QDataStream& in, FormModSim* frm);

    friend QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, FormModSim* frm);
    friend QXmlStreamReader& operator >>(QXmlStreamReader& xml, FormModSim* frm);

public:
    static QVersionNumber VERSION;

    explicit FormModSim(int id, ModbusMultiServer& server, QSharedPointer<DataSimulator> simulator, MainWindow* parent);
    ~FormModSim();

    int formId() const { return _formId; }

    QString filename() const;
    void setFilename(const QString& filename);

    QVector<quint16> data() const;

    DisplayDefinition displayDefinition() const;
    void setDisplayDefinition(const DisplayDefinition& dd);

    ByteOrder byteOrder() const;
    void setByteOrder(ByteOrder order);

    QString codepage() const;
    void setCodepage(const QString& name);

    DisplayMode displayMode() const;
    void setDisplayMode(DisplayMode mode);

    DataDisplayMode dataDisplayMode() const;
    void setDataDisplayMode(DataDisplayMode mode);

    ScriptSettings scriptSettings() const;
    void setScriptSettings(const ScriptSettings& ss);

    QString script() const;
    void setScript(const QString& text);

    QString searchText() const;

    bool displayHexAddresses() const;
    void setDisplayHexAddresses(bool on);

    CaptureMode captureMode() const;
    void startTextCapture(const QString& file);
    void stopTextCapture();

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& clr);

    QColor foregroundColor() const;
    void setForegroundColor(const QColor& clr);

    QColor statusColor() const;
    void setStatusColor(const QColor& clr);

    QFont font() const;
    void setFont(const QFont& font);

    void print(QPrinter* painter);

    ModbusSimulationMap2 simulationMap() const;
    void startSimulation(QModbusDataUnit::RegisterType type, quint16 addr, const ModbusSimulationParams& params);

    QModbusDataUnit serializeModbusDataUnit(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const;
    void configureModbusDataUnit(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, const QVector<quint16>& values) const;

    AddressDescriptionMap2 descriptionMap() const;
    void setDescription(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc);

    bool canRunScript() const;
    bool canStopScript() const;

    bool canUndo() const;
    bool canRedo() const;
    bool canPaste() const;

    void runScript();
    void stopScript();

    LogViewState logViewState() const;
    void setLogViewState(LogViewState state);

protected:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

public slots:
    void show();
    void connectEditSlots();
    void disconnectEditSlots();

signals:
    void showed();
    void closing();
    void byteOrderChanged(ByteOrder);
    void codepageChanged(const QString&);
    void pointTypeChanged(QModbusDataUnit::RegisterType);
    void displayModeChanged(DisplayMode mode);
    void scriptSettingsChanged(const ScriptSettings&);
    void captureError(const QString& error);
    void doubleClicked();

private slots:
    void on_lineEditAddress_valueChanged(const QVariant&);
    void on_lineEditLength_valueChanged(const QVariant&);
    void on_lineEditDeviceId_valueChanged(const QVariant&, const QVariant&);
    void on_comboBoxAddressBase_addressBaseChanged(AddressBase base);
    void on_comboBoxModbusPointType_pointTypeChanged(QModbusDataUnit::RegisterType value);
    void on_outputWidget_itemDoubleClicked(quint16 addr, const QVariant& value);
    void on_mbConnected(const ConnectionDetails& cd);
    void on_mbDisconnected(const ConnectionDetails& cd);
    void on_mbRequest(QSharedPointer<const ModbusMessage> msg);
    void on_mbResponse(QSharedPointer<const ModbusMessage> msgReq, QSharedPointer<const ModbusMessage> msgResp);
    void on_mbDataChanged(quint8 deviceId, const QModbusDataUnit& data);
    void on_mbDefinitionsChanged(const ModbusDefinitions& defs);
    void on_simulationStarted(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr);
    void on_simulationStopped(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr);
    void on_dataSimulated(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, QVariant value);

private:
    void updateStatus();
    void onDefinitionChanged();
    JScriptControl* scriptControl();
    bool isLoggingRequest(QSharedPointer<const ModbusMessage> msgReq) const;

private:
    Ui::FormModSim *ui;
    MainWindow* _parent;
    int _formId;
    QString _filename;
    bool _verboseLogging;
    ScriptSettings _scriptSettings;
    ModbusMultiServer& _mbMultiServer;
    QSharedPointer<DataSimulator> _dataSimulator;
};

///
/// \brief operator <<
/// \param out
/// \param frm
/// \return
///
inline QSettings& operator <<(QSettings& out, FormModSim* frm)
{
    if(!frm) return out;

    out.setValue("FormVersion", FormModSim::VERSION.toString());
    out.setValue("Font", frm->font());
    out.setValue("ForegroundColor", frm->foregroundColor());
    out.setValue("BackgroundColor", frm->backgroundColor());
    out.setValue("StatusColor", frm->statusColor());

    const auto wnd = frm->parentWidget();
    out.setValue("ViewMaximized", wnd->isMaximized());
    if(!wnd->isMaximized() && !wnd->isMinimized())
    {
        out.setValue("ViewSize", wnd->size());
    }

    out << frm->displayMode();
    out << frm->dataDisplayMode();
    out << frm->byteOrder();
    out << frm->displayDefinition();
    out.setValue("DisplayHexAddresses", frm->displayHexAddresses());
    out.setValue("Codepage", frm->codepage());
    out << frm->scriptSettings();
    out << frm->scriptControl();

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param frm
/// \return
///
inline QSettings& operator >>(QSettings& in, FormModSim* frm)
{
    if(!frm) return in;

    QVersionNumber version;
    version = QVersionNumber::fromString(in.value("FormVersion").toString());

    DisplayMode displayMode;
    in >> displayMode;

    DataDisplayMode dataDisplayMode;
    in >> dataDisplayMode;

    ByteOrder byteOrder;
    in >> byteOrder;

    DisplayDefinition displayDefinition;
    in >> displayDefinition;

    ScriptSettings scriptSettings;
    in >> scriptSettings;

    in >> frm->scriptControl();

    bool isMaximized;
    isMaximized = in.value("ViewMaximized").toBool();

    QSize wndSize;
    wndSize = in.value("ViewSize").toSize();

    auto wnd = frm->parentWidget();
    if(!version.isNull() || version >= QVersionNumber(1, 8)) {
        frm->setFont(in.value("Font", defaultMonospaceFont()).value<QFont>());
        frm->setForegroundColor(in.value("ForegroundColor", QColor(Qt::black)).value<QColor>());
        frm->setBackgroundColor(in.value("BackgroundColor", QColor(Qt::white)).value<QColor>());
        frm->setStatusColor(in.value("StatusColor", QColor(Qt::red)).value<QColor>());
    }

    wnd->resize(wndSize);
    if(isMaximized) wnd->setWindowState(Qt::WindowMaximized);

    frm->setDisplayMode(displayMode);
    frm->setDataDisplayMode(dataDisplayMode);
    frm->setByteOrder(byteOrder);
    frm->setDisplayDefinition(displayDefinition);
    frm->setDisplayHexAddresses(in.value("DisplayHexAddresses").toBool());
    frm->setCodepage(in.value("Codepage").toString());
    frm->setScriptSettings(scriptSettings);

    if(scriptSettings.RunOnStartup) {
        frm->runScript();
    }

    return in;
}

///
/// \brief operator <<
/// \param out
/// \param frm
/// \return
///
inline QDataStream& operator <<(QDataStream& out, FormModSim* frm)
{
    if(!frm) return out;

    out << frm->formId();

    const auto wnd = frm->parentWidget();
    out << wnd->isMaximized();

    const auto windowSize = (wnd->isMinimized() || wnd->isMaximized()) ? wnd->sizeHint() : wnd->size();
    out << windowSize;

    out << frm->displayMode();
    out << frm->dataDisplayMode();
    out << frm->displayHexAddresses();

    out << frm->backgroundColor();
    out << frm->foregroundColor();
    out << frm->statusColor();
    out << frm->font();

    const auto dd = frm->displayDefinition();
    out << dd.FormName;
    out << dd.DeviceId;
    out << dd.PointType;
    out << dd.PointAddress;
    out << dd.Length;
    out << dd.LogViewLimit;
    out << dd.ZeroBasedAddress;
    out << dd.AutoscrollLog;
    out << dd.VerboseLogging;
    out << dd.DataViewColumnsDistance;
    out << dd.LeadingZeros;

    out << frm->byteOrder();
    out << frm->simulationMap();
    out << frm->scriptControl();
    out << frm->scriptSettings();
    out << frm->descriptionMap();
    out << frm->codepage();

    const auto unit = frm->serializeModbusDataUnit(dd.DeviceId, dd.PointType, dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1), dd.Length);
    out << unit.registerType();
    out << unit.startAddress();
    out << unit.values();

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param frm
/// \return
///
inline QDataStream& operator >>(QDataStream& in, FormModSim* frm)
{
    if(!frm) return in;
    const auto ver = frm->property("Version").value<QVersionNumber>();

    bool isMaximized;
    in >> isMaximized;

    QSize windowSize;
    in >> windowSize;

    DisplayMode displayMode;
    in >> displayMode;

    DataDisplayMode dataDisplayMode;
    in >> dataDisplayMode;

    bool hexAddresses;
    in >> hexAddresses;

    QColor bkgClr;
    in >> bkgClr;

    QColor fgClr;
    in >> fgClr;

    QColor stCrl;
    in >> stCrl;

    QFont font;
    in >> font;

    DisplayDefinition dd;
    bool UseGlobalUnitMap;

    if(ver >= QVersionNumber(1,11))
    {
        in >> dd.FormName;
    }
    if(ver >= QVersionNumber(1, 5))
    {
        in >> dd.DeviceId;
        in >> dd.PointType;
        in >> dd.PointAddress;
        in >> dd.Length;
        in >> dd.LogViewLimit;
    }
    if(ver >= QVersionNumber(1, 6))
    {
        in >> dd.ZeroBasedAddress;
    }

    if(ver >= QVersionNumber(1, 9))
    {
        if(ver < QVersionNumber(1, 11)) {
            in >> UseGlobalUnitMap;
        }

        in >> dd.AutoscrollLog;
        in >> dd.VerboseLogging;

        if(ver >= QVersionNumber(1, 11)) {
            in >> dd.DataViewColumnsDistance;
        }

        if(ver >= QVersionNumber(1, 12)) {
            in >> dd.LeadingZeros;
        }
    }

    ModbusSimulationMap simulationMap;
    ModbusSimulationMap2 simulationMap2;
    ByteOrder byteOrder = ByteOrder::Direct;
    if(ver >= QVersionNumber(1, 1))
    {
        in >> byteOrder;

        if(ver >= QVersionNumber(1, 10)) {
            in >> simulationMap2;
        }
        else {
            in >> simulationMap;
        }
    }

    ScriptSettings scriptSettings;
    if(ver >=  QVersionNumber(1, 2))
    {
        in >> frm->scriptControl();

        if(ver >= QVersionNumber(1, 8)) {
            in >> scriptSettings;
        }
        else {
            in >> scriptSettings.Mode;
            in >> scriptSettings.Interval;
            in >> scriptSettings.UseAutoComplete;
        }
    }

    AddressDescriptionMap descriptionMap;
    AddressDescriptionMap2 descriptionMap2;
    if(ver >=  QVersionNumber(1, 3))
    {
        if(ver >= QVersionNumber(1, 10)) {
            in >> descriptionMap2;
        }
        else {
            in >> descriptionMap;
        }
    }

    QString codepage;
    if(ver >= QVersionNumber(1, 7))
    {
        in >> codepage;
    }

    if(in.status() != QDataStream::Ok)
        return in;

    auto wnd = frm->parentWidget();
    wnd->resize(windowSize);
    wnd->setWindowState(Qt::WindowActive);
    if(isMaximized) wnd->setWindowState(Qt::WindowMaximized);
    else wnd->resize(windowSize);

    frm->setDisplayMode(displayMode);
    frm->setDataDisplayMode(dataDisplayMode);
    frm->setDisplayHexAddresses(hexAddresses);
    frm->setBackgroundColor(bkgClr);
    frm->setForegroundColor(fgClr);
    frm->setStatusColor(stCrl);
    frm->setFont(font);
    frm->setDisplayDefinition(dd);
    frm->setByteOrder(byteOrder);
    frm->setCodepage(codepage);
    frm->setScriptSettings(scriptSettings);

    if(ver >= QVersionNumber(1,10)) {
        for(auto&& k : simulationMap2.keys())
            frm->startSimulation(k.Type, k.Address, simulationMap2[k]);

        for(auto&& k : descriptionMap2.keys())
            frm->setDescription(k.DeviceId, k.Type, k.Address, descriptionMap2[k]);
    }
    else {
        for(auto&& k : simulationMap.keys())
            frm->startSimulation(k.first, k.second, simulationMap[k]);

        for(auto&& k : descriptionMap.keys())
            frm->setDescription(dd.DeviceId, k.first, k.second, descriptionMap[k]);
    }

    if(ver >= QVersionNumber(1, 7))
    {
        QModbusDataUnit::RegisterType type;
        int startAddress;
        QVector<quint16> values;

        in >> type;
        in >> startAddress;
        in >> values;

        frm->configureModbusDataUnit(dd.DeviceId, type, startAddress, values);
    }

    return in;
}

///
/// \brief operator <<
/// \param xml
/// \param frm
/// \return
///
inline QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, FormModSim* frm)
{
    if (!frm) return xml;

    xml.writeStartElement("FormModSim");

    xml.writeAttribute("Version", FormModSim::VERSION.toString());
    xml.writeAttribute("DisplayMode", enumToString<DisplayMode>(frm->displayMode()));
    xml.writeAttribute("DataDisplayMode", enumToString<DataDisplayMode>(frm->dataDisplayMode()));
    xml.writeAttribute("DisplayHexAddresses", boolToString(frm->displayHexAddresses()));
    xml.writeAttribute("Codepage", frm->codepage());
    xml.writeAttribute("ByteOrder", enumToString<ByteOrder>(frm->byteOrder()));

    const auto wnd = frm->parentWidget();
    xml.writeStartElement("Window");
    xml.writeAttribute("Maximized", boolToString(wnd->isMaximized()));
    xml.writeAttribute("Minimized", boolToString(wnd->isMinimized()));

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

    const auto dd = frm->displayDefinition();
    xml << dd;

    {
        const auto simulationMap = frm->simulationMap();
        xml.writeStartElement("ModbusSimulationMap");

        for (auto it = simulationMap.constBegin(); it != simulationMap.constEnd(); ++it) {
            const ModbusSimulationMapKey& key = it.key();
            const ModbusSimulationParams& params = it.value();

            if(params.Mode != SimulationMode::No && key.DeviceId == dd.DeviceId && key.Type == dd.PointType)
            {
                xml.writeStartElement("Simulation");
                xml.writeAttribute("Address", QString::number(key.Address + (dd.ZeroBasedAddress ? 0 : 1)));
                xml << params;
                xml.writeEndElement();
            }
        }

        xml.writeEndElement(); // ModbusSimulationMap
    }

    xml << frm->scriptControl();
    xml << frm->scriptSettings();

    {
        const auto descriptionMap = frm->descriptionMap();
        xml.writeStartElement("AddressDescriptionMap");

        for (auto it = descriptionMap.constBegin(); it != descriptionMap.constEnd(); ++it) {
            const AddressDescriptionMapKey& key = it.key();
            const QString& description = it.value();

            if(!description.isEmpty() && key.DeviceId == dd.DeviceId && key.Type == dd.PointType)
            {
                xml.writeStartElement("Description");
                xml.writeAttribute("Address", QString::number(key.Address));
                xml.writeCDATA(description);
                xml.writeEndElement();
            }
        }

        xml.writeEndElement(); // AddressDescriptionMap
    }

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

    xml.writeEndElement(); // FormModSim

    return xml;
}

///
/// \brief operator >>
/// \param xml
/// \param frm
/// \return
///
inline QXmlStreamReader& operator >>(QXmlStreamReader& xml, FormModSim* frm)
{
    if (!frm) return xml;

    if (xml.isStartElement() && xml.name() == QLatin1String("FormModSim")) {
        DataDisplayMode ddm;
        DisplayDefinition dd;
        QHash<quint16, quint16> data;
        QHash<quint16, QString> descriptions;
        QHash<quint16, ModbusSimulationParams> simulations;

        const QXmlStreamAttributes attributes = xml.attributes();

        if (attributes.hasAttribute("DisplayMode")) {
            const DisplayMode mode = enumFromString<DisplayMode>(attributes.value("DisplayMode").toString());
            frm->setDisplayMode(mode);
        }

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
            else if (xml.name() == QLatin1String("DisplayDefinition")) {
                xml >> dd;
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
                auto scriptControl = frm->scriptControl();
                xml >> scriptControl;
            }
            else if (xml.name() == QLatin1String("ScriptSettings")) {
                ScriptSettings settings;
                xml >> settings;
                frm->setScriptSettings(settings);
            }
            else if (xml.name() == QLatin1String("AddressDescriptionMap")) {
                while (xml.readNextStartElement()) {
                    if (xml.name() == QLatin1String("Description")) {

                        const QXmlStreamAttributes attributes = xml.attributes();
                        bool ok; const quint16 address = attributes.value("Address").toUShort(&ok);

                        if(ok) {
                            QString description;
                            if (xml.isCDATA()) {
                                description = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                            } else {
                                description = xml.readElementText();
                            }
                            descriptions[address] = description;
                        }

                    } else {
                        xml.skipCurrentElement();
                    }
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
                            if(item->Mode != SimulationMode::No && item->Mode != SimulationMode::Toggle)
                                frm->startSimulation(dd.PointType, item.key() - (dd.ZeroBasedAddress ? 0 : 1), item.value());
                            break;
                        default: break;
                    }
                }
            }

            if(!descriptions.isEmpty()) {
                QHashIterator it(descriptions);
                while(it.hasNext()) {
                    const auto item = it.next();
                    frm->setDescription(dd.DeviceId, dd.PointType, item.key(), item.value());
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

#endif // FORMMODSIM_H
