#ifndef FORMMODSIM_H
#define FORMMODSIM_H

#include <QWidget>
#include <QTimer>
#include <QPrinter>
#include <QVersionNumber>
#include "fontutils.h"
#include "datasimulator.h"
#include "modbusmultiserver.h"
#include "displaydefinition.h"
#include "outputwidget.h"
#include "scriptcontrol.h"
#include "scriptsettings.h"

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

    ModbusSimulationMap simulationMap() const;
    void startSimulation(QModbusDataUnit::RegisterType type, quint16 addr, const ModbusSimulationParams& params);

    QModbusDataUnit serializeModbusDataUnit(QModbusDataUnit::RegisterType pointType,
                                            quint16 pointAddress,
                                            quint16 length) const;
    void configureModbusDataUnit(QModbusDataUnit::RegisterType type,
                                             quint16 startAddress,
                                             const QVector<quint16>& values) const;

    AddressDescriptionMap descriptionMap() const;
    void setDescription(QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc);

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
    void closeEvent(QCloseEvent *event) override;

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

private slots:
    void on_lineEditAddress_valueChanged(const QVariant&);
    void on_lineEditLength_valueChanged(const QVariant&);
    void on_lineEditDeviceId_valueChanged(const QVariant&);
    void on_comboBoxAddressBase_addressBaseChanged(AddressBase base);
    void on_comboBoxModbusPointType_pointTypeChanged(QModbusDataUnit::RegisterType value);
    void on_outputWidget_itemDoubleClicked(quint16 addr, const QVariant& value);
    void on_mbDeviceIdChanged(quint8 deviceId);
    void on_mbConnected(const ConnectionDetails& cd);
    void on_mbDisconnected(const ConnectionDetails& cd);
    void on_mbRequest(const QModbusRequest& req, ModbusMessage::ProtocolType protocol, int transactionId);
    void on_mbResponse(const QModbusRequest& req, const QModbusResponse& resp, ModbusMessage::ProtocolType protocol, int transactionId);
    void on_mbDataChanged(const QModbusDataUnit& data);
    void on_simulationStarted(QModbusDataUnit::RegisterType type, quint16 addr);
    void on_simulationStopped(QModbusDataUnit::RegisterType type, quint16 addr);
    void on_dataSimulated(DataDisplayMode mode, QModbusDataUnit::RegisterType type, quint16 addr, QVariant value);

private:
    void updateStatus();
    void onDefinitionChanged();
    ScriptControl* scriptControl();
    bool isLoggedRequest(const QModbusRequest& req, ModbusMessage::ProtocolType protocol) const;

private:
    Ui::FormModSim *ui;
    MainWindow* _parent;
    int _formId;
    QString _filename;
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
    out << dd.DeviceId;
    out << dd.PointType;
    out << dd.PointAddress;
    out << dd.Length;
    out << dd.LogViewLimit;
    out << dd.ZeroBasedAddress;

    out << frm->byteOrder();
    out << frm->simulationMap();
    out << frm->scriptControl();
    out << frm->scriptSettings();
    out << frm->descriptionMap();
    out << frm->codepage();

    const auto unit = frm->serializeModbusDataUnit(dd.PointType, dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1), dd.Length);
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

    ModbusSimulationMap simulationMap;
    ByteOrder byteOrder = ByteOrder::Direct;
    if(ver >= QVersionNumber(1, 1))
    {
        in >> byteOrder;
        in >> simulationMap;
    }

    ScriptSettings scriptSettings;
    if(ver >=  QVersionNumber(1, 2))
    {
        in >> frm->scriptControl();
        in >> scriptSettings;
    }

    AddressDescriptionMap descriptionMap;
    if(ver >=  QVersionNumber(1, 3))
    {
        in >> descriptionMap;
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

    for(auto&& k : simulationMap.keys())
        frm->startSimulation(k.first, k.second,  simulationMap[k]);

    for(auto&& k : descriptionMap.keys())
        frm->setDescription(k.first, k.second, descriptionMap[k]);

    if(ver >= QVersionNumber(1, 7))
    {
        QModbusDataUnit::RegisterType type;
        int startAddress;
        QVector<quint16> values;

        in >> type;
        in >> startAddress;
        in >> values;

        frm->configureModbusDataUnit(type, startAddress, values);
    }

    return in;
}

#endif // FORMMODSIM_H
